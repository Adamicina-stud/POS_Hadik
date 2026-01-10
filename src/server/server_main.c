#include <asm-generic/errno-base.h>
#include <errno.h>
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
  int listen_fd;
  int client_fd;
  int close;
} thread_data_t;

void *listen_for_input(void *arg) {
  thread_data_t *data = (thread_data_t *) arg;
  printf("Input listener vytvoreny %d\n", data->client_fd);
  
  while (data->close == 0) {
    char line[256];
    recv_line(data->client_fd, line, sizeof(line));
    char dir;
    printf("Line recieved! %s\n", line);

    if (sscanf(line, "DIR %c", &dir) == 1) {
      pthread_mutex_lock(&data->mutex);
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
      pthread_mutex_unlock(&data->mutex);

    } else if (sscanf(line, "LEAVE") == 1) {
      pthread_mutex_lock(&data->mutex);
      game_remove_player_from_grid(data->client_fd);
      pthread_mutex_unlock(&data->mutex);
    }
  }

  pthread_mutex_destroy(&data->mutex);
  return NULL;
}


void *listen_for_join(void *arg) {
  thread_data_t *data = (thread_data_t *) arg;
  
  thread_data_t new_data[100];
  //thread_data_t input_data[100]
  pthread_t new_input_thread[100];
  int new_threads_count = 0;

  while (data->close == 0) {
    int client_fd = net_accept(data->listen_fd);
    if (client_fd < 0) {
      if (errno == EINTR) continue;
      continue;
    }
    printf("Klient pripojeny!");

    // Precita spravu JOIN
    char client_name[256];
    recv_line(client_fd, client_name, sizeof(client_name));
    printf("Od klienta: %s", client_name);
    
    pthread_mutex_lock(&data->mutex);
    game_add_player(client_fd, client_name);
    
    /*
    thread_data_t* new_data = malloc(sizeof(*input_data));
    if (!new_data) {
      perror("malloc");
      close(client_fd);
      continue;
    }

    *new_data = (thread_data_t){
      .client_fd = client_fd,
      .listen_fd = data->listen_fd,
      .close = 0,
    };
    pthread_mutex_init(&new_data->mutex, NULL);
    */

    new_data[new_threads_count].mutex = data->mutex;
    new_data[new_threads_count].listen_fd = data->listen_fd;
    new_data[new_threads_count].client_fd = client_fd;
    new_data[new_threads_count].close = 0;


    pthread_create(&new_input_thread[new_threads_count], NULL, listen_for_input, &new_data[new_threads_count]);
    for (int i = 0; i < new_threads_count; i++) {
      if (new_data[i].close == 1) {
        pthread_join(new_input_thread[i], NULL);
        net_close(new_data[i].client_fd);
      }
    }
    pthread_detach(new_input_thread[new_threads_count]);

    pthread_mutex_unlock(&data->mutex);
  }
  return NULL;
}


// argumenty: 
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


  /*
  //-------------- TEST ------------------
  game_add_player(1, "TEST");
  int directions[10] = {0, 0, 0, 0, 0, 0, 1, 3, 3, 3};

  for (int i = 0; i < 8; i++) {
    game_send_grid_to_clients(tick);
    game_set_dir(1 ,directions[i]);
    game_tick();
  }
  char a[2000] = "GAME OVER! WINNER ";
  strcat(a, game_get_winner());
  printf(a);
  return 0;
  //-------------- TEST ------------------
  */ 
  
  thread_data_t join_listener_data;
  join_listener_data.client_fd = 0;
  join_listener_data.listen_fd = listen_fd;
  join_listener_data.close = 0;
  pthread_mutex_init(&join_listener_data.mutex, NULL);

  pthread_t listener_thread;
  if (pthread_create(&listener_thread, NULL, listen_for_join, &join_listener_data) != 0) {
    fprintf(stderr, "Nepodarilo sa vytvorit listen for join thread");
    net_close(listen_fd);
    pthread_mutex_destroy(&join_listener_data.mutex);
    return 1;
  }
  
  /*
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
  thread_data.listen_fd = listen_fd;
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
  */


  // Game loop
  int game_ended = 0;
  while (game_ended == 0) {
    pthread_mutex_lock(&join_listener_data.mutex);

    game_send_grid_to_clients(tick); 
    
    game_tick();

    game_ended = game_over();

    pthread_mutex_unlock(&join_listener_data.mutex);
    
    sleep(1);
  }
  
  pthread_mutex_lock(&join_listener_data.mutex);
  join_listener_data.close = 1;
  pthread_mutex_unlock(&join_listener_data.mutex);
  
  pthread_join(listener_thread, NULL);

  net_close(listen_fd);
  
  return 0;
}

