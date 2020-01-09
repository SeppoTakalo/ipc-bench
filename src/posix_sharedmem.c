/*
    Measure latency of POSIX shared memory containing POSIX semaphore


    Copyright (c) 2019 Seppo Takalo <seppo.takalo@arm.com>
    Copyright (c) 2016 Erik Rigtorp <erik@rigtorp.se>

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use,
    copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following
    conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>


#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0) &&                           \
    defined(_POSIX_MONOTONIC_CLOCK)
#define HAS_CLOCK_GETTIME_MONOTONIC
#endif

#define SHM_NAME "/my_memory"
#define SHM_SIZE 1024

struct my_memory_region {
    sem_t writer_sem;
    sem_t reader_sem;
    char text[256];
} *shm;

int main(int argc, char *argv[])
{
    int64_t count, i, delta;
#ifdef HAS_CLOCK_GETTIME_MONOTONIC
    struct timespec start, stop;
#else
    struct timeval start, stop;
#endif

    if (argc != 2) {
        printf("usage: sysv_semaphore <roundtrip-count>\n");
        return 1;
    }

    count = atol(argv[1]);

    printf("roundtrip count: %li\n", (long)count);

    /* Create shared memory */
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0);
    if (-1 == fd) {
        perror("shm_open()");
        return 1;
    }
    if (ftruncate(fd, SHM_SIZE)) {
        perror("ftruncate()");
        return 1;
    }

    shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == shm) {
        perror("mmap()");
        return 1;
    }
    /* File handle, and actual shared memory region
     * can now be removed, as kernel will then remove it once remaining applications
     * call munmap() or close all handles to it, or exit.
     * We still have mmap() handle into the region so it remains effective.
     */
    if (close(fd)) {
        perror("close()");
        return 1;
    }
    if (shm_unlink(SHM_NAME)) {
        perror("shm_unlink()");
        return 1;
    }

    /* Init process shared semaphores */
    if (sem_init(&shm->writer_sem, 1, 1) || sem_init(&shm->reader_sem, 1, 0)) {
        perror("sem_init()");
        return 1;
    }

    if (!fork()) { /* child */
        for (i = 0; i < count; i++) {
            sem_wait(&shm->reader_sem);
            snprintf(shm->text, 256, "Pong");
            sem_post(&shm->writer_sem);
        }

    } else { /* parent */

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
            sem_wait(&shm->writer_sem);
            snprintf(shm->text, 256, "Ping");
            sem_post(&shm->reader_sem);
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
        wait(NULL);
        printf("average latency: %lli ns\n", delta / (count * 2));
    }

    return 0;
}
