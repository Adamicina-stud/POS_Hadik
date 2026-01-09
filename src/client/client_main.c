
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

  int w, h, tick, score;
  char grid[GRID_MAX_H * (GRID_MAX_W + 1)];
  int has_frame;

  int running;
  int disconnected;
} client_state_t;

static void *net_thread_fn(void *arg) {
  client_state_t *st = (client_state_t *) arg;
  int fd = st->fd;
  
  //lokálne buffre (aby sme držali mutex čo najmenej)
  int w = 0, h = 0, tick = 0;
  char grid_buf[GRID_MAX_H * (GRID_MAX_W + 1)];

  while(1) {
    //kontrola či nastane koniec
    pthread_mutex_lock(&st->mutex);
    int running = st->running;
    pthread_mutex_unlock(&st->mutex);
    if (!running) break;

    int rc = client_recv_frame(fd, &w, &h, &tick, grid_buf, sizeof(grid_buf));
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
    st->tick = tick;
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

  // aby sa neposielal 1000x rovnaký smer;
  direction_t last_dir = DIR_NONE;

  // lokálne buffre pre UI (kreslenie bez držania mutexu)
  int w = 0, h = 0, tick = 0;
  // buffer na grid: (MAX_H * (MAX_W+1)) je bezpečné, lebo frame validuje MAX_W/MAX_H
  static char grid_local[(GRID_MAX_H) * (GRID_MAX_W + 1)];
  int have_local = 0;

  while (1) {
    // kontrola či beží
    pthread_mutex_lock(&st.mutex);
    int running = st.running;
    int disconnected = st.disconnected;

    if (st.has_frame) {
      w = st.w;
      h = st.h;
      tick = st.tick;
      memcpy(grid_local, st.grid, sizeof(grid_local));
      have_local = 1;
    }
    pthread_mutex_unlock(&st.mutex);

    if (!running || disconnected) break;

    if (have_local) {  //vykresli frame ak je
      ui_draw(w, h, tick, grid_local);
    } else {
      ui_draw_waiting();
    }

    // c) klávesy (neblokuje, lebo ui_init má nodelay)
    int ch = ui_get_key();
    if (ch != -1) {
      if (ch == 'q' || ch == 'Q' || ch == 27) { // ESC
        // nastav stop
        pthread_mutex_lock(&st.mutex);
        st.running = 0;
        pthread_mutex_unlock(&st.mutex);
          
        //odblokuj recv v net threade
        shutdown(fd, SHUT_RDWR);
        break;
      }

      // mapuj kláves -> dir (DIR_NONE ak nič)
      direction_t dir = input_key_to_dir(ch);
      if (dir != DIR_NONE && dir != last_dir) {
        char dc = input_dir_to_char(dir); // 'U','D','L','R'
        client_send_dir(fd, dc);
        last_dir = dir;
      }
    }

    //malý sleep aby to nežralo CPU 
    //usleep(16 * 1000); // ~60fps
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
  if (st.disconnected) {
    fprintf(stderr, "Server sa odpojil alebo nastala chyba siete.");
  }
  
  pthread_mutex_destroy(&st.mutex);
  return 0;
}
