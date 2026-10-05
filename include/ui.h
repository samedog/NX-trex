#ifndef UI_H
#define UI_H

#include <switch.h>

// Framebuffer-backed drawing helpers for the menu,
// call ui_begin() with the buffer and stride returned from
// osint_begin_ui(), do your drawing, then ui_end()

void ui_begin(u32 *buf, u32 stride);
void ui_end(void);

void ui_pixel(int x, int y, u32 color);
void ui_hline(int x0, int x1, int y, u32 color);

// Text: CHAR_W * FONT_SCALE pixels wide per glyph, CHAR_H * FONT_SCALE pixels tall.
void ui_text(int x, int y, const char *s, u32 color);
int  ui_text_width(const char *s);
void ui_text_centered(int cx, int y, const char *s, u32 color);

#define UI_SCR_W      1280
#define UI_SCR_H      720
#define UI_CHAR_W     8
#define UI_CHAR_H     8
#define UI_FONT_SCALE 4
#define UI_CELL_W     (UI_CHAR_W * UI_FONT_SCALE)

#endif