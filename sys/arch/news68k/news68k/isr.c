/*	$NetBSD: isr.c,v 1.3 2000/06/29 08:02:52 mrg Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, Gordon W. Ross, and Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * from mvme68k/mvme68k/isr.c and sun3/sun3/isr.c
 * This should be in /sys/arch/m68k/m68k?
 */

/*
 * Link and dispatch interrupts.
 */

#include "opt_inet.h"
#include "opt_atalk.h"
#include "opt_ccitt.h"
#include "opt_iso.h"
#include "opt_ns.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <machine/cpu.h>

#include <news68k/news68k/isr.h>

isr_autovec_list_t isr_autovec[NISRAUTOVEC];
struct	isr_vectored isr_vectored[NISRVECTORED];

void set_vector_entry __P((int, void *));
void * get_vector_entry __P((int));

void
isrinit()
{
	int i;

	/* Initialize the autovector lists. */
	for (i = 0; i < NISRAUTOVEC; ++i) {
		LIST_INIT(&isr_autovec[i]);
	}
}

/*
 * Establish an autovectored interrupt handler.
 * Called by driver attach functions.
 */
void
isrlink_autovec(func, arg, ipl, priority)
	int (*func) __P((void *));
	void *arg;
	int ipl;
	int priority;
{
	struct isr_autovec *newisr, *curisr;
	isr_autovec_list_t *list;

	if ((ipl < 0) || (ipl >= NISRAUTOVEC))
		panic("isrlink_autovec: bad ipl %d", ipl);

	newisr = (struct isr_autovec *)malloc(sizeof(struct isr_autovec),
	    M_DEVBUF, M_NOWAIT);
	if (newisr == NULL)
		panic("isrlink_autovec: can't allocate space for isr");

	/* Fill in the new entry. */
	newisr->isr_func = func;
	newisr->isr_arg = arg;
	newisr->isr_ipl = ipl;
	newisr->isr_priority = priority;

	/*
	 * Some devices are particularly sensitive to interrupt
	 * handling latency.  The SCC, for example, can lose many
	 * characters if its interrupt isn't handled with reasonable
	 * speed.
	 *
	 * To work around this problem, each device can give itself a
	 * "priority".  An unbuffered SCC would give itself a higher
	 * priority than a SCSI device, for example.
	 *
	 * This solution was originally developed for the hp300, which
	 * has a flat spl scheme (by necessity).  Thankfully, the
	 * MVME systems don't have this problem, though this may serve
	 * a useful purpose in any case.
	 */

	/*
	 * Get the appropriate ISR list.  If the list is empty, no
	 * additional work is necessary; we simply insert ourselves
	 * at the head of the list.
	 */
	list = &isr_autovec[ipl];
	if (list->lh_first == NULL) {
		LIST_INSERT_HEAD(list, newisr, isr_link);
		return;
	}

	/*
	 * A little extra work is required.  We traverse the list
	 * and place ourselves after any ISRs with our current (or
	 * higher) priority.
	 */
	for (curisr = list->lh_first; curisr->isr_link.le_next != NULL;
	    curisr = curisr->isr_link.le_next) {
		if (newisr->isr_priority > curisr->isr_priority) {
			LIST_INSERT_BEFORE(curisr, newisr, isr_link);
			return;
		}
	}

	/*
	 * We're the least important entry, it seems.  We just go
	 * on the end.
	 */
	LIST_INSERT_AFTER(curisr, newisr, isr_link);
}

/*
 * Establish a vectored interrupt handler.
 * Called by bus interrupt establish functions.
 */
void
isrlink_vectored(func, arg, ipl, vec)
	int (*func) __P((void *));
	void *arg;
	int ipl, vec;
{
	struct isr_vectored *isr;

	if ((ipl < 0) || (ipl >= NISRAUTOVEC))
		panic("isrlink_vectored: bad ipl %d", ipl);
	if ((vec < ISRVECTORED) || (vec >= ISRVECTORED + NISRVECTORED))
		panic("isrlink_vectored: bad vec 0x%x", vec);

	isr = &isr_vectored[vec - ISRVECTORED];

	if ((vectab[vec] != badtrap) || (isr->isr_func != NULL))
		panic("isrlink_vectored: vec 0x%x not available", vec);

	/* Fill in the new entry. */
	isr->isr_func = func;
	isr->isr_arg = arg;
	isr->isr_ipl = ipl;

	/* Hook into the vector table. */
	vectab[vec] = intrhand_vectored;
}

