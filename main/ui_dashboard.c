/*
 * BitAxe Dashboard UI — 5-screen LVGL layout for 240x240 display
 *
 * Screen 1 (Main):      hashrate hero + key vitals at a glance
 * Screen 2 (Perf):      rolling hashrate averages, efficiency, errors
 * Screen 3 (Thermal):   temperatures, power, J/TH, fan, core voltage
 * Screen 4 (Mining):    shares, difficulty, pool status, uptime
 * Screen 5 (Network):   pool, worker, WiFi, IP, MAC, firmware
 */

#include "ui_dashboard.h"

#include <stdio.h>
#include <string.h>

/* Custom Space Grotesk fonts */
LV_FONT_DECLARE(font_space_grotesk_14);
LV_FONT_DECLARE(font_space_grotesk_20);
LV_FONT_DECLARE(font_space_grotesk_28);

/* ------------------------------------------------------------------ */
/*  Color palette                                                      */
/* ------------------------------------------------------------------ */

#define CLR_BG          lv_color_hex(0x000000)
#define CLR_DIVIDER     lv_color_hex(0x1A1A1A)

#define CLR_TEXT_PRI    lv_color_hex(0xFFFFFF)
#define CLR_TEXT_SEC    lv_color_hex(0x999999)
#define CLR_TEXT_DIM    lv_color_hex(0x555555)

#define CLR_GREEN       lv_color_hex(0x4ADE80)
#define CLR_AMBER       lv_color_hex(0xFBBF24)
#define CLR_RED         lv_color_hex(0xF87171)

#define CLR_ACCENT      lv_color_hex(0x60A5FA)

/* ------------------------------------------------------------------ */
/*  FontAwesome 5 icon codepoints (merged into 14px font)              */
/* ------------------------------------------------------------------ */

/* Existing */
#define ICON_TEMP    "\xEF\x8B\x89"   /* U+F2C9 thermometer-half */
#define ICON_BOLT    "\xEF\x83\xA7"   /* U+F0E7 bolt */
#define ICON_FAN     "\xEF\xA1\xA3"   /* U+F863 fan */
#define ICON_CUBE    "\xEF\x86\xB2"   /* U+F1B2 cube */
#define ICON_TROPHY  "\xEF\x82\x91"   /* U+F091 trophy */
#define ICON_PLUG    "\xEF\x87\xA6"   /* U+F1E6 plug */
#define ICON_CHIP    "\xEF\x8B\x9B"   /* U+F2DB microchip */

/* New */
#define ICON_TACHO   "\xEF\x8F\xBD"   /* U+F3FD tachometer-alt */
#define ICON_SIGNAL  "\xEF\x80\x92"   /* U+F012 signal */
#define ICON_WARN    "\xEF\x81\xB1"   /* U+F071 exclamation-triangle */
#define ICON_CHECK   "\xEF\x81\x98"   /* U+F058 check-circle */
#define ICON_CLOCK   "\xEF\x80\x97"   /* U+F017 clock */
#define ICON_SERVER  "\xEF\x88\xB3"   /* U+F233 server */
#define ICON_PCT     "\xEF\x95\x81"   /* U+F541 percentage */

/* ------------------------------------------------------------------ */
/*  Coin detection                                                     */
/* ------------------------------------------------------------------ */

typedef enum {
    COIN_UNKNOWN = 0,
    COIN_BTC,
    COIN_BCH,
    COIN_BTG,
} coin_type_t;

LV_IMAGE_DECLARE(btc_24);
LV_IMAGE_DECLARE(bch_24);
LV_IMAGE_DECLARE(btg_24);

/* ------------------------------------------------------------------ */
/*  Screen objects                                                     */
/* ------------------------------------------------------------------ */

#define NUM_SCREENS 5

static lv_obj_t *s_screens[NUM_SCREENS] = {NULL};
static lv_obj_t *s_scr_connect = NULL;
static lv_obj_t *s_scr_setup  = NULL;
static lv_obj_t *s_reset_lbl  = NULL;
static lv_display_t *s_disp    = NULL;
static int s_current_screen    = 0;

/* --- Screen 1: Main Dashboard --- */
static lv_obj_t *s_hash_lbl     = NULL;
static lv_obj_t *s_hash_bar     = NULL;
static lv_obj_t *s_exp_hash_lbl = NULL;
static lv_obj_t *s_coin_img     = NULL;
static coin_type_t s_last_coin  = COIN_UNKNOWN;

static lv_obj_t *s_temp_accent  = NULL;
static lv_obj_t *s_pwr_accent   = NULL;
static lv_obj_t *s_acc_accent   = NULL;
static lv_obj_t *s_diff_accent  = NULL;

static lv_obj_t *s_temp_icon    = NULL;
static lv_obj_t *s_temp_lbl     = NULL;
static lv_obj_t *s_vr_lbl       = NULL;

static lv_obj_t *s_pwr_icon     = NULL;
static lv_obj_t *s_pwr_lbl      = NULL;
static lv_obj_t *s_fan_lbl      = NULL;

static lv_obj_t *s_acc_icon     = NULL;
static lv_obj_t *s_acc_lbl      = NULL;
static lv_obj_t *s_rej_lbl      = NULL;

static lv_obj_t *s_diff_icon    = NULL;
static lv_obj_t *s_diff_lbl     = NULL;

static lv_obj_t *s_psu_lbl      = NULL;
static lv_obj_t *s_asic_lbl     = NULL;
static lv_obj_t *s_freq_lbl     = NULL;
static lv_obj_t *s_up_lbl       = NULL;

/* --- Screen 2: Performance --- */
static lv_obj_t *s_perf_hash_lbl  = NULL;
static lv_obj_t *s_perf_exp_lbl   = NULL;
static lv_obj_t *s_perf_bar_1m    = NULL;
static lv_obj_t *s_perf_val_1m    = NULL;
static lv_obj_t *s_perf_bar_10m   = NULL;
static lv_obj_t *s_perf_val_10m   = NULL;
static lv_obj_t *s_perf_bar_1h    = NULL;
static lv_obj_t *s_perf_val_1h    = NULL;
static lv_obj_t *s_perf_eff_lbl   = NULL;
static lv_obj_t *s_perf_err_lbl   = NULL;
static lv_obj_t *s_perf_freq_lbl  = NULL;

/* --- Screen 3: Thermal & Power --- */
static lv_obj_t *s_therm_asic_lbl = NULL;
static lv_obj_t *s_therm_vr_lbl   = NULL;
static lv_obj_t *s_therm_t2_lbl   = NULL;
static lv_obj_t *s_therm_bar      = NULL;
static lv_obj_t *s_therm_pwr_lbl  = NULL;
static lv_obj_t *s_therm_volt_lbl = NULL;
static lv_obj_t *s_therm_cur_lbl  = NULL;
static lv_obj_t *s_therm_eff_lbl  = NULL;
static lv_obj_t *s_therm_fan_lbl  = NULL;
static lv_obj_t *s_therm_fan_bar  = NULL;
static lv_obj_t *s_therm_cv_lbl   = NULL;

/* --- Screen 4: Mining Stats --- */
static lv_obj_t *s_mine_acc_lbl   = NULL;
static lv_obj_t *s_mine_rej_lbl   = NULL;
static lv_obj_t *s_mine_rate_lbl  = NULL;
static lv_obj_t *s_mine_rate_bar  = NULL;
static lv_obj_t *s_mine_bdiff_lbl = NULL;
static lv_obj_t *s_mine_pdiff_lbl = NULL;
static lv_obj_t *s_mine_fb_lbl    = NULL;
static lv_obj_t *s_mine_up_lbl    = NULL;

/* --- Screen 5: Network & System --- */
static lv_obj_t *s_net_pool_lbl   = NULL;
static lv_obj_t *s_net_worker_lbl = NULL;
static lv_obj_t *s_net_wifi_lbl   = NULL;
static lv_obj_t *s_net_wifi_bars[5] = {NULL};
static lv_obj_t *s_net_ip4_lbl    = NULL;
static lv_obj_t *s_net_mac_lbl    = NULL;
static lv_obj_t *s_net_host_lbl   = NULL;
static lv_obj_t *s_net_asic_lbl   = NULL;
static lv_obj_t *s_net_fw_lbl     = NULL;
static lv_obj_t *s_net_axeos_lbl  = NULL;
static lv_obj_t *s_net_up_lbl     = NULL;

/* --- Connection screen --- */
static lv_obj_t *s_connect_lbl    = NULL;

/* --- Blink state for critical temperature (>74.5°C) --- */
static bool s_temp_blink_on       = true;
static bool s_temp_critical       = false;
static lv_timer_t *s_blink_timer  = NULL;

