#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "client.h"
#include "message.h"
#include "packet.h"

// 이름 받고 클라이언트 등록
static Client *name_register(int fd) {
    PacketHeader header;
    char name[NAME_LEN];

    send_to_fd("Enter your name:\n", fd, PACKET_TYPE_MESSAGE);
    if (recv_packet(fd, &header, name, sizeof(name)) < 0) {
        return NULL;
    }
    if (header.type != PACKET_TYPE_LOGIN) {
        send_to_fd("[System] Invalid packet type for name registration.\n\n", fd, PACKET_TYPE_MESSAGE);
        return NULL;
    }
    name[header.length < NAME_LEN - 1 ? header.length : NAME_LEN - 1] = '\0';
    if (find_by_client_name(clients_head, name)) {
        send_to_fd("[System] That name is already taken.\n\n", fd, PACKET_TYPE_MESSAGE);
        return NULL;
    }
    pthread_mutex_lock(&mutex);
    Client *result = add_client(&clients_head, name, fd, DEFAULT_ROOM, NULL);
    pthread_mutex_unlock(&mutex);

    return result;
}

// 루프 돌면서 이름 등록
static Client *handle_client_registration(int fd) {
    while (1) {
        Client *self = name_register(fd);
        if (self != NULL) {
            char buf[BUF_SIZE];
            snprintf(buf, sizeof buf, "[System] %s joined the system.\n", self->name);
            broadcast(buf, NULL, PACKET_TYPE_MESSAGE);
            printf("[LOG] %s connected\n", self->name);
            return self;
        }
        send_to_fd("[System] Try another name.\n", fd, PACKET_TYPE_MESSAGE);
    }
    return NULL;
}

static int request_kick_client(Client *kicker, Client *target) {
    if (target == NULL) {
        send_to_fd("[System] Invalid client name.\n", kicker->fd, PACKET_TYPE_MESSAGE);
        return -1;
    }
    Room *target_room = find_by_room_id(rooms_head, target->chat_room);
    if (target_room == NULL) {
        send_to_fd("[System] Invalid room name.\n", kicker->fd, PACKET_TYPE_MESSAGE);
        return -1;
    }
    if (target == kicker) {
        send_to_fd("[System] You cannot kick yourself.\n", kicker->fd, PACKET_TYPE_MESSAGE);
        return -1;
    }
    if (target->chat_room != kicker->chat_room) {
        send_to_fd("[System] That user is not in your room.\n", kicker->fd, PACKET_TYPE_MESSAGE);
        return -1;
    }
    if (strcmp(target_room->host_name, kicker->name) != 0) {
        send_to_fd("[System] You are not the host of this room.\n", kicker->fd, PACKET_TYPE_MESSAGE);
        return -1;
    }

    char notice[BUF_SIZE];
    snprintf(notice, sizeof(notice),
             "[System] User '%s' has been kicked by '%s'.\n",
             target->name, kicker->name);
    multicast(notice, kicker, PACKET_TYPE_MESSAGE);
    send_to_fd(notice, kicker->fd, PACKET_TYPE_MESSAGE);

    send_to_fd("[System] You have been kicked from the room.\n", target->fd, PACKET_TYPE_MESSAGE);
    leave_room(target);
    return 0;
}

static int request_change_host(Client *host, Client *target) {
    if (target == NULL) {
        send_to_fd("[System] Invalid client name.\n", host->fd, PACKET_TYPE_MESSAGE);
        return -1;
    }

    Room *target_room = find_by_room_id(rooms_head, target->chat_room);
    if (target_room == NULL) {
        send_to_fd("[System] Invalid room name.\n", host->fd, PACKET_TYPE_MESSAGE);
        return -1;
    }

    if (host == target) {
        send_to_fd("[System] You cannot change yourself.\n", host->fd, PACKET_TYPE_MESSAGE);
        return -1;
    }

    if (target->chat_room != host->chat_room) {
        send_to_fd("[System] That user is not in your room.\n", host->fd, PACKET_TYPE_MESSAGE);
        return -1;
    }

    if (strcmp(target_room->host_name, host->name) != 0) {
        send_to_fd("[System] You are not the host of this room.\n", host->fd, PACKET_TYPE_MESSAGE);
        return -1;
    }

    if (change_host(target_room, target) == 0) {
        char msg[BUF_SIZE];
        snprintf(msg, sizeof(msg),
                 "[System] '%s' has transferred host privileges to '%s'.\n",
                 host->name, target->name);
        multicast(msg, host, PACKET_TYPE_MESSAGE);
        send_to_fd(msg, host->fd, PACKET_TYPE_MESSAGE);

        send_to_fd("[System] You are no longer the host.\n", host->fd, PACKET_TYPE_MESSAGE);
        send_to_fd("[System] You are now the host of this room.\n", target->fd, PACKET_TYPE_MESSAGE);
    } else {
        send_to_fd("[System] Failed to change host.\n", host->fd, PACKET_TYPE_MESSAGE);
    }

    return 0;
}

