#include <switch.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <ctype.h>
#include "vecx.h"
#include "osint.h"
#include "e8910.h"
#include "font8x8.h"
#include "ui.h"
#include <sys/stat.h>
#include <errno.h>
//#include <math.h>

// menu state

enum menu_state { MENU_MAIN, MENU_CARTS, MENU_EMU };

typedef struct {
    char name[64];
    char path[300];
} cart_entry;

#define MAX_CARTS 64
#define WHITE RGBA8_MAXALPHA(255, 255, 255)

static cart_entry carts[MAX_CARTS];
static int        cart_count = 0;
static int        menu_sel   = 0;
static int        cart_top   = 0;
static int        two_players_joycons = 0;
static enum menu_state state = MENU_MAIN;

static u32 *g_buf = NULL;
static u32  g_stride = 0;

static inline unsigned isqrt32(unsigned x)
{
    unsigned r = 0;
    unsigned bit = 1u << 30;
    while (bit > x) bit >>= 2;
    while (bit != 0) {
        if (x >= r + bit) {
            x -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return r;
}

static inline unsigned curve15(unsigned n)
{
    /* want n^1.5 scaled. n^1.5 = n * sqrt(n).
     * sqrt(n) in 0..181 (since 181^2 ~= 32761).
     * n * sqrt(n) / 181 to bring it back to 0..32767. */
    unsigned r = isqrt32(n);                 /* 0..181 */
    return (unsigned)(((unsigned long)n * r) / 181);
}


// cart scanning / loading

static void scan_carts(void){
    cart_count = 0;
    DIR *d = opendir("sdmc:/NX-trex/roms");
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && cart_count < MAX_CARTS) {
        const char *n = ent->d_name;
        size_t len = strlen(n);
        if (len < 5) continue;

        const char *ext = n + len - 4;
        if (strcasecmp(ext, ".vec") != 0 && strcasecmp(ext, ".bin") != 0)
            continue;

        snprintf(carts[cart_count].path, sizeof(carts[cart_count].path),
                 "sdmc:/NX-trex/roms/%s", n);

        size_t j;
        for (j = 0; j < len && j < sizeof(carts[cart_count].name) - 1; j++)
            carts[cart_count].name[j] = (char)toupper((unsigned char)n[j]);
        carts[cart_count].name[j] = '\0';
        cart_count++;
    }
    closedir(d);
}

static int load_cart(const char *path){
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz != 4096 && sz != 8192 && sz != 16384 && sz != 32768) {
        fclose(f);
        return 0;
    }
    size_t n = fread(cart, 1, (size_t)sz, f);
    fclose(f);
    if (n != (size_t)sz) return 0;
    cart_size = (unsigned)sz;
    cart_mask = cart_size - 1;
    return 1;
}

static void clear_cart(void){
    cart_size = 0;
    cart_mask = 0;
    memset(cart, 0, sizeof(cart));
}

// menu rendering

static void render_main_menu(void){
    const char *items[] = {
        "RUN MINE STORM",
        "LOAD CART",
        two_players_joycons ? "2P MODE: ON" : "2P MODE: OFF",
        "EXIT"
    };
    const int n_items = 4;
    const int cell  = UI_CHAR_W * UI_FONT_SCALE;
    const int slot  = cell * (2 + 14);
    const int x0    = UI_SCR_W / 2 - slot / 2;

    ui_text_centered(UI_SCR_W / 2, 120, "VECTREX", WHITE);
    ui_hline(UI_SCR_W / 2 - 400, UI_SCR_W / 2 + 400, 180, WHITE);

    for (int i = 0; i < n_items; i++) {
        int y = 260 + i * 64;
        const char *prefix = (i == menu_sel) ? "> " : "  ";
        ui_text(x0,            y, prefix, WHITE);
        ui_text(x0 + 2 * cell, y, items[i], WHITE);
    }
}

static void render_cart_menu(void){
    ui_text_centered(UI_SCR_W / 2, 80, "VECTREX - CARTS", WHITE);
    ui_hline(UI_SCR_W / 2 - 400, UI_SCR_W / 2 + 400, 140, WHITE);

    const int cell     = UI_CHAR_W * UI_FONT_SCALE;
    const int slot     = cell * (2 + 24);         /* "> " + up to 24-char names */
    const int x0       = UI_SCR_W / 2 - slot / 2;
    const int top_y    = 200;
    const int row_h    = 48;
    const int max_rows = 8;
    const int back_y   = top_y + max_rows * row_h + 20;
    
    if (cart_count == 0) {
        ui_text_centered(UI_SCR_W / 2, 320, "(NO CARTS FOUND)", WHITE);
        ui_text_centered(UI_SCR_W / 2, 380, "PUT .VEC FILES IN", WHITE);
        ui_text_centered(UI_SCR_W / 2, 430, "SD:/NX-TREX/ROMS/", WHITE);
        int y = 520;
        ui_text(x0, y, "> ", WHITE);
        ui_text(x0 + 2 * cell, y, "BACK", WHITE);
        return;
    }

    if (menu_sel < cart_top) cart_top = menu_sel;
    if (menu_sel >= cart_top + max_rows) cart_top = menu_sel - max_rows + 1;

    int rows = cart_count < max_rows ? cart_count : max_rows;
    for (int i = 0; i < rows; i++) {
        int idx = cart_top + i;
        int y = top_y + i * row_h;
        const char *prefix = (idx == menu_sel) ? "> " : "  ";
        ui_text(x0,            y, prefix, WHITE);
        ui_text(x0 + 2 * cell, y, carts[idx].name, WHITE);
    }

    int back_idx = cart_count;
    const char *bprefix = (menu_sel == back_idx) ? "> " : "  ";
    ui_text(x0,            back_y, bprefix, WHITE);
    ui_text(x0 + 2 * cell, back_y, "BACK", WHITE);
}


