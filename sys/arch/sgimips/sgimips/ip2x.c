/*	$NetBSD: ip2x.c,v 1.8 2004/01/12 12:50:07 sekiya Exp $	*/

/*
 * Copyright (c) 2001, 2002 Rafal K. Boni
 * All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ip2x.c,v 1.8 2004/01/12 12:50:07 sekiya Exp $");

#include "opt_cputype.h"
#include "opt_machtypes.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/sysconf.h>
#include <machine/machtype.h>
#include <machine/bus.h>
#include <mips/locore.h>

#include <mips/cache.h>

#include <sgimips/dev/int2reg.h>

u_int32_t next_clk_intr;
u_int32_t missed_clk_intrs;
static unsigned long last_clk_intr;
static bus_space_handle_t ioh;		/* for int2/3 */
static bus_space_tag_t iot;

static struct evcnt mips_int5_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "int 5 (clock)");

static struct evcnt mips_spurint_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "spurious interrupts");

void		ip2x_init(void);
int 		ip2x_local0_intr(void);
int		ip2x_local1_intr(void);
int 		ip2x_mappable_intr(void *);
void		ip2x_intr(u_int, u_int, u_int, u_int);
void		ip2x_intr_establish(int, int, int (*)(void *), void *);

unsigned long 	ip2x_clkread(void);
unsigned long	ip2x_cal_timer(void);

/* ip22_cache.S */
extern void	ip22_sdcache_do_wbinv(vaddr_t, vaddr_t);
extern void	ip22_sdcache_enable(void);
extern void	ip22_sdcache_disable(void);

/* imc has the bus reset code */
extern void	imc_bus_reset(void);
extern void	imc_bus_error(void);
extern void	imc_watchdog_tickle(void);

void
ip2x_init(void)
{
	u_int i;
	u_int32_t sysid;
	unsigned long cps;
	unsigned long ctrdiff[3];

	if ( !strcmp(cpu_model, "SGI-IP20"))
	{
		mach_type = MACH_SGI_IP20;
		ioh = MIPS_PHYS_TO_KSEG1(0x1fb801c0);

		sysid = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fbd0000);
		mach_boardrev = (sysid & 0x7000) >> 12;
	}
	else
	{
		mach_type = MACH_SGI_IP22;
		sysid = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1fbd9858);
		if (sysid & 1)
		{
			mach_subtype = MACH_SGI_IP22_FULLHOUSE;
			ioh = MIPS_PHYS_TO_KSEG1(0x1fbd9000);
		}
		else
		{
			mach_subtype = MACH_SGI_IP22_GUINESS;
			ioh = MIPS_PHYS_TO_KSEG1(0x1fbd9880);
		}

		mach_boardrev = (sysid >> 1) & 0x0f;

		/* Wire interrupts 7, 11 to mappable interrupt 0,1 handlers */
		intrtab[7].ih_fun = ip2x_mappable_intr;
		intrtab[7].ih_arg	= (void*) 0;

		intrtab[11].ih_fun = ip2x_mappable_intr;
		intrtab[11].ih_arg	= (void*) 1;
	}

	/* Clean out interrupt masks */
	bus_space_write_4(iot, ioh, INT2_LOCAL0_MASK, 0);
	bus_space_write_4(iot, ioh, INT2_LOCAL1_MASK, 0);
	bus_space_write_4(iot, ioh, INT2_MAP_MASK0, 0);
	bus_space_write_4(iot, ioh, INT2_MAP_MASK1, 0);

	/* Reset timer interrupts */
	bus_space_write_4(iot, ioh, INT2_TIMER_CLEAR, 0x03);

	platform.iointr = ip2x_intr;
	platform.bus_reset = imc_bus_reset;
	platform.intr_establish = ip2x_intr_establish;

	biomask = 0x0700;
	netmask = 0x0700;
	ttymask = 0x0f00;
	clockmask = 0xbf00;

	/* Prime cache */
	ip2x_cal_timer();

	cps = 0;
	for(i = 0; i < sizeof(ctrdiff) / sizeof(ctrdiff[0]); i++) {
		do {
			ctrdiff[i] = ip2x_cal_timer();
		} while (ctrdiff[i] == 0);

		cps += ctrdiff[i];
	}

	cps = cps / (sizeof(ctrdiff) / sizeof(ctrdiff[0]));

	printf("Timer calibration, got %lu cycles (%lu, %lu, %lu)\n", cps,
	    ctrdiff[0], ctrdiff[1], ctrdiff[2]);

	platform.clkread = ip2x_clkread;

	/* Counter on R4k/R4400/R4600/R5k counts at half the CPU frequency */
	curcpu()->ci_cpu_freq = 2 * cps * hz;
	curcpu()->ci_cycles_per_hz = curcpu()->ci_cpu_freq / (2 * hz);
	curcpu()->ci_divisor_delay = curcpu()->ci_cpu_freq / (2 * 1000000);
	MIPS_SET_CI_RECIPRICAL(curcpu());

	evcnt_attach_static(&mips_int5_evcnt);
	evcnt_attach_static(&mips_spurint_evcnt);

	printf("CPU clock speed = %lu.%02luMHz\n",
	    curcpu()->ci_cpu_freq / 1000000,
	    (curcpu()->ci_cpu_freq / 10000) % 100);
}

