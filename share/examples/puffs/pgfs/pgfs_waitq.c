/*	$NetBSD: pgfs_waitq.c,v 1.1 2011/10/12 01:05:00 yamt Exp $	*/

/*-
 * Copyright (c)2010,2011 YAMAMOTO Takashi,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * a dumb wait queue on puffs_cc
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: pgfs_waitq.c,v 1.1 2011/10/12 01:05:00 yamt Exp $");
#endif /* not lint */

#include <assert.h>
#include <puffs.h>

#include "pgfs_waitq.h"
#include "pgfs_debug.h"

struct waiter {
	TAILQ_ENTRY(waiter) list;
	struct puffs_cc *cc;
};

void
waitq_init(struct waitq *wq)
{

	TAILQ_INIT(wq);
}

void
waiton(struct waitq *wq, struct puffs_cc *cc)
{
	struct waiter w;

	/*
	 * insert to the tail of the queue.
	 * some users rely on the FIFO behaviour.
	 */
	w.cc = cc;
	TAILQ_INSERT_TAIL(wq, &w, list);
	puffs_cc_yield(cc);
}

struct puffs_cc *
wakeup_one(struct waitq *wq)
{

	struct waiter *w;

	w = TAILQ_FIRST(wq);
	if (w != NULL) {
		TAILQ_REMOVE(wq, w, list);
		puffs_cc_schedule(w->cc);
		DPRINTF("waking up %p\n", w->cc);
		return w->cc;
	}
	return NULL;
}

#if 0
static void
wakeup(struct waitq *wq)
{

	struct waiter *w;

	TAILQ_FOREACH(w, wq, list) {
		puffs_cc_schedule(w->cc);
		DPRINTF("waking up %p\n", w->cc);
	}
	TAILQ_INIT(wq);
}
#endif
