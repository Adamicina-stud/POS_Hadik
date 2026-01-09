
#ifndef SERVER_GAME_H
#define SERVER_GAME_H

// Inicializácia hry
void game_init(int width, int height);

// Pridanie hráča (JOIN)
// client_id je napr. client_fd
int game_add_player(int client_id, const char *name);

// Nastavenie smeru hráča (DIR)
void game_set_dir(int client_id, int dir);

// Jeden herný tick (pohyb, kolízie, vytvaranie ovocia)
void game_tick();

// Pokusi sa num_of_attempts krat pridat jedlo na nahodne prazdne policko
int game_add_fruit(int num_of_attempts);

// Postaví aktuálny grid do bufferu
// grid[y][x]
void game_build_grid();

// Posle po riadkoch grid hracom
void game_send_grid_to_clients(int tick);

// Vráti rozmery gridu
int game_width();
int game_height();

// Uvoľnenie pamäte
void game_cleanup();

#endif
