/*	$NetBSD: mips_3x30.c,v 1.5 2001/03/30 23:51:14 wdk Exp $	*/

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

extern void MachFPInterrupt (u_int, u_int, u_int, struct frame *);

/* Local functions */
void pizazz_init (void);
void pizazz_intr (u_int, u_int, u_int, u_int);
int  pizazz_level0_intr (void *);
void pizazz_level5_intr (int, int, int);
void pizazz_intr_establish  (int, int (*)(void *), void *);

#define INT_MASK_FPU MIPS_INT_MASK_3

void
pizazz_init(void)
{
	platform.iobus = "obio";
	platform.cons_init = NULL;
	platform.iointr = pizazz_intr;
	platform.intr_establish = pizazz_intr_establish;

	pizazz_intr_establish(SYS_INTR_LEVEL0, pizazz_level0_intr, NULL);

	strcpy(cpu_model, "Mips 3230 Magnum (Pizazz)");
	cpuspeed = 25;
}

#define	HANDLE_INTR(intr, mask)					\
	do {							\
		if (ipending & (mask)) {			\
			CALL_INTR(intr);			\
		}						\
	} while (0)

void
pizazz_intr(status, cause, pc, ipending)
	u_int status;	/* status register at time of the exception */
	u_int cause;	/* cause register at time of exception */
	u_int pc;	/* program counter where to continue */
	u_int ipending;
{
	/* handle clock interrupts ASAP */
	if (ipending & MIPS_INT_MASK_2) {	        /* Timer Interrupt */
	        void rambo_clkintr (struct clockframe *);
	        struct clockframe cf;
		
		cf.pc = pc;
		cf.sr = status;

		rambo_clkintr(&cf);

		/* keep clock interrupts enabled when we return */
		cause &= ~MIPS_INT_MASK_2;
	}

	/* If clock interrupts were enabled, re-enable them ASAP. */
	_splset(MIPS_SR_INT_IE | (status & MIPS_INT_MASK_2));

	if (ipending & MIPS_INT_MASK_5)		/* level 5 interrupt */
		pizazz_level5_intr(pc, cause, status);

	HANDLE_INTR(SYS_INTR_FDC,	MIPS_INT_MASK_4);
	HANDLE_INTR(SYS_INTR_SCSI,	MIPS_INT_MASK_1);
	HANDLE_INTR(SYS_INTR_LEVEL0,	MIPS_INT_MASK_0);

	/* XXX:  Keep FDC interrupt masked off */
	cause &= ~(MIPS_INT_MASK_0 | MIPS_INT_MASK_1 | MIPS_INT_MASK_5);

	_splset((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);

	/* FPU nofiticaition */
	if (ipending & INT_MASK_FPU) {
		if (!USERMODE(status))
			panic("kernel used FPU: PC %x, CR %x, SR %x",
			      pc, cause, status);
		MachFPInterrupt(status, cause, pc, curproc->p_md.md_regs);
	}
}

/*
 * Level 0 interrupt handler 
 *
 * Pizazz shares Lance, SCC, Expansion slot and Keyboard on level 0
 * A secondary interrupt status register shows the real interrupt source
 */
int
pizazz_level0_intr(arg)
	void *arg;
{
	register int stat;

	/* stat register is active low */
	stat = ~*(volatile u_char *)INTREG_0;

	if (stat & INT_ExpSlot)
		CALL_INTR(SYS_INTR_ATBUS);

	if (stat & INT_Lance)
		CALL_INTR(SYS_INTR_ETHER);

	if (stat & INT_SCC)
		CALL_INTR(SYS_INTR_SCC0);

	return 0;
}

/*
 * Motherboard Parity Error
 */
void
pizazz_level5_intr(pc, cause, status)
	int pc;
	int cause;
	int status;
{
	u_int32_t ereg;

	ereg = *(u_int32_t *)RAMBO_ERREG;

	printf("interrupt: pc=%p cr=%x sr=%x\n", (void *)pc, cause, status);
	printf("parity error: %p mask: 0x%x\n", (void *)ereg, ereg & 0xf);
	panic("memory fault");
}

void
pizazz_intr_establish(level, func, arg)
	int level;
	int (*func) (void *);
	void *arg;
{
	if (level < 0 || level >= MAX_INTR_COOKIES)
		panic("invalid interrupt level");

	if (intrtab[level].ih_fun != NULL)
		panic("cannot share interrupt %d", level);

	intrtab[level].ih_fun = func;
	intrtab[level].ih_arg = arg;
}
