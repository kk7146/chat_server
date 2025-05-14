#ifndef ROOM_H
#define ROOM_H

#include "server.h"

int create_room(const char *name);
int join_room(int client_idx, int room_id);
int leave_room(int client_idx);

#endif