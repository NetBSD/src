/*	$NetBSD: job.c,v 1.2 2010/05/06 18:53:17 christos Exp $	*/

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
__RCSID("$NetBSD: job.c,v 1.2 2010/05/06 18:53:17 christos Exp $");
#endif
#endif

#include "cron.h"

typedef	struct _job {
	struct _job	*next;
	entry		*e;
	user		*u;
	time_t		t;
} job;

static job	*jhead = NULL, *jtail = NULL;

static int okay_to_go(job *);

void
job_add(entry *e, user *u, time_t target_time) {
	job *j;

	/* if already on queue, keep going */
	for (j = jhead; j != NULL; j = j->next)
		if (j->e == e && j->u == u) {
			j->t = target_time;
			return;
		}

	/* build a job queue element */
	if ((j = malloc(sizeof(*j))) == NULL)
		return;
	j->next = NULL;
	j->e = e;
	j->u = u;
	j->t = target_time;

	/* add it to the tail */
	if (jhead == NULL)
		jhead = j;
	else
		jtail->next = j;
	jtail = j;
}

int
job_runqueue(void) {
	job *j, *jn;
	int run = 0;

	for (j = jhead; j; j = jn) {
		if (okay_to_go(j))
			do_command(j->e, j->u);
		else {
			char *x = mkprints(j->e->cmd, strlen(j->e->cmd));
			char *usernm = env_get("LOGNAME", j->e->envp);

			log_it(usernm, getpid(), "CMD (skipped)", x);
			free(x);
		}
		jn = j->next;
		free(j);
		run++;
	}
	jhead = jtail = NULL;
	return (run);
}


static int
okay_to_go(job *j)
{
	char *within, *t;
	long delta;

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
