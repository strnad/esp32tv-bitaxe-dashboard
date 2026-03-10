/*
 * Captive Portal — WiFi AP + DNS redirect + HTTP config page
 */

#include "app_portal.h"
#include "app_config.h"

#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "portal";

#define AP_SSID         "Bitaxe-Dashboard-Setup"
#define AP_CHANNEL      1
#define AP_MAX_CONN     4
#define DNS_PORT        53
#define AP_IP           "192.168.4.1"

static httpd_handle_t s_httpd = NULL;
static TaskHandle_t s_dns_task = NULL;
static int s_dns_sock = -1;

/* ------------------------------------------------------------------ */
/*  HTML config page (embedded)                                        */
/* ------------------------------------------------------------------ */

static const char CONFIG_PAGE_HTML[] =
"<!DOCTYPE html>"
"<html><head>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>Bitaxe Dashboard Setup</title>"
"<style>"
"*{box-sizing:border-box;margin:0;padding:0}"
"body{font-family:-apple-system,system-ui,sans-serif;background:#111;color:#e0e0e0;"
"min-height:100vh;display:flex;align-items:center;justify-content:center;padding:16px}"
".card{background:#1a1a1a;border:1px solid #333;border-radius:12px;padding:24px;"
"width:100%;max-width:380px}"
"h1{font-size:20px;text-align:center;margin-bottom:20px;color:#60a5fa}"
"label{display:block;font-size:13px;color:#999;margin-bottom:4px;margin-top:14px}"
"input{width:100%;padding:10px 12px;background:#222;border:1px solid #444;"
"border-radius:6px;color:#fff;font-size:15px;outline:none}"
"input:focus{border-color:#60a5fa}"
"button{width:100%;padding:12px;margin-top:20px;background:#60a5fa;color:#000;"
"border:none;border-radius:6px;font-size:16px;font-weight:600;cursor:pointer}"
"button:active{background:#3b82f6}"
".note{text-align:center;font-size:12px;color:#666;margin-top:12px}"
"</style></head><body>"
"<div class='card'>"
"<h1>Bitaxe Dashboard Setup</h1>"
"<form method='POST' action='/save'>"
"<label>WiFi SSID</label>"
"<input name='ssid' required maxlength='32' placeholder='Your WiFi network'>"
"<label>WiFi Password</label>"
"<input name='pass' type='password' maxlength='64' placeholder='WiFi password'>"
"<label>Miner IP Address</label>"
"<input name='host' required maxlength='64' placeholder='e.g. 192.168.1.50' value='" CONFIG_BITAXE_API_HOST "'>"
"<button type='submit'>Save &amp; Restart</button>"
"</form>"
"<p class='note'>Device will restart and connect to your WiFi.</p>"
"</div></body></html>";

static const char SAVE_OK_HTML[] =
"<!DOCTYPE html>"
"<html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>Saved</title>"
"<style>body{font-family:sans-serif;background:#111;color:#e0e0e0;"
"display:flex;align-items:center;justify-content:center;min-height:100vh}"
"</style></head><body>"
"<div style='text-align:center'>"
"<h2 style='color:#4ade80'>Settings Saved!</h2>"
"<p style='margin-top:12px;color:#999'>Restarting device...</p>"
"</div></body></html>";

/* ------------------------------------------------------------------ */
/*  URL-decode helper                                                  */
/* ------------------------------------------------------------------ */

static void url_decode(char *dst, const char *src, size_t dst_size)
{
    size_t di = 0;
    while (*src && di < dst_size - 1) {
        if (*src == '%' && src[1] && src[2]) {
            char hex[3] = { src[1], src[2], 0 };
            dst[di++] = (char)strtol(hex, NULL, 16);
            src += 3;
        } else if (*src == '+') {
            dst[di++] = ' ';
            src++;
        } else {
            dst[di++] = *src++;
        }
    }
    dst[di] = '\0';
}

/* Extract a form field value from URL-encoded POST body */
static bool form_get_field(const char *body, const char *name, char *out, size_t out_size)
{
    char search[64];
    snprintf(search, sizeof(search), "%s=", name);
    const char *start = strstr(body, search);
    if (!start) return false;
    start += strlen(search);

    const char *end = strchr(start, '&');
    size_t len = end ? (size_t)(end - start) : strlen(start);

    /* Temporary buffer for encoded value */
    char encoded[256];
    if (len >= sizeof(encoded)) len = sizeof(encoded) - 1;
    memcpy(encoded, start, len);
    encoded[len] = '\0';

    url_decode(out, encoded, out_size);
    return true;
}

/* ------------------------------------------------------------------ */
/*  HTTP handlers                                                      */
/* ------------------------------------------------------------------ */

static esp_err_t handler_root(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, CONFIG_PAGE_HTML, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t handler_save(httpd_req_t *req)
{
    char body[512];
    int received = httpd_req_recv(req, body, sizeof(body) - 1);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data");
        return ESP_FAIL;
    }
    body[received] = '\0';

    char ssid[33] = {0};
    char pass[65] = {0};
    char host[128] = {0};

    if (!form_get_field(body, "ssid", ssid, sizeof(ssid)) || strlen(ssid) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID required");
        return ESP_FAIL;
    }
    form_get_field(body, "pass", pass, sizeof(pass));
    if (!form_get_field(body, "host", host, sizeof(host)) || strlen(host) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Host required");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Saving config: SSID='%s', host='%s'", ssid, host);

    config_set_wifi(ssid, pass);
    config_set_api_host(host);

    /* Send success page, then reboot after a short delay */
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, SAVE_OK_HTML, HTTPD_RESP_USE_STRLEN);

    vTaskDelay(pdMS_TO_TICKS(1500));
    esp_restart();

    return ESP_OK; /* unreachable */
}

