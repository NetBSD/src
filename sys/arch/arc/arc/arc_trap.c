/*	$NetBSD: arc_trap.c,v 1.10 2000/03/03 12:50:19 soda Exp $	*/
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
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/buf.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#include <net/netisr.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <machine/trap.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/cpu.h>
#include <machine/pio.h>
#include <machine/autoconf.h>
#include <machine/mips_opcode.h>

#include <arc/pica/pica.h>
#include <arc/pica/rd94.h>
#include <arc/arc/arctype.h>

#include <sys/cdefs.h>
#include <sys/syslog.h>

#define	MIPS_INT_LEVELS	8

struct {
	int	int_mask;
	int	(*int_hand)(u_int, struct clockframe *);
} cpu_int_tab[MIPS_INT_LEVELS];

int cpu_int_mask;	/* External cpu interrupt mask */

int
arc_hardware_intr(mask, pc, statusReg, causeReg)
	unsigned mask;
	unsigned pc;
	unsigned statusReg;
	unsigned causeReg;
{
	register int i;
	struct clockframe cf;

	cf.pc = pc;
	cf.sr = statusReg;
#if 0 /* remove this for merged mips trap code. (this member isn't used) */
	cf.cr = causeReg;
#endif

	/*
	 *  Check off all enabled interrupts. Called interrupt routine
	 *  returns mask of interrupts to reenable.
	 */
	for(i = 0; i < MIPS_INT_LEVELS; i++) {
		if(cpu_int_tab[i].int_mask & mask) {
			causeReg &= (*cpu_int_tab[i].int_hand)(mask, &cf);
		}
	}

	/*
	 *  Reenable all non served hardware levels.
	 */
	return ((statusReg & ~causeReg & MIPS3_HARD_INT_MASK) | MIPS_SR_INT_IE);
}

/*
 *	Set up handler for external interrupt events.
 *	Events are checked in priority order.
 */
void
set_intr(mask, int_hand, prio)
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

	/*
	 *  Update external interrupt mask but don't enable clock.
	 */
	switch(cputype) {
	case ACER_PICA_61:
	case MAGNUM:
		out32(R4030_SYS_EXT_IMASK,
		    cpu_int_mask & (~MIPS_INT_MASK_4 >> 10));
		break;
	case NEC_RD94:
		out32(RD94_SYS_EXT_IMASK,
		    cpu_int_mask & (~MIPS_INT_MASK_3 >> 10));
		break;
	case DESKSTATION_TYNE:
		break;
	case DESKSTATION_RPC44:
		break;
	case ALGOR_P4032:
	case ALGOR_P5064:
		break;
	}
}

#if 0
/*
 *----------------------------------------------------------------------
 *
 * MemErrorInterrupts --
 *   arc_errintr - for the ACER PICA_61
 *
 *	Handler an interrupt for the control register.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
arc_errintr()
{
	volatile u_short *sysCSRPtr =
		(u_short *)MIPS_PHYS_TO_KSEG1(KN01_SYS_CSR);
	u_short csr;

	csr = *sysCSRPtr;

	if (csr & KN01_CSR_MERR) {
		printf("Memory error at 0x%x\n",
			*(unsigned *)MIPS_PHYS_TO_KSEG1(KN01_SYS_ERRADR));
		panic("Mem error interrupt");
	}
	*sysCSRPtr = (csr & ~KN01_CSR_MBZ) | 0xff;
}
#endif
