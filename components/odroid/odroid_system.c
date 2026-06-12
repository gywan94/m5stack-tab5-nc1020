#include "odroid_system.h"
#include "pins_config.h"
#include "bsp/m5stack_tab5.h"
#include "ppa_engine.h"
#include "gamepad.h"
#include "audio.h"

#include <string.h>
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "odroid_system";
static bool s_initialized = false;

void odroid_system_init(void)
{
    if (s_initialized) return;

    ESP_LOGI(TAG, "=== Tab5 System Init ===");

    bsp_board_init();

    ESP_LOGI(TAG, "Initializing PPA 2D engine...");
    ESP_ERROR_CHECK(ppa_engine_init());

    ESP_LOGI(TAG, "Initializing USB HID gamepad...");
    gamepad_config_t gp_cfg = GAMEPAD_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(gamepad_init(&gp_cfg));

    ESP_LOGI(TAG, "Initializing audio...");
    audio_config_t audio_cfg = AUDIO_CONFIG_DEFAULT();
    audio_cfg.mclk_io    = I2S_MCLK_IO;
    audio_cfg.bclk_io    = I2S_BCLK_IO;
    audio_cfg.ws_io      = I2S_WS_IO;
    audio_cfg.dout_io    = I2S_DOUT_IO;
    audio_cfg.din_io     = I2S_DIN_IO;
    audio_cfg.pa_ctrl_io = AUDIO_PA_IO;
    audio_cfg.i2c_handle = NULL;
    audio_cfg.sample_rate = 16000;
    audio_cfg.volume     = 60;
    audio_init(&audio_cfg);

    s_initialized = true;
    ESP_LOGI(TAG, "=== System Init Complete ===");
}

void *odroid_system_get_i2c_bus(void)
{
    return NULL;
}

void odroid_system_application_set(int slot)
{
    ESP_LOGW(TAG, "Application set to slot %d (stub)", slot);
}

void odroid_system_sleep(void)
{
    ESP_LOGW(TAG, "odroid_system_sleep() stub");
}

void odroid_system_led_set(int value)
{
    (void)value;
}
