/*
 * NM-TV-154 Capacitive Touch Button
 *
 * Uses the ESP32 built-in touch_pad peripheral.
 * The touch pad number is configurable via menuconfig (default T0 = GPIO 4).
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

typedef void (*touch_callback_t)(void);

/* Initialize the capacitive touch pad with filtering and start the polling task. */
esp_err_t touch_button_init(void);

/* Returns true if the touch button is currently pressed. */
bool touch_button_is_pressed(void);

/* Register a callback invoked on each touch press (debounced). Set NULL to clear. */
void touch_button_register_callback(touch_callback_t cb);
