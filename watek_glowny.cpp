#include "main.h"
#include "watek_glowny.h"

void observatory()
{
    using namespace std::chrono_literals;

    auto start = std::chrono::steady_clock::now();
    while (true)
    {
        auto l = std::unique_lock<std::mutex>(mtx);
        incrementClock();
        debug("lamportClock: %d", lamportClock);

        int percent = randomValue(0, 100);
        debug("percent: %d", percent);
        if (percent <= ASTEROID_FOUND_PROB)
        {
            // println("Found asteroid");
            int amount = randomValue(MIN_ASTEROID_FOUND, MAX_ASTEROID_FOUND);
            debug("amount: %d", amount);

            packet_t *pkt = new packet_t;
            pkt->ts = lamportClock;
            pkt->data = amount;

            sendAllTelepaths(pkt, ASTEROID_FOUND);
            println("Sent messages about %d new asteroids", amount);
        }
        l.unlock();

        // int sleepTime = randomValue(OBSERVATORY_SLEEP_MIN, OBSERVATORY_SLEEP_MAX);
        // println("Sleeping for next %ds", sleepTime);

        std::this_thread::sleep_for(10ms);

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

        if (duration > 30)
        {
            println("Sleeping forever...");
            while (true)
            {
                std::this_thread::sleep_for(std::chrono::hours(24));
            }
        }
    }
}

void telepath()
{
    auto start = std::chrono::steady_clock::now();

    while (true)
    {
        auto l = std::unique_lock<std::mutex>(mtx);

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

        if (duration > 1)
        {
            println("AsteroidCount: %d", asteroidCount);
        }

        int currentState = getState();
        switch (currentState)
        {
        case REST:
        {
            if (!justStarted)
            {
                println("I'm exhausted, going to sleep before further work...");
                int sleepTime = randomValue(TELEPATH_SLEEP_MIN, TELEPATH_SLEEP_MAX);
                // debug("sleepTime: %d", sleepTime);
                // sleep(sleepTime);
            }
            justStarted = false;

            changeState(WAIT_PAIR);

            break;
        }
        case WAIT_PAIR:
        {
            enterPairQueue();

            // waitForStateChange(currentState, l);
            println("Czekam na zmianę stanu z obecnego WAIT_PAIR");
            cv.wait(l, [currentState]()
                    { return getState() != currentState; });
            break;
        }
        case PAIRED:
        {
            // waitForStateChange(currentState, l);
            println("Czekam na zmianę stanu z obecnego PAIRED");
            cv.wait(l, [currentState]()
                    { return getState() != currentState; });
            break;
        }
        case WAIT_ASTEROID:
        {
            enterAsteroidQueue();

            // waitForStateChange(currentState, l);
            println("Czekam na zmianę stanu z obecnego WAIT_ASTEROID");
            cv.wait(l, [currentState]()
                    { return getState() != currentState; });
            break;
        }
        default:
            break;
        }

        l.unlock();
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
