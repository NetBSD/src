/*	$NetBSD: arc_trap.c,v 1.20.8.2 2002/06/24 22:03:38 nathanw Exp $	*/
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <mips/locore.h>

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/pio.h>

#include <arc/jazz/pica.h>
#include <arc/jazz/rd94.h>

int arc_hardware_intr __P((u_int32_t, u_int32_t, u_int32_t, u_int32_t));

#define	MIPS_INT_LEVELS	8

struct {
	int	int_mask;
	int	(*int_hand)(u_int, struct clockframe *);
} cpu_int_tab[MIPS_INT_LEVELS];

int cpu_int_mask;	/* External cpu interrupt mask */

int
arc_hardware_intr(status, cause, pc, ipending)
	u_int32_t status;
	u_int32_t cause;
	u_int32_t pc;
	u_int32_t ipending;
{
	register int i;
	struct clockframe cf;

	cf.pc = pc;
	cf.sr = status;

	/*
	 *  Check off all enabled interrupts. Called interrupt routine
	 *  returns mask of interrupts to reenable.
	 */
	for(i = 0; i < MIPS_INT_LEVELS; i++) {
		if(cpu_int_tab[i].int_mask & ipending) {
			cause &= (*cpu_int_tab[i].int_hand)(ipending, &cf);
		}
	}

	/*
	 *  Reenable all non served hardware levels.
	 */
	return ((status & ~cause & MIPS3_HARD_INT_MASK) | MIPS_SR_INT_IE);
}

/*
 *	Set up handler for external interrupt events.
 *	Events are checked in priority order.
 */
void
arc_set_intr(mask, int_hand, prio)
	int	mask;
	int	(*int_hand)(u_int, struct clockframe *);
	int	prio;
{
	if(prio > MIPS_INT_LEVELS)
		panic("set_intr: to high priority");

	if(cpu_int_tab[prio].int_mask != 0 &&
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
 * N.B., curlwp might be NULL.
 */
void
cpu_intr(status, cause, pc, ipending)
	u_int32_t status;
	u_int32_t cause;
	u_int32_t pc;
	u_int32_t ipending;		/* pending interrupts & enable mask */
{
#if defined(MIPS3) && defined(MIPS_INT_MASK_CLOCK)
	if ((ipending & MIPS_INT_MASK_CLOCK) && CPUISMIPS3) {
		/*
		 *  Writing a value to the Compare register,
		 *  as a side effect, clears the timer interrupt request.
		 */
		mips3_cp0_compare_write(mips3_cp0_count_read());
	}
#endif

	uvmexp.intrs++;
	/* real device interrupt */
	if (ipending & INT_MASK_REAL_DEV) {
		_splset(arc_hardware_intr(status, cause, pc, ipending));
	}

#if defined(MIPS1) && defined(INT_MASK_FPU)
	if ((ipending & INT_MASK_FPU) && CPUISMIPS1) {
		intrcnt[FPU_INTR]++;
		if (!USERMODE(status))
			panic("kernel used FPU: PC %x, CR %x, SR %x",
			    pc, cause, status);
		MachFPInterrupt(status, cause, pc, curlwp->p_md.md_regs);
	}
#endif

	/* 'softnet' interrupt */
	if (ipending & MIPS_SOFT_INT_MASK_1) {
		clearsoftnet();
		uvmexp.softs++;
		netintr();
	}

	/* 'softclock' interrupt */
	if (ipending & MIPS_SOFT_INT_MASK_0) {
		clearsoftclock();
		uvmexp.softs++;
		intrcnt[SOFTCLOCK_INTR]++;
		softclock(NULL);
	}
}
