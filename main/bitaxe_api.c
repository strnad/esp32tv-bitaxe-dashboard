/*
 * BitAxe API Client — HTTP fetch + cJSON parser
 */

#include "bitaxe_api.h"

#include <string.h>
#include <stdio.h>
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "bitaxe_api";

#define API_RESPONSE_BUF_SIZE  4096
#define API_TIMEOUT_MS         5000

static char s_url[128];
static char s_response_buf[API_RESPONSE_BUF_SIZE];

esp_err_t bitaxe_api_init(const char *host)
{
    snprintf(s_url, sizeof(s_url), "http://%s/api/system/info", host);
    ESP_LOGI(TAG, "BitAxe API URL: %s", s_url);
    return ESP_OK;
}

/* Helper: safely copy a cJSON string field */
static void json_get_string(const cJSON *root, const char *key, char *dst, size_t dst_size)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (cJSON_IsString(item) && item->valuestring) {
        strncpy(dst, item->valuestring, dst_size - 1);
        dst[dst_size - 1] = '\0';
    }
}

/* Helper: safely get a cJSON int field */
static int json_get_int(const cJSON *root, const char *key, int fallback)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (cJSON_IsNumber(item)) {
        return item->valueint;
    }
    return fallback;
}

/* Helper: safely get a cJSON double/float field */
static float json_get_float(const cJSON *root, const char *key, float fallback)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (cJSON_IsNumber(item)) {
        return (float)item->valuedouble;
    }
    return fallback;
}

esp_err_t bitaxe_api_fetch(bitaxe_data_t *out)
{
    memset(out, 0, sizeof(*out));

    esp_http_client_config_t config = {
        .url = s_url,
        .timeout_ms = API_TIMEOUT_MS,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP open failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    int content_length = esp_http_client_fetch_headers(client);
    int status = esp_http_client_get_status_code(client);

    if (status != 200) {
        ESP_LOGE(TAG, "HTTP status %d", status);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    /* Read response body */
    int read_len = esp_http_client_read(client, s_response_buf, API_RESPONSE_BUF_SIZE - 1);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (read_len <= 0) {
        ESP_LOGE(TAG, "No data received (content_length=%d)", content_length);
        return ESP_FAIL;
    }
    s_response_buf[read_len] = '\0';

    /* Parse JSON */
    cJSON *root = cJSON_Parse(s_response_buf);
    if (!root) {
        ESP_LOGE(TAG, "JSON parse failed");
        return ESP_FAIL;
    }

    /* Mining */
    out->hash_rate = json_get_float(root, "hashRate", 0.0f);
    out->hash_rate_1m = json_get_float(root, "hashRate_1m", 0.0f);
    out->hash_rate_10m = json_get_float(root, "hashRate_10m", 0.0f);
    out->hash_rate_1h = json_get_float(root, "hashRate_1h", 0.0f);
    out->expected_hashrate = json_get_float(root, "expectedHashrate", 0.0f);
    out->error_percentage = json_get_float(root, "errorPercentage", 0.0f);
    out->shares_accepted = json_get_int(root, "sharesAccepted", 0);
    out->shares_rejected = json_get_int(root, "sharesRejected", 0);
    out->frequency = json_get_int(root, "frequency", 0);

    /* bestDiff can be string or number depending on firmware version */
    const cJSON *best_diff = cJSON_GetObjectItemCaseSensitive(root, "bestDiff");
    if (cJSON_IsString(best_diff) && best_diff->valuestring) {
        strncpy(out->best_diff, best_diff->valuestring, sizeof(out->best_diff) - 1);
    } else if (cJSON_IsNumber(best_diff)) {
        snprintf(out->best_diff, sizeof(out->best_diff), "%.0f", best_diff->valuedouble);
    }

    /* Pool */
    out->pool_difficulty = json_get_float(root, "poolDifficulty", 0.0f);
    const cJSON *fb = cJSON_GetObjectItemCaseSensitive(root, "isUsingFallbackStratum");
    out->is_using_fallback = cJSON_IsTrue(fb);

    /* Thermal */
    out->temp = json_get_float(root, "temp", 0.0f);
    out->vr_temp = json_get_float(root, "vrTemp", 0.0f);
    out->temp2 = json_get_float(root, "temp2", 0.0f);

    /* Power */
    out->power = json_get_float(root, "power", 0.0f);
    out->voltage = json_get_float(root, "voltage", 0.0f);
    out->current = json_get_float(root, "current", 0.0f);
    out->core_voltage = json_get_float(root, "coreVoltageActual", 0.0f);
    out->core_voltage_set = json_get_float(root, "coreVoltage", 0.0f);
    out->fan_speed = json_get_int(root, "fanspeed", 0);

    /* System */
    out->uptime_seconds = json_get_int(root, "uptimeSeconds", 0);
    json_get_string(root, "ASICModel", out->asic_model, sizeof(out->asic_model));
    json_get_string(root, "hostname", out->hostname, sizeof(out->hostname));
    json_get_string(root, "version", out->version, sizeof(out->version));
    json_get_string(root, "axeOSVersion", out->axe_os_version, sizeof(out->axe_os_version));
    json_get_string(root, "stratumURL", out->stratum_url, sizeof(out->stratum_url));
    json_get_string(root, "stratumUser", out->stratum_user, sizeof(out->stratum_user));
    json_get_string(root, "ssid", out->ssid, sizeof(out->ssid));
    out->wifi_rssi = json_get_int(root, "wifiRSSI", 0);
    json_get_string(root, "wifiStatus", out->wifi_status, sizeof(out->wifi_status));
    json_get_string(root, "macAddr", out->mac_addr, sizeof(out->mac_addr));
    json_get_string(root, "ipv4", out->ip_addr, sizeof(out->ip_addr));

    /* Block */
    out->block_found = json_get_int(root, "blockFound", 0);
    const cJSON *snb = cJSON_GetObjectItemCaseSensitive(root, "showNewBlock");
    out->show_new_block = cJSON_IsTrue(snb);

    cJSON_Delete(root);

    out->valid = true;
    ESP_LOGI(TAG, "Fetched: %.1f GH/s, %.1fC, %dW",
             out->hash_rate, out->temp, (int)out->power);
    return ESP_OK;
}
