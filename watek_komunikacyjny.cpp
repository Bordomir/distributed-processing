#include "main.h"
#include "watek_komunikacyjny.h"

void observatoryKom()
{
    MPI_Status status;
    MPI_Request request;
    packet_t pakiet;
    while (true)
    {
        // println("Waiting for mesage");

        MPI_Recv(&pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        auto l = std::unique_lock<std::mutex>(mtx);

        switch (status.MPI_TAG)
        {
        default:
            updateClock(pakiet.ts);
            incrementClock();
            println("Received message %s from %d", tag2string(status.MPI_TAG), status.MPI_SOURCE);
            break;
        }
        l.unlock();
    }
}

void manageMessageREST(packet_t pakiet, MPI_Status status)
{
    switch (status.MPI_TAG)
    {
    case PAIR_REQ:
    {
        pairQueue.push(std::make_pair(pakiet.ts, status.MPI_SOURCE));

        pairACK(status.MPI_SOURCE);

        break;
    }
    case PAIR_RELEASE:
    {
        pairQueue.pop();
        pairQueue.pop();
        // println("Removed 2 processes from the pair queue");

        break;
    }
    case ASTEROID_REQ:
    {
        asteroidQueue.push(std::make_pair(pakiet.ts, status.MPI_SOURCE));

        asteroidACK(status.MPI_SOURCE);

        break;
    }
    case ASTEROID_RELEASE:
    {
        asteroidCount--;
        asteroidQueue.pop();
        asteroidQueue.pop();
        // println("Destroyed 1 asteroid and removed 1 process from the asteroid queue");

        break;
    }
    case ASTEROID_FOUND:
    {
        asteroidCount += pakiet.data;
        // println("Observatory found %d new asteroids", pakiet.data);

        break;
    }
    default:
    {
        debug("Received message %d from %d", status.MPI_TAG, status.MPI_SOURCE);
        break;
    }
    }
}

void manageMessageWAIT_PAIR(packet_t pakiet, MPI_Status status)
{
    switch (status.MPI_TAG)
    {
    case PAIR_REQ:
    {
        pairQueue.push(std::make_pair(pakiet.ts, status.MPI_SOURCE));

        pairACK(status.MPI_SOURCE);

        tryToSendPairProposal();

        tryToPair();

        break;
    }
    case PAIR_RELEASE:
    {
        pairQueue.pop();
        pairQueue.pop();
        // println("Removed 2 processes from the pair queue");

        tryToSendPairProposal();

        tryToPair();

        if (pair == status.MPI_SOURCE)
        {
            changeState(PAIRED);
        }

        break;
    }
    case PAIR_ACK:
    {
        tryToSendPairProposal();

        tryToPair();

        break;
    }
    case PAIR_PROPOSAL:
    {

        if (pair == -1)
        {
            pair = status.MPI_SOURCE;
        }

        tryToPair();

        break;
    }
    case ASTEROID_REQ:
    {
        asteroidQueue.push(std::make_pair(pakiet.ts, status.MPI_SOURCE));

        asteroidACK(status.MPI_SOURCE);

        break;
    }
    case ASTEROID_RELEASE:
    {
        asteroidCount--;
        asteroidQueue.pop();
        asteroidQueue.pop();
        // println("Destroyed 1 asteroid and removed 1 process from the asteroid queue");

        break;
    }
    case ASTEROID_FOUND:
    {
        asteroidCount += pakiet.data;
        // println("Observatory found %d new asteroids", pakiet.data);

        break;
    }
    default:
        debug("Received message %d from %d", status.MPI_TAG, status.MPI_SOURCE);
        break;
    }
}

void manageMessagePAIRED(packet_t pakiet, MPI_Status status)
{
    switch (status.MPI_TAG)
    {
    case PAIR_REQ:
    {
        pairQueue.push(std::make_pair(pakiet.ts, status.MPI_SOURCE));

        pairACK(status.MPI_SOURCE);

        break;
    }
    case PAIR_RELEASE:
    {
        pairQueue.pop();
        pairQueue.pop();
        // println("Removed 2 processes from the pair queue");

        break;
    }
    case ASTEROID_REQ:
    {
        asteroidQueue.push(std::make_pair(pakiet.ts, status.MPI_SOURCE));

        asteroidACK(status.MPI_SOURCE);

        break;
    }
    case ASTEROID_RELEASE:
    {
        asteroidCount--;
        asteroidQueue.pop();
        asteroidQueue.pop();
        // println("Destroyed 1 asteroid and removed 1 process from the asteroid queue");
        println("Otrzymałem ASTEROID_RELEASE, wychodzę z pary, a moją parą był %d", pair);
        if (pair == status.MPI_SOURCE)
        {
            pair = -1;
            changeState(REST);
        }

        break;
    }
    case ASTEROID_FOUND:
    {
        asteroidCount += pakiet.data;
        // println("Observatory found %d new asteroids", pakiet.data);

        break;
    }
    default:
        debug("Received message %d from %d", status.MPI_TAG, status.MPI_SOURCE);
        break;
    }
}

void manageMessageWAIT_ASTEROID(packet_t pakiet, MPI_Status status)
{
    switch (status.MPI_TAG)
    {
    case PAIR_REQ:
    {
        pairQueue.push(std::make_pair(pakiet.ts, status.MPI_SOURCE));

        pairACK(status.MPI_SOURCE);

        break;
    }
    case PAIR_RELEASE:
    {
        pairQueue.pop();
        pairQueue.pop();
        // println("Removed 2 processes from the pair queue");

        break;
    }
    case ASTEROID_REQ:
    {
        asteroidQueue.push(std::make_pair(pakiet.ts, status.MPI_SOURCE));

        asteroidACK(status.MPI_SOURCE);

        break;
    }
    case ASTEROID_RELEASE:
    {
        asteroidCount--;
        asteroidQueue.pop();
        asteroidQueue.pop();
        // println("Destroyed 1 asteroid and removed 1 process from the asteroid queue");

        break;
    }
    case ASTEROID_ACK:
    {
        tryToDestroyAsteroid();

        break;
    }
    case ASTEROID_FOUND:
    {
        asteroidCount += pakiet.data;
        // println("Observatory found %d new asteroids", pakiet.data);

        tryToDestroyAsteroid();

        break;
    }
    default:
        debug("Received message %d from %d", status.MPI_TAG, status.MPI_SOURCE);
        break;
    }
}

void telepathKom()
{
    MPI_Status status;
    MPI_Request request;
    int flag = 1;
    packet_t pakiet;
    while (true)
    {
        // println("Waiting for message");
        MPI_Recv(&pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        auto l = std::unique_lock<std::mutex>(mtx);

        if (flag == 0)
        {
            continue;
        }

        int currentState = getState();
        if (status.MPI_SOURCE != 0)
        {
            println("Received message in state %s with tag %s from %d", tag2string(currentState), tag2string(status.MPI_TAG), status.MPI_SOURCE);
        }

        updateClock(pakiet.ts);
        incrementClock();
        lastMessageLamportClocks[status.MPI_SOURCE] = pakiet.ts;
        // if (status.MPI_SOURCE != 0)
        // {
        //     println("lamportClock: %d", lamportClock);
        // }
        switch (currentState)
        {
        case REST:
            manageMessageREST(pakiet, status);
            break;
        case WAIT_PAIR:
            manageMessageWAIT_PAIR(pakiet, status);
            break;
        case PAIRED:
            manageMessagePAIRED(pakiet, status);
            break;
        case WAIT_ASTEROID:
            manageMessageWAIT_ASTEROID(pakiet, status);
            break;
        default:
            break;
        }

        l.unlock();
    }
}

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *ptr)
{
    switch (rank)
    {
    case OBSERVATORY:
        observatoryKom();
        break;
    default:
        telepathKom();
        break;
    }

    return NULL;

    /*
        MPI_Status status;
        int is_message = FALSE;
        packet_t pakiet;
        // Obrazuje pętlę odbierającą pakiety o różnych typach
        while ( stan!=InFinish ) {
            debug("czekam na recv");
            MPI_Recv( &pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            switch ( status.MPI_TAG ) {
                case FINISH:
                    changeState(InFinish);
                    break;
                case APP_PKT:
                    debug("Dostałem pakiet od %d z danymi %d",pakiet.src, pakiet.data);
                    break;
                default:
            break;
            }
        }
    */
}
