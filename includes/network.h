#ifndef NETWORK_H
#define NETWORK_H

#include "server.h"

int create_server_socket(int port);
void *accept_loop(void *arg);

#endif