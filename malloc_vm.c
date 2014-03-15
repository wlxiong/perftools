#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>

#define NTHREADS  10
#define NALLOCS  10000
#define ALLOCSIZE  10000

static volatile int go;
static volatile int die;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void *ps[NALLOCS];  // allocations that are freed in turn by each thread
static void *pps1[NTHREADS];  // straggling allocations to prevent arena free
static void *pps2[NTHREADS];  // straggling allocations to prevent arena free

void
my_sleep(
    int ms
    )
{
    int rv;
    struct timespec ts;
    struct timespec rem;

    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    for (;;) {
        rv = nanosleep(&ts, &rem);
        if (! rv) {
            break;
        }
        assert(errno == EINTR);
        ts = rem;
    }
}

void *
my_thread(
    void *context
    )
{
    int i;
    int rv;
    void *p;

    // first we spin to get our own arena
    while (go == 0) {
        p = malloc(ALLOCSIZE);
        assert(p);
        if (rand()%20000 == 0) {
            my_sleep(10);
        }
        free(p);
    }

    // then we give main a chance to print stats
    while (go == 1) {
        my_sleep(100);
    }
    assert(go == 2);

    // then one thread at a time, do our big allocs
    rv = pthread_mutex_lock(&mutex);
    assert(! rv);
    printf("thread %d alloc 100MB\n", (int)(intptr_t)context);
    for (i = 0; i < NALLOCS; i++) {
        ps[i] = malloc(ALLOCSIZE);
        assert(ps[i]);
    }
    printf("thread %d free 100MB-20kB\n", (int)(intptr_t)context);
    // N.B. we leave two allocations straggling
    pps1[(int)(intptr_t)context] = ps[0];
    for (i = 1; i < NALLOCS-1; i++) {
        free(ps[i]);
    }
    pps2[(int)(intptr_t)context] = ps[i];
    rv = pthread_mutex_unlock(&mutex);
    assert(! rv);
}

int
main()
{
    int i;
    int rv;
    pthread_t thread;

    printf("creating %d threads\n", NTHREADS);
    for (i = 0; i < NTHREADS; i++) {
        rv = pthread_create(&thread, NULL, my_thread, (void *)(intptr_t)i);
        assert(! rv);
        rv = pthread_detach(thread);
        assert(! rv);
    }

    printf("allowing threads to contend to create preferred arenas\n");
    my_sleep(20000);

    printf("display preferred arenas\n");
    go = 1;
    my_sleep(1000);
    malloc_stats();

    printf("allowing threads to allocate 100MB each, sequentially in turn\n");
    go = 2;
    my_sleep(5000);
    malloc_stats();

    // free the stragglers
    for (i = 0; i < NTHREADS; i++) {
        free(pps1[i]);
        free(pps2[i]);
    }

    return 0;
}