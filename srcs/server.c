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
