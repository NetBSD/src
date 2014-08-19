/*	$NetBSD: int.c,v 1.24.12.2 2014/08/20 00:03:22 tls Exp $	*/

/*
 * Copyright (c) 2009 Stephen M. Rumble 
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
 * INT1/INT2/INT3 interrupt controllers (IP6, IP10, IP12, IP20, IP22, IP24...)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: int.c,v 1.24.12.2 2014/08/20 00:03:22 tls Exp $");

#define __INTR_PRIVATE
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
#include <sys/bus.h>
#include <mips/locore.h>

#include <mips/cache.h>

#include <sgimips/dev/int1reg.h>
#include <sgimips/dev/int2reg.h>
#include <sgimips/dev/int2var.h>

static bus_space_handle_t ioh;
static bus_space_tag_t iot;

static int	int_match(device_t, cfdata_t, void *);
static void	int_attach(device_t, device_t, void *);
static void    *int1_intr_establish(int, int, int (*)(void *), void *);
static void    *int2_intr_establish(int, int, int (*)(void *), void *);
static void 	int1_local_intr(vaddr_t, uint32_t, uint32_t);
static void 	int2_local0_intr(vaddr_t, uint32_t, uint32_t);
static void	int2_local1_intr(vaddr_t, uint32_t, uint32_t);
static int 	int2_mappable_intr(void *);
static void	int_8254_cal(void);
static u_int	int_8254_get_timecount(struct timecounter *);
static void	int_8254_intr0(vaddr_t, uint32_t, uint32_t);
static void	int_8254_intr1(vaddr_t, uint32_t, uint32_t);

#ifdef MIPS3
static u_long	int2_cpu_freq(device_t);
static u_long	int2_cal_timer(void);
#endif

static struct timecounter int_8254_timecounter = {
	int_8254_get_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	~0u,			/* counter_mask */
	0,			/* frequency; set in int_8254_cal */
	"int i8254",		/* name */
	100,			/* quality */
	NULL,			/* prev */
	NULL,			/* next */
};

static u_long int_8254_tc_count;

CFATTACH_DECL_NEW(int, 0,
    int_match, int_attach, NULL, NULL);

static int
int_match(device_t parent, cfdata_t match, void *aux)
{

	switch (mach_type) {
	case MACH_SGI_IP6 | MACH_SGI_IP10:
	case MACH_SGI_IP12:
	case MACH_SGI_IP20:
	case MACH_SGI_IP22:
		return 1;
	}

	return 0;
}

