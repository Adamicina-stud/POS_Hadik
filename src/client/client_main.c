#include <stdio.h>
#include <stdlib.h>

#include "client_net.h"
#include "client_ui.h"
#include "client_input.h"

#include "../common/common.h"

int main(int argc, char **argv) {
    const char *ip   = "127.0.0.1";
    int port         = DEFAULT_PORT;
    const char *name = "player";

    if (argc >= 2) ip = argv[1];
    if (argc >= 3) port = atoi(argv[2]);
    if (argc >= 4) name = argv[3];

    // 1) connect
    int fd = client_connect(ip, port);
    if (fd < 0) {
        fprintf(stderr, "Nepodarilo sa pripojiť na server.\n");
        return 1;
    }

    // 2) JOIN
    if (client_send_join(fd, name) < 0) {
        fprintf(stderr, "Nepodarilo sa poslať JOIN.\n");
        client_send_leave(fd);
        // close spravíš v net_close ak ho máš; ak nie, tak close(fd) tu
        return 1;
    }

    // 3) ncurses init
    ui_init();

    int w = 0, h = 0, tick = 0;

    // buffer na grid: (MAX_H * (MAX_W+1)) je bezpečné, lebo frame validuje MAX_W/MAX_H
    //static char grid_buf[(MAX_H) * (MAX_W + 1)];
    static char grid_buf[(1000) * (1000 + 1)];

    int running = 1;
    while (running) {
        // a) prijmi 1 frame
        int rc = client_recv_frame(fd, &w, &h, &tick, grid_buf, sizeof(grid_buf));
        if (rc == 0) {
            // server sa odpojil
            break;
        }
        if (rc < 0) {
            // chyba protokolu / čítania
            break;
        }

        // b) vykresli frame
        ui_draw(w, h, tick, grid_buf);

        // c) klávesy (neblokuje, lebo ui_init má nodelay)
        int ch = ui_get_key();
        if (ch != -1) {
            if (ch == 'q' || ch == 'Q' || ch == 27) { // ESC
                client_send_leave(fd);
                running = 0;
                break;
            }

            // mapuj kláves -> dir (DIR_NONE ak nič)
            direction_t dir = input_key_to_dir(ch);
            if (dir != DIR_NONE) {
                char dc = input_dir_to_char(dir); // 'U','D','L','R'
                client_send_dir(fd, dc);
            }
        }
    }

    // 4) cleanup
    ui_end();

    return 0;
}