static int request_rename_room(Client *requester, const char *new_name) {
    if (new_name == NULL || strlen(new_name) == 0) {
        send_to_fd("[System] New room name cannot be empty.\n", requester->fd, PACKET_TYPE_MESSAGE);
        return -1;
    }

    Room *target_room = find_by_room_id(rooms_head, requester->chat_room);
    if (target_room == NULL) {
        send_to_fd("[System] You are not in a valid room.\n", requester->fd, PACKET_TYPE_MESSAGE);
        return -1;
    }

    if (target_room->id == DEFAULT_ROOM) {
        send_to_fd("[System] Default room name cannot be changed.\n", requester->fd, PACKET_TYPE_MESSAGE);
        return -1;
    }

    if (strcmp(target_room->host_name, requester->name) != 0) {
        send_to_fd("[System] Only the host can rename the room.\n", requester->fd, PACKET_TYPE_MESSAGE);
        return -1;
    }

    if (find_by_room_name(rooms_head, new_name) != NULL) {
        send_to_fd("[System] That room name is already taken.\n", requester->fd, PACKET_TYPE_MESSAGE);
        return -1;
    }

    char old_name[NAME_LEN];
    strncpy(old_name, target_room->name, NAME_LEN);
    old_name[NAME_LEN - 1] = '\0';

    rename_room(target_room, new_name);

    char msg[BUF_SIZE];
    snprintf(msg, sizeof(msg),
             "[System] Room name has been changed from '%s' to '%s' by host.\n",
             old_name, target_room->name);
    multicast(msg, requester, PACKET_TYPE_MESSAGE);
    send_to_fd(msg, requester->fd, PACKET_TYPE_MESSAGE);

    return 0;
}

