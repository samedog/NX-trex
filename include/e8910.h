#ifndef E8910_H
#define E8910_H
void e8910_init_sound(void);
void e8910_done_sound(void);
void e8910_write(int r, int v);
void e8910_update(void);
#endif