/*
 * Captive Portal — WiFi AP + DNS redirect + HTTP config page
 *
 * Starts a WiFi access point with a web-based configuration form.
 * DNS redirects all domains to the AP IP so phones auto-open the page.
 */

#pragma once

#include "esp_err.h"

/* Start the captive portal (AP + DNS + HTTP server).
 * This function returns immediately; servers run in background tasks.
 * Call after esp_netif_init() and esp_event_loop_create_default(). */
esp_err_t portal_start(void);

/* Stop the captive portal and release resources. */
void portal_stop(void);
