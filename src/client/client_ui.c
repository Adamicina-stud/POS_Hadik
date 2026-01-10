#include "client_ui.h"

#include <ncurses.h>
#include <string.h>

void ui_init(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);   // getch() nebude blokovať
  curs_set(0);             // skry kurzor
}

int ui_get_key(void) {
  int ch = getch();
  if (ch == ERR) return -1;
  return ch;
}

void ui_draw(int w, int h, int tick, int score, const char *grid_out) {
  // horný riadok: info
  erase();
  mvprintw(0, 0, "Score: %d Tick: %d  Grid: %dx%d   (q=quit)", score, tick, w, h);
  
  /*
  int max_rows = LINES - 2; // koľko riadkov sa zmestí do riadku 2
  if (max_rows < 0) max_rows = 0;
  if (h > max_rows) h = max_rows;

  int max_cols = COLS;
  if (w > max_cols) w = max_cols;
  */

  // grid kreslíme od riadku 2 (aby sme mali miesto na info)
  for (int y = 0; y < h; y++) {
    const char *row = grid_out + (size_t)y * (size_t)(w + 1);
    //mvaddnstr(2 + y, 0, row, w);
    mvprintw(2 + y, 0, "%s", row);
    clrtoeol(); // vymaže zvyšok riadku (keď sa skracuje)
  }
  

  refresh();
}

void ui_draw_waiting(void) {
  erase();
  mvprintw(0, 0, "Waiting for server frame... (pres q to quit.)");
  refresh();
}

void ui_end(void) {
    endwin();
}
