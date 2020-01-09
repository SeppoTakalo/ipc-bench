/*
    Measure latency of System V message queue


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
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/errno.h>

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0) &&                           \
    defined(_POSIX_MONOTONIC_CLOCK)
#define HAS_CLOCK_GETTIME_MONOTONIC
#endif

struct msgbuf {
    long mtype;       /* message type, must be > 0 */
    char mtext[1];    /* message data */
};

int main(int argc, char *argv[])
{
    int mq_up;
    int mq_down;

    int size;
    struct msgbuf *buf;
    int64_t count, i, delta;
#ifdef HAS_CLOCK_GETTIME_MONOTONIC
    struct timespec start, stop;
#else
    struct timeval start, stop;
#endif

    if (argc != 3) {
        printf("usage: %s <message-size> <roundtrip-count>\n", argv[0]);
        return 1;
    }

    size = atoi(argv[1]);
    count = atol(argv[2]);

    buf = (struct msgbuf *)malloc(size + sizeof(struct msgbuf));
    if (buf == NULL) {
        perror("malloc");
        return 1;
    }
    buf->mtype = 1; // Must be positive integer

    printf("message size: %i octets\n", size);
    printf("roundtrip count: %li\n", count);

    mq_up = msgget(IPC_PRIVATE, 0644 | IPC_CREAT | IPC_EXCL);
    mq_down = msgget(IPC_PRIVATE, 0644 | IPC_CREAT | IPC_EXCL);

    if ((-1 == mq_up) || (-1 == mq_down)) {
        perror("msgget");
        return 1;
    }

    if (!fork()) { /* child */
        for (i = 0; i < count; i++) {
            if (msgrcv(mq_down, buf, size, 0, 0) < 0) {
                perror("msgrcv");
                return 1;
            }

            if (msgsnd(mq_up, buf, size, 0)) {
                perror("msgsnd");
                return 1;
            }
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
            if (msgsnd(mq_down, buf, size, 0)) {
                perror("msgsnd");
                return 1;
            }
            if (msgrcv(mq_up, buf, size, 0, 0) < 0) {
                perror("msgrcv");
                return 1;
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

        printf("average latency: %li ns\n", delta / (count * 2));
        msgctl(mq_up, IPC_RMID, NULL);
        msgctl(mq_down, IPC_RMID, NULL);
    }

    return 0;
}
