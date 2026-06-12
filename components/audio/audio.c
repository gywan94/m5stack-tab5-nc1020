#include "audio.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "bsp/m5stack_tab5.h"

static const char *TAG = "audio";

static i2s_chan_handle_t       s_tx     = NULL;
static i2s_chan_handle_t       s_rx     = NULL;
static const audio_codec_data_if_t *s_data_if = NULL;
static esp_codec_dev_handle_t  s_codec  = NULL;
static int                     s_rate   = 0;

esp_err_t audio_init(const audio_config_t *config)
{
    if (s_codec) return ESP_OK;
    if (!config || !config->i2c_handle) {
        ESP_LOGE(TAG, "missing I2C bus handle");
        return ESP_ERR_INVALID_ARG;
    }

    /* I2S: TX + RX (master, auto-clear), matching UserDemo */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(config->i2s_num, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    if (i2s_new_channel(&chan_cfg, &s_tx, &s_rx) != ESP_OK) {
        /* BSP already owns this I2S controller — reuse its codec handle */
        ESP_LOGW(TAG, "I2S%d occupied by BSP — reusing BSP codec handle", config->i2s_num);
        s_codec = bsp_audio_get_play_dev();
        if (!s_codec) { ESP_LOGE(TAG, "BSP codec not available"); return ESP_FAIL; }
        s_rate = config->sample_rate;
        audio_set_sample_rate(config->sample_rate);
        esp_codec_dev_set_out_mute(s_codec, false);
        esp_codec_dev_set_out_vol(s_codec, config->volume);
        ESP_LOGI(TAG, "Audio via BSP codec: %dHz, vol=%d", s_rate, config->volume);
        return ESP_OK;
    }

    /* TX: STD mono 16-bit @ sample_rate */
    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(config->sample_rate),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = config->mclk_io, .bclk = config->bclk_io,
            .ws   = config->ws_io,   .dout = config->dout_io,
            .din  = config->din_io,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_tx, &std_cfg), TAG, "tx_std");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_tx), TAG, "tx_en");

    /* RX: TDM 4-slot 16-bit @ 48000 (ES7210 4-mic, not used but needed for I2S clock) */
    i2s_tdm_config_t tdm_cfg = {
        .clk_cfg = {
            .sample_rate_hz  = 48000,
            .clk_src         = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple   = I2S_MCLK_MULTIPLE_256,
            .bclk_div        = 8,
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode      = I2S_SLOT_MODE_STEREO,
            .slot_mask      = I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2 | I2S_TDM_SLOT3,
            .ws_width       = I2S_TDM_AUTO_WS_WIDTH,
            .ws_pol         = false,
            .bit_shift      = true,
            .left_align     = false,
            .big_endian     = false,
            .bit_order_lsb  = false,
            .skip_mask      = false,
            .total_slot     = I2S_TDM_AUTO_SLOT_NUM,
        },
        .gpio_cfg = {
            .mclk = config->mclk_io, .bclk = config->bclk_io,
            .ws   = config->ws_io,   .dout = config->dout_io,
            .din  = config->din_io,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_tdm_mode(s_rx, &tdm_cfg), TAG, "rx_tdm");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_rx), TAG, "rx_en");

    /* Codec data interface (I2S) */
    audio_codec_i2s_cfg_t i2s_dif_cfg = {
        .port = config->i2s_num,
        .tx_handle = s_tx,
        .rx_handle = s_rx,
    };
    s_data_if = audio_codec_new_i2s_data(&i2s_dif_cfg);
    if (!s_data_if) { ESP_LOGE(TAG, "i2s data_if failed"); return ESP_FAIL; }

    /* GPIO interface (UserDemo creates this even with pa_pin=-1) */
    audio_codec_new_gpio();

    /* I2C control interface */
    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = 0,
        .addr = ES8388_CODEC_DEFAULT_ADDR,
        .bus_handle = config->i2c_handle,
    };
    const audio_codec_ctrl_if_t *ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    if (!ctrl_if) { ESP_LOGE(TAG, "i2c ctrl_if failed"); return ESP_FAIL; }

    /* HW gain (matching UserDemo) */
    esp_codec_dev_hw_gain_t gain = {
        .pa_voltage        = 5.0,
        .codec_dac_voltage = 3.3,
    };

    es8388_codec_cfg_t es_cfg = {
        .codec_mode  = ESP_CODEC_DEV_WORK_MODE_DAC,
        .master_mode = false,
        .ctrl_if     = ctrl_if,
        .pa_pin      = -1,
        .hw_gain     = gain,
    };
    const audio_codec_if_t *es_dev = es8388_codec_new(&es_cfg);
    if (!es_dev) { ESP_LOGE(TAG, "es8388_codec_new failed"); return ESP_FAIL; }

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = es_dev,
        .data_if  = s_data_if,
    };
    s_codec = esp_codec_dev_new(&dev_cfg);
    if (!s_codec) { ESP_LOGE(TAG, "esp_codec_dev_new failed"); return ESP_FAIL; }

    /* Reopen with final parameters (matching UserDemo bsp_codec_es8388_set) */
    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
        .channel         = 2,
        .sample_rate     = config->sample_rate,
    };
    esp_codec_dev_close(s_codec);
    ESP_RETURN_ON_ERROR(esp_codec_dev_open(s_codec, &fs), TAG, "codec_open");
    s_rate = config->sample_rate;

    esp_codec_dev_set_out_mute(s_codec, false);
    esp_codec_dev_set_out_vol(s_codec, config->volume);
    ESP_LOGI(TAG, "ES8388 ready: I2S%d %dHz stereo, vol=%d", config->i2s_num, s_rate, config->volume);

    /* Quick diagnostic: write directly to I2S TX to test data path */
    {
        vTaskDelay(pdMS_TO_TICKS(200));
        int16_t test[128];
        for (int i = 0; i < 64; i++) {
            int16_t v = (i < 32) ? 15000 : -15000;
            test[i*2] = v; test[i*2+1] = v;
        }
        size_t written = 0;
        esp_err_t ret = i2s_channel_write(s_tx, test, sizeof(test), &written, 1000);
        ESP_LOGW(TAG, "DIAG: i2s_channel_write ret=%d, written=%d (tx=%p)", ret, (int)written, (void*)s_tx);
    }
    return ESP_OK;
}

