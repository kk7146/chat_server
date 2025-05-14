#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"

void *client_thread(void *arg);
int register_client(int fd, const char *name);

#endif