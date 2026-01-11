#include "server_game.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../common/common.h"
#include "server_net.h"

#define MAX_PLAYERS 8

//static int W, H;
//static player_t players[MAX_PLAYERS];
//static int player_count = 0;
//static int first_player_joined = 0;
//static char grid[MAX_H][MAX_W];
//static int fruit_count = 0;                     // ammount of fruit currently on the map
//static int mode = 0;                            // 0 - score mode; 1 - timed mode
//static int winner = 0;
//static int total_time = 0;
//static int elapsed_time = 0;                    // total_time - time

void game_init(int width, int height, int time, int p_mode, int p_walls, game_data *g) {
  g->W = width;
  g->H = height;
  g->player_count = 0;
  g->total_time = time;
  g->first_player_joined = 0;
  g->fruit_count = 0;
  g->winner = 0;
  if (time > 0) {
    g->elapsed_time = time;
  } else {
    g->elapsed_time = 0;
  }
  g->mode = p_mode;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    g->players[i].score = 0;
  }
  game_build_grid(p_walls, g);
}


int get_player_count(game_data *g) {
  printf("Realny počet hračov: %d\n", g->player_count);
  if (g->first_player_joined == 0) return 1;
  return g->player_count;
}

int game_add_player(game_data *g, int client_id, const char *name) {
  if (g->player_count >= MAX_PLAYERS) return -1;

  g->players[g->player_count].client_id = client_id;
  strcpy(g->players[g->player_count].name, name);
  g->players[g->player_count].dir = DIR_UP;
  g->players[g->player_count].head.x = (g->H / 2) + g->player_count;      // Dat do funkcie ktora najde miesto
  g->players[g->player_count].head.y = (g->W / 2) + g->player_count;      //  - | | -
  g->players[g->player_count].fruit_eaten = 0;
  g->players[g->player_count].score = 0;
  g->players[g->player_count].pause_time = 0;
  g->players[g->player_count].last_dir = DIR_UP;
  g->players[g->player_count].unpaused = 0;
  for (int segment = 0; segment < 256; segment++) {
    g->players[g->player_count].body[segment].x = -1;
    g->players[g->player_count].body[segment].y = -1;
  }
  if (g->first_player_joined == 0) {
    g->first_player_joined++;
  }
  g->player_count++;
  printf("Pripojil sa novy hrac! Pocet hracov: %d\n", g->player_count);
  return 0;
}

void game_set_dir(int client_id, int dir, game_data *g) {
  for (int i = 0; i < g->player_count; i++) {
    if (g->players[i].client_id == client_id) {
      int dir_chaged = 0;
      if (g->players[i].pause_time > 0) {
        if (dir == DIR_NONE) {
          g->players[i].unpaused = 1;
        }
        continue;
      }

      switch (g->players[i].dir) {
        case DIR_UP:
          if (dir != DIR_DOWN && g->players[i].dir != DIR_NONE) {
            g->players[i].last_dir = g->players[i].dir;
            g->players[i].dir = dir;
            dir_chaged = 1;
          }
          break;
        case DIR_DOWN:
          if (dir != DIR_UP && g->players[i].dir != DIR_NONE) {
            g->players[i].last_dir = g->players[i].dir;
            g->players[i].dir = dir; 
            dir_chaged = 1;
          }
          break;
        case DIR_LEFT:
          if (dir != DIR_RIGHT && g->players[i].dir != DIR_NONE) {
            g->players[i].last_dir = g->players[i].dir;
            g->players[i].dir = dir; 
            dir_chaged = 1;
          }
          break;
        case DIR_RIGHT:
          if (dir != DIR_LEFT && g->players[i].dir != DIR_NONE) {
            g->players[i].last_dir = g->players[i].dir;
            g->players[i].dir = dir; 
            dir_chaged = 1;
          }
          break;
        case DIR_NONE:
          g->players[i].dir = dir;
          g->players[i].pause_time = 3;
          g->players[i].unpaused = 0;
          dir_chaged = 1;
          break;
        default:
          g->players[i].dir = DIR_NONE;
          break;
      }
      if (dir_chaged == 1) {
        printf("Direction changed to: %d\n", g->players[i].dir);
      }
      return;
    }
  }
}

