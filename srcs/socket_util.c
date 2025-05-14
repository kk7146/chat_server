#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>

#include "client.h"
#include "socket_util.h"

void *accept_loop(void *arg) {
    int server_fd = *(int *)arg;
    free(arg);

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int *fd = malloc(sizeof(int));
        *fd = accept(server_fd, (struct sockaddr *)&cli_addr, &cli_len);
        if (*fd < 0) {
            free(fd);
            continue;
        }

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, fd);
        pthread_detach(tid);
    }
    return NULL;
}

int create_server_socket(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { INADDR_ANY }
    };
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);
    return server_fd;
}