#pragma once
#include "bsp/m5stack_tab5.h"
#include "esp_lcd_panel_ops.h"
#include <stdint.h>

extern esp_lcd_panel_handle_t bsp_get_panel_handle(void);

static inline uint16_t st7701_lcd_width(void)  { return 720; }
static inline uint16_t st7701_lcd_height(void) { return 1280; }

static inline void st7701_lcd_draw_rgb_bitmap(int x, int y, int w, int h, const uint16_t *data)
{
    esp_lcd_panel_handle_t panel = bsp_get_panel_handle();
    if (panel && data && w > 0 && h > 0) {
        esp_lcd_panel_draw_bitmap(panel, x, y, x + w, y + h, data);
    }
}

static inline void st7701_lcd_fill_screen(uint16_t color)
{
    (void)color;
}
