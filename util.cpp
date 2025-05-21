#include "main.h"
#include "util.h"
MPI_Datatype MPI_PAKIET_T;

struct tagNames_t
{
    const char *name;
    int tag;
} tagNames[] = {
    {"REST", REST},
    {"WAIT_PAIR", WAIT_PAIR},
    {"PAIRED", PAIRED},
    {"WAIT_ASTEROID", WAIT_ASTEROID},
    {"PAIR_REQ", PAIR_REQ},
    {"PAIR_RELEASE", PAIR_RELEASE},
    {"PAIR_ACK", PAIR_ACK},
    {"PAIR_PROPOSAL", PAIR_PROPOSAL},
    {"ASTEROID_REQ", ASTEROID_REQ},
    {"ASTEROID_RELEASE", ASTEROID_RELEASE},
    {"ASTEROID_ACK", ASTEROID_ACK},
    {"ASTEROID_FOUND", ASTEROID_FOUND}};

const char *tag2string(int tag)
{
    for (int i = 0; i < sizeof(tagNames) / sizeof(struct tagNames_t); i++)
    {
        if (tagNames[i].tag == tag)
            return tagNames[i].name;
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
    // int blocklengths[NITEMS] = {1, 1, 1};
    int blocklengths[NITEMS] = {1, 1};
    // MPI_Datatype typy[NITEMS] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Datatype typy[NITEMS] = {MPI_INT, MPI_INT};

    MPI_Aint offsets[NITEMS];
    // offsets[0] = offsetof(packet_t, ts);
    // offsets[1] = offsetof(packet_t, src);
    // offsets[2] = offsetof(packet_t, data);
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, data);

    MPI_Type_create_struct(NITEMS, blocklengths, offsets, typy, &MPI_PAKIET_T);

    MPI_Type_commit(&MPI_PAKIET_T);
}

