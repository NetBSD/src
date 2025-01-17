// SPDX-FileCopyrightText: 2023 Olivier Dion <odion@efficios.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * test_wfstack.c
 *
 * Userspace RCU library - test wftack race conditions
 */

#define _LGPL_SOURCE

#include <stdlib.h>

#include <pthread.h>

#include <urcu/wfstack.h>

#include "tap.h"

#define NR_TESTS 1
#define NR_PRODUCERS 4
#define LOOP 100

static void async_run(struct cds_wfs_stack *queue)
{
	struct cds_wfs_node *node = malloc(sizeof(*node));

	cds_wfs_node_init(node);

	cds_wfs_push(queue, node);
}

static void *async_loop(void *queue)
{
	size_t k = 0;

	while (k < LOOP * NR_PRODUCERS) {
		free(cds_wfs_pop_blocking(queue));
		++k;
	}

	return NULL;
}

static void *spawn_jobs(void *queue)
{
	for (size_t k = 0; k < LOOP; ++k) {
		async_run(queue);
	}

	return 0;
}

int main(void)
{
	pthread_t consumer;
	pthread_t producers[NR_PRODUCERS];
	struct cds_wfs_stack queue;

	plan_tests(NR_TESTS);

	cds_wfs_init(&queue);
	pthread_create(&consumer, NULL, async_loop, &queue);

	for (size_t k = 0; k < NR_PRODUCERS; ++k) {
		pthread_create(&producers[k], NULL, spawn_jobs, &queue);
	}

	pthread_join(consumer, NULL);
	for (size_t k = 0; k < NR_PRODUCERS; ++k) {
		pthread_join(producers[k], NULL);
	}

	ok1("No race conditions");

	return exit_status();
}
