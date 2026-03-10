/*
 * BitAxe Dashboard — Main Entry Point
 *
 * Initialization order:
 *   1. Backlight init (enables display power on GPIO 21)
 *   2. Display init (SPI bus, ST7789, LVGL)
 *   3. Turn on backlight
 *   4. Build dashboard UI, show splash
 *   5. Touch button init
 *   6. Common WiFi/NVS init
 *   7. Load config from NVS
 *   8. Check for config reset (long press during boot)
 *   9. Connect WiFi or start captive portal
 *  10. Start BitAxe polling task
 */

#include "app_backlight.h"
#include "app_config.h"
#include "app_display.h"
#include "app_portal.h"
#include "app_touch.h"
#include "app_wifi.h"
#include "bitaxe_api.h"
#include "ui_dashboard.h"
#include "board_config.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "main";

#define RESET_HOLD_MS       3000    /* Hold touch 3s at boot to reset config */
#define RESET_CHECK_MS      100     /* Polling interval during reset check */

/* Touch button callback — cycle between dashboard screens */
static void on_touch_pressed(void)
{
    display_lock();
    ui_dashboard_next_screen();
    display_unlock();
}

/* Polling task — periodically fetches data from BitAxe API */
static void bitaxe_poll_task(void *arg)
{
    bitaxe_data_t data;

    while (1) {
        if (bitaxe_api_fetch(&data) == ESP_OK) {
            display_lock();
            ui_dashboard_update(&data);
            display_unlock();
        } else {
            display_lock();
            ui_dashboard_show_error("BitAxe unreachable");
            display_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(CONFIG_BITAXE_POLL_INTERVAL_MS));
    }
}

/* Check if touch button is held during boot for config reset.
 * Shows countdown on display. Returns true if reset was triggered. */
static bool check_boot_reset(void)
{
    if (!touch_button_is_pressed()) {
        return false;
    }

    ESP_LOGI(TAG, "Touch detected at boot — hold %d ms to reset config", RESET_HOLD_MS);

    int held_ms = 0;
    while (held_ms < RESET_HOLD_MS) {
        if (!touch_button_is_pressed()) {
            ESP_LOGI(TAG, "Touch released, reset cancelled");
            return false;
        }

        int remaining_s = (RESET_HOLD_MS - held_ms + 999) / 1000;
        display_lock();
        ui_dashboard_show_reset_progress(remaining_s);
        display_unlock();

        vTaskDelay(pdMS_TO_TICKS(RESET_CHECK_MS));
        held_ms += RESET_CHECK_MS;
    }

    ESP_LOGW(TAG, "Config reset triggered!");
    config_reset(); /* Does not return — reboots */
    return true;
}

/* Determine if WiFi is unconfigured (default Kconfig placeholder + no NVS) */
static bool is_wifi_unconfigured(void)
{
    if (config_is_configured()) {
        return false;
    }
    /* If Kconfig SSID is empty or still the default placeholder, treat as unconfigured */
    const char *ssid = config_get_wifi_ssid();
    if (ssid[0] == '\0' || strcmp(ssid, "your-ssid") == 0) {
        return true;
    }
    return false;
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== BitAxe Dashboard ===");

    /* 1. Enable display power and initialize backlight PWM */
    ESP_ERROR_CHECK(backlight_init());

    /* 2. Initialize SPI, ST7789, and LVGL */
    ESP_ERROR_CHECK(display_init());

    /* 3. Turn on backlight */
    ESP_ERROR_CHECK(backlight_set_brightness(CONFIG_BITAXE_BACKLIGHT_PERCENT));

    /* 4. Build dashboard UI and show connecting splash */
    display_lock();
    ui_dashboard_init(display_get_handle());
    ui_dashboard_show_connecting();
    display_unlock();

    /* 5. Initialize touch button */
    ESP_ERROR_CHECK(touch_button_init());

    /* 6. Initialize NVS, netif, event loop (shared by STA and AP modes) */
    ESP_ERROR_CHECK(wifi_common_init());

    /* 7. Load config from NVS */
    ESP_ERROR_CHECK(config_init());

    /* 8. Check for config reset (long press at boot) */
    vTaskDelay(pdMS_TO_TICKS(500)); /* Brief window for touch detection */
    check_boot_reset();

    /* 9. Connect WiFi or start captive portal */
    if (is_wifi_unconfigured()) {
        ESP_LOGI(TAG, "WiFi not configured — starting setup portal");
        display_lock();
        ui_dashboard_show_setup_mode();
        display_unlock();

        ESP_ERROR_CHECK(portal_start());

        /* Register touch callback for screen nav even in portal mode */
        touch_button_register_callback(on_touch_pressed);

        /* Block forever — portal handles save + reboot */
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    /* Normal mode — connect to WiFi */
    const char *ssid = config_get_wifi_ssid();
    const char *pass = config_get_wifi_pass();

    ESP_LOGI(TAG, "Connecting to WiFi '%s'...", ssid);
    esp_err_t wifi_ret = wifi_connect_sta(ssid, pass);

    if (wifi_ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi connection failed");
        display_lock();
        ui_dashboard_show_error("WiFi failed - check config");
        display_unlock();

        /* If we have NVS config, let user reset via reboot + long press */
        touch_button_register_callback(on_touch_pressed);
        return;
    }

    /* 10. Initialize BitAxe API client and start polling */
    ESP_ERROR_CHECK(bitaxe_api_init(config_get_api_host()));

    touch_button_register_callback(on_touch_pressed);

    xTaskCreate(bitaxe_poll_task, "bitaxe_poll",
                BOARD_POLL_TASK_STACK_SIZE, NULL,
                BOARD_POLL_TASK_PRIORITY, NULL);

    ESP_LOGI(TAG, "Dashboard running. Polling every %d ms.", CONFIG_BITAXE_POLL_INTERVAL_MS);
}
