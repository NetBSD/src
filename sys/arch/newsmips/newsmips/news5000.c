/*	$NetBSD: news5000.c,v 1.1 1999/12/22 05:53:21 tsubai Exp $	*/

/*-
 * Copyright (C) 1999 SHIMIZU Ryo.  All rights reserved.
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
#include <sys/systm.h>
#include <sys/kernel.h>

#include <machine/adrsmap.h>
#include <machine/cpu.h>
#include <machine/intr.h>

#include <newsmips/apbus/apbusvar.h>
#include <newsmips/newsmips/machid.h>

static void level5_interrupt __P((void));
static void level4_interrupt __P((void));
static void level3_interrupt __P((void));
static void level2_interrupt __P((unsigned int, unsigned int));
static void level1_interrupt __P((void));
static void level0_interrupt __P((void));

static void abif_error4 __P((void));

/*
 * Handle news5000 interrupts.
 */
int
news5000_intr(mask, pc, statusReg, causeReg)
	u_int mask;
	u_int pc;		/* program counter where to continue */
	u_int statusReg;	/* status register at time of the exception */
	u_int causeReg;		/* cause register at time of exception */
{
	if (mask & MIPS_INT_MASK_2) {
		level2_interrupt(pc,statusReg);
		apbus_wbflush();
		causeReg &= ~MIPS_INT_MASK_2;
	}
	/* If clock interrupts were enabled, re-enable them ASAP. */
	splx(MIPS_SR_INT_IE | (statusReg & MIPS_INT_MASK_2));

	if (mask & MIPS_INT_MASK_5) {
		level5_interrupt();
		apbus_wbflush();
		causeReg &= ~MIPS_INT_MASK_5;
	}

	if (mask & MIPS_INT_MASK_4) {
		level4_interrupt();
		apbus_wbflush();
		causeReg &= ~MIPS_INT_MASK_4;
	}

	if (mask & MIPS_INT_MASK_3) {
		level3_interrupt();
		apbus_wbflush();
		causeReg &= ~MIPS_INT_MASK_3;
	}

	if (mask & MIPS_INT_MASK_1) {
		level1_interrupt();
		apbus_wbflush();
		causeReg &= ~MIPS_INT_MASK_1;
	}

	if (mask & MIPS_INT_MASK_0) {
		level0_interrupt();
		apbus_wbflush();
		causeReg &= ~MIPS_INT_MASK_0;
	}

	return ((statusReg & ~causeReg & MIPS_HARD_INT_MASK) |  MIPS_SR_INT_IE);
}

void
level5_interrupt()
{
	printf("level5 interrupt\n");
}

void
level4_interrupt()
{
	volatile int *intenp = (volatile int *)NEWS5000_INTMASK4;
	int int4stat, saved_inten;

	int4stat = *(volatile int *)NEWS5000_INTSTAT4;
	saved_inten = *intenp;
	*intenp = 0;
	if (int4stat & NEWS5000_INT4_SBIF) {
printf("SBIF ERROR\n");
		/* sbif_error4(); */
	} else if (int4stat & NEWS5000_INT4_MFBIF) {
printf("MBIF ERROR\n");
/*
		mbif_error4();
		if (fxif_error4)
		    fxif_error4();
*/
	} else if (int4stat & NEWS5000_INT4_ABIF) {
/* printf("ABIF ERROR\n"); */
		abif_error4();
	}
	*intenp = saved_inten;
}

static void
abif_error4()
{
#define	AB_BERRADR	0xb4c00018
#define	AB_BERRDATA	0xb4c00020
#define	AB_BERRPARITY	0xb4c00028

	int stat;

printf("abif_error4\n");

	stat = *(volatile u_int *)NEWS5000_APBUS_INTSTAT &
	       *(volatile u_int *)NEWS5000_APBUS_INTMASK;

	if (stat & NEWS5000_INTAPBUS_DMAADDRESS) {
		printf("AB I/F: DMA address error\n");
	}
	if (stat & (NEWS5000_INTAPBUS_RDTIMEO|NEWS5000_INTAPBUS_WRTIMEO)) {
		if (stat & NEWS5000_INTAPBUS_RDTIMEO)
			printf("AB I/F: read timeout\n");
		if (stat & NEWS5000_INTAPBUS_WRTIMEO)
			printf("AB I/F: write timeout\n");

		printf("	address = %x:%x\n",
		    *(volatile u_int *)AB_BERRADR, *(volatile u_int *)(AB_BERRADR + 4));
	}
	*(volatile u_int *)NEWS5000_APBUS_INTSTAT = stat;
}

