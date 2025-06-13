#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "server.h"

Client *clients_head = NULL;
Room *rooms_head = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


char *show_clients(int isAdmin) {
    static char list[BUF_SIZE * 4];
    size_t used = 0;
    size_t remain = sizeof(list);

    pthread_mutex_lock(&mutex);
    snprintf(list, remain, "============== Connected Users ==============\n");
    used = strlen(list);

    Client *current = clients_head;
    while (current != NULL) {
        char line[256];
        if (isAdmin) {
            snprintf(line, sizeof(line),
                     "ID:%s\tTID:%lu\tRoom:%d\n",
                     current->name,
                     (unsigned long)current->tid,
                     current->chat_room);
        } else {
            snprintf(line, sizeof(line),
                     "ID:%s\n",
                     current->name);
        }

        size_t line_len = strlen(line);
        if (used + line_len < sizeof(list)) {
            memcpy(list + used, line, line_len);
            used += line_len;
            list[used] = '\0';
        } else { // list 공간 부족 시
            snprintf(list + used, sizeof(list) - used,
                     "[... some users not shown due to size limit ...]\n");
            break;
        }

        current = current->next;
    }
    const char *footer = "=============================================\n";
    if (used + strlen(footer) < sizeof(list)) {
        strncat(list, footer, sizeof(list) - strlen(list) - 1);
    }

    pthread_mutex_unlock(&mutex);
    return list;
}

