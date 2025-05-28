#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>

#define CHAT_PORT 9000
#define ADMIN_PORT 9001
#define MAX_CLIENTS 64
#define MAX_ROOMS 64
#define BUF_SIZE 256
#define NAME_LEN 32
#define ROOM_NAME_LEN 32
#define ROOM_CLIENTS_DEFAULT_SIZE 20 // 방 사이즈 기본값.
#define ROOM_CLIENTS_MAX_SIZE 50 // 설정할 수 있는 방의 크기 MAX
#define DEFAULT_ROOM 0

typedef struct Client Client;
typedef struct Room Room;

struct Client {
    int fd;
    char name[NAME_LEN];
    pthread_t tid;
    int chat_room;
    Client *pending_request_from;
    Client *next;
    Client *prev;
};

struct Room {
    int id;
    char name[ROOM_NAME_LEN];
    int max_clients;
    int clients_num;
    char host_name[NAME_LEN];
    Room *next;
    Room *prev;
};

extern Client *clients_head;
extern Room *rooms_head;
extern pthread_mutex_t mutex;

char *show_clients(int isAdmin);
char *show_rooms(int isAdmin);

Client* find_by_client_name(Client *head, const char *name);
Client* find_by_client_fd(Client *head, int fd);
int count_client(Client *head);
void free_all_clients(Client **head);
Client *add_client(Client **head, const char *name, int fd, int chat_room_id, Client* pending_request_from);
int remove_client(Client **head, const char *name);

char *show_rooms(int isAdmin);
Room* find_by_room_name(Room *head, const char *name);
Room* find_by_room_id(Room *head, int id);
int count_room(Room *head);
Room* add_room(Room **head, const char *name, int fd, int room_size);
int join_room(Client *target_client, Room *target_room);
int leave_room(Client *target_client);
void free_all_rooms(Room **head);
void remove_room_kick_all_clients(Room *target); // 방 삭제랑 동일한 의미
int change_host(Room *target_Room, Client *target_client);

#endif