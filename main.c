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
#include "dev/serial-line.h"

#include <string.h>
#include <stdio.h> /* For printf() */
#include <random.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Main"
#define LOG_LEVEL LOG_LEVEL_INFO

#include "helpers.h"

#define SEND_INTERVAL 5 * CLOCK_SECOND
#define SECONDS_BEFORE_TESTED 60
#define SECONDS_BEFORE_CURED 90

#define INITIAL_INFECTED ((&linkaddr_node_addr)->u8[0] <= 2)
#define INFECTION_RATE (random_rand() % 20 == 0)

#define GREEN_LED 0x01
#define BLUE_LED 0x02
#define RED_LED 0x04

/*---------------------------------------------------------------------------*/
PROCESS(main_process, "Main");
AUTOSTART_PROCESSES(&main_process);

struct command DKStore[1024] = {0};
int DKStoreIndex = 0;

struct command RPIStore[1024] = {0};
int RPIStoreIndex = 0;

struct command TEKStore[20] = {0};
int TEKStoreIndex = 0;

uint8_t isInfected = 0;
uint8_t isTestedPositive = 0;
uint8_t isQuarantined = 0;

struct ctimer quarantineTimer;

void setQuarantined()
{
    if (isQuarantined == 1)
        return;
    LOG_WARN("quarantined\n");
    isQuarantined = 1;
    leds_on(BLUE_LED);
} 
void setTestedPositive()
{
    if (isTestedPositive == 1)
        return;
    LOG_WARN("tested positive\n");
    isTestedPositive = 1;
    leds_on(GREEN_LED);
    setQuarantined();
}
void setCured()
{
    if (isInfected == -1)
        return;
    LOG_WARN("cured\n");
    isInfected = -1;
    leds_off(RED_LED);

    // being cured while not having had symtoms is equivalent to going to quarantine
    setQuarantined();
}
void setInfected()
{
    if (isInfected == 1)
        return;
    LOG_WARN("got infected\n");
    isInfected = 1;
    leds_on(RED_LED);

    static struct ctimer symptomsTimer;
    static struct ctimer cureTimer;
    ctimer_set(&symptomsTimer, CLOCK_SECOND * SECONDS_BEFORE_TESTED, &setTestedPositive, NULL);
    ctimer_set(&cureTimer, CLOCK_SECOND * SECONDS_BEFORE_CURED, &setCured, NULL);
}

void calculateMatches(char* data){ 
    int matches = 0;

    for (int i = 0; data[i] != 0;i+=4){
        char key[5];
        memcpy(key, &data[i], 4); 
        key[4] = '\0';
        DKStore[DKStoreIndex++].RPI_h = (uint16_t)hex2int(key);
    }
    LOG_INFO("now at %d\n", DKStoreIndex);
    //TODO: for each r in RPIStore check match and matches++

    if (matches > 2)
    {
        LOG_WARN("exposure warning\n");
        setQuarantined();
    }
}

/*---------------------------------------------------------------------------*/
void
input_callback(const void *data, uint16_t len,
                const linkaddr_t *src, const linkaddr_t *dest)
{
    if (len != sizeof(struct command))
        return;
    static struct command cmd;
    memcpy(&cmd, data, sizeof(struct command));

    //store up to 1024 entries, then overwrite old records
    memcpy(&RPIStore[RPIStoreIndex++], &cmd, sizeof(struct command));
    if (RPIStoreIndex >= 1024)
        RPIStoreIndex = 0;

    if (cmd.RPI_l == 1)
        if (INFECTION_RATE)
            setInfected();
    LOG_INFO("Received RPI: 0x%04X, %04X\n", cmd.RPI_h, cmd.RPI_l);
}

PROCESS_THREAD(main_process, ev, data)
{
    PROCESS_BEGIN();

    //make node with id 1 infected from the beginning
    if (INITIAL_INFECTED){
        setInfected();
    }

    static struct etimer periodic_timer;
    fix_randomness(&linkaddr_node_addr);
    nullnet_set_input_callback(input_callback);

    //init reading from serial
    serial_line_init();

    //wait a random amount, to desync the nodes
    etimer_set(&periodic_timer, (random_rand() % (5 * CLOCK_SECOND)));
    while (1)
    {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        static struct command cmd;
        cmd.RPI_h = random_rand();
        cmd.RPI_l = isInfected; //indicator for being infected, for now, FIXME:

        static uint8_t sentKeys = 0;
        if (isTestedPositive == 1 && sentKeys == 0) 
        { //send disgnosis keys
            sentKeys = 1;
            printf("POST /diagnosis-keys\t\t");
            for (int i = 0; i < 20;i++)
                if (TEKStore[i].RPI_h != 0 || TEKStore[i].RPI_l != 0)
                    printf("%04X,", TEKStore[i].RPI_h);
            printf("\n");
            PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message && data != NULL);
        }
        if (isQuarantined == 0){
            //updateDiagnosisKeys 
            printf("GET /diagnosis-keys?index=%d\n",DKStoreIndex);
            PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message && data != NULL);
            LOG_INFO("received line: %s\n", (char *)data);
            calculateMatches((char *)data);
        }
        if (isQuarantined == 0)
        {
            LOG_INFO("Sending RPI: 0x%04X, %04X\n", cmd.RPI_h, cmd.RPI_l);
            nullnet_buf = (uint8_t *)&cmd;
            nullnet_len = sizeof(cmd);
            NETSTACK_NETWORK.output(NULL);

            //store last 20 TEK
            memcpy(&TEKStore[TEKStoreIndex++], &cmd, sizeof(struct command));
            if (TEKStoreIndex >= 20)
                TEKStoreIndex = 0;
        }
        etimer_set(&periodic_timer, SEND_INTERVAL);
        etimer_reset(&periodic_timer);
        }
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
