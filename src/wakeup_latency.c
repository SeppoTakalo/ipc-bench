/*
 * Copyright (c) 2003 Paul Herman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DELAY_US	100L

int main (int ac, char **av) {
	long s;
	double min, max, avg, diff;
	struct timeval tv1, tv2;
	int count = 200;

	if (ac > 1 && av[1])
		count = strtol(av[1], NULL, 10);

	printf("Running %d loops of %ld us delays with usleep()\n", count, DELAY_US);
	min = 1.0;
	max = avg = 0.0;
	for(int i=0;i < count; i++) {
		gettimeofday(&tv1, NULL);
		s = DELAY_US + tv1.tv_usec;
		usleep(DELAY_US);
		gettimeofday(&tv2, NULL);

		diff = (double)(tv2.tv_usec - s)/1e6;
		diff += (double)(tv2.tv_sec - tv1.tv_sec);
		if (diff < min)
			min = diff;
		if (diff > max)
			max = diff;
		avg += diff;
	}
	avg *= 1000000.0;
	min *= 1000000.0;
	max *= 1000000.0;
	avg = (double)avg / (double)count;
	printf("min: %.3f us\nmax: %.3f us\nAverage: %.3f us\n", min, max, avg);
	return 0;
}