/* --- Block Found overlay --- */
static lv_obj_t  *s_block_overlay = NULL;
static lv_obj_t  *s_block_lbl     = NULL;
static lv_timer_t *s_block_blink_timer = NULL;
static bool        s_block_blink_on    = true;

/* ------------------------------------------------------------------ */
/*  Blink timer for critical temperature                               */
/* ------------------------------------------------------------------ */

static void blink_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!s_temp_critical) return;
    s_temp_blink_on = !s_temp_blink_on;

    lv_opa_t opa = s_temp_blink_on ? LV_OPA_COVER : LV_OPA_20;

    /* Screen 1 temp labels */
    if (s_temp_lbl) lv_obj_set_style_text_opa(s_temp_lbl, opa, 0);
    if (s_temp_icon) lv_obj_set_style_text_opa(s_temp_icon, opa, 0);

    /* Screen 3 ASIC temp label */
    if (s_therm_asic_lbl) lv_obj_set_style_text_opa(s_therm_asic_lbl, opa, 0);
}

/* ------------------------------------------------------------------ */
/*  Block Found blink timer                                            */
/* ------------------------------------------------------------------ */

static void block_blink_cb(lv_timer_t *timer)
{
    (void)timer;
    s_block_blink_on = !s_block_blink_on;
    if (s_block_lbl) {
        lv_obj_set_style_text_opa(s_block_lbl,
            s_block_blink_on ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    }
}

static void build_block_overlay(void)
{
    /* Fullscreen overlay — created once, stays hidden until needed */
    s_block_overlay = lv_obj_create(NULL);
    lv_obj_set_size(s_block_overlay, 240, 240);
    lv_obj_set_style_bg_color(s_block_overlay, CLR_BG, 0);
    lv_obj_set_style_bg_opa(s_block_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_block_overlay, 0, 0);
    lv_obj_set_style_pad_all(s_block_overlay, 0, 0);
    lv_obj_remove_flag(s_block_overlay, LV_OBJ_FLAG_SCROLLABLE);

    /* "BLOCK" on one line, "FOUND!!!" on the next */
    s_block_lbl = lv_label_create(s_block_overlay);
    lv_label_set_text(s_block_lbl, "BLOCK\nFOUND!!!");
    lv_obj_set_style_text_font(s_block_lbl, &font_space_grotesk_28, 0);
    lv_obj_set_style_text_color(s_block_lbl, CLR_RED, 0);
    lv_obj_set_style_text_align(s_block_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_letter_space(s_block_lbl, 2, 0);
    lv_obj_center(s_block_lbl);
}

/* ------------------------------------------------------------------ */
/*  Helpers                                                            */
/* ------------------------------------------------------------------ */

static lv_color_t hashrate_color(float ghs)
{
    if (ghs <= 0.0f)    return CLR_RED;
    if (ghs < 875.0f) {
        uint8_t mix = (uint8_t)((ghs / 875.0f) * 255);
        return lv_color_mix(CLR_AMBER, CLR_RED, mix);
    }
    if (ghs < 1750.0f) {
        uint8_t mix = (uint8_t)(((ghs - 875.0f) / 875.0f) * 255);
        return lv_color_mix(CLR_ACCENT, CLR_AMBER, mix);
    }
    if (ghs < 2625.0f) {
        uint8_t mix = (uint8_t)(((ghs - 1750.0f) / 875.0f) * 255);
        return lv_color_mix(CLR_GREEN, CLR_ACCENT, mix);
    }
    return CLR_GREEN;
}

static lv_color_t temp_color(float temp)
{
    if (temp < 70.0f)  return CLR_ACCENT;   /* blue — normal */
    if (temp < 74.0f)  return CLR_AMBER;    /* amber — warm */
    return CLR_RED;                          /* red — hot */
}

static void format_uptime(int seconds, char *buf, size_t buf_size)
{
    int d = seconds / 86400;
    int h = (seconds % 86400) / 3600;
    int m = (seconds % 3600) / 60;
    if (d > 0)      snprintf(buf, buf_size, "%dd %dh %dm", d, h, m);
    else if (h > 0) snprintf(buf, buf_size, "%dh %dm", h, m);
    else            snprintf(buf, buf_size, "%dm", m);
}

static void format_uptime_full(int seconds, char *buf, size_t buf_size)
{
    int d = seconds / 86400;
    int h = (seconds % 86400) / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    if (d > 0)       snprintf(buf, buf_size, "%dd %dh %dm %ds", d, h, m, s);
    else if (h > 0)  snprintf(buf, buf_size, "%dh %dm %ds", h, m, s);
    else             snprintf(buf, buf_size, "%dm %ds", m, s);
}

static void format_diff(const char *raw, char *buf, size_t buf_size)
{
    double val = 0;
    if (sscanf(raw, "%lf", &val) == 1 && val >= 1000) {
        if      (val >= 1e12) snprintf(buf, buf_size, "%.2fT", val / 1e12);
        else if (val >= 1e9)  snprintf(buf, buf_size, "%.2fG", val / 1e9);
        else if (val >= 1e6)  snprintf(buf, buf_size, "%.2fM", val / 1e6);
        else if (val >= 1e3)  snprintf(buf, buf_size, "%.2fK", val / 1e3);
        else                  snprintf(buf, buf_size, "%.0f", val);
    } else {
        strncpy(buf, raw, buf_size - 1);
        buf[buf_size - 1] = '\0';
    }
}

static void format_number_comma(int val, char *buf, size_t sz)
{
    if (val < 1000) {
        snprintf(buf, sz, "%d", val);
    } else if (val < 1000000) {
        snprintf(buf, sz, "%d,%03d", val / 1000, val % 1000);
    } else {
        snprintf(buf, sz, "%d,%03d,%03d",
                 val / 1000000, (val / 1000) % 1000, val % 1000);
    }
}

static void format_hashrate(float ghs, char *buf, size_t sz)
{
    if (ghs >= 1000.0f)
        snprintf(buf, sz, "%.2f TH/s", ghs / 1000.0f);
    else
        snprintf(buf, sz, "%.1f GH/s", ghs);
}

static void format_pool_diff(float diff, char *buf, size_t sz)
{
    if (diff >= 1e12)      snprintf(buf, sz, "%.2fT", diff / 1e12);
    else if (diff >= 1e9)  snprintf(buf, sz, "%.2fG", diff / 1e9);
    else if (diff >= 1e6)  snprintf(buf, sz, "%.2fM", diff / 1e6);
    else if (diff >= 1e3)  snprintf(buf, sz, "%.1fK", diff / 1e3);
    else                   snprintf(buf, sz, "%.0f", diff);
}

static coin_type_t detect_coin(const char *stratum_user)
{
    if (!stratum_user || !stratum_user[0]) return COIN_UNKNOWN;

    if (strncmp(stratum_user, "bc1", 3) == 0)           return COIN_BTC;
    if (strncmp(stratum_user, "bitcoincash:", 12) == 0)  return COIN_BCH;
    if (strncmp(stratum_user, "btg1", 4) == 0)           return COIN_BTG;

    switch (stratum_user[0]) {
        case '1': case '3': return COIN_BTC;
        case 'q': case 'p': return COIN_BCH;
        case 'G':           return COIN_BTG;
        default:            return COIN_UNKNOWN;
    }
}

/* ------------------------------------------------------------------ */
/*  Widget factories                                                   */
/* ------------------------------------------------------------------ */

static lv_obj_t *make_label(lv_obj_t *parent, const lv_font_t *font,
                             lv_color_t color, lv_align_t align,
                             int x, int y, const char *text)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_align(lbl, align, x, y);
    return lbl;
}

static void create_dot(lv_obj_t *parent, int x, int y, lv_color_t color)
{
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 6, 6);
    lv_obj_set_pos(dot, x, y);
    lv_obj_set_style_bg_color(dot, color, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(dot, 3, 0);
}

static void create_page_dots(lv_obj_t *parent, int active_idx)
{
    /* 5 dots × 6px + 4 gaps × 4px = 46px. Start x = (240-46)/2 = 97 */
    for (int i = 0; i < NUM_SCREENS; i++) {
        create_dot(parent, 97 + i * 10, 228,
                   i == active_idx ? CLR_ACCENT : CLR_TEXT_DIM);
    }
}

static void create_sep(lv_obj_t *parent, int y)
{
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_remove_style_all(line);
    lv_obj_set_size(line, 224, 1);
    lv_obj_set_style_bg_color(line, CLR_DIVIDER, 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, y);
}

static lv_obj_t *create_accent_bar(lv_obj_t *parent, int x, int y, int h,
                                    lv_color_t color)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, 4, h);
    lv_obj_set_pos(bar, x, y);
    lv_obj_set_style_bg_color(bar, color, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bar, 2, 0);
    return bar;
}

