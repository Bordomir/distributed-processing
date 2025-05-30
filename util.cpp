#include "main.h"
#include "util.h"
MPI_Datatype MPI_PAKIET_T;

std::string printVector(std::vector<bool> vec)
{
    std::string result = "[";
    for (bool i: vec)
    {
        if(i){
            result += '1';
        } else 
        {
            result += '0';
        }
    }
    result += ']';
    return result;
}

std::string printVector(std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<std::pair<int, int>>> q)
{
    std::string result = "[";
    while (! q.empty() ) {
        result += '(';
        result += std::to_string(q.top().first);
        result += ':';
        result += std::to_string(q.top().second);
        result += ')';
        result += ',';
        q.pop();
    }
    result += ']';
    return result;
}


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
    for (unsigned int i = 0; i < sizeof(tagNames) / sizeof(struct tagNames_t); i++)
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
    println("Changed state to %s", tag2string(newState));
    incrementClock();
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
    incrementClock();
    packet_t pkt;
    pkt.ts = lamportClock;

    sendPacket(&pkt, destination, PAIR_ACK);
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

void tryToSendPairProposal()
{
//  [4] [t420]: Changed state to WAIT_PAIR
//  [4] [t420]: Entering the pair queue to pair with another telepath
//  [4] [t421]: Sent messages PAIR_REQ to all telepaths
//  [4] [t428]: Found a pair
//  [4] [t428]:    pairAckCount:6; topQueue:2; topQueueClock:419; rank:4; queueClock:421; isPairAckReceived:[01011111]
//  [4] [t428]:    pairQueue:[(419:2),(421:4),]
//  [4] [t428]: Sent PAIR_PROPOSAL to 2
//  [4] [t428]: Paired with other telepath
//  [4] [t428]: Changed state to PAIRED
//  [7] [t422]: Changed state to WAIT_PAIR
//  [7] [t422]: Entering the pair queue to pair with another telepath
//  [7] [t423]: Sent messages PAIR_REQ to all telepaths
//  [2] [t429]: Found a pair
//  [7] [t431]: Found a pair
//  [7] [t431]:    pairAckCount:6; topQueue:2; topQueueClock:419; rank:7; queueClock:423; isPairAckReceived:[01011111]
//  [7] [t431]:    pairQueue:[(419:2),(421:4),(423:7),]
//  [7] [t431]: Sent PAIR_PROPOSAL to 2
//  [7] [t431]: Paired with other telepath
//  [7] [t431]: Changed state to PAIRED

    if (pairQueue.empty())
    {
        debug("Pair queue is empty");
        return;
    }
    int topQueue = pairQueue.top().second;
    if ((pairAckCount == size - 2) && (topQueue != rank) && (isPairAckReceived[rank]) && (pair == -1))
    {
        incrementClock();

        println("Found a pair");
        println("\tpairAckCount:%d; topQueue:%d; topQueueClock:%d; rank:%d; queueClock:%d; isPairAckReceived:%s", pairAckCount, topQueue, pairQueue.top().first, rank, queueClock, printVector(isPairAckReceived).c_str());
        println("\tpairQueue:%s",printVector(pairQueue).c_str());

        packet_t pkt;
        pkt.ts = lamportClock;

        sendPacket(&pkt, topQueue, PAIR_PROPOSAL);
        println("Sent PAIR_PROPOSAL to %d", topQueue);

        pair = topQueue;
    }
}

void tryToPair()
{

    if (pairQueue.empty())
    {
        debug("Pair queue is empty");
        return;
    }
    int topQueue = pairQueue.top().second;
    if ((pairAckCount == size - 1) && (topQueue == rank) && (pair != -1))
    {
        incrementClock();

        println("Found a pair");
        println("\tpairAckCount:%d; topQueue:%d; topQueueClock:%d; rank:%d; queueClock:%d; isPairAckReceived:%s", pairAckCount, topQueue, pairQueue.top().first, rank, queueClock, printVector(isPairAckReceived).c_str());
        println("\tpairQueue:%s",printVector(pairQueue).c_str());

        exitPairQueue();

        changeState(WAIT_ASTEROID);
    }
}

void exitPairQueue()
{
    println("Exiting the pair queue after finding pair");
    incrementClock();

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
    incrementClock();
    packet_t pkt;
    pkt.ts = lamportClock;
    sendPacket(&pkt, destination, ASTEROID_ACK);
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
    if (asteroidAckCount >= size - asteroidCount)
    {
        println("Received right to destroy asteroid");
        incrementClock();

        println("BOOOM, asteroid shattered!");
        pair = -1;

        incrementClock();

        exitAsteroidQueue();

        changeState(REST);
    }
}

void exitAsteroidQueue()
{
    println("Exiting the asteroid queue after destroying asteroid");
    incrementClock();

    queueClock = -1;

    packet_t pkt;
    pkt.ts = lamportClock;

    sendAllTelepaths(&pkt, ASTEROID_RELEASE);
    println("Sent messages ASTEROID_RELEASE to all telepaths");
}