void
level3_interrupt()
{
	printf("level3 interrupt\n");
}

void
level2_interrupt(pc, statusReg)
	unsigned int pc;
	unsigned int statusReg;
{
#ifdef DEBUG
	static int l2cnt = 0;
#endif
	unsigned int int2stat;
	struct clockframe cf;

	int2stat = *(volatile unsigned int *)NEWS5000_INTSTAT2;

#ifdef DEBUG
	l2cnt++;
	if (l2cnt == 50) {
		*(volatile unsigned int *)NEWS5000_LED3 = 1;
	}
	if (l2cnt == 100) {
		*(volatile unsigned int *)NEWS5000_LED3 = 0;
		l2cnt = 0;
	}
#endif

	if (int2stat & NEWS5000_INT2_TIMER0) {
		*(volatile unsigned int *)NEWS5000_TIMER0 = 1;

		cf.pc = pc;
		cf.sr = statusReg;

		hardclock(&cf);
		intrcnt[HARDCLOCK_INTR]++;
	}

	if (int2stat & NEWS5000_INT2_TIMER1) {
		*(volatile unsigned int *)NEWS5000_TIMER1 = 1;
	}
}

void
level1_interrupt()
{
	unsigned int int1stat;
	int nintcall;

	int1stat = *(volatile unsigned int *)NEWS5000_INTSTAT1;

	if (int1stat) {
		/* printf("level1 interrupt (%08x)\n", int1stat); */
		nintcall = apbus_intr_call(1,int1stat);

		if (!nintcall) {
			printf("level1_interrupt: no match interrupt mask! %04x\n",int1stat);
		}
	} else {
		printf("level1 stray interrupt?\n");
	}

}

void
level0_interrupt()
{
	unsigned int int0stat;
	int nintcall;

	int0stat = *(volatile unsigned int *)NEWS5000_INTSTAT0;

	if (int0stat) {
		/* printf("level0 interrupt (%08x)\n", int0stat); */
		nintcall = apbus_intr_call(0, int0stat);

		if (!nintcall) {
			printf("level0_interrupt: no match interrupt mask! %04x\n",int0stat);
		}

	} else {
		printf("level0 stray interrupt?\n");
	}

}

void
enable_intr_5000()
{
	/* APbus Interrupt (DMAC, SONIC, FIFO*, FDC) */
	*(volatile unsigned int *)NEWS5000_INTMASK0 =
		NEWS5000_INTSLOT_ALL|NEWS5000_INT0_ALL;

	/* APbus Interrupt (KBD, SERIAL, AUDIO*, PARALLEL, FB */
	*(volatile unsigned int *)NEWS5000_INTMASK1 =
		NEWS5000_INTSLOT_ALL|NEWS5000_INT1_ALL;

	*(volatile unsigned int *)NEWS5000_INTMASK4 =
		NEWS5000_INT4_ALL;

	*(volatile unsigned int *)NEWS5000_INTCLR5 = 0xffffffff;
	*(volatile unsigned int *)NEWS5000_INTMASK5 =
		NEWS5000_INT5_ABIF|NEWS5000_INT5_MBIF|NEWS5000_INT5_SBIF;
}

void
disable_intr_5000()
{
	*(volatile unsigned int *)NEWS5000_INTMASK0 = 0;
	*(volatile unsigned int *)NEWS5000_INTMASK1 = 0;
	*(volatile unsigned int *)NEWS5000_INTMASK2 = 0;
	*(volatile unsigned int *)NEWS5000_INTMASK3 = 0;
	*(volatile unsigned int *)NEWS5000_INTMASK4 = 0;
	*(volatile unsigned int *)NEWS5000_INTMASK5 = 0;
}

void
readidrom_5000(rom)
	u_char *rom;
{
	u_int32_t *p = (void *)NEWS5000_IDROM;
	int i;

	for (i = 0; i < sizeof (struct idrom); i++, p += 2)
		*rom++ = ((*p & 0x0f) << 4) + (*(p + 1) & 0x0f);
}

extern struct idrom idrom;

void
news5000_init()
{
	enable_intr = enable_intr_5000;
	disable_intr = disable_intr_5000;

	readidrom_5000((u_char *)&idrom);
}
