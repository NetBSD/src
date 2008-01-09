/*	$NetBSD: news3400.c,v 1.18.32.1 2008/01/09 01:47:32 matt Exp $	*/

/*-
 * Copyright (C) 1999 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: news3400.c,v 1.18.32.1 2008/01/09 01:47:32 matt Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <machine/adrsmap.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/psl.h>
#include <newsmips/newsmips/machid.h>

#include <newsmips/dev/hbvar.h>

#if !defined(SOFTFLOAT)
extern void MachFPInterrupt(unsigned, unsigned, unsigned, struct frame *);
#endif

int news3400_badaddr(void *, u_int);

static void news3400_level0_intr(void);
static void news3400_level1_intr(void);
static void news3400_enable_intr(void);
static void news3400_disable_intr(void);
static void news3400_enable_timer(void);
static void news3400_readidrom(uint8_t *);

static volatile int badaddr_flag;

#define INT_MASK_FPU MIPS_INT_MASK_3

/*
 * Handle news3400 interrupts.
 */
void
news3400_intr(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{
	struct clockframe cf;
	struct cpu_info *ci;

	ci = curcpu();
	ci->ci_idepth++;

	/* handle clock interrupts ASAP */
	if (ipending & MIPS_INT_MASK_2) {
		int stat;

		stat = *(volatile uint8_t *)INTST0;
		stat &= INTST0_TIMINT|INTST0_KBDINT|INTST0_MSINT;

		*(volatile uint8_t *)INTCLR0 = stat;
		if (stat & INTST0_TIMINT) {
			cf.pc = pc;
			cf.sr = status;
			hardclock(&cf);
			intrcnt[HARDCLOCK_INTR]++;
			stat &= ~INTST0_TIMINT;
		}

		if (stat)
			hb_intr_dispatch(2, stat);

		cause &= ~MIPS_INT_MASK_2;
	}
	/* If clock interrupts were enabled, re-enable them ASAP. */
	_splset(MIPS_SR_INT_IE | (status & MIPS_INT_MASK_2));

	if (ipending & MIPS_INT_MASK_5) {
		*(volatile uint8_t *)INTCLR0 = INTCLR0_PERR;
		printf("Memory error interrupt(?) at 0x%x\n", pc);
		cause &= ~MIPS_INT_MASK_5;
	}

	/* asynchronous bus error */
	if (ipending & MIPS_INT_MASK_4) {
		*(volatile uint8_t *)INTCLR0 = INTCLR0_BERR;
		cause &= ~MIPS_INT_MASK_4;
		badaddr_flag = 1;
	}

	if (ipending & MIPS_INT_MASK_1) {
		news3400_level1_intr();
		cause &= ~MIPS_INT_MASK_1;
	}

	if (ipending & MIPS_INT_MASK_0) {
		news3400_level0_intr();
		cause &= ~MIPS_INT_MASK_0;
	}

	_splset((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);

	/* FPU nofiticaition */
	if (ipending & INT_MASK_FPU) {
		if (!USERMODE(status))
			panic("kernel used FPU: PC %x, CR %x, SR %x",
			      pc, cause, status);

		intrcnt[FPU_INTR]++;
#if !defined(SOFTFLOAT)
		MachFPInterrupt(status, cause, pc, curlwp->l_md.md_regs);
#endif
	}

	ci->ci_idepth--;
}

#define LEVEL0_MASK \
	(INTST1_DMA|INTST1_SLOT1|INTST1_SLOT3|INTST1_EXT1|INTST1_EXT3)

static void
news3400_level0_intr(void)
{
	volatile uint8_t *intst1 = (void *)INTST1;
	volatile uint8_t *intclr1 = (void *)INTCLR1;
	uint8_t stat;

	stat = *intst1 & LEVEL0_MASK;
	*intclr1 = stat;

	hb_intr_dispatch(0, stat);

	if (stat & INTST1_SLOT1)
		intrcnt[SLOT1_INTR]++;
	if (stat & INTST1_SLOT3)
		intrcnt[SLOT3_INTR]++;
}

#define LEVEL1_MASK0	(INTST0_CFLT|INTST0_CBSY)
#define LEVEL1_MASK1	(INTST1_BEEP|INTST1_SCC|INTST1_LANCE)

static void
news3400_level1_intr(void)
{
	volatile uint8_t *inten1 = (void *)INTEN1;
	volatile uint8_t *intst1 = (void *)INTST1;
	volatile uint8_t *intclr1 = (void *)INTCLR1;
	uint8_t stat1, saved_inten1;

	saved_inten1 = *inten1;

	*inten1 = 0;		/* disable BEEP, LANCE, and SCC */

	stat1 = *intst1 & LEVEL1_MASK1;
	*intclr1 = stat1;

	stat1 &= saved_inten1;

	hb_intr_dispatch(1, stat1);

	*inten1 = saved_inten1;

	if (stat1 & INTST1_SCC)
		intrcnt[SERIAL0_INTR]++;
	if (stat1 & INTST1_LANCE)
		intrcnt[LANCE_INTR]++;
}

int
news3400_badaddr(void *addr, u_int size)
{
	volatile u_int x;

	badaddr_flag = 0;

	switch (size) {
	case 1:
		x = *(volatile uint8_t *)addr;
		break;
	case 2:
		x = *(volatile uint16_t *)addr;
		break;
	case 4:
		x = *(volatile uint32_t *)addr;
		break;
	}

	return badaddr_flag;
}

static void
news3400_enable_intr(void)
{
	volatile uint8_t *inten0 = (void *)INTEN0;
	volatile uint8_t *inten1 = (void *)INTEN1;
	volatile uint8_t *intclr0 = (void *)INTCLR0;
	volatile uint8_t *intclr1 = (void *)INTCLR1;

	/* clear all interrupts */
	*intclr0 = 0xff;
	*intclr1 = 0xff;

	/*
	 * It's not a time to enable timer yet.
	 *
	 *	INTEN0:  PERR ABORT BERR TIMER KBD  MS    CFLT CBSY
	 *		  o     o    o     x    o    o     x    x
	 *	INTEN1:  BEEP SCC  LANCE DMA  SLOT1 SLOT3 EXT1 EXT3
	 *		  x     o    o     o    o    o     x    x
	 */

	*inten0 = INTEN0_PERR | INTEN0_ABORT | INTEN0_BERR |
		  INTEN0_KBDINT | INTEN0_MSINT;

	*inten1 = INTEN1_SCC | INTEN1_LANCE | INTEN1_DMA |
		  INTEN1_SLOT1 | INTEN1_SLOT3;
}

static void
news3400_disable_intr(void)
{

	volatile uint8_t *inten0 = (void *)INTEN0;
	volatile uint8_t *inten1 = (void *)INTEN1;

	*inten0 = 0;
	*inten1 = 0;
}

static void
news3400_enable_timer(void)
{

	/* initialize interval timer */
	*(volatile uint8_t *)ITIMER = IOCLOCK / 6144 / 100 - 1;

	/* enable timer interrupt */
	*(volatile uint8_t *)INTEN0 |= (uint8_t)INTEN0_TIMINT;
}

static void
news3400_readidrom(uint8_t *rom)
{
	uint8_t *p = (uint8_t *)IDROM;
	int i;

	for (i = 0; i < sizeof (struct idrom); i++, p += 2)
		*rom++ = ((*p & 0x0f) << 4) + (*(p + 1) & 0x0f);
}

extern struct idrom idrom;

void
news3400_init(void)
{

	enable_intr = news3400_enable_intr;
	disable_intr = news3400_disable_intr;
	enable_timer = news3400_enable_timer;

	news3400_readidrom((uint8_t *)&idrom);
	hostid = idrom.id_serial;
}
