/*	$NetBSD: news3400.c,v 1.22.14.2 2017/12/03 11:36:33 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: news3400.c,v 1.22.14.2 2017/12/03 11:36:33 jdolecek Exp $");

#define __INTR_PRIVATE
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <mips/locore.h>

#include <machine/adrsmap.h>
#include <machine/psl.h>
#include <newsmips/newsmips/machid.h>

#include <newsmips/dev/hbvar.h>

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
 * This is a mask of bits to clear in the SR when we go to a
 * given interrupt priority level.
 */
static const struct ipl_sr_map news3400_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] =		0,
	[IPL_SOFTCLOCK] =	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =		MIPS_SOFT_INT_MASK,
	[IPL_VM] =		MIPS_SOFT_INT_MASK
				| MIPS_INT_MASK_0
				| MIPS_INT_MASK_1,
	[IPL_SCHED] =		MIPS_SOFT_INT_MASK
				| MIPS_INT_MASK_0
				| MIPS_INT_MASK_1
				| MIPS_INT_MASK_2,
	[IPL_DDB] =		MIPS_INT_MASK,
	[IPL_HIGH] =		MIPS_INT_MASK,
    },
};

/*
 * Handle news3400 interrupts.
 */
void
news3400_intr(int ppl, uint32_t pc, uint32_t status)
{
	uint32_t ipending;
	int ipl;

	while (ppl < (ipl = splintr(&ipending))) {

	/* handle clock interrupts ASAP */
		if (ipending & MIPS_INT_MASK_2) {
			int stat;

			stat = *(volatile uint8_t *)INTST0;
			stat &= INTST0_TIMINT|INTST0_KBDINT|INTST0_MSINT;

			*(volatile uint8_t *)INTCLR0 = stat;
			if (stat & INTST0_TIMINT) {
				struct clockframe cf = {
					.pc = pc,
					.sr = status,
					.intr = (curcpu()->ci_idepth > 1),
				};
				hardclock(&cf);
				intrcnt[HARDCLOCK_INTR]++;
			}

			if (stat)
				hb_intr_dispatch(2, stat);

		}

		if (ipending & MIPS_INT_MASK_5) {
			*(volatile uint8_t *)INTCLR0 = INTCLR0_PERR;
			printf("Memory error interrupt(?) at 0x%x\n", pc);
		}

		/* asynchronous bus error */
		if (ipending & MIPS_INT_MASK_4) {
			*(volatile uint8_t *)INTCLR0 = INTCLR0_BERR;
			badaddr_flag = 1;
		}

		if (ipending & MIPS_INT_MASK_1) {
			news3400_level1_intr();
		}

		if (ipending & MIPS_INT_MASK_0) {
			news3400_level0_intr();
		}

		/* FPU notification */
		if (ipending & INT_MASK_FPU) {
			if (!USERMODE(status))
				panic("kernel used FPU: PC %x, SR %x",
				      pc, status);

			intrcnt[FPU_INTR]++;
#if !defined(FPEMUL)
			mips_fpu_intr(pc, curlwp->l_md.md_utf);
#endif
		}
	}
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
	uint32_t cause;
	volatile u_int x;
	volatile uint8_t *intclr0 = (void *)INTCLR0;

	badaddr_flag = 0;

	/* clear bus error interrupt */
	*intclr0 = INTCLR0_BERR;

	/* bus error will cause INT4 */
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
	__USE(x);

	/* also check CPU INT4 here for bus errors during splhigh() */
	if (badaddr_flag == 0) {
		cause = mips_cp0_cause_read();
		if ((cause & MIPS_INT_MASK_4) != 0) {
			badaddr_flag = 1;
			*intclr0 = INTCLR0_BERR;
		}
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

	/* always enable bus error check so that news3400_badaddr() works */
	*inten0 = INTEN0_BERR;
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

	ipl_sr_map = news3400_ipl_sr_map;

	enable_intr = news3400_enable_intr;
	disable_intr = news3400_disable_intr;
	enable_timer = news3400_enable_timer;

	news3400_readidrom((uint8_t *)&idrom);
	hostid = idrom.id_serial;
}
