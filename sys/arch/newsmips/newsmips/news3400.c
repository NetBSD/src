/*	$NetBSD: news3400.c,v 1.4.8.1 2002/06/24 22:06:36 nathanw Exp $	*/

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

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <machine/adrsmap.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/psl.h>
#include <newsmips/newsmips/machid.h>

void level0_intr (void);
void level1_intr (void);
void hb_intr_dispatch (int);
void MachFPInterrupt (unsigned, unsigned, unsigned, struct frame *);

int news3400_badaddr (void *, u_int);
static void enable_intr_3400 (void);
static void disable_intr_3400 (void);
static void readidrom_3400 (u_char *);
void news3400_init (void);

static int badaddr_flag;

#define INT_MASK_FPU MIPS_INT_MASK_3

/*
 * Handle news3400 interrupts.
 */
void
news3400_intr(status, cause, pc, ipending)
	u_int status;	/* status register at time of the exception */
	u_int cause;	/* cause register at time of exception */
	u_int pc;	/* program counter where to continue */
	u_int ipending;
{
	struct clockframe cf;

	/* handle clock interrupts ASAP */
	if (ipending & MIPS_INT_MASK_2) {
		register int stat;

		stat = *(volatile u_char *)INTST0;
		stat &= INTST0_TIMINT|INTST0_KBDINT|INTST0_MSINT;

		*(volatile u_char *)INTCLR0 = stat;
		if (stat & INTST0_TIMINT) {
			cf.pc = pc;
			cf.sr = status;
			hardclock(&cf);
			intrcnt[HARDCLOCK_INTR]++;
			stat &= ~INTST0_TIMINT;
		}

		if (stat)
			hb_intr_dispatch(2);

		cause &= ~MIPS_INT_MASK_2;
	}
	/* If clock interrupts were enabled, re-enable them ASAP. */
	_splset(MIPS_SR_INT_IE | (status & MIPS_INT_MASK_2));

	if (ipending & MIPS_INT_MASK_5) {
		*(volatile char *)INTCLR0 = INTCLR0_PERR;
		printf("Memory error interrupt(?) at 0x%x\n", pc);
		cause &= ~MIPS_INT_MASK_5;
	}

	/* asynchronous bus error */
	if (ipending & MIPS_INT_MASK_4) {
		*(volatile char *)INTCLR0 = INTCLR0_BERR;
		cause &= ~MIPS_INT_MASK_4;
		badaddr_flag = 1;
	}

	if (ipending & MIPS_INT_MASK_1) {
		level1_intr();
		cause &= ~MIPS_INT_MASK_1;
	}

	if (ipending & MIPS_INT_MASK_0) {
		level0_intr();
		cause &= ~MIPS_INT_MASK_0;
	}

	_splset((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);

	/* FPU nofiticaition */
	if (ipending & INT_MASK_FPU) {
		if (!USERMODE(status))
			panic("kernel used FPU: PC %x, CR %x, SR %x",
			      pc, cause, status);

		intrcnt[FPU_INTR]++;
		/* dealfpu(status, cause, pc); */
		MachFPInterrupt(status, cause, pc, curlwp->p_md.md_regs);
	}
}

#define LEVEL0_MASK \
	(INTST1_DMA|INTST1_SLOT1|INTST1_SLOT3|INTST1_EXT1|INTST1_EXT3)

void
level0_intr()
{
	volatile u_char *istat1 = (void *)INTST1;
	volatile u_char *iclr1 = (void *)INTCLR1;
	int stat;

	stat = *istat1 & LEVEL0_MASK;
	*iclr1 = stat;

	hb_intr_dispatch(0);

	if (stat & INTST1_SLOT1)
		intrcnt[SLOT1_INTR]++;
	if (stat & INTST1_SLOT3)
		intrcnt[SLOT3_INTR]++;
}

#define LEVEL1_MASK0	(INTST0_CFLT|INTST0_CBSY)
#define LEVEL1_MASK1	(INTST1_BEEP|INTST1_SCC|INTST1_LANCE)

void
level1_intr()
{
	volatile u_char *ien1 = (void *)INTEN1;
	volatile u_char *istat1 = (void *)INTST1;
	volatile u_char *iclr1 = (void *)INTCLR1;
	int stat1, saved_ie1;

	saved_ie1 = *ien1;

	*ien1 = 0;		/* disable BEEP, LANCE, and SCC */

	stat1 = *istat1 & LEVEL1_MASK1;
	*iclr1 = stat1;

	stat1 &= saved_ie1;

	hb_intr_dispatch(1);

	*ien1 = saved_ie1;

	if (stat1 & INTST1_SCC)
		intrcnt[SERIAL0_INTR]++;
	if (stat1 & INTST1_LANCE)
		intrcnt[LANCE_INTR]++;
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

static void
enable_intr_3400(void)
{
	volatile u_int8_t *inten0 = (void *)INTEN0;
	volatile u_int8_t *inten1 = (void *)INTEN1;
	volatile u_int8_t *intclr0 = (void *)INTCLR0;
	volatile u_int8_t *intclr1 = (void *)INTCLR1;

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
disable_intr_3400(void)
{
	volatile u_int8_t *inten0 = (void *)INTEN0;
	volatile u_int8_t *inten1 = (void *)INTEN1;

	*inten0 = 0;
	*inten1 = 0;
}

static void
readidrom_3400(rom)
	register u_char *rom;
{
	register u_char *p = (u_char *)IDROM;
	register int i;

	for (i = 0; i < sizeof (struct idrom); i++, p += 2)
		*rom++ = ((*p & 0x0f) << 4) + (*(p + 1) & 0x0f);
}

extern struct idrom idrom;

void
news3400_init()
{
	enable_intr = enable_intr_3400;
	disable_intr = disable_intr_3400;

	readidrom_3400((u_char *)&idrom);
	hostid = idrom.id_serial;
}
