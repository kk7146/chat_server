#ifndef MESSAGE_H
#define MESSAGE_H

#include "server.h"

void broadcast(const char *msg, Client* self);
void unicast(const char *msg, Client* target);
void multicast(const char *msg, Client* self);

#endif