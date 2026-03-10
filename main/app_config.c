/*
 * Runtime Configuration Store (NVS)
 */

#include "app_config.h"

#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_system.h"

static const char *TAG = "config";

#define NVS_NAMESPACE   "config"
#define KEY_WIFI_SSID   "wifi_ssid"
#define KEY_WIFI_PASS   "wifi_pass"
#define KEY_API_HOST    "api_host"
#define KEY_CONFIGURED  "configured"

static char s_wifi_ssid[33];
static char s_wifi_pass[65];
static char s_api_host[128];
static bool s_configured = false;

esp_err_t config_init(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No saved config, using Kconfig defaults");
        strncpy(s_wifi_ssid, CONFIG_BITAXE_WIFI_SSID, sizeof(s_wifi_ssid) - 1);
        strncpy(s_wifi_pass, CONFIG_BITAXE_WIFI_PASSWORD, sizeof(s_wifi_pass) - 1);
        strncpy(s_api_host, CONFIG_BITAXE_API_HOST, sizeof(s_api_host) - 1);
        s_configured = false;
        return ESP_OK;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t flag = 0;
    err = nvs_get_u8(handle, KEY_CONFIGURED, &flag);
    s_configured = (err == ESP_OK && flag == 1);

    if (s_configured) {
        size_t len;

        len = sizeof(s_wifi_ssid);
        if (nvs_get_str(handle, KEY_WIFI_SSID, s_wifi_ssid, &len) != ESP_OK) {
            strncpy(s_wifi_ssid, CONFIG_BITAXE_WIFI_SSID, sizeof(s_wifi_ssid) - 1);
        }

        len = sizeof(s_wifi_pass);
        if (nvs_get_str(handle, KEY_WIFI_PASS, s_wifi_pass, &len) != ESP_OK) {
            strncpy(s_wifi_pass, CONFIG_BITAXE_WIFI_PASSWORD, sizeof(s_wifi_pass) - 1);
        }

        len = sizeof(s_api_host);
        if (nvs_get_str(handle, KEY_API_HOST, s_api_host, &len) != ESP_OK) {
            strncpy(s_api_host, CONFIG_BITAXE_API_HOST, sizeof(s_api_host) - 1);
        }

        ESP_LOGI(TAG, "Loaded NVS config: SSID='%s', host='%s'", s_wifi_ssid, s_api_host);
    } else {
        ESP_LOGI(TAG, "NVS exists but not configured, using Kconfig defaults");
        strncpy(s_wifi_ssid, CONFIG_BITAXE_WIFI_SSID, sizeof(s_wifi_ssid) - 1);
        strncpy(s_wifi_pass, CONFIG_BITAXE_WIFI_PASSWORD, sizeof(s_wifi_pass) - 1);
        strncpy(s_api_host, CONFIG_BITAXE_API_HOST, sizeof(s_api_host) - 1);
    }

    nvs_close(handle);
    return ESP_OK;
}

bool config_is_configured(void)
{
    return s_configured;
}

const char *config_get_wifi_ssid(void)
{
    return s_wifi_ssid;
}

const char *config_get_wifi_pass(void)
{
    return s_wifi_pass;
}

const char *config_get_api_host(void)
{
    return s_api_host;
}

esp_err_t config_set_wifi(const char *ssid, const char *pass)
{
    nvs_handle_t handle;
    ESP_RETURN_ON_ERROR(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle), TAG, "nvs_open");

    ESP_RETURN_ON_ERROR(nvs_set_str(handle, KEY_WIFI_SSID, ssid), TAG, "set ssid");
    ESP_RETURN_ON_ERROR(nvs_set_str(handle, KEY_WIFI_PASS, pass), TAG, "set pass");
    ESP_RETURN_ON_ERROR(nvs_set_u8(handle, KEY_CONFIGURED, 1), TAG, "set flag");
    ESP_RETURN_ON_ERROR(nvs_commit(handle), TAG, "commit");

    nvs_close(handle);

    strncpy(s_wifi_ssid, ssid, sizeof(s_wifi_ssid) - 1);
    s_wifi_ssid[sizeof(s_wifi_ssid) - 1] = '\0';
    strncpy(s_wifi_pass, pass, sizeof(s_wifi_pass) - 1);
    s_wifi_pass[sizeof(s_wifi_pass) - 1] = '\0';
    s_configured = true;

    ESP_LOGI(TAG, "WiFi config saved: SSID='%s'", ssid);
    return ESP_OK;
}

esp_err_t config_set_api_host(const char *host)
{
    nvs_handle_t handle;
    ESP_RETURN_ON_ERROR(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle), TAG, "nvs_open");

    ESP_RETURN_ON_ERROR(nvs_set_str(handle, KEY_API_HOST, host), TAG, "set host");
    ESP_RETURN_ON_ERROR(nvs_set_u8(handle, KEY_CONFIGURED, 1), TAG, "set flag");
    ESP_RETURN_ON_ERROR(nvs_commit(handle), TAG, "commit");

    nvs_close(handle);

    strncpy(s_api_host, host, sizeof(s_api_host) - 1);
    s_api_host[sizeof(s_api_host) - 1] = '\0';
    s_configured = true;

    ESP_LOGI(TAG, "API host saved: '%s'", host);
    return ESP_OK;
}

void config_reset(void)
{
    ESP_LOGW(TAG, "Erasing config and rebooting...");

    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_erase_all(handle);
        nvs_commit(handle);
        nvs_close(handle);
    }

    esp_restart();
}
