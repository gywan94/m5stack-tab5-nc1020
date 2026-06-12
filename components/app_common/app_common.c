#include "app_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

#include "bsp/m5stack_tab5.h"
#include "odroid_display.h"
#include "odroid_sdcard.h"
#include "ppa_engine.h"
#include "audio.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "esp_cache.h"

extern esp_err_t bsp_board_init(void);

/* Paint the DSI panel black the instant it's up — the single scan-out
 * framebuffer (num_fbs=1) otherwise shows uninitialized PSRAM (the boot-time
 * "flicker") until the emulator first draws ~7s later. */
static void app_clear_panel_black(void)
{
    esp_lcd_panel_handle_t panel = bsp_get_panel_handle();
    if (!panel) return;
    size_t sz = (size_t)720 * 1280 * 2;
    uint16_t *buf = heap_caps_aligned_calloc(64, 1, sz,
                                             MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    if (!buf) return;
    esp_cache_msync(buf, sz, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    esp_lcd_panel_draw_bitmap(panel, 0, 0, 720, 1280, buf);
    heap_caps_free(buf);
}

static const char *TAG = "app_common";

#define CRASH_GUARD_NVS_NS  "app_guard"
#define CRASH_GUARD_NVS_KEY "boot_cnt"
#define CRASH_GUARD_MAX     3

static void crash_guard_check(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(CRASH_GUARD_NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "crash_guard: can't open NVS ns, skipping");
        return;
    }

    uint8_t cnt = 0;
    nvs_get_u8(h, CRASH_GUARD_NVS_KEY, &cnt);
    ESP_LOGI(TAG, "crash_guard: boot_count = %u (max %u)", cnt, CRASH_GUARD_MAX);

    if (cnt >= CRASH_GUARD_MAX) {
        ESP_LOGE(TAG, "crash_guard: %u consecutive crashes, rolling back!", cnt);
        nvs_set_u8(h, CRASH_GUARD_NVS_KEY, 0);
        nvs_commit(h);
        nvs_close(h);
        app_return_to_launcher();
    }

    nvs_set_u8(h, CRASH_GUARD_NVS_KEY, cnt + 1);
    nvs_commit(h);
    nvs_close(h);
}

static void crash_guard_clear(void)
{
    nvs_handle_t h;
    if (nvs_open(CRASH_GUARD_NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, CRASH_GUARD_NVS_KEY, 0);
        nvs_commit(h);
        nvs_close(h);
        ESP_LOGI(TAG, "crash_guard: boot counter cleared");
    }
}

static void crash_guard_timer_cb(TimerHandle_t xTimer)
{
    crash_guard_clear();
    xTimerDelete(xTimer, 0);
}

void app_init(void)
{
    ESP_LOGI(TAG, "=== Emulator App Init (Tab5) ===");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    crash_guard_check();

    ESP_ERROR_CHECK(bsp_board_init());

    /* Kill the boot-time flicker immediately (before the slow init below). */
    app_clear_panel_black();

    /* PPA 2D engine powers every display_flush() (rotate/scale). Without it,
     * s_ppa_srm_client stays NULL and every flush returns ESP_ERR_INVALID_ARG,
     * so nothing is ever drawn. (Previously only initialized inside the unused
     * odroid_system_init(); app_init() calls bsp_board_init() directly.) */
    ESP_ERROR_CHECK(ppa_engine_init());

    /* Audio: I2S + ES8388 codec on the shared SYS I2C bus. Like ppa_engine_init,
     * this was previously only done in the unused odroid_system_init(), so there
     * was no sound. Tab5 I2S pins: MCLK30 BCLK27 WS29 DOUT26 DIN28. */
    audio_config_t acfg = AUDIO_CONFIG_DEFAULT();
    acfg.i2s_num     = 1;
    acfg.mclk_io     = 30;
    acfg.bclk_io     = 27;
    acfg.ws_io       = 29;
    acfg.dout_io     = 26;
    acfg.din_io      = 28;
    acfg.pa_ctrl_io  = -1;
    acfg.sample_rate = 48000;   /* matching UserDemo */
    acfg.volume      = 100;
    acfg.i2c_handle  = bsp_get_sys_i2c();
    ESP_LOGW(TAG, "Audio: calling audio_init(I2S1, 48kHz, vol=100)...");
    if (audio_init(&acfg) != ESP_OK) {
        ESP_LOGW(TAG, "audio_init failed — continuing without sound");
    }

    ili9341_init();

    /* No SD here — NC1020 runs entirely from flash (ncrom + /data). SD-mode
     * builds open the card themselves in nc1020_run(). */

    ESP_LOGI(TAG, "=== App Init Complete ===");

    /* We reached a healthy init. Schedule the crash-guard boot counter to be
     * cleared after a short grace period (the cb deletes the timer). Without
     * this the counter only ever increments and eventually forces a rollback. */
    TimerHandle_t clr = xTimerCreate("crash_clr", pdMS_TO_TICKS(5000), pdFALSE,
                                     NULL, crash_guard_timer_cb);
    if (clr) xTimerStart(clr, 0);

    app_check_safe_boot();
}

void app_check_safe_boot(void)
{
    ESP_LOGI(TAG, "Safe-boot: booting directly (no touch check)");
    vTaskDelay(pdMS_TO_TICKS(200));
}

int app_get_rom_path(char *buf, int buflen)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open("rom", NVS_READONLY, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Can't open ROM NVS namespace");
        buf[0] = '\0';
        return -1;
    }

    size_t len = buflen;
    err = nvs_get_str(h, "path", buf, &len);
    nvs_close(h);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No ROM path in NVS");
        buf[0] = '\0';
        return -1;
    }

    ESP_LOGI(TAG, "ROM path: %s", buf);
    return 0;
}

void app_return_to_launcher(void)
{
    ESP_LOGI(TAG, "Returning to launcher...");

    const esp_partition_t *factory = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (factory != NULL) {
        esp_ota_set_boot_partition(factory);
    }

    esp_restart();
    while (1) {}
}
