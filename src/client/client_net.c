#include "client_net.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../common/protocol.h"
#include "../common/common.h"

int client_connect(const char *ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP: %s\n", ip);
        close(fd);
        return -1;
    }

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return -1;
    }

    return fd;
}

int client_send_join(int fd, const char *name) {
    if (!name) name = "Player";
    char msg[128];
    snprintf(msg, sizeof(msg), "JOIN %s\n", name);
    return send_str(fd, msg);
}

int client_send_dir(int fd, char dir_char) {
    // očakávame 'U','D','L','R'
    char msg[16];
    snprintf(msg, sizeof(msg), "DIR %c\n", dir_char);
    return send_str(fd, msg);
}

int client_send_leave(int fd) {
    return send_line(fd, "LEAVE");
}

static int parse_state_line(const char *line, int *w, int *h, int *tick) {
    // vráti 1 ak OK, inak 0
    int ww, hh, tt;
    int ok = sscanf(line, "STATE %d %d %d", &ww, &hh, &tt);
    if (ok != 3) return 0;

    if (ww <= 0 || hh <= 0 || ww > GRID_MAX_W || hh > GRID_MAX_H) return 0;

    if (w) *w = ww;
    if (h) *h = hh;
    if (tick) *tick = tt;
    return 1;
}

int client_recv_frame(int fd, int *w, int *h, int *tick, char *grid_out, size_t grid_out_size) {
    if (!grid_out || grid_out_size == 0) return -1;

    char line[2048];

    // 1) STATE
    int n = recv_line(fd, line, sizeof(line));
    if (n == 0) return 0;       // server sa odpojil
    if (n < 0) return -1;

    if (!parse_state_line(line, w, h, tick)) {
        fprintf(stderr, "Bad STATE line: %s", line);
        return -1;
    }

    // 2) GRID
    n = recv_line(fd, line, sizeof(line));
    if (n == 0) return 0;
    if (n < 0) return -1;

    if (strncmp(line, "GRID", 4) != 0) {
        fprintf(stderr, "Expected GRID, got: %s", line);
        return -1;
    }

    int ww = *w;
    int hh = *h;

    // potrebujeme uložiť hh riadkov, každý max (ww + 1) bajtov (nul-terminácia)
    size_t need = (size_t)hh * (size_t)(ww + 1);
    if (grid_out_size < need) {
        fprintf(stderr, "grid_out buffer too small (need %zu)\n", need);
        return -1;
    }

    // 3) riadky gridu
    for (int y = 0; y < hh; y++) {
        n = recv_line(fd, line, sizeof(line));
        if (n == 0) return 0;
        if (n < 0) return -1;

        // odstráň '\n' na konci, aby sa to dobre kreslilo
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

        // ak server pošle kratší riadok, doplníme bodkami (aby ncurses nekreslilo bordel)
        // ak pošle dlhší, skrátime na ww
        char *dst = grid_out + (size_t)y * (size_t)(ww + 1);
        size_t copy_len = strlen(line);
        if (copy_len > (size_t)ww) copy_len = (size_t)ww;

        memset(dst, '.', (size_t)ww);
        memcpy(dst, line, copy_len);
        dst[ww] = '\0';
    }

    return 1;
}

void client_net_close(int fd) {
  close(fd);
}
