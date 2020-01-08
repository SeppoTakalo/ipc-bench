/*
    Measure latency of gettimeofday()

*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0) &&                           \
    defined(_POSIX_MONOTONIC_CLOCK)
#define HAS_CLOCK_GETTIME_MONOTONIC
#endif

int main(int argc, char *argv[])
{
    int i;
    int64_t count, delta;
#ifdef HAS_CLOCK_GETTIME_MONOTONIC
    struct timespec start, stop, temp;
#else
    struct timeval start, stop, temp;
#endif

    if (argc != 2) {
        printf("usage: gettimeofday <count>\n");
        return 1;
    }

    count = atol(argv[1]);

    printf("measurements count: %li\n", count);

#ifdef HAS_CLOCK_GETTIME_MONOTONIC
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        perror("clock_gettime");
        return 1;
    }
#else
    if (gettimeofday(&start, NULL) == -1) {
        perror("gettimeofday");
        return 1;
    }
#endif

    for (i = 0; i < count; i++) {
#ifdef HAS_CLOCK_GETTIME_MONOTONIC
        if (clock_gettime(CLOCK_MONOTONIC, &temp) == -1) {
            perror("clock_gettime");
            return 1;
        }
#else
        if (gettimeofday(&temp, NULL) == -1) {
            perror("gettimeofday");
            return 1;
        }
#endif
    }

#ifdef HAS_CLOCK_GETTIME_MONOTONIC
    if (clock_gettime(CLOCK_MONOTONIC, &stop) == -1) {
        perror("clock_gettime");
        return 1;
    }

    delta = ((stop.tv_sec - start.tv_sec) * 1000000000 +
             (stop.tv_nsec - start.tv_nsec));

#else
    if (gettimeofday(&stop, NULL) == -1) {
        perror("gettimeofday");
        return 1;
    }

    delta =
        (stop.tv_sec - start.tv_sec) * 1000000000 + (stop.tv_usec - start.tv_usec) * 1000;

#endif

    printf("average latency: %li ns\n", delta / (count * 2));

    return 0;
}
