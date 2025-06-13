#ifndef MESSAGE_H
#define MESSAGE_H

#include "server.h"

void broadcast(const char *msg, Client* self, PacketType type);
void unicast(const char *msg, Client* target, PacketType type);
void multicast(const char *msg, Client* self, PacketType type);
void send_to_fd(const char *msg, int fd, PacketType type);

#endif