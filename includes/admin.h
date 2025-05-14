#ifndef ADMIN_H
#define ADMIN_H

#include "server.h"

void *admin_thread(void *arg);
void *admin_loop(void *arg);

#endif