static void
int_attach(device_t parent, device_t self, void *aux)
{
	uint32_t address;

	switch (mach_type) {
	case MACH_SGI_IP6 | MACH_SGI_IP10:
		address = INT1_IP6_IP10;
		break;

	case MACH_SGI_IP12:
		address = INT2_IP12;
		break;

	case MACH_SGI_IP20:
		address = INT2_IP20;
		break;

	case MACH_SGI_IP22:
		if (mach_subtype == MACH_SGI_IP22_FULLHOUSE)
			address = INT2_IP22;
		else
			address = INT2_IP24;
		break;

	default:
		panic("\nint0: passed match, but failed attach?");
	}

	printf(" addr 0x%x\n", address);

	bus_space_map(iot, address, 0, 0, &ioh);
	iot = SGIMIPS_BUS_SPACE_NORMAL;

	switch (mach_type) {
	case MACH_SGI_IP6 | MACH_SGI_IP10:
		/* Clean out interrupt masks */
		bus_space_write_1(iot, ioh, INT1_LOCAL_MASK, 0);

		/* Turn off timers and clear interrupts */
		bus_space_write_1(iot, ioh, INT1_TIMER_CONTROL,
		    (TIMER_SEL0 | TIMER_16BIT | TIMER_SWSTROBE));
		bus_space_write_1(iot, ioh, INT1_TIMER_CONTROL,
		    (TIMER_SEL1 | TIMER_16BIT | TIMER_SWSTROBE));
		bus_space_write_1(iot, ioh, INT1_TIMER_CONTROL,
		    (TIMER_SEL2 | TIMER_16BIT | TIMER_SWSTROBE));
		wbflush();
		delay(4);
		bus_space_read_1(iot, ioh, INT1_TIMER_0_ACK);
		bus_space_read_1(iot, ioh, INT1_TIMER_1_ACK);

		platform.intr_establish = int1_intr_establish;
		platform.intr1 = int1_local_intr;
		platform.intr2 = int_8254_intr0;
		platform.intr4 = int_8254_intr1;
		int_8254_cal();
		break;

	case MACH_SGI_IP12:
	case MACH_SGI_IP20:
	case MACH_SGI_IP22:
		/* Clean out interrupt masks */
		bus_space_write_1(iot, ioh, INT2_LOCAL0_MASK, 0);
		bus_space_write_1(iot, ioh, INT2_LOCAL1_MASK, 0);
		bus_space_write_1(iot, ioh, INT2_MAP_MASK0, 0);
		bus_space_write_1(iot, ioh, INT2_MAP_MASK1, 0);

		/* Reset timer interrupts */
		bus_space_write_1(iot, ioh, INT2_TIMER_CONTROL,
		    (TIMER_SEL0 | TIMER_16BIT | TIMER_SWSTROBE));
		bus_space_write_1(iot, ioh, INT2_TIMER_CONTROL,
		    (TIMER_SEL1 | TIMER_16BIT | TIMER_SWSTROBE));
		bus_space_write_1(iot, ioh, INT2_TIMER_CONTROL,
		    (TIMER_SEL2 | TIMER_16BIT | TIMER_SWSTROBE));
		wbflush();
		delay(4);
		bus_space_write_1(iot, ioh, INT2_TIMER_CLEAR, 0x03);

		if (mach_type == MACH_SGI_IP12) {
			platform.intr_establish = int2_intr_establish;
			platform.intr1 = int2_local0_intr;
			platform.intr2 = int2_local1_intr;
			platform.intr3 = int_8254_intr0;
			platform.intr4 = int_8254_intr1;
			int_8254_cal();
		} else {
			platform.intr_establish = int2_intr_establish;
			platform.intr0 = int2_local0_intr;
			platform.intr1 = int2_local1_intr;
#ifdef MIPS3
			curcpu()->ci_cpu_freq = int2_cpu_freq(self);
#endif
		}
		break;

	default:
		panic("int0: unsupported machine type %i\n", mach_type);
	}

	curcpu()->ci_cycles_per_hz = curcpu()->ci_cpu_freq / (2 * hz);
	curcpu()->ci_divisor_delay = curcpu()->ci_cpu_freq / (2 * 1000000);

	if (mach_type == MACH_SGI_IP22) {
		/* Wire interrupts 7, 11 to mappable interrupt 0,1 handlers */
		intrtab[7].ih_fun = int2_mappable_intr;
		intrtab[7].ih_arg = (void*) 0;

		intrtab[11].ih_fun = int2_mappable_intr;
		intrtab[11].ih_arg = (void*) 1;
	}
}

