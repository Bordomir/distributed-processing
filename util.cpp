#include "main.h"
#include "util.h"
MPI_Datatype MPI_PAKIET_T;

struct tagNames_t{
    const char *name;
    int tag;
} tagNames[] = { 
    { "PAIR_REQ", PAIR_REQ },
    { "PAIR_RELEASE", PAIR_RELEASE },
    { "PAIR_ACK", PAIR_ACK },
    { "PAIR_PROPOSAL", PAIR_PROPOSAL },
    { "ASTEROID_REQ", ASTEROID_REQ },
    { "ASTEROID_RELEASE", ASTEROID_RELEASE },
    { "ASTEROID_ACK", ASTEROID_ACK },
    { "ASTEROID_FOUND", ASTEROID_FOUND }
};

const char const *tag2string( int tag )
{
    for (int i=0; i <sizeof(tagNames)/sizeof(struct tagNames_t);i++) {
	if ( tagNames[i].tag == tag )  return tagNames[i].name;
    }
    return "<unknown>";
}
/* tworzy typ MPI_PAKIET_T
*/
void inicjuj_typ_pakietu()
{
    /* Stworzenie typu */
    /* Poniższe (aż do MPI_Type_commit) potrzebne tylko, jeżeli
       brzydzimy się czymś w rodzaju MPI_Send(&typ, sizeof(pakiet_t), MPI_BYTE....
    */
    /* sklejone z stackoverflow */
    int       blocklengths[NITEMS] = {1,1,1};
    // int blocklengths[NITEMS] = {1,1};
    MPI_Datatype typy[NITEMS] = {MPI_INT, MPI_INT, MPI_INT};
    // MPI_Datatype typy[NITEMS] = {MPI_INT, MPI_INT};

    MPI_Aint     offsets[NITEMS]; 
    // offsets[0] = offsetof(packet_t, data);
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, data);

    MPI_Type_create_struct(NITEMS, blocklengths, offsets, typy, &MPI_PAKIET_T);

    MPI_Type_commit(&MPI_PAKIET_T);
}

/* opis patrz util.h */
void sendPacket(packet_t *pkt, int destination, int tag)
{
    int freepkt=0;
    if (pkt==0) { pkt = new packet_t; freepkt=1;}
    pkt->src = rank;
    MPI_Send( pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
    debug("Wysyłam %s do %d\n", tag2string(tag), destination);
    if (freepkt) delete pkt;
}

int randomValue(int min, int max)
{
    int diff = max - min;
    int rand = random()%diff;
    return rand+min;
}


state_t getState()
{
    state_t returnState;
    pthread_mutex_lock( &stateMut );
    returnState = stan;
    pthread_mutex_unlock( &stateMut );
    return returnState;
}

void changeState( state_t newState )
{
    pthread_mutex_lock( &stateMut );
    if (stan==InFinish) { 
	pthread_mutex_unlock( &stateMut );
        return;
    }
    stan = newState;
    pthread_mutex_unlock( &stateMut );
}

void incrementClock()
{
    pthread_mutex_lock( &clockMut );
    lamportClock++;
    pthread_mutex_unlock( &clockMut );
}

void changeClock( int newClock )
{
    pthread_mutex_lock( &clockMut );
    lamportClock = newClock;
    pthread_mutex_unlock( &clockMut );
}

void updateClock( int newClock )
{
    pthread_mutex_lock( &clockMut );
    if( lamportClock > newClock )
    {
        pthread_mutex_unlock( &clockMut );
        return;
    }
    lamportClock = newClock;
    pthread_mutex_unlock( &clockMut );
}

void sendAllTelepaths(packet_t *pkt, int tag)
{
    for(int i = 1; i < size; i++){
        sendPacket(pkt, i, tag);
    }
}

