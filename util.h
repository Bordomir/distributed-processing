#ifndef UTILH
#define UTILH
#include "main.h"

/* typ pakietu */
typedef struct
{
    int ts; /* timestamp (zegar lamporta */
    // int src;

    int data; // timestamp (zegar lamporta) lub dane jeżeli typ wiadomości to ASTEROID_FOUND
} packet_t;
/* packet_t ma trzy pola, więc NITEMS=3. Wykorzystane w inicjuj_typ_pakietu */
// #define NITEMS 3
#define NITEMS 2

/* Typy wiadomości */
// #define APP_PKT 11
// #define FINISH 12

// Typy związane z 1. kolejką
#define PAIR_REQ 1     // żądanie o dobranie się w parę,
#define PAIR_RELEASE 2 // poinformowanie o dobraniu się w parę która oznacza że 2 procesy opuściły kolejkę,
#define PAIR_ACK 3     // pozwolenie na dobranie się w parę,

#define PAIR_PROPOSAL 4 // propozycja dobrania się w parę od 2. procesu w kolejce,

// Typy związane z 2. kolejką
#define ASTEROID_REQ 5     // żądanie o pozwolenie na zniszczenie asteroidy,
#define ASTEROID_RELEASE 6 // poinformowanie zniszczeniu asteroidy przez proces i opuszczeniu przez niego koncepcyjnej kolejki,
#define ASTEROID_ACK 7     // pozwolenie na zniszczenie asteroidy,

#define ASTEROID_FOUND 8 // wiadomość od obserwatorium o nowych asteroidach

extern MPI_Datatype MPI_PAKIET_T;
const char *tag2string(int tag);
void inicjuj_typ_pakietu();

/* wysyłanie pakietu, skrót: wskaźnik do pakietu (0 oznacza stwórz pusty pakiet), do kogo, z jakim typem */
void sendPacket(packet_t *pkt, int destination, int tag);

int randomValue(int min, int max);
#endif
