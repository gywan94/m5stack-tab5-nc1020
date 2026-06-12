#include "odroid_vpad.h"
#include "vpad_font8.h"
#include "bsp/m5stack_tab5.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "esp_cache.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "vpad";

/* Panel geometry: 720x1280 portrait. Buttons live in the bottom half
 * (panel y in [PAD_Y0, 1280)). */
#define PANEL_W   720
#define PANEL_H   1280
#define PAD_Y0    640
#define PAD_H     (PANEL_H - PAD_Y0)   /* 640 */

/* Button hit rectangles in PANEL coordinates (x0,y0,x1,y1, button index).
 * D-pad zones overlap so a single finger near a corner triggers a diagonal. */
typedef struct { int x0, y0, x1, y1, btn; } vbtn_t;
static const vbtn_t VBTNS[] = {
    {110, 690, 230,  865, ODROID_INPUT_UP},
    {110, 865, 230, 1035, ODROID_INPUT_DOWN},
    { 30, 775, 175,  950, ODROID_INPUT_LEFT},
    {175, 775, 320,  950, ODROID_INPUT_RIGHT},
    {560, 790, 690,  920, ODROID_INPUT_A},
    {425, 885, 555, 1015, ODROID_INPUT_B},
    {235,1095, 360, 1165, ODROID_INPUT_SELECT},
    {385,1095, 510, 1165, ODROID_INPUT_START},
    {565,1095, 690, 1165, ODROID_INPUT_MENU},
};
#define NBTN (sizeof(VBTNS) / sizeof(VBTNS[0]))

static uint16_t *s_buf = NULL;   /* PAD: 720 x PAD_H RGB565 */
static bool s_active = false;

/* colors (RGB565) */
#define C_BG    0x18E3
#define C_DPAD  0x6B4D
#define C_A     0xF800   /* red */
#define C_B     0xFD20   /* orange */
#define C_SYS   0x8C71   /* gray */
#define C_TXT   0xFFFF

static inline void px(int lx, int ly, uint16_t c)
{
    if (lx < 0 || lx >= PANEL_W || ly < 0 || ly >= PAD_H) return;
    s_buf[ly * PANEL_W + lx] = c;
}

static void fill_rect(int lx, int ly, int w, int h, uint16_t c)
{
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            px(lx + x, ly + y, c);
}

static void fill_circle(int cx, int cy, int r, uint16_t c)
{
    for (int y = -r; y <= r; y++)
        for (int x = -r; x <= r; x++)
            if (x * x + y * y <= r * r) px(cx + x, cy + y, c);
}

/* Draw one ASCII glyph scaled by `s` at local (lx,ly). */
static void blit_char(int lx, int ly, char ch, int s, uint16_t c)
{
    if (ch < 0x20 || ch > 0x7e) return;
    const uint8_t *g = vpad_font8[ch - 0x20];
    for (int row = 0; row < 16; row++)
        for (int col = 0; col < 8; col++)
            if (g[row] & (0x80 >> col))
                fill_rect(lx + col * s, ly + row * s, s, s, c);
}

/* Draw a string centered horizontally on cx (local coords), baseline top = ly. */
static void blit_text_centered(int cx, int ly, const char *str, int s, uint16_t c)
{
    int w = (int)strlen(str) * 8 * s;
    int x = cx - w / 2;
    for (const char *p = str; *p; p++) {
        blit_char(x, ly, *p, s, c);
        x += 8 * s;
    }
}

static inline int L(int panel_y) { return panel_y - PAD_Y0; }  /* panel->local y */

void odroid_vpad_draw(void)
{
    if (!s_buf) {
        s_buf = heap_caps_aligned_calloc(64, 1, (size_t)PANEL_W * PAD_H * 2,
                                         MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
        if (!s_buf) {
            ESP_LOGE(TAG, "vpad buffer alloc failed");
            return;
        }
    }

    /* background */
    fill_rect(0, 0, PANEL_W, PAD_H, C_BG);

    /* D-pad cross (covers the UP/DOWN/LEFT/RIGHT hit zones) */
    fill_rect(110, L(700), 120, 320, C_DPAD);   /* vertical bar  */
    fill_rect(40,  L(785), 260, 150, C_DPAD);   /* horizontal bar*/

    /* A / B face buttons */
    fill_circle(625, L(855), 62, C_A);
    blit_char(625 - 12, L(855) - 24, 'A', 3, C_TXT);
    fill_circle(490, L(950), 62, C_B);
    blit_char(490 - 12, L(950) - 24, 'B', 3, C_TXT);

    /* SELECT / START / MENU */
    fill_rect(235, L(1095), 125, 70, C_SYS);
    blit_text_centered(297, L(1108), "SEL", 2, C_TXT);
    fill_rect(385, L(1095), 125, 70, C_SYS);
    blit_text_centered(447, L(1108), "START", 1, C_TXT);
    fill_rect(565, L(1095), 125, 70, C_SYS);
    blit_text_centered(627, L(1108), "MENU", 1, C_TXT);

    esp_lcd_panel_handle_t panel = bsp_get_panel_handle();
    if (panel) {
        esp_cache_msync(s_buf, (size_t)PANEL_W * PAD_H * 2, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
        esp_lcd_panel_draw_bitmap(panel, 0, PAD_Y0, PANEL_W, PANEL_H, s_buf);
    }
    s_active = true;
    ESP_LOGI(TAG, "virtual gamepad drawn (bottom half)");
}

void odroid_vpad_disable(void)
{
    s_active = false;
}

bool odroid_vpad_is_active(void)
{
    return s_active;
}

void odroid_vpad_poll(odroid_gamepad_state *state)
{
    if (!s_active) return;

    /* The game loop polls input at ~90 Hz, but the ST7123 touch controller
     * doesn't like being read over I2C that fast (the browser polled at 30 Hz
     * and worked). Rate-limit the actual touch read to ~60 Hz and reuse the
     * cached button state in between. */
    static int64_t last_us = 0;
    static int cached[ODROID_INPUT_MAX] = {0};

    int64_t now = esp_timer_get_time();
    if (now - last_us >= 16000) {
        last_us = now;
        memset(cached, 0, sizeof(cached));

        uint16_t xs[8], ys[8];
        int n = bsp_touch_read_points(xs, ys, 8);
        for (int i = 0; i < n; i++) {
            for (size_t b = 0; b < NBTN; b++) {
                if (xs[i] >= VBTNS[b].x0 && xs[i] < VBTNS[b].x1 &&
                    ys[i] >= VBTNS[b].y0 && ys[i] < VBTNS[b].y1) {
                    cached[VBTNS[b].btn] = 1;
                }
            }
        }
    }

    for (int i = 0; i < ODROID_INPUT_MAX; i++) {
        state->values[i] |= cached[i];
    }
}
