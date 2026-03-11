/*
 * NM-TV-154 Display Subsystem
 *
 * SPI bus → ST7789 panel IO → ST7789 panel driver → LVGL
 * Based on the ESP-IDF spi_lcd_touch example pattern.
 */

#include "app_display.h"
#include "board_config.h"

#include <sys/lock.h>
#include <sys/param.h>
#include <unistd.h>

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_timer.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "display";

static lv_display_t *s_display = NULL;
static _lock_t s_lvgl_api_lock;

/* ------------------------------------------------------------------ */
/*  LVGL flush callback — sends pixels to the ST7789 via DMA          */
/* ------------------------------------------------------------------ */
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel = lv_display_get_user_data(disp);
    int x1 = area->x1;
    int x2 = area->x2;
    int y1 = area->y1;
    int y2 = area->y2;

    /* SPI LCD is big-endian, LVGL native is little-endian — swap bytes */
    lv_draw_sw_rgb565_swap(px_map, (x2 + 1 - x1) * (y2 + 1 - y1));
    esp_lcd_panel_draw_bitmap(panel, x1, y1, x2 + 1, y2 + 1, px_map);
}

/* Called by the SPI driver when DMA transfer completes */
static bool on_color_trans_done(esp_lcd_panel_io_handle_t io,
                                esp_lcd_panel_io_event_data_t *edata,
                                void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

/* LVGL tick callback — called every BOARD_LVGL_TICK_PERIOD_MS ms */
static void lvgl_tick_cb(void *arg)
{
    lv_tick_inc(BOARD_LVGL_TICK_PERIOD_MS);
}

/* LVGL task — runs lv_timer_handler() in a loop */
static void lvgl_task(void *arg)
{
    ESP_LOGI(TAG, "LVGL task started");
    uint32_t delay_ms = 0;
    while (1) {
        _lock_acquire(&s_lvgl_api_lock);
        delay_ms = lv_timer_handler();
        _lock_release(&s_lvgl_api_lock);
        delay_ms = MAX(delay_ms, BOARD_LVGL_TASK_MIN_DELAY_MS);
        delay_ms = MIN(delay_ms, BOARD_LVGL_TASK_MAX_DELAY_MS);
        usleep(1000 * delay_ms);
    }
}

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

esp_err_t display_init(void)
{
    /* --- Step 1: Initialize SPI bus --- */
    ESP_LOGI(TAG, "Initializing SPI bus (SPI2_HOST)");
    spi_bus_config_t bus_cfg = {
        .sclk_io_num = BOARD_LCD_PIN_SCLK,
        .mosi_io_num = BOARD_LCD_PIN_MOSI,
        .miso_io_num = BOARD_LCD_PIN_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = BOARD_LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(
        spi_bus_initialize(BOARD_LCD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO),
        TAG, "SPI bus init failed");

    /* --- Step 2: Create panel IO (SPI device) --- */
    ESP_LOGI(TAG, "Installing panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = BOARD_LCD_PIN_DC,
        .cs_gpio_num = BOARD_LCD_PIN_CS,
        .pclk_hz = BOARD_LCD_SPI_FREQ_HZ,
        .lcd_cmd_bits = BOARD_LCD_CMD_BITS,
        .lcd_param_bits = BOARD_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BOARD_LCD_SPI_HOST, &io_cfg, &io_handle),
        TAG, "Panel IO init failed");

    /* --- Step 3: Create ST7789 panel driver --- */
    ESP_LOGI(TAG, "Installing ST7789 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = BOARD_LCD_PIN_RST,    /* -1: SW reset */
        .rgb_ele_order = BOARD_LCD_COLOR_ORDER,  /* RGB — viz board_config.h */
        .bits_per_pixel = BOARD_LCD_BITS_PER_PIXEL,
    };
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_st7789(io_handle, &panel_cfg, &panel_handle),
        TAG, "ST7789 panel init failed");

    /* --- Step 4: Reset and initialize panel --- */
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    /* ST7789 with 240x240 needs color inversion for correct colors */
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

    /* Turn on the display */
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "ST7789 panel ready (%dx%d)", BOARD_LCD_H_RES, BOARD_LCD_V_RES);

    /* --- Step 5: Initialize LVGL --- */
    ESP_LOGI(TAG, "Initializing LVGL");
    lv_init();

    s_display = lv_display_create(BOARD_LCD_H_RES, BOARD_LCD_V_RES);

    /* Allocate two DMA-capable draw buffers */
    size_t buf_sz = BOARD_LCD_H_RES * BOARD_LVGL_DRAW_BUF_LINES * sizeof(lv_color16_t);
    void *buf1 = spi_bus_dma_memory_alloc(BOARD_LCD_SPI_HOST, buf_sz, 0);
    void *buf2 = spi_bus_dma_memory_alloc(BOARD_LCD_SPI_HOST, buf_sz, 0);
    assert(buf1 && buf2);

    lv_display_set_buffers(s_display, buf1, buf2, buf_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(s_display, panel_handle);
    lv_display_set_color_format(s_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(s_display, lvgl_flush_cb);

    /* --- Step 6: Register DMA done callback for non-blocking flush --- */
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = on_color_trans_done,
    };
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, s_display));

    /* --- Step 7: LVGL tick timer (2ms periodic) --- */
    const esp_timer_create_args_t tick_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    esp_timer_handle_t tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, BOARD_LVGL_TICK_PERIOD_MS * 1000));

    /* --- Step 8: LVGL task --- */
    xTaskCreate(lvgl_task, "lvgl", BOARD_LVGL_TASK_STACK_SIZE, NULL, BOARD_LVGL_TASK_PRIORITY, NULL);

    ESP_LOGI(TAG, "LVGL initialized");
    return ESP_OK;
}

lv_display_t *display_get_handle(void)
{
    return s_display;
}

void display_lock(void)
{
    _lock_acquire(&s_lvgl_api_lock);
}

void display_unlock(void)
{
    _lock_release(&s_lvgl_api_lock);
}
