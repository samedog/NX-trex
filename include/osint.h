#ifndef OSINT_H
#define OSINT_H

#include <switch.h>

/* Platform interface for the vecx core.
 * Implement these for each target. */

void osint_init(void);      /* set up framebuffer/window  */
void osint_exit(void);      /* tear down                  */
void osint_render(void);    /* draw vectors_draw[]        */

/* Framebuffer passthrough for menu / overlays.
 * osint_begin_ui() returns a writable u32* buffer (already cleared to
 * black) and stores its stride in *stride. osint_end_ui() presents it.
 * Both are safe no-ops if the framebuffer is unavailable. */
u32 *osint_begin_ui(u32 *stride);
void osint_end_ui(void);

#endif