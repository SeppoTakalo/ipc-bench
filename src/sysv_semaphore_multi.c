/*
    Measure latency of System V semaphore


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
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0) &&                           \
    defined(_POSIX_MONOTONIC_CLOCK)
#define HAS_CLOCK_GETTIME_MONOTONIC
#endif

#ifdef __linux__
union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                           (Linux-specific) */
};
#endif

int main(int argc, char *argv[])
{
    int semid;
    int64_t count, i, delta;
    int childrens;

#ifdef HAS_CLOCK_GETTIME_MONOTONIC
    struct timespec start, stop;
#else
    struct timeval start, stop;
#endif

    if (argc != 3) {
        printf("usage: sysv_semaphore <roundtrip-count> <number of childs>\n");
        return 1;
    }

    count = atol(argv[1]);
    childrens = atoi(argv[2]);
    printf("Number of childs: %d\n", childrens);

    printf("roundtrip count: %li\n", (long)count);

    semid = semget(IPC_PRIVATE, 2 * childrens, IPC_CREAT | S_IRUSR | S_IWUSR);

    if (-1 == semid) {
        perror("semget");
        return 1;
    }

    /* Init semaphores */
    u_short vals[2 * childrens];
    memset(vals, 0, 2 * childrens * sizeof(u_short));
    union semun semarg = {.array = vals};
    if (semctl(semid, 0, SETALL, semarg)) {
        perror("semctl(SETALL)");
        return 1;
    }

    long childs[childrens];
    for (int j=0; j < childrens; j++) {
        childs[j] = (long)fork();
        if (!childs[j]) { /* child */
            struct sembuf sop_wait = {
                .sem_num = j,
                .sem_op = -1,
                .sem_flg = 0
            };
            struct sembuf sop_release = {
                .sem_num = childrens + j,
                .sem_op = 1,
                .sem_flg = 0
            };

            for (i = 0; i < count; i++) {
                if (semop(semid, &sop_wait, 1)) {
                    perror("semop");
                    return 1;
                }
                if (semop(semid, &sop_release, 1)) {
                    perror("semop");
                    return 1;
                }
            }
            /* Child exit here */
            return 0;
        }
    }

    /* parent */

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
        for (int j=0; j < childrens; j++) {
            struct sembuf sop_release = {
                .sem_num = j,
                .sem_op = 1,
                .sem_flg = 0
            };
            if (semop(semid, &sop_release, 1)) {
                perror("semop");
                return 1;
            }
        }
        for (int j=0; j < childrens; j++) {
            struct sembuf sop_wait = {
                .sem_num = childrens + j,
                .sem_op = -1,
                .sem_flg = 0
            };
            if (semop(semid, &sop_wait, 1)) {
                perror("semop");
                return 1;
            }
        }
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
    if (semctl(semid, 0, IPC_RMID)) {
        perror("semctl(IPC_RMID)");
        return 1;
    }
    printf("average latency: %lli ns\n", delta / (count * 2));

    return 0;
}
