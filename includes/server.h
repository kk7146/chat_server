#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>

#define CHAT_PORT 9000
#define ADMIN_PORT 9001
#define MAX_CLIENTS 64
#define MAX_ROOM_CLIENTS 20
#define MAX_ROOMS 64
#define BUF_SIZE 256
#define NAME_LEN 32
#define ROOM_NAME_LEN 32
#define DEFAULT_ROOM 0

typedef struct {
    int fd;
    char name[NAME_LEN];
    pthread_t tid;
    int chat_room;
    int pending_request_from;
    Client *next;
    Client *prev;
} Client;

typedef struct {
    int id;
    char name[ROOM_NAME_LEN];
    Client *users;
    Room *next;
    Room *prev;
} Room;

extern Client *clients_head;
extern Room rooms[MAX_ROOMS];
extern pthread_mutex_t mutex;

char *show_users(int isAdmin);
char *show_rooms(int isAdmin);

Client* find_by_client_name(Client *head, char *name);
Client* find_by_client_fd(Client *head, int fd);
void free_all_clients(Client **head);
int add_client(Client **head, char *name, int fd, int chat_room_id, int pending_request_from);
void remove_client(Client **head, const char *name);


#endif