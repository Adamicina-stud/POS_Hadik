#include <asm-generic/errno-base.h>
#include <bits/pthreadtypes.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
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
  pthread_mutex_t *mutex;
  int listen_fd;
  int client_fd[MAX_PLAYERS];
  int close;
  int create_these_client_fds[MAX_PLAYERS];
} thread_data_t;

typedef struct {
  pthread_mutex_t *mutex;
  int listen_fd;
  int client_fd;
  int close;
} input_thread_data_t;

void *listen_for_input(void *arg) {
  input_thread_data_t *data = (input_thread_data_t *) arg;
  printf("Input listener vytvoreny %d\n", data->client_fd);
  
  while (data->close == 0) {
    char line[256];
    recv_line(data->client_fd, line, sizeof(line));
    char dir;
    pthread_mutex_lock(data->mutex);
    //printf("Line recieved! %s\n", line);

    if (sscanf(line, "DIR %c", &dir) == 1) {
      printf("Direction: %s\n", line);
      int dir_int = DIR_NONE;
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
        case 'P':
          dir_int = DIR_NONE;
          break;
        default:
          dir_int = DIR_NONE;
          break;
      }
      game_set_dir(data->client_fd, dir_int);
      printf("Direction changed: %d\n", dir_int);
    
    } else if (sscanf(line, "LEAVE") == 1) {
      game_remove_player_from_grid(data->client_fd);
      data->close = 1;
    } else if (sscanf(line, "LIR %c", &dir) == 1) {
      game_remove_player_from_grid(data->client_fd);
      data->close = 1;
    }
    pthread_mutex_unlock(data->mutex);
  }
  printf("Ending join thread %d\n", data->client_fd);
  pthread_mutex_destroy(data->mutex);
  return NULL;
}


void *listen_for_join(void *arg) {
  thread_data_t *data = (thread_data_t *) arg;

  while (data->close == 0) {
    int client_fd = net_accept(data->listen_fd);
    if (client_fd < 0) {
      if (errno == EINTR) continue;
      continue;
    }
    printf("Klient pripojeny!");

    pthread_mutex_lock(data->mutex);

    // Precita spravu JOIN
    char client_name[256];
    recv_line(client_fd, client_name, sizeof(client_name));
    printf("Od klienta: %s", client_name);
    
    //pthread_mutex_lock(data->mutex);
    game_add_player(client_fd, client_name);
    
    // Zisti ci v hre je max hracov
    int free_player_spot = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
      if (data->client_fd[i] == 0) {
        data->client_fd[i] = client_fd;
        data->create_these_client_fds[i] = client_fd;
        free_player_spot = 1;
        break;
      }
    }

    if (free_player_spot == 0) {
      printf("Nie je volne miesto\n");
      pthread_mutex_unlock(data->mutex);
      continue;
    }

    pthread_mutex_unlock(data->mutex);
  }
  return NULL;
}


// argumenty v poradi: 
// sirka, vyska, port, 
// max_cas/max_skore: 0 - bez casu/skore, X - hra sa do X casu/skore
// mod_hry: 0 - skore, 1 - cas 
// prekazky: 0 - prazdne pole, 1 - steny
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
  
  // Cas trvania hry/Max skore; 0 - bez casu/skore
  int time_score = 0;
  if (argc >= 5) {
    time_score = strtol(argv[4], &c, 10);
  }

  // Typ hry; 0 - cas, 1 - skore
  int mode = 0;
  if (argc >= 6) {
    mode = strtol(argv[5], &c, 10);
  }
  
  // Typ hernj plochy; 0 - prazdna, 1 - steny
  int walls = 0;
  if (argc >= 7) {
    walls = strtol(argv[6], &c, 10);
  }

  game_init(width, height, time_score, mode, walls);

  printf("Grid %d x %d \n", width, height);
  printf("Server beží na porte %d...\n", port);

  // Vytvorenie threadu ktory pocuna pre JOIN spravu
  thread_data_t join_listener_data;
  join_listener_data.listen_fd = listen_fd;
  join_listener_data.close = 0;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    join_listener_data.client_fd[i] = 0;
    join_listener_data.create_these_client_fds[i] = 0;
  }
  pthread_mutex_t mutex;
  pthread_mutex_init(&mutex, NULL);
  join_listener_data.mutex = &mutex;

  pthread_t listener_thread;
  if (pthread_create(&listener_thread, NULL, listen_for_join, &join_listener_data) != 0) {
    fprintf(stderr, "Nepodarilo sa vytvorit listen for join thread");
    net_close(listen_fd);
    pthread_mutex_destroy(join_listener_data.mutex);
    return 1;
  }
  
  input_thread_data_t input_data[MAX_PLAYERS];
  pthread_t input_threads[MAX_PLAYERS];

  for (int i = 0; i < MAX_PLAYERS; i++) {
    input_data[i].client_fd = 0;
    input_data[i].mutex = &mutex;
    input_data[i].listen_fd = listen_fd;
    input_data[i].close = 0;
  }


  // Game loop
  int game_ended = 0;
  while (game_ended == 0) {
    pthread_mutex_lock(join_listener_data.mutex);
    
    // Vytvori nove thready pre input
    for (int i = 0; i < MAX_PLAYERS; i++) {
      if (join_listener_data.create_these_client_fds[i] != 0) {
        int client_fd = join_listener_data.create_these_client_fds[i];

        for (int j = 0; j < MAX_PLAYERS; j++) {
          if (input_data[j].client_fd == 0) {
            input_data[j].client_fd = client_fd;
            pthread_create(&input_threads[j], NULL, listen_for_input, &input_data[j]);
            printf("Created thread for client: %d\n", client_fd);
            break;
          }
        }

        join_listener_data.create_these_client_fds[i] = 0;
      }

      // Zavre nepouzite thready
      if (input_data[i].close == 1) {
        pthread_join(input_threads[i], NULL);
      }
    }
    
    game_tick();

    game_send_grid_to_clients(tick); 

    game_ended = game_over();

    pthread_mutex_unlock(join_listener_data.mutex);
    
    sleep(1);
  }
  
  pthread_mutex_lock(join_listener_data.mutex);
  join_listener_data.close = 1;
  pthread_mutex_unlock(join_listener_data.mutex);
  
  pthread_join(listener_thread, NULL);

  net_close(listen_fd);
  
  return 0;
}

