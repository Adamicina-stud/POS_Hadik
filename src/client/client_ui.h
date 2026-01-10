#ifndef CLIENT_UI_H
#define CLIENT_UI_H

#include <stddef.h>

// Inicializuje ncurses
void ui_init(void);

// Vykreslí frame: tick + grid (grid_out je buffer z client_recv_frame)
void ui_draw(const char *name,int w, int h, int tick, int score, const char *grid_out, int paused);

void ui_draw_waiting(void);

// Uprace ncurses
void ui_end(void);

// Prečíta klávesu bez blokovania (vyžaduje nodelay)
// vráti: -1 ak nič, inak znak
int ui_get_key(void);

#endif
