#include "main.h"
#include "util.h"
MPI_Datatype MPI_PAKIET_T;

std::string printVector(std::vector<bool> vec)
{
    std::string result = "[";
    for (bool i : vec)
    {
        if (i)
        {
            result += '1';
        }
        else
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
    while (!q.empty())
    {
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

std::string printVector(std::vector<int> vec)
{
    std::string result = "[";
    for (auto el : vec)
    {
        result += std::to_string(el);
        result += ", ";
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

    MPI_Send(pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);

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
    return stan;
}

void changeState(int newState)
{
    // if (stan==InFinish) {
    // pthread_mutex_unlock( &stateMut );
    //     return;
    // }
    stan = newState;
    println("Changed state to %s", tag2string(newState));
    incrementClock();
    cv.notify_all();
}

// void waitForStateChange(int currentState, std::unique_lock<std::mutex> &ul)
// {
//     cv.wait(ul, [currentState]()
//             { return getState() != currentState; });
// }

void incrementClock()
{
    lamportClock++;
}

void changeClock(int newClock)
{
    lamportClock = newClock;
}

void updateClock(int newClock)
{
    if (lamportClock > newClock)
    {
        return;
    }
    lamportClock = newClock;
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

    int currentClock = lamportClock;
    packet_t pkt;
    pkt.ts = currentClock;
    pairRequestClock = currentClock;

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

void tryToSendPairProposal()
{
    if (pairQueue.empty())
    {
        debug("Pair queue is empty");
        return;
    }

    if (pairQueue.size() <= 1)
    {
        return;
    }

    bool allProcessesClocksGreaterThanMyReqClock = true;

    for (int i = 1; i < size - 1; i++)
    {
        if (lastMessageLamportClocks[i] <= pairRequestClock)
        {
            allProcessesClocksGreaterThanMyReqClock = false;
        }
    }

    if (pair != -1)
    {
        return;
        // throw std::runtime_error("Pair is not none");
    }

    const int topQueue = pairQueue.top().second;

    auto pairQueueCopy = pairQueue;
    pairQueueCopy.pop();
    if (pairQueueCopy.top().second == rank && allProcessesClocksGreaterThanMyReqClock)
    {
        incrementClock();

        println("Found a pair");
        println("\ttopQueue:%d; topQueueClock:%d; rank:%d", topQueue, pairQueue.top().first, rank);
        println("\tpairQueue:%s", printVector(pairQueue).c_str());

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

    if (pairQueue.size() <= 1)
    {
        return;
    }

    bool allProcessesClocksGreaterThanMyReqClock = true;

    for (int i = 1; i < size - 1; i++)
    {
        if (lastMessageLamportClocks[i] <= pairRequestClock)
        {
            allProcessesClocksGreaterThanMyReqClock = false;
        }
    }

    int topQueue = pairQueue.top().second;
    if ((topQueue == rank) && (pair != -1) && allProcessesClocksGreaterThanMyReqClock)
    {
        incrementClock();

        println("Found a pair");
        println("\ttopQueue:%d; topQueueClock:%d; rank:%d", topQueue, pairQueue.top().first, rank);
        println("\tpairQueue:%s", printVector(pairQueue).c_str());

        exitPairQueue();

        changeState(WAIT_ASTEROID);
    }
}

void exitPairQueue()
{
    println("Exiting the pair queue after finding pair");
    incrementClock();

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

    int currentClock = lamportClock;
    packet_t pkt;
    pkt.ts = currentClock;
    asteroidClock = currentClock;

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

void tryToDestroyAsteroid()
{

    bool allProcessesClocksGreaterThanMyReqClock = true;

    for (int i = 1; i < size - 1; i++)
    {
        if (lastMessageLamportClocks[i] <= pairRequestClock)
        {
            allProcessesClocksGreaterThanMyReqClock = false;
        }
    }

    int topQueue = pairQueue.top().second;
    if ((topQueue == rank) && allProcessesClocksGreaterThanMyReqClock)
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

    packet_t pkt;
    pkt.ts = lamportClock;

    sendAllTelepaths(&pkt, ASTEROID_RELEASE);
    println("Sent messages ASTEROID_RELEASE to all telepaths");
}
