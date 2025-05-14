#ifndef SOCKET_UTIL_H
#define SOCKET_UTIL_H

#include "server.h"

int create_server_socket(int port);
void *accept_loop(void *arg);

#endif