/*
 * Runtime Configuration Store (NVS)
 *
 * Stores WiFi credentials and miner address in NVS flash.
 * Falls back to Kconfig defaults when NVS is not configured.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

/* Initialize NVS and load configuration.
 * Must be called after nvs_flash_init(). */
esp_err_t config_init(void);

/* Returns true if user has saved settings via the captive portal. */
bool config_is_configured(void);

/* Getters — return NVS values if configured, otherwise Kconfig defaults. */
const char *config_get_wifi_ssid(void);
const char *config_get_wifi_pass(void);
const char *config_get_api_host(void);

/* Save WiFi credentials to NVS. */
esp_err_t config_set_wifi(const char *ssid, const char *pass);

/* Save miner API host to NVS. */
esp_err_t config_set_api_host(const char *host);

/* Erase all config from NVS and reboot. Does not return. */
void config_reset(void);