/*
 * NB: Do not re-enable interrupts here -- reentrancy here can cause all
 * sorts of Bad Things(tm) to happen, including kernel stack overflows.
 */
void
ip2x_intr(u_int32_t status, u_int32_t cause, u_int32_t pc, u_int32_t ipending)
{
	u_int32_t newcnt;
	struct clockframe cf;

	imc_watchdog_tickle();

	if (ipending & MIPS_INT_MASK_5) {
		last_clk_intr = mips3_cp0_count_read();

		next_clk_intr += curcpu()->ci_cycles_per_hz;
		mips3_cp0_compare_write(next_clk_intr);
		newcnt = mips3_cp0_count_read();

		/*
		 * Missed one or more clock interrupts, so let's start
		 * counting again from the current value.
		 */
		if ((next_clk_intr - newcnt) & 0x80000000) {
		    missed_clk_intrs++;

		    next_clk_intr = newcnt + curcpu()->ci_cycles_per_hz;
		    mips3_cp0_compare_write(next_clk_intr);
		}

		cf.pc = pc;
		cf.sr = status;

		hardclock(&cf);
		mips_int5_evcnt.ev_count++;

		cause &= ~MIPS_INT_MASK_5;
	}

	if (ipending & MIPS_INT_MASK_0) {
		if (ip2x_local0_intr())
			cause &= ~MIPS_INT_MASK_0;
	}

	if (ipending & MIPS_INT_MASK_1) {
		if (ip2x_local1_intr())
			cause &= ~MIPS_INT_MASK_1;
	}

	if (ipending & MIPS_INT_MASK_4) {
		imc_bus_error();
		cause &= ~MIPS_INT_MASK_4;
	}

	if (cause & status & MIPS_HARD_INT_MASK)
		mips_spurint_evcnt.ev_count++;
}

int
ip2x_mappable_intr(void *arg)
{
	int i;
	int ret;
	int intnum;
	u_int32_t mstat;
	u_int32_t mmask;
	int which = (int)arg;

	ret = 0;
	mstat = bus_space_read_4(iot, ioh, INT2_MAP_STATUS);
	mmask = bus_space_read_4(iot, ioh, INT2_MAP_MASK0 + (which << 2));

	mstat &= mmask;

	for (i = 0; i < 8; i++) {
		intnum = i + 16 + (which << 3);
		if (mstat & (1 << i)) {
			if (intrtab[intnum].ih_fun != NULL)
				ret |= (intrtab[intnum].ih_fun)
				    (intrtab[intnum].ih_arg);
			else
				printf("Unexpected mappable interrupt %d\n",
				    intnum);
		}
	}

	return ret;
}

int
ip2x_local0_intr(void)
{
	int i;
	int ret;
	u_int32_t l0stat;
	u_int32_t l0mask;

	ret = 0;
	l0stat = bus_space_read_4(iot, ioh, INT2_LOCAL0_STATUS);
	l0mask = bus_space_read_4(iot, ioh, INT2_LOCAL0_MASK);

	l0stat &= l0mask;

	for (i = 0; i < 8; i++) {
		if (l0stat & (1 << i)) {
			if (intrtab[i].ih_fun != NULL)
				ret |= (intrtab[i].ih_fun)(intrtab[i].ih_arg);
			else
				printf("Unexpected local0 interrupt %d\n", i);
		}
	}

	return ret;
}

