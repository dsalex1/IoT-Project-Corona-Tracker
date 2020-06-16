
#include "net/netstack.h"
#include <random.h>

uint8_t linkaddr_to_node_id(const linkaddr_t *linkaddr) {
  return linkaddr->u8[0];
}

void fix_randomness(const linkaddr_t *linkaddr) {
  random_init(linkaddr_to_node_id(linkaddr));
}
