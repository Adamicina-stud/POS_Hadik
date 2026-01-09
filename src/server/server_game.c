#include "server_game.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../common/common.h"
#include "server_net.h"

#define MAX_PLAYERS 8

typedef struct {
  int x, y;
} vector2;

typedef struct {
  int client_id;
  char name[MAX_NAME_LEN];
  int dir;
  vector2 head;
  vector2 body[256];
  int fruit_eaten;
} player_t;

static int W, H;
static player_t players[MAX_PLAYERS];
static int player_count = 0;
static char grid[MAX_H][MAX_W];
static int fruit_count = 0;

void game_init(int width, int height) {
  W = width;
  H = height;
  player_count = 0;
  game_build_grid();
}

int game_add_player(int client_id, const char *name) {
  if (player_count >= MAX_PLAYERS) return -1;

  players[player_count].client_id = client_id;
  strcpy(players[player_count].name, name);
  players[player_count].dir = 0;
  players[player_count].head.x = player_count + 1;      // Dat do funkcie ktora najde miesto
  players[player_count].head.y = player_count + 1;      //  - | | -
  players[player_count].fruit_eaten = 0;
  for (int segment = 0; segment < 256; segment++) {
    players[player_count].body[segment].x = -1;
    players[player_count].body[segment].y = -1;
  }
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
  for (int player = 0; player < player_count; player++) {
    int last_x = players[player].head.x;
    int last_y = players[player].head.y;

    // Posunutie hlavy
    switch (players[player].dir) {
      case DIR_UP:
        players[player].head.y--;
        break;
      case DIR_DOWN:
        players[player].head.y++;
        break;
      case DIR_LEFT:
        players[player].head.x--;
        break;
      case DIR_RIGHT:
        players[player].head.x++;
        break;
    }

    // Posunutie tela
    for (int segment = 0; segment < 256; segment++) {
      if (players[player].body[segment].x == -1) {         // Segment neexistuje
        if (players[player].fruit_eaten > 0) {             // ak zjedol had ovocie prida sa segment
          players[player].fruit_eaten--;
          players[player].body[segment].x = last_x;
          players[player].body[segment].y = last_y;
          break;
        }
        grid[last_x][last_y] = '.';
        break;
      }

      if (segment == 0) grid[last_x][last_y] = 'o';
      int last_x_temp = players[player].body[segment].x;  // Pomocne premenne
      int last_y_temp = players[player].body[segment].y;

      players[player].body[segment].x = last_x;
      players[player].body[segment].y = last_y;

      last_x = last_x_temp;
      last_y = last_y_temp;
    }
  }

  // Kontrola kolizii hlav
  for (int player = 0; player < player_count; player++) {
    switch (grid[players[player].head.x][players[player].head.y]) {
      case '*':
        players[player].fruit_eaten++;
      case '.':
        grid[players[player].head.x][players[player].head.y] = '@';
        break;
      case '#':
      case 'o':
        //game_over();
        break;
      default:
        break;
    }
  }
  
  if (fruit_count < 10) {
    game_add_fruit(10);
  }
}

int game_add_fruit(int num_of_attempts) {
  int num = 0;
  int x = 0;
  int y = 0;

  while (num < num_of_attempts) {
    x = rand() % W;
    y = rand() % H;

    if (grid[x][y] == '.') {
      grid[x][y] = '*';
      fruit_count++;
      return 0;
    }
    num++;
  }

  return 1;
}

void game_build_grid() {
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      grid[y][x] = '.';
    }
  }
}

void game_send_grid_to_clients(int tick) {
  for (int player = 0; player < player_count; player++) {
    char message[50];
    snprintf(message, sizeof(message), "STATE %d %d %d", W, H, tick);
    net_send_line(players[player].client_id, message);

    net_send_line(players[player].client_id, "GRID");
    for (int y = 0; y < H; y++) {
      char line[W + 1];
      for (int x = 0; x < W; x++) {
        line[x] = grid[y][x];
        printf("%c", line[x]);                                                    // TEST
      }
      line[W] = '\0';
      printf("    %c\n", line[W - 1]);                                            // TEST
      net_send_line(players[player].client_id, ".............................."); //
      //net_send_line(players[player].client_id, line);
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
