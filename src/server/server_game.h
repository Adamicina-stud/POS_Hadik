
#ifndef SERVER_GAME_H
#define SERVER_GAME_H

// Inicializácia hry
void game_init(int width, int height);

// Pridanie hráča (JOIN)
// client_id je napr. client_fd
int game_add_player(int client_id, const char *name);

// Nastavenie smeru hráča (DIR)
void game_set_dir(int client_id, int dir);

// Jeden herný tick (pohyb, kolízie, atď.)
void game_tick();

// Postaví aktuálny grid do bufferu
// grid[y][x]
void game_build_grid(char **grid_out);

// Vráti rozmery gridu
int game_width();
int game_height();

// Uvoľnenie pamäte
void game_cleanup();

#endif
