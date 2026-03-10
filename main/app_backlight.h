/*
 * NM-TV-154 Backlight & Display Power Control
 *
 * Backlight on GPIO 19 is ACTIVE LOW — PWM duty is inverted internally.
 * Display power on GPIO 21 is ACTIVE LOW — driven LOW to enable.
 *
 * Call backlight_init() BEFORE display_init() because the display
 * power pin must be driven LOW before any SPI communication.
 */

#pragma once

#include "esp_err.h"

/* Initialize display power (GPIO 21 → LOW) and backlight PWM on GPIO 19.
 * Backlight starts OFF until backlight_set_brightness() is called. */
esp_err_t backlight_init(void);

/* Set backlight brightness 0–100%. Handles active-LOW inversion internally. */
esp_err_t backlight_set_brightness(uint8_t percent);

/* Convenience: full brightness */
esp_err_t backlight_on(void);

/* Convenience: backlight off */
esp_err_t backlight_off(void);
