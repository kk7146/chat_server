#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "message.h"
#include "packet.h"

void send_packet_to(Client *target, PacketType type, const char *msg) {
    if (target == NULL || msg == NULL) return;
    send_packet(target->fd, type, msg, strlen(msg));
}

void broadcast(const char *msg, Client* self) {
    Client *current = clients_head;
    while (current != NULL) {
        if (self == NULL || strcmp(current->name, self->name) != 0)
            send(current->fd, msg, strlen(msg), 0);
        current = current->next;
    }
}

void unicast(const char *msg, Client* target) {
    if (target == NULL)
        return ;
    send(target->fd, msg, strlen(msg), 0);
}

void multicast(const char *msg, Client* self) {
    if (self == NULL)
        return ;
    Client *current = clients_head;
    while (current != NULL) {
        if (strcmp(current->name, self->name) != 0 && self->chat_room == current->chat_room)
            send(current->fd, msg, strlen(msg), 0);
        current = current->next;
    }
}
