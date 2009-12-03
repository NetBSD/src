/*	$NetBSD: intr.c,v 1.62 2009/12/03 05:06:16 mrg Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT OT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)intr.c	8.3 (Berkeley) 11/11/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.62 2009/12/03 05:06:16 mrg Exp $");

#include "opt_ddb.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/ctlreg.h>
#include <machine/instr.h>
#include <machine/trap.h>

/*
 * The following array is to used by locore.s to map interrupt packets
 * to the proper IPL to send ourselves a softint.  It should be filled
 * in as the devices are probed.  We should eventually change this to a
 * vector table and call these things directly.
 */
struct intrhand *intrlev[MAXINTNUM];

void	strayintr(const struct trapframe64 *, int);
int	intr_list_handler(void *);

/*
 * Stray interrupt handler.  Clear it if possible.
 * If not, and if we get 10 interrupts in 10 seconds, panic.
 */
int ignore_stray = 1;
int straycnt[16];

void
strayintr(const struct trapframe64 *fp, int vectored)
{
	static int straytime, nstray;
	int timesince;
	char buf[256];
#if 0
	extern int swallow_zsintrs;
#endif

	if (fp->tf_pil < 16)
		straycnt[(int)fp->tf_pil]++;

	if (ignore_stray)
		return;

	/* If we're in polled mode ignore spurious interrupts */
	if ((fp->tf_pil == PIL_SER) /* && swallow_zsintrs */) return;

	snprintb(buf, sizeof(buf), PSTATE_BITS,
	    (fp->tf_tstate>>TSTATE_PSTATE_SHIFT));
	printf("stray interrupt ipl %u pc=%llx npc=%llx pstate=%s vecttored=%d\n",
	    fp->tf_pil, (unsigned long long)fp->tf_pc,
	    (unsigned long long)fp->tf_npc,  buf, vectored);

	timesince = time_second - straytime;
	if (timesince <= 10) {
		if (++nstray > 500)
			panic("crazy interrupts");
	} else {
		straytime = time_second;
		nstray = 1;
	}
#ifdef DDB
	Debugger();
#endif
}

/*
 * PCI devices can share interrupts so we need to have
 * a handler to hand out interrupts.
 */
int
intr_list_handler(void *arg)
{
	int claimed = 0;
	struct intrhand *ih = (struct intrhand *)arg;

	if (!arg) panic("intr_list_handler: no handlers!");
	while (ih && !claimed) {
		claimed = (*ih->ih_fun)(ih->ih_arg);
#ifdef DEBUG
		{
			extern int intrdebug;
			if (intrdebug & 1)
				printf("intr %p %x arg %p %s\n",
					ih, ih->ih_number, ih->ih_arg,
					claimed ? "claimed" : "");
		}
#endif
		ih = ih->ih_next;
	}
	return (claimed);
}

#ifdef MULTIPROCESSOR
static int intr_biglock_wrapper(void *);

static int
intr_biglock_wrapper(void *vp)
{
	struct intrhand *ih = vp;
	int ret;

	KERNEL_LOCK(1, NULL);
	ret = (*ih->ih_realfun)(ih->ih_realarg);
	KERNEL_UNLOCK_ONE(NULL);

	return ret;
}
#endif

/*
 * Attach an interrupt handler to the vector chain for the given level.
 * This is not possible if it has been taken away as a fast vector.
 */
void
intr_establish(int level, bool mpsafe, struct intrhand *ih)
{
	struct intrhand *q = NULL;
	int s;

	/*
	 * This is O(N^2) for long chains, but chains are never long
	 * and we do want to preserve order.
	 */
	ih->ih_pil = level; /* XXXX caller should have done this before */
	ih->ih_pending = 0; /* XXXX caller should have done this before */
	ih->ih_next = NULL;

#ifdef MULTIPROCESSOR
	if (!mpsafe) {
		ih->ih_realarg = ih->ih_arg;
		ih->ih_realfun = ih->ih_fun;
		ih->ih_arg = ih;
		ih->ih_fun = intr_biglock_wrapper;
	}
#endif

	s = splhigh();
	/*
	 * Store in fast lookup table
	 */
#ifdef NOT_DEBUG
	if (!ih->ih_number) {
		printf("\nintr_establish: NULL vector fun %p arg %p pil %p\n",
			  ih->ih_fun, ih->ih_arg, ih->ih_number, ih->ih_pil);
		Debugger();
	}
#endif
	if (ih->ih_number < MAXINTNUM && ih->ih_number >= 0) {
		if ((q = intrlev[ih->ih_number])) {
			struct intrhand *nih;
			/*
			 * Interrupt is already there.  We need to create a
			 * new interrupt handler and interpose it.
			 */
#ifdef NOT_DEBUG
			printf("intr_establish: intr reused %x\n", 
				ih->ih_number);
#endif
			if (q->ih_fun != intr_list_handler) {
				nih = (struct intrhand *)
					malloc(sizeof(struct intrhand),
						M_DEVBUF, M_NOWAIT);
				/* Point the old IH at the new handler */
				*nih = *q;
				nih->ih_next = NULL;
				q->ih_arg = (void *)nih;
				q->ih_fun = intr_list_handler;
			}
			/* Add the ih to the head of the list */
			ih->ih_next = (struct intrhand *)q->ih_arg;
			q->ih_arg = (void *)ih;
		} else {
			intrlev[ih->ih_number] = ih;
		}
#ifdef NOT_DEBUG
		printf("\nintr_establish: vector %x pil %x mapintr %p "
			"clrintr %p fun %p arg %p\n",
			ih->ih_number, ih->ih_pil, (void *)ih->ih_map,
			(void *)ih->ih_clr, (void *)ih->ih_fun,
			(void *)ih->ih_arg);
		/*Debugger();*/
#endif
	} else
		panic("intr_establish: bad intr number %x", ih->ih_number);

	splx(s);
}

/*
 * Prepare an interrupt handler used for send_softint.
 */
void *
sparc_softintr_establish(int pil, int (*fun)(void *), void *arg)
{
	struct intrhand *ih;

	ih = malloc(sizeof(struct intrhand), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (ih == NULL)
		panic("could not allocate softint interrupt handler");

	ih->ih_fun = fun;
	ih->ih_pil = pil;
	ih->ih_arg = arg;
	return ih;
}

void
sparc_softintr_disestablish(void *cookie)
{

	free(cookie, M_DEVBUF);
}

void
sparc_softintr_schedule(void *cookie)
{
	struct intrhand *ih = (struct intrhand *)cookie;

	send_softint(-1, ih->ih_pil, ih);
}
