// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Userspace RCU library - test program
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <urcu/arch.h>
#include <urcu/assert.h>

#include "thread-id.h"

#define _LGPL_SOURCE
#include <urcu.h>

pthread_mutex_t rcu_copy_mutex = PTHREAD_MUTEX_INITIALIZER;

static
void rcu_copy_mutex_lock(void)
{
	int ret;
	ret = pthread_mutex_lock(&rcu_copy_mutex);
	if (ret) {
		perror("Error in pthread mutex lock");
		exit(-1);
	}
}

static
void rcu_copy_mutex_unlock(void)
{
	int ret;

	ret = pthread_mutex_unlock(&rcu_copy_mutex);
	if (ret) {
		perror("Error in pthread mutex unlock");
		exit(-1);
	}
}

struct test_array {
	int a;
};

static struct test_array *test_rcu_pointer;

#define OUTER_READ_LOOP	2000U
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
	unsigned int i, j;
	struct test_array *local_ptr;
	caa_cycles_t time1, time2;

	printf("thread_begin %s, tid %lu\n",
		"reader", urcu_get_thread_id());
	sleep(2);

	rcu_register_thread();

	time1 = caa_get_cycles();
	for (i = 0; i < OUTER_READ_LOOP; i++) {
		for (j = 0; j < INNER_READ_LOOP; j++) {
			rcu_read_lock();
			local_ptr = rcu_dereference(test_rcu_pointer);
			if (local_ptr) {
				urcu_posix_assert(local_ptr->a == 8);
			}
			rcu_read_unlock();
		}
	}
	time2 = caa_get_cycles();

	rcu_unregister_thread();

	reader_time[(unsigned long)arg] = time2 - time1;

	sleep(2);
	printf("thread_end %s, tid %lu\n",
		"reader", urcu_get_thread_id());
	return ((void*)1);

}

static
void *thr_writer(void *arg)
{
	unsigned int i, j;
	struct test_array *new, *old;
	caa_cycles_t time1, time2;

	printf("thread_begin %s, tid %lu\n",
		"writer", urcu_get_thread_id());
	sleep(2);

	for (i = 0; i < OUTER_WRITE_LOOP; i++) {
		for (j = 0; j < INNER_WRITE_LOOP; j++) {
			time1 = caa_get_cycles();
			new = malloc(sizeof(struct test_array));
			rcu_copy_mutex_lock();
			old = test_rcu_pointer;
			if (old) {
				urcu_posix_assert(old->a == 8);
			}
			new->a = 8;
			old = rcu_xchg_pointer(&test_rcu_pointer, new);
			rcu_copy_mutex_unlock();
			synchronize_rcu();
			/* can be done after unlock */
			if (old) {
				old->a = 0;
			}
			free(old);
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
	free(test_rcu_pointer);
	printf("Time per read : %g cycles\n",
	       (double)tot_rtime / ((double)NR_READ * (double)READ_LOOP));
	printf("Time per write : %g cycles\n",
	       (double)tot_wtime / ((double)NR_WRITE * (double)WRITE_LOOP));

	free(reader_time);
	free(writer_time);
	free(tid_reader);
	free(tid_writer);

	return 0;
}
