/*	$NetBSD: intr.c,v 1.1.2.5 2002/02/11 20:07:18 jdolecek Exp $	*/

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

#include "opt_irqstats.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <machine/cpu.h>

#include <net/netisr.h>

#include <machine/conf.h>
#include <arm/arm32/machdep.h>
 
#ifndef NPLCOM
#define NPLCOM 0
#endif

/* Prototypes */
static void clearsoftintr __P((u_int)); 
 
u_int soft_interrupts = 0;

extern int current_spl_level;

extern unsigned spl_mask;

/* Generate soft interrupt counts if IRQSTATS is defined */
#ifdef IRQSTATS
extern u_int sintrcnt[];
#define INC_SINTRCNT(x) ++sintrcnt[x]
#else
#define INC_SINTRCNT(x)
#endif	/* IRQSTATS */

#define	COUNT	uvmexp.softs;

/* Prototypes */

#include "com.h"
#if NCOM > 0
extern void comsoft	__P((void));
#endif	/* NCOM > 0 */

#if NPLCOM > 0
extern void plcomsoft	__P((void));
#endif	/* NPLCOM > 0 */

/* Eventually these will become macros */

void
setsoftintr(intrmask)
	u_int intrmask;
{
	atomic_set_bit(&soft_interrupts, intrmask);
}

static void
clearsoftintr(intrmask)
	u_int intrmask;
{
	atomic_clear_bit(&soft_interrupts, intrmask);
}

void
setsoftclock()
{
	atomic_set_bit(&soft_interrupts, SOFTIRQ_BIT(SOFTIRQ_CLOCK));
}

void
setsoftnet()
{
	atomic_set_bit(&soft_interrupts, SOFTIRQ_BIT(SOFTIRQ_NET));
}

void
setsoftserial()
{
	atomic_set_bit(&soft_interrupts, SOFTIRQ_BIT(SOFTIRQ_SERIAL));
}

/* Handle software interrupts */

void
dosoftints()
{
	u_int softints;
	int s;

	softints = soft_interrupts & spl_smasks[current_spl_level];
	if (softints == 0) return;

	/*
	 * Software clock interrupts
	 */

	if (softints & SOFTIRQ_BIT(SOFTIRQ_CLOCK)) {
		s = splsoftclock();
		++COUNT;
		INC_SINTRCNT(SOFTIRQ_CLOCK);
		clearsoftintr(SOFTIRQ_BIT(SOFTIRQ_CLOCK));
		softclock(NULL);
		(void)splx(s);
	}

	/*
	 * Network software interrupts
	 */

	if (softints & SOFTIRQ_BIT(SOFTIRQ_NET)) {
		s = splsoftnet();
		++COUNT;
		INC_SINTRCNT(SOFTIRQ_NET);
		clearsoftintr(SOFTIRQ_BIT(SOFTIRQ_NET));

#define DONETISR(bit, fn) do {					\
		if (netisr & (1 << bit)) {			\
			atomic_clear_bit(&netisr, (1 << bit));	\
			fn();					\
		}						\
} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR

		(void)splx(s);
	}
	/*
	 * Serial software interrupts
	 */

	if (softints & SOFTIRQ_BIT(SOFTIRQ_SERIAL)) {
		s = splsoftserial();
		++COUNT;
		INC_SINTRCNT(SOFTIRQ_SERIAL);
		clearsoftintr(SOFTIRQ_BIT(SOFTIRQ_SERIAL));
#if NCOM > 0
		comsoft();
#endif	/* NCOM > 0 */
#if NPLCOM > 0
		plcomsoft();
#endif	/* NPLCOM > 0 */
		(void)splx(s);
	}
}

int current_spl_level = _SPL_SERIAL;
u_int spl_masks[_SPL_LEVELS + 1];
u_int spl_smasks[_SPL_LEVELS];
int safepri = _SPL_0;

extern u_int irqmasks[];

void
set_spl_masks()
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
	spl_masks[_SPL_IMP]        = irqmasks[IPL_IMP];
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
		spl_smasks[loop] |= SOFTIRQ_BIT(SOFTIRQ_SERIAL);
	for (loop = 0; loop < _SPL_SOFTNET; ++loop)
		spl_smasks[loop] |= SOFTIRQ_BIT(SOFTIRQ_NET);
	for (loop = 0; loop < _SPL_SOFTCLOCK; ++loop)
		spl_smasks[loop] |= SOFTIRQ_BIT(SOFTIRQ_CLOCK);
}

#ifdef DIAGNOSTIC
void
dump_spl_masks()
{
	int loop;

	for (loop = 0; loop < _SPL_LEVELS; ++loop) {
		printf("spl_mask[%d]=%08x splsmask[%d]=%08x\n", loop,
		    spl_masks[loop], loop, spl_smasks[loop]);
	}
}
#endif

/* End of intr.c */
