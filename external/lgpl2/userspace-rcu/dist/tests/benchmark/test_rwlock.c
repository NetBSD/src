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
#include <urcu/tls-compat.h>
#include "thread-id.h"

/* hardcoded number of CPUs */
#define NR_CPUS 16384

#ifndef DYNAMIC_LINK_TEST
#define _LGPL_SOURCE
#endif
#include <urcu.h>

struct test_array {
	int a;
};

/*
 * static rwlock initializer is broken on Cygwin. Use runtime
 * initialization.
 */
pthread_rwlock_t lock;

static unsigned long wdelay;

static volatile struct test_array test_array = { 8 };

static unsigned long duration;

/* read-side C.S. duration, in loops */
static unsigned long rduration;

/* write-side C.S. duration, in loops */
static unsigned long wduration;

static inline void loop_sleep(unsigned long loops)
{
	while (loops-- != 0)
		caa_cpu_relax();
}

static int verbose_mode;

#define printf_verbose(fmt, args...)		\
	do {					\
		if (verbose_mode)		\
			printf(fmt, args);	\
	} while (0)

static unsigned int cpu_affinities[NR_CPUS];
static unsigned int next_aff = 0;
static int use_affinity = 0;

pthread_mutex_t affinity_mutex = PTHREAD_MUTEX_INITIALIZER;

static void set_affinity(void)
{
#ifdef HAVE_SCHED_SETAFFINITY
	cpu_set_t mask;
	int cpu, ret;
#endif /* HAVE_SCHED_SETAFFINITY */

	if (!use_affinity)
		return;

#ifdef HAVE_SCHED_SETAFFINITY
	ret = pthread_mutex_lock(&affinity_mutex);
	if (ret) {
		perror("Error in pthread mutex lock");
		exit(-1);
	}
	cpu = cpu_affinities[next_aff++];
	ret = pthread_mutex_unlock(&affinity_mutex);
	if (ret) {
		perror("Error in pthread mutex unlock");
		exit(-1);
	}

	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	sched_setaffinity(0, sizeof(mask), &mask);
#endif /* HAVE_SCHED_SETAFFINITY */
}

static DEFINE_URCU_TLS(unsigned long long, nr_writes);
static DEFINE_URCU_TLS(unsigned long long, nr_reads);

static unsigned int nr_readers;
static unsigned int nr_writers;

pthread_mutex_t rcu_copy_mutex = PTHREAD_MUTEX_INITIALIZER;

static
void *thr_reader(void *_count)
{
	unsigned long long *count = _count;

	printf_verbose("thread_begin %s, tid %lu\n",
			"reader", urcu_get_thread_id());

	set_affinity();

	wait_until_go();

	for (;;) {
		int a, ret;

		ret = pthread_rwlock_rdlock(&lock);
		if (ret) {
			fprintf(stderr, "reader pthread_rwlock_rdlock: %s\n", strerror(ret));
			abort();
		}

		a = test_array.a;
		urcu_posix_assert(a == 8);
		if (caa_unlikely(rduration))
			loop_sleep(rduration);

		ret = pthread_rwlock_unlock(&lock);
		if (ret) {
			fprintf(stderr, "reader pthread_rwlock_unlock: %s\n", strerror(ret));
			abort();
		}

		URCU_TLS(nr_reads)++;
		if (caa_unlikely(!test_duration_read()))
			break;
	}

	*count = URCU_TLS(nr_reads);

	printf_verbose("thread_end %s, tid %lu, count %llu\n",
			"reader", urcu_get_thread_id(), *count);
	return ((void*)1);

}

static
void *thr_writer(void *_count)
{
	unsigned long long *count = _count;

	printf_verbose("thread_begin %s, tid %lu\n",
			"writer", urcu_get_thread_id());

	set_affinity();

	wait_until_go();

	for (;;) {
		int ret;

		ret = pthread_rwlock_wrlock(&lock);
		if (ret) {
			fprintf(stderr, "writer pthread_rwlock_wrlock: %s\n", strerror(ret));
			abort();
		}

		test_array.a = 0;
		test_array.a = 8;
		if (caa_unlikely(wduration))
			loop_sleep(wduration);

		ret = pthread_rwlock_unlock(&lock);
		if (ret) {
			fprintf(stderr, "writer pthread_rwlock_unlock: %s\n", strerror(ret));
			abort();
		}

		URCU_TLS(nr_writes)++;
		if (caa_unlikely(!test_duration_write()))
			break;
		if (caa_unlikely(wdelay))
			loop_sleep(wdelay);
	}
	*count = URCU_TLS(nr_writes);

	printf_verbose("thread_end %s, tid %lu, count %llu\n",
			"writer", urcu_get_thread_id(), *count);
	return ((void*)2);
}

