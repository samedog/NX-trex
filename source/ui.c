#include "ui.h"
#include "font8x8.h"

static u32 *s_buf = NULL;
static u32  s_stride = 0;

void ui_begin(u32 *buf, u32 stride)
{
    s_buf = buf;
    s_stride = stride;
}

void ui_end(void)
{
    s_buf = NULL;
    s_stride = 0;
}

void ui_pixel(int x, int y, u32 color)
{
    if (!s_buf) return;
    if (x < 0 || x >= UI_SCR_W || y < 0 || y >= UI_SCR_H) return;
    s_buf[(y * (s_stride / 4)) + x] = color;
}

void ui_hline(int x0, int x1, int y, u32 color)
{
    if (!s_buf) return;
    if (y < 0 || y >= UI_SCR_H) return;
    if (x0 < 0) x0 = 0;
    if (x1 > UI_SCR_W - 1) x1 = UI_SCR_W - 1;
    for (int x = x0; x <= x1; x++)
        s_buf[(y * (s_stride / 4)) + x] = color;
}

static void draw_char(int x, int y, char c, u32 color)
{
    unsigned char uc = (unsigned char)c;
    if (uc > 0x7E) uc = '?';
    if (uc < 0x20) uc = ' ';
    const uint8_t *glyph = font8x8_basic[uc];
    for (int row = 0; row < UI_CHAR_H; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < UI_CHAR_W; col++) {
            if (bits & (1 << col)) {
                for (int sy = 0; sy < UI_FONT_SCALE; sy++) {
                    for (int sx = 0; sx < UI_FONT_SCALE; sx++) {
                        ui_pixel(x + col * UI_FONT_SCALE + sx,
                                 y + row * UI_FONT_SCALE + sy,
                                 color);
                    }
                }
            }
        }
    }
}

void ui_text(int x, int y, const char *s, u32 color)
{
    while (*s) {
        draw_char(x, y, *s, color);
        x += UI_CELL_W;
        s++;
    }
}

int ui_text_width(const char *s)
{
    int n = 0;
    while (*s++) n++;
    return n * UI_CELL_W;
}

void ui_text_centered(int cx, int y, const char *s, u32 color)
{
    ui_text(cx - ui_text_width(s) / 2, y, s, color);
}