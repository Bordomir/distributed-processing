#include "main.h"
#include "watek_komunikacyjny.h"

void observatoryKom()
{
    MPI_Status status;
    packet_t pakiet;
    while (TRUE)
    {
	    println("Waiting for mesage");
        MPI_Recv( &pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        switch ( status.MPI_TAG ) {
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
    updateClock(pakiet.ts);
    // incrementClock();
    debug("lamportClock: %d", lamportClock)
    switch ( status.MPI_TAG ) {
        case PAIR_REQ:
            pairQueue.push(std::make_pair(pakiet.ts, status.MPI_SOURCE));

            pairACK(status.MPI_SOURCE);

            break;
        case PAIR_RELEASE:
            incrementClock();
            debug("lamportClock: %d", lamportClock)

            pairQueue.pop();
            pairQueue.pop();
            println("Removed 2 processes from the pair queue")
            
            break;
        case ASTEROID_REQ:
            asteroidACK(status.MPI_SOURCE);
            
            break;
        case ASTEROID_RELEASE:
            incrementClock();
            debug("lamportClock: %d", lamportClock)

            asteroidCount--;
            println("Destroyed 1 asteroid and removed 1 process from the asteroid queue")
            
            break;
        case ASTEROID_FOUND:
            incrementClock();
            debug("lamportClock: %d", lamportClock)

            asteroidCount += pakiet.data;
            println("Observatory found %d new asteroids", pakiet.data)
            
            break;
        default:
            debug("Received message %d from %d", status.MPI_TAG, status.MPI_SOURCE);
            break;
    }

}

void manageMessageWAIT_PAIR(packet_t pakiet, MPI_Status status)
{

}

void manageMessagePAIRED(packet_t pakiet, MPI_Status status)
{

}

void manageMessageWAIT_ASTEROID(packet_t pakiet, MPI_Status status)
{

}

void telepathKom()
{
    MPI_Status status;
    packet_t pakiet;
    while (TRUE)
    {
	    println("Waiting for message");
        MPI_Recv( &pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        state_t currentState = getState();
        debug("Received message in state %d with tag %s from %d", currentState, status.MPI_TAG, status.MPI_SOURCE);
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
        case ROOT:
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
