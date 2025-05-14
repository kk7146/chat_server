#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "message.h"

void broadcast(const char *msg, int exclude_fd) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1 && clients[i].fd != exclude_fd) {
            send(clients[i].fd, msg, strlen(msg), 0);
        }
    }
    pthread_mutex_unlock(&mutex);
}

int find_users_tid() {
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].tid == pthread_self())
            return i;
}

void unicast(const char *msg, char dest_idx[NAME_LEN]) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1 && strcmp(clients[i].name, dest_idx) == 0) {
            send(clients[i].fd, msg, strlen(msg), 0);
        }
    }
    pthread_mutex_unlock(&mutex);
}

void multicast(const char *msg, int exclude_fd) {
    pthread_mutex_lock(&mutex);
    int user_idx = find_users_tid();
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1 && clients[i].fd != exclude_fd && clients[i].chat_room == clients[user_idx].chat_room) {
            send(clients[i].fd, msg, strlen(msg), 0);
        }
    }
    pthread_mutex_unlock(&mutex);
}