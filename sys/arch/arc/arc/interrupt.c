/*	$NetBSD: interrupt.c,v 1.6.2.1 2011/06/06 09:04:57 jruoho Exp $	*/
/*	$OpenBSD: trap.c,v 1.22 1999/05/24 23:08:59 jason Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
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
 * from: Utah Hdr: trap.c 1.32 91/04/06
 *
 *	@(#)trap.c	8.5 (Berkeley) 1/11/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: interrupt.c,v 1.6.2.1 2011/06/06 09:04:57 jruoho Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/intr.h>
#include <sys/cpu.h>

#include <mips/locore.h>

#include <machine/autoconf.h>
#include <machine/pio.h>

#include <arc/arc/timervar.h>
#include <arc/jazz/pica.h>
#include <arc/jazz/rd94.h>

struct cpu_inttab {
	uint32_t int_mask;
	uint32_t (*int_hand)(uint32_t, struct clockframe *);
};
static struct cpu_inttab cpu_int_tab[ARC_NINTPRI];

uint32_t cpu_int_mask;	/* External cpu interrupt mask */

#ifdef ENABLE_INT5_STATCLOCK
struct evcnt statclock_ev =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "cpu", "statclock");
#endif

/*
 *	Set up handler for external interrupt events.
 *	Events are checked in priority order.
 */
void
arc_set_intr(uint32_t mask, uint32_t (*int_hand)(uint32_t, struct clockframe *),
    int prio)
{

	if (prio >= ARC_NINTPRI)
		panic("arc_set_intr: too high priority");

	if (cpu_int_tab[prio].int_mask != 0 &&
	    (cpu_int_tab[prio].int_mask != mask ||
	     cpu_int_tab[prio].int_hand != int_hand)) {
		panic("set_intr: int already set");
	}

	cpu_int_tab[prio].int_hand = int_hand;
	cpu_int_tab[prio].int_mask = mask;
	cpu_int_mask |= mask >> 10;
}

/*
 * Handle an interrupt.
 */
void
cpu_intr(int ppl, vaddr_t pc, uint32_t status)
{
	struct cpu_inttab *inttab;
	struct clockframe cf;
	uint32_t ipending;
	u_int i;
	int ipl;

	curcpu()->ci_data.cpu_nintr++;

	cf.pc = pc;
	cf.sr = status;
	cf.intr = (curcpu()->ci_idepth > 1);

	while (ppl < (ipl = splintr(&ipending))) {
		/* check MIPS3 internal clock interrupt */
		if (ipending & MIPS_INT_MASK_5) {
#ifdef ENABLE_INT5_STATCLOCK
			/* call statclock(9) handler */
			statclockintr(&cf);
			statclock_ev.ev_count++;
#else
			/*
			 * Writing a value to the Compare register, as a side
			 * effect, clears the timer interrupt request.
			 */
			mips3_cp0_compare_write(0);
#endif
		}

		/*
		 * If there is an independent timer interrupt handler,
		 * call it first.
		 */
		inttab = &cpu_int_tab[ARC_INTPRI_TIMER_INT];
		if (inttab->int_mask & ipending) {
			(*inttab->int_hand)(ipending, &cf);
		}

		/*
		 *  Check off all other enabled interrupts.
		 *  Called handlers return mask of interrupts to be reenabled.
		 */
		for (inttab++, i = ARC_INTPRI_TIMER_INT + 1;
		     i < ARC_NINTPRI;
		     inttab++, i++) {
			if (inttab->int_mask & ipending) {
				(*inttab->int_hand)(ipending, &cf);
			}
		}
		(void)splhigh();
	}
}
