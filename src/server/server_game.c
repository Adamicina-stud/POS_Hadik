#include "server_game.h"

#include <stdlib.h>
#include <string.h>

#define MAX_PLAYERS 8

typedef struct {
  int client_id;
  int dir;
  int x, y;
} player_t;

static int W, H;
static player_t players[MAX_PLAYERS];
static int player_count = 0;

void game_init(int width, int height) {
  W = width;
  H = height;
  player_count = 0;
}

int game_add_player(int client_id, const char *name) {
  if (player_count >= MAX_PLAYERS) return -1;

  players[player_count].client_id = client_id;
  players[player_count].dir = 0;
  players[player_count].x = player_count + 1;
  players[player_count].y = player_count + 1;
  player_count++;

  (void)name; // zatiaľ nepoužité
  return 0;
}

void game_set_dir(int client_id, int dir) {
  for (int i = 0; i < player_count; i++) {
    if (players[i].client_id == client_id) {
      players[i].dir = dir;
      return;
    }
  }
}

void game_tick() {
  // zatiaľ nič – v M2 tu príde pohyb hada
}

void game_build_grid(char **grid_out) {
  for (int y = 0; y < H; y++) {
    memset(grid_out[y], '.', W);
    grid_out[y][W] = '\0';
  }

  // vykreslíme hráčov ako 'O'
  for (int i = 0; i < player_count; i++) {
    int x = players[i].x;
    int y = players[i].y;
    if (x >= 0 && x < W && y >= 0 && y < H) {
      grid_out[y][x] = 'O';
    }
  }
}

int game_width() {
  return W;
}

int game_height() {
  return H;
}

void game_cleanup() {
  
}