static lv_obj_t *make_screen(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, CLR_BG, 0);
    return scr;
}

static lv_obj_t *make_bar(lv_obj_t *parent, int x, int y, int w, int h,
                           int range_max, lv_color_t ind_color)
{
    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_set_size(bar, w, h);
    lv_obj_set_pos(bar, x, y);
    lv_bar_set_range(bar, 0, range_max);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, h / 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, ind_color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, h / 2, LV_PART_INDICATOR);
    return bar;
}

/* ------------------------------------------------------------------ */
/*  Screen 1: Main Dashboard                                           */
/* ------------------------------------------------------------------ */
static void build_screen_main(void)
{
    lv_obj_t *scr = make_screen();
    s_screens[0] = scr;

    /* Coin logo — top right */
    s_coin_img = lv_image_create(scr);
    lv_obj_set_size(s_coin_img, 24, 24);
    lv_obj_set_pos(s_coin_img, 210, 6);
    lv_obj_add_flag(s_coin_img, LV_OBJ_FLAG_HIDDEN);

    /* Hashrate value */
    s_hash_lbl = lv_label_create(scr);
    lv_label_set_text(s_hash_lbl, "--- GH/s");
    lv_obj_set_style_text_font(s_hash_lbl, &font_space_grotesk_28, 0);
    lv_obj_set_style_text_color(s_hash_lbl, CLR_TEXT_PRI, 0);
    lv_obj_align(s_hash_lbl, LV_ALIGN_TOP_MID, 0, 4);

    /* Efficiency bar: -10% to +10%, symmetrical from center */
    s_hash_bar = lv_bar_create(scr);
    lv_obj_set_size(s_hash_bar, 196, 12);
    lv_obj_set_pos(s_hash_bar, 22, 38);
    lv_bar_set_mode(s_hash_bar, LV_BAR_MODE_SYMMETRICAL);
    lv_bar_set_range(s_hash_bar, -100, 100);
    lv_bar_set_value(s_hash_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_hash_bar, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_hash_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(s_hash_bar, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_hash_bar, CLR_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_hash_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_hash_bar, 6, LV_PART_INDICATOR);

    /* Expected hashrate comparison */
    s_exp_hash_lbl = make_label(scr, &font_space_grotesk_14,
                                 CLR_TEXT_DIM, LV_ALIGN_TOP_MID, 0, 52, "");

    create_sep(scr, 68);

    /* --- Temperature + Power row (y=70-112) --- */
    s_temp_accent = create_accent_bar(scr, 2, 70, 42, CLR_GREEN);
    s_pwr_accent  = create_accent_bar(scr, 122, 70, 42, CLR_AMBER);

    s_temp_icon = make_label(scr, &font_space_grotesk_14,
                              CLR_GREEN, LV_ALIGN_TOP_LEFT, 12, 78, ICON_TEMP);
    s_temp_lbl = lv_label_create(scr);
    lv_label_set_text(s_temp_lbl, "--\xc2\xb0""C");
    lv_obj_set_style_text_font(s_temp_lbl, &font_space_grotesk_28, 0);
    lv_obj_set_style_text_color(s_temp_lbl, CLR_GREEN, 0);
    lv_obj_align(s_temp_lbl, LV_ALIGN_TOP_LEFT, 30, 72);

    s_vr_lbl = make_label(scr, &font_space_grotesk_14,
                           CLR_TEXT_DIM, LV_ALIGN_TOP_LEFT, 12, 100,
                           ICON_CHIP " --\xc2\xb0""C");

    s_pwr_icon = make_label(scr, &font_space_grotesk_14,
                              CLR_AMBER, LV_ALIGN_TOP_LEFT, 130, 78, ICON_BOLT);
    s_pwr_lbl = lv_label_create(scr);
    lv_label_set_text(s_pwr_lbl, "--W");
    lv_obj_set_style_text_font(s_pwr_lbl, &font_space_grotesk_28, 0);
    lv_obj_set_style_text_color(s_pwr_lbl, CLR_TEXT_PRI, 0);
    lv_obj_align(s_pwr_lbl, LV_ALIGN_TOP_LEFT, 148, 72);

    s_fan_lbl = make_label(scr, &font_space_grotesk_14,
                            CLR_TEXT_DIM, LV_ALIGN_TOP_LEFT, 130, 100,
                            ICON_FAN " --%");

    create_sep(scr, 114);

    /* --- Efficiency + Diff row (y=116-158) --- */
    s_acc_accent  = create_accent_bar(scr, 2, 116, 42, CLR_GREEN);
    s_diff_accent = create_accent_bar(scr, 122, 116, 42, CLR_ACCENT);

    s_acc_icon = make_label(scr, &font_space_grotesk_14,
                              CLR_GREEN, LV_ALIGN_TOP_LEFT, 12, 124, ICON_PCT);
    s_acc_lbl = lv_label_create(scr);
    lv_label_set_text(s_acc_lbl, "--");
    lv_obj_set_style_text_font(s_acc_lbl, &font_space_grotesk_28, 0);
    lv_obj_set_style_text_color(s_acc_lbl, CLR_TEXT_PRI, 0);
    lv_obj_align(s_acc_lbl, LV_ALIGN_TOP_LEFT, 30, 118);

    s_rej_lbl = make_label(scr, &font_space_grotesk_14,
                            CLR_TEXT_DIM, LV_ALIGN_TOP_LEFT, 30, 146, "J/TH");

    s_diff_icon = make_label(scr, &font_space_grotesk_14,
                               CLR_ACCENT, LV_ALIGN_TOP_LEFT, 130, 124, ICON_TROPHY);
    s_diff_lbl = lv_label_create(scr);
    lv_label_set_text(s_diff_lbl, "---");
    lv_obj_set_style_text_font(s_diff_lbl, &font_space_grotesk_28, 0);
    lv_obj_set_style_text_color(s_diff_lbl, CLR_TEXT_PRI, 0);
    lv_obj_align(s_diff_lbl, LV_ALIGN_TOP_LEFT, 148, 118);

    create_sep(scr, 160);

    /* --- System info (y=162-220) --- */
    s_psu_lbl = make_label(scr, &font_space_grotesk_20,
                            CLR_TEXT_PRI, LV_ALIGN_TOP_LEFT, 10, 164, "--mV");
    s_asic_lbl = make_label(scr, &font_space_grotesk_20,
                             CLR_TEXT_PRI, LV_ALIGN_TOP_RIGHT, -10, 164, "--mV");
    s_freq_lbl = make_label(scr, &font_space_grotesk_20,
                             CLR_TEXT_PRI, LV_ALIGN_TOP_LEFT, 10, 190, "---MHz");
    s_up_lbl = make_label(scr, &font_space_grotesk_20,
                           CLR_TEXT_PRI, LV_ALIGN_TOP_RIGHT, -10, 190, "---");

    create_page_dots(scr, 0);
}

/* ------------------------------------------------------------------ */
/*  Screen 2: Performance                                              */
/* ------------------------------------------------------------------ */
static void build_screen_perf(void)
{
    lv_obj_t *scr = make_screen();
    s_screens[1] = scr;

    make_label(scr, &font_space_grotesk_20,
               CLR_ACCENT, LV_ALIGN_TOP_MID, 0, 4, "PERFORMANCE");
    create_sep(scr, 28);

    /* Hero hashrate */
    s_perf_hash_lbl = lv_label_create(scr);
    lv_label_set_text(s_perf_hash_lbl, "--- GH/s");
    lv_obj_set_style_text_font(s_perf_hash_lbl, &font_space_grotesk_28, 0);
    lv_obj_set_style_text_color(s_perf_hash_lbl, CLR_TEXT_PRI, 0);
    lv_obj_align(s_perf_hash_lbl, LV_ALIGN_TOP_MID, 0, 32);

    /* vs expected */
    s_perf_exp_lbl = make_label(scr, &font_space_grotesk_14,
                                 CLR_TEXT_DIM, LV_ALIGN_TOP_MID, 0, 62, "");

    create_sep(scr, 80);

    /* Rolling average bars: 1m, 10m, 1h */
    /* Row: label(30px) gap(4px) bar(130px) gap(6px) value(60px) */
    int bar_y[] = {86, 108, 130};
    lv_color_t bar_clr[] = {CLR_GREEN, CLR_ACCENT, CLR_AMBER};
    const char *bar_names[] = {"1m", "10m", "1h"};
    lv_obj_t **bars[]  = {&s_perf_bar_1m,  &s_perf_bar_10m,  &s_perf_bar_1h};
    lv_obj_t **vals[]  = {&s_perf_val_1m,  &s_perf_val_10m,  &s_perf_val_1h};

    for (int i = 0; i < 3; i++) {
        make_label(scr, &font_space_grotesk_14,
                   CLR_TEXT_SEC, LV_ALIGN_TOP_LEFT, 10, bar_y[i] + 2,
                   bar_names[i]);
        *bars[i] = make_bar(scr, 44, bar_y[i] + 4, 120, 10, 3500, bar_clr[i]);
        *vals[i] = make_label(scr, &font_space_grotesk_14,
                               CLR_TEXT_PRI, LV_ALIGN_TOP_LEFT, 170, bar_y[i] + 2,
                               "---");
    }

    create_sep(scr, 152);

    /* Efficiency */
    s_perf_eff_lbl = lv_label_create(scr);
    lv_label_set_text(s_perf_eff_lbl, "--%");
    lv_obj_set_style_text_font(s_perf_eff_lbl, &font_space_grotesk_28, 0);
    lv_obj_set_style_text_color(s_perf_eff_lbl, CLR_GREEN, 0);
    lv_obj_align(s_perf_eff_lbl, LV_ALIGN_TOP_LEFT, 10, 158);

    make_label(scr, &font_space_grotesk_14,
               CLR_TEXT_DIM, LV_ALIGN_TOP_LEFT, 10, 186, "efficiency");

    /* Error rate */
    s_perf_err_lbl = make_label(scr, &font_space_grotesk_20,
                                 CLR_GREEN, LV_ALIGN_TOP_RIGHT, -10, 160,
                                 "ERR 0.00%");

    /* Frequency */
    s_perf_freq_lbl = make_label(scr, &font_space_grotesk_20,
                                  CLR_TEXT_PRI, LV_ALIGN_TOP_RIGHT, -10, 186,
                                  "---MHz");

    create_page_dots(scr, 1);
}

/* ------------------------------------------------------------------ */
/*  Screen 3: Thermal & Power                                          */
/* ------------------------------------------------------------------ */
static void build_screen_thermal(void)
{
    lv_obj_t *scr = make_screen();
    s_screens[2] = scr;

    make_label(scr, &font_space_grotesk_20,
               CLR_ACCENT, LV_ALIGN_TOP_MID, 0, 4, "THERMAL & POWER");
    create_sep(scr, 28);

    /* 3 temperature columns */
    /* Column 1: ASIC (x=10-78) */
    make_label(scr, &font_space_grotesk_14,
               CLR_TEXT_DIM, LV_ALIGN_TOP_LEFT, 10, 32, ICON_TEMP " ASIC");
    s_therm_asic_lbl = lv_label_create(scr);
    lv_label_set_text(s_therm_asic_lbl, "--\xc2\xb0""C");
    lv_obj_set_style_text_font(s_therm_asic_lbl, &font_space_grotesk_28, 0);
    lv_obj_set_style_text_color(s_therm_asic_lbl, CLR_GREEN, 0);
    lv_obj_align(s_therm_asic_lbl, LV_ALIGN_TOP_LEFT, 10, 48);

    /* Column 2: VR (x=90-150) */
    make_label(scr, &font_space_grotesk_14,
               CLR_TEXT_DIM, LV_ALIGN_TOP_LEFT, 90, 32, ICON_CHIP " VR");
    s_therm_vr_lbl = lv_label_create(scr);
    lv_label_set_text(s_therm_vr_lbl, "--\xc2\xb0""C");
    lv_obj_set_style_text_font(s_therm_vr_lbl, &font_space_grotesk_20, 0);
    lv_obj_set_style_text_color(s_therm_vr_lbl, CLR_GREEN, 0);
    lv_obj_align(s_therm_vr_lbl, LV_ALIGN_TOP_LEFT, 90, 50);

    /* Column 3: Board (x=170-230) */
    make_label(scr, &font_space_grotesk_14,
               CLR_TEXT_DIM, LV_ALIGN_TOP_LEFT, 170, 32, "BOARD");
    s_therm_t2_lbl = lv_label_create(scr);
    lv_label_set_text(s_therm_t2_lbl, "--\xc2\xb0""C");
    lv_obj_set_style_text_font(s_therm_t2_lbl, &font_space_grotesk_20, 0);
    lv_obj_set_style_text_color(s_therm_t2_lbl, CLR_TEXT_DIM, 0);
    lv_obj_align(s_therm_t2_lbl, LV_ALIGN_TOP_LEFT, 170, 50);

    create_sep(scr, 76);

    /* Temperature bar */
    s_therm_bar = make_bar(scr, 10, 82, 220, 10, 100, CLR_GREEN);
    create_sep(scr, 98);

    /* Power section */
    s_therm_pwr_lbl = lv_label_create(scr);
    lv_label_set_text(s_therm_pwr_lbl, "--W");
    lv_obj_set_style_text_font(s_therm_pwr_lbl, &font_space_grotesk_28, 0);
    lv_obj_set_style_text_color(s_therm_pwr_lbl, CLR_AMBER, 0);
    lv_obj_align(s_therm_pwr_lbl, LV_ALIGN_TOP_LEFT, 10, 104);

    s_therm_volt_lbl = make_label(scr, &font_space_grotesk_20,
                                   CLR_TEXT_PRI, LV_ALIGN_TOP_RIGHT, -10, 104,
                                   "--mV");
    s_therm_cur_lbl = make_label(scr, &font_space_grotesk_14,
                                  CLR_TEXT_SEC, LV_ALIGN_TOP_RIGHT, -10, 128,
                                  "--A");

    create_sep(scr, 144);

    /* Efficiency J/TH */
    s_therm_eff_lbl = lv_label_create(scr);
    lv_label_set_text(s_therm_eff_lbl, "-- J/TH");
    lv_obj_set_style_text_font(s_therm_eff_lbl, &font_space_grotesk_28, 0);
    lv_obj_set_style_text_color(s_therm_eff_lbl, CLR_GREEN, 0);
    lv_obj_align(s_therm_eff_lbl, LV_ALIGN_TOP_LEFT, 10, 150);

    create_sep(scr, 180);

    /* Fan */
    s_therm_fan_lbl = make_label(scr, &font_space_grotesk_20,
                                  CLR_TEXT_PRI, LV_ALIGN_TOP_LEFT, 10, 186,
                                  ICON_FAN " --%");
    s_therm_fan_bar = make_bar(scr, 100, 192, 90, 8, 100, CLR_ACCENT);

    /* Core voltage set vs actual */
    s_therm_cv_lbl = make_label(scr, &font_space_grotesk_14,
                                 CLR_TEXT_SEC, LV_ALIGN_TOP_LEFT, 10, 210,
                                 "Core: --mV / --mV");

    create_page_dots(scr, 2);
}

/* ------------------------------------------------------------------ */
/*  Screen 4: Mining Stats                                             */
/* ------------------------------------------------------------------ */
static void build_screen_mining(void)
{
    lv_obj_t *scr = make_screen();
    s_screens[3] = scr;

    make_label(scr, &font_space_grotesk_20,
               CLR_ACCENT, LV_ALIGN_TOP_MID, 0, 4, "MINING STATS");
    create_sep(scr, 28);

    /* Accepted shares */
    make_label(scr, &font_space_grotesk_14,
               CLR_GREEN, LV_ALIGN_TOP_LEFT, 10, 34, ICON_CHECK " ACCEPTED");
    s_mine_acc_lbl = lv_label_create(scr);
    lv_label_set_text(s_mine_acc_lbl, "---");
    lv_obj_set_style_text_font(s_mine_acc_lbl, &font_space_grotesk_28, 0);
    lv_obj_set_style_text_color(s_mine_acc_lbl, CLR_GREEN, 0);
    lv_obj_align(s_mine_acc_lbl, LV_ALIGN_TOP_LEFT, 10, 50);

    /* Rejected shares */
    s_mine_rej_lbl = make_label(scr, &font_space_grotesk_20,
                                 CLR_TEXT_DIM, LV_ALIGN_TOP_RIGHT, -10, 50,
                                 "R: 0");

    /* Acceptance rate */
    s_mine_rate_lbl = make_label(scr, &font_space_grotesk_20,
                                  CLR_GREEN, LV_ALIGN_TOP_LEFT, 10, 80,
                                  "100.00%");

    s_mine_rate_bar = make_bar(scr, 10, 104, 220, 8, 10000, CLR_GREEN);

    create_sep(scr, 118);

    /* Best difficulty */
    make_label(scr, &font_space_grotesk_14,
               CLR_ACCENT, LV_ALIGN_TOP_LEFT, 10, 124, ICON_TROPHY " BEST DIFF");
    s_mine_bdiff_lbl = lv_label_create(scr);
    lv_label_set_text(s_mine_bdiff_lbl, "---");
    lv_obj_set_style_text_font(s_mine_bdiff_lbl, &font_space_grotesk_28, 0);
    lv_obj_set_style_text_color(s_mine_bdiff_lbl, CLR_ACCENT, 0);
    lv_obj_align(s_mine_bdiff_lbl, LV_ALIGN_TOP_LEFT, 10, 140);

    /* Pool difficulty */
    s_mine_pdiff_lbl = make_label(scr, &font_space_grotesk_14,
                                   CLR_TEXT_SEC, LV_ALIGN_TOP_LEFT, 10, 168,
                                   "Pool diff: ---");

    create_sep(scr, 184);

    /* Fallback status */
    s_mine_fb_lbl = make_label(scr, &font_space_grotesk_20,
                                CLR_GREEN, LV_ALIGN_TOP_LEFT, 10, 190,
                                ICON_CHECK " PRIMARY");

    /* Uptime */
    s_mine_up_lbl = make_label(scr, &font_space_grotesk_14,
                                CLR_TEXT_SEC, LV_ALIGN_TOP_RIGHT, -10, 194,
                                ICON_CLOCK " ---");

    create_page_dots(scr, 3);
}

/* ------------------------------------------------------------------ */
/*  Screen 5: Network & System                                         */
/* ------------------------------------------------------------------ */
static void build_screen_network(void)
{
    lv_obj_t *scr = make_screen();
    s_screens[4] = scr;

    make_label(scr, &font_space_grotesk_20,
               CLR_ACCENT, LV_ALIGN_TOP_MID, 0, 4, "NETWORK & SYSTEM");
    create_sep(scr, 28);

    /* Pool URL */
    make_label(scr, &font_space_grotesk_14,
               CLR_TEXT_DIM, LV_ALIGN_TOP_LEFT, 10, 34, ICON_SERVER " POOL");
    s_net_pool_lbl = lv_label_create(scr);
    lv_label_set_text(s_net_pool_lbl, "---");
    lv_obj_set_style_text_font(s_net_pool_lbl, &font_space_grotesk_14, 0);
    lv_obj_set_style_text_color(s_net_pool_lbl, CLR_TEXT_PRI, 0);
    lv_obj_align(s_net_pool_lbl, LV_ALIGN_TOP_LEFT, 10, 50);
    lv_obj_set_width(s_net_pool_lbl, 220);
    lv_label_set_long_mode(s_net_pool_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);

    /* Worker */
    s_net_worker_lbl = lv_label_create(scr);
    lv_label_set_text(s_net_worker_lbl, "---");
    lv_obj_set_style_text_font(s_net_worker_lbl, &font_space_grotesk_14, 0);
    lv_obj_set_style_text_color(s_net_worker_lbl, CLR_TEXT_SEC, 0);
    lv_obj_align(s_net_worker_lbl, LV_ALIGN_TOP_LEFT, 10, 66);
    lv_obj_set_width(s_net_worker_lbl, 220);
    lv_label_set_long_mode(s_net_worker_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);

    create_sep(scr, 82);

    /* WiFi SSID + RSSI */
    s_net_wifi_lbl = make_label(scr, &font_space_grotesk_14,
                                 CLR_TEXT_PRI, LV_ALIGN_TOP_LEFT, 10, 88,
                                 ICON_SIGNAL " ---");

    /* WiFi signal bars: 5 rectangles with increasing height */
    int bar_heights[] = {4, 6, 8, 10, 12};
    for (int i = 0; i < 5; i++) {
        lv_obj_t *b = lv_obj_create(scr);
        lv_obj_remove_style_all(b);
        lv_obj_set_size(b, 8, bar_heights[i]);
        lv_obj_set_pos(b, 180 + i * 11, 100 - bar_heights[i] + 12);
        lv_obj_set_style_bg_color(b, CLR_TEXT_DIM, 0);
        lv_obj_set_style_bg_opa(b, LV_OPA_30, 0);
        lv_obj_set_style_radius(b, 1, 0);
        s_net_wifi_bars[i] = b;
    }

    create_sep(scr, 116);

    /* IP */
    s_net_ip4_lbl = make_label(scr, &font_space_grotesk_14,
                                CLR_TEXT_PRI, LV_ALIGN_TOP_LEFT, 10, 122,
                                "IP: ---");

    /* MAC */
    s_net_mac_lbl = make_label(scr, &font_space_grotesk_14,
                                CLR_TEXT_DIM, LV_ALIGN_TOP_LEFT, 10, 138,
                                "MAC: ---");

    create_sep(scr, 156);

    /* Hostname + ASIC model */
    s_net_host_lbl = make_label(scr, &font_space_grotesk_14,
                                 CLR_TEXT_PRI, LV_ALIGN_TOP_LEFT, 10, 162,
                                 "---");
    s_net_asic_lbl = make_label(scr, &font_space_grotesk_14,
                                 CLR_TEXT_PRI, LV_ALIGN_TOP_RIGHT, -10, 162,
                                 "---");

    /* Firmware + AxeOS version */
    s_net_fw_lbl = make_label(scr, &font_space_grotesk_14,
                               CLR_TEXT_SEC, LV_ALIGN_TOP_LEFT, 10, 178,
                               "FW: ---");
    s_net_axeos_lbl = make_label(scr, &font_space_grotesk_14,
                                  CLR_TEXT_SEC, LV_ALIGN_TOP_RIGHT, -10, 178,
                                  "");

    /* Uptime */
    s_net_up_lbl = make_label(scr, &font_space_grotesk_20,
                               CLR_TEXT_PRI, LV_ALIGN_TOP_LEFT, 10, 198,
                               ICON_CLOCK " ---");

    create_page_dots(scr, 4);
}

/* ------------------------------------------------------------------ */
/*  Connection screen                                                  */
/* ------------------------------------------------------------------ */
static void build_screen_connect(void)
{
    s_scr_connect = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_connect, CLR_BG, 0);

    lv_obj_t *card = lv_obj_create(s_scr_connect);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, 180, 120);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, CLR_BG, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_color(card, CLR_ACCENT, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_opa(card, LV_OPA_COVER, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *spinner = lv_spinner_create(card);
    lv_spinner_set_anim_params(spinner, 1000, 200);
    lv_obj_set_size(spinner, 48, 48);
    lv_obj_align(spinner, LV_ALIGN_TOP_MID, 0, 14);
    lv_obj_set_style_arc_color(spinner, CLR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, CLR_BG, LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner, 4, LV_PART_MAIN);

    s_connect_lbl = lv_label_create(card);
    lv_label_set_text(s_connect_lbl, "Connecting...");
    lv_obj_set_style_text_color(s_connect_lbl, CLR_TEXT_SEC, 0);
    lv_obj_set_style_text_font(s_connect_lbl, &font_space_grotesk_14, 0);
    lv_obj_align(s_connect_lbl, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_set_style_text_align(s_connect_lbl, LV_TEXT_ALIGN_CENTER, 0);
}

/* ------------------------------------------------------------------ */
/*  Setup mode screen (captive portal)                                 */
/* ------------------------------------------------------------------ */
static void build_screen_setup(void)
{
    s_scr_setup = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr_setup, CLR_BG, 0);

    /* Title */
    lv_obj_t *title = lv_label_create(s_scr_setup);
    lv_label_set_text(title, "WiFi Setup");
    lv_obj_set_style_text_color(title, CLR_ACCENT, 0);
    lv_obj_set_style_text_font(title, &font_space_grotesk_28, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    /* Instructions */
    lv_obj_t *instr = lv_label_create(s_scr_setup);
    lv_label_set_text(instr, "Connect to WiFi:");
    lv_obj_set_style_text_color(instr, CLR_TEXT_SEC, 0);
    lv_obj_set_style_text_font(instr, &font_space_grotesk_14, 0);
    lv_obj_align(instr, LV_ALIGN_TOP_MID, 0, 68);

    /* AP SSID name */
    lv_obj_t *ssid = lv_label_create(s_scr_setup);
    lv_label_set_text(ssid, "Bitaxe-Dashboard-Setup");
    lv_obj_set_style_text_color(ssid, CLR_TEXT_PRI, 0);
    lv_obj_set_style_text_font(ssid, &font_space_grotesk_14, 0);
    lv_obj_align(ssid, LV_ALIGN_TOP_MID, 0, 88);

    /* Browser hint */
    lv_obj_t *hint = lv_label_create(s_scr_setup);
    lv_label_set_text(hint, "Then open browser at:");
    lv_obj_set_style_text_color(hint, CLR_TEXT_SEC, 0);
    lv_obj_set_style_text_font(hint, &font_space_grotesk_14, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 120);

    /* IP address */
    lv_obj_t *ip = lv_label_create(s_scr_setup);
    lv_label_set_text(ip, "192.168.4.1");
    lv_obj_set_style_text_color(ip, CLR_GREEN, 0);
    lv_obj_set_style_text_font(ip, &font_space_grotesk_20, 0);
    lv_obj_align(ip, LV_ALIGN_TOP_MID, 0, 142);

    /* Note about auto-redirect */
    lv_obj_t *note = lv_label_create(s_scr_setup);
    lv_label_set_text(note, "(or wait for auto-redirect)");
    lv_obj_set_style_text_color(note, CLR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(note, &font_space_grotesk_14, 0);
    lv_obj_align(note, LV_ALIGN_TOP_MID, 0, 176);

    /* Reset label (reused for reset countdown overlay) */
    s_reset_lbl = lv_label_create(s_scr_connect);
    lv_label_set_text(s_reset_lbl, "");
    lv_obj_set_style_text_color(s_reset_lbl, CLR_AMBER, 0);
    lv_obj_set_style_text_font(s_reset_lbl, &font_space_grotesk_14, 0);
    lv_obj_align(s_reset_lbl, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_text_align(s_reset_lbl, LV_TEXT_ALIGN_CENTER, 0);
}

/* ------------------------------------------------------------------ */
/*  Update functions                                                   */
/* ------------------------------------------------------------------ */

static void update_screen_main(const bitaxe_data_t *data)
{
    char buf[64];

    /* Hashrate */
    format_hashrate(data->hash_rate, buf, sizeof(buf));
    lv_label_set_text(s_hash_lbl, buf);

    /* Efficiency bar: -10% to +10% deviation from expected hashrate */
    if (data->expected_hashrate > 0.0f) {
        float deviation = ((data->hash_rate / data->expected_hashrate) - 1.0f) * 100.0f;
        /* Clamp to -10..+10, map to bar range -100..+100 */
        if (deviation > 10.0f) deviation = 10.0f;
        if (deviation < -10.0f) deviation = -10.0f;
        int32_t bar_val = (int32_t)(deviation * 10.0f);
        lv_bar_set_value(s_hash_bar, bar_val, LV_ANIM_ON);

        /* Color: green when positive, amber near zero, red when negative */
        lv_color_t bar_clr;
        if (deviation >= 2.0f) {
            bar_clr = CLR_GREEN;
        } else if (deviation >= -2.0f) {
            bar_clr = CLR_AMBER;
        } else {
            /* Blend from amber to red as deviation goes from -2% to -10% */
            uint8_t mix = (uint8_t)((-deviation - 2.0f) * 255.0f / 8.0f);
            if (mix > 255) mix = 255;
            bar_clr = lv_color_mix(CLR_RED, CLR_AMBER, mix);
        }
        lv_obj_set_style_text_color(s_hash_lbl, bar_clr, 0);
        lv_obj_set_style_bg_color(s_hash_bar, bar_clr, LV_PART_INDICATOR);
    } else {
        lv_color_t hclr = hashrate_color(data->hash_rate);
        lv_obj_set_style_text_color(s_hash_lbl, hclr, 0);
        lv_bar_set_value(s_hash_bar, 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_hash_bar, hclr, LV_PART_INDICATOR);
    }

    /* Expected hashrate comparison */
    if (data->expected_hashrate > 0.0f) {
        float eff = (data->hash_rate / data->expected_hashrate) * 100.0f;
        snprintf(buf, sizeof(buf), "exp: %.0f GH/s (%.0f%%)",
                 data->expected_hashrate, eff);
        lv_label_set_text(s_exp_hash_lbl, buf);
        lv_color_t eff_clr = eff >= 95.0f ? CLR_GREEN :
                              eff >= 85.0f ? CLR_AMBER : CLR_RED;
        lv_obj_set_style_text_color(s_exp_hash_lbl, eff_clr, 0);
    } else {
        lv_label_set_text(s_exp_hash_lbl, "");
    }

    /* Coin logo */
    coin_type_t coin = detect_coin(data->stratum_user);
    if (coin != s_last_coin) {
        s_last_coin = coin;
        if (coin == COIN_UNKNOWN) {
            lv_obj_add_flag(s_coin_img, LV_OBJ_FLAG_HIDDEN);
        } else {
            const lv_image_dsc_t *src = NULL;
            switch (coin) {
                case COIN_BTC: src = &btc_24; break;
                case COIN_BCH: src = &bch_24; break;
                case COIN_BTG: src = &btg_24; break;
                default: break;
            }
            if (src) {
                lv_image_set_src(s_coin_img, src);
                lv_obj_remove_flag(s_coin_img, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    /* Temperature */
    lv_color_t tclr = temp_color(data->temp);
    snprintf(buf, sizeof(buf), "%.1f\xc2\xb0""C", data->temp);
    lv_label_set_text(s_temp_lbl, buf);
    lv_obj_set_style_text_color(s_temp_lbl, tclr, 0);
    snprintf(buf, sizeof(buf), ICON_CHIP " %.1f\xc2\xb0""C", data->vr_temp);
    lv_label_set_text(s_vr_lbl, buf);
    lv_obj_set_style_bg_color(s_temp_accent, tclr, 0);
    lv_obj_set_style_text_color(s_temp_icon, tclr, 0);

    /* Critical temp blink control (>74.5°C) */
    if (data->temp > 74.5f) {
        if (!s_temp_critical) {
            s_temp_critical = true;
            s_temp_blink_on = true;
            lv_timer_resume(s_blink_timer);
        }
    } else {
        if (s_temp_critical) {
            s_temp_critical = false;
            lv_timer_pause(s_blink_timer);
            /* Restore full opacity */
            lv_obj_set_style_text_opa(s_temp_lbl, LV_OPA_COVER, 0);
            lv_obj_set_style_text_opa(s_temp_icon, LV_OPA_COVER, 0);
            lv_obj_set_style_text_opa(s_therm_asic_lbl, LV_OPA_COVER, 0);
        }
    }

    /* Power */
    snprintf(buf, sizeof(buf), "%.1fW", data->power);
    lv_label_set_text(s_pwr_lbl, buf);
    snprintf(buf, sizeof(buf), ICON_FAN " %d%%", data->fan_speed);
    lv_label_set_text(s_fan_lbl, buf);

    /* J/TH Efficiency (replaced shares) */
    {
        float th_s = data->hash_rate / 1000.0f;
        if (th_s > 0.001f) {
            float j_per_th = data->power / th_s;
            snprintf(buf, sizeof(buf), "%.1f", j_per_th);
            lv_label_set_text(s_acc_lbl, buf);
            lv_color_t eff_clr = j_per_th < 20.0f ? CLR_GREEN :
                                  j_per_th < 30.0f ? CLR_AMBER : CLR_RED;
            lv_obj_set_style_text_color(s_acc_lbl, eff_clr, 0);
            lv_obj_set_style_text_color(s_acc_icon, eff_clr, 0);
            lv_obj_set_style_bg_color(s_acc_accent, eff_clr, 0);
        } else {
            lv_label_set_text(s_acc_lbl, "--");
            lv_obj_set_style_text_color(s_acc_lbl, CLR_TEXT_DIM, 0);
        }
        lv_label_set_text(s_rej_lbl, "J/TH");
        lv_obj_set_style_text_color(s_rej_lbl, CLR_TEXT_DIM, 0);
    }

    /* Best diff */
    char diff_buf[32];
    format_diff(data->best_diff, diff_buf, sizeof(diff_buf));
    lv_label_set_text(s_diff_lbl, diff_buf);

    /* Voltages */
    snprintf(buf, sizeof(buf), "%.0fmV", data->voltage);
    lv_label_set_text(s_psu_lbl, buf);
    if (data->core_voltage > 0.0f) {
        snprintf(buf, sizeof(buf), "%.0fmV", data->core_voltage);
    } else {
        snprintf(buf, sizeof(buf), "--mV");
    }
    lv_label_set_text(s_asic_lbl, buf);

    /* Frequency + uptime */
    snprintf(buf, sizeof(buf), "%dMHz", data->frequency);
    lv_label_set_text(s_freq_lbl, buf);
    char up_buf[32];
    format_uptime(data->uptime_seconds, up_buf, sizeof(up_buf));
    lv_label_set_text(s_up_lbl, up_buf);
}

static void update_screen_perf(const bitaxe_data_t *data)
{
    char buf[64];

    /* Hero hashrate */
    format_hashrate(data->hash_rate, buf, sizeof(buf));
    lv_label_set_text(s_perf_hash_lbl, buf);
    lv_obj_set_style_text_color(s_perf_hash_lbl, hashrate_color(data->hash_rate), 0);

    /* vs expected */
    if (data->expected_hashrate > 0.0f) {
        snprintf(buf, sizeof(buf), "vs expected: %.0f GH/s", data->expected_hashrate);
        lv_label_set_text(s_perf_exp_lbl, buf);
    } else {
        lv_label_set_text(s_perf_exp_lbl, "");
    }

    /* Bar range = max(expected, actual, 100) */
    float bar_max = data->expected_hashrate;
    if (data->hash_rate > bar_max) bar_max = data->hash_rate;
    if (data->hash_rate_1m > bar_max) bar_max = data->hash_rate_1m;
    if (data->hash_rate_10m > bar_max) bar_max = data->hash_rate_10m;
    if (data->hash_rate_1h > bar_max) bar_max = data->hash_rate_1h;
    if (bar_max < 100.0f) bar_max = 100.0f;
    int32_t range = (int32_t)(bar_max * 1.1f);

    /* Rolling averages */
    float vals[] = {data->hash_rate_1m, data->hash_rate_10m, data->hash_rate_1h};
    lv_obj_t *bars[] = {s_perf_bar_1m, s_perf_bar_10m, s_perf_bar_1h};
    lv_obj_t *lbls[] = {s_perf_val_1m, s_perf_val_10m, s_perf_val_1h};

    for (int i = 0; i < 3; i++) {
        lv_bar_set_range(bars[i], 0, range);
        lv_bar_set_value(bars[i], (int32_t)vals[i], LV_ANIM_ON);

        format_hashrate(vals[i], buf, sizeof(buf));
        lv_label_set_text(lbls[i], buf);
    }

    /* Efficiency % */
    if (data->expected_hashrate > 0.0f) {
        float eff = (data->hash_rate / data->expected_hashrate) * 100.0f;
        snprintf(buf, sizeof(buf), "%.1f%%", eff);
        lv_label_set_text(s_perf_eff_lbl, buf);
        lv_color_t clr = eff >= 95.0f ? CLR_GREEN :
                          eff >= 85.0f ? CLR_AMBER : CLR_RED;
        lv_obj_set_style_text_color(s_perf_eff_lbl, clr, 0);
    } else {
        lv_label_set_text(s_perf_eff_lbl, "--%");
        lv_obj_set_style_text_color(s_perf_eff_lbl, CLR_TEXT_DIM, 0);
    }

    /* Error rate */
    snprintf(buf, sizeof(buf), "ERR %.2f%%", data->error_percentage);
    lv_label_set_text(s_perf_err_lbl, buf);
    lv_color_t err_clr = data->error_percentage < 1.0f ? CLR_GREEN :
                          data->error_percentage < 5.0f ? CLR_AMBER : CLR_RED;
    lv_obj_set_style_text_color(s_perf_err_lbl, err_clr, 0);

    /* Frequency */
    snprintf(buf, sizeof(buf), "%dMHz", data->frequency);
    lv_label_set_text(s_perf_freq_lbl, buf);
}

static void update_screen_thermal(const bitaxe_data_t *data)
{
    char buf[64];

    /* ASIC temp */
    lv_color_t asic_clr = temp_color(data->temp);
    snprintf(buf, sizeof(buf), "%.1f\xc2\xb0""C", data->temp);
    lv_label_set_text(s_therm_asic_lbl, buf);
    lv_obj_set_style_text_color(s_therm_asic_lbl, asic_clr, 0);

    /* VR temp */
    lv_color_t vr_clr = temp_color(data->vr_temp);
    snprintf(buf, sizeof(buf), "%.1f\xc2\xb0""C", data->vr_temp);
    lv_label_set_text(s_therm_vr_lbl, buf);
    lv_obj_set_style_text_color(s_therm_vr_lbl, vr_clr, 0);

    /* Board temp */
    if (data->temp2 > 0.0f) {
        snprintf(buf, sizeof(buf), "%.1f\xc2\xb0""C", data->temp2);
        lv_label_set_text(s_therm_t2_lbl, buf);
        lv_obj_set_style_text_color(s_therm_t2_lbl, temp_color(data->temp2), 0);
    } else {
        lv_label_set_text(s_therm_t2_lbl, "--\xc2\xb0""C");
        lv_obj_set_style_text_color(s_therm_t2_lbl, CLR_TEXT_DIM, 0);
    }

    /* Temperature bar — show max temp */
    float max_temp = data->temp;
    if (data->vr_temp > max_temp) max_temp = data->vr_temp;
    if (data->temp2 > max_temp) max_temp = data->temp2;
    lv_bar_set_value(s_therm_bar, (int32_t)max_temp, LV_ANIM_ON);
    lv_obj_set_style_bg_color(s_therm_bar, temp_color(max_temp), LV_PART_INDICATOR);

    /* Power */
    snprintf(buf, sizeof(buf), "%.1fW", data->power);
    lv_label_set_text(s_therm_pwr_lbl, buf);

    snprintf(buf, sizeof(buf), "%.0fmV", data->voltage);
    lv_label_set_text(s_therm_volt_lbl, buf);

    snprintf(buf, sizeof(buf), "%.2fA", data->current);
    lv_label_set_text(s_therm_cur_lbl, buf);

    /* J/TH efficiency */
    float th_s = data->hash_rate / 1000.0f;
    if (th_s > 0.001f) {
        float j_per_th = data->power / th_s;
        snprintf(buf, sizeof(buf), "%.1f J/TH", j_per_th);
        lv_label_set_text(s_therm_eff_lbl, buf);
        lv_color_t clr = j_per_th < 20.0f ? CLR_GREEN :
                          j_per_th < 30.0f ? CLR_AMBER : CLR_RED;
        lv_obj_set_style_text_color(s_therm_eff_lbl, clr, 0);
    } else {
        lv_label_set_text(s_therm_eff_lbl, "-- J/TH");
        lv_obj_set_style_text_color(s_therm_eff_lbl, CLR_TEXT_DIM, 0);
    }

    /* Fan */
    snprintf(buf, sizeof(buf), ICON_FAN " %d%%", data->fan_speed);
    lv_label_set_text(s_therm_fan_lbl, buf);
    lv_bar_set_value(s_therm_fan_bar, data->fan_speed, LV_ANIM_ON);

    /* Core voltage set vs actual */
    if (data->core_voltage_set > 0.0f && data->core_voltage > 0.0f) {
        snprintf(buf, sizeof(buf), "Core: %.0fmV / %.0fmV",
                 data->core_voltage_set, data->core_voltage);
        lv_label_set_text(s_therm_cv_lbl, buf);
        float diff = data->core_voltage_set - data->core_voltage;
        if (diff < 0) diff = -diff;
        lv_obj_set_style_text_color(s_therm_cv_lbl,
            diff > 20.0f ? CLR_RED : CLR_TEXT_SEC, 0);
    } else {
        lv_label_set_text(s_therm_cv_lbl, "Core: --");
        lv_obj_set_style_text_color(s_therm_cv_lbl, CLR_TEXT_DIM, 0);
    }
}

static void update_screen_mining(const bitaxe_data_t *data)
{
    char buf[64];

    /* Accepted */
    format_number_comma(data->shares_accepted, buf, sizeof(buf));
    lv_label_set_text(s_mine_acc_lbl, buf);

    /* Rejected */
    snprintf(buf, sizeof(buf), "R: %d", data->shares_rejected);
    lv_label_set_text(s_mine_rej_lbl, buf);
    lv_obj_set_style_text_color(s_mine_rej_lbl,
        data->shares_rejected > 0 ? CLR_RED : CLR_TEXT_DIM, 0);

    /* Acceptance rate */
    int total = data->shares_accepted + data->shares_rejected;
    float acc_rate = (total > 0)
        ? (data->shares_accepted * 100.0f / total)
        : 100.0f;
    snprintf(buf, sizeof(buf), "%.2f%% accepted", acc_rate);
    lv_label_set_text(s_mine_rate_lbl, buf);
    lv_color_t rate_clr = acc_rate >= 99.0f ? CLR_GREEN :
                           acc_rate >= 95.0f ? CLR_AMBER : CLR_RED;
    lv_obj_set_style_text_color(s_mine_rate_lbl, rate_clr, 0);

    lv_bar_set_value(s_mine_rate_bar, (int32_t)(acc_rate * 100), LV_ANIM_ON);
    lv_obj_set_style_bg_color(s_mine_rate_bar, rate_clr, LV_PART_INDICATOR);

    /* Best difficulty */
    char diff_buf[32];
    format_diff(data->best_diff, diff_buf, sizeof(diff_buf));
    lv_label_set_text(s_mine_bdiff_lbl, diff_buf);

    /* Pool difficulty */
    char pd_buf[32];
    format_pool_diff(data->pool_difficulty, pd_buf, sizeof(pd_buf));
    snprintf(buf, sizeof(buf), "Pool diff: %s", pd_buf);
    lv_label_set_text(s_mine_pdiff_lbl, buf);

    /* Fallback pool status */
    if (data->is_using_fallback) {
        lv_label_set_text(s_mine_fb_lbl, ICON_WARN " FALLBACK");
        lv_obj_set_style_text_color(s_mine_fb_lbl, CLR_RED, 0);
    } else {
        lv_label_set_text(s_mine_fb_lbl, ICON_CHECK " PRIMARY");
        lv_obj_set_style_text_color(s_mine_fb_lbl, CLR_GREEN, 0);
    }

    /* Uptime */
    char up_buf[32];
    format_uptime(data->uptime_seconds, up_buf, sizeof(up_buf));
    snprintf(buf, sizeof(buf), ICON_CLOCK " %s", up_buf);
    lv_label_set_text(s_mine_up_lbl, buf);
}

static void update_screen_network(const bitaxe_data_t *data)
{
    char buf[64];

    /* Pool URL */
    lv_label_set_text(s_net_pool_lbl,
                      data->stratum_url[0] ? data->stratum_url : "---");

    /* Worker */
    lv_label_set_text(s_net_worker_lbl,
                      data->stratum_user[0] ? data->stratum_user : "---");

    /* WiFi */
    snprintf(buf, sizeof(buf), ICON_SIGNAL " %s %ddBm",
             data->ssid, data->wifi_rssi);
    lv_label_set_text(s_net_wifi_lbl, buf);

    lv_color_t wifi_clr;
    if (data->wifi_rssi > -50) wifi_clr = CLR_GREEN;
    else if (data->wifi_rssi > -70) wifi_clr = CLR_AMBER;
    else wifi_clr = CLR_RED;
    lv_obj_set_style_text_color(s_net_wifi_lbl, wifi_clr, 0);

    /* WiFi signal bars */
    int filled;
    if (data->wifi_rssi > -50) filled = 5;
    else if (data->wifi_rssi > -60) filled = 4;
    else if (data->wifi_rssi > -70) filled = 3;
    else if (data->wifi_rssi > -80) filled = 2;
    else filled = 1;

    for (int i = 0; i < 5; i++) {
        if (i < filled) {
            lv_obj_set_style_bg_color(s_net_wifi_bars[i], wifi_clr, 0);
            lv_obj_set_style_bg_opa(s_net_wifi_bars[i], LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_bg_color(s_net_wifi_bars[i], CLR_TEXT_DIM, 0);
            lv_obj_set_style_bg_opa(s_net_wifi_bars[i], LV_OPA_30, 0);
        }
    }

    /* IP */
    if (data->ip_addr[0]) {
        snprintf(buf, sizeof(buf), "IP: %s", data->ip_addr);
    } else {
        snprintf(buf, sizeof(buf), "IP: %s", CONFIG_BITAXE_API_HOST);
    }
    lv_label_set_text(s_net_ip4_lbl, buf);

    /* MAC */
    if (data->mac_addr[0]) {
        snprintf(buf, sizeof(buf), "MAC: %s", data->mac_addr);
    } else {
        snprintf(buf, sizeof(buf), "MAC: ---");
    }
    lv_label_set_text(s_net_mac_lbl, buf);

    /* Hostname */
    lv_label_set_text(s_net_host_lbl,
                      data->hostname[0] ? data->hostname : "---");

    /* ASIC model */
    lv_label_set_text(s_net_asic_lbl,
                      data->asic_model[0] ? data->asic_model : "---");

    /* Firmware version */
    if (data->version[0]) {
        snprintf(buf, sizeof(buf), "FW: %s", data->version);
    } else {
        snprintf(buf, sizeof(buf), "FW: ---");
    }
    lv_label_set_text(s_net_fw_lbl, buf);

    /* AxeOS version */
    if (data->axe_os_version[0]) {
        snprintf(buf, sizeof(buf), "OS: %s", data->axe_os_version);
        lv_label_set_text(s_net_axeos_lbl, buf);
    } else {
        lv_label_set_text(s_net_axeos_lbl, "");
    }

    /* Uptime */
    char up_buf[32];
    format_uptime_full(data->uptime_seconds, up_buf, sizeof(up_buf));
    snprintf(buf, sizeof(buf), ICON_CLOCK " %s", up_buf);
    lv_label_set_text(s_net_up_lbl, buf);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

void ui_dashboard_init(lv_display_t *disp)
{
    s_disp = disp;
    build_screen_main();
    build_screen_perf();
    build_screen_thermal();
    build_screen_mining();
    build_screen_network();
    build_screen_connect();
    build_screen_setup();
    build_block_overlay();

    /* Blink timer for critical temp — 500ms interval */
    s_blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
    lv_timer_pause(s_blink_timer);

    /* Blink timer for block found — 300ms interval (fast flash) */
    s_block_blink_timer = lv_timer_create(block_blink_cb, 300, NULL);
    lv_timer_pause(s_block_blink_timer);
}

void ui_dashboard_show_connecting(void)
{
    if (s_connect_lbl) {
        lv_label_set_text(s_connect_lbl, "Connecting...");
        lv_obj_set_style_text_color(s_connect_lbl, CLR_TEXT_SEC, 0);
    }
    lv_screen_load(s_scr_connect);
}

void ui_dashboard_show_error(const char *msg)
{
    if (s_connect_lbl) {
        lv_label_set_text(s_connect_lbl, msg);
        lv_obj_set_style_text_color(s_connect_lbl, CLR_RED, 0);
    }
    lv_screen_load(s_scr_connect);
}

void ui_dashboard_update(const bitaxe_data_t *data)
{
    if (!data || !data->valid) return;

    update_screen_main(data);
    update_screen_perf(data);
    update_screen_thermal(data);
    update_screen_mining(data);
    update_screen_network(data);

    /* Block Found overlay */
    lv_obj_t *active = lv_screen_active();

    if (data->show_new_block) {
        if (active != s_block_overlay) {
            lv_screen_load(s_block_overlay);
        }
        s_block_blink_on = true;
        lv_timer_resume(s_block_blink_timer);
    } else {
        if (active == s_block_overlay) {
            /* Return to the current dashboard screen */
            lv_screen_load_anim(s_screens[s_current_screen],
                                LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
        }
        lv_timer_pause(s_block_blink_timer);
        if (s_block_lbl) {
            lv_obj_set_style_text_opa(s_block_lbl, LV_OPA_COVER, 0);
        }
    }

    /* Switch from connect screen on first valid data */
    if (active == s_scr_connect) {
        s_current_screen = 0;
        lv_screen_load_anim(s_screens[0],
                            LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
    }
}

void ui_dashboard_next_screen(void)
{
    lv_obj_t *active = lv_screen_active();
    if (active == s_scr_connect || active == s_scr_setup) return;

    /* Dismiss block overlay on touch */
    if (active == s_block_overlay) {
        lv_timer_pause(s_block_blink_timer);
        lv_screen_load_anim(s_screens[s_current_screen],
                            LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
        return;
    }

    int prev = s_current_screen;
    s_current_screen = (s_current_screen + 1) % NUM_SCREENS;

    /* Wrap: last→first goes right, otherwise left */
    lv_screen_load_anim_t anim = (prev == NUM_SCREENS - 1 && s_current_screen == 0)
        ? LV_SCR_LOAD_ANIM_MOVE_RIGHT
        : LV_SCR_LOAD_ANIM_MOVE_LEFT;

    lv_screen_load_anim(s_screens[s_current_screen], anim, 300, 0, false);
}

void ui_dashboard_show_setup_mode(void)
{
    if (s_scr_setup) {
        lv_screen_load(s_scr_setup);
    }
}

void ui_dashboard_show_reset_progress(int seconds_remaining)
{
    if (s_reset_lbl) {
        char buf[48];
        snprintf(buf, sizeof(buf), "Reset in %d...", seconds_remaining);
        lv_label_set_text(s_reset_lbl, buf);
    }
    /* Make sure connect screen is visible */
    if (lv_screen_active() != s_scr_connect) {
        lv_screen_load(s_scr_connect);
    }
}
