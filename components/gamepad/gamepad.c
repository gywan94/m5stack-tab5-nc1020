#include "gamepad.h"
#include "esp_log.h"

static const char *TAG = "gamepad";

esp_err_t gamepad_init(const gamepad_config_t *config)
{
    (void)config;
    ESP_LOGW(TAG, "gamepad_init() stub");
    return ESP_OK;
}

void gamepad_deinit(void) {}

int gamepad_poll(void) { return 0; }

void gamepad_get_state(gamepad_state_t *state) { memset(state, 0, sizeof(*state)); }

void gamepad_get_vid_pid(uint16_t *vid, uint16_t *pid) { *vid = 0x054C; *pid = 0x05C4; }
