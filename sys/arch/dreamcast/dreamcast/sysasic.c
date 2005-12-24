/*	$NetBSD: sysasic.c,v 1.11 2005/12/24 20:06:59 perry Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sysasic.c,v 1.11 2005/12/24 20:06:59 perry Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/syslog.h>

#include <sh3/exception.h>

#include <machine/intr.h>
#include <machine/sysasicvar.h>

#define SYSASIC_INTR_ST		0xa05f6900
#define SYSASIC_INTR_EN(level)	(0xa05f6910 + ((level) << 4))

#define SYSASIC_IRQ_LEVEL_13	0
#define SYSASIC_IRQ_LEVEL_11	1
#define SYSASIC_IRQ_LEVEL_9	2
#define SYSASIC_IRQ_LEVEL_MAX	2
#define SYSASIC_IRQ_INDEX_TO_IRQ(i)	(13 - 2 * (i))

#define IPL_IRL9	IPL_BIO
#define IPL_IRL11	IPL_NET
#define IPL_IRL13	IPL_TTY

/* per-irq */
struct sysasic_intrhand {
	/* for quick check on interrupt */
#define SYSASIC_EVENT_NMAP	((SYSASIC_EVENT_MAX + 1 + (32 - 1)) / 32)
#define SYSASIC_EVENT_INTR_MAP(ev)	((ev) >> 5)
#define SYSASIC_EVENT_INTR_BIT(ev)	((unsigned) 1 << ((ev) & 31))
	unsigned	syh_events[SYSASIC_EVENT_NMAP];	/* enabled */
	unsigned	syh_hndmap[SYSASIC_EVENT_NMAP];	/* handler installed */

	void	*syh_intc;
	int	syh_idx;
} sysasic_intrhand[SYSASIC_IRQ_LEVEL_MAX + 1];

/* per-event */
struct	syh_eventhand {
	int	(*hnd_fn)(void *);	/* sub handler */
	void	*hnd_arg;
	struct sysasic_intrhand *hnd_syh;
} sysasic_eventhand[SYSASIC_EVENT_MAX + 1];

int sysasic_intr(void *);

const char * __pure
sysasic_intr_string(int ipl)
{

	switch (ipl) {
	default:
#ifdef DEBUG
		panic("sysasic_intr_string: unknown ipl %d", ipl);
#endif
	case IPL_IRL9:
		return "SH4 IRL 9";
	case IPL_IRL11:
		return "SH4 IRL 11";
	case IPL_IRL13:
		return "SH4 IRL 13";
	}
	/* NOTREACHED */
}

/*
 * Set up an interrupt handler to start being called.
 */
void *
sysasic_intr_establish(int event, int ipl, int (*ih_fun)(void *), void *ih_arg)
{
	struct sysasic_intrhand *syh;
	struct syh_eventhand *hnd;
	int idx;
	static const int idx2evt[3] = {
		SH_INTEVT_IRL13, SH_INTEVT_IRL11, SH_INTEVT_IRL9
	};
#ifdef DEBUG
	int i;
#endif

	KDASSERT(event >= 0 && event <= SYSASIC_EVENT_MAX);
	KDASSERT(ih_fun);

	/*
	 * Dreamcast use SH4 INTC as IRL mode.  If IRQ is specified,
	 * its priority level is fixed.
	 *
	 * We use IPL to specify the IRQ for clearness, that is, we use
	 * a splxxx() and IPL_XXX pair in a device driver.
	 */
	switch (ipl) {
	default:
#ifdef DEBUG
		panic("sysasic_intr_establish: unknown ipl %d", ipl);
#endif
	case IPL_IRL9:
		idx = SYSASIC_IRQ_LEVEL_9;
		break;
	case IPL_IRL11:
		idx = SYSASIC_IRQ_LEVEL_11;
		break;
	case IPL_IRL13:
		idx = SYSASIC_IRQ_LEVEL_13;
		break;
	}

	syh = &sysasic_intrhand[idx];

	if (syh->syh_intc == NULL) {
		syh->syh_idx	= idx;
		syh->syh_intc	= intc_intr_establish(idx2evt[idx], IST_LEVEL,
		    ipl, sysasic_intr, syh);
	}

#ifdef DEBUG
	/* check if the event handler is already installed */
	for (i = 0; i <= SYSASIC_IRQ_LEVEL_MAX; i++)
		if ((sysasic_intrhand[i].syh_hndmap[SYSASIC_EVENT_INTR_MAP(event)] &
		    SYSASIC_EVENT_INTR_BIT(event)) != 0)
			panic("sysasic_intr_establish: event %d already installed irq %d",
			    event, SYSASIC_IRQ_INDEX_TO_IRQ(i));
#endif

	/* mark this event is established */
	syh->syh_hndmap[SYSASIC_EVENT_INTR_MAP(event)] |=
	    SYSASIC_EVENT_INTR_BIT(event);

	hnd = &sysasic_eventhand[event];
	hnd->hnd_fn = ih_fun;
	hnd->hnd_arg = ih_arg;
	hnd->hnd_syh = syh;
	sysasic_intr_enable(hnd, 1);

	return (void *)hnd;
}