void *client_thread(void *arg) {
    int len;
    int fd = *(int *)arg;
    Client *self = handle_client_registration(fd);
    free(arg);
    PacketHeader header;
    char buf[BUF_SIZE];

    while (recv_packet(fd, &header, buf, sizeof(buf)) == 0) {
        char msg[BUF_SIZE + NAME_LEN];
        size_t len = header.length;
        if (len >= BUF_SIZE) len = BUF_SIZE - 1;
            buf[len] = '\0';

        switch (header.type) {
            case PACKET_TYPE_MESSAGE : {
                snprintf(msg, sizeof(msg), "[%s] %.200s", self->name, buf);
                msg[sizeof(msg) - 1] = '\0';
                pthread_mutex_lock(&mutex);
                multicast(msg, self, PACKET_TYPE_MESSAGE);
                pthread_mutex_unlock(&mutex);
                break;   
            }
            case PACKET_TYPE_LIST_USERS : {
                const char *info = show_clients(0);
                send_to_fd(info, fd, PACKET_TYPE_MESSAGE);
                break;
            }
            case PACKET_TYPE_LIST_ROOMS : {
                const char *info = show_rooms(0);
                send_to_fd(info, fd, PACKET_TYPE_MESSAGE);
                break;
            }
            case PACKET_TYPE_DM : {
                char *name_str = strtok(buf, " ");
                char *msg_dm = strtok(NULL, "");
                if (msg_dm) {
                    char *newline = strchr(msg_dm, '\n');
                    if (newline) *newline = '\0';
                }
                snprintf(msg, sizeof msg, "[%s] dm(%s)\n", self->name, msg_dm);
                pthread_mutex_lock(&mutex);
                Client *target = find_by_client_name(clients_head, name_str);
                if (target && msg_dm) {
                    unicast(msg, target, PACKET_TYPE_MESSAGE);
                } else {
                    unicast("Usage: /dm <id> <message>\n", self, PACKET_TYPE_MESSAGE);
                }
                pthread_mutex_unlock(&mutex);
                break;
            }
            case PACKET_TYPE_CREATE_ROOM : {
                char *room_name = strtok(buf, " ");
                char *size_str = strtok(NULL, " ");
                if (!room_name || !size_str) {
                    send_to_fd("[System] Usage: /new <room_name> <room_size>\n", fd, PACKET_TYPE_MESSAGE);
                }
                else {
                    int room_size = atoi(size_str);
                    pthread_mutex_lock(&mutex);
                    Room *room_id = add_room(&rooms_head, room_name, self->fd, room_size);
                    pthread_mutex_unlock(&mutex);
                    if (room_id == NULL)
                        send_to_fd("[System] Failed to create a chat room.\n", fd, PACKET_TYPE_MESSAGE);
                    else {
                        send_to_fd("[System] Chat room created successfully.\n", fd, PACKET_TYPE_MESSAGE);
                        printf("[LOG] %s created room '%s' (size %d)\n", self->name, room_name, room_size);
                    }
                }
                break;
            }
            case PACKET_TYPE_JOIN_ROOM : {
                char *room_str = strtok(buf, " ");
                if (!room_str) {
                    send_to_fd("[System] Usage: /enter <room_idx>\n", fd, PACKET_TYPE_MESSAGE);
                }
                else {
                    int room_id = atoi(room_str);
                    pthread_mutex_lock(&mutex);
                    Room *target = find_by_room_id(rooms_head, room_id);
                    if (target == NULL) {
                        send_to_fd("[System] Invalid room ID.\n", fd, PACKET_TYPE_MESSAGE);
                    }
                    else if (join_room(self, target) == -1)
                        send_to_fd("[System] Failed to enter the chat room.\n", fd, PACKET_TYPE_MESSAGE);
                    else
                        send_to_fd("[System] You have entered the chat room.\n", fd, PACKET_TYPE_MESSAGE);
                    pthread_mutex_unlock(&mutex);
                }
                break;
            }
            case PACKET_TYPE_KICK_USER_IN_ROOM : {
                char *client_str = strtok(buf, " ");
                if (client_str) {
                    client_str[strcspn(client_str, "\r\n")] = '\0';
                }
                if (!client_str) {
                    send_to_fd("[System] Usage: /kick <user_name>\n", fd, PACKET_TYPE_MESSAGE);
                } else {
                    pthread_mutex_lock(&mutex);
                    Client *target_client = find_by_client_name(clients_head, client_str);
                    if (target_client == NULL) {
                        send_to_fd("[System] User not found.\n", fd, PACKET_TYPE_MESSAGE);
                    } else {
                        request_kick_client(self, target_client);
                    }
                    pthread_mutex_unlock(&mutex);
                }
                break;
            }
            case PACKET_TYPE_GRANT_HOST_ROOM : {
                char *client_str = strtok(buf, " ");
                if (client_str) {
                    client_str[strcspn(client_str, "\r\n")] = '\0';
                }
                if (!client_str) {
                    send_to_fd("[System] Usage: /grant <user_name>\n", fd, PACKET_TYPE_MESSAGE);
                } else {
                    pthread_mutex_lock(&mutex);
                    Client *target_client = find_by_client_name(clients_head, client_str);
                    if (target_client == NULL) {
                        send_to_fd("[System] User not found.\n", fd, PACKET_TYPE_MESSAGE);
                    } else {
                        request_change_host(self, target_client);
                    }
                    pthread_mutex_unlock(&mutex);
                }
                break;
            }
            case PACKET_TYPE_RENAME_ROOM : {
                char *client_str = strtok(buf, " ");
                if (client_str) {
                    client_str[strcspn(client_str, "\r\n")] = '\0';
                }
                if (!client_str) {
                    send_to_fd("[System] Usage: /rename <room_name>\n", fd, PACKET_TYPE_MESSAGE);
                } else {
                    pthread_mutex_lock(&mutex);
                    request_rename_room(self, client_str);
                    pthread_mutex_unlock(&mutex);
                }
                break;
            }
            case PACKET_TYPE_LEAVE_ROOM : {
                pthread_mutex_lock(&mutex);
                int result = leave_room(self);
                pthread_mutex_unlock(&mutex);
            
                if (result == 0) {
                    send_to_fd("[System] You left the room.\n", fd, PACKET_TYPE_MESSAGE);
                } else {
                    send_to_fd("[System] You are not in a room.\n", fd, PACKET_TYPE_MESSAGE);
                }
                break;
            }
            default : {
                send_to_fd("[System] Unknown packet type.\n", fd, PACKET_TYPE_ERROR);
                break;
            }
        }
    }

    pthread_mutex_lock(&mutex);
    if (self) {
        snprintf(buf, sizeof buf, "[System] %s left the system.\n", self->name);
        broadcast(buf, NULL, PACKET_TYPE_MESSAGE);
        printf("[LOG] %s disconnected\n", self->name);
        remove_client(&clients_head, self->name);
    } else {
        printf("[LOG] unknown client disconnected\n");
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}
