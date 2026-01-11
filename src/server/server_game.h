#ifndef SERVER_GAME_H
#define SERVER_GAME_H

#include "../common/common.h"

typedef struct {
  int x, y;
} vector2;

typedef struct {
  int client_id;
  char name[MAX_NAME_LEN];
  int dir;
  int last_dir;
  vector2 head;
  vector2 body[256];
  int fruit_eaten;
  int score;
  int pause_time;
  int unpaused;
} player_t;

typedef struct {
  int W, H;
  player_t players[MAX_PLAYERS];
  int player_count;
  int first_player_joined;
  char grid[MAX_H][MAX_W];
  int fruit_count;
  int mode;
  int winner;
  int total_time;
  int elapsed_time;
} game_data;

// Inicializácia hry
void game_init(int width, int height, int time, int p_mode, int p_walls, game_data *g);

int get_player_count(game_data *g);

// Pridanie hráča (JOIN)
// client_id je napr. client_fd
int game_add_player(game_data *g, int client_id, const char *name);

// Nastavenie smeru hráča (DIR)
void game_set_dir(int client_id, int dir, game_data *g);

// Jeden herný tick (pohyb, kolízie, vytvaranie ovocia)
void game_tick(game_data *g);

// Pokusi sa num_of_attempts krat pridat jedlo na nahodne prazdne policko
int game_add_fruit(int num_of_attempts, game_data *g);

// Postaví aktuálny grid do bufferu
// grid[y][x]
void game_build_grid(int walls, game_data *g);

// Posle po riadkoch grid hracom
void game_send_grid_to_clients(int tick, game_data *g);

void game_remove_player_from_grid(int player_num, game_data *g);

// 0 - hra pokracuje
// 1 - hra sa skoncila
int game_over(game_data *g);

char* game_get_winner();

// Vráti rozmery gridu
int game_width();
int game_height();

// Uvoľnenie pamäte
void game_cleanup();

#endif
