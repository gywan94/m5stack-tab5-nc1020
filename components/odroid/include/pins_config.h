#pragma once

/* ─── M5Stack Tab5 (ESP32-P4, ST7123 720x1280 MIPI-DSI) ─────────── */

/* LCD Resolution (native portrait panel) */
#define LCD_H_RES 720
#define LCD_V_RES 1280

/* LCD Reset Pin (driven via PI4IO expander, not a GPIO) */
#define LCD_RST   -1

/* LCD Backlight Pin — Tab5 backlight is GPIO22 (shared with BSP LEDC) */
#define LCD_BL_IO          22
#define LCD_BK_LIGHT_GPIO  22

/* Touch — ST7123 TDDI shares the SYS I2C bus (SDA=31, SCL=32, INT=23) */
#define TP_I2C_SDA  31
#define TP_I2C_SCL  32
#define TP_RST      -1
#define TP_INT      23

/* I2S / ES8388 Audio Codec Pins (Tab5) */
#define I2S_MCLK_IO   30
#define I2S_BCLK_IO   27
#define I2S_WS_IO     29
#define I2S_DOUT_IO   26   /* ESP32-P4 -> ES8388 SDIN */
#define I2S_DIN_IO    28   /* ES8388 DOUT -> ESP32-P4 */
#define AUDIO_PA_IO   -1   /* Power amp enable handled via PI4IO expander */

/* SD MMC Pins */
#define SD_MMC_CLK  43
#define SD_MMC_CMD  44
#define SD_MMC_D0   39
#define SD_MMC_D1   40
#define SD_MMC_D2   41
#define SD_MMC_D3   42
