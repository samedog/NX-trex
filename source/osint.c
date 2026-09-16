
#include <switch.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "osint.h"
#include "vecx.h"

static Framebuffer fb;
static NWindow *win;

static u8 gamma_lut[128];


void osint_init(void)
{
    win = nwindowGetDefault();
    framebufferCreate(&fb, win, 1280, 720, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);

    for (int i = 0; i < 128; i++) {
        float x = (float)i / 127.0f;
        /* x^0.7 approximated: 0.3*x + 0.7*sqrt(x). Cheap and close enough. */
        float y = 0.3f * x + 0.7f * sqrtf(x);
        gamma_lut[i] = (u8)(y * 255.0f + 0.5f);
    }
}

void osint_exit(void)
{
    framebufferClose(&fb);
}

static inline void put_pixel(u32 *buf, u32 stride, int x, int y, u32 color)
{
    if (x < 0 || x >= 1280 || y < 0 || y >= 720) return;
    buf[(y * (stride / 4)) + x] = color;
}

static void draw_line(u32 *buf, u32 stride, int x0, int y0, int x1, int y1, u32 color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        put_pixel(buf, stride, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void osint_render(void)
{
    u32 stride;
    u32 *buf = (u32 *)framebufferBegin(&fb, &stride);
    if (!buf) return;
    memset(buf, 0, stride * 720);

    float scale_x = (float)1280 / ALG_MAX_X;
    float scale_y = (float)720  / ALG_MAX_Y;
    float scale = (scale_y < scale_x) ? scale_y : scale_x;
    int offx = (1280 - (int)(ALG_MAX_X * scale)) / 2;
    int offy = (720  - (int)(ALG_MAX_Y * scale)) / 2;

    for (long v = 0; v < vector_draw_cnt; v++) {
        int x0 = offx + (int)(vectors_draw[v].x0 * scale);
        int y0 = offy + (int)(vectors_draw[v].y0 * scale);
        int x1 = offx + (int)(vectors_draw[v].x1 * scale);
        int y1 = offy + (int)(vectors_draw[v].y1 * scale);
        
        unsigned char c = vectors_draw[v].color;
        if (c >= 128) c = 127;
        u32 color = RGBA8_MAXALPHA(gamma_lut[c], gamma_lut[c], gamma_lut[c]);

        draw_line(buf, stride, x0, y0, x1, y1, color);
    }

    framebufferEnd(&fb);
}

u32 *osint_begin_ui(u32 *stride)
{
    u32 s;
    u32 *buf = (u32 *)framebufferBegin(&fb, &s);
    if (stride) *stride = s;
    if (!buf) return NULL;
    memset(buf, 0, s * 720);
    return buf;
}

void osint_end_ui(void)
{
    framebufferEnd(&fb);
}