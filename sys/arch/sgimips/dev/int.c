/*	$NetBSD: int.c,v 1.17 2008/01/09 20:38:36 wiz Exp $	*/

/*
 * Copyright (c) 2004 Christopher SEKIYA
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

/*
 * INT/INT2/INT3 interrupt controller (used in Indy's, Indigo's, etc..)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: int.c,v 1.17 2008/01/09 20:38:36 wiz Exp $");

#include "opt_cputype.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/timetc.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <dev/ic/i8253reg.h>
#include <machine/sysconf.h>
#include <machine/machtype.h>
#include <machine/bus.h>
#include <mips/locore.h>

#include <mips/cache.h>

#include <sgimips/dev/int2reg.h>
#include <sgimips/dev/int2var.h>

static bus_space_handle_t ioh;
static bus_space_tag_t iot;

struct int_softc {
	struct device sc_dev;
};


static int	int_match(struct device *, struct cfdata *, void *);
static void	int_attach(struct device *, struct device *, void *);
static void 	int_local0_intr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
static void	int_local1_intr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
static int 	int_mappable_intr(void *);
static void    *int_intr_establish(int, int, int (*)(void *), void *);
static void	int_8254_cal(void);
static u_int	int_8254_get_timecount(struct timecounter *);
static void	int_8254_intr0(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
static void	int_8254_intr1(u_int32_t, u_int32_t, u_int32_t, u_int32_t);

#ifdef MIPS3
static u_long	int_cal_timer(void);
#endif

static struct timecounter int_8254_timecounter = {
	int_8254_get_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	~0u,			/* counter_mask */
	500000,			/* frequency */
	"int i8254",		/* name */
	100,			/* quality */
	NULL,			/* prev */
	NULL,			/* next */
};

static u_long int_8254_tc_count;

CFATTACH_DECL(int, sizeof(struct int_softc),
	int_match, int_attach, NULL, NULL);

static int
int_match(struct device *parent, struct cfdata *match, void *aux)
{

	if ((mach_type == MACH_SGI_IP12) || (mach_type == MACH_SGI_IP20) ||
	    (mach_type == MACH_SGI_IP22) )
		return 1;

	return 0;
}

static void
int_attach(struct device *parent, struct device *self, void *aux)
{
	u_int32_t address;

	if (mach_type == MACH_SGI_IP12)
		address = INT_IP12;
	else if (mach_type == MACH_SGI_IP20)
		address = INT_IP20;
	else if (mach_type == MACH_SGI_IP22) {
		if (mach_subtype == MACH_SGI_IP22_FULLHOUSE)
			address = INT_IP22;
		else
			address = INT_IP24;
	} else
		panic("\nint0: passed match, but failed attach?");

	printf(" addr 0x%x\n", address);

	bus_space_map(iot, address, 0, 0, &ioh);
	iot = SGIMIPS_BUS_SPACE_NORMAL;

	/* Clean out interrupt masks */
	bus_space_write_4(iot, ioh, INT2_LOCAL0_MASK, 0);
	bus_space_write_4(iot, ioh, INT2_LOCAL1_MASK, 0);
	bus_space_write_4(iot, ioh, INT2_MAP_MASK0, 0);
	bus_space_write_4(iot, ioh, INT2_MAP_MASK1, 0);

	/* Reset timer interrupts */
	bus_space_write_4(iot, ioh, INT2_TIMER_CLEAR, 0x03);

	switch (mach_type) {
		case MACH_SGI_IP12:
			platform.intr1 = int_local0_intr;
			platform.intr2 = int_local1_intr;
			platform.intr3 = int_8254_intr0;
			platform.intr4 = int_8254_intr1;
			int_8254_cal();
			tc_init(&int_8254_timecounter);
			break;
#ifdef MIPS3
		case MACH_SGI_IP20:
		case MACH_SGI_IP22:
		{
			int i;
			unsigned long cps;
			unsigned long ctrdiff[3];

			platform.intr0 = int_local0_intr;
			platform.intr1 = int_local1_intr;

			/* calibrate timer */
			int_cal_timer();

			cps = 0;
			for (i = 0;
			    i < sizeof(ctrdiff) / sizeof(ctrdiff[0]); i++) {
				do {
					ctrdiff[i] = int_cal_timer();
				} while (ctrdiff[i] == 0);

				cps += ctrdiff[i];
			}

			cps = cps / (sizeof(ctrdiff) / sizeof(ctrdiff[0]));

			printf("%s: bus %luMHz, CPU %luMHz\n",
			    self->dv_xname, cps / 10000, cps / 5000);

			/* R4k/R4400/R4600/R5k count at half CPU frequency */
			curcpu()->ci_cpu_freq = 2 * cps * hz;
		}
#endif /* MIPS3 */

			break;
		default:
			panic("int0: unsupported machine type %i\n", mach_type);
			break;
	}

	curcpu()->ci_cycles_per_hz = curcpu()->ci_cpu_freq / (2 * hz);
	curcpu()->ci_divisor_delay = curcpu()->ci_cpu_freq / (2 * 1000000);
	MIPS_SET_CI_RECIPROCAL(curcpu());

	if (mach_type == MACH_SGI_IP22) {
		/* Wire interrupts 7, 11 to mappable interrupt 0,1 handlers */
		intrtab[7].ih_fun = int_mappable_intr;
		intrtab[7].ih_arg = (void*) 0;

		intrtab[11].ih_fun = int_mappable_intr;
		intrtab[11].ih_arg = (void*) 1;
	}

	platform.intr_establish = int_intr_establish;
}

