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
  int last_dir;
  vector2 head;
  vector2 body[256];
  int fruit_eaten;
  int score;
  int pause_time;
  int unpaused;       // 0 pauzol hru, 1 hru odpauzol a za 3 ticky bude v hre
} player_t;

static int W, H;
static player_t players[MAX_PLAYERS];
static int player_count = 0;
static char grid[MAX_H][MAX_W];
static int fruit_count = 0;                     // ammount of fruit currently on the map
static int mode = 0;                            // 0 - score mode; 1 - timed mode
static int winner = 0;
static int total_time = 0;
static int elapsed_time = 0;                    // total_time - time

void game_init(int width, int height, int time, int p_mode, int p_walls) {
  W = width;
  H = height;
  player_count = 0;
  total_time = time;
  if (time > 0) {
    elapsed_time = time;
  } else {
    elapsed_time = 0;
  }
  mode = p_mode;
  game_build_grid(p_walls);
}

int game_add_player(int client_id, const char *name) {
  if (player_count >= MAX_PLAYERS) return -1;

  players[player_count].client_id = client_id;
  strcpy(players[player_count].name, name);
  players[player_count].dir = DIR_UP;
  players[player_count].head.x = (H / 2) + player_count;      // Dat do funkcie ktora najde miesto
  players[player_count].head.y = (W / 2) + player_count;      //  - | | -
  players[player_count].fruit_eaten = 0;
  players[player_count].score = 0;
  players[player_count].pause_time = 0;
  players[player_count].last_dir = DIR_UP;
  players[player_count].unpaused = 0;
  for (int segment = 0; segment < 256; segment++) {
    players[player_count].body[segment].x = -1;
    players[player_count].body[segment].y = -1;
  }
  player_count++;
  printf("Pripojil sa novy hrac! Pocet hracov: %d\n", player_count);
  return 0;
}

void game_set_dir(int client_id, int dir) {
  for (int i = 0; i < player_count; i++) {
    if (players[i].client_id == client_id) {
      int dir_chaged = 0;
      if (players[i].pause_time > 0) {
        if (dir == DIR_NONE) {
          players[i].unpaused = 1;
        }
        continue;
      }

      switch (players[i].dir) {
        case DIR_UP:
          if (dir != DIR_DOWN && players[i].dir != DIR_NONE) {
            players[i].last_dir = players[i].dir;
            players[i].dir = dir;
            dir_chaged = 1;
          }
          break;
        case DIR_DOWN:
          if (dir != DIR_UP && players[i].dir != DIR_NONE) {
            players[i].last_dir = players[i].dir;
            players[i].dir = dir; 
            dir_chaged = 1;
          }
          break;
        case DIR_LEFT:
          if (dir != DIR_RIGHT && players[i].dir != DIR_NONE) {
            players[i].last_dir = players[i].dir;
            players[i].dir = dir; 
            dir_chaged = 1;
          }
          break;
        case DIR_RIGHT:
          if (dir != DIR_LEFT && players[i].dir != DIR_NONE) {
            players[i].last_dir = players[i].dir;
            players[i].dir = dir; 
            dir_chaged = 1;
          }
          break;
        case DIR_NONE:
          players[i].dir = dir;
          players[i].pause_time = 3;
          players[i].unpaused = 0;
          dir_chaged = 1;
          break;
        default:
          players[i].dir = DIR_NONE;
          break;
      }
      if (dir_chaged == 1) {
        printf("Direction changed to: %d\n", players[i].dir);
      }
      return;
    }
  }
}

