#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "message.h"
#include "packet.h"

// 뮤텍스 안 걸고 그냥 나한테 보낼 때 쓰기에 좋습니다.
void send_to_fd(const char *msg, int fd) {
    if (fd == -1)
        return ;
    send_packet(fd, PACKET_TYPE_MESSAGE, msg, strlen(msg));
    send(fd, msg, strlen(msg), 0);
}

void broadcast(const char *msg, Client* self) {
    Client *current = clients_head;
    while (current != NULL) {
        if (self == NULL || strcmp(current->name, self->name) != 0)
            send_packet(current->fd, PACKET_TYPE_MESSAGE, msg, strlen(msg));
        current = current->next;
    }
}

void unicast(const char *msg, Client* target) {
    if (target == NULL)
        return ;
    send_packet(target->fd, PACKET_TYPE_MESSAGE, msg, strlen(msg));
}

void multicast(const char *msg, Client* self) {
    if (self == NULL)
        return ;
    Client *current = clients_head;
    while (current != NULL) {
        if (strcmp(current->name, self->name) != 0 && self->chat_room == current->chat_room)
            send_packet(current->fd, PACKET_TYPE_MESSAGE, msg, strlen(msg));
        current = current->next;
    }
}
