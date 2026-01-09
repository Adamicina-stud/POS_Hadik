#ifndef SERVER_NET_H
#define SERVER_NET_H

// Inicializuje server socket (socket + bind + listen)
// vráti listen_fd alebo -1
int net_init(int port);

// Prijme nového klienta (accept)
// vráti client_fd alebo -1
int net_accept(int listen_fd);

// Prečíta 1 riadok od klienta (JOIN / DIR / LEAVE)
// buffer musí mať aspoň size bajtov
// návrat:
//  >0 počet bajtov
//   0 klient sa odpojil
//  <0 chyba
int net_recv_line(int client_fd, char *buf, int size);

// Pošle 1 riadok klientovi (pridá \n)
int net_send_line(int client_fd, const char *line);

// Zavrie socket klienta
void net_close(int fd);

#endif