int
int_mappable_intr(void *arg)
{
	int i;
	int ret;
	int intnum;
	u_int32_t mstat;
	u_int32_t mmask;
	int which = (int)arg;
	struct sgimips_intrhand *ih;

	ret = 0;
	mstat = bus_space_read_4(iot, ioh, INT2_MAP_STATUS);
	mmask = bus_space_read_4(iot, ioh, INT2_MAP_MASK0 + (which << 2));

	mstat &= mmask;

	for (i = 0; i < 8; i++) {
		intnum = i + 16 + (which << 3);
		if (mstat & (1 << i)) {
			for (ih = &intrtab[intnum]; ih != NULL;
							ih = ih->ih_next) {
				if (ih->ih_fun != NULL)
					ret |= (ih->ih_fun)(ih->ih_arg);
				else
					printf("int0: unexpected mapped "
					       "interrupt %d\n", intnum);
			}
		}
	}

	return ret;
}

void
int_local0_intr(u_int32_t status, u_int32_t cause, u_int32_t pc,
		u_int32_t ipending)
{
	int i;
	u_int32_t l0stat;
	u_int32_t l0mask;
	struct sgimips_intrhand *ih;

	l0stat = bus_space_read_4(iot, ioh, INT2_LOCAL0_STATUS);
	l0mask = bus_space_read_4(iot, ioh, INT2_LOCAL0_MASK);

	l0stat &= l0mask;

	for (i = 0; i < 8; i++) {
		if (l0stat & (1 << i)) {
			for (ih = &intrtab[i]; ih != NULL; ih = ih->ih_next) {
				if (ih->ih_fun != NULL)
					(ih->ih_fun)(ih->ih_arg);
				else
					printf("int0: unexpected local0 "
					       "interrupt %d\n", i);
			}
		}
	}
}

void
int_local1_intr(u_int32_t status, u_int32_t cause, u_int32_t pc,
		u_int32_t ipending)
{
	int i;
	u_int32_t l1stat;
	u_int32_t l1mask;
	struct sgimips_intrhand *ih;

	l1stat = bus_space_read_4(iot, ioh, INT2_LOCAL1_STATUS);
	l1mask = bus_space_read_4(iot, ioh, INT2_LOCAL1_MASK);

	l1stat &= l1mask;

	for (i = 0; i < 8; i++) {
		if (l1stat & (1 << i)) {
			for (ih = &intrtab[8+i]; ih != NULL; ih = ih->ih_next) {
				if (ih->ih_fun != NULL)
					(ih->ih_fun)(ih->ih_arg);
				else
					printf("int0: unexpected local1 "
					       " interrupt %x\n", 8 + i);
			}
		}
	}
}

void *
int_intr_establish(int level, int ipl, int (*handler) (void *), void *arg)
{
	u_int32_t mask;

	if (level < 0 || level >= NINTR)
		panic("invalid interrupt level");

	if (intrtab[level].ih_fun == NULL) {
		intrtab[level].ih_fun = handler;
		intrtab[level].ih_arg = arg;
		intrtab[level].ih_next = NULL;
	} else {
		struct sgimips_intrhand *n, *ih = malloc(sizeof *ih,
							 M_DEVBUF, M_NOWAIT);

		if (ih == NULL) {
			printf("int_intr_establish: can't allocate handler\n");
			return (void *)NULL;
		}

		ih->ih_fun = handler;
		ih->ih_arg = arg;
		ih->ih_next = NULL;

		for (n = &intrtab[level]; n->ih_next != NULL; n = n->ih_next)
			;

		n->ih_next = ih;

		return (void *)NULL;	/* vector already set */
	}


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

	return (void *)NULL;
}

