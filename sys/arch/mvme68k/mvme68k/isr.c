/*	$NetBSD: isr.c,v 1.25 2002/02/12 20:38:38 scw Exp $	*/

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
 * Link and dispatch interrupts.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/vmmeter.h>

#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <machine/cpu.h>

#include <mvme68k/mvme68k/isr.h>

volatile unsigned int interrupt_depth;
isr_autovec_list_t isr_autovec[NISRAUTOVEC];
struct	isr_vectored isr_vectored[NISRVECTORED];
static const char irqgroupname[] = "hard irqs";
struct	evcnt mvme68k_irq_evcnt[] = {
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, irqgroupname, "spur"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, irqgroupname, "lev1"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, irqgroupname, "lev2"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, irqgroupname, "lev3"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, irqgroupname, "lev4"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, irqgroupname, "lev5"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, irqgroupname, "lev6"),
	EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, irqgroupname, "nmi")
};

extern	int intrcnt[];		/* from locore.s. XXXSCW: will go away soon */
extern	void (*vectab[]) __P((void));
extern	void badtrap __P((void));
extern	void intrhand_vectored __P((void));

static	int spurintr __P((void *));


void
isrinit()
{
	int i;

	/* Initialize the autovector lists. */
	for (i = 0; i < NISRAUTOVEC; ++i)
		LIST_INIT(&isr_autovec[i]);

	/* Initialise the interrupt event counts */
	for (i = 0; i < (sizeof(mvme68k_irq_evcnt) / sizeof(struct evcnt)); i++)
		evcnt_attach_static(&mvme68k_irq_evcnt[i]);

	/* Arrange to trap Spurious and NMI auto-vectored Interrupts */
	isrlink_autovec(spurintr, NULL, 0, 0, NULL);
	isrlink_autovec(nmihand, NULL, 7, 0, NULL);
}

/*
 * Establish an autovectored interrupt handler.
 * Called by driver attach functions.
 */
void
isrlink_autovec(func, arg, ipl, priority, evcnt)
	int (*func) __P((void *));
	void *arg;
	int ipl;
	int priority;
	struct evcnt *evcnt;
{
	struct isr_autovec *newisr, *curisr;
	isr_autovec_list_t *list;

#ifdef DIAGNOSTIC
	if ((ipl < 0) || (ipl >= NISRAUTOVEC))
		panic("isrlink_autovec: bad ipl %d", ipl);
#endif

	newisr = (struct isr_autovec *)malloc(sizeof(struct isr_autovec),
	    M_DEVBUF, M_NOWAIT);
	if (newisr == NULL)
		panic("isrlink_autovec: can't allocate space for isr");

	/* Fill in the new entry. */
	newisr->isr_func = func;
	newisr->isr_arg = arg;
	newisr->isr_ipl = ipl;
	newisr->isr_priority = priority;
	newisr->isr_evcnt = evcnt;

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
isrlink_vectored(func, arg, ipl, vec, evcnt)
	int (*func) __P((void *));
	void *arg;
	int ipl, vec;
	struct evcnt *evcnt;
{
	struct isr_vectored *isr;

#ifdef DIAGNOSTIC
	if ((ipl < 0) || (ipl >= NISRAUTOVEC))
		panic("isrlink_vectored: bad ipl %d", ipl);
	if ((vec < ISRVECTORED) || (vec >= ISRVECTORED + NISRVECTORED))
		panic("isrlink_vectored: bad vec 0x%x", vec);
#endif

	isr = &isr_vectored[vec - ISRVECTORED];

#ifdef DIAGNOSTIC
	if ((vectab[vec] != badtrap) || (isr->isr_func != NULL))
		panic("isrlink_vectored: vec 0x%x not available", vec);
#endif

	/* Fill in the new entry. */
	isr->isr_func = func;
	isr->isr_arg = arg;
	isr->isr_ipl = ipl;
	isr->isr_evcnt = evcnt;