int
int2_mappable_intr(void *arg)
{
	int i;
	int ret;
	int intnum;
	uint32_t mstat;
	uint32_t mmask;
	int which = (intptr_t)arg;
	struct sgimips_intrhand *ih;

	ret = 0;
	mstat = bus_space_read_1(iot, ioh, INT2_MAP_STATUS);
	mmask = bus_space_read_1(iot, ioh, INT2_MAP_MASK0 + (which << 2));

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

static void
int1_local_intr(vaddr_t pc, uint32_t status, uint32_t ipend)
{
	int i;
	uint16_t stat;
	uint8_t  mask;
	struct sgimips_intrhand *ih;

	stat = bus_space_read_2(iot, ioh, INT1_LOCAL_STATUS);
	mask = bus_space_read_1(iot, ioh, INT1_LOCAL_MASK);

	/* for STATUS, a 0 bit means interrupt is pending */
	stat = ~stat & mask;

	for (i = 0; stat != 0; i++, stat >>= 1) {
		if (stat & 1) {
			for (ih = &intrtab[i]; ih != NULL; ih = ih->ih_next) {
				if (ih->ih_fun != NULL)
					(ih->ih_fun)(ih->ih_arg);
				else
					printf("int0: unexpected local "
					       "interrupt %d\n", i);
			}
		}
	}
}

void
int2_local0_intr(vaddr_t pc, uint32_t status, uint32_t ipending)
{
	int i;
	uint32_t l0stat;
	uint32_t l0mask;
	struct sgimips_intrhand *ih;

	l0stat = bus_space_read_1(iot, ioh, INT2_LOCAL0_STATUS);
	l0mask = bus_space_read_1(iot, ioh, INT2_LOCAL0_MASK);

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
int2_local1_intr(vaddr_t pc, uint32_t status, uint32_t ipending)
{
	int i;
	uint32_t l1stat;
	uint32_t l1mask;
	struct sgimips_intrhand *ih;

	l1stat = bus_space_read_1(iot, ioh, INT2_LOCAL1_STATUS);
	l1mask = bus_space_read_1(iot, ioh, INT2_LOCAL1_MASK);

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
int1_intr_establish(int level, int ipl, int (*handler) (void *), void *arg)
{
	uint8_t mask;

	if (level < 0 || level >= NINTR)
		panic("invalid interrupt level");

	if (intrtab[level].ih_fun == NULL) {
		intrtab[level].ih_fun = handler;
		intrtab[level].ih_arg = arg;
		intrtab[level].ih_next = NULL;
	} else {
		struct sgimips_intrhand *n, *ih;

		ih = malloc(sizeof *ih, M_DEVBUF, M_NOWAIT);
		if (ih == NULL) {
			printf("int0: can't allocate handler\n");
			return (void *)NULL;
		}

		ih->ih_fun = handler;
		ih->ih_arg = arg;
		ih->ih_next = NULL;

		for (n = &intrtab[level]; n->ih_next != NULL; n = n->ih_next)
			;

		n->ih_next = ih;

		return NULL;	/* vector already set */
	}

	if (level < 8) {
		mask = bus_space_read_1(iot, ioh, INT1_LOCAL_MASK);
		mask |= (1 << level);
		bus_space_write_1(iot, ioh, INT1_LOCAL_MASK, mask);
	} else {
		printf("int0: level >= 16 (%d)\n", level);
	}

	return NULL;
}

void *
int2_intr_establish(int level, int ipl, int (*handler) (void *), void *arg)
{
	uint32_t mask;

	if (level < 0 || level >= NINTR)
		panic("invalid interrupt level");

	if (intrtab[level].ih_fun == NULL) {
		intrtab[level].ih_fun = handler;
		intrtab[level].ih_arg = arg;
		intrtab[level].ih_next = NULL;
	} else {
		struct sgimips_intrhand *n, *ih;

		ih = malloc(sizeof *ih, M_DEVBUF, M_NOWAIT);
		if (ih == NULL) {
			printf("int0: can't allocate handler\n");
			return NULL;
		}

		ih->ih_fun = handler;
		ih->ih_arg = arg;
		ih->ih_next = NULL;

		for (n = &intrtab[level]; n->ih_next != NULL; n = n->ih_next)
			;

		n->ih_next = ih;

		return NULL;	/* vector already set */
	}

	if (level < 8) {
		mask = bus_space_read_1(iot, ioh, INT2_LOCAL0_MASK);
		mask |= (1 << level);
		bus_space_write_1(iot, ioh, INT2_LOCAL0_MASK, mask);
	} else if (level < 16) {
		mask = bus_space_read_1(iot, ioh, INT2_LOCAL1_MASK);
		mask |= (1 << (level - 8));
		bus_space_write_1(iot, ioh, INT2_LOCAL1_MASK, mask);
	} else if (level < 24) {
		/* Map0 interrupt maps to l0 bit 7, so turn that on too */
		mask = bus_space_read_1(iot, ioh, INT2_LOCAL0_MASK);
		mask |= (1 << 7);
		bus_space_write_1(iot, ioh, INT2_LOCAL0_MASK, mask);

		mask = bus_space_read_1(iot, ioh, INT2_MAP_MASK0);
		mask |= (1 << (level - 16));
		bus_space_write_1(iot, ioh, INT2_MAP_MASK0, mask);
	} else {
		/* Map1 interrupt maps to l1 bit 3, so turn that on too */
		mask = bus_space_read_1(iot, ioh, INT2_LOCAL1_MASK);
		mask |= (1 << 3);
		bus_space_write_1(iot, ioh, INT2_LOCAL1_MASK, mask);

		mask = bus_space_read_1(iot, ioh, INT2_MAP_MASK1);
		mask |= (1 << (level - 24));
		bus_space_write_1(iot, ioh, INT2_MAP_MASK1, mask);
	}

	return NULL;
}

#ifdef MIPS3
static u_long
int2_cpu_freq(device_t self)
{
	int i;
	unsigned long cps;
	unsigned long ctrdiff[3];

	/* calibrate timer */
	int2_cal_timer();

	cps = 0;
	for (i = 0;
	    i < sizeof(ctrdiff) / sizeof(ctrdiff[0]); i++) {
		do {
			ctrdiff[i] = int2_cal_timer();
		} while (ctrdiff[i] == 0);

		cps += ctrdiff[i];
	}

	cps = cps / (sizeof(ctrdiff) / sizeof(ctrdiff[0]));

	printf("%s: bus %luMHz, CPU %luMHz\n",
	    device_xname(self), cps / 10000, cps / 5000);

	/* R4k/R4400/R4600/R5k count at half CPU frequency */
	return (2 * cps * hz);
}

static u_long
int2_cal_timer(void)
{
	int s;
	int roundtime;
	int sampletime;
	int msb;
	unsigned long startctr, endctr;

	/*
	 * NOTE: HZ must be greater than 15 for this to work, as otherwise
	 * we'll overflow the counter.  We round the answer to nearest 1
	 * MHz of the master (2x) clock.
	 */
	roundtime = (1000000 / hz) / 2;
	sampletime = (1000000 / hz) + 0xff;

	s = splhigh();

	bus_space_write_1(iot, ioh, INT2_TIMER_CONTROL,
	    (TIMER_SEL2 | TIMER_16BIT | TIMER_RATEGEN));
	bus_space_write_1(iot, ioh, INT2_TIMER_2, (sampletime & 0xff));
	bus_space_write_1(iot, ioh, INT2_TIMER_2, (sampletime >> 8));

	startctr = mips3_cp0_count_read();

	/* Wait for the MSB to count down to zero */
	do {
		bus_space_write_1(iot, ioh, INT2_TIMER_CONTROL, TIMER_SEL2);
		(void)bus_space_read_1(iot, ioh, INT2_TIMER_2);
		msb = bus_space_read_1(iot, ioh, INT2_TIMER_2) & 0xff;

		endctr = mips3_cp0_count_read();
	} while (msb);

	/* Turn off timer */
	bus_space_write_1(iot, ioh, INT2_TIMER_CONTROL,
	    (TIMER_SEL2 | TIMER_16BIT | TIMER_SWSTROBE));

	splx(s);

	return (endctr - startctr) / roundtime * roundtime;
}
#endif /* MIPS3 */

/*
 * A master clock is wired to TIMER_2, which in turn clocks the two other
 * timers. The master frequencies are as follows:
 *     IP6,  IP10:		3.6864MHz
 *     IP12, IP20, IP22:	1MHz
 *     IP17:			10MHz
 *
 * TIMER_0 and TIMER_1 interrupts are tied to MIPS interrupts as follows:
 *     IP6,  IP10:		TIMER_0: INT2, TIMER_1: INT4
 *     IP12:			TIMER_0: INT3, TIMER_1: INT4
 *     IP17, IP20, IP22:	TIMER_0: INT2, TIMER_1: INT3
 *
 * NB: Apparently int2 doesn't like counting down from one, but two works.
 */
void
int_8254_cal(void)
{
	bus_size_t timer_control, timer_0, timer_1, timer_2;
	int s;

	switch (mach_type) {
	case MACH_SGI_IP6 | MACH_SGI_IP10:
		int_8254_timecounter.tc_frequency = 3686400 / 8;
		timer_control	= INT1_TIMER_CONTROL;
		timer_0		= INT1_TIMER_0;
		timer_1		= INT1_TIMER_1;
		timer_2		= INT1_TIMER_2;
		break;

	case MACH_SGI_IP12:
		int_8254_timecounter.tc_frequency = 1000000 / 8;
		timer_control	= INT2_TIMER_CONTROL;
		timer_0		= INT2_TIMER_0;
		timer_1		= INT2_TIMER_1;
		timer_2		= INT2_TIMER_2;
		break;

	default:
		panic("int_8254_cal");
	}

	s = splhigh();

	/* Timer0 is our hz. */
	bus_space_write_1(iot, ioh, timer_control,
	    TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
	bus_space_write_1(iot, ioh, timer_0,
	    (int_8254_timecounter.tc_frequency / hz) % 256);
	wbflush();
	delay(4);
	bus_space_write_1(iot, ioh, timer_0,
	    (int_8254_timecounter.tc_frequency / hz) / 256);

	/* Timer1 is for timecounting. */
	bus_space_write_1(iot, ioh, timer_control,
	    TIMER_SEL1 | TIMER_RATEGEN | TIMER_16BIT);
	bus_space_write_1(iot, ioh, timer_1, 0xff);
	wbflush();
	delay(4);
	bus_space_write_1(iot, ioh, timer_1, 0xff);

	/* Timer2 clocks timer0 and timer1. */
	bus_space_write_1(iot, ioh, timer_control,
	    TIMER_SEL2 | TIMER_RATEGEN | TIMER_16BIT);
	bus_space_write_1(iot, ioh, timer_2, 8);
	wbflush();
	delay(4);
	bus_space_write_1(iot, ioh, timer_2, 0);

	splx(s);

	tc_init(&int_8254_timecounter);
}

static u_int
int_8254_get_timecount(struct timecounter *tc)
{
	int s;
	u_int count;
	u_char lo, hi;

	s = splhigh();

	switch (mach_type) {
	case MACH_SGI_IP6 | MACH_SGI_IP10:
		bus_space_write_1(iot, ioh, INT1_TIMER_CONTROL,
		    TIMER_SEL1 | TIMER_LATCH);
		lo = bus_space_read_1(iot, ioh, INT1_TIMER_1);
		hi = bus_space_read_1(iot, ioh, INT1_TIMER_1);
		break;

	case MACH_SGI_IP12:
		bus_space_write_1(iot, ioh, INT2_TIMER_CONTROL,
		    TIMER_SEL1 | TIMER_LATCH);
		lo = bus_space_read_1(iot, ioh, INT2_TIMER_1);
		hi = bus_space_read_1(iot, ioh, INT2_TIMER_1);
		break;

	default:
		panic("int_8254_get_timecount");
	}

	count = 0xffff - ((hi << 8) | lo);
	splx(s);

	return (int_8254_tc_count + count);
}

static void
int_8254_intr0(vaddr_t pc, uint32_t status, uint32_t ipending)
{
	struct clockframe cf;

	cf.pc = pc;
	cf.sr = status;
	cf.intr = (curcpu()->ci_idepth > 1);

	hardclock(&cf);

	switch (mach_type) {
	case MACH_SGI_IP6 | MACH_SGI_IP10:
		bus_space_read_1(iot, ioh, INT1_TIMER_0_ACK);
		break;

	case MACH_SGI_IP12:
		bus_space_write_1(iot, ioh, INT2_TIMER_CLEAR, 0x01);
		break;

	default:
		panic("int_8254_intr0");
	}
}

static void
int_8254_intr1(vaddr_t pc, uint32_t status, uint32_t ipending)
{
	int s;

	s = splhigh();

	int_8254_tc_count += 0xffff;
	switch (mach_type) {
	case MACH_SGI_IP6 | MACH_SGI_IP10:
		bus_space_read_1(iot, ioh, INT1_TIMER_1_ACK);
		break;

	case MACH_SGI_IP12:
		bus_space_write_1(iot, ioh, INT2_TIMER_CLEAR, 0x02);
		break;

	default:
		panic("int_8254_intr1");
	}

	splx(s);
}

void
int2_wait_fifo(uint32_t flag)
{

	if (ioh == 0)
		delay(5000);
	else
		while (bus_space_read_1(iot, ioh, INT2_LOCAL0_STATUS) & flag)
			;
}