/* Player 1 Vectrex buttons: 1P = B/A/Y/X, 2P = D-Pad Down/Right/Left/Up. */

/* ------------------------------------------------------------------ *
 * Menu input is always bound to Player 1's controller so that the whole
 * menu can be driven from a single Joy-Con in either mode.
 *
 *   1P : both Joy-Con report as one pad.
 *   2P : Player 1 is the Left Joy-Con held sideways; its analog stick is
 *        read in the rotated orientation (the same rotation map_analog()
 *        applies).
 *
 * Navigation is the left analog stick only (no D-Pad). Confirm / Back are
 * Player 1's Vectrex buttons 4 and 1 (X / B in 1P, D-Pad Up / Down in 2P).
 * ------------------------------------------------------------------ */
static u64 menu_up(void)
{
    /* Analog stick only. On the rotated Left Joy-Con, on-screen "up" is the
     * stick's right direction. */
    return two_players_joycons
        ? HidNpadButton_StickLRight
        : HidNpadButton_StickLUp;
}

static u64 menu_down(void)
{
    return two_players_joycons
        ? HidNpadButton_StickLLeft
        : HidNpadButton_StickLDown;
}

static u64 menu_confirm(void)
{
    return two_players_joycons ? HidNpadButton_Right : HidNpadButton_X;   /* Player 1's button 4 */
}

static u64 menu_back(void)
{
    return two_players_joycons ? HidNpadButton_Left : HidNpadButton_B;   /* Player 1's button 1 */
}

/* Global hotkeys, available in every state, always bound to Player 1.
 *   1P : the Joy-Con's own L / R shoulders.
 *   2P : Player 1 is the Left Joy-Con held sideways, where SL / SR are its
 *        L / R buttons, so those are used instead.
 * Minus / Plus are kept as aliases. */
static u64 sys_return(void)
{
    return two_players_joycons
        ? (HidNpadButton_LeftSL | HidNpadButton_Minus)
        : (HidNpadButton_L      | HidNpadButton_Minus);
}

static u64 sys_quit(void)
{
    return two_players_joycons
        ? (HidNpadButton_LeftSR | HidNpadButton_Plus)
        : (HidNpadButton_R      | HidNpadButton_Plus);
}


// Returns 1 if we just transitioned into MENU_EMU this frame.
static int handle_menu_input(u64 k_down){
    if (state == MENU_MAIN) {
        if (k_down & menu_up())   { if (menu_sel > 0) menu_sel--; }
        if (k_down & menu_down()) { if (menu_sel < 3) menu_sel++; }

        if (k_down & menu_confirm()) {
            if (menu_sel == 0) {
                clear_cart(); vecx_reset();
                state = MENU_EMU; return 1;
            } else if (menu_sel == 1) {
                state = MENU_CARTS; menu_sel = 0; cart_top = 0;
            } else if (menu_sel == 2) {
                two_players_joycons = !two_players_joycons;   /* toggle, stay in menu */
            }
            /* menu_sel == 3 (EXIT) handled by caller */
        }
    } else if (state == MENU_CARTS) {
        int n_items = cart_count + 1;

        if (k_down & menu_up()) {
            if (menu_sel > 0) menu_sel--;
        }
        if (k_down & menu_down()) {
            if (menu_sel < n_items - 1) menu_sel++;
        }
        if (k_down & menu_back()) {
            state = MENU_MAIN;
            menu_sel = 0;
        }

        if (k_down & menu_confirm()) {
            if (menu_sel == cart_count) {
                state = MENU_MAIN;
                menu_sel = 0;
            } else if (cart_count > 0) {
                if (load_cart(carts[menu_sel].path)) {
                    vecx_reset();
                    state = MENU_EMU;
                    return 1;
                }
                // failed: stay in menu
            }
        }
    }
    return 0;
}

/* Fill alg_jch* from a stick, optionally rotating 90° for horizontal
 * Joy-Con grip. In 2P mode, both sticks are rotated the same way:
 *   sx = -y, sy = -x
 * Set rotate = 1 for that, 0 for no rotation. */
