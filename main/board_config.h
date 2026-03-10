/*
 * NM-TV-154 Board Configuration
 *
 * Hardware: ESP32-WROOM-32 (ESP32-D0), 4MB Flash, 240MHz dual-core
 * Display:  1.54" IPS TFT, ST7789 controller, 240x240 RGB565
 * Input:    Capacitive touch button (top), Boot button (GPIO 0)
 * Power:    5V USB-C, ~0.8W
 *
 * Pin assignments determined by reverse engineering the original NMMiner
 * firmware and confirmed against the official NMMiner development guide:
 * https://www.nmminer.com/2026/03/02/how-to-develop-nm-tv-custom-firmware/
 */

#pragma once

#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "hal/lcd_types.h"

/* ------------------------------------------------------------------ */
/*  Display: ST7789 over SPI (HSPI bus = SPI2_HOST)                   */
/*  Pins 13/14/15 are HSPI native IOMUX — max throughput, no GPIO     */
/*  matrix routing overhead.                                          */
/* ------------------------------------------------------------------ */
#define BOARD_LCD_SPI_HOST          SPI2_HOST
#define BOARD_LCD_PIN_MOSI          13
#define BOARD_LCD_PIN_SCLK          14
#define BOARD_LCD_PIN_CS            15
#define BOARD_LCD_PIN_DC            2
#define BOARD_LCD_PIN_MISO          (-1)    /* Not connected */
#define BOARD_LCD_PIN_RST           (-1)    /* Tied to EN — always on, driver uses SW reset */

#define BOARD_LCD_H_RES             240
#define BOARD_LCD_V_RES             240
#define BOARD_LCD_SPI_FREQ_HZ       (40 * 1000 * 1000)
#define BOARD_LCD_CMD_BITS          8
#define BOARD_LCD_PARAM_BITS        8
#define BOARD_LCD_COLOR_ORDER       LCD_RGB_ELEMENT_ORDER_RGB
#define BOARD_LCD_BITS_PER_PIXEL    16

/* ------------------------------------------------------------------ */
/*  Backlight: GPIO 19, ACTIVE LOW (LOW = full brightness)            */
/*  Display power: GPIO 21, ACTIVE LOW (LOW = display powered on)     */
/*  IMPORTANT: Drive GPIO 21 LOW before any SPI communication!        */
/* ------------------------------------------------------------------ */
#define BOARD_LCD_PIN_BL            19
#define BOARD_LCD_BL_ON_LEVEL       0       /* LOW = backlight ON */
#define BOARD_LCD_BL_OFF_LEVEL      1

#define BOARD_LCD_PIN_PWR           21
#define BOARD_LCD_PWR_ON_LEVEL      0       /* LOW = power ON */
#define BOARD_LCD_PWR_OFF_LEVEL     1

/* ------------------------------------------------------------------ */
/*  Backlight PWM (LEDC)                                              */
/*  Since BL is active LOW, duty is inverted:                         */
/*    100% brightness → duty = 0  (pin held LOW)                      */
/*      0% brightness → duty = max (pin held HIGH)                    */
/* ------------------------------------------------------------------ */
#define BOARD_BL_LEDC_TIMER         LEDC_TIMER_0
#define BOARD_BL_LEDC_MODE          LEDC_LOW_SPEED_MODE
#define BOARD_BL_LEDC_CHANNEL       LEDC_CHANNEL_0
#define BOARD_BL_LEDC_DUTY_RES      LEDC_TIMER_13_BIT   /* 0–8191 */
#define BOARD_BL_LEDC_FREQ_HZ       5000

/* ------------------------------------------------------------------ */
/*  Capacitive touch button (top of device)                           */
/*  Uses ESP32 built-in touch_pad peripheral on T9 = GPIO 32.        */
/*  Confirmed by on-device diagnostic with live display readout.      */
/*                                                                    */
/*  Standard ESP32 behavior — value DROPS on touch:                   */
/*    Not touching: ~797    Touching: ~570                            */
/*  Threshold = 680 (midpoint). Detect touch when value < threshold.  */
/* ------------------------------------------------------------------ */
#define BOARD_TOUCH_PAD_NUM         CONFIG_NMTV_TOUCH_PAD_NUM
#define BOARD_TOUCH_THRESHOLD       680

/* ------------------------------------------------------------------ */
/*  Boot button                                                       */
/* ------------------------------------------------------------------ */
#define BOARD_BOOT_BUTTON_GPIO      0

/* ------------------------------------------------------------------ */
/*  LVGL tuning                                                       */
/* ------------------------------------------------------------------ */
#define BOARD_LVGL_DRAW_BUF_LINES   20
#define BOARD_LVGL_TICK_PERIOD_MS    2
#define BOARD_LVGL_TASK_STACK_SIZE   (6 * 1024)
#define BOARD_LVGL_TASK_PRIORITY     2
#define BOARD_LVGL_TASK_MAX_DELAY_MS 500
#define BOARD_LVGL_TASK_MIN_DELAY_MS (1000 / CONFIG_FREERTOS_HZ)

/* ------------------------------------------------------------------ */
/*  BitAxe polling task                                                */
/* ------------------------------------------------------------------ */
#define BOARD_POLL_TASK_STACK_SIZE   (8 * 1024)
#define BOARD_POLL_TASK_PRIORITY     4
