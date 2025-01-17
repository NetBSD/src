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

struct per_thread_lock {
	pthread_mutex_t lock;
} __attribute__((aligned(CAA_CACHE_LINE_SIZE)));	/* cache-line aligned */

static struct per_thread_lock *per_thread_lock;

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
		errno = ret;
		perror("Error in pthread mutex lock");
		abort();
	}
	cpu = cpu_affinities[next_aff++];
	ret = pthread_mutex_unlock(&affinity_mutex);
	if (ret) {
		errno = ret;
		perror("Error in pthread mutex unlock");
		abort();
	}
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	sched_setaffinity(0, sizeof(mask), &mask);
#endif /* HAVE_SCHED_SETAFFINITY */
}

static DEFINE_URCU_TLS(unsigned long long, nr_writes);
static DEFINE_URCU_TLS(unsigned long long, nr_reads);

static
unsigned long long __attribute__((aligned(CAA_CACHE_LINE_SIZE))) *tot_nr_writes;
static
unsigned long long __attribute__((aligned(CAA_CACHE_LINE_SIZE))) *tot_nr_reads;

static unsigned int nr_readers;
static unsigned int nr_writers;

static void urcu_mutex_lock(pthread_mutex_t *lock)
{
	int ret;

	ret = pthread_mutex_lock(lock);
	if (ret) {
		errno = ret;
		perror("Error in pthread mutex lock");
		abort();
	}
}

static void urcu_mutex_unlock(pthread_mutex_t *lock)
{
	int ret;

	ret = pthread_mutex_unlock(lock);
	if (ret) {
		errno = ret;
		perror("Error in pthread mutex unlock");
		abort();
	}
}

static
void *thr_reader(void *data)
{
	unsigned long tidx = (unsigned long)data;

	printf_verbose("thread_begin %s, tid %lu\n",
			"reader", urcu_get_thread_id());

	set_affinity();

	wait_until_go();

	for (;;) {
		int v;

		urcu_mutex_lock(&per_thread_lock[tidx].lock);
		v = test_array.a;
		urcu_posix_assert(v == 8);
		if (caa_unlikely(rduration))
			loop_sleep(rduration);
		urcu_mutex_unlock(&per_thread_lock[tidx].lock);
		URCU_TLS(nr_reads)++;
		if (caa_unlikely(!test_duration_read()))
			break;
	}

	tot_nr_reads[tidx] = URCU_TLS(nr_reads);
	printf_verbose("thread_end %s, tid %lu\n",
			"reader", urcu_get_thread_id());
	return ((void*)1);

}

static
void *thr_writer(void *data)
{
	unsigned long wtidx = (unsigned long)data;
	long tidx;

	printf_verbose("thread_begin %s, tid %lu\n",
			"writer", urcu_get_thread_id());

	set_affinity();

	wait_until_go();

	for (;;) {
		for (tidx = 0; tidx < (long)nr_readers; tidx++) {
			urcu_mutex_lock(&per_thread_lock[tidx].lock);
		}
		test_array.a = 0;
		test_array.a = 8;
		if (caa_unlikely(wduration))
			loop_sleep(wduration);
		for (tidx = (long)nr_readers - 1; tidx >= 0; tidx--) {
			urcu_mutex_unlock(&per_thread_lock[tidx].lock);
		}
		URCU_TLS(nr_writes)++;
		if (caa_unlikely(!test_duration_write()))
			break;
		if (caa_unlikely(wdelay))
			loop_sleep(wdelay);
	}

	printf_verbose("thread_end %s, tid %lu\n",
			"writer", urcu_get_thread_id());
	tot_nr_writes[wtidx] = URCU_TLS(nr_writes);
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
	printf_verbose("Reader duration : %lu loops.\n", rduration);
	printf_verbose("thread %-6s, tid %lu\n",
			"main", urcu_get_thread_id());

	tid_reader = calloc(nr_readers, sizeof(*tid_reader));
	tid_writer = calloc(nr_writers, sizeof(*tid_writer));
	tot_nr_reads = calloc(nr_readers, sizeof(*tot_nr_reads));
	tot_nr_writes = calloc(nr_writers, sizeof(*tot_nr_writes));
	per_thread_lock = calloc(nr_readers, sizeof(*per_thread_lock));
	for (i_thr = 0; i_thr < nr_readers; i_thr++) {
		pthread_mutex_init(&per_thread_lock[i_thr].lock, NULL);
	}

	next_aff = 0;

	for (i_thr = 0; i_thr < nr_readers; i_thr++) {
		err = pthread_create(&tid_reader[i_thr], NULL, thr_reader,
				     (void *)(long)i_thr);
		if (err != 0)
			exit(1);
	}
	for (i_thr = 0; i_thr < nr_writers; i_thr++) {
		err = pthread_create(&tid_writer[i_thr], NULL, thr_writer,
				     (void *)(long)i_thr);
		if (err != 0)
			exit(1);
	}

	test_for(duration);

	for (i_thr = 0; i_thr < nr_readers; i_thr++) {
		err = pthread_join(tid_reader[i_thr], &tret);
		if (err != 0)
			exit(1);
		tot_reads += tot_nr_reads[i_thr];
	}
	for (i_thr = 0; i_thr < nr_writers; i_thr++) {
		err = pthread_join(tid_writer[i_thr], &tret);
		if (err != 0)
			exit(1);
		tot_writes += tot_nr_writes[i_thr];
	}

	printf_verbose("total number of reads : %llu, writes %llu\n", tot_reads,
	       tot_writes);
	printf("SUMMARY %-25s testdur %4lu nr_readers %3u rdur %6lu wdur %6lu "
		"nr_writers %3u "
		"wdelay %6lu nr_reads %12llu nr_writes %12llu nr_ops %12llu\n",
		argv[0], duration, nr_readers, rduration, wduration,
		nr_writers, wdelay, tot_reads, tot_writes,
		tot_reads + tot_writes);

	free(tid_reader);
	free(tid_writer);
	free(tot_nr_reads);
	free(tot_nr_writes);
	free(per_thread_lock);
	return 0;
}
