#include <arpa/inet.h>     // Práca s IP adresami
#include <netinet/in.h>    // sockaddr_in štruktúra
#include <sys/socket.h>    // funkcie pre sockety
#include <unistd.h>        // read, write, close
#include <stdio.h>         // printf, perror
#include <string.h>
#include <stdlib.h>        // atoi, exit

#include "../common/common.h"    // naše konštanty (GRID_W, GRID_H, port…)
#include "../common/protocol.h"  // naše funkcie na posielanie/čítanie správ

// --- Funkcia ktorá vytvorí socket a pripojí sa na server ---
static int connect_to(const char *ip, int port) {
    // 1) Vytvoríme socket (ako keby sme vytvorili komunikačný “telefón”)
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket failed"); // vypíše chybu ak sa socket nepodarí vytvoriť
        return -1;
    }

    // 2) Pripravíme si adresu servera (kam sa chceme pripojiť)
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr)); // vynulujeme štruktúru
    addr.sin_family = AF_INET;      // IPv4
    addr.sin_port = htons(port);    // Port pre server

    // 3) Prekonvertujeme IP string na binárnu formu
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        printf("Zlá IP adresa!\n");
        close(fd);
        return -1;
    }

    // 4) Pripojíme sa na server (vytočíme “číslo” servera)
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect failed"); // vypíše chybu ak server nebeží alebo je zlý port/IP
        close(fd);
        return -1;
    }

    return fd; // vrátime file‑descriptor socketu (to je číslo nášho spojenia)
}

int main(int argc, char **argv) {

  // Default hodnoty (ak by sme nič nezadali v termináli)
  const char *ip = "127.0.0.1";  // lokálny server
  int port = DEFAULT_PORT;       // 5555 (z common.h)
  const char *name = "Fenrir60"; // meno hráča

  // Ak zadáme argumenty v termináli, tak si ich vezmeme
  if (argc >= 2) ip = argv[1];
  if (argc >= 3) port = atoi(argv[2]);
  if (argc >= 4) name = argv[3];

  // Zavoláme funkciu a pripojíme sa na server
  int sock_fd = connect_to(ip, port);
  if (sock_fd < 0) {
    printf("Nepodarilo sa pripojiť na server 😢\n");
    return 1; // ukončíme program s chybou
  }

  printf("Pripojený na server! 🔥\n");

  // Pošleme serveru JOIN správu
  char msg[128];
  snprintf(msg, sizeof(msg), "JOIN %s\n", name);

  if (send_str(sock_fd, msg) < 0) {
      perror("send failed");
      close(sock_fd);
      return 1;
  }

  // Čítame odpoveď od servera a vypíšeme ju
  char line[512];

  while(1) {
    // Prečítanie STATE
    int n = recv_line(sock_fd, line, sizeof(line));
    if (n <= 0) return 1;

    int width, height, tick;
    int ok = sscanf(line, "STATE %d %d %d", &width, &height, &tick);
    if (ok != 3) {
      fprintf(stderr, "Bad STATE line: %s", line);
      close(sock_fd);
      return 1;
    }

    printf("Server posiela grid %dx%d\n", width, height);

    if (width <= 0 || height <=0) { //pridať kontrolu max vyšky a širky ked bude v common/
      fprintf(stderr, "grid size out of range: %dx%d\n", width, height);
    }

    //Prečitanie GRID 
    n = recv_line(sock_fd, line, sizeof(line));
    if (n <= 0) return 1;
    if (strncmp(line, "GRID", 4) != 0) {
      fprintf(stderr, "Expected GRID, got: %s\n", line);
    }

    for (int y = 0; y < height; y++) 
    {
      n = recv_line(sock_fd, line, sizeof(line));
      if(n <= 0) break;
      printf("GRID[%d]: %s", y, line);
    }
  }

  // Zatvoríme spojenie (ako keby sme zložili hovor)
  close(sock_fd);
  printf("Spojenie ukončené.\n");

  return 0;
}
