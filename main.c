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
#define INFECTION_RATE (random_rand() % 15 == 0)

#define RPI_PER_TEK 4
#define DK_STORED 128
#define RPI_STORED 1024
#define TEK_STORED 3

#define IGNORE_EXPOSURE_WARNING 0

#define GREEN_LED 0x01
#define BLUE_LED 0x02
#define RED_LED 0x04

/*---------------------------------------------------------------------------*/
PROCESS(main_process, "Main");
AUTOSTART_PROCESSES(&main_process);

uint16_t DKStore[DK_STORED] = {0};
int DKStoreIndex = 0;

struct command RPIStore[RPI_STORED] = {0};
int RPIStoreIndex = 0;

uint16_t TEKStore[TEK_STORED] = {0};
int TEKStoreIndex = 0;

uint8_t isInfected = 0;
uint8_t isTestedPositive = 0;
uint8_t isQuarantined = 0;

struct ctimer quarantineTimer;

uint16_t generateRPI_h(uint16_t TEK, uint8_t index)
{
    return TEK;
}
uint16_t generateRPI_l(uint16_t TEK, uint8_t index)
{
    return index + 1;
}

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
    if (isQuarantined == 0)
        leds_on(GREEN_LED);
    setQuarantined();
}
void setCured()
{
    if (isInfected == -1)
        return;
    LOG_WARN("cured\n");
    isInfected = -1;
    //leds_off(RED_LED);

    // being cured while not having had symtoms is equivalent to going to quarantine
    // setQuarantined();
}
void setInfected()
{
    if (isInfected != 0)
        return;
    LOG_WARN("got infected\n");
    isInfected = 1;
    leds_on(RED_LED);

    static struct ctimer symptomsTimer;
    static struct ctimer cureTimer;
    ctimer_set(&symptomsTimer, CLOCK_SECOND * SECONDS_BEFORE_TESTED, &setTestedPositive, NULL);
    ctimer_set(&cureTimer, CLOCK_SECOND * SECONDS_BEFORE_CURED, &setCured, NULL);
}

void calculateMatches(char *data)
{
    int matches = 0;

    for (int i = 0; data[i] != 0; i += 4)
    {
        char key[5];
        memcpy(key, &data[i], 4);
        key[4] = '\0';
        DKStore[DKStoreIndex++] = (uint16_t)hex2int(key);
    }
    LOG_INFO("now at %d\n", DKStoreIndex);

    for (int j = 0; j < RPI_STORED; j++)
    {
        if (RPIStore[j].RPI_h == 0)
            continue;
        for (int k = 0; k < DK_STORED; k++)
        {
            if (DKStore[k] == 0)
                continue;
            for (int i = 0; i < RPI_PER_TEK; i++)
            {
                if (RPIStore[j].RPI_h == generateRPI_h(DKStore[k], i) &&
                    RPIStore[j].RPI_l == generateRPI_l(DKStore[k], i))
                    matches++;
            }
        }
    }

    if (matches > 2)
    {
        #if IGNORE_EXPOSURE_WARNING == 1
            LOG_WARN("exposure warning ignored\n");
        #else
            LOG_WARN("exposure warning\n");
            setQuarantined();
        #endif
    }
}

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
                    const linkaddr_t *src, const linkaddr_t *dest)
{
    if (len != sizeof(struct command))
        return;
    static struct command cmd;
    memcpy(&cmd, data, sizeof(struct command));



    //read infection bit
    if ((cmd.RPI_l & 0x8000) == 0x8000)
        if (INFECTION_RATE) 
            setInfected();
    LOG_INFO("Received RPI: 0x%04X, %04X\n", cmd.RPI_h, cmd.RPI_l);

    cmd.RPI_l &= 0x7FFF; //clear infection data bit for storage
    

    //store up to 1024 entries, then overwrite old records
    memcpy(&RPIStore[RPIStoreIndex++], &cmd, sizeof(struct command));
    if (RPIStoreIndex >= 1024)
        RPIStoreIndex = 0;
}

PROCESS_THREAD(main_process, ev, data)
{
    PROCESS_BEGIN();

    //make node with id 1 infected from the beginning
    if (INITIAL_INFECTED)
    {
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
        static uint8_t RPIIndex = RPI_PER_TEK;
        static uint16_t TEK;

        if (RPIIndex == RPI_PER_TEK)
        {
            TEK = random_rand();
            RPIIndex = 0;

            //store last [TEK_STORED] TEKs
            TEKStore[TEKStoreIndex++] = TEK;
            if (TEKStoreIndex >= TEK_STORED)
                TEKStoreIndex = 0;
        }
        cmd.RPI_h = generateRPI_h(TEK, RPIIndex);
        cmd.RPI_l = generateRPI_l(TEK, RPIIndex) | isInfected * 0x8000; //indicator for being infected, to identify infections for simulation
        RPIIndex++;

        static uint8_t sentKeys = 0;
        if (isTestedPositive == 1 && sentKeys == 0)
        { //send disgnosis keys
            sentKeys = 1;
            printf("POST /diagnosis-keys\t\t");
            for (int i = 0; i < TEK_STORED; i++)
                if (TEKStore[i] != 0 )
                    printf("%04X,", TEKStore[i]);
            printf("\n");
            PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message && data != NULL);
        }
        if (isQuarantined == 0)
        {
            //updateDiagnosisKeys
            printf("GET /diagnosis-keys?index=%d\n", DKStoreIndex);
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
        }
        etimer_set(&periodic_timer, SEND_INTERVAL);
        etimer_reset(&periodic_timer);
    }
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