void game_tick() {
  if(total_time > 0) {
    elapsed_time--;
  } else {
    elapsed_time++;
  }
  for (int player = 0; player < player_count; player++) {
    // Hrac pauzol hru
    if (players[player].dir == DIR_NONE) {
      if (players[player].unpaused == 1) {
        if (players[player].pause_time <= 0) {
          players[player].dir = players[player].last_dir;
          players[player].unpaused = 0;
        } else {
          players[player].pause_time--;
          printf("Pause time: %d\n", players[player].pause_time);
        }
      }
      continue;
    }
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
    
    // Prechod na opacnu stranu hernej plochy
    if (players[player].head.x <= -1) {
      players[player].head.x = W - 1;
      grid[players[player].head.x][players[player].head.y] = '@';
    } else if (players[player].head.x >= W) {
      players[player].head.x = 0;
      grid[players[player].head.x][players[player].head.y] = '@';
    }

    if (players[player].head.y <= -1) {
      players[player].head.y = H - 1;
      grid[players[player].head.x][players[player].head.y] = '@';
    } else if (players[player].head.y >= H) {
      players[player].head.y = 0;
      grid[players[player].head.x][players[player].head.y] = '@';
    }

    // Kolizie
    switch (grid[players[player].head.x][players[player].head.y]) {
      case '*':
        players[player].fruit_eaten++;
        players[player].score++;
        grid[players[player].head.x][players[player].head.y] = '@';
        fruit_count--;
        break;
      case '.':
        grid[players[player].head.x][players[player].head.y] = '@';
        grid[last_x][last_y] = '.';
        break;
      case '#':
      case 'o':
        game_remove_player_from_grid(players[player].client_id);
        break;
      default:
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

  if (fruit_count < player_count) {
    game_add_fruit(10);
  }
}

int game_add_fruit(int num_of_attempts) {
  int num = 0;

  while (num < num_of_attempts) {
    int x = rand() % W;
    int y = rand() % H;

    if (grid[x][y] == '.') {
      grid[x][y] = '*';
      fruit_count++;
      return 0;
    }
    num++;
  }

  return 1;
}

void game_build_grid(int walls) {
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      grid[x][y] = '.';
      if (walls == 1 && (y == 0 || y == H - 1 || x == 0 || x == W - 1)) {
        grid[x][y] = '#';
      }
    }
  }
}

void game_send_grid_to_clients(int tick) {
  for (int player = 0; player < player_count; player++) {
    char message[50];
    snprintf(message, sizeof(message), "STATE %d %d %d %d %d", W, H, players[player].pause_time, players[player].score, elapsed_time);
    net_send_line(players[player].client_id, message);

    net_send_line(players[player].client_id, "GRID");
    for (int y = 0; y < H; y++) {
      char line[W + 1];
      for (int x = 0; x < W; x++) {
        line[x] = grid[x][y];
        //printf("%c", line[x]);                                                   
        }
      //printf("\n");
      line[W] = '\0';
      net_send_line(players[player].client_id, line);
    }
  }
}

void game_remove_player_from_grid(int player_id) {
  // Najde hraca
  int player_num = 0;
  for (int i = 0; i < player_count; i++) {
    if (players[i].client_id == player_id) {
      player_num = i;
      break;
    }
  }

  /*
  if (player_num == 0) {
    return;
  }
  */

  // Odstrani hlavu 
  if (grid[players[player_num].head.x][players[player_num].head.y] == '@') {
    grid[players[player_num].head.x][players[player_num].head.y] = '.';
  }

  // Odstrani telo
  for (int segment = 0; segment < 256; segment++) {
    if (players[player_num].body[segment].x == -1) {
      break;
    }

    grid[players[player_num].body[segment].x][players[player_num].body[segment].y] = '.';

    players[player_num].body[segment].x = -1;
    players[player_num].body[segment].y = -1;
  }

  players[player_num].head.x = -1;
  players[player_num].head.y = -1;
  
  if (players[player_num].client_id != 0) {
    printf("Sending end line\n");
    //net_send_line(players[player_num].client_id, "END UMREL SI");
    
  }
  //net_close(players[player_num].client_id);
  player_count--;
}

int game_over() {
  int game_end = 0;
  // 0 - dosiahnute max skore
  // 1 - vyprsal cas
  if (mode == 0) {
    for (int i = 0; i < player_count; i++) {
      if (players[i].score >= 100) {
        winner = i;
        game_end = 1;
      }
    }
  } else if (total_time > 0) {
    if (elapsed_time <= 0) {
      int highest_score = 0;
      for (int i = 0; i < player_count; i++) {
        if (players[i].score > highest_score) {
          winner = i;
          game_end = 1;
        }
      }
    }
  }

  if (game_end == 1) {
    for (int i = 0; i < player_count; i++) {
      char line[MAX_NAME_LEN + 10] = "END ";
      if (players[i].name == players[winner].name) {
        strcat(line, "VYHRAL SI");
      } else {
        strcat(line, "PREHRAL SI");
      }
      net_send_line(players[i].client_id, line);
      net_close(players[i].client_id);
      players[i].client_id = 0;
    }
  }

  return game_end;
}

char* game_get_winner() {
  return players[winner].name;
}

int game_width() {
  return W;
}

int game_height() {
  return H;
}

void game_cleanup() {
  
}
