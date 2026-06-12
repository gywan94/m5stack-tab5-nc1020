#include "kbd_hw.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

/* Bus / device — matches the M5Tab5-Keyboard-UserDemo wiring. */
#define KB_PORT  I2C_NUM_1
#define KB_SDA   0
#define KB_SCL   1
#define KB_ADDR  0x6D
#define KB_FREQ  400000

/* Register map (subset). */
#define REG_INT_STA       0x01
#define REG_EVENT_NUM     0x02
#define REG_KEYBOARD_MODE 0x10
#define REG_KEY_EVENT     0x20
#define REG_VERSION       0xFE
#define MODE_NORMAL       0x00

static const char *TAG = "kbd_hw";

static i2c_master_bus_handle_t s_bus = NULL;
static i2c_master_dev_handle_t s_dev = NULL;
static bool s_ok = false;

static esp_err_t rd(uint8_t reg, uint8_t *val)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, val, 1, 20);
}
static esp_err_t wr(uint8_t reg, uint8_t val)
{
    uint8_t b[2] = { reg, val };
    return i2c_master_transmit(s_dev, b, 2, 20);
}

bool kbd_hw_init(void)
{
    if (s_ok) return true;

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port          = KB_PORT,
        .sda_io_num        = KB_SDA,
        .scl_io_num        = KB_SCL,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
    };
    bus_cfg.flags.enable_internal_pullup = true;

    esp_err_t r = i2c_new_master_bus(&bus_cfg, &s_bus);
    if (r != ESP_OK) {
        ESP_LOGW(TAG, "I2C bus create failed: %s", esp_err_to_name(r));
        s_bus = NULL;
        return false;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = KB_ADDR,
        .scl_speed_hz    = KB_FREQ,
    };
    r = i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev);
    if (r != ESP_OK) {
        ESP_LOGW(TAG, "I2C add device failed: %s", esp_err_to_name(r));
        i2c_del_master_bus(s_bus);
        s_bus = NULL;
        return false;
    }

    /* Probe: read INT_STA. If the keyboard case isn't attached this NAKs. */
    uint8_t v = 0;
    if (rd(REG_INT_STA, &v) != ESP_OK) {
        ESP_LOGW(TAG, "keyboard not detected at 0x%02X (case detached?)", KB_ADDR);
        i2c_master_bus_rm_device(s_dev);
        s_dev = NULL;
        i2c_del_master_bus(s_bus);
        s_bus = NULL;
        return false;
    }

    wr(REG_KEYBOARD_MODE, MODE_NORMAL);

    uint8_t ver = 0;
    rd(REG_VERSION, &ver);
    ESP_LOGI(TAG, "M5Tab5 keyboard ready (FW 0x%02X), NORMAL mode", ver);

    s_ok = true;
    return true;
}

int kbd_hw_poll(kbd_hw_event_t *evs, int max)
{
    if (!s_ok || !evs || max <= 0) return 0;

    uint8_t count = 0;
    if (rd(REG_EVENT_NUM, &count) != ESP_OK) return 0;

    int n = 0;
    for (int i = 0; i < count && n < max; i++) {
        uint8_t e = 0xFF;
        if (rd(REG_KEY_EVENT, &e) != ESP_OK) break;
        if (e == 0xFF) break;                 /* queue drained */
        evs[n].pressed = (e & 0x80) != 0;
        evs[n].row     = (e >> 4) & 0x07;
        evs[n].col     = e & 0x0F;
        n++;
    }
    return n;
}

void kbd_hw_deinit(void)
{
    if (s_dev) { i2c_master_bus_rm_device(s_dev); s_dev = NULL; }
    if (s_bus) { i2c_del_master_bus(s_bus);       s_bus = NULL; }
    s_ok = false;
}
