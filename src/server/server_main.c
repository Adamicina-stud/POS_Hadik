#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../common/common.h"
#include "../common/protocol.h"
#include "server_game.h"
#include "server_net.h"

typedef struct {
  pthread_mutex_t mutex;
  int client_fd;
  int close;
} thread_data_t;

void *listen_for_input(void *arg) {
  thread_data_t *data = (thread_data_t *) arg;
  
  while (1) {
    char line[256];
    recv_line(data->client_fd, line, sizeof(line));
    char dir;
    printf("Line recieved!\n");
    if (sscanf(line, "DIR %c", &dir) == 1) {
      pthread_mutex_lock(&data->mutex);
      int dir_int = 0;
      switch (dir) {
        case 'U':
          dir_int = DIR_UP;
          break;
        case 'D':
          dir_int = DIR_DOWN;
          break;
        case 'L':
          dir_int = DIR_LEFT;
          break;
        case 'R':
          dir_int = DIR_RIGHT;
          break;
        default:
          break;
      }
      game_set_dir(data->client_fd, dir_int);
      printf("Direction changed: %d\n", dir_int);
      pthread_mutex_unlock(&data->mutex);
    }
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  int tick = 1000000;  // 1 000 000 - 1 sekunda      100 000 - 0.1 sekundy
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
  
  game_init(width, height, 0);

  printf("Grid %d x %d \n", width, height);
  printf("Server beží na porte %d...\n", port);
  
  //-------------- TEST ------------------
  game_add_player(1, "TEST");
  int directions[10] = {0, 0, 0, 0, 0, 0, 1, 3, 3, 3};

  for (int i = 0; i < 8; i++) {
    game_send_grid_to_clients(tick);
    game_set_dir(1 ,directions[i]);
    game_tick();
  }
  return 0;
  //-------------- TEST ------------------
  
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
 

  // Vytvorenie threadu
  thread_data_t thread_data;
  thread_data.client_fd = client_fd;
  thread_data.close = 0;
  pthread_mutex_init(&thread_data.mutex, NULL);
  
  
  pthread_t input_thread;
  if (pthread_create(&input_thread, NULL, listen_for_input, &thread_data) != 0) {
    fprintf(stderr, "Nepodarilo sa vytvorit listen thread");
    net_close(client_fd);
    net_close(listen_fd);
    pthread_mutex_destroy(&thread_data.mutex);
    return 1;
  }
  

  // Game loop
  while (1) {
    pthread_mutex_lock(&thread_data.mutex);

    game_send_grid_to_clients(tick); 
    
    game_tick();

    pthread_mutex_unlock(&thread_data.mutex);
    
    sleep(1);
  }
  
  net_close(client_fd);
  net_close(listen_fd);
  
  return 0;
}

