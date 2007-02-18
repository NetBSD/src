/*	$NetBSD: intr.c,v 1.15 2007/02/18 07:13:18 matt Exp $	*/

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Soft interrupt and other generic interrupt functions.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.15 2007/02/18 07:13:18 matt Exp $");

#include "opt_irqstats.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <sys/conf.h>

#include <uvm/uvm_extern.h>

#include <machine/atomic.h>
#include <machine/intr.h>
#include <machine/cpu.h>

#include <net/netisr.h>

#include <arm/arm32/machdep.h>
 
extern int current_spl_level;

extern unsigned spl_mask;

/* Generate soft interrupt counts if IRQSTATS is defined */
/* Prototypes */
static void clearsoftintr(u_int); 
 
u_int soft_interrupts = 0;

#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
#ifdef IRQSTATS
extern u_int sintrcnt[];
#define INC_SINTRCNT(x) ++sintrcnt[x]
#else
#define INC_SINTRCNT(x)
#endif	/* IRQSTATS */
#else
#define INC_SINTRCNT(x)
#endif  /* !__HAVE_GENERIC_SOFT_INTERRUPTS */

#define	COUNT	uvmexp.softs;

/* Prototypes */

#include "com.h"
#if NCOM > 0
extern void comsoft(void);
#endif	/* NCOM > 0 */

#if NPLCOM > 0
extern void plcomsoft(void);
#endif	/* NPLCOM > 0 */

/* Eventually these will become macros */

#define	SI_SOFTMASK(si)	(1U << (si))

static inline void
clearsoftintr(u_int intrmask)
{
	atomic_clear_bit(&soft_interrupts, intrmask);
}

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
void
_setsoftintr(int si)
{
	atomic_set_bit(&soft_interrupts, SI_SOFTMASK(si));
}
#else /* __HAVE_GENERIC_SOFT_INTERRUPTS */
#define	SI_SOFTNET	SOFTIRQ_NET
#define	SI_SOFTCLOCK	SOFTIRQ_CLOCK
#define	SI_SOFTSERIAL	SOFTIRQ_SERIAL
void
setsoftintr(u_int intrmask)
{
	atomic_set_bit(&soft_interrupts, intrmask);
}

void
setsoftclock(void)
{
	atomic_set_bit(&soft_interrupts, SI_MASK(SI_SOFTCLOCK));
}

void
setsoftnet(void)
{
	atomic_set_bit(&soft_interrupts, SI_MASK(SI_SOFTNET));
}

void
setsoftserial(void)
{
	atomic_set_bit(&soft_interrupts, SI_MASK(SI_SOFTSERIAL));
}

static void
softnet(void)
{
#define DONETISR(bit, fn) do {					\
		if (netisr & (1 << bit)) {			\
			atomic_clear_bit(&netisr, (1 << bit));	\
			fn();					\
		}						\
} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR
}
#endif /* !__HAVE_GENERIC_SOFT_INTERRUPTS */

/* Handle software interrupts */

void
dosoftints(void)
{
	u_int softints;
	int s;

	softints = soft_interrupts & spl_smasks[current_spl_level];
	if (softints == 0) return;

	/*
	 * Serial software interrupts
	 */
	if (softints & SI_SOFTMASK(SI_SOFTSERIAL)) {
		s = splsoftserial();
		++COUNT;
		INC_SINTRCNT(SI_SOFTSERIAL);
		clearsoftintr(SI_SOFTMASK(SI_SOFTSERIAL));
#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
#if NCOM > 0
		comsoft();
#endif	/* NCOM > 0 */
#if NPLCOM > 0
		plcomsoft();
#endif	/* NPLCOM > 0 */
#else
		softintr_dispatch(SI_SOFTSERIAL);
#endif
		(void)splx(s);
	}

	/*
	 * Network software interrupts
	 */
	if (softints & SI_SOFTMASK(SI_SOFTNET)) {
		s = splsoftnet();
		++COUNT;
		INC_SINTRCNT(SI_SOFTNET);
		clearsoftintr(SI_SOFTMASK(SI_SOFTNET));
#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
		softnet();
#else
		softintr_dispatch(SI_SOFTNET);
#endif
		(void)splx(s);
	}

	/*
	 * Software clock interrupts
	 */
	if (softints & SI_SOFTMASK(SI_SOFTCLOCK)) {
		s = splsoftclock();
		++COUNT;
		INC_SINTRCNT(SI_SOFTCLOCK);
		clearsoftintr(SI_SOFTMASK(SI_SOFTCLOCK));
#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
		softclock(NULL);
#else
		softintr_dispatch(SI_SOFTCLOCK);
#endif
		(void)splx(s);
	}

	/*
	 * Misc software interrupts
	 */
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	if (softints & SI_SOFTMASK(SI_SOFT)) {
		s = splsoft();
		++COUNT;
		clearsoftintr(SI_SOFTMASK(SI_SOFT));
		softintr_dispatch(SI_SOFT);
		(void)splx(s);
	}
#endif
}

