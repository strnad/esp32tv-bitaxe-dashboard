/*
 * NM-TV-154 Capacitive Touch Button
 *
 * ESP32 classic uses the legacy touch sensor API (v1).
 * A polling task reads the filtered touch value and calls the registered
 * callback on debounced press events.
 *
 * HARDWARE NOTE: The top touch button is on T9 (GPIO 32).
 * Standard ESP32 behavior — value drops on touch:
 *   Not touching: ~797    Touching: ~570
 * Confirmed via on-device diagnostic with live display readout.
 */

#include "app_touch.h"
#include "board_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/touch_pad.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "touch";

#define TOUCH_FILTER_PERIOD_MS  10
#define DEBOUNCE_MS             50
#define POLL_INTERVAL_MS        20

static volatile bool s_pressed = false;
static touch_callback_t s_callback = NULL;

static void touch_poll_task(void *arg)
{
    uint16_t value = 0;
    bool was_pressed = false;
    TickType_t press_start = 0;

    ESP_LOGI(TAG, "Touch polling task started (pad T%d, threshold=%d)",
             BOARD_TOUCH_PAD_NUM, BOARD_TOUCH_THRESHOLD);

    while (1) {
        touch_pad_read_filtered(BOARD_TOUCH_PAD_NUM, &value);

        /* Standard: value drops below threshold when touched */
        bool touching = (value < BOARD_TOUCH_THRESHOLD);

        if (touching && !was_pressed) {
            press_start = xTaskGetTickCount();
            was_pressed = true;
        } else if (touching && was_pressed) {
            if (!s_pressed && (xTaskGetTickCount() - press_start) >= pdMS_TO_TICKS(DEBOUNCE_MS)) {
                s_pressed = true;
                ESP_LOGI(TAG, "Touch pressed (value=%u)", value);
                if (s_callback) {
                    s_callback();
                }
            }
        } else {
            was_pressed = false;
            s_pressed = false;
        }

        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
}

esp_err_t touch_button_init(void)
{
    ESP_LOGI(TAG, "Initializing touch pad T%d (GPIO 32)", BOARD_TOUCH_PAD_NUM);

    ESP_RETURN_ON_ERROR(touch_pad_init(), TAG, "touch_pad_init failed");
    ESP_RETURN_ON_ERROR(touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER), TAG, "set FSM mode failed");

    ESP_RETURN_ON_ERROR(touch_pad_config(BOARD_TOUCH_PAD_NUM, 0), TAG, "touch_pad_config failed");

    ESP_RETURN_ON_ERROR(touch_pad_filter_start(TOUCH_FILTER_PERIOD_MS), TAG, "filter start failed");

    vTaskDelay(pdMS_TO_TICKS(200));

    uint16_t initial_value = 0;
    touch_pad_read_filtered(BOARD_TOUCH_PAD_NUM, &initial_value);
    ESP_LOGI(TAG, "Touch pad T%d baseline: %u (threshold: %d)",
             BOARD_TOUCH_PAD_NUM, initial_value, BOARD_TOUCH_THRESHOLD);

    xTaskCreate(touch_poll_task, "touch", 4096, NULL, 3, NULL);
    return ESP_OK;
}

bool touch_button_is_pressed(void)
{
    return s_pressed;
}

void touch_button_register_callback(touch_callback_t cb)
{
    s_callback = cb;
}
