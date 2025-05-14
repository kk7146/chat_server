#include <stdio.h>
#include <string.h>
#include "server.h"

int create_room(const char *name) {
    for (int i = 0; i < MAX_ROOM; i++) {
        if (rooms[i].id == -1) {
            rooms[i].id = i;
            strncpy(rooms[i].name, name, NAME_LEN);
            for (int j = 0; j < MAX_ROOM_CLIENTS; j++)
                rooms[i].users[j] = NULL;
            return i;
        }
    }
    return -1; // 실패 (룸 자리가 없음)
}

int join_room(int client_idx, int room_id) {
    for (int i = 0; i < MAX_ROOM; i++) {
        if (rooms[i].id == room_id) {
            for (int j = 0; j < MAX_ROOM_CLIENTS; j++) {
                if (rooms[i].users[j] == NULL) {
                    rooms[i].users[j] = &clients[client_idx];
                    clients[client_idx].chat_room = room_id;
                    pthread_mutex_unlock(&mutex);
                    return 0;
                }
            }
            return -1; // 실패 (룸에 사람이 꽉 참)
        }
    }
    return -1; // 실패 (해당 룸 없음)
}

int leave_room(int client_idx) {
    int current_room = clients[client_idx].chat_room;
    for (int i = 0; i < MAX_ROOM; i++) {
        if (rooms[i].id == current_room) {
            for (int j = 0; j < MAX_ROOM_CLIENTS; j++) {
                if (rooms[i].users[j] == &clients[client_idx]) {
                    rooms[i].users[j] = NULL;
                    clients[client_idx].chat_room = DEFAULT_ROOM;
                    return 0;
                }
            }
        }
    }
    return -1; // 실패 (해당 룸 없음)
}