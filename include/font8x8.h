#ifndef FONT8X8_H
#define FONT8X8_H

#include <stdint.h>

/* font8x8_basic - public domain, by Daniel Hepper.
 * https://github.com/dhepper/font8x8
 * Each glyph is 8 bytes, one byte per row. Bit 0 = leftmost pixel. */
extern const uint8_t font8x8_basic[128][8];

#endif