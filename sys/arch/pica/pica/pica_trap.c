/*	$NetBSD: pica_trap.c,v 1.5 1997/06/23 02:56:50 jonathan Exp $	*/

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
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/device.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif
#include <net/netisr.h>

#include <machine/trap.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/cpu.h>
#include <machine/pio.h>
#include <machine/autoconf.h>
#include <machine/pte.h>
#include <machine/pmap.h>
#include <machine/mips_opcode.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <pica/pica/pica.h>

#include <sys/cdefs.h>
#include <sys/syslog.h>

struct	proc *machFPCurProcPtr;		/* pointer to last proc to use FP */

struct {
	int	int_mask;
	int	(*int_hand)();
} cpu_int_tab[8];

int cpu_int_mask;	/* External cpu interrupt mask */

int
pica_hardware_intr(mask, pc, statusReg, causeReg)
	unsigned mask;
	unsigned pc;
	unsigned statusReg;
	unsigned causeReg;
{
	register int i;
	/*
	 *  Check off all enabled interrupts. Called interrupt routine
	 *  returns mask of interrupts to reenable.
	 */
	for(i = 0; i < 5; i++) {
		if(cpu_int_tab[i].int_mask & mask) {
			causeReg &= (*cpu_int_tab[i].int_hand)(mask, pc, statusReg, causeReg);
		}
	}

	/*
	 *  Reenable all non served hardware levels.
	 */
	return ((statusReg & ~causeReg & MIPS_HARD_INT_MASK) |
		 MIPS_SR_INT_ENAB);
}


void
set_intr(mask, int_hand, prio)
	int	mask;
	int	(*int_hand)();
	int	prio;
{
	if(prio > 4)
		panic("set_intr: to high priority");

	if(cpu_int_tab[prio].int_mask != 0)
		panic("set_intr: int already set");

	cpu_int_tab[prio].int_hand = int_hand;
	cpu_int_tab[prio].int_mask = mask;
	cpu_int_mask |= mask >> 10;

	/*
	 *  Update external interrupt mask but dont enable clock.
	 */
	out32(PICA_SYS_EXT_IMASK, cpu_int_mask & (~MIPS_INT_MASK_4 >> 10));
}


/*
 *----------------------------------------------------------------------
 *
 * MemErrorInterrupts --
 *   pica_errintr - for the ACER PICA_61
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
pica_errintr()
{

	/*
	 * XXX
	 * how does the Pica report memory errors?
	 * how should we handle them?
	 */
#if 0
	volatile u_short *sysCSRPtr =
		(u_short *)MACH_PHYS_TO_UNCACHED(KN01_SYS_CSR);
	u_short csr;

	csr = *sysCSRPtr;

	if (csr & KN01_CSR_MERR) {
		printf("Memory error at 0x%x\n",
			*(unsigned *)MACH_PHYS_TO_UNCACHED(KN01_SYS_ERRADR));
		panic("Mem error interrupt");
	}
	*sysCSRPtr = (csr & ~KN01_CSR_MBZ) | 0xff;
#endif
}
