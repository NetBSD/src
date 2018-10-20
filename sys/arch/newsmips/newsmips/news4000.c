/*	$NetBSD: news4000.c,v 1.1.2.2 2018/10/20 06:58:29 pgoyette Exp $	*/

/*-
 * Copyright (C) 2000 NONAKA Kimihiro.  All rights reserved.
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

#define __INTR_PRIVATE
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/timetc.h>
#include <sys/intr.h>

#include <machine/adrsmap.h>
#include <machine/cpu.h>

#include <mips/locore.h>

#include <newsmips/apbus/apbusvar.h>
#include <newsmips/newsmips/machid.h>

static void news4000_level0_intr(void);
static void news4000_level1_intr(void);

static void news4000_enable_intr(void);
static void news4000_disable_intr(void);
static void news4000_enable_timer(void);
static void news4000_readidrom(uint8_t *);

/*
 * This is a mask of bits of to clear in the SR when we go to a
 * given interrupt priority level.
 */
static const struct ipl_sr_map news4000_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] = 		0,
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
 * Handle news4000 interrupts.
 */
void
news4000_intr(int ppl, vaddr_t pc, uint32_t status)
{
	uint32_t ipending;
	int ipl;

	while (ppl < (ipl = splintr(&ipending))) {
		if (ipending & MIPS_INT_MASK_2) {
			uint32_t int2stat;

			int2stat = *(volatile uint32_t *)NEWS4000_INTST2;

			if (int2stat & NEWS4000_INT2_TIMER) {
				struct clockframe cf = {
					.pc = pc,
					.sr = status,
					.intr = (curcpu()->ci_idepth > 1),
				};
				*(volatile uint32_t *)NEWS4000_TIMER = 1;

				hardclock(&cf);
				intrcnt[HARDCLOCK_INTR]++;
			}

			apbus_wbflush();
		}

		if (ipending & MIPS_INT_MASK_5) {
			uint32_t int5stat =
			    *(volatile uint32_t *)NEWS4000_INTST5;
			printf("level5 interrupt: status = %04x\n", int5stat);
			apbus_wbflush();
		}

		if (ipending & MIPS_INT_MASK_4) {
			uint32_t int4stat =
			    *(volatile uint32_t *)NEWS4000_INTST4;
			printf("level4 interrupt: status = %04x\n", int4stat);
			apbus_wbflush();
		}

		if (ipending & MIPS_INT_MASK_3) {
			printf("level3 interrupt\n");
			apbus_wbflush();
		}

		if (ipending & MIPS_INT_MASK_1) {
			news4000_level1_intr();
			apbus_wbflush();
		}

		if (ipending & MIPS_INT_MASK_0) {
			news4000_level0_intr();
			apbus_wbflush();
		}
	}
}

static void
news4000_level1_intr(void)
{
	uint32_t int1stat;

	int1stat = *(volatile uint32_t *)NEWS4000_INTST1;

#if 0
	*(volatile uint32_t *)NEWS4000_LED = 0xe;	/* XXX */
#endif

	printf("level1_intr stat = 0x%x\n", int1stat);

	if (int1stat) {
		if (apbus_intr_dispatch(1, int1stat) == 0)
			printf("level1_intr: no handler (mask 0x%04x)\n",
			    int1stat);
	} else
		printf("level1 stray interrupt?\n");
}

static void
news4000_level0_intr(void)
{
	uint32_t int0stat;

	int0stat = *(volatile uint32_t *)NEWS4000_INTST0;

	if (int0stat) {
		if (apbus_intr_dispatch(0, int0stat) == 0)
			printf("level0_intr: no handler (mask 0x%04x)\n",
			    int0stat);
	} else
		printf("level0 stray interrupt?\n");
}

static void
news4000_enable_intr(void)
{

	*(volatile uint32_t *)NEWS4000_INTEN0 = 0xffffffff;
	*(volatile uint32_t *)NEWS4000_INTEN1 = 0xffffffff;
	*(volatile uint32_t *)NEWS4000_INTEN2 = 1;
	*(volatile uint32_t *)NEWS4000_INTEN3 = 0;
	*(volatile uint32_t *)NEWS4000_INTEN4 = 0;	/* 3 */
	*(volatile uint32_t *)NEWS4000_INTEN5 = 0;	/* 3 */
}

static void
news4000_disable_intr(void)
{

	*(volatile uint32_t *)NEWS4000_INTEN0 = 0;
	*(volatile uint32_t *)NEWS4000_INTEN1 = 0;
	*(volatile uint32_t *)NEWS4000_INTEN2 = 0;
	*(volatile uint32_t *)NEWS4000_INTEN3 = 0;
	*(volatile uint32_t *)NEWS4000_INTEN4 = 0;
	*(volatile uint32_t *)NEWS4000_INTEN5 = 0;
}

static void
news4000_enable_timer(void)
{

#if 0
	news4000_tc_init();
#endif

	/* enable timer interrpt */
	*(volatile uint32_t *)NEWS4000_TIMERCTL = 1;
}

extern struct idrom idrom;

static void
news4000_readidrom(uint8_t *rom)
{
	volatile uint32_t *status_port = (uint32_t *)NEWS4000_IDROM_STATUS;
	volatile uint32_t *data_port = (uint32_t *)NEWS4000_IDROM_DATA;
	int offset;

	while ((*status_port & 1) == 0)
		continue;

	for (offset = 0; offset < sizeof(struct idrom); offset++, rom++) {
		*data_port = offset;
		
		while ((*status_port & 1) == 0)
			continue;

		*rom = (uint8_t)(*data_port & 0xff);
	}
}

void
news4000_init(void)
{

	ipl_sr_map = news4000_ipl_sr_map;

	enable_intr = news4000_enable_intr;
	disable_intr = news4000_disable_intr;
	enable_timer = news4000_enable_timer;

	news_wbflush = (uint32_t *)NEWS4000_WBFLUSH;

	news4000_readidrom((uint8_t *)&idrom);
	hostid = idrom.id_serial;
}