/* Captive portal detection endpoints — redirect to root */
static esp_err_t handler_redirect(httpd_req_t *req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://" AP_IP "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

/* ------------------------------------------------------------------ */
/*  DNS server — resolves ALL queries to AP_IP                         */
/* ------------------------------------------------------------------ */

/* Minimal DNS response: copy query, set response flags, append A record */
static void dns_task(void *arg)
{
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(DNS_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    s_dns_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_dns_sock < 0) {
        ESP_LOGE(TAG, "DNS socket create failed");
        vTaskDelete(NULL);
        return;
    }

    if (bind(s_dns_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "DNS socket bind failed");
        close(s_dns_sock);
        s_dns_sock = -1;
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "DNS server started on port %d", DNS_PORT);

    uint8_t buf[512];
    struct sockaddr_in client_addr;
    socklen_t addr_len;

    /* AP IP as 4 bytes */
    uint8_t ap_ip[4] = {192, 168, 4, 1};

    while (1) {
        addr_len = sizeof(client_addr);
        int len = recvfrom(s_dns_sock, buf, sizeof(buf), 0,
                           (struct sockaddr *)&client_addr, &addr_len);
        if (len < 12) continue; /* too short for DNS header */

        /* Build response in-place */
        buf[2] = 0x81; /* QR=1, Opcode=0, AA=1, TC=0, RD=0 */
        buf[3] = 0x80; /* RA=1, Z=0, RCODE=0 (no error) */
        /* ANCOUNT = 1 */
        buf[6] = 0x00;
        buf[7] = 0x01;

        /* Find end of question section (skip QNAME + QTYPE + QCLASS) */
        int qname_end = 12;
        while (qname_end < len && buf[qname_end] != 0) {
            qname_end += buf[qname_end] + 1;
        }
        qname_end++; /* skip null terminator */
        int question_end = qname_end + 4; /* QTYPE(2) + QCLASS(2) */

        if (question_end > len) continue;

        /* Append answer: pointer to QNAME + TYPE A + CLASS IN + TTL + RDLENGTH + IP */
        int pos = question_end;
        if (pos + 16 > (int)sizeof(buf)) continue;

        buf[pos++] = 0xC0; /* name pointer */
        buf[pos++] = 0x0C; /* offset 12 = start of QNAME */
        buf[pos++] = 0x00; buf[pos++] = 0x01; /* TYPE A */
        buf[pos++] = 0x00; buf[pos++] = 0x01; /* CLASS IN */
        buf[pos++] = 0x00; buf[pos++] = 0x00;
        buf[pos++] = 0x00; buf[pos++] = 0x0A; /* TTL = 10s */
        buf[pos++] = 0x00; buf[pos++] = 0x04; /* RDLENGTH = 4 */
        memcpy(&buf[pos], ap_ip, 4);
        pos += 4;

        sendto(s_dns_sock, buf, pos, 0,
               (struct sockaddr *)&client_addr, addr_len);
    }
}

/* ------------------------------------------------------------------ */
/*  WiFi AP setup                                                      */
/* ------------------------------------------------------------------ */

static esp_err_t start_wifi_ap(void)
{
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = AP_CHANNEL,
            .max_connection = AP_MAX_CONN,
            .authmode = WIFI_AUTH_OPEN,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP started: SSID='%s', IP=%s", AP_SSID, AP_IP);
    return ESP_OK;
}

/* ------------------------------------------------------------------ */
/*  HTTP server setup                                                  */
/* ------------------------------------------------------------------ */

static esp_err_t start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    config.lru_purge_enable = true;

    ESP_RETURN_ON_ERROR(httpd_start(&s_httpd, &config), TAG, "httpd_start");

    /* Main config page */
    httpd_uri_t uri_root = {
        .uri = "/", .method = HTTP_GET, .handler = handler_root,
    };
    httpd_register_uri_handler(s_httpd, &uri_root);

    /* Save endpoint */
    httpd_uri_t uri_save = {
        .uri = "/save", .method = HTTP_POST, .handler = handler_save,
    };
    httpd_register_uri_handler(s_httpd, &uri_save);

    /* Captive portal detection endpoints */
    const char *redirect_uris[] = {
        "/generate_204",        /* Android */
        "/gen_204",             /* Android alt */
        "/hotspot-detect.html", /* iOS/macOS */
        "/connecttest.txt",     /* Windows */
        "/redirect",            /* Windows alt */
    };
    for (int i = 0; i < sizeof(redirect_uris) / sizeof(redirect_uris[0]); i++) {
        httpd_uri_t uri = {
            .uri = redirect_uris[i], .method = HTTP_GET, .handler = handler_redirect,
        };
        httpd_register_uri_handler(s_httpd, &uri);
    }

    ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);
    return ESP_OK;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

esp_err_t portal_start(void)
{
    ESP_LOGI(TAG, "Starting captive portal...");

    ESP_RETURN_ON_ERROR(start_wifi_ap(), TAG, "WiFi AP");
    ESP_RETURN_ON_ERROR(start_http_server(), TAG, "HTTP server");

    /* Start DNS redirect task */
    xTaskCreate(dns_task, "dns", 4096, NULL, 5, &s_dns_task);

    ESP_LOGI(TAG, "Captive portal ready. Connect to '%s' and open http://%s/",
             AP_SSID, AP_IP);
    return ESP_OK;
}

void portal_stop(void)
{
    if (s_httpd) {
        httpd_stop(s_httpd);
        s_httpd = NULL;
    }
    if (s_dns_sock >= 0) {
        close(s_dns_sock);
        s_dns_sock = -1;
    }
    if (s_dns_task) {
        vTaskDelete(s_dns_task);
        s_dns_task = NULL;
    }
    esp_wifi_stop();
    esp_wifi_deinit();
}