/* opis patrz util.h */
void sendPacket(packet_t *pkt, int destination, int tag)
{
    debug("Początek wysyłania");
    int freepkt = 0;
    if (pkt == 0)
    {
        pkt = new packet_t;
        freepkt = 1;
    }
    // pkt->src = rank;

    if (providedMode == MPI_THREAD_SERIALIZED)
    {
        pthread_mutex_lock(&mpiMut);
        MPI_Send(pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
        pthread_mutex_unlock(&mpiMut);
    }
    else
    {
        MPI_Send(pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
    }
    debug("Wysyłam %s do %d", tag2string(tag), destination);
    if (freepkt == 1)
    {
        delete pkt;
    }
    debug("Koniec wysyłania");
}

int randomValue(int min, int max)
{
    int diff = max - min;
    int rand = random() % diff;
    return rand + min;
}

int getState()
{
    int returnState;
    pthread_mutex_lock(&stateMut);
    returnState = stan;
    pthread_mutex_unlock(&stateMut);
    return returnState;
}

void changeState(int newState)
{
    pthread_mutex_lock(&stateMut);
    // if (stan==InFinish) {
    // pthread_mutex_unlock( &stateMut );
    //     return;
    // }
    stan = newState;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&stateMut);
}

void waitForStateChange(int currentState)
{
    pthread_mutex_lock(&stateMut);
    if (currentState != stan)
    {
        pthread_mutex_unlock(&stateMut);
        return;
    }
    pthread_cond_wait(&cond, &stateMut);
    pthread_mutex_unlock(&stateMut);
}

void incrementClock()
{
    pthread_mutex_lock(&clockMut);
    lamportClock++;
    pthread_mutex_unlock(&clockMut);
}

void changeClock(int newClock)
{
    pthread_mutex_lock(&clockMut);
    lamportClock = newClock;
    pthread_mutex_unlock(&clockMut);
}

void updateClock(int newClock)
{
    pthread_mutex_lock(&clockMut);
    if (lamportClock > newClock)
    {
        pthread_mutex_unlock(&clockMut);
        return;
    }
    lamportClock = newClock;
    pthread_mutex_unlock(&clockMut);
}

void sendAllTelepaths(packet_t *pkt, int tag)
{
    for (int i = 1; i < size; i++)
    {
        sendPacket(pkt, i, tag);
    }
}

void enterPairQueue()
{
    println("Entering the pair queue to pair with another telepath");

    incrementClock();
    debug("lamportClock: %d", lamportClock);

    std::fill(isPairAckReceived.begin(), isPairAckReceived.end(), false);
    pairAckCount = 0;

    int currentClock = lamportClock;
    packet_t pkt;
    pkt.ts = currentClock;
    queueClock = currentClock;

    sendAllTelepaths(&pkt, PAIR_REQ);
    println("Sent messages PAIR_REQ to all telepaths");
}

void pairACK(int destination)
{
    packet_t pkt;
    pkt.ts = lamportClock;

    sendPacket(&pkt, destination, PAIR_ACK);
    // println("Sent PAIR_ACK to %d", destination);
}

void incrementPairACK(int process)
{
    if (!isPairAckReceived[process])
    {
        isPairAckReceived[process] = true;
        pairAckCount++;
    }
    debug("ack: %d", pairAckCount);
}

void tryToPair()
{
    debug("Początek próby parowania");
    if (pairQueue.empty())
    {
        debug("Koniec próby parowania");
        return;
    }
    int topQueue = pairQueue.top().second;
    if ((pairAckCount >= size - 2) && (topQueue != rank))
    {
        println("Found a pair");

        packet_t pkt;
        pkt.ts = lamportClock;

        sendPacket(&pkt, topQueue, PAIR_PROPOSAL);
        println("Sent PAIR_PROPOSAL to %d", topQueue);

        pair = topQueue;

        sendAllTelepaths(&pkt, PAIR_RELEASE);
        println("Sent messages PAIR_RELEASE to all telepaths");

        changeState(PAIRED);
    }
    debug("Koniec próby parowania");
}

void exitPairQueue()
{
    println("Exiting the pair queue after finding pair");

    queueClock = -1;

    packet_t pkt;
    pkt.ts = lamportClock;

    sendAllTelepaths(&pkt, PAIR_RELEASE);
    println("Sent messages PAIR_RELEASE to all telepaths");
}

void enterAsteroidQueue()
{
    println("Entering the asteroid queue to destroy asteroid");

    incrementClock();
    debug("lamportClock: %d", lamportClock);

    std::fill(isAsteroidAckReceived.begin(), isAsteroidAckReceived.end(), false);
    asteroidAckCount = 0;

    int currentClock = lamportClock;

    packet_t pkt;
    pkt.ts = currentClock;
    queueClock = currentClock;

    sendAllTelepaths(&pkt, ASTEROID_REQ);
    println("Sent messages ASTEROID_REQ to all telepaths");
}

void asteroidACK(int destination)
{
    packet_t pkt;
    pkt.ts = lamportClock;
    sendPacket(&pkt, destination, ASTEROID_ACK);
    // println("Sent ASTEROID_ACK to %d", destination);
}

void incrementAsteroidACK(int process)
{
    if (!isAsteroidAckReceived[process])
    {
        isAsteroidAckReceived[process] = true;
        asteroidAckCount++;
    }
    debug("ack:%d", asteroidAckCount);
}

void tryToDestroyAsteroid()
{
    debug("Początek próby niszczenia asteroidy");
    debug("asteroidAckCount: %d; size:%d; asteroidCount:%d", asteroidAckCount, size, asteroidCount);
    if (asteroidAckCount >= size - asteroidCount)
    {
        println("Received right to destroy asteroid");

        packet_t pkt;
        pkt.ts = lamportClock;

        pair = -1;

        sendAllTelepaths(&pkt, ASTEROID_RELEASE);
        println("Sent messages ASTEROID_RELEASE to all telepaths");

        changeState(REST);
    }
    debug("Koniec próby niszczenia asteroidy");
}

void exitAsteroidQueue()
{
    println("Exiting the asteroid queue after destroying asteroid");

    queueClock = -1;

    packet_t pkt;
    pkt.ts = lamportClock;

    sendAllTelepaths(&pkt, ASTEROID_RELEASE);
    println("Sent messages ASTEROID_RELEASE to all telepaths");
}
