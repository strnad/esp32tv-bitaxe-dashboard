/*
 * WiFi STA Connection
 *
 * Connects to a WiFi network with auto-reconnect.
 * SSID and password are provided at runtime (from NVS config or Kconfig).
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

/* Initialize NVS flash, netif, and event loop.
 * Must be called once before wifi_connect_sta() or portal_start(). */
esp_err_t wifi_common_init(void);

/* Connect to a WiFi network in STA mode.
 * Blocks until connected or timeout (30s).
 * Call wifi_common_init() first. */
esp_err_t wifi_connect_sta(const char *ssid, const char *password);

/* Returns true if WiFi STA is currently connected. */
bool wifi_is_connected(void);
