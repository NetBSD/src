/*	$NetBSD: job.c,v 1.2.48.1 2020/04/21 18:41:55 martin Exp $	*/

/* Copyright 1988,1990,1993,1994 by Paul Vixie
 * All rights reserved
 */

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1997,2000 by Internet Software Consortium, Inc.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <sys/cdefs.h>

#if !defined(lint) && !defined(LINT)
#if 0
static char rcsid[] = "Id: job.c,v 1.6 2004/01/23 18:56:43 vixie Exp";
#else
__RCSID("$NetBSD: job.c,v 1.2.48.1 2020/04/21 18:41:55 martin Exp $");
#endif
#endif

#include "cron.h"

typedef	struct _job {
	SIMPLEQ_ENTRY(_job) entries;
	entry		*e;
	user		*u;
	time_t		t;
	pid_t		pid;
} job;

static SIMPLEQ_HEAD(job_queue, _job) jobs = SIMPLEQ_HEAD_INITIALIZER(jobs);

static int okay_to_go(job *);

void
job_add(entry *e, user *u, time_t target_time) {
	job *j;

	/* if already on queue, keep going */
	SIMPLEQ_FOREACH(j, &jobs, entries) {
		if (j->e == e && j->u == u) {
			j->t = target_time;
			return;
		}
	}

	/* build a job queue element */
	if ((j = calloc(1, sizeof(*j))) == NULL)
		return;
	j->e = e;
	j->u = u;
	j->t = target_time;
	j->pid = -1;

	/* add it to the tail */
	SIMPLEQ_INSERT_TAIL(&jobs, j, entries);
}

void
job_remove(entry *e, user *u)
{
	job *j, *prev = NULL;

	SIMPLEQ_FOREACH(j, &jobs, entries) {
		if (j->e == e && j->u == u) {
			if (prev == NULL)
				SIMPLEQ_REMOVE_HEAD(&jobs, entries);
			else
				SIMPLEQ_REMOVE_AFTER(&jobs, prev, entries);
			free(j);
			break;
		}
		prev = j;
	}
}

void
job_exit(pid_t jobpid)
{
	job *j, *prev = NULL;

	/* If a singleton exited, remove and free it. */
	SIMPLEQ_FOREACH(j, &jobs, entries) {
		if (jobpid == j->pid) {
			if (prev == NULL)
				SIMPLEQ_REMOVE_HEAD(&jobs, entries);
			else
				SIMPLEQ_REMOVE_AFTER(&jobs, prev, entries);
			free(j);
			break;
		}
		prev = j;
	}
}

int
job_runqueue(void) {
	struct job_queue singletons = SIMPLEQ_HEAD_INITIALIZER(singletons);
	job *j;
	int run = 0;

	while ((j = SIMPLEQ_FIRST(&jobs))) {
		SIMPLEQ_REMOVE_HEAD(&jobs, entries);
		if (okay_to_go(j)) {
			j->pid = do_command(j->e, j->u);
			run++;
		} else {
			char *x = mkprints(j->e->cmd, strlen(j->e->cmd));
			char *usernm = env_get("LOGNAME", j->e->envp);

			log_it(usernm, getpid(), "CMD (skipped)", x);
			free(x);
		}
		if (j->pid != -1)
			SIMPLEQ_INSERT_TAIL(&singletons, j, entries);
		else
			free(j);
	}
	SIMPLEQ_CONCAT(&jobs, &singletons);
	return (run);
}


static int
okay_to_go(job *j)
{
	char *within, *t;
	long delta;

	if (j->pid != -1)
		return 0;

	if (j->e->flags & WHEN_REBOOT)
		return (1);

	within = env_get("CRON_WITHIN", j->e->envp);
	if (within == NULL)
		return (1);

	/* XXX handle 2m, 4h, etc? */
	errno = 0;
	delta = strtol(within, &t, 10);
	if (errno == ERANGE || *t != '\0' || delta <= 0)
		return (1);

	return ((j->t + delta) > time(NULL));
}