Client* find_by_client_name(Client *head, const char *name) {
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

int count_client(Client *head) {
    int counter = 0;
    Client *current = head;

    while (current != NULL) {
        counter++;
        current = current->next;
    }
    return counter;
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

Client *add_client(Client **head, const char *name, int fd, int chat_room_id, Client* pending_request_from) {
    Client *new_node = (Client *)malloc(sizeof(Client));
    if (new_node == NULL) {
        send(fd, "[System] Internal error\n", 25, 0);
        return NULL;
    }
    new_node->next = NULL;
    new_node->prev = NULL;
    
    // MAX_CLIENTS보다 크면 안됨
    if (count_client(*head) >= MAX_CLIENTS) {
        send(fd, "[System] Too many users. Try later.\n", 35, 0);
        free(new_node);
        return NULL;
    }

    // 똑같은 이름의 유저의 경우
    if (find_by_client_name(*head, name) != NULL) {
        send(fd, "[System] That username is already taken.\n", 39, 0);
        free(new_node);
        return NULL;
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
        return new_node;
    }

    // 맨 끝 노드 찾이서 넣기
    Client *tail = *head;
    while (tail->next != NULL) {
        tail = tail->next;
    }
    tail->next = new_node;
    new_node->prev = tail;
    return new_node;
}

int remove_client(Client **head, const char *name) {
    Client *remove_node = find_by_client_name(*head, name);

    if (remove_node == NULL)
        return -1;
    if (remove_node->prev != NULL)
        remove_node->prev->next = remove_node->next;
    else
        *head = remove_node->next;
    if (remove_node->next != NULL)
        remove_node->next->prev = remove_node->prev;
    free(remove_node);
    return 0;
}


char *show_rooms(int isAdmin) {
    static char list[BUF_SIZE * 4];
    size_t used = 0;
    size_t remain = sizeof(list);

    pthread_mutex_lock(&mutex);
    snprintf(list, remain, "============== Available Rooms ==============\n");
    used = strlen(list);

    Room *current = rooms_head;
    while (current != NULL) {
        char line[256];
        if (isAdmin) {
            snprintf(line, sizeof(line),
                     "ID:%d\tName:%s\tHost:%s\t[%d/%d]\n",
                     current->id,
                     current->name,
                     current->host_name,
                     current->clients_num,
                     current->max_clients);
        } else {
            snprintf(line, sizeof(line),
                     "ID:%d\tName:%s\t[%d/%d]\n",
                     current->id,
                     current->name,
                     current->clients_num,
                     current->max_clients);
        }

        size_t line_len = strlen(line);
        if (used + line_len < sizeof(list)) {
            memcpy(list + used, line, line_len);
            used += line_len;
            list[used] = '\0';
        } else {
            // list 공간 부족 시에 나오는 메세지
            snprintf(list + used, sizeof(list) - used,
                     "[... some users not shown due to size limit ...]\n");
            break;
        }

        current = current->next;
    }
    const char *footer = "=============================================\n";
    if (used + strlen(footer) < sizeof(list)) {
        strncat(list, footer, sizeof(list) - strlen(list) - 1);
    }

    pthread_mutex_unlock(&mutex);
    return list;
}

Room* find_by_room_name(Room *head, const char *name) {
    Room *current = head;

    if (name == NULL) return NULL;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

Room* find_by_room_id(Room *head, int id) {
    Room *current = head;

    while (current != NULL) {
        if (current->id == id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

int count_room(Room *head) {
    int counter = 0;
    Room *current = head;

    while (current != NULL) {
        counter++;
        current = current->next;
    }
    return counter;
}

static int find_smallest_room_id(Room *head) {
    int next_id = 0;
    Room *current = head;
    while (current && current->id == next_id) {
        next_id++;
        current = current->next;
    }
    return next_id;
}

Room *add_room(Room **head, const char *name, int fd, int room_size) {
    if (room_size <= 0 || room_size > ROOM_CLIENTS_MAX_SIZE)
        room_size = ROOM_CLIENTS_DEFAULT_SIZE;
    Room *new_node = (Room *)malloc(sizeof(Room));
    new_node->next = NULL;
    new_node->prev = NULL;
    if (new_node == NULL) {
        if (fd != -1)
            send(fd, "[System] Internal error\n", 25, 0);
        return NULL;
    }
    
    // MAX_ROOMS보다 크면 안됨
    if (count_room(*head) >= MAX_ROOMS) {
        if (fd != -1)
            send(fd, "[System] Too many rooms. Try later.\n", 35, 0);
        free(new_node);
        return NULL;
    }

    // 똑같은 이름의 룸이 있는 경우
    if (find_by_room_name(*head, name) != NULL) {
        if (fd != -1)
            send(fd, "[System] That room name is already taken.\n", 39, 0);
        free(new_node);
        return NULL;
    }

    // 값 넣기
    strncpy(new_node->name, name, NAME_LEN - 1);
    new_node->name[NAME_LEN - 1] = '\0';
    new_node->host_name[0] = '\0';
    new_node->id = find_smallest_room_id(*head);
    new_node->max_clients = room_size;
    new_node->clients_num = 0;

    // 삽입 위치 결정
    if (*head == NULL || (*head)->id > new_node->id) {
        new_node->next = *head;
        new_node->prev = NULL;
        if (*head) 
            (*head)->prev = new_node;
        *head = new_node;
    } else {
        Room *curr = *head;
        while (curr->next && curr->next->id < new_node->id)
            curr = curr->next;
        new_node->next = curr->next;
        if (curr->next)
            curr->next->prev = new_node;
        curr->next = new_node;
        new_node->prev = curr;
    }
    return new_node;
}

int join_room(Client *target_client, Room *target_room) {
    if (target_room == NULL)
        return -1;
    if (target_room->max_clients <= target_room->clients_num)
        return -1;
    if (target_room->id != DEFAULT_ROOM && target_room->host_name[0] == '\0') {
        change_host(target_room, target_client);
    }
    target_client->chat_room = target_room->id;
    target_room->clients_num++;
    return (0);
}

static Client *first_client(Room *target) {
    Client *current = clients_head;

    while (current != NULL) {
        if (strcmp(target->host_name, current->name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

int change_host(Room *target_room, Client *target_client) {
    strncpy(target_room->host_name, target_client->name, NAME_LEN - 1);
    target_room->host_name[NAME_LEN - 1] = '\0';
    return 0;
}

int rename_room(Room *target_room, const char *name) {
    if (!target_room || !name || strlen(name) == 0) {
        return -1;
    }
    if (strncmp(target_room->name, name, NAME_LEN) == 0) {
        return 0;
    }
    strncpy(target_room->name, name, NAME_LEN - 1);
    target_room->name[NAME_LEN - 1] = '\0';
    return 0;
}

// 단순히 방을 지우는 함수. 외부에서 호출되면 안됨. 방에 있는 유저 고려는 안 했음.
static void remove_room(Room **head, int id) {
    Room *remove_node = find_by_room_id(*head, id);

    if (remove_node == NULL)
        return ;
    if (remove_node->prev != NULL)
        remove_node->prev->next = remove_node->next;
    else
        *head = remove_node->next;
    if (remove_node->next != NULL)
        remove_node->next->prev = remove_node->prev;
    
    free(remove_node);
    return ;
}

int leave_room(Client *target_client) {
    Room *target_room = find_by_room_id(rooms_head, target_client->chat_room);
    if (target_room == NULL)
        return -1;
    target_client->chat_room = DEFAULT_ROOM;
    if (target_room->clients_num > 0) // 아래에서 0 이하면 삭제해서 빼도 괜찮긴 한데. 언더로 플로우 나는 경우. 일단 방어함.
        target_room->clients_num--;
    if (strcmp(target_room->host_name, target_client->name) == 0)
        change_host(first_client(target_room), target_room); // 이 부분 살짝 불안정한가?
    if (target_room->id != DEFAULT_ROOM &&target_room->clients_num <= 0)
        remove_room(&rooms_head, target_room->id);
    return (0);
}

// 방을 삭제할 때 호출. 유저 관리까지 포함.
void remove_room_kick_all_clients(Room *target) {
    Client *current = clients_head;
    Client *next; // 방이 삭제될 수 있기 때문에 미리 next 저장하게 함.

    while (current != NULL) {
        next = current->next;  // 미리 next 저장
        if (current->chat_room == target->id) {
            leave_room(current); // leave_room 내부에 remove_room 호출하는게 있음.
        }
        current = next;
    }
}

void free_all_rooms(Room **head) {
    Room *current = *head;
    while (current != NULL) {
        Room *next = current->next;
        free(current);
        current = next;
    }
    *head = NULL;
}
