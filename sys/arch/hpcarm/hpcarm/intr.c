/*	$NetBSD: intr.c,v 1.11.6.1 2007/07/15 13:16:01 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.11.6.1 2007/07/15 13:16:01 ad Exp $");

#include "opt_irqstats.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/atomic.h>
#include <machine/intr.h>
#include <machine/cpu.h>

#include <net/netisr.h>

u_int soft_interrupts = 0;

extern int current_spl_level;
extern int softintr_dispatch(int);

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
extern void comsoft(void);
#endif	/* NCOM > 0 */

#include "sacom.h"
#if NSACOM > 0
extern void sacomsoft(void);
#endif	/* NSACOM > 0 */

/* Eventually these will become macros */

void setsoftintr(u_int);
void clearsoftintr(u_int);
void dosoftints(void);

void
setsoftintr(u_int intrmask)
{
	atomic_set_bit(&soft_interrupts, intrmask);
}

void
clearsoftintr(u_int intrmask)
{
	atomic_clear_bit(&soft_interrupts, intrmask);
}

void
setsoftnet(void)
{
	atomic_set_bit(&soft_interrupts, SOFTIRQ_BIT(SOFTIRQ_NET));
}

int astpending;

void    set_spl_masks(void);

int current_spl_level = _SPL_HIGH;
u_int spl_masks[_SPL_LEVELS + 1];
u_int spl_smasks[_SPL_LEVELS];
int safepri = _SPL_0;

/* Handle software interrupts */

void
dosoftints(void)
{
	u_int softints;
	int s;

	softintr_dispatch(current_spl_level);

	softints = soft_interrupts & spl_smasks[current_spl_level];
	if (softints == 0)
		return;

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
}

void
set_spl_masks(void)
{
	int loop;

	for (loop = 0; loop < _SPL_LEVELS; ++loop)
		spl_smasks[loop] = 0;

	for (loop = 0; loop <= _SPL_SOFTCLOCK; loop++)
		spl_masks[loop]    = imask[IPL_SOFTCLOCK];

	spl_masks[_SPL_SOFTNET]    = imask[IPL_SOFTNET];
	spl_masks[_SPL_BIO]        = imask[IPL_BIO];
	spl_masks[_SPL_NET]        = imask[IPL_NET];
	spl_masks[_SPL_SOFTSERIAL] = imask[IPL_SOFTSERIAL];
	spl_masks[_SPL_TTY]        = imask[IPL_TTY];
	spl_masks[_SPL_VM]         = imask[IPL_VM];
	spl_masks[_SPL_AUDIO]      = imask[IPL_AUDIO];
	spl_masks[_SPL_CLOCK]      = imask[IPL_CLOCK];
	spl_masks[_SPL_HIGH]       = imask[IPL_HIGH];
	spl_masks[_SPL_SERIAL]     = imask[IPL_SERIAL];
	spl_masks[_SPL_LEVELS]     = 0;

	spl_smasks[_SPL_0] = 0xffffffff;
	for (loop = 0; loop < _SPL_SOFTSERIAL; ++loop)
		spl_smasks[loop] |= SOFTIRQ_BIT(SOFTIRQ_SERIAL);
	for (loop = 0; loop < _SPL_SOFTNET; ++loop)
		spl_smasks[loop] |= SOFTIRQ_BIT(SOFTIRQ_NET);
	for (loop = 0; loop < _SPL_SOFTCLOCK; ++loop)
		spl_smasks[loop] |= SOFTIRQ_BIT(SOFTIRQ_CLOCK);
}

int
ipl_to_spl(ipl_t ipl)
{

	switch (ipl) {
	case IPL_NONE:
		return _SPL_0;
	case IPL_SOFTCLOCK:
		return _SPL_SOFTCLOCK;
	case IPL_SOFTNET:
		return _SPL_SOFTNET;
	case IPL_BIO:
		return _SPL_BIO;
	case IPL_NET:
		return _SPL_NET;
	case IPL_SOFTSERIAL:
		return _SPL_SOFTSERIAL;
	case IPL_TTY:
		return _SPL_TTY;
	case IPL_VM:
		return _SPL_VM;
	case IPL_AUDIO:
		return _SPL_AUDIO;
	case IPL_CLOCK:
		return _SPL_CLOCK;
	case IPL_STATCLOCK:
		return _SPL_STATCLOCK;
	case IPL_HIGH:
		return _SPL_HIGH;
	case IPL_SERIAL:
		return _SPL_SERIAL;
	default:
		panic("bogus ipl %d", ipl);
	}
}

#ifdef DIAGNOSTIC
void	dump_spl_masks(void);

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