static
void show_usage(char **argv)
{
	printf("Usage : %s nr_readers nr_writers duration (s) <OPTIONS>\n",
		argv[0]);
	printf("OPTIONS:\n");
	printf("	[-d delay] (writer period (us))\n");
	printf("	[-c duration] (reader C.S. duration (in loops))\n");
	printf("	[-e duration] (writer C.S. duration (in loops))\n");
	printf("	[-v] (verbose output)\n");
	printf("	[-a cpu#] [-a cpu#]... (affinity)\n");
	printf("\n");
}

int main(int argc, char **argv)
{
	int err;
	pthread_t *tid_reader, *tid_writer;
	void *tret;
	unsigned long long *count_reader, *count_writer;
	unsigned long long tot_reads = 0, tot_writes = 0;
	int i, a;
	unsigned int i_thr;

	if (argc < 4) {
		show_usage(argv);
		return -1;
	}
	cmm_smp_mb();

	err = sscanf(argv[1], "%u", &nr_readers);
	if (err != 1) {
		show_usage(argv);
		return -1;
	}

	err = sscanf(argv[2], "%u", &nr_writers);
	if (err != 1) {
		show_usage(argv);
		return -1;
	}
	
	err = sscanf(argv[3], "%lu", &duration);
	if (err != 1) {
		show_usage(argv);
		return -1;
	}

	for (i = 4; i < argc; i++) {
		if (argv[i][0] != '-')
			continue;
		switch (argv[i][1]) {
		case 'a':
			if (argc < i + 2) {
				show_usage(argv);
				return -1;
			}
			a = atoi(argv[++i]);
			cpu_affinities[next_aff++] = a;
			use_affinity = 1;
			printf_verbose("Adding CPU %d affinity\n", a);
			break;
		case 'c':
			if (argc < i + 2) {
				show_usage(argv);
				return -1;
			}
			rduration = atol(argv[++i]);
			break;
		case 'd':
			if (argc < i + 2) {
				show_usage(argv);
				return -1;
			}
			wdelay = atol(argv[++i]);
			break;
		case 'e':
			if (argc < i + 2) {
				show_usage(argv);
				return -1;
			}
			wduration = atol(argv[++i]);
			break;
		case 'v':
			verbose_mode = 1;
			break;
		}
	}

	printf_verbose("running test for %lu seconds, %u readers, %u writers.\n",
		duration, nr_readers, nr_writers);
	printf_verbose("Writer delay : %lu loops.\n", wdelay);
	printf_verbose("Writer duration : %lu loops.\n", wduration);
	printf_verbose("Reader duration : %lu loops.\n", rduration);
	printf_verbose("thread %-6s, tid %lu\n",
			"main", urcu_get_thread_id());

	err = pthread_rwlock_init(&lock, NULL);
	if (err != 0) {
		fprintf(stderr, "pthread_rwlock_init: (%d) %s\n", err, strerror(err));
		exit(1);
	}

	tid_reader = calloc(nr_readers, sizeof(*tid_reader));
	tid_writer = calloc(nr_writers, sizeof(*tid_writer));
	count_reader = calloc(nr_readers, sizeof(*count_reader));
	count_writer = calloc(nr_writers, sizeof(*count_writer));

	next_aff = 0;

	for (i_thr = 0; i_thr < nr_readers; i_thr++) {
		err = pthread_create(&tid_reader[i_thr], NULL, thr_reader,
				     &count_reader[i_thr]);
		if (err != 0)
			exit(1);
	}
	for (i_thr = 0; i_thr < nr_writers; i_thr++) {
		err = pthread_create(&tid_writer[i_thr], NULL, thr_writer,
				     &count_writer[i_thr]);
		if (err != 0)
			exit(1);
	}

	test_for(duration);

	for (i_thr = 0; i_thr < nr_readers; i_thr++) {
		err = pthread_join(tid_reader[i_thr], &tret);
		if (err != 0)
			exit(1);
		tot_reads += count_reader[i_thr];
	}
	for (i_thr = 0; i_thr < nr_writers; i_thr++) {
		err = pthread_join(tid_writer[i_thr], &tret);
		if (err != 0)
			exit(1);
		tot_writes += count_writer[i_thr];
	}

	printf_verbose("total number of reads : %llu, writes %llu\n", tot_reads,
	       tot_writes);
	printf("SUMMARY %-25s testdur %4lu nr_readers %3u rdur %6lu wdur %6lu "
		"nr_writers %3u "
		"wdelay %6lu nr_reads %12llu nr_writes %12llu nr_ops %12llu\n",
		argv[0], duration, nr_readers, rduration, wduration,
		nr_writers, wdelay, tot_reads, tot_writes,
		tot_reads + tot_writes);

	err = pthread_rwlock_destroy(&lock);
	if (err != 0) {
		fprintf(stderr, "pthread_rwlock_destroy: (%d) %s\n", err, strerror(err));
		exit(1);
	}
	free(tid_reader);
	free(tid_writer);
	free(count_reader);
	free(count_writer);
	return 0;
}