	/* Hook into the vector table. */
	vectab[vec] = intrhand_vectored;
}

/*
 * Return a pointer to the evcnt structure for
 * the specified ipl.
 */
struct evcnt *
isrlink_evcnt(ipl)
	int ipl;
{

#ifdef DIAGNOSTIC
	if (ipl < 0 ||
	    ipl >= (sizeof(mvme68k_irq_evcnt) / sizeof(struct evcnt)))
		panic("isrlink_evcnt: bad ipl %d", ipl);
#endif

	return (&mvme68k_irq_evcnt[ipl]);
}

/*
 * Unhook a vectored interrupt.
 */
void
isrunlink_vectored(vec)
	int vec;
{

#ifdef DIAGNOSTIC
	if ((vec < ISRVECTORED) || (vec >= ISRVECTORED + NISRVECTORED))
		panic("isrunlink_vectored: bad vec 0x%x", vec);

	if (vectab[vec] != intrhand_vectored)
		panic("isrunlink_vectored: not vectored interrupt");
#endif

	vectab[vec] = badtrap;
	memset(&isr_vectored[vec - ISRVECTORED], 0, sizeof(struct isr_vectored));
}

/*
 * This is the dispatcher called by the low-level
 * assembly language autovectored interrupt routine.
 */
void
isrdispatch_autovec(frame)
	struct clockframe *frame;
{
	struct isr_autovec *isr;
	isr_autovec_list_t *list;
	int handled, ipl;
	void *arg;
	static int straycount, unexpected;

	ipl = (frame->vec >> 2) - ISRAUTOVEC;

#ifdef DIAGNOSTIC
	if ((ipl < 0) || (ipl >= NISRAUTOVEC))
		panic("isrdispatch_autovec: bad vec 0x%x\n", frame->vec);
#endif

	intrcnt[ipl]++;	/* XXXSCW: Will go away soon */
	mvme68k_irq_evcnt[ipl].ev_count++;
	uvmexp.intrs++;

	list = &isr_autovec[ipl];
	if (list->lh_first == NULL) {
		printf("isrdispatch_autovec: ipl %d unexpected\n", ipl);
		if (++unexpected > 10)
			panic("too many unexpected interrupts");
		return;
	}

	/* Give all the handlers a chance. */
	handled = 0;
	for (isr = list->lh_first ; isr != NULL; isr = isr->isr_link.le_next) {
		arg = isr->isr_arg ? isr->isr_arg : frame;
		if ((*isr->isr_func)(arg) != 0) {
			if (isr->isr_evcnt)
				isr->isr_evcnt->ev_count++;
			handled++;
		}
	}

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
isrdispatch_vectored(ipl, frame)
	int ipl;
	struct clockframe *frame;
{
	struct isr_vectored *isr;
	int vec;

	vec = (frame->vec >> 2) - ISRVECTORED;

#ifdef DIAGNOSTIC
	if ((vec < 0) || (vec >= NISRVECTORED))
		panic("isrdispatch_vectored: bad vec 0x%x\n", frame->vec);
#endif

	isr = &isr_vectored[vec];

	intrcnt[ipl]++;	/* XXXSCW: Will go away soon */
	mvme68k_irq_evcnt[ipl].ev_count++;
	uvmexp.intrs++;

	if (isr->isr_func == NULL) {
		printf("isrdispatch_vectored: no handler for vec 0x%x\n",
		    frame->vec);
		vectab[vec + ISRVECTORED] = badtrap;
		return;
	}

	/*
	 * Handler gets exception frame if argument is NULL.
	 */
	if ((*isr->isr_func)(isr->isr_arg ? isr->isr_arg : frame) == 0)
		printf("isrdispatch_vectored: vec 0x%x not claimed\n",
		    frame->vec);
	else
	if (isr->isr_evcnt)
		isr->isr_evcnt->ev_count++;
}

/*
 * netisr junk...
 * should use an array of chars instead of
 * a bitmask to avoid atomicity locking issues.
 */

void
netintr()
{
	int n, s;

	s = splhigh();
	n = netisr;
	netisr = 0;
	splx(s);

#define DONETISR(bit, fn) do {		\
		if (n & (1 << bit)) 	\
			fn();		\
		} while (0)

	s = splsoftnet();

#include <net/netisr_dispatch.h>

#undef DONETISR

	splx(s);
}

/* ARGSUSED */
static int
spurintr(void *arg)
{

	return (1);
}
