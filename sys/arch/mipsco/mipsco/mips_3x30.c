/*	$NetBSD: mips_3x30.c,v 1.1 2000/08/12 22:58:54 wdk Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/kernel.h>

#include <machine/trap.h>
#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/mainboard.h>
#include <machine/sysconf.h>

#include "le.h"

extern void asc_intr __P(());
extern void kbm_rint __P((int));

extern void MachFPInterrupt __P((u_int, u_int, u_int, struct frame *));

/* Local functions */
void pizazz_intr __P((u_int, u_int, u_int, u_int));
void pizazz_level0_intr __P((void));

void
pizazz_init()
{
	platform.iobus = "obio";
	platform.cons_init = NULL;
	platform.iointr = pizazz_intr;

	strcpy(cpu_model, "Mips 3230 Magnum (Pizazz)");
	cpuspeed = 25;
}

void
pizazz_intr(status, cause, pc, ipending)
	u_int status;	/* status register at time of the exception */
	u_int cause;	/* cause register at time of exception */
	u_int pc;	/* program counter where to continue */
	u_int ipending;
{
	/* handle clock interrupts ASAP */
	if (ipending & MIPS_INT_MASK_2) {	        /* Timer Interrupt */
	        void rambo_clkintr __P((struct clockframe *));
	        struct clockframe cf;
		
		cf.pc = pc;
		cf.sr = status;

		rambo_clkintr(&cf);

		/* keep clock interrupts enabled when we return */
		cause &= ~MIPS_INT_MASK_2;
	}

	/* If clock interrupts were enabled, re-enable them ASAP. */
	_splset(MIPS_SR_INT_IE | (status & MIPS_INT_MASK_2));

	if (ipending & MIPS_INT_MASK_5) {		/* level 5 interrupt */
	    printf("level 5 interrupt: PC %x CR %x SR %x\n",
		   pc, cause, status);
	    /* cause &= ~MIPS_INT_MASK_5; */
	    /* panic("level 5 interrupt"); */
	}
	if (ipending & MIPS_INT_MASK_4) {		/* level 4 interrupt */
		void fdintr __P((int));
		fdintr(0);
		/*	cause &= ~MIPS_INT_MASK_4; */
	}
	if (ipending & MIPS_INT_MASK_1) {		/* level 1 interrupt */
		asc_intr();
		cause &= ~MIPS_INT_MASK_1;
	}
	if (ipending & MIPS_INT_MASK_0) {		/* level 0 interrupt */
		pizazz_level0_intr();
		cause &= ~MIPS_INT_MASK_0;
	}

	_splset((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);

	/* FPU nofiticaition */
	if (ipending & MIPS_INT_MASK_3) {
		if (!USERMODE(status))
			panic("kernel used FPU: PC %x, CR %x, SR %x",
			      pc, cause, status);
		/* dealfpu(status, cause, pc); */
		MachFPInterrupt(status, cause, pc, curproc->p_md.md_regs);
	}
}


/*
 * Level 0 interrupt handler 
 */
void
pizazz_level0_intr()
{
	register int stat;

	/* stat register is active low */
	stat = ~*(volatile u_char *)INTREG_0;

	if (stat & INT_Lance) {
		extern int leintr __P((int));
		int s = splimp();

		leintr(0);
		splx(s);
	}
	if (stat & INT_SCC) {
		extern void zs_intr __P((void));

		zs_intr();
	}
}



