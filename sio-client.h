#ifndef CHATNET_SIOCLIENT_H_
#define CHATNET_SIOCLIENT_H_

#include <libgen.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ipc.h"
#include "str.h"
#include "util.h"

void sioclientinit(char* execname);

void sioclientcleanup();

#endif // ends CHATNET_SIOCLIENT_H_
