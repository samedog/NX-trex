#ifndef __E6809_H
#define __E6809_H

#include <stdint.h>

/* ---- Memory / instruction hooks (function pointers) ---- */

extern unsigned char (*e6809_read8)(unsigned address);
extern void          (*e6809_write8)(unsigned address, unsigned char data);

/* Called once per instruction, before decode. pc = address of opcode.
 * Set to NULL if you don't need it. */
extern void          (*e6809_instruction)(unsigned address);

/* ---- CPU API ---- */

void     e6809_reset(void);
unsigned e6809_sstep(unsigned irq_i, unsigned irq_f);

struct reg6809 {
    uint16_t pc;
    uint16_t x, y, u, s;
    uint8_t  a, b, dp, cc;
};

struct reg6809 *e6809_get_regs(void);

/* Optional: enable/disable stderr tracing of interrupts and bad opcodes. */
void e6809_set_trace(int on);

#endif