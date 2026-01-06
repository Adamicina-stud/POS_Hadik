#include "protocol.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int send_all(int fd, const void *buf, size_t len) {
    const char *p = (const char*)buf;
    size_t sent = 0;

    while (sent < len) {
        ssize_t n = write(fd, p + sent, len - sent);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        sent += (size_t)n;
    }
    return 0;
}

int send_str(int fd, const char *s) {
    if (!s) return -1;
    return send_all(fd, s, strlen(s));
}

int send_line(int fd, const char *line) {
    if (!line) return -1;

    size_t len = strlen(line);
    if (len == 0) {
        return send_all(fd, "\n", 1);
    }

    // Ak už končí '\n', pošli priamo
    if (line[len - 1] == '\n') {
        return send_all(fd, line, len);
    }

    // Inak pošli line + '\n' (bez zbytočného malloc keď sa dá)
    if (send_all(fd, line, len) < 0) return -1;
    return send_all(fd, "\n", 1);
}

int recv_line(int fd, char *buf, size_t buf_size) {
    if (!buf || buf_size == 0) return -1;

    size_t i = 0;
    while (i + 1 < buf_size) {
        char c;
        ssize_t n = read(fd, &c, 1);
        if (n == 0) {
            // EOF
            if (i == 0) {
                buf[0] = '\0';
                return 0;
            }
            break;
        }
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }

        buf[i++] = c;
        if (c == '\n') break;
    }

    buf[i] = '\0';
    return (int)i;
}
