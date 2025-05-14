#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "client.h"
#include "room.h"
#include "message.h"

int register_client(int fd, const char *name) {
    int idx = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == -1) {
            clients[i].fd = fd;
            strncpy(clients[i].name, name, NAME_LEN);
            clients[i].tid = pthread_self();
            clients[i].chat_room = DEFAULT_ROOM;
            clients[i].pending_request_from = -1;
            idx = i;
            return idx;
        }
    }
}

void *client_thread(void *arg) {
    int fd = *(int *)arg;
    free(arg);
    char buf[BUF_SIZE];
    char name[NAME_LEN];

    send(fd, "Enter your name: ", 18, 0);
    int len = recv(fd, name, NAME_LEN - 1, 0);
    if (len <= 0) {
        close(fd);
        return NULL;
    }
    name[len - 1] = '\0';

    pthread_mutex_lock(&mutex);
    int idx = register_client(fd, name);
    pthread_mutex_unlock(&mutex);

    if (idx == -1) {
        send(fd, "Too many users. Try later.\n", 28, 0);
        close(fd);
        return NULL;
    }

    snprintf(buf, sizeof buf, "%s joined the system.\n", name);
    broadcast(buf, fd);
    printf("[+] %s connected\n", name);

    while ((len = recv(fd, buf, BUF_SIZE - 1, 0)) > 0) {
        buf[len] = '\0';
        char msg[BUF_SIZE + NAME_LEN];
        if (buf[0] == '/') {
            if (strncmp(buf, "/users", 6) == 0) {
                const char *info = show_users(0);
                send(fd, info, strlen(info), 0);
            }
            else if (strncmp(buf, "/chats", 6) == 0) {
                const char *info = show_rooms(0);
                send(fd, info, strlen(info), 0);
            }
            else if (strncmp(buf, "/dm", 3) == 0) {
                char *cmd = strtok(buf, " ");
                char *id_str = strtok(NULL, " ");
                char *msg_dm = strtok(NULL, "");
                if (msg_dm) {
                    char *newline = strchr(msg_dm, '\n');
                    if (newline) *newline = '\0';
                }
                snprintf(msg, sizeof msg, "[%s] dm(%s)\n", name, msg_dm);
            
                if (id_str && msg_dm) {
                    unicast(msg, id_str);
                } else {
                    send(fd, "Usage: /dm <id> <message>\n", 27, 0);
                }
            }
            else if (strncmp(buf, "/start", 6) == 0) {
                char *cmd = strtok(buf, " ");
                char *target_name = strtok(NULL, " \n");
            
                if (target_name) {
                    int target_idx = -1;
                    pthread_mutex_lock(&mutex);
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i].fd != -1 && strcmp(clients[i].name, target_name) == 0) {
                            target_idx = i;
                            break;
                        }
                    }
                    if (target_idx != -1) {
                        clients[target_idx].pending_request_from = idx;
                        snprintf(msg, sizeof msg, "[System] %s has requested a 1:1 chat. Type /accept to join.\n", name);
                        send(clients[target_idx].fd, msg, strlen(msg), 0);
                    } else {
                        send(fd, "[System] User not found.\n", 26, 0);
                    }
                    pthread_mutex_unlock(&mutex);
                } else {
                    send(fd, "Usage: /start <id>\n", 20, 0);
                }
            }
            else if (strncmp(buf, "/accept", 7) == 0) {
                pthread_mutex_lock(&mutex);
                int requester = clients[idx].pending_request_from;
                if (requester != -1 && clients[requester].fd != -1) {
                    char room_name[NAME_LEN * 2 + 4];
                    snprintf(room_name, sizeof(room_name), "%s&%s", clients[requester].name, clients[idx].name);
            
                    int room_id = create_room(room_name);
                    if (room_id != -1) {
                        join_room(requester, room_id);
                        join_room(idx, room_id);
                        clients[idx].pending_request_from = -1;
            
                        snprintf(msg, sizeof msg, "[System] 1:1 chat with %s started.\n", clients[requester].name);
                        send(clients[idx].fd, msg, strlen(msg), 0);
            
                        snprintf(msg, sizeof msg, "[System] %s accepted your request.\n", clients[idx].name);
                        send(clients[requester].fd, msg, strlen(msg), 0);
                    } else {
                        send(clients[idx].fd, "[System] Failed to create room.\n", 33, 0);
                    }
                } else {
                    send(clients[idx].fd, "[System] No pending requests.\n", 30, 0);
                }
                pthread_mutex_unlock(&mutex);
            }
            else if (strncmp(buf, "/new", 4) == 0) {
                char *cmd = strtok(buf, " ");
                char *room_str = strtok(NULL, " ");
                if (!room_str) {
                    send(fd, "[System] Usage: /new <room_name>\n", 34, 0);
                }
                else {
                    int room_id = create_room(room_str);
                    if (room_id == -1)
                        send(fd, "[System] Failed to create a chat room.\n", 38, 0);
                    else
                        send(fd, "[System] Chat room created successfully.\n", 43, 0);
                }
            }
            else if (strncmp(buf, "/enter", 6) == 0) {
                char *cmd = strtok(buf, " ");
                char *room_str = strtok(NULL, " ");
                if (!room_str) {
                    send(fd, "[System] Usage: /enter <room_idx>\n", 33, 0);
                }
                else {
                    int room_id = atoi(room_str);
                    if (room_id <= 0) {
                        send(fd, "[System] Invalid room ID.\n", 28, 0);
                    }
                    if (join_room(idx, room_id) == -1)
                        send(fd, "[System] Failed to enter the chat room.\n", 40, 0);
                    else
                        send(fd, "[System] You have entered the chat room.\n", 41, 0);
                }
            }
            else if (strncmp(buf, "/exit", 5) == 0) {
                pthread_mutex_lock(&mutex);
                int result = leave_room(idx);
                pthread_mutex_unlock(&mutex);
            
                if (result == 0) {
                    send(fd, "[System] You left the 1:1 chat.\n", 32, 0);
                } else {
                    send(fd, "[System] You are not in a room.\n", 33, 0);
                }
            }
            else {
                send(fd, "[System] Unknown command.\n", 27, 0);
            }
        }
        else{
            snprintf(msg, sizeof msg, "[%s] %s", name, buf); // 오버플로 위험 있다는 거 발견..
            multicast(msg, fd);
        }
    }

    pthread_mutex_lock(&mutex);
    clients[idx].fd = -1;
    pthread_mutex_unlock(&mutex);

    snprintf(buf, sizeof buf, "%s left the system.\n", name);
    broadcast(buf, -1);
    printf("[-] %s disconnected\n", name);
    close(fd);
    return NULL;
}