int current_spl_level = _SPL_SERIAL;
u_int spl_masks[_SPL_LEVELS + 1];
u_int spl_smasks[_SPL_LEVELS];
int safepri = _SPL_0;

extern u_int irqmasks[];

void
set_spl_masks(void)
{
	int loop;

	for (loop = 0; loop < _SPL_LEVELS; ++loop) {
		spl_masks[loop] = 0xffffffff;
		spl_smasks[loop] = 0;
	}

	spl_masks[_SPL_BIO]        = irqmasks[IPL_BIO];
	spl_masks[_SPL_NET]        = irqmasks[IPL_NET];
	spl_masks[_SPL_SOFTSERIAL] = irqmasks[IPL_TTY];
	spl_masks[_SPL_TTY]        = irqmasks[IPL_TTY];
	spl_masks[_SPL_VM]         = irqmasks[IPL_VM];
	spl_masks[_SPL_AUDIO]      = irqmasks[IPL_AUDIO];
	spl_masks[_SPL_CLOCK]      = irqmasks[IPL_CLOCK];
#ifdef IPL_STATCLOCK
	spl_masks[_SPL_STATCLOCK]  = irqmasks[IPL_STATCLOCK];
#else
	spl_masks[_SPL_STATCLOCK]  = irqmasks[IPL_CLOCK];
#endif
	spl_masks[_SPL_HIGH]       = irqmasks[IPL_HIGH];
	spl_masks[_SPL_SERIAL]     = irqmasks[IPL_SERIAL];
	spl_masks[_SPL_LEVELS]     = 0;

	spl_smasks[_SPL_0] = 0xffffffff;
	for (loop = 0; loop < _SPL_SOFTSERIAL; ++loop)
		spl_smasks[loop] |= SI_SOFTMASK(SI_SOFTSERIAL);
	for (loop = 0; loop < _SPL_SOFTNET; ++loop)
		spl_smasks[loop] |= SI_SOFTMASK(SI_SOFTNET);
	for (loop = 0; loop < _SPL_SOFTCLOCK; ++loop)
		spl_smasks[loop] |= SI_SOFTMASK(SI_SOFTCLOCK);
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	for (loop = 0; loop < _SPL_SOFT; ++loop)
		spl_smasks[loop] |= SI_SOFTMASK(SI_SOFT);
#endif
}

static const int ipl_to_spl_map[] = {
	[IPL_NONE] = 1 + _SPL_0,
#ifdef IPL_SOFT
	[IPL_SOFT] = 1 + _SPL_SOFT,
#endif /* IPL_SOFTCLOCK */
#if defined(IPL_SOFTCLOCK)
	[IPL_SOFTCLOCK] = 1 + _SPL_SOFTCLOCK,
#endif /* defined(IPL_SOFTCLOCK) */
#if defined(IPL_SOFTNET)
	[IPL_SOFTNET] = 1 + _SPL_SOFTNET,
#endif /* defined(IPL_SOFTNET) */
	[IPL_BIO] = 1 + _SPL_BIO,
	[IPL_NET] = 1 + _SPL_NET,
#if defined(IPL_SOFTSERIAL)
	[IPL_SOFTSERIAL] = 1 + _SPL_SOFTSERIAL,
#endif /* defined(IPL_SOFTSERIAL) */
	[IPL_TTY] = 1 + _SPL_TTY,
	[IPL_VM] = 1 + _SPL_VM,
	[IPL_AUDIO] = 1 + _SPL_AUDIO,
	[IPL_CLOCK] = 1 + _SPL_CLOCK,
	[IPL_STATCLOCK] = 1 + _SPL_STATCLOCK,
	[IPL_HIGH] = 1 + _SPL_HIGH,
	[IPL_SERIAL] = 1 + _SPL_SERIAL,
};

int
ipl_to_spl(int ipl)
{
	KASSERT(ipl < __arraycount(ipl_to_spl_map));
	KASSERT(ipl_to_spl_map[ipl]);

	return ipl_to_spl_map[ipl] - 1;
}

#ifdef DIAGNOSTIC
void
dump_spl_masks(void)
{
	int loop;

	for (loop = 0; loop < _SPL_LEVELS; ++loop) {
		printf("spl_mask[%d]=%08x splsmask[%d]=%08x\n", loop,
		    spl_masks[loop], loop, spl_smasks[loop]);
	}
}
#endif

/* End of intr.c */