static void map_analog(HidAnalogStickState stick, int rotate,
                       unsigned *out_x, unsigned *out_y)
{
    const int DEADZONE = 5000;

    int sx, sy;
    if (rotate == 1) {
        sx = -stick.y;
        sy = -stick.x;
    } else if (rotate == 2) {
        sx =  stick.y;
        sy = -stick.x;
    } else {
        sx =  stick.x;
        sy =  stick.y;
    }

    if (sx > -DEADZONE && sx < DEADZONE) sx = 0;
    if (sy > -DEADZONE && sy < DEADZONE) sy = 0;

    unsigned ax = (sx < 0) ? (unsigned)(-sx) : (unsigned)sx;
    unsigned ay = (sy < 0) ? (unsigned)(-sy) : (unsigned)sy;
    ax = curve15(ax);
    ay = curve15(ay);

    int cx = (sx < 0) ? -(int)ax : (int)ax;
    int cy = (sy < 0) ? -(int)ay : (int)ay;

    *out_x = (unsigned)(((cx + 32768) * 255) / 65535);
    *out_y = (unsigned)(((cy + 32768) * 255) / 65535);
}

/* Map a set of four buttons into a nibble of snd_regs[14].
 * `bits` is the button mask (already extracted by the caller),
 * `shift` is 0 for P1 (lower nibble) or 4 for P2 (upper nibble). */
static uint8_t map_buttons_into(uint8_t btns, u64 k,
                                u64 b1, u64 b2, u64 b3, u64 b4,
                                int shift)
{
    if (k & b1) btns &= ~(0x01 << shift);
    if (k & b2) btns &= ~(0x02 << shift);
    if (k & b3) btns &= ~(0x04 << shift);
    if (k & b4) btns &= ~(0x08 << shift);
    return btns;
}

// MAIN

int main(int argc, char **argv)
{
    romfsInit();
    // ensure sdmc:/NX-trex/roms exists.
    mkdir("sdmc:/NX-trex", 0755);
    mkdir("sdmc:/NX-trex/roms", 0755);
    osint_init();

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    FILE *f = fopen("romfs:/rom.dat", "rb");
    if (!f) { osint_exit(); romfsExit(); return 1; }
    size_t n = fread(rom, 1, sizeof(rom), f);
    fclose(f);
    if (n != sizeof(rom)) { osint_exit(); romfsExit(); return 1; }

    scan_carts();
    e8910_init_sound();

    const u64 TICKS_PER_FRAME = armGetSystemTickFreq() / 30;
    u64 next = armGetSystemTick();

    int exit_requested = 0;

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 k_down = padGetButtonsDown(&pad);

        if (k_down & sys_quit()) { 
            exit_requested = 1; 
        }

        if (state == MENU_EMU) {
            u64 k = padGetButtons(&pad);

            if (!two_players_joycons) {
                /* 1P: left stick, no rotation.
                * A/B/X/Y (as B,A,Y,X order) → P1 buttons 1-4. */
                map_analog(padGetStickPos(&pad, 0), 0, &alg_jch0, &alg_jch1);
                uint8_t btns = 0xFF;
                btns = map_buttons_into(btns, k, HidNpadButton_B, HidNpadButton_A, HidNpadButton_Y, HidNpadButton_X, 0);
                snd_regs[14] = btns;
            } else {
                /* 2P: both sticks rotated 90° (horizontal Joy-Con grip). */
                map_analog(padGetStickPos(&pad, 0), 1, &alg_jch0, &alg_jch1);
                map_analog(padGetStickPos(&pad, 1), 2, &alg_jch2, &alg_jch3);
                uint8_t btns = 0xFF;
                /* P1: lower nibble - Left, Down, Up, Right -> buttons 1-4 */
                btns = map_buttons_into(btns, k, HidNpadButton_Left,  HidNpadButton_Down, HidNpadButton_Up,   HidNpadButton_Right, 0);
                /* P2: upper nibble - A, X, B, Y -> buttons 1-4 */
                btns = map_buttons_into(btns, k, HidNpadButton_A, HidNpadButton_X, HidNpadButton_B, HidNpadButton_Y, 4);
                snd_regs[14] = btns;
            }

            if (k_down & sys_return()) {
                state = MENU_MAIN;
                menu_sel = 0;
            } else {
                vecx_emu(50000);   // calls osint_render internally
                e8910_update();
            }
        } else {
            // ---- menu tick
            handle_menu_input(k_down);
            if (state == MENU_MAIN && (k_down & menu_confirm()) && menu_sel == 3)
                exit_requested = 1;

            if (!exit_requested && state != MENU_EMU) {
                g_buf = osint_begin_ui(&g_stride);
                if (g_buf) {
                    ui_begin(g_buf, g_stride);
                    if (state == MENU_MAIN)       render_main_menu();
                    else if (state == MENU_CARTS) render_cart_menu();
                    ui_end();
                    osint_end_ui();
                }
                g_buf = NULL;
            }
        }

        if (exit_requested) break;

        next += TICKS_PER_FRAME;
        u64 now = armGetSystemTick();
        if (now < next) {
            u64 ns = (next - now) * 1000000000ULL / armGetSystemTickFreq();
            svcSleepThread(ns);
        } else {
            next = now;
        }
    }

    e8910_done_sound();
    osint_exit();
    romfsExit();
    return 0;
}