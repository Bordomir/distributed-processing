#ifndef MAINH
#define MAINH
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <queue>
#include <utility>
#include <vector>
#include <set>
#include <string>

#include "util.h"
/* boolean */
// #define TRUE 1
// #define FALSE 0
// #define SEC_IN_STATE 1
// #define STATE_CHANGE_PROB 10

#define OBSERVATORY 0

#define ASTEROID_FOUND_PROB 25

#define MIN_ASTEROID_FOUND 1
#define MAX_ASTEROID_FOUND 5

#define OBSERVATORY_SLEEP_MIN 1
#define OBSERVATORY_SLEEP_MAX 5

#define TELEPATH_SLEEP_MIN 5
#define TELEPATH_SLEEP_MAX 15

#define REST 11          // stan początkowy lub proces odpoczywa po zniszczeniu asteroidy,
#define WAIT_PAIR 12     // proces czeka na dobrania się w parę,
#define PAIRED 13        // proces jest dobrany w parę ale nie znajduje się w kolejce asteroidQueue,
#define WAIT_ASTEROID 14 // proces jest dobrany w parę i czeka na asteroidę,

extern int rank;
extern int size;

extern int lamportClock;
extern int queueClock;
extern std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<std::pair<int, int>>> pairQueue;
extern int pairAckCount;
extern std::vector<bool> isPairAckReceived;
extern int asteroidAckCount;
extern std::vector<bool> isAsteroidAckReceived;
extern int asteroidCount;
extern int pair;

extern int providedMode;
extern int justStarted;
// typedef enum
// {
// InRun,
// InMonitor,
// InSend,
// InFinish,
// REST,         // stan początkowy lub proces odpoczywa po zniszczeniu asteroidy,
// WAIT_PAIR,    // proces czeka na dobrania się w parę,
// PAIRED,       // proces jest dobrany w parę ale nie znajduje się w kolejce asteroidQueue,
// WAIT_ASTEROID // proces jest dobrany w parę i czeka na asteroidę,
// } state_t;
extern int stan;
extern pthread_t threadKom, threadMon;

extern pthread_mutex_t stateMut, clockMut, mpiMut;
extern pthread_cond_t cond;

/* macro debug - działa jak printf, kiedy zdefiniowano
   DEBUG, kiedy DEBUG niezdefiniowane działa jak instrukcja pusta

   używa się dokładnie jak printfa, tyle, że dodaje kolorków i automatycznie
   wyświetla rank

   w związku z tym, zmienna "rank" musi istnieć.

   w printfie: definicja znaku specjalnego "%c[%d;%dm [%d]" escape[styl bold/normal;kolor [RANK]
                                           FORMAT:argumenty doklejone z wywołania debug poprzez __VA_ARGS__
                       "%c[%d;%dm"       wyczyszczenie atrybutów    27,0,37
                                            UWAGA:
                                                27 == kod ascii escape.
                                                Pierwsze %c[%d;%dm ( np 27[1;10m ) definiuje styl i kolor literek
                                                Drugie   %c[%d;%dm czyli 27[0;37m przywraca domyślne kolory i brak pogrubienia (bolda)
                                                ...  w definicji makra oznacza, że ma zmienną liczbę parametrów

*/
#ifdef DEBUG
#define debug(FORMAT, ...) printf("%c[%d;%dm [%d] [t%d] [%s]: " FORMAT "%c[%d;%dm\n", 27, (1 + (rank / 7)) % 2, 31 + (6 + rank) % 7, rank, lamportClock, tag2string(stan), ##__VA_ARGS__, 27, 0, 37);
#else
#define debug(...) ;
#endif

// makro println - to samo co debug, ale wyświetla się zawsze
#define println(FORMAT, ...) printf("%c[%d;%dm [%d] [t%d] [%s]: " FORMAT "%c[%d;%dm\n", 27, (1 + (rank / 7)) % 2, 31 + (6 + rank) % 7, rank, lamportClock, tag2string(stan), ##__VA_ARGS__, 27, 0, 37);

int getState();
void changeState(int);
void waitForStateChange(int);

void incrementClock();
void changeClock(int);
void updateClock(int);

void sendAllTelepaths(packet_t *, int);

void enterPairQueue();
void pairACK(int);
void incrementPairACK(int);
void tryToSendPairProposal();
void tryToPair();
void exitPairQueue();

void enterAsteroidQueue();
void asteroidACK(int);
void incrementAsteroidACK(int);
void tryToDestroyAsteroid();
void exitAsteroidQueue();

#endif
