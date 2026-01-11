#include "server_net.h"

//#include <arpa/inet.h>
#include <netinet/in.h>
//#include <sys/socket.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>

#include "../common/protocol.h"
//#include "../common/common.h"

int net_init(int port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return -1;
  }

  int opt = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
    
  // Priradenie socketu: IP a port
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(fd);
    return -1;
  }

  if (listen(fd, 5) < 0) {
    perror("listen");
    close(fd);
    return -1;
  }

  return fd;
}

int net_accept(int listen_fd) {
  int client_fd = accept(listen_fd, NULL, NULL);
  if (client_fd < 0) {
    perror("accept");
    return -1;
  }
  return client_fd;
}

int net_recv_line(int client_fd, char *buf, int size) {
  return recv_line(client_fd, buf, size);
}

int net_send_line(int client_fd, const char *line) {
  return send_line(client_fd, line);
}

void net_close(int fd) {
  if (fd >= 0) close(fd);
}
