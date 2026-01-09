#ifndef CLIENT_NET_H
#define CLIENT_NET_H

#include <stddef.h>

// Pripojí sa na server (TCP). Vráti socket fd alebo -1.
int client_connect(const char *ip, int port);

// Pošle JOIN <name>
int client_send_join(int fd, const char *name);

// Pošle DIR U/D/L/R (dir_char je 'U','D','L','R')
int client_send_dir(int fd, char dir_char);

// Pošle LEAVE
int client_send_leave(int fd);

// Načíta 1 frame od servera vo formáte:
// STATE w h tick
// GRID
// <h riadkov po w znakov>
// Výstup:
// - w, h, tick naplní
// - grid_out musí byť veľké aspoň (h * (w+1)) bajtov
//   (každý riadok uloží ako nul-terminovaný string za sebou)
// Vráti: 1 = OK, 0 = server sa odpojil, -1 = chyba
int client_recv_frame(int fd, int *w, int *h, int *tick, char *grid_out, size_t grid_out_size);

void client_net_close(int fd);

#endif
