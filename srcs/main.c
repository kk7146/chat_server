#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netinet/in.h>
#include "socket_util.h"
#include "admin.h"
#include "server.h"

int main() {
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i].fd = -1;
    for (int i = 0; i < MAX_ROOMS; i++) rooms[i].id = -1;

    create_room("Main room");

    int chat_fd = create_server_socket(CHAT_PORT);
    int admin_fd = create_server_socket(ADMIN_PORT);

    pthread_t chat_tid, admin_tid;
    int *chat_arg = malloc(sizeof(int)); *chat_arg = chat_fd;
    int *admin_arg = malloc(sizeof(int)); *admin_arg = admin_fd;

    pthread_create(&chat_tid, NULL, accept_loop, chat_arg);
    pthread_create(&admin_tid, NULL, admin_loop, admin_arg);

    printf("서버 시작: 채팅 포트 %d, 관리자 포트 %d\n", CHAT_PORT, ADMIN_PORT);

    pthread_join(chat_tid, NULL);
    pthread_join(admin_tid, NULL);
    close(chat_fd);
    close(admin_fd);
    return 0;
}