void game_tick(game_data *g) {
  if(g->total_time > 0) {
    g->elapsed_time--;
  } else {
    g->elapsed_time++;
  }
  for (int player = 0; player < g->player_count; player++) {
    // Hrac pauzol hru
    if (g->players[player].dir == DIR_NONE) {
      if (g->players[player].unpaused == 1) {
        if (g->players[player].pause_time <= 0) {
          g->players[player].dir = g->players[player].last_dir;
          g->players[player].unpaused = 0;
        } else {
          g->players[player].pause_time--;
          printf("Pause time: %d\n", g->players[player].pause_time);
        }
      }
      continue;
    }
    int last_x = g->players[player].head.x;
    int last_y = g->players[player].head.y;


    // Posunutie hlavy
    switch (g->players[player].dir) {
      case DIR_UP:
        g->players[player].head.y--;
        break;
      case DIR_DOWN:
        g->players[player].head.y++;
        break;
      case DIR_LEFT:
        g->players[player].head.x--;
        break;
      case DIR_RIGHT:
        g->players[player].head.x++;
        break;
    }
    
    // Prechod na opacnu stranu hernej plochy
    if (g->players[player].head.x <= -1) {
      g->players[player].head.x = g->W - 1;
      g->grid[g->players[player].head.x][g->players[player].head.y] = '@';
    } else if (g->players[player].head.x >= g->W) {
      g->players[player].head.x = 0;
      g->grid[g->players[player].head.x][g->players[player].head.y] = '@';
    }

    if (g->players[player].head.y <= -1) {
      g->players[player].head.y = g->H - 1;
      g->grid[g->players[player].head.x][g->players[player].head.y] = '@';
    } else if (g->players[player].head.y >= g->H) {
      g->players[player].head.y = 0;
      g->grid[g->players[player].head.x][g->players[player].head.y] = '@';
    }

    // Kolizie
    switch (g->grid[g->players[player].head.x][g->players[player].head.y]) {
      case '*':
        g->players[player].fruit_eaten++;
        g->players[player].score++;
        g->grid[g->players[player].head.x][g->players[player].head.y] = '@';
        g->fruit_count--;
        break;
      case '.':
        g->grid[g->players[player].head.x][g->players[player].head.y] = '@';
        g->grid[last_x][last_y] = '.';
        break;
      case '#':
      case 'o':
        game_remove_player_from_grid(g->players[player].client_id, g);
        break;
      default:
        break;
    }


    // Posunutie tela
    for (int segment = 0; segment < 256; segment++) {
      if (g->players[player].body[segment].x == -1) {         // Segment neexistuje
        if (g->players[player].fruit_eaten > 0) {             // ak zjedol had ovocie prida sa segment
          g->players[player].fruit_eaten--;
          g->players[player].body[segment].x = last_x;
          g->players[player].body[segment].y = last_y;
          break;
        }
        g->grid[last_x][last_y] = '.';
        break;
      }

      if (segment == 0) g->grid[last_x][last_y] = 'o';
      int last_x_temp = g->players[player].body[segment].x;  // Pomocne premenne
      int last_y_temp = g->players[player].body[segment].y;

      g->players[player].body[segment].x = last_x;
      g->players[player].body[segment].y = last_y;

      last_x = last_x_temp;
      last_y = last_y_temp;
    }
  }

  if (g->fruit_count < g->player_count) {
    game_add_fruit(10, g);
  }
}

int game_add_fruit(int num_of_attempts, game_data *g) {
  int num = 0;

  while (num < num_of_attempts) {
    int x = rand() % g->W;
    int y = rand() % g->H;

    if (g->grid[x][y] == '.') {
      g->grid[x][y] = '*';
      g->fruit_count++;
      return 0;
    }
    num++;
  }

  return 1;
}

