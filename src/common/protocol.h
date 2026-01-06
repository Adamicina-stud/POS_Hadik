#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>

// Pošle všetky bajty (blocking), vráti 0 alebo -1
int send_all(int fd, const void *buf, size_t len);

//Pošle C-string (bez nulového znaku), Vráti 0 alebo -1
int send_str(int fd, const char *s);

//Pošle riadok ukončený '\n'. Ak line neobsahuje '\n' doplní ho.
// Vráti 0 alebo -1
int send_line(int fd, const char *line);

//Prečíta riadok do buf (vrátane '\n' ak prišiel), vždy nul terminuje.
//Vráti počet prečítaných znakov (>0), 0 pro EOF, -1 pri chybe
int recv_line(int fd, char *buf, size_t buf_size);

#endif
