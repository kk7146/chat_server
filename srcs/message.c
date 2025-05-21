#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "message.h"

// 여기 근데 pthread_mutex_lock 안 걸어도 괜찮지 않나? 싶긴 하지만..

void broadcast(const char *msg, Client* self) {
    pthread_mutex_lock(&mutex);
    Client *current = clients_head;
    while (current != NULL) {
        if (self == NULL || strcmp(current->name, self->name) != 0)
            send(current->fd, msg, strlen(msg), 0);
        current = current->next;
    }
    pthread_mutex_unlock(&mutex);
}

void unicast(const char *msg, Client* target) {
    if (target == NULL)
        return ;
    pthread_mutex_lock(&mutex);
    Client *current = clients_head;
    while (current != NULL) {
        if (strcmp(current->name, target->name) == 0)
            send(current->fd, msg, strlen(msg), 0);
        current = current->next;
    }
    pthread_mutex_unlock(&mutex);
}

void multicast(const char *msg, Client* self) {
    if (self == NULL)
        return ;
    pthread_mutex_lock(&mutex);
    Client *current = clients_head;
    while (current != NULL) {
        if (strcmp(current->name, self->name) != 0 && self->chat_room == current->chat_room)
            send(current->fd, msg, strlen(msg), 0);
        current = current->next;
    }
    pthread_mutex_unlock(&mutex);
}
