/*
 * Odroid Audio Compatibility Layer — ESP32-P4 Implementation
 *
 * Wraps the ES8311 codec audio component.
 * Audio hardware is initialized in odroid_system_init(), so this
 * just provides the volume control and PCM submission interfaces.
 */

#include "odroid_audio.h"
#include "odroid_settings.h"
#include "audio.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "odroid_audio";
static int s_sample_rate = 16000;
static odroid_volume_level s_volume_level = ODROID_VOLUME_LEVEL3;

/* Software attenuation: MUTE, 10%, 30%, 50%, 100% */
static const int volume_table[ODROID_VOLUME_LEVEL_COUNT] = {0, 100, 300, 500, 1000};
static const int volume_pct_table[ODROID_VOLUME_LEVEL_COUNT] = {0, 10, 30, 50, 100};

void odroid_audio_init(int sample_rate)
{
    s_sample_rate = sample_rate;

    esp_err_t ret = audio_set_sample_rate(sample_rate);
    ESP_LOGI(TAG, "odroid_audio_init: sample_rate=%d, ret=%d", sample_rate, ret);

    odroid_volume_level level = ODROID_VOLUME_LEVEL3; /* default 50% */
    int32_t stored = odroid_settings_Volume_get();
    if (stored >= 1 && stored < ODROID_VOLUME_LEVEL_COUNT) {
        level = (odroid_volume_level)stored;
    }
    s_volume_level = level;
    odroid_audio_volume_set(level);
    ESP_LOGI(TAG, "Audio init: vol=%d (stored=%ld)", level, (long)stored);
}

void odroid_audio_terminate(void)
{
    audio_stop();
    ESP_LOGI(TAG, "Audio terminated");
}

void odroid_audio_volume_set(int volume)
{
    if (volume < 0) volume = 0;
    if (volume >= ODROID_VOLUME_LEVEL_COUNT) volume = ODROID_VOLUME_LEVEL_COUNT - 1;
    s_volume_level = (odroid_volume_level)volume;

    int pct = volume_pct_table[volume];
    audio_set_volume(pct);
    ESP_LOGI(TAG, "Volume set: %d/%d → %d%%", volume, ODROID_VOLUME_LEVEL_COUNT - 1, pct);
}

odroid_volume_level odroid_audio_volume_get(void)
{
    return s_volume_level;
}

void odroid_audio_volume_change(void)
{
    int level = (s_volume_level + 1) % ODROID_VOLUME_LEVEL_COUNT;
    odroid_audio_volume_set(level);
    odroid_settings_Volume_set(level);
}

void odroid_audio_submit(short *stereoAudioBuffer, int frameCount)
{
    if (!stereoAudioBuffer || frameCount <= 0) return;

    int total_samples = frameCount * 2; /* stereo: L, R, L, R, ... */

    /* Apply software volume attenuation (integer math: vol_num/1000) */
    int vol_num = volume_table[s_volume_level];
    if (vol_num < 1000) {
        for (int i = 0; i < total_samples; ++i) {
            stereoAudioBuffer[i] = (short)(((int)stereoAudioBuffer[i] * vol_num) / 1000);
        }
    }

    size_t len = total_samples * sizeof(short);
    audio_play_pcm(stereoAudioBuffer, len, s_sample_rate);
}

void odroid_audio_submit_zero(void)
{
    /* Submit a small silent buffer to clear any DMA queues */
    short silence[512];
    memset(silence, 0, sizeof(silence));
    audio_play_pcm(silence, sizeof(silence), s_sample_rate);
}

int odroid_audio_sample_rate_get(void)
{
    return s_sample_rate;
}
