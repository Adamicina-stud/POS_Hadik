#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../common/common.h"
#include "../common/protocol.h"

int main(void) {
    int listen_fd;

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
    addr.sin_port = htons(5557);         // 5555
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

    printf("Server beží na porte %d...\n", DEFAULT_PORT);

    // Accept – čakáme na klienta
    int client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        close(listen_fd);
        return 1;
    }

    printf("Klient pripojený!\n");

    // Prečítame správu od klienta (JOIN)
    char line[256];
    recv_line(client_fd, line, sizeof(line));
    printf("Od klienta: %s", line);

    // Pošleme odpoveď (fake stav hry)
    send_line(client_fd, "STATE 40 20 0");
    send_line(client_fd, "GRID");

    for (int y = 0; y < GRID_H; y++) {
        send_line(client_fd, "........................................");
    }

    // Zatvorenie
    close(client_fd);
    close(listen_fd);

    return 0;
}
