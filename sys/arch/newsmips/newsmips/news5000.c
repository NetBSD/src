/*	$NetBSD: news5000.c,v 1.5 2000/07/29 08:21:44 tsubai Exp $	*/

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

static void level1_intr __P((void));
static void level0_intr __P((void));

/*
 * Handle news5000 interrupts.
 */
void
news5000_intr(status, cause, pc, ipending)
	u_int status;	/* status register at time of the exception */
	u_int cause;	/* cause register at time of exception */
	u_int pc;	/* program counter where to continue */
	u_int ipending;
{
	if (ipending & MIPS_INT_MASK_2) {
#ifdef DEBUG
		static int l2cnt = 0;
#endif
		u_int int2stat;
		struct clockframe cf;

		int2stat = *(volatile u_int *)NEWS5000_INTST2;

#ifdef DEBUG
		l2cnt++;
		if (l2cnt == 50) {
			*(volatile u_int *)NEWS5000_LED_SEC = 1;
		}
		if (l2cnt == 100) {
			*(volatile u_int *)NEWS5000_LED_SEC = 0;
			l2cnt = 0;
		}
#endif

		if (int2stat & NEWS5000_INT2_TIMER0) {
			*(volatile u_int *)NEWS5000_TIMER0 = 1;

			cf.pc = pc;
			cf.sr = status;

			hardclock(&cf);
			intrcnt[HARDCLOCK_INTR]++;
		}

		apbus_wbflush();
		cause &= ~MIPS_INT_MASK_2;
	}
	/* If clock interrupts were enabled, re-enable them ASAP. */
	_splset(MIPS_SR_INT_IE | (status & MIPS_INT_MASK_2));

	if (ipending & MIPS_INT_MASK_5) {
		u_int int5stat = *(volatile u_int *)NEWS5000_INTST5;
		printf("level5 interrupt (%08x)\n", int5stat);

		apbus_wbflush();
		cause &= ~MIPS_INT_MASK_5;
	}

	if (ipending & MIPS_INT_MASK_4) {
		u_int int4stat = *(volatile u_int *)NEWS5000_INTST4;
		printf("level4 interrupt (%08x)\n", int4stat);

		apbus_wbflush();
		cause &= ~MIPS_INT_MASK_4;
	}

	if (ipending & MIPS_INT_MASK_3) {
		u_int int3stat = *(volatile u_int *)NEWS5000_INTST3;
		printf("level3 interrupt (%08x)\n", int3stat);

		apbus_wbflush();
		cause &= ~MIPS_INT_MASK_3;
	}

	if (ipending & MIPS_INT_MASK_1) {
		level1_intr();
		apbus_wbflush();
		cause &= ~MIPS_INT_MASK_1;
	}

	if (ipending & MIPS_INT_MASK_0) {
		level0_intr();
		apbus_wbflush();
		cause &= ~MIPS_INT_MASK_0;
	}

	_splset((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);
}


void
level1_intr()
{
	u_int int1stat;

	int1stat = *(volatile u_int *)NEWS5000_INTST1;

	if (int1stat) {
		if (apbus_intr_call(1, int1stat) == 0)
			printf("level1_intr: no handler (mask 0x%04x)\n",
			       int1stat);
	} else
		printf("level1 stray interrupt?\n");
}

void
level0_intr()
{
	u_int int0stat;

	int0stat = *(volatile u_int *)NEWS5000_INTST0;

	if (int0stat) {
		if (apbus_intr_call(0, int0stat) == 0)
			printf("level0_intr: no handler (mask 0x%04x)\n",
			       int0stat);
	} else
		printf("level0 stray interrupt?\n");
}

void
enable_intr_5000()
{
	volatile u_int *inten0 = (void *)NEWS5000_INTEN0;
	volatile u_int *inten1 = (void *)NEWS5000_INTEN1;

	*inten0 = NEWS5000_INT0_DMAC | NEWS5000_INT0_SONIC |
		  NEWS5000_INT0_FDC;
	*inten1 = NEWS5000_INT1_KBD | NEWS5000_INT1_SCC |
		  NEWS5000_INT1_AUDIO0 | NEWS5000_INT1_AUDIO1 |
		  NEWS5000_INT1_PARALLEL | NEWS5000_INT1_FB;

	/* It's not a time to enable timer yet. */
	/* *(volatile u_int *)NEWS5000_INTEN2 = 0; */

	/* currently INT3-INT5 are not used */
	*(volatile u_int *)NEWS5000_INTEN3 = 0;
	*(volatile u_int *)NEWS5000_INTEN4 = 0;
	*(volatile u_int *)NEWS5000_INTEN5 = 0;
}

void
disable_intr_5000()
{
	*(volatile u_int *)NEWS5000_INTEN0 = 0;
	*(volatile u_int *)NEWS5000_INTEN1 = 0;
	*(volatile u_int *)NEWS5000_INTEN2 = 0;
	*(volatile u_int *)NEWS5000_INTEN3 = 0;
	*(volatile u_int *)NEWS5000_INTEN4 = 0;
	*(volatile u_int *)NEWS5000_INTEN5 = 0;
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
	hostid = idrom.id_serial;
}
