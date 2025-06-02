#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "admin.h"

void *admin_thread(void *arg) {
    int admin_fd = *(int *)arg;
    free(arg);
    char cmd[BUF_SIZE];

    while (1) {
        int len = recv(admin_fd, cmd, sizeof(cmd) - 1, 0);
        if (len <= 0) break;
        cmd[len] = '\0';

        if (strncmp(cmd, "users", 5) == 0) {
            const char *info = show_clients(1);
            send(admin_fd, info, strlen(info), 0);
        }
        if (strncmp(cmd, "chats", 5) == 0) {
            const char *info = show_rooms(1);
            send(admin_fd, info, strlen(info), 0);
        }
    }
    close(admin_fd);
    return NULL;
}

void *admin_loop(void *arg) {
    int admin_fd = *(int *)arg;
    free(arg);

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int *fd = malloc(sizeof(int));
        *fd = accept(admin_fd, (struct sockaddr *)&cli_addr, &cli_len);
        if (*fd < 0) {
            free(fd);
            continue;
        }

        pthread_t tid;
        pthread_create(&tid, NULL, admin_thread, fd);
        pthread_detach(tid);
    }
    return NULL;
}