void game_build_grid(int walls, game_data *g) {
  for (int y = 0; y < g->H; y++) {
    for (int x = 0; x < g->W; x++) {
      g->grid[x][y] = '.';
      if (walls == 1 && (y == 0 || y == g->H - 1 || x == 0 || x == g->W - 1)) {
        g->grid[x][y] = '#';
      }
    }
  }
}

void game_send_grid_to_clients(int tick, game_data *g) {
  for (int player = 0; player < g->player_count; player++) {
    char message[50];
    snprintf(message, sizeof(message), "STATE %d %d %d %d %d", g->W, g->H, g->players[player].pause_time, g->players[player].score, g->elapsed_time);
    net_send_line(g->players[player].client_id, message);

    net_send_line(g->players[player].client_id, "GRID");
    for (int y = 0; y < g->H; y++) {
      char line[g->W + 1];
      for (int x = 0; x < g->W; x++) {
        line[x] = g->grid[x][y];
        //printf("%c", line[x]);                                                   
        }
      //printf("\n");
      line[g->W] = '\0';
      net_send_line(g->players[player].client_id, line);
    }
  }
}

void game_remove_player_from_grid(int player_id, game_data *g) {
  // nájdi index hráča podľa client_id
  int idx = -1;
  for (int i = 0; i < g->player_count; i++) {
    if (g->players[i].client_id == player_id) {
      idx = i;
      break;
    }
  }
  if (idx == -1) return;

  // vymaž hlavu z gridu (bezpečne v rozsahu)
  int hx = g->players[idx].head.x;
  int hy = g->players[idx].head.y;
  if (hx >= 0 && hx < g->W && hy >= 0 && hy < g->H) {
    if (g->grid[hx][hy] == '@') g->grid[hx][hy] = '.';
  }

  // vymaž telo
  for (int s = 0; s < 256; s++) {
    int bx = g->players[idx].body[s].x;
    int by = g->players[idx].body[s].y;
    if (bx == -1) break;

    if (bx >= 0 && bx < g->W && by >= 0 && by < g->H) {
      if (g->grid[bx][by] == 'o') g->grid[bx][by] = '.';
      else if (g->grid[bx][by] == '@') g->grid[bx][by] = '.';
    }

    g->players[idx].body[s].x = -1;
    g->players[idx].body[s].y = -1;
  }

  g->players[idx].head.x = -1;
  g->players[idx].head.y = -1;

  // zavri socket iba tohto hráča
  if (g->players[idx].client_id != 0) {
    net_close(g->players[idx].client_id);
  }

  // SWAP-REMOVE: posledného hráča presuň na miesto odstráneného
  // (aby pole ostalo kompaktné a player_count sedel)
  int last = g->player_count - 1;
  if (idx != last) {
    g->players[idx] = g->players[last];
  }

  // zníž počet hráčov
  g->player_count--;
  printf("Hráč sa odpojil. Počet hráčov: %d\n", g->player_count);
}

int game_over(game_data *g) {
  int game_end = 0;
  // 0 - dosiahnute max skore
  // 1 - vyprsal cas
  if (g->mode == 0) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
      if (g->players[i].score >= 100) {
        g->winner = i;
        game_end = 1;
      }
    }
  } else if (g->mode == 1) {
    if (g->elapsed_time <= 0) {
      int highest_score = 0;
      for (int i = 0; i < g->player_count; i++) {
        if (g->players[i].score > highest_score) {
          g->winner = i;
          game_end = 1;
        }
      }
    }
  }
  
  
  if (game_end == 1) {
    for (int i = 0; i < g->player_count; i++) {
      char line[MAX_NAME_LEN + 10] = "END ";
      if (g->players[i].name == g->players[g->winner].name) {
        strcat(line, "VYHRAL SI");
      } else {
        strcat(line, "PREHRAL SI");
      }
      net_send_line(g->players[i].client_id, line);
      net_close(g->players[i].client_id);
      g->players[i].client_id = 0;
    }
  }


  return game_end;
}

/*
char* game_get_winner() {
  return players[winner].name;
}

int game_width() {
  return W;
}

int game_height() {
  return H;
}
*/

void game_cleanup() {
  
}
