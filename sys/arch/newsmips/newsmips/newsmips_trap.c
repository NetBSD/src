/*	$NetBSD: newsmips_trap.c,v 1.8 1999/12/18 06:54:05 tsubai Exp $	*/

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

#include "opt_ktrace.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif

#include <machine/trap.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/adrsmap.h>

#include <newsmips/dev/scc.h>

#include "sc.h"
/*#include "lp.h"*/
/*#include "fd.h"*/
/*#include "sb.h"*/
/* XXX hokanimo ippai */

void level0_intr __P((void));
void level1_intr __P((void));
void dma_intr __P((void));
void print_int_stat __P((char *));
void news3400_errintr __P((u_int));

int sc_intr __P((void));
void hb_intr_dispatch __P((int));

static int badaddr_flag;

/*
 * Handle news3400 interrupts.
 */
int
news3400_intr(mask, pc, statusReg, causeReg)
	unsigned mask;
	unsigned pc;		/* program counter where to continue */
	unsigned statusReg;	/* status register at time of the exception */
	unsigned causeReg;	/* cause register at time of exception */
{
	struct clockframe cf;

	/* handle clock interrupts ASAP */
	if (mask & MIPS_INT_MASK_2) {		/* level 2 interrupt */
		register int stat;

		stat = *(volatile u_char *)INTST0;
		stat &= INTST0_TIMINT|INTST0_KBDINT|INTST0_MSINT;
		*(volatile u_char *)INTCLR0 = stat;

		if (stat & INTST0_TIMINT) {	/* timer */
			static int led_count = 0;

			cf.pc = pc;
			cf.sr = statusReg;
			hardclock(&cf);
			intrcnt[HARDCLOCK_INTR]++;
			if (++led_count > hz) {
				led_count = 0;
				*(volatile u_char *)DEBUG_PORT ^= DP_LED1;
			}
			stat &= ~INTST0_TIMINT;
		}

		if (stat)
			hb_intr_dispatch(2);

		/* keep clock interrupts enabled when we return */
		causeReg &= ~MIPS_INT_MASK_2;
	}
	/* If clock interrupts were enabled, re-enable them ASAP. */
	splx(MIPS_SR_INT_ENA_CUR | (statusReg & MIPS_INT_MASK_2));

	if (mask & MIPS_INT_MASK_5) {		/* level 5 interrupt */
		*(char *)INTCLR0 = INTCLR0_PERR;
		news3400_errintr(pc);
		causeReg &= ~MIPS_INT_MASK_5;
	}
	if (mask & MIPS_INT_MASK_4) {		/* level 4 interrupt */
		/*
		 * asynchronous bus error
		 */
		*(char *)INTCLR0 = INTCLR0_BERR;
		causeReg &= ~MIPS_INT_MASK_4;
		badaddr_flag = 1;
	}
	if (mask & MIPS_INT_MASK_1) {		/* level 1 interrupt */
		level1_intr();
		causeReg &= ~MIPS_INT_MASK_1;
	}
	if (mask & MIPS_INT_MASK_0) {		/* level 0 interrupt */
		level0_intr();
		causeReg &= ~MIPS_INT_MASK_0;
	}

	return ((statusReg & ~causeReg & MIPS_HARD_INT_MASK) |
		MIPS_SR_INT_ENA_CUR);
}

void
news3400_errintr(pc)
	u_int pc;
{
	printf("Memory error interrupt(?) at 0x%x\n", pc);
}

#include <newsmips/dev/dmac_0448.h>

/*
 * news3400 - INT0 service routine.
 *
 * INTST0 bit	4:	dma
 *		3:	slot #1
 *		2:	slot #3
 *		1:	external #1
 *		0:	external #3
 */

#define	LEVEL0_MASK	\
	(INTST1_DMA|INTST1_SLOT1|INTST1_SLOT3|INTST1_EXT1|INTST1_EXT3)

void
level0_intr()
{
	register int stat;

	stat = *(volatile u_char *)INTST1 & LEVEL0_MASK;
	*(u_char *)INTCLR1 = stat;

	hb_intr_dispatch(0);

	if (stat & INTST1_SLOT1)
		intrcnt[SLOT1_INTR]++;
	if (stat & INTST1_SLOT3)
		intrcnt[SLOT3_INTR]++;
	if (stat & INTST1_EXT1)
		print_int_stat("EXT #1");
	if (stat & INTST1_EXT3)
		print_int_stat("EXT #3");
}

