#ifndef HELPERS_H
#define HELPERS_H

#include "net/netstack.h"
#include "sys/log.h"
#include "command.h"

uint8_t linkaddr_to_node_id(const linkaddr_t *linkaddr);

void fix_randomness(const linkaddr_t *linkaddr);

uint32_t hex2int(char *hex);

#endif
