// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Per thread locks - test program
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

#include <urcu/arch.h>
#include <urcu/assert.h>

#include "thread-id.h"

#include <urcu.h>

struct test_array {
	int a;
};

static struct test_array test_array = { 8 };

struct per_thread_lock {
	pthread_mutex_t lock;
} __attribute__((aligned(CAA_CACHE_LINE_SIZE)));	/* cache-line aligned */

static struct per_thread_lock *per_thread_lock;

#define OUTER_READ_LOOP	200U
#define INNER_READ_LOOP	100000U
#define READ_LOOP ((unsigned long long)OUTER_READ_LOOP * INNER_READ_LOOP)

#define OUTER_WRITE_LOOP 10U
#define INNER_WRITE_LOOP 200U
#define WRITE_LOOP ((unsigned long long)OUTER_WRITE_LOOP * INNER_WRITE_LOOP)

static int num_read;
static int num_write;

#define NR_READ num_read
#define NR_WRITE num_write

static caa_cycles_t __attribute__((aligned(CAA_CACHE_LINE_SIZE))) *reader_time;
static caa_cycles_t __attribute__((aligned(CAA_CACHE_LINE_SIZE))) *writer_time;

static
void *thr_reader(void *arg)
{
	caa_cycles_t time1, time2;
	long tidx = (long)arg;
	unsigned int i, j;
	int ret;

	printf("thread_begin %s, tid %lu\n",
		"reader", urcu_get_thread_id());
	sleep(2);

	time1 = caa_get_cycles();
	for (i = 0; i < OUTER_READ_LOOP; i++) {
		for (j = 0; j < INNER_READ_LOOP; j++) {
			ret = pthread_mutex_lock(&per_thread_lock[tidx].lock);
			if (ret) {
				perror("Error in pthread mutex lock");
				exit(-1);
			}
			urcu_posix_assert(test_array.a == 8);
			ret = pthread_mutex_unlock(&per_thread_lock[tidx].lock);
			if (ret) {
				perror("Error in pthread mutex unlock");
				exit(-1);
			}
		}
	}
	time2 = caa_get_cycles();

	reader_time[tidx] = time2 - time1;

	sleep(2);
	printf("thread_end %s, tid %lu\n",
		"reader", urcu_get_thread_id());
	return ((void*)1);

}

static
void *thr_writer(void *arg)
{
	caa_cycles_t time1, time2;
	unsigned int i, j;
	long tidx;
	int ret;

	printf("thread_begin %s, tid %lu\n",
		"writer", urcu_get_thread_id());
	sleep(2);

	for (i = 0; i < OUTER_WRITE_LOOP; i++) {
		for (j = 0; j < INNER_WRITE_LOOP; j++) {
			time1 = caa_get_cycles();
			for (tidx = 0; tidx < NR_READ; tidx++) {
				ret = pthread_mutex_lock(&per_thread_lock[tidx].lock);
				if (ret) {
					perror("Error in pthread mutex lock");
					exit(-1);
				}
			}
			test_array.a = 8;
			for (tidx = NR_READ - 1; tidx >= 0; tidx--) {
				ret = pthread_mutex_unlock(&per_thread_lock[tidx].lock);
				if (ret) {
					perror("Error in pthread mutex unlock");
					exit(-1);
				}
			}
			time2 = caa_get_cycles();
			writer_time[(unsigned long)arg] += time2 - time1;
			usleep(1);
		}
	}

	printf("thread_end %s, tid %lu\n",
		"writer", urcu_get_thread_id());
	return ((void*)2);
}

int main(int argc, char **argv)
{
	int err;
	pthread_t *tid_reader, *tid_writer;
	void *tret;
	int i;
	caa_cycles_t tot_rtime = 0;
	caa_cycles_t tot_wtime = 0;

	if (argc < 2) {
		printf("Usage : %s nr_readers nr_writers\n", argv[0]);
		exit(-1);
	}
	num_read = atoi(argv[1]);
	num_write = atoi(argv[2]);

	reader_time = calloc(num_read, sizeof(*reader_time));
	writer_time = calloc(num_write, sizeof(*writer_time));
	tid_reader = calloc(num_read, sizeof(*tid_reader));
	tid_writer = calloc(num_write, sizeof(*tid_writer));

	printf("thread %-6s, tid %lu\n",
		"main", urcu_get_thread_id());

	per_thread_lock = malloc(sizeof(struct per_thread_lock) * NR_READ);

	for (i = 0; i < NR_READ; i++) {
		pthread_mutex_init(&per_thread_lock[i].lock, NULL);
	}
	for (i = 0; i < NR_READ; i++) {
		err = pthread_create(&tid_reader[i], NULL, thr_reader,
				     (void *)(long)i);
		if (err != 0)
			exit(1);
	}
	for (i = 0; i < NR_WRITE; i++) {
		err = pthread_create(&tid_writer[i], NULL, thr_writer,
				     (void *)(long)i);
		if (err != 0)
			exit(1);
	}

	sleep(10);

	for (i = 0; i < NR_READ; i++) {
		err = pthread_join(tid_reader[i], &tret);
		if (err != 0)
			exit(1);
		tot_rtime += reader_time[i];
	}
	for (i = 0; i < NR_WRITE; i++) {
		err = pthread_join(tid_writer[i], &tret);
		if (err != 0)
			exit(1);
		tot_wtime += writer_time[i];
	}
	printf("Time per read : %g cycles\n",
	       (double)tot_rtime / ((double)NR_READ * (double)READ_LOOP));
	printf("Time per write : %g cycles\n",
	       (double)tot_wtime / ((double)NR_WRITE * (double)WRITE_LOOP));
	free(per_thread_lock);

	free(reader_time);
	free(writer_time);
	free(tid_reader);
	free(tid_writer);

	return 0;
}