int
ip2x_local1_intr(void)
{
	int i;
	int ret;
	u_int32_t l1stat;
	u_int32_t l1mask;

	l1stat = bus_space_read_4(iot, ioh, INT2_LOCAL1_STATUS);
	l1mask = bus_space_read_4(iot, ioh, INT2_LOCAL1_MASK);

	l1stat &= l1mask;

	ret = 0;
	for (i = 0; i < 8; i++) {
		if (l1stat & (1 << i)) {
			if (intrtab[8 + i].ih_fun != NULL)
				ret |= (intrtab[8 + i].ih_fun)
				    (intrtab[8 + i].ih_arg);
			else
				printf("Unexpected local1 interrupt %x\n",
				    8 + i );
		}
	}

	return ret;
}

void
ip2x_intr_establish(int level, int ipl, int (*handler) (void *), void *arg)
{
	u_int32_t mask;

	if (level < 0 || level >= NINTR)
		panic("invalid interrupt level");

	if (intrtab[level].ih_fun != NULL)
	{
		printf("warning: ip2x cannot share interrupts yet.\n");
		return;
	}

	intrtab[level].ih_fun = handler;
	intrtab[level].ih_arg = arg;

	if (level < 8) {
		mask = bus_space_read_4(iot, ioh, INT2_LOCAL0_MASK);
		mask |= (1 << level);
		bus_space_write_4(iot, ioh, INT2_LOCAL0_MASK, mask);
	} else if (level < 16) {
		mask = bus_space_read_4(iot, ioh, INT2_LOCAL1_MASK);
		mask |= (1 << (level - 8));
		bus_space_write_4(iot, ioh, INT2_LOCAL1_MASK, mask);
	} else if (level < 24) {
		/* Map0 interrupt maps to l0 bit 7, so turn that on too */
		mask = bus_space_read_4(iot, ioh, INT2_LOCAL0_MASK);
		mask |= (1 << 7);
		bus_space_write_4(iot, ioh, INT2_LOCAL0_MASK, mask);

		mask = bus_space_read_4(iot, ioh, INT2_MAP_MASK0);
		mask |= (1 << (level - 16));
		bus_space_write_4(iot, ioh, INT2_MAP_MASK0, mask);
	} else {
		/* Map1 interrupt maps to l1 bit 3, so turn that on too */
		mask = bus_space_read_4(iot, ioh, INT2_LOCAL1_MASK);
		mask |= (1 << 3);
		bus_space_write_4(iot, ioh, INT2_LOCAL1_MASK, mask);

		mask = bus_space_read_4(iot, ioh, INT2_MAP_MASK1);
		mask |= (1 << (level - 24));
		bus_space_write_4(iot, ioh, INT2_MAP_MASK1, mask);
	}
}

unsigned long
ip2x_clkread(void)
{
	uint32_t res, count;

	count = mips3_cp0_count_read() - last_clk_intr;
	MIPS_COUNT_TO_MHZ(curcpu(), count, res);
	return (res);
}

unsigned long
ip2x_cal_timer(void)
{
	int s;
	int roundtime;
	int sampletime;
	int startmsb, lsb, msb;
	unsigned long startctr, endctr;

	/*
	 * NOTE: HZ must be greater than 15 for this to work, as otherwise
	 * we'll overflow the counter.  We round the answer to hearest 1
	 * MHz of the master (2x) clock.
	 */
	roundtime = (1000000 / hz) / 2;
	sampletime = (1000000 / hz) + 0xff;
	startmsb = (sampletime >> 8);

	s = splhigh();

	bus_space_write_4(iot, ioh, INT2_TIMER_CONTROL, (0x80 | 0x30 | 0x04));
	bus_space_write_4(iot, ioh, INT2_TIMER_2, (sampletime & 0xff));
	bus_space_write_4(iot, ioh, INT2_TIMER_2, (sampletime >> 8));

	startctr = mips3_cp0_count_read();

	/* Wait for the MSB to count down to zero */
	do {
		bus_space_write_4(iot, ioh, INT2_TIMER_CONTROL, (0x80 | 0x00));
		lsb = bus_space_read_4(iot, ioh, INT2_TIMER_2) & 0xff;
		msb = bus_space_read_4(iot, ioh, INT2_TIMER_2) & 0xff;

		endctr = mips3_cp0_count_read();
	} while (msb);

	/* Turn off timer */
	bus_space_write_4(iot, ioh, INT2_TIMER_CONTROL, (0x80 | 0x30 | 0x08));

	splx(s);

	return (endctr - startctr) / roundtime * roundtime;
}