#ifdef MIPS3
static u_long
int_cal_timer(void)
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

	bus_space_write_4(iot, ioh, INT2_TIMER_CONTROL,
		( TIMER_SEL2 | TIMER_16BIT | TIMER_RATEGEN) );
	bus_space_write_4(iot, ioh, INT2_TIMER_2, (sampletime & 0xff));
	bus_space_write_4(iot, ioh, INT2_TIMER_2, (sampletime >> 8));

	startctr = mips3_cp0_count_read();

	/* Wait for the MSB to count down to zero */
	do {
		bus_space_write_4(iot, ioh, INT2_TIMER_CONTROL, TIMER_SEL2 );
		lsb = bus_space_read_4(iot, ioh, INT2_TIMER_2) & 0xff;
		msb = bus_space_read_4(iot, ioh, INT2_TIMER_2) & 0xff;

		endctr = mips3_cp0_count_read();
	} while (msb);

	/* Turn off timer */
	bus_space_write_4(iot, ioh, INT2_TIMER_CONTROL,
		( TIMER_SEL2 | TIMER_16BIT | TIMER_SWSTROBE) );

	splx(s);

	return (endctr - startctr) / roundtime * roundtime;
}
#endif /* MIPS3 */

/*
 * A 1.000MHz master clock is wired to TIMER2, which in turn clocks the two
 * other timers. On IP12 TIMER1 interrupts on MIPS interrupt 1 and TIMER2
 * on MIPS interrupt 2.
 *
 * Apparently int2 doesn't like counting down from one, but two works, so
 * we get a good 500000Hz.
 */
void
int_8254_cal(void)
{
	int s;

	s = splhigh();

	bus_space_write_1(iot, ioh, INT2_TIMER_0 + 15,
	    TIMER_SEL0|TIMER_RATEGEN|TIMER_16BIT);
	bus_space_write_1(iot, ioh, INT2_TIMER_0 + 3, (500000 / hz) % 256);
	wbflush();
	delay(4);
	bus_space_write_1(iot, ioh, INT2_TIMER_0 + 3, (500000 / hz) / 256);

	bus_space_write_1(iot, ioh, INT2_TIMER_0 + 15,
	    TIMER_SEL1|TIMER_RATEGEN|TIMER_16BIT);
	bus_space_write_1(iot, ioh, INT2_TIMER_0 + 7, 0xff);
	wbflush();
	delay(4);
	bus_space_write_1(iot, ioh, INT2_TIMER_0 + 7, 0xff);

	bus_space_write_1(iot, ioh, INT2_TIMER_0 + 15,
	    TIMER_SEL2|TIMER_RATEGEN|TIMER_16BIT);
	bus_space_write_1(iot, ioh, INT2_TIMER_0 + 11, 2);
	wbflush();
	delay(4);
	bus_space_write_1(iot, ioh, INT2_TIMER_0 + 11, 0);

	splx(s);
}


static u_int
int_8254_get_timecount(struct timecounter *tc)
{
	int s;
	u_int count;
	u_char lo, hi;

	s = splhigh();

        bus_space_write_1(iot, ioh, INT2_TIMER_0 + 15,
	    TIMER_SEL1 | TIMER_LATCH);
	lo = bus_space_read_1(iot, ioh, INT2_TIMER_0 + 7);
	hi = bus_space_read_1(iot, ioh, INT2_TIMER_0 + 7);
	count = 0xffff - ((hi << 8) | lo);

	splx(s);

	return (int_8254_tc_count + count);
}

static void
int_8254_intr0(u_int32_t status, u_int32_t cause, u_int32_t pc,
    u_int32_t ipending)
{
	struct clockframe cf;

	cf.pc = pc;
	cf.sr = status;

	hardclock(&cf);

	bus_space_write_4(iot, ioh, INT2_TIMER_CLEAR, 1);
}


static void
int_8254_intr1(u_int32_t status, u_int32_t cause, u_int32_t pc,
    u_int32_t ipending)
{
	int s;

	s = splhigh();

	int_8254_tc_count += 0xffff;
	bus_space_write_4(iot, ioh, INT2_TIMER_CLEAR, 2);

	splx(s);
}

void
int2_wait_fifo(u_int32_t flag)
{
	if (ioh == 0)
		delay(5000);
	else
		while (bus_space_read_4(iot, ioh, INT2_LOCAL0_STATUS) & flag)
			;
}