/*
 * Unhook a vectored interrupt.
 */
void
isrunlink_vectored(vec)
	int vec;
{

	if ((vec < ISRVECTORED) || (vec >= ISRVECTORED + NISRVECTORED))
		panic("isrunlink_vectored: bad vec 0x%x", vec);

	if (vectab[vec] != intrhand_vectored)
		panic("isrunlink_vectored: not vectored interrupt");

	vectab[vec] = badtrap;
	bzero(&isr_vectored[vec - ISRVECTORED], sizeof(struct isr_vectored));
}

/*
 * This is the dispatcher called by the low-level
 * assembly language autovectored interrupt routine.
 */
void
isrdispatch_autovec(evec)
	int evec;		/* format | vector offset */
{
	struct isr_autovec *isr;
	isr_autovec_list_t *list;
	int handled = 0, ipl, vec;
	static int straycount, unexpected;

	vec = (evec & 0xfff) >> 2;
	if ((vec < ISRAUTOVEC) || (vec >= (ISRAUTOVEC + NISRAUTOVEC)))
		panic("isrdispatch_autovec: bad vec 0x%x\n", vec);
	ipl = vec - ISRAUTOVEC;

	intrcnt[ipl]++;
	uvmexp.intrs++;

	list = &isr_autovec[ipl];
	if (list->lh_first == NULL) {
		printf("isrdispatch_autovec: ipl %d unexpected\n", ipl);
		if (++unexpected > 10)
			panic("too many unexpected interrupts");
		return;
	}

	/* Give all the handlers a chance. */
	for (isr = list->lh_first ; isr != NULL; isr = isr->isr_link.le_next)
		handled |= (*isr->isr_func)(isr->isr_arg);

	if (handled)
		straycount = 0;
	else if (++straycount > 50)
		panic("isr_dispatch_autovec: too many stray interrupts");
	else
		printf("isrdispatch_autovec: stray level %d interrupt\n", ipl);
}

/*
 * This is the dispatcher called by the low-level
 * assembly language vectored interrupt routine.
 */
void
isrdispatch_vectored(pc, evec, frame)
	int pc, evec;
	void *frame;
{
	struct isr_vectored *isr;
	int ipl, vec;

	vec = (evec & 0xfff) >> 2;
	ipl = (getsr() >> 8) & 7;

	intrcnt[ipl]++;
	uvmexp.intrs++;

	if ((vec < ISRVECTORED) || (vec >= (ISRVECTORED + NISRVECTORED)))
		panic("isrdispatch_vectored: bad vec 0x%x\n", vec);
	isr = &isr_vectored[vec - ISRVECTORED];

	if (isr->isr_func == NULL) {
		printf("isrdispatch_vectored: no handler for vec 0x%x\n", vec);
		vectab[vec] = badtrap;
		return;
	}

	/*
	 * Handler gets exception frame if argument is NULL.
	 */
	if ((*isr->isr_func)(isr->isr_arg ? isr->isr_arg : frame) == 0)
		printf("isrdispatch_vectored: vec 0x%x not claimed\n", vec);
}

void
isrlink_custom(level, handler)
	int level;
	void *handler;
{
	set_vector_entry(ISRAUTOVEC + level, handler);
}

/*
 * XXX - could just kill these... [from sun3]
 */
void
set_vector_entry(entry, handler)
	int entry;
	void *handler;
{
	if ((entry < 0) || (entry >= NVECTORS))
		panic("set_vector_entry: setting vector too high or low\n");
	vectab[entry] = handler;
}

void *
get_vector_entry(entry)
	int entry;
{
	if ((entry < 0) || (entry >= NVECTORS))
		panic("get_vector_entry: setting vector too high or low\n");
	return ((void *) vectab[entry]);
}

/*
 * XXX Why on earth isn't this in a common file?!
 */

/*
 * Declarations for the netisr functions...
 * They are in the header files, but that's not
 * really a good reason to drag all those in.
 */
void netintr __P((void));
void arpintr __P((void));
void ipintr __P((void));
void ip6intr __P((void));
void atintr __P((void));
void nsintr __P((void));
void clnlintr __P((void));
void ccittintr __P((void));
void pppintr __P((void));

void
netintr()
{

#define DONETISR(bit, fn) do {		\
	if (netisr & (1 << bit)) {	\
		netisr &= ~(1 << bit);	\
		fn();			\
	}				\
} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR

}
