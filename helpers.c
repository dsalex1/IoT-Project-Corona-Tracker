
#include "net/netstack.h"
#include <random.h>

uint8_t linkaddr_to_node_id(const linkaddr_t *linkaddr) {
  return linkaddr->u8[0];
}

void fix_randomness(const linkaddr_t *linkaddr) {
  random_init(linkaddr_to_node_id(linkaddr));
}

/**
 * found at: https://stackoverflow.com/a/39052987/7421438, by radhoo
 * hex2int
 * take a hex string and convert it to a 32bit number (max 8 hex digits)
 */
uint32_t hex2int(char *hex)
{
  uint32_t val = 0;
  while (*hex)
  {
    // get current character then increment
    uint8_t byte = *hex++;
    // transform hex character to the 4bit equivalent number, using the ascii table indexes
    if (byte >= '0' && byte <= '9')
      byte = byte - '0';
    else if (byte >= 'a' && byte <= 'f')
      byte = byte - 'a' + 10;
    else if (byte >= 'A' && byte <= 'F')
      byte = byte - 'A' + 10;
    // shift 4 to make space for new digit, and add the 4 bits of the new digit
    val = (val << 4) | (byte & 0xF);
  }
  return val;
}