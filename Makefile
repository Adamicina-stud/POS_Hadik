CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -O2 -pthread
INCLUDES=-Isrc -Isrc/common -Isrc/server -Isrc/client

COMMON_SRCS=src/common/protocol.c
COMMON_HDRS=src/common/common.h src/common/protocol.h

SERVER_SRCS=src/server/server_main.c \
						src/server/server_game.c \
						src/server/server_net.c \
						$(COMMON_SRCS)

CLIENT_SRCS=src/client/client_main.c \
						src/client/client_net.c \
						src/client/client_ui.c \
						src/client/client_input.c \
						$(COMMON_SRCS)

all: server client

server: $(SERVER_SRCS) $(COMMON_HDRS)
	$(CC) $(CFLAGS) $(INCLUDES) $(SERVER_SRCS) -o server

client: $(CLIENT_SRCS) $(COMMON_HDRS)
	$(CC) $(CFLAGS) $(INCLUDES) $(CLIENT_SRCS) -lncurses -o client

clean:
	rm -f server client
