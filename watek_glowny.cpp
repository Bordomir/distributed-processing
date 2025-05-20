#include "main.h"
#include "watek_glowny.h"

void observatory()
{
    srandom(rank);
    int tag;
    while (TRUE)
    {
        incrementClock();
        debug("lamportClock: %d", lamportClock)

        int percent = randomValue(0, 100);
        debug("percent: %d", percent)
        if (percent <= ASTEROID_FOUND_PROB) {
            println("Found asteroid")
            int amount = randomValue(MIN_ASTEROID_FOUND, MAX_ASTEROID_FOUND);
            debug("amount: %d", amount)

            packet_t *pkt;
            pkt->ts = lamportClock;
            pkt->data = amount;
            
            sendAllTelepaths(pkt, ASTEROID_FOUND);
            println("Sent messages about %d new asteroids", amount)
        }
        int sleepTime = randomValue(OBSERVATORY_SLEEP_MIN, OBSERVATORY_SLEEP_MAX);
        debug("sleepTime: %d", sleepTime)
        println("Sleeping")
        sleep(sleepTime);
    }
}

void telepath()
{
    srandom(rank);
    int tag;
    while (TRUE)
    {
        state_t currentState = getState();
        switch (currentState)
        {
            case REST:
                println("Sleeping")
                int sleepTime = randomValue(TELEPATH_SLEEP_MIN, TELEPATH_SLEEP_MAX);
                debug("sleepTime: %d", sleepTime)
                sleep(sleepTime);
                
                // To eliminate critical section messages has to be sent after state change
                pthread_mutex_lock( &stateMut );

                println("Trying to pair with other telepath")

                incrementClock();
                debug("lamportClock: %d", lamportClock)

                pairAckCount = 0;

                packet_t *pkt;
                pkt->ts = lamportClock;

                sendAllTelepaths(pkt, PAIR_REQ);
                println("Sent messages PAIR_REQ to all telepaths")
                changeState(WAIT_PAIR);
                println("Changed state to WAIT_PAIR")

                pthread_mutex_unlock( &stateMut );

                break;
            case WAIT_PAIR:
                pthread_cond_wait(&cond, &condMut);
                println("Paired with other telepath")
                break;
            case PAIRED:
                pthread_cond_wait(&cond, &condMut);
                println("No longer paired with other telepath")
                break;
            case WAIT_ASTEROID:
                println("Trying to destroy asteroid")

                incrementClock();
                debug("lamportClock: %d", lamportClock)

                asteroidAckCount = 0;

                packet_t *pkt;
                pkt->ts = lamportClock;

                sendAllTelepaths(pkt, ASTEROID_REQ);
                println("Sent messages ASTEROID_REQ to all telepaths")
                
                pthread_cond_wait(&cond, &condMut);
                println("No longer paired with other telepath")
                break;
            default:
                break;
        }
    }
}

void mainLoop()
{
    switch (rank)
    {
        case ROOT:
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
