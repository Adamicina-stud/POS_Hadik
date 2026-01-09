#include <bits/pthreadtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../common/common.h"
#include "../common/protocol.h"
#include "server_game.h"
#include "server_net.h"



void *method(void *arg) {
  
}

int main(int argc, char *argv[]) {
  int tick = 200000;
  int port = DEFAULT_PORT;
  char *c;
  if (argc >= 4) port = strtol(argv[3], &c, 10);

  // Vytvorenie socketu
  int listen_fd = net_init(port);
  if (listen_fd < 0) {
    fprintf(stderr, "Nepodarilo sa spustit server!");
    return 1;
  }

  // Sirka a vyska mapy
  int width = GRID_W;
  int height = GRID_H;

  if (argc >= 3) {
    int arg0 = strtol(argv[1], &c, 10); 
    int arg1 = strtol(argv[2], &c, 10);

    if (arg0 > MAX_W) {     
      width = MAX_W;        
    } else {
      width = arg0;
    }

    if (arg1 > MAX_H) {    
      height = MAX_H;     
    } else {
      height = arg1;
    }

    if (arg0 < 0) width = GRID_W;
    if (arg1 < 0) height = GRID_H;
  }
  
  game_init(width, height);

  printf("Grid %d x %d \n", width, height);
  printf("Server beží na porte %d...\n", port);
  
  
  // Accept – čakáme na klienta
  int  client_fd = net_accept(listen_fd);
  if (client_fd < 0) {
    fprintf(stderr, "Klient nebol prijaty");
    net_close(listen_fd);
    return 1;
  }
  printf("Klient pripojený!\n");


  // Prečítame správu od klienta (JOIN)
  char client_name[256];
  recv_line(client_fd, client_name, sizeof(client_name));
  printf("Od klienta: %s", client_name);

  game_add_player(client_fd, client_name);

  // Game loop
  while (1) {
    
    game_send_grid_to_clients(tick);

    // Input
    char line[256];
    while (recv_line(client_fd, line, sizeof(line)) > 0) {
      char dir;
      if (sscanf(line, "DIR %c", &dir) == 1) {
        game_set_dir(client_fd, dir);
      }
    }
    
    game_tick();
    
    sleep(2);
  }
  
  net_close(client_fd);
  net_close(listen_fd);
  
  return 0;
}


/*

  void game_over(int client) {
    head.x = -1;
    head.y = -1;
    for (int i = 0; i <= MAX_SNAKE_LENGTH; i++) {
      if (body[i].x == -1) break;

      grid[body[i].x][body[i].y] = '.';

      body[i].x = -1;
      body[i].y = -1;
    }
  }
*/
