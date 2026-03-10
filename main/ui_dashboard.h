/*
 * BitAxe Dashboard UI
 *
 * Five-screen dashboard for 240x240 display:
 *   Screen 1: Main dashboard (hashrate, temp, power, shares, diff)
 *   Screen 2: Performance (rolling averages, efficiency, errors)
 *   Screen 3: Thermal & Power (temps, watts, J/TH, fan, voltage)
 *   Screen 4: Mining Stats (shares, difficulty, pool status)
 *   Screen 5: Network & System (pool, WiFi, IP, firmware)
 *
 * Touch button cycles between screens.
 * All functions must be called with display_lock() held.
 */

#pragma once

#include "lvgl.h"
#include "bitaxe_api.h"

/* Build the dashboard UI on the active display. */
void ui_dashboard_init(lv_display_t *disp);

/* Update all dashboard screens with fresh data. */
void ui_dashboard_update(const bitaxe_data_t *data);

/* Show a "Connecting to WiFi..." splash screen. */
void ui_dashboard_show_connecting(void);

/* Show an error message on screen. */
void ui_dashboard_show_error(const char *msg);

/* Cycle to the next screen (called on touch press). */
void ui_dashboard_next_screen(void);

/* Show the WiFi setup mode screen (captive portal instructions). */
void ui_dashboard_show_setup_mode(void);

/* Show config reset countdown overlay (seconds remaining). */
void ui_dashboard_show_reset_progress(int seconds_remaining);
