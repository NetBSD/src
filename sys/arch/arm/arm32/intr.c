/*	$NetBSD: intr.c,v 1.25.6.1 2008/01/08 22:09:25 bouyer Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.25.6.1 2008/01/08 22:09:25 bouyer Exp $");

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

static u_int spl_smasks[_SPL_LEVELS];

/* Eventually these will become macros */

#ifdef __HAVE_FAST_SOFTINTS
/* Generate soft interrupt counts if IRQSTATS is defined */
/* Prototypes */
static void clearsoftintr(u_int); 
 
static u_int soft_interrupts = 0;
#define	SI_SOFTMASK(si)	(1U << (si))

static inline void
clearsoftintr(u_int intrmask)
{
	atomic_clear_bit(&soft_interrupts, intrmask);
}

void
_setsoftintr(int si)
{
	atomic_set_bit(&soft_interrupts, SI_SOFTMASK(si));
}

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
		clearsoftintr(SI_SOFTMASK(SI_SOFTSERIAL));
		softintr_dispatch(SI_SOFTSERIAL);
		(void)splx(s);
	}

	/*
	 * Network software interrupts
	 */
	if (softints & SI_SOFTMASK(SI_SOFTNET)) {
		s = splsoftnet();
		clearsoftintr(SI_SOFTMASK(SI_SOFTNET));
		softintr_dispatch(SI_SOFTNET);
		(void)splx(s);
	}

	/*
	 * Block software interrupts
	 */
	if (softints & SI_SOFTMASK(SI_SOFTBIO)) {
		s = splsoftbio();
		clearsoftintr(SI_SOFTMASK(SI_SOFTBIO));
		softintr_dispatch(SI_SOFTBIO);
		(void)splx(s);
	}

	/*
	 * Software clock interrupts
	 */
	if (softints & SI_SOFTMASK(SI_SOFTCLOCK)) {
		s = splsoftclock();
		clearsoftintr(SI_SOFTMASK(SI_SOFTCLOCK));
		softintr_dispatch(SI_SOFTCLOCK);
		(void)splx(s);
	}
}
#endif

int current_spl_level = _SPL_HIGH;
u_int spl_masks[_SPL_LEVELS + 1];
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

	spl_masks[_SPL_VM]         = irqmasks[IPL_VM];
	spl_masks[_SPL_SCHED]      = irqmasks[IPL_SCHED];
	spl_masks[_SPL_HIGH]       = irqmasks[IPL_HIGH];
	spl_masks[_SPL_LEVELS]     = 0;

	spl_smasks[_SPL_0] = 0xffffffff;
#ifdef __HAVE_FAST_SOFTINTS
	for (loop = 0; loop < _SPL_SOFTSERIAL; ++loop)
		spl_smasks[loop] |= SI_SOFTMASK(SI_SOFTSERIAL);
	for (loop = 0; loop < _SPL_SOFTNET; ++loop)
		spl_smasks[loop] |= SI_SOFTMASK(SI_SOFTNET);
	for (loop = 0; loop < _SPL_SOFTCLOCK; ++loop)
		spl_smasks[loop] |= SI_SOFTMASK(SI_SOFTCLOCK);
	for (loop = 0; loop < _SPL_SOFTBIO; ++loop)
		spl_smasks[loop] |= SI_SOFTMASK(SI_SOFTBIO);
#endif
}

static const int ipl_to_spl_map[] = {
	[IPL_NONE] = 1 + _SPL_0,
	[IPL_SOFTCLOCK] = 1 + _SPL_SOFTCLOCK,
	[IPL_SOFTBIO] = 1 + _SPL_SOFTBIO,
	[IPL_SOFTNET] = 1 + _SPL_SOFTNET,
	[IPL_SOFTSERIAL] = 1 + _SPL_SOFTSERIAL,
	[IPL_VM] = 1 + _SPL_VM,
	[IPL_SCHED] = 1 + _SPL_SCHED,
	[IPL_HIGH] = 1 + _SPL_HIGH,
};

int
ipl_to_spl(ipl_t ipl)
{
	int spl;

	KASSERT(ipl < __arraycount(ipl_to_spl_map));
	KASSERT(ipl_to_spl_map[ipl]);

	spl = ipl_to_spl_map[ipl] - 1;
	KASSERT(spl < 0x100);

	return spl;
}

#ifdef DIAGNOSTIC
void
dump_spl_masks(void)
{
	int loop;

	for (loop = 0; loop < _SPL_LEVELS; ++loop) {
		printf("spl_masks[%d]=%08x splsmask[%d]=%08x\n", loop,
		    spl_masks[loop], loop, spl_smasks[loop]);
	}
}
#endif

/* End of intr.c */
