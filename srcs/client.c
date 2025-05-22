#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "client.h"
#include "message.h"

// 이름 받고 클라이언트 등록 시도
static Client *name_register(int fd) {
    char name[NAME_LEN];

    send(fd, "Enter your name: ", 18, 0);
    int len = recv(fd, name, NAME_LEN - 1, 0);
    if (len <= 0) return NULL;

    name[len - 1] = '\0';
    if (find_client_name(clients_head, name)) {
        send(fd, "[System] That name is already taken.\n", 37, 0);
        return NULL;
    }

    pthread_mutex_lock(&mutex);
    Client *result = add_client(&clients_head, name, fd, DEFAULT_ROOM, NULL);
    pthread_mutex_unlock(&mutex);

    return result;
}

// 루프 돌면서 이름 등록 시도
static Client *handle_client_registration(int fd) {
    while (1) {
        Client *self = name_register(fd);
        if (self != NULL) {
            char buf[BUF_SIZE];
            snprintf(buf, sizeof buf, "%s joined the system.\n", self->name);
            broadcast(buf, NULL);
            printf("[LOG] %s connected\n", self->name);
            return self;
        }
        send(fd, "[System] Try another name.\n", 29, 0);
    }
    return NULL;
}

static void send_to_fd(const char *msg, int fd) { // unicast self는 락 걸어야 하는데 그럴 필요가 없을 때 쓰기 좋음.
    if (fd == -1)
        return ;
    send(fd, msg, strlen(msg), 0);
}

void *client_thread(void *arg) {
    int len;
    int fd = *(int *)arg;
    Client *self = handle_client_registration(fd);
    free(arg);
    char buf[BUF_SIZE];

    while ((len = recv(fd, buf, BUF_SIZE - 1, 0)) > 0) {
        buf[len] = '\0';
        char msg[BUF_SIZE + NAME_LEN];
        if (buf[0] == '/') {
            if (strncmp(buf, "/users", 6) == 0) {
                const char *info = show_users(0);
                send_to_fd(info, fd);
            }
            else if (strncmp(buf, "/chats", 6) == 0) {
                const char *info = show_rooms(0);
                send_to_fd(info, fd);
            }
            else if (strncmp(buf, "/dm", 3) == 0) {
                char *cmd = strtok(buf, " ");
                char *name_str = strtok(NULL, " ");
                char *msg_dm = strtok(NULL, "");
                if (msg_dm) {
                    char *newline = strchr(msg_dm, '\n');
                    if (newline) *newline = '\0';
                }
                snprintf(msg, sizeof msg, "[%s] dm(%s)\n", self->name, msg_dm);
            
                pthread_mutex_lock(&mutex);
                Client *target = find_by_client_name(clients_head, name_str);
                if (target && msg_dm) {
                    unicast(msg, target);
                } else {
                    unicast("Usage: /dm <id> <message>\n", self);
                }
                pthread_mutex_unlock(&mutex);
            }
            else if (strncmp(buf, "/start", 6) == 0) {
                char *cmd = strtok(buf, " ");
                char *target_name = strtok(NULL, " \n");
            
                if (target_name) {
                    pthread_mutex_lock(&mutex);
                    Client* target = find_by_client_name(clients_head, target_name);
                    if (target != NULL) {
                        target->pending_request_from = self;
                        snprintf(msg, sizeof msg, "[System] %s has requested a 1:1 chat. Type /accept to join.\n", self->name);
                        unicast(msg, target);
                    } else {
                        unicast("[System] User not found.\n", self);
                    }
                    pthread_mutex_unlock(&mutex);
                } else {
                    send_to_fd("Usage: /start <id>\n", fd);
                }
            }
            else if (strncmp(buf, "/accept", 7) == 0) {
                Client *requester = self->pending_request_from;
                if (requester != NULL && requester->fd != -1) {
                    char room_name[NAME_LEN * 2 + 4];
                    snprintf(room_name, sizeof(room_name), "%s&%s", requester->name, self->name);

                    pthread_mutex_lock(&mutex);
                    Room *room_current = add_room(&rooms_head, room_name, self->fd, 2);
                    if (room_current != NULL) {
                        join_room(requester, room_current);
                        join_room(self, room_current);
                        self->pending_request_from = NULL;
            
                        snprintf(msg, sizeof msg, "[System] 1:1 chat with %s started.\n", requester->name);
                        unicast(msg, self);
            
                        snprintf(msg, sizeof msg, "[System] %s accepted your request.\n", self->name);
                        unicast(msg, requester);
                        printf("[LOG] %s created room '%s' (size %d)\n", self->name, room_name, 2);
                    } else {
                        send_to_fd("[System] Failed to create room.\n", fd);
                    }
                    pthread_mutex_unlock(&mutex);
                } else {
                    send_to_fd("[System] No pending requests.\n", fd);
                }
            }
            else if (strncmp(buf, "/new", 4) == 0) {
                char *cmd = strtok(buf, " ");
                char *room_name = strtok(NULL, " ");
                char *size_str = strtok(NULL, " ");
                if (!room_name || !size_str) {
                    send_to_fd("[System] Usage: /new <room_name> <room_size>\n", fd);
                }
                else {
                    int room_size = atoi(size_str);
                    pthread_mutex_lock(&mutex);
                    Room *room_id = add_room(&rooms_head, room_name, self->fd, room_size);
                    pthread_mutex_unlock(&mutex);
                    if (room_id == NULL)
                        send_to_fd("[System] Failed to create a chat room.\n", fd);
                    else {
                        send_to_fd("[System] Chat room created successfully.\n", fd);
                        printf("[LOG] %s created room '%s' (size %d)\n", self->name, room_name, room_size);
                    }
                }
            }
            else if (strncmp(buf, "/enter", 6) == 0) {
                char *cmd = strtok(buf, " ");
                char *room_str = strtok(NULL, " ");
                if (!room_str) {
                    send_to_fd("[System] Usage: /enter <room_idx>\n", fd);
                }
                else {
                    int room_id = atoi(room_str);
                    if (room_id < 0) {
                        send_to_fd("[System] Invalid room ID.\n", fd);
                    }
                    else if (join_room(self, room_id) == -1)
                        send_to_fd("[System] Failed to enter the chat room.\n", fd);
                    else
                        send_to_fd("[System] You have entered the chat room.\n", fd);
                }
            }
            else if (strncmp(buf, "/exit", 5) == 0) {
                pthread_mutex_lock(&mutex);
                int result = leave_room(self);
                pthread_mutex_unlock(&mutex);
            
                if (result == 0) {
                    send_to_fd("[System] You left the room.\n", fd);
                } else {
                    send_to_fd("[System] You are not in a room.\n", fd);
                }
            }
            else {
                send_to_fd("[System] Unknown command.\n", fd);
            }
        }
        else{
            snprintf(msg, sizeof(msg) - 1, "[%s] %s", self->name, buf);
            msg[sizeof(msg) - 1] = '\0';
            pthread_mutex_lock(&mutex);
            multicast(msg, self);
            pthread_mutex_unlock(&mutex);
        }
    }

    pthread_mutex_lock(&mutex);
    if (self) {
        snprintf(buf, sizeof buf, "%s left the system.\n", self->name);
        broadcast(buf, NULL);
        printf("[LOG] %s disconnected\n", self->name);
        remove_client(&clients_head, self->name);
    } else {
        printf("[LOG] unknown client disconnected\n");
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}
