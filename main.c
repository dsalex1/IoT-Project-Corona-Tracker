/**
 * \file
 *         Project Master
 * \author
 *         Alexander Seidler <dsalex1@ymail.com>
 *
 */

#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "dev/leds.h"

#include <string.h>
#include <stdio.h> /* For printf() */
#include <random.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Main"
#define LOG_LEVEL LOG_LEVEL_INFO

#include "helpers.h"

#define SEND_INTERVAL 5 * CLOCK_SECOND

/*---------------------------------------------------------------------------*/
PROCESS(main_process, "Main");
AUTOSTART_PROCESSES(&main_process);

struct command RPIStore[1024];
int RPIStoreIndex = 0;

uint8_t isInfected = 0;

void setInfected(){
    LOG_INFO("got infected\n");
    isInfected = 1;
    leds_set(LEDS_RED);
}

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
                const linkaddr_t *src, const linkaddr_t *dest)
{
    if (len != sizeof(struct command))
        return;
    static struct command cmd;
    memcpy(&cmd, data, sizeof(struct command));

    //store up to 1024 entries, then overwrite old records
    memcpy(&RPIStore[RPIStoreIndex++], &cmd, sizeof(struct command));
    RPIStoreIndex &= 1024;
   
    if (cmd.RPI_l == 1)
        if (random_rand() % 5 == 0)
            setInfected();
    LOG_INFO("Received RPI: 0x%04X, %04X\n", cmd.RPI_h, cmd.RPI_l);
}

PROCESS_THREAD(main_process, ev, data)
{
    PROCESS_BEGIN();

    //make node with id 1 infected from the beginning
    if ((&linkaddr_node_addr)->u8[0] == 1)
        setInfected();

    static struct etimer periodic_timer;
    fix_randomness(&linkaddr_node_addr);
    nullnet_set_input_callback(input_callback);

    //wait a random amount, to desync the nodes
    etimer_set(&periodic_timer, (random_rand() % (5*CLOCK_SECOND)));
    while (1)
    {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        static struct command cmd;
        cmd.RPI_h = random_rand();
        cmd.RPI_l = isInfected; //indicator for being infected, for now

        LOG_INFO("Sending RPI: 0x%04X, %04X\n", cmd.RPI_h, cmd.RPI_l);

        nullnet_buf = (uint8_t *)&cmd;
        nullnet_len = sizeof(cmd);
        NETSTACK_NETWORK.output(NULL);

        etimer_set(&periodic_timer, SEND_INTERVAL);
        etimer_reset(&periodic_timer);
    }
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
