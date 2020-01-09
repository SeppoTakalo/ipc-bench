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
#include <fcntl.h> /* Defines O_* constants */
#include <sys/stat.h> /* Defines mode constants */
#include <mqueue.h>

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0) &&                           \
    defined(_POSIX_MONOTONIC_CLOCK)
#define HAS_CLOCK_GETTIME_MONOTONIC
#endif

int main(int argc, char *argv[])
{
    mqd_t mq_up;
    mqd_t mq_down;

    int size;
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

    void *buf = malloc(size);
    if (buf == NULL) {
        perror("malloc");
        return 1;
    }

    printf("message size: %i octets\n", size);
    printf("roundtrip count: %li\n", count);

    /* Create message queue */
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = size;
    
    mq_unlink("/UP");
    mq_unlink("/DOWN");
    mq_up = mq_open("/UP", O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, &attr);
    mq_down = mq_open("/DOWN", O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, &attr);

    if ((-1 == mq_up) || (-1 == mq_down)) {
        perror("open_mq");
        return 1;
    }

    if (!fork()) { /* child */
        for (i = 0; i < count; i++) {
            if (mq_receive(mq_down, buf, size, NULL) == -1) {
                perror("mq_receive");
                return 1;
            }

            if (mq_send(mq_up, buf, size, 0) == -1) {
                perror("mq_send");
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
            if (mq_send(mq_down, buf, size, 0) == -1) {
                perror("mq_send");
                return 1;
            }
            if (mq_receive(mq_up, buf, size, NULL) == -1) {
                perror("mq_receive");
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
        mq_close(mq_up);
        mq_close(mq_down);
        mq_unlink("/UP");
        mq_unlink("/DOWN");
    }

    return 0;
}
