#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int i2s_num;           /*!< I2S port number (0 or 1) */
    int mclk_io;           /*!< MCLK GPIO */
    int bclk_io;           /*!< BCLK (SCLK) GPIO */
    int ws_io;             /*!< WS (LRCK) GPIO */
    int dout_io;           /*!< Data Out GPIO (to ES8388 SDIN) */
    int din_io;            /*!< Data In GPIO (from ES8388 DOUT) */
    int pa_ctrl_io;        /*!< Power amplifier control GPIO (-1 to disable) */
    int sample_rate;       /*!< Sample rate in Hz */
    int volume;            /*!< Initial volume 0-100 */
    i2c_master_bus_handle_t i2c_handle; /*!< Existing I2C bus handle */
} audio_config_t;

#define AUDIO_CONFIG_DEFAULT() { \
    .i2s_num = 1,               \
    .mclk_io = 30,              \
    .bclk_io = 27,              \
    .ws_io = 29,                \
    .dout_io = 26,              \
    .din_io = 28,               \
    .pa_ctrl_io = -1,           \
    .sample_rate = 48000,       \
    .volume = 80,               \
    .i2c_handle = NULL,         \
}

esp_err_t audio_init(const audio_config_t *config);
esp_err_t audio_play_tone(uint32_t freq_hz, uint32_t duration_ms, int volume);
esp_err_t audio_stop(void);
esp_err_t audio_set_volume(int volume);
esp_err_t audio_set_sample_rate(int sample_rate);
void audio_reset_sample_rate(void);
esp_err_t audio_play_pcm(const void *data, size_t len, int sample_rate);

#ifdef __cplusplus
}
#endif
