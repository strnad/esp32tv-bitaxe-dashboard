idf_component_register(
    SRCS "app_main.c"
         "app_display.c"
         "app_backlight.c"
         "app_touch.c"
         "app_wifi.c"
         "app_config.c"
         "app_portal.c"
         "bitaxe_api.c"
         "ui_dashboard.c"
         "fonts/font_space_grotesk_14.c"
         "fonts/font_space_grotesk_20.c"
         "fonts/font_space_grotesk_28.c"
         "logos/btc_24.c"
         "logos/bch_24.c"
         "logos/btg_24.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_wifi nvs_flash esp_lcd driver esp_timer esp_http_client esp_http_server json
)
