#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/socket.h>

#include "client_net.h"
#include "client_ui.h"
#include "client_input.h"

#include "../common/common.h"

typedef struct {
  int fd;
  pthread_mutex_t mutex;

  int w, h, score;
  char grid[MAX_H * (MAX_W + 1)];
  int has_frame;
  int running;
  int disconnected;
  
  int paused;
  int ended;
  char end_reason[256];
} client_state_t;

static void *net_thread_fn(void *arg) {
  client_state_t *st = (client_state_t *) arg;
  int fd = st->fd;
  
  //lokálne buffre (aby sme držali mutex čo najmenej)
  int w = 0, h = 0, paused = 0, score = 0, time = 0;
  int running = 0;
  char grid_buf[MAX_H * (MAX_W + 1)];

  while(1) {
    //kontrola či nastane koniec
    pthread_mutex_lock(&st->mutex);
    running = st->running;
    pthread_mutex_unlock(&st->mutex);
    if (!running) break;
  
    char end_reason[256];
    int rc = client_recv_frame(fd, &w, &h, &paused, &score, &time, grid_buf, sizeof(grid_buf), end_reason, sizeof(end_reason));
    if (rc == 2) {
      pthread_mutex_lock(&st->mutex);
      st->ended = 1;
      snprintf(st->end_reason, sizeof(st->end_reason), "%s", end_reason);
      st->running = 0;
      pthread_mutex_unlock(&st->mutex);
      break;
    }

    if (rc <= 0) {
      //0 = server sa odpojil, -1 = chyba
      pthread_mutex_lock(&st->mutex);
      st->disconnected = 1;
      st->running = 0;
      pthread_mutex_unlock(&st->mutex);
      break;
    }

    //ulož frame
    pthread_mutex_lock(&st->mutex);
    st->w = w;
    st->h = h;
    st->paused = paused;
    //st->tick = tick;
    st->score = score;
    memcpy(st->grid, grid_buf, sizeof(grid_buf));
    st->has_frame = 1;
    pthread_mutex_unlock(&st->mutex);
  }

  return NULL;
}

int main(int argc, char **argv) {
  const char *ip   = "127.0.0.1";
  int port         = DEFAULT_PORT;
  const char *name = "player";

  if (argc >= 2) ip = argv[1];
  if (argc >= 3) port = atoi(argv[2]);
  if (argc >= 4) name = argv[3];

  // 1) connect
  int fd = client_connect(ip, port);
  if (fd < 0) {
      fprintf(stderr, "Nepodarilo sa pripojiť na server.\n");
      return 1;
  }

  // 2) JOIN
  if (client_send_join(fd, name) < 0) {
      fprintf(stderr, "Nepodarilo sa poslať JOIN.\n");
      client_send_leave(fd);
      client_net_close(fd);
      return 1;
  }

  // init shared state 
  client_state_t st;
  memset(&st, 0, sizeof(st));
  pthread_mutex_init(&st.mutex, NULL);
  st.fd = fd;
  st.running = 1;
  st.disconnected = 0;
  st.has_frame = 0;

  //vytvorenie thread 
  pthread_t net_thread;

  if (pthread_create(&net_thread, NULL, net_thread_fn, &st) != 0) {
    fprintf(stderr, "Nepodarilo sa spustiť NET thread.\n");
    client_send_leave(fd);
    client_net_close(fd);
    pthread_mutex_destroy(&st.mutex);
    return 1;
  }

  // ncurses init
  ui_init();

  // aby sa neposielal viacráz rovnaký smer;
  //direction_t last_dir = DIR_NONE;

  // lokálne buffre pre UI (kreslenie bez držania mutexu)
  int w = 0, h = 0, score = 0, paused = 0, time = 0;
  // buffer na grid: (MAX_H * (MAX_W+1)) je bezpečné, lebo frame validuje MAX_W/MAX_H
  static char grid_local[(MAX_H) * (MAX_W + 1)];
  int have_local = 0;

  while (1) {
    // kontrola či beží
    pthread_mutex_lock(&st.mutex);
    int running = st.running;
    int disconnected = st.disconnected;

    if (st.has_frame) {
      w = st.w;
      h = st.h;
      paused = st.paused;
      score = st.score;
      memcpy(grid_local, st.grid, sizeof(grid_local));
      have_local = 1;
    }
    pthread_mutex_unlock(&st.mutex);

    if (!running || disconnected) break;

    if (have_local) {  //vykresli frame ak je
      ui_draw(name , w, h, paused, score, time, grid_local);
    } else {
      ui_draw_waiting();
    }
    
    //klávesy (neblokuje, lebo ui_init má nodelay)
    int ch = ui_get_key();
    if (ch != -1) {
      if (ch == 'q' || ch == 'Q' || ch == 27) { // ESC
        // nastav stop
        client_send_leave(fd);
        pthread_mutex_lock(&st.mutex);
        st.running = 0;
        pthread_mutex_unlock(&st.mutex);
          
        //odblokuj recv v net threade
        shutdown(fd, SHUT_RDWR);
        break;
      }

      if (ch == 'p' || ch == 'P') {
        client_send_dir(fd, 'P'); // Pošle p ako direction
      }
      

      //if (paused > 0) {continue;}
      // mapuj kláves -> dir (DIR_NONE ak nič)
      direction_t dir = input_key_to_dir(ch);
      if (dir != DIR_NONE) { //&& dir != last_dir) {
        char dc = input_dir_to_char(dir); // 'U','D','L','R'
        client_send_dir(fd, dc);
        //last_dir = dir;
      }
    }
    usleep(10*1000); // 10ms
  }

  // cleanup
  ui_end();

  //ukončenie ak server ešte beží.
  client_send_leave(fd);

  // čakanie na thread
  pthread_join(net_thread, NULL);
  
  //zatvorenie socketu
  client_net_close(fd);

  //info nakoniec
  if (st.ended) {
    fprintf(stderr, "KONIEC HRY: %s\n", st.end_reason);
    //fprintf(stderr, "Press Enter...\n"); // idk čo to spravý
    //getchar();
  }
  else if (st.disconnected) {
    fprintf(stderr, "Server sa odpojil alebo nastala chyba siete.");
  }
  
  pthread_mutex_destroy(&st.mutex);
  return 0;
}
