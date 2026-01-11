#include "client_ui.h"
#include "client_input.h"

#include <ncurses.h>
#include <string.h>

void ui_init(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);   // getch() nebude blokovať
  curs_set(0);             // skry kurzor
  
  if (has_colors()) {
    start_color();            // pre farby
    use_default_colors();

    init_pair(1, COLOR_GREEN,  -1);  // telo hada
    init_pair(2, COLOR_YELLOW, -1);  // hlava hada
    init_pair(3, COLOR_RED,    -1);  // jedlo
    init_pair(4, COLOR_BLUE,   -1);  // stena / border
    init_pair(5, COLOR_WHITE,  -1);  // prázdno (alebo nechaj default) 
  }
}

int ui_get_key(void) {
  int ch = getch();
  if (ch == ERR) return -1;
  return ch;
}

void ui_draw(const char *name, int w, int h, int paused, int score, int time, const char *grid_out) {
  // horný riadok: info
  erase();
  if (paused >= 3) {
    mvprintw(1, 2, "%s \t\t\tGAME PAUSED", name);
  }
  else if (paused > 0) {
    mvprintw(1, 2, "%s \t\t\t     %d", name, paused);
  }
  else {
    mvprintw(1, 2, "%s", name);
  }
  mvprintw(2, 2, 
           "Time: %d  Score: %d  Grid: %dx%d   (q=quit)", 
           time, score, w, h);

  //rámik okolo obrazovky
  if (has_colors()) attron(COLOR_PAIR(4));
  box(stdscr, 0, 0);
  if (has_colors()) attron(COLOR_PAIR(4));

  //kde začne samotný grid (odsadený od rámika);
  int off_y = 2;
  int off_x = 2;
  
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
    //mvaddnstr(2 + y, 0, row, w); //výpis celého riadku pre kontrolu či sa zmestí
    //mvprintw(2 + y, 0, "%s", row); //výpis celého riadku

  
    for (int x = 0; x < w; x++) {
      char c = row[x];

      chtype out_ch = ACS_CKBOARD; //' ';
      int pair = 0;

      switch (c) {
        case '.':
          //out_ch = ' ';        // prázdno = medzera (vyzerá čistejšie)
          pair = 5;
          break;

        case '*':
          //out_ch = ACS_CKBOARD; //ACS_DIAMOND; // jedlo
          pair = 3;
          break;

        case '#':
          //out_ch = ACS_CKBOARD; // stena
          pair = 4;
          break;

        case '@':              // hlava
          //out_ch = ACS_CKBOARD; //'O';
          pair = 2;
          break;

        case 'o':              // telo
          //out_ch = ACS_CKBOARD; //'o';
          pair = 1;
          break;

        default:
          out_ch = c;          // fallback
          pair = 0;
          break;
      }
      int yy = off_y + 2 + y;
      int xx = off_x + 2 * x;

    
      if (pair && has_colors()) attron(COLOR_PAIR(pair));
      mvaddch(yy, xx, out_ch);
      mvaddch(yy, xx + 1, out_ch); //vyplny druhý stlpec
      if (pair && has_colors()) attroff(COLOR_PAIR(pair));
    }
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
