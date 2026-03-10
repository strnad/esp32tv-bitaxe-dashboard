/*
 * BitAxe API Client
 *
 * Fetches mining data from BitAxe REST API (/api/system/info)
 * and parses the JSON response into a C struct.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    /* Mining */
    float    hash_rate;          /* GH/s — current */
    float    hash_rate_1m;       /* GH/s — 1-minute average */
    float    hash_rate_10m;      /* GH/s — 10-minute average */
    float    hash_rate_1h;       /* GH/s — 1-hour average */
    float    expected_hashrate;  /* GH/s — theoretical max */
    float    error_percentage;   /* ASIC error rate % */
    char     best_diff[32];
    int      shares_accepted;
    int      shares_rejected;
    int      frequency;          /* MHz */

    /* Pool */
    float    pool_difficulty;
    bool     is_using_fallback;  /* fallback stratum active */

    /* Thermal */
    float    temp;               /* ASIC temp C */
    float    vr_temp;            /* VR temp C */
    float    temp2;              /* secondary sensor C */

    /* Power */
    float    power;              /* Watts */
    float    voltage;            /* Volts (PSU input) */
    float    current;            /* Amps */
    float    core_voltage;       /* ASIC core voltage actual mV */
    float    core_voltage_set;   /* ASIC core voltage configured mV */
    int      fan_speed;          /* percent */

    /* System */
    int      uptime_seconds;
    char     asic_model[32];
    char     hostname[32];
    char     version[32];
    char     axe_os_version[32];
    char     stratum_url[128];
    char     stratum_user[128];
    char     ssid[33];
    int      wifi_rssi;
    char     wifi_status[32];
    char     mac_addr[18];
    char     ip_addr[46];

    /* Block */
    int      block_found;        /* number of blocks found */
    bool     show_new_block;     /* true when a new block was just found */

    /* Status */
    bool     valid;              /* true if last fetch succeeded */
} bitaxe_data_t;

/* Initialize the API client with the given miner host/IP. */
esp_err_t bitaxe_api_init(const char *host);

/* Fetch data from the BitAxe API. Fills *out and sets out->valid.
 * Returns ESP_OK on success, ESP_FAIL on error. */
esp_err_t bitaxe_api_fetch(bitaxe_data_t *out);
