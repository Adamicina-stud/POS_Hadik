#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../common/common.h"
#include "../common/protocol.h"

int send_grid_to_clients(int clients, char grid[MAX_W][MAX_H], int width, int height, int tick) {
  char message[256];
  snprintf(message, sizeof(message), "STATE %d %d %d", width, height, tick);
  send_line(clients, message);

  send_line(clients, "GRID");
  for (int h = 0; h <= height; h++) {
    char line[width];
    for (int w = 0; w <= width; w++) {
      line[w] = grid[w][h];
    }
    send_line(clients, line);
  }

  return 0;
}

int main(int argc, char *argv[]) {
  int listen_fd;
  int tick = 200000;
  int port = 5557;
  
  // Vytvorenie socketu
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    perror("socket");
    return 1;
  }

  // Nastavenie adresy servera
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;           // IPv4 
  addr.sin_addr.s_addr = INADDR_ANY;   // prijímame spojenia z hocikade
  addr.sin_port = htons(port);
  //if (argc >= 2) port = atoi(argv[1]);
  //addr.sin_port = htons((uint16_t)port);

  // Bind – priradíme socketu IP + port
  if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(listen_fd);
    return 1;
  }

  // Listen – začneme počúvať
  if (listen(listen_fd, 10) < 0) {
    perror("listen");
    close(listen_fd);
    return 1;
  }

  int width = GRID_W;
  int height = GRID_H;

  if (argc >= 3) {
    char *c;
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
  
  printf("Server beží na porte %d...\n", port);

  // Accept – čakáme na klienta
  int client_fd = accept(listen_fd, NULL, NULL);
  if (client_fd < 0) {
    perror("accept");
    close(listen_fd);
    return 1;
  }

  printf("Klient pripojený!\n");
  
  // Prečítame správu od klienta (JOIN)
  char recieved_line[256];
  recv_line(client_fd, recieved_line, sizeof(recieved_line));
  printf("Od klienta: %s", recieved_line);

  // Pošleme odpoveď (fake stav hry)
  char message[256];
  snprintf(message, sizeof(message), "%s %d %d %d", "STATE", width, height, tick);
  //send_line(client_fd, message);
  //send_line(client_fd, "GRID");
  
  char grid[width][height];
  for (int h = 0; h <= height; h++) {
    for (int w = 0; w <= width; w++) {
      grid[w][h] = '.';
    }
  }
  
  int temp_head_x = width / 2;
  int temp_head_y = height / 2;

  int a = 0;
  while (a < 10) {
    int last_x = temp_head_x;
    int last_y = temp_head_y;
    
    if (temp_head_y == 0){
      temp_head_y = height;
    } else {
      temp_head_y -= 1;
    }

    grid[last_x][last_y] = '.';
    grid[temp_head_x][temp_head_y] = '@';

    /*
    int last_x = head.x;
    int last_y = head.y;
    
    for (int client = 0; client <=  sizeof(clients) / sizeof(clients[0]); client++) {
      // Posunutie hlavy
      direction_t dir = DIR_UP;
      switch (dir) {
        case DIR_UP:
          head.y -= 1;
          break;
        case DIR_DOWN:
          head.y += 1;
          break;
        case DIR_LEFT:
          head.x -= 1;
          break;
        case DIR_RIGHT:
          head.x += 1;
          break;
        default:
          break;
      }
    
      // Posunutie tela
      for (int i = 0; i < MAX_SNAKE_LENGTH; i++) {
        if (body[i].x == -1) {                      // Prvy neexistujuci segment
          if (fruit_eaten > 0) {                    // ak had zjedol ovocie tak sa prida segment na koniec tela
            fruit_eaten--;
            body[i].x = last_x;
            body[i].y = last_y;
            break;
          }                                       
          grid[last_x][last_y] = ".";               // inak sa pozicia predosleho segmentu odstrani z gridu (lebo na tej pozicii uz nie je)
          break;
        }

        if (i == 0) grid[last_x][last_y] = "o";     // Prvy segment tela sa prida do gridu
        int last_x_temp = body[i].x;    // Pomocne premenne
        int last_y_temp = body[i].y;    // <---

        body[i].x = last_x;
        body[i].y = last_y;
        last_x = last_x_temp;
        last_y = last_y_temp;
      }
    }

    // Kontrola kolizii hlav 
    for (int client = 0; client <= sizeof(clients) / sizeof(clients[0]); client++) {
      switch (grid[head.x][head.y]) {
        case '*':                       // Ovocie
          fruit_eaten++;
        case '.':                       // Prazdne policko
          grid[head.x][head.y] = "@";
          break; 
        case '#':                       // Prekazka
        case 'o':                       // Telo hada
          game_over();
          break;
      }
    }
    add_fruit();

  void add_fruit() {
    int valid_position = 0;
    int x = 0;
    int y = 0;

    while (valid_position = 0) {
      x = rand() % width;
      y = rand() % height;
      
      if (grid[x][y] == '.') valid_position = 1;
    }

    grid[x][y] == '*';
    return;
  }

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

    // Posielanie gridu klientovi
    send_grid_to_clients(client_fd, grid, width, height, tick);
    /*
    send_line(client_fd, message);
    send_line(client_fd, "GRID");
    for (int h = 0; h <= height; h++) {
      char line[width];
      for (int w = 0; w <= width; w++) {
        line[w] = grid[w][h];
      }
      send_line(client_fd, line);
    }
    */
    usleep(tick);     // sleep(sekundy);
    a++;
  }
    

  // Zatvorenie
  close(client_fd);
  close(listen_fd);

  return 0;
}
