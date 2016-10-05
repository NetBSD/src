/*	$NetBSD: news5000.c,v 1.20.32.1 2016/10/05 20:55:33 skrll Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: news5000.c,v 1.20.32.1 2016/10/05 20:55:33 skrll Exp $");

#define __INTR_PRIVATE
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/timetc.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <mips/locore.h>

#include <machine/adrsmap.h>

#include <newsmips/apbus/apbusvar.h>
#include <newsmips/newsmips/machid.h>

static void news5000_level1_intr(void);
static void news5000_level0_intr(void);

static void news5000_enable_intr(void);
static void news5000_disable_intr(void);
static void news5000_enable_timer(void);
static void news5000_readidrom(uint8_t *);
static void news5000_tc_init(void);
static uint32_t news5000_getfreerun(struct timecounter *);

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given interrupt priority level.
 */
static const struct ipl_sr_map news5000_ipl_sr_map = {
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
 * Handle news5000 interrupts.
 */
void
news5000_intr(int ppl, vaddr_t pc, uint32_t status)
{
	uint32_t ipending;
	int ipl;

	while (ppl < (ipl = splintr(&ipending))) {

		if (ipending & MIPS_INT_MASK_2) {
#ifdef DEBUG
			static int l2cnt = 0;
#endif
			uint32_t int2stat;

			int2stat = *(volatile uint32_t *)NEWS5000_INTST2;

#ifdef DEBUG
			l2cnt++;
			if (l2cnt == 50) {
				*(volatile uint32_t *)NEWS5000_LED_SEC = 1;
			}
			if (l2cnt == 100) {
				*(volatile uint32_t *)NEWS5000_LED_SEC = 0;
				l2cnt = 0;
			}
#endif

			if (int2stat & NEWS5000_INT2_TIMER0) {
				struct clockframe cf = {
					.pc = pc,
					.sr = status,
					.intr = (curcpu()->ci_idepth > 1),
				};
				*(volatile uint32_t *)NEWS5000_TIMER0 = 1;
				hardclock(&cf);
				intrcnt[HARDCLOCK_INTR]++;
			}

			apbus_wbflush();
		}

		if (ipending & MIPS_INT_MASK_5) {
			uint32_t int5stat;

			int5stat = *(volatile u_int *)NEWS5000_INTST5;
			printf("level5 interrupt (%08x)\n", int5stat);

			apbus_wbflush();
		}

		if (ipending & MIPS_INT_MASK_4) {
			uint32_t int4stat;

			int4stat = *(volatile uint32_t *)NEWS5000_INTST4;
			printf("level4 interrupt (%08x)\n", int4stat);
			if (int4stat & NEWS5000_INT4_APBUS) {
				uint32_t stat;

				stat = *(volatile uint32_t *)NEWS5000_APBUS_INTST;
				printf("APbus error 0x%04x\n", stat & 0xffff);
				if (stat & NEWS5000_APBUS_INT_DMAADDR) {
					printf("DMA Address Error: "
					    "slot=%x, addr=0x%08x\n",
					    *(volatile uint32_t *)NEWS5000_APBUS_DER_S,
					    *(volatile uint32_t *)NEWS5000_APBUS_DER_A);
				}
				if (stat & NEWS5000_APBUS_INT_RDTIMEO)
					printf("IO Read Timeout: addr=0x%08x\n",
					    *(volatile uint32_t *)NEWS5000_APBUS_BER_A);
				if (stat & NEWS5000_APBUS_INT_WRTIMEO)
					printf("IO Write Timeout: addr=0x%08x\n",
					    *(volatile uint32_t *)NEWS5000_APBUS_BER_A);
				*(volatile uint32_t *)0xb4c00014 = stat;
			}

			apbus_wbflush();
		}

		if (ipending & MIPS_INT_MASK_3) {
			uint32_t int3stat;

			int3stat = *(volatile uint32_t *)NEWS5000_INTST3;
			printf("level3 interrupt (%08x)\n", int3stat);

			apbus_wbflush();
		}

		if (ipending & MIPS_INT_MASK_1) {
			news5000_level1_intr();
			apbus_wbflush();
		}

		if (ipending & MIPS_INT_MASK_0) {
			news5000_level0_intr();
			apbus_wbflush();
		}
	}
}


static void
news5000_level1_intr(void)
{
	uint32_t int1stat;

	int1stat = *(volatile uint32_t *)NEWS5000_INTST1;

	if (int1stat) {
		if (apbus_intr_dispatch(1, int1stat) == 0)
			printf("level1_intr: no handler (mask 0x%04x)\n",
			       int1stat);
	} else
		printf("level1 stray interrupt?\n");
}

static void
news5000_level0_intr(void)
{
	uint32_t int0stat;

	int0stat = *(volatile uint32_t *)NEWS5000_INTST0;

	if (int0stat) {
		if (apbus_intr_dispatch(0, int0stat) == 0)
			printf("level0_intr: no handler (mask 0x%04x)\n",
			       int0stat);
	} else
		printf("level0 stray interrupt?\n");
}

static void
news5000_enable_intr(void)
{

	/* INT0 and INT1 has been enabled at attach */
	/* INT2 -- It's not a time to enable timer yet. */
	/* INT3 -- not used for NWS-5000 */

	*(volatile uint32_t *)NEWS5000_INTEN4 = NEWS5000_INT4_APBUS;
	*(volatile uint32_t *)NEWS5000_APBUS_INTMSK = 0xffff;

	/* INT5 -- currently ignored */
	*(volatile uint32_t *)NEWS5000_INTEN5 = 0;
}

static void
news5000_disable_intr(void)
{

	*(volatile uint32_t *)NEWS5000_INTEN0 = 0;
	*(volatile uint32_t *)NEWS5000_INTEN1 = 0;
	*(volatile uint32_t *)NEWS5000_INTEN2 = 0;
	*(volatile uint32_t *)NEWS5000_INTEN3 = 0;
	*(volatile uint32_t *)NEWS5000_INTEN4 = 0;
	*(volatile uint32_t *)NEWS5000_INTEN5 = 0;
}

static void
news5000_enable_timer(void)
{

	news5000_tc_init();

	/* enable timer interrpt */
	*(volatile uint32_t *)NEWS5000_INTEN2 = NEWS5000_INT2_TIMER0;
}

static uint32_t
news5000_getfreerun(struct timecounter *tc)
{
	return *(volatile uint32_t *)NEWS5000_FREERUN;
}

static void
news5000_tc_init(void)
{
	static struct timecounter tc = {
		.tc_get_timecount = news5000_getfreerun,
		.tc_frequency = 1000000,
		.tc_counter_mask = ~0,
		.tc_name = "news5000_freerun",
		.tc_quality = 100,
	};

	tc_init(&tc);
}
	

static void
news5000_readidrom(uint8_t *rom)
{
	uint32_t *p = (void *)NEWS5000_IDROM;
	int i;

	for (i = 0; i < sizeof(struct idrom); i++, p += 2)
		*rom++ = ((*p & 0x0f) << 4) + (*(p + 1) & 0x0f);
}

extern struct idrom idrom;

void
news5000_init(void)
{

	ipl_sr_map = news5000_ipl_sr_map;

	enable_intr = news5000_enable_intr;
	disable_intr = news5000_disable_intr;
	enable_timer = news5000_enable_timer;

	news5000_readidrom((uint8_t *)&idrom);
	hostid = idrom.id_serial;

	/* XXX reset uPD72067 FDC to avoid spurious interrupts */
#define NEWS5000_FDC_FDOUT 0xbed20000
#define FDO_FRST 0x04
	*(volatile uint8_t *)NEWS5000_FDC_FDOUT = FDO_FRST;
}