/*
 * news3400 - INT1 service routine.
 *
 * INTST0 bit	1:	centro fault
 *		0:	centro busy
 * INTST1 bit	7:	beep
 *		6:	scc
 *		5:	lance
 */

#define LEVEL1_MASK2	(INTST0_CFLT|INTST0_CBSY)
#define LEVEL1_MASK1	(INTST1_BEEP|INTST1_SCC|INTST1_LANCE)

void
level1_intr()
{
	register int stat;
	register u_int saved_inten1 = *(u_char *)INTEN1;

	*(u_char *)INTEN1 = 0;		/* disable intr: beep, lance, scc */

	stat = *(volatile u_char *)INTST1 & LEVEL1_MASK1;
	*(u_char *)INTCLR1 = stat;

	stat &= saved_inten1;

	if (stat & INTST1_BEEP) {
		*(volatile u_char *)INTCLR1 = INTCLR1_BEEP;
		print_int_stat("BEEP");
	}
	if (stat & INTST1_SCC)
		intrcnt[SERIAL0_INTR]++;

	if (stat & INTST1_LANCE)
		intrcnt[LANCE_INTR]++;

	hb_intr_dispatch(1);

	*(u_char *)INTEN1 = saved_inten1;

#if NLP > 0
	/*
	 * The PARK2 cannot find centro interrupt correctly.
	 * We must check it by reading the cause register of cpu
	 * while other interrupts are disabled.
	 */
	{
		register int causereg;
		int s = splhigh();

		causereg = get_causereg();
		(void) splx(s);

		if ((causereg & CAUSE_IP4) == 0)
			return;
	}
#endif

	stat = (int)(*(u_char *)INTST0) & LEVEL1_MASK2;
	*(u_char *)INTCLR0 = stat;

	if (stat & INTST0_CBSY)		/* centro busy */
#if NLP > 0
		lpxint(0);
#else
		printf("stray intr: CBSY\n");
#endif
}

/*
 * DMA interrupt service routine.
 * XXX only SCSI works now.
 */
void
dma_intr()
{
        register volatile u_char *gsp = (u_char *)DMAC_GSTAT;
        register u_int gstat = *gsp;
        register int mrqb, i;

	/*
	 * when DMA intrrupt occurs there remain some untransferred data.
	 * wait data transfer completion.
	 */
	mrqb = (gstat & (CH0_INT|CH1_INT|CH2_INT|CH3_INT)) << 1;
	if (gstat & mrqb) {
		/*
		 * SHOULD USE DELAY()
		 */
		for (i = 0; i < 50; i++)
			;
		if (*gsp & mrqb)
			printf("dma_intr: MRQ\n");
	}

#if NSC > 0
	/* SCSI Dispatch */
	if (gstat & CH_INT(CH_SCSI))
		sc_intr();
#endif

#if NFD > 0
        /* FDC Interrupt Dispatch */
	if (gstat & CH_INT(CH_FDC))
		fdc_intr(0);
#endif /* NFD > 0 */

#if NSB > 0
        /* Audio Interface Dispatch */
	sbintr(0);
#endif /* NSB > 0 */

        /* Video I/F Dispatch */
	if (gstat & CH_INT(CH_VIDEO))
		;
}

void
print_int_stat(msg)
	char *msg;
{
	int s0 = *(volatile u_char *)INTST0;
	int s1 = *(volatile u_char *)INTST1;

	if (msg)
		printf("%s: ", msg);
	else
		printf("intr: ");
	printf("INTST0=0x%x, INTST1=0x%x.\n", s0, s1);
}

int
news3400_badaddr(addr, size)
	void *addr;
	u_int size;
{
	volatile int x;

	badaddr_flag = 0;

	switch (size) {
	case 1:
		x = *(volatile int8_t *)addr;
		break;
	case 2:
		x = *(volatile int16_t *)addr;
		break;
	case 4:
		x = *(volatile int32_t *)addr;
		break;
	}

	return badaddr_flag;
}
