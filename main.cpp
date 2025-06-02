#include "main.h"
#include "watek_glowny.h"
#include "watek_komunikacyjny.h"

int rank, size, lamportClock, asteroidCount, pair, pairRequestClock, asteroidClock, providedMode;
bool justStarted = true;
SimplePriorityQueue pairQueue;
SimplePriorityQueue asteroidQueue;
std::vector<int> lastAsteroidMessageLamportClocks; 
std::vector<int> lastPairMessageLamportClocks; 
// state_t stan=InRun;
int stan = REST;
pthread_t threadKom, threadMon;
pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER, clockMut = PTHREAD_MUTEX_INITIALIZER, mpiMut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
std::condition_variable cv;
std::mutex mtx;

void finalizuj()
{
    pthread_mutex_destroy(&stateMut);
    /* Czekamy, aż wątek potomny się zakończy */
    println("czekam na wątek \"komunikacyjny\"\n");
    pthread_join(threadKom, NULL);
    MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
}

void check_thread_support(int provided)
{
    printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided)
    {
    case MPI_THREAD_SINGLE:
        printf("Brak wsparcia dla wątków, kończę\n");
        /* Nie ma co, trzeba wychodzić */
        fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
        MPI_Finalize();
        exit(-1);
        break;
    case MPI_THREAD_FUNNELED:
        printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
        break;
    case MPI_THREAD_SERIALIZED:
        /* Potrzebne zamki wokół wywołań biblioteki MPI */
        printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
        break;
    case MPI_THREAD_MULTIPLE:
        printf("Pełne wsparcie dla wątków\n"); /* tego chcemy. Wszystkie inne powodują problemy */
        break;
    default:
        printf("Nikt nic nie wie\n");
    }
}

int main(int argc, char **argv)
{
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);
    srand(rank);
    srandom(rank*1);
    inicjuj_typ_pakietu(); // tworzy typ pakietu
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    lamportClock = 0;
    asteroidCount = 0;
    lastAsteroidMessageLamportClocks.resize(size, 0);
    lastPairMessageLamportClocks.resize(size, 0);
    providedMode = provided;
    pair = -1;
    pthread_cond_init(&cond, NULL);
    pthread_create(&threadKom, NULL, startKomWatek, 0);

    mainLoop();

    finalizuj();
    return 0;
}
