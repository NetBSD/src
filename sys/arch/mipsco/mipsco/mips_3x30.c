/*	$NetBSD: mips_3x30.c,v 1.15.6.1 2014/08/20 00:03:13 tls Exp $	*/

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

#define	__INTR_PRIVATE
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mips_3x30.c,v 1.15.6.1 2014/08/20 00:03:13 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <machine/locore.h>
#include <machine/trap.h>
#include <machine/psl.h>
#include <machine/mainboard.h>
#include <machine/sysconf.h>

/* Local functions */
void pizazz_init(void);
void pizazz_intr(uint32_t, vaddr_t, uint32_t);
int  pizazz_level0_intr(void *);
void pizazz_level5_intr(uint32_t, vaddr_t);
void pizazz_intr_establish (int, int (*)(void *), void *);

#define INT_MASK_FPU MIPS_INT_MASK_3

void
pizazz_init(void)
{
	platform.iobus = "obio";
	platform.cons_init = NULL;
	platform.iointr = pizazz_intr;
	platform.intr_establish = pizazz_intr_establish;

	ipl_sr_map = mipsco_ipl_sr_map;

	pizazz_intr_establish(SYS_INTR_LEVEL0, pizazz_level0_intr, NULL);

	cpu_setmodel("Mips 3230 Magnum (Pizazz)");
	cpuspeed = 25;
}

#define	HANDLE_INTR(intr, mask)					\
	do {							\
		if (ipending & (mask)) {			\
			CALL_INTR(intr);			\
		}						\
	} while (0)

void
pizazz_intr(uint32_t status, vaddr_t pc, uint32_t ipending)
{
	/* handle clock interrupts ASAP */
	if (ipending & MIPS_INT_MASK_2) {	        /* Timer Interrupt */
	        void rambo_clkintr (struct clockframe *);
	        struct clockframe cf;
		
		cf.pc = pc;
		cf.sr = status;
		cf.intr = (curcpu()->ci_idepth > 1);

		rambo_clkintr(&cf);
	}

	if (ipending & MIPS_INT_MASK_5)		/* level 5 interrupt */
		pizazz_level5_intr(status, pc);

	HANDLE_INTR(SYS_INTR_FDC,	MIPS_INT_MASK_4);
	HANDLE_INTR(SYS_INTR_SCSI,	MIPS_INT_MASK_1);
	HANDLE_INTR(SYS_INTR_LEVEL0,	MIPS_INT_MASK_0);

#if !defined(NOFPU)
	/* FPU nofiticaition */
	if (ipending & INT_MASK_FPU) {
		if (!USERMODE(status))
			panic("kernel used FPU: PC %x, SR %x",
			      pc, status);
		mips_fpu_intr(pc, curlwp->l_md.md_utf);
	}
#endif
}

/*
 * Level 0 interrupt handler 
 *
 * Pizazz shares Lance, SCC, Expansion slot and Keyboard on level 0
 * A secondary interrupt status register shows the real interrupt source
 */
int
pizazz_level0_intr(void *arg)
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
pizazz_level5_intr(uint32_t status, vaddr_t pc)
{
	u_int32_t ereg;

	ereg = *(u_int32_t *)RAMBO_ERREG;

	printf("interrupt: pc=%p sr=%x\n", (void *)pc, status);
	printf("parity error: %p mask: 0x%x\n", (void *)ereg, ereg & 0xf);
	panic("memory fault");
}

void
pizazz_intr_establish(int level, int (*func) (void *), void *arg)
{
	if (level < 0 || level >= MAX_INTR_COOKIES)
		panic("invalid interrupt level");

	if (intrtab[level].ih_fun != NULL)
		panic("cannot share interrupt %d", level);

	intrtab[level].ih_fun = func;
	intrtab[level].ih_arg = arg;
}
