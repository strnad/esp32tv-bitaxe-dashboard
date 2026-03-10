/*
 * NM-TV-154 Display Subsystem
 *
 * Initializes SPI bus, ST7789 panel driver, and LVGL.
 * Call backlight_init() BEFORE display_init().
 *
 * LVGL is NOT thread-safe. Use display_lock()/display_unlock()
 * around all lv_* calls from outside the LVGL task.
 */

#pragma once

#include "esp_err.h"
#include "lvgl.h"

/* Initialize SPI bus, ST7789 panel, LVGL library, tick timer, and LVGL task. */
esp_err_t display_init(void);

/* Get the LVGL display handle for creating UI elements. */
lv_display_t *display_get_handle(void);

/* Acquire the LVGL API mutex. Must be held for any lv_* call from non-LVGL tasks. */
void display_lock(void);

/* Release the LVGL API mutex. */
void display_unlock(void);