void audio_deinit(void)
{
    if (s_codec) { esp_codec_dev_close(s_codec); esp_codec_dev_delete(s_codec); s_codec = NULL; }
    if (s_tx) { i2s_channel_disable(s_tx); i2s_del_channel(s_tx); s_tx = NULL; }
    if (s_rx) { i2s_channel_disable(s_rx); i2s_del_channel(s_rx); s_rx = NULL; }
    s_data_if = NULL;
    s_rate = 0;
}

void audio_play(const int16_t *data, size_t len)
{
    audio_play_pcm(data, len * sizeof(int16_t), s_rate);
}

esp_err_t audio_set_volume(int vol)
{
    if (!s_codec) return ESP_ERR_INVALID_STATE;
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;
    esp_codec_dev_set_out_mute(s_codec, vol == 0);
    return esp_codec_dev_set_out_vol(s_codec, vol);
}

esp_err_t audio_set_sample_rate(int rate)
{
    if (!s_codec) return ESP_ERR_INVALID_STATE;
    if (rate == s_rate) return ESP_OK;
    esp_codec_dev_close(s_codec);
    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16, .channel = 1, .sample_rate = rate,
    };
    esp_err_t r = esp_codec_dev_open(s_codec, &fs);
    if (r == ESP_OK) s_rate = rate;
    return r;
}

void audio_reset_sample_rate(void) { s_rate = -1; }

esp_err_t audio_stop(void) { return ESP_OK; }

esp_err_t audio_play_tone(uint32_t freq_hz, uint32_t duration_ms, int volume)
{
    (void)freq_hz; (void)duration_ms; (void)volume;
    return ESP_OK;
}

esp_err_t audio_play_pcm(const void *data, size_t len, int sample_rate)
{
    if (!s_codec || !data || len == 0) return ESP_ERR_INVALID_STATE;
    if (sample_rate != s_rate) audio_set_sample_rate(sample_rate);
    return esp_codec_dev_write(s_codec, (void *)data, len);
}
