#include "main.h"
#include "watek_komunikacyjny.h"

void observatoryKom()
{
    MPI_Status status;
    packet_t pakiet;
    while (true)
    {
        println("Waiting for mesage");
        if (providedMode == MPI_THREAD_SERIALIZED)
        {
            pthread_mutex_lock(&mpiMut);
            MPI_Recv(&pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            pthread_mutex_unlock(&mpiMut);
        }
        else
        {
            MPI_Recv(&pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        }

        switch (status.MPI_TAG)
        {
        default:
            updateClock(pakiet.ts);
            incrementClock();
            println("Received message %d from %d", status.MPI_TAG, status.MPI_SOURCE);
            break;
        }
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
        asteroidACK(status.MPI_SOURCE);

        break;
    }
    case ASTEROID_RELEASE:
    {
        asteroidCount--;
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

        // Process is not in queue yet
        if (queueClock == -1)
        {
            pairACK(status.MPI_SOURCE);
        }
        else
        {
            // Process has lower priority
            if (queueClock > pakiet.ts ||
                (queueClock == pakiet.ts && rank > status.MPI_SOURCE))
            {
                pairACK(status.MPI_SOURCE);
            }
        }

        break;
    }
    case PAIR_RELEASE:
    {
        int process = pairQueue.top().second;
        pairQueue.pop();
        incrementPairACK(process);

        process = pairQueue.top().second;
        pairQueue.pop();
        incrementPairACK(process);
        // println("Removed 2 processes from the pair queue");

        tryToPair();

        break;
    }
    case PAIR_ACK:
    {
        incrementPairACK(status.MPI_SOURCE);

        tryToPair();

        break;
    }
    case PAIR_PROPOSAL:
    {
        println("Found a pair");

        pair = status.MPI_SOURCE;

        changeState(WAIT_ASTEROID);

        break;
    }
    case ASTEROID_REQ:
    {
        asteroidACK(status.MPI_SOURCE);

        break;
    }
    case ASTEROID_RELEASE:
    {
        asteroidCount--;
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
        asteroidACK(status.MPI_SOURCE);

        break;
    }
    case ASTEROID_RELEASE:
    {
        asteroidCount--;
        // println("Destroyed 1 asteroid and removed 1 process from the asteroid queue");

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
        // Process is not in queue yet
        if (queueClock == -1)
        {
            asteroidACK(status.MPI_SOURCE);
        }
        else
        {
            // Process has lower priority
            if (queueClock > pakiet.ts ||
                (queueClock == pakiet.ts && rank > status.MPI_SOURCE))
            {
                asteroidACK(status.MPI_SOURCE);
            }
        }

        break;
    }
    case ASTEROID_RELEASE:
    {
        asteroidCount--;
        // println("Destroyed 1 asteroid and removed 1 process from the asteroid queue");

        incrementAsteroidACK(status.MPI_SOURCE);

        break;
    }
    case ASTEROID_ACK:
    {
        incrementAsteroidACK(status.MPI_SOURCE);

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
    packet_t pakiet;
    while (true)
    {
        // println("Waiting for message");
        if (providedMode == MPI_THREAD_SERIALIZED)
        {
            pthread_mutex_lock(&mpiMut);
            MPI_Recv(&pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            pthread_mutex_unlock(&mpiMut);
        }
        else
        {
            MPI_Recv(&pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        }

        state_t currentState = getState();
        debug("Received message in state %d with tag %s from %d", currentState, status.MPI_TAG, status.MPI_SOURCE);

        updateClock(pakiet.ts);
        incrementClock();
        debug("lamportClock: %d", lamportClock);
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
