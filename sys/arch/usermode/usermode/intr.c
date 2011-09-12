/* $NetBSD: intr.c,v 1.5 2011/09/12 12:24:34 reinoud Exp $ */

/*-
 * Copyright (c) 2011 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.5 2011/09/12 12:24:34 reinoud Exp $");

#include <sys/types.h>

#include <machine/intr.h>
#include <machine/thunk.h>

//#define INTR_USE_SIGPROCMASK

#define MAX_QUEUED_EVENTS 64

static int usermode_x = IPL_NONE;

#ifdef INTR_USE_SIGPROCMASK
static bool block_sigalrm = false;
#endif


struct spl_intr_event {
	void (*func)(void *);
	void *arg;
};

struct spl_intr_event spl_intrs[IPL_HIGH+1][MAX_QUEUED_EVENTS];
int spl_intr_wr[IPL_HIGH+1];
int spl_intr_rd[IPL_HIGH+1];

void
splinit(void)
{
	int i;
	for (i = 0; i <= IPL_HIGH; i++) {
		spl_intr_rd[i] = 1;
		spl_intr_wr[i] = 1;
	}
}

void
spl_intr(int x, void (*func)(void *), void *arg)
{
	struct spl_intr_event *spli;

	if (x >= usermode_x) {
		func(arg);
		return;
	}

	//printf("\nX : %d\n", x);
	spli = &spl_intrs[x][spl_intr_wr[x]];
	spli->func = func;
	spli->arg = arg;

	spl_intr_wr[x] = (spl_intr_wr[x] + 1) % MAX_QUEUED_EVENTS;
	if (spl_intr_wr[x] == spl_intr_rd[x])
		panic("%s: spl list %d full!\n", __func__, x);
}

int
splraise(int x)
{
	int oldx = usermode_x;

	if (x > usermode_x) {
		usermode_x = x;
	}

#ifdef INTR_USE_SIGPROCMASK
	if (x >= IPL_SCHED && !block_sigalrm) {
		thunk_sigblock(SIGALRM);
		block_sigalrm = true;
	}
#endif

	return oldx;
}

void
spllower(int x)
{
	struct spl_intr_event *spli;
	int y;

	/* `eat' interrupts that came by until we got back to x */
	if (usermode_x > x) {
		for (y = usermode_x; y >= x; y--) {
			while (spl_intr_rd[y] != spl_intr_wr[y]) {
				// printf("spl y %d firing\n", y);
				spli = &spl_intrs[y][spl_intr_rd[y]];
				if (!spli->func)
					panic("%s: spli->func is NULL for ipl %d, rd %d, wr %d\n",
						__func__, y, spl_intr_rd[y], spl_intr_wr[y]);
				spli->func(spli->arg);
				spl_intr_rd[y] = (spl_intr_rd[y] + 1) % MAX_QUEUED_EVENTS;
			}
		}
		usermode_x = x;
	}

#ifdef INTR_USE_SIGPROCMASK
	if (x < IPL_SCHED && block_sigalrm) {
		thunk_sigunblock(SIGALRM);
		block_sigalrm = false;
	}
#endif
}
