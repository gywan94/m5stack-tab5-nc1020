#include "rom_store.h"
#include "esp_partition.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "rom_store";

#define NCROM_SUBTYPE 0x40   /* matches partitions_ota.csv `ncrom` */

static const uint8_t *s_base = NULL;
static esp_partition_mmap_handle_t s_handle = 0;
static uint32_t s_rom_off, s_rom_z_len, s_rom_raw_len, s_nor_off, s_nor_len;
static bool s_ok = false;
static bool s_tried = false;

static void ensure(void)
{
    if (s_tried) return;
    s_tried = true;

    const esp_partition_t *p = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, (esp_partition_subtype_t)NCROM_SUBTYPE, "ncrom");
    if (!p) {
        ESP_LOGW(TAG, "ncrom partition not found — SD fallback");
        return;
    }

    const void *ptr = NULL;
    esp_err_t r = esp_partition_mmap(p, 0, p->size, ESP_PARTITION_MMAP_DATA,
                                     &ptr, &s_handle);
    if (r != ESP_OK) {
        ESP_LOGW(TAG, "ncrom mmap failed: %s", esp_err_to_name(r));
        return;
    }
    s_base = (const uint8_t *)ptr;

    if (memcmp(s_base, "NCBLOB1\0", 8) != 0) {
        ESP_LOGW(TAG, "ncrom: bad magic — partition not flashed?");
        return;
    }
    const uint32_t *h = (const uint32_t *)(s_base + 8);
    s_rom_z_len   = h[0];
    s_rom_raw_len = h[1];
    s_nor_len     = h[2];
    s_nor_off     = h[3];
    s_rom_off     = h[4];
    s_ok = true;
    ESP_LOGI(TAG, "ncrom mapped: rom_z=%u raw=%u nor=%u",
             (unsigned)s_rom_z_len, (unsigned)s_rom_raw_len, (unsigned)s_nor_len);
}

const uint8_t *rom_store_rom_z(size_t *z_len, size_t *raw_len)
{
    ensure();
    if (!s_ok) return NULL;
    if (z_len)   *z_len = s_rom_z_len;
    if (raw_len) *raw_len = s_rom_raw_len;
    return s_base + s_rom_off;
}

const uint8_t *rom_store_nor(size_t *len)
{
    ensure();
    if (!s_ok) return NULL;
    if (len) *len = s_nor_len;
    return s_base + s_nor_off;
}
