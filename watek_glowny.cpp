#include "main.h"
#include "watek_glowny.h"

void observatory()
{
    int tag;
    while (true)
    {
        incrementClock();
        debug("lamportClock: %d", lamportClock);

        int percent = randomValue(0, 100);
        debug("percent: %d", percent);
        if (percent <= ASTEROID_FOUND_PROB)
        {
            println("Found asteroid") int amount = randomValue(MIN_ASTEROID_FOUND, MAX_ASTEROID_FOUND);
            debug("amount: %d", amount);

            packet_t *pkt;
            pkt->ts = lamportClock;
            pkt->data = amount;

            sendAllTelepaths(pkt, ASTEROID_FOUND);
            println("Sent messages about %d new asteroids", amount);
        }
        int sleepTime = randomValue(OBSERVATORY_SLEEP_MIN, OBSERVATORY_SLEEP_MAX);
        println("Sleeping for next %ds", sleepTime);
        sleep(sleepTime);
    }
}

void telepath()
{
    int tag;
    while (true)
    {
        int currentState = getState();
        switch (currentState)
        {
        case REST:
        {
            println("Sleeping") int sleepTime = randomValue(TELEPATH_SLEEP_MIN, TELEPATH_SLEEP_MAX);
            debug("sleepTime: %d", sleepTime);
            sleep(sleepTime);

            changeState(WAIT_PAIR);

            break;
        }
        case WAIT_PAIR:
        {
            enterPairQueue(); // Niezgodne ze sprawozdaniem

            waitForStateChange(currentState);
            break;
        }
        case PAIRED:
        {
            waitForStateChange(currentState);
            break;
        }
        case WAIT_ASTEROID:
        {
            enterAsteroidQueue();

            waitForStateChange(currentState);
            break;
        }
        default:
            break;
        }
    }
}

void mainLoop()
{
    switch (rank)
    {
    case OBSERVATORY:
        observatory();
        break;
    default:
        telepath();
        break;
    }

    /*
        srandom(rank);
        int tag;

        while (stan != InFinish) {
            int perc = random()%100;

            if (perc<STATE_CHANGE_PROB) {
                if (stan==InRun) {
                    debug("Zmieniam stan na wysyłanie");
                    changeState( InSend );
                    packet_t *pkt = malloc(sizeof(packet_t));
                    pkt->data = perc;
                    perc = random()%100;
                    tag = ( perc < 25 ) ? FINISH : APP_PKT;
                    debug("Perc: %d", perc);

                    sendPacket( pkt, (rank+1)%size, tag);
                    changeState( InRun );
                    debug("Skończyłem wysyłać");
                } else {
                }
            }
            sleep(SEC_IN_STATE);
        }
    */
}
