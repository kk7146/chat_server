#ifndef MESSAGE_H
#define MESSAGE_H

#include "server.h"

void broadcast(const char *msg, int exclude_fd);
void unicast(const char *msg, char dest_idx[NAME_LEN]);
void multicast(const char *msg, int exclude_fd);
int find_users_tid();

#endif