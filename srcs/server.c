#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "server.h"

Client clients[MAX_CLIENTS];
Room rooms[MAX_ROOMS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

char *show_users(int isAdmin) {
    static char list[BUF_SIZE * 4];
    pthread_mutex_lock(&mutex);
    strcpy(list, "============== Connected Users ==============\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) {
            char line[128];
            snprintf(line, sizeof(line), "ID:%s", clients[i].name);

            if (isAdmin) {
                char admin_info[128];
                snprintf(admin_info, sizeof(admin_info), "\tTID:%lu\tRoom:%d", 
                        (unsigned long)clients[i].tid, clients[i].chat_room);
                strcat(line, admin_info);
            }
            strcat(line, "\n");
            strcat(list, line);
        }
    }

    strcat(list, "=============================================\n");
    pthread_mutex_unlock(&mutex);
    return list;
}

char *show_rooms(int isAdmin) {
    static char list[BUF_SIZE * 4];
    pthread_mutex_lock(&mutex);
    strcpy(list, "======== Available Chat Rooms ========\n");

    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].id != -1) {
            char line[128];
            snprintf(line, sizeof(line), "Room ID:%d    Room Name:%s", rooms[i].id, rooms[i].name);

            if (isAdmin) {
                //char admin_info[128];
                //snprintf(admin_info, sizeof(admin_info), "\tTID:%lu\tRoom:%d", 
                //        (unsigned long)rooms[i].tid, rooms[i].chat_room);
                //strcat(line, admin_info);
            }
            strcat(line, "\n");
            strcat(list, line);
        }
    }

    strcat(list, "======================================\n");
    pthread_mutex_unlock(&mutex);
    return list;
}

Client* find_by_client_name(Client *head, char *name) {
    Client *current = head;

    if (name == NULL || head == NULL) return NULL;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

Client* find_by_client_fd(Client *head, int fd) {
    Client *current = head;

    if (head == NULL) return NULL;
    while (current != NULL) {
        if (current->fd == fd) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void free_all_clients(Client **head) {
    Client *current = *head;
    while (current != NULL) {
        Client *next = current->next;
        free(current);
        current = next;
    }
    *head = NULL;
}

int add_client(Client **head, char *name, int fd, int chat_room_id, int pending_request_from) {
    Client *new_node = (Client *)malloc(sizeof(Client));
    if (new_node == NULL) {
        send(fd, "[System] internal error\n", 25, 0);
        return -1;
    }
    
    // 똑같은 이름의 유저의 경우
    if (find_by_client_name(*head, name) == NULL) {
        send(fd, "[System] That username is already taken.\n", 39, 0);
        return -1;
    }

    // 값 넣기
    strncpy(new_node->name, name, NAME_LEN - 1);
    new_node->name[NAME_LEN - 1] = '\0';
    new_node->fd = fd;
    new_node->chat_room = chat_room_id;
    new_node->tid = pthread_self();
    new_node->pending_request_from = pending_request_from;

     // 첫 유저일 경우
    if (*head == NULL) {
        new_node->prev = NULL;
        *head = new_node;
        return (0);
    }

    // 맨 끝 노드 찾이서 넣기
    Client *tail = *head;
    while (tail->next != NULL) {
        tail = tail->next;
    }
    tail->next = new_node;
    new_node->prev = tail;
    return (0);
}

void remove_client(Client **head, const char *name) {
    Client *remove_node = find_by_client_name(*head, name);

    if (remove_node == NULL)
        return ;
    if (remove_node->prev != NULL)
        remove_node->prev->next = remove_node->next;
    else
        *head = remove_node->next;
    if (remove_node->next != NULL)
        remove_node->next->prev = remove_node->prev;
    free(remove_node);
    return;
}