/*
 * Deregister an interrupt handler.
 */
void
sysasic_intr_disestablish(void *arg)
{
	struct syh_eventhand *hnd = arg;
	struct sysasic_intrhand *syh;
	int event;
	int i;

	event = hnd - sysasic_eventhand;
	KDASSERT(event >= 0 && event <= SYSASIC_EVENT_MAX);
	syh = hnd->hnd_syh;

#ifdef DIAGNOSTIC
	if ((syh->syh_hndmap[SYSASIC_EVENT_INTR_MAP(event)] &
	    SYSASIC_EVENT_INTR_BIT(event)) == 0)
		panic("sysasic_intr_disestablish: event %d not installed for irq %d",
		    event, SYSASIC_IRQ_INDEX_TO_IRQ(syh->syh_idx));
#endif

	sysasic_intr_enable(hnd, 0);
	hnd->hnd_fn = 0;
	hnd->hnd_arg = 0;
	hnd->hnd_syh = 0;

	syh->syh_hndmap[SYSASIC_EVENT_INTR_MAP(event)] &=
	    ~SYSASIC_EVENT_INTR_BIT(event);

	/* deinstall intrc if no event exists */
	for (i = 0; i < SYSASIC_EVENT_NMAP; i++)
		if (syh->syh_hndmap[i])
			return;
	intc_intr_disestablish(syh->syh_intc);
	syh->syh_intc = 0;
}

void
sysasic_intr_enable(void *arg, int on)
{
	struct syh_eventhand *hnd = arg;
	struct sysasic_intrhand *syh;
	int event;
	volatile uint32_t *masks, *stats;
	int evmap;
	unsigned evbit;

	event = hnd - sysasic_eventhand;
	KDASSERT(event >= 0 && event <= SYSASIC_EVENT_MAX);
	syh = hnd->hnd_syh;

#ifdef DIAGNOSTIC
	if ((syh->syh_hndmap[SYSASIC_EVENT_INTR_MAP(event)] &
	    SYSASIC_EVENT_INTR_BIT(event)) == 0)
		panic("sysasic_intr_enable: event %d not installed for irq %d",
		    event, SYSASIC_IRQ_INDEX_TO_IRQ(syh->syh_idx));
#endif

	masks = (volatile uint32_t *) SYSASIC_INTR_EN(syh->syh_idx);
	stats = (volatile uint32_t *) SYSASIC_INTR_ST;
	evmap = SYSASIC_EVENT_INTR_MAP(event);
	evbit = SYSASIC_EVENT_INTR_BIT(event);

	if (on) {
		/* clear pending event if any */
		stats[evmap] = evbit;

		/* set event map */
		syh->syh_events[evmap] |= evbit;

		/* enable interrupt */
		masks[evmap] = syh->syh_events[evmap];
	} else {
		/* disable interrupt */
		masks[evmap] = syh->syh_events[evmap] & ~evbit;

		/* clear pending event if any */
		stats[evmap] = evbit;

		/* clear event map */
		syh->syh_events[evmap] &= ~evbit;
	}
}

int
sysasic_intr(void *arg)
{
	struct sysasic_intrhand *syh = arg;
	unsigned ev;
	int n, pos;
	uint32_t *evwatched;
	volatile uint32_t *evmap;
	struct syh_eventhand *evh;
#ifdef DEBUG
	int handled = 0;
	static int reportcnt = 10;
#endif

	/* bitmap of watched events */
	evwatched = syh->syh_events;

	/* status / clear */
	evmap = (volatile uint32_t *) SYSASIC_INTR_ST;

	for (n = 0; n < (SYSASIC_EVENT_NMAP << 5); n |= 31, n++, evmap++) {
		if ((ev = *evwatched++) && (ev &= *evmap)) {

			/* clear interrupts */
			*evmap = ev;

			n--;	/* to point at current bit after  n += pos */

			/* call handlers */
			do {
				pos = ffs(ev);
				n += pos;
#ifdef __OPTIMIZE__
				/* optimized, assuming 1 <= pos <= 32 */
				asm("shld	%2,%0"
				    : "=r" (ev) : "0" (ev), "r" (-pos));
#else
				/* ``shift count >= bit width'' is undefined */
				if (pos >= 32)
					ev = 0;
				else
					ev >>= pos;
#endif

				evh = &sysasic_eventhand[n];
#ifdef DEBUG
				KDASSERT(evh->hnd_fn);
				if ((*evh->hnd_fn)(evh->hnd_arg))
					handled = 1;
#else
				(*evh->hnd_fn)(evh->hnd_arg);
#endif
			} while (ev);
		}
	}

#ifdef DEBUG
	if (!handled && reportcnt > 0) {
		reportcnt--;
		log(LOG_ERR, "sysasic_intr: stray irq%d interrupt%s\n",
		    SYSASIC_IRQ_INDEX_TO_IRQ(syh->syh_idx),
		    reportcnt == 0 ? "; stopped logging" : "");
	}
#endif

	return 0;
}
