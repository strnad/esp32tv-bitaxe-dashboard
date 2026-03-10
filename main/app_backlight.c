/*
 * NM-TV-154 Backlight & Display Power Control
 */

#include "app_backlight.h"
#include "board_config.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "backlight";

#define BL_MAX_DUTY ((1 << BOARD_BL_LEDC_DUTY_RES) - 1)  /* 8191 for 13-bit */

esp_err_t backlight_init(void)
{
    /* --- Display power: GPIO 21 → LOW to enable --- */
    gpio_config_t pwr_cfg = {
        .pin_bit_mask = 1ULL << BOARD_LCD_PIN_PWR,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&pwr_cfg));
    gpio_set_level(BOARD_LCD_PIN_PWR, BOARD_LCD_PWR_ON_LEVEL);
    ESP_LOGI(TAG, "Display power enabled (GPIO %d → %d)", BOARD_LCD_PIN_PWR, BOARD_LCD_PWR_ON_LEVEL);

    /* --- Backlight PWM via LEDC --- */
    ledc_timer_config_t timer_cfg = {
        .speed_mode = BOARD_BL_LEDC_MODE,
        .duty_resolution = BOARD_BL_LEDC_DUTY_RES,
        .timer_num = BOARD_BL_LEDC_TIMER,
        .freq_hz = BOARD_BL_LEDC_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_cfg), TAG, "LEDC timer config failed");

    ledc_channel_config_t ch_cfg = {
        .speed_mode = BOARD_BL_LEDC_MODE,
        .channel = BOARD_BL_LEDC_CHANNEL,
        .timer_sel = BOARD_BL_LEDC_TIMER,
        .gpio_num = BOARD_LCD_PIN_BL,
        .duty = BL_MAX_DUTY,  /* Start OFF: active LOW, so max duty = pin HIGH = OFF */
        .hpoint = 0,
        .intr_type = LEDC_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&ch_cfg), TAG, "LEDC channel config failed");

    ESP_LOGI(TAG, "Backlight PWM initialized (GPIO %d, %d Hz)", BOARD_LCD_PIN_BL, BOARD_BL_LEDC_FREQ_HZ);
    return ESP_OK;
}

esp_err_t backlight_set_brightness(uint8_t percent)
{
    if (percent > 100) {
        percent = 100;
    }

    /*
     * Active LOW inversion:
     *   percent=100 → duty=0        (pin LOW  = full brightness)
     *   percent=0   → duty=BL_MAX   (pin HIGH = off)
     */
    uint32_t duty = BL_MAX_DUTY - (BL_MAX_DUTY * percent / 100);

    ESP_RETURN_ON_ERROR(ledc_set_duty(BOARD_BL_LEDC_MODE, BOARD_BL_LEDC_CHANNEL, duty), TAG, "set duty failed");
    ESP_RETURN_ON_ERROR(ledc_update_duty(BOARD_BL_LEDC_MODE, BOARD_BL_LEDC_CHANNEL), TAG, "update duty failed");

    ESP_LOGI(TAG, "Brightness set to %d%% (duty=%lu)", percent, (unsigned long)duty);
    return ESP_OK;
}

esp_err_t backlight_on(void)
{
    return backlight_set_brightness(100);
}

esp_err_t backlight_off(void)
{
    return backlight_set_brightness(0);
}
