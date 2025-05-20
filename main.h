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

#include "util.h"
/* boolean */
#define TRUE 1
#define FALSE 0
#define SEC_IN_STATE 1
#define STATE_CHANGE_PROB 10

#define ROOT 0

#define ASTEROID_FOUND_PROB 25

#define MIN_ASTEROID_FOUND 1
#define MAX_ASTEROID_FOUND 5

#define OBSERVATORY_SLEEP_MIN 1
#define OBSERVATORY_SLEEP_MAX 5

#define TELEPATH_SLEEP_MIN 5
#define TELEPATH_SLEEP_MAX 15

extern int rank;
extern int size;
extern int lamportClock;
extern int pairAckCount;
extern int asteroidAckCount;
extern std::priority_queue<std::pair<int,int>> pairQueue;
typedef enum {
    InRun, 
    InMonitor, 
    InSend, 
    InFinish,
    REST,           // stan początkowy lub proces odpoczywa po zniszczeniu asteroidy,
    WAIT_PAIR,      // proces czeka na dobrania się w parę,
    PAIRED,         // proces jest dobrany w parę ale nie znajduje się w kolejce asteroidQueue,
    WAIT_ASTEROID   // proces jest dobrany w parę i czeka na asteroidę,
    } state_t;
extern state_t stan;
extern pthread_t threadKom, threadMon;

extern pthread_mutex_t stateMut, clockMut, condMut;
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
#define debug(FORMAT,...) printf("%c[%d;%dm [%d] [t%d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamportClock, ##__VA_ARGS__, 27,0,37);
#else
#define debug(...) ;
#endif

// makro println - to samo co debug, ale wyświetla się zawsze
#define println(FORMAT,...) printf("%c[%d;%dm [%d] [t%d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamportClock, ##__VA_ARGS__, 27,0,37);

state_t getState();
void changeState( state_t );

void incrementClock();
void changeClock( int );
void updateClock( int );

void sendAllTelepaths( packet_t*, int );

#endif
