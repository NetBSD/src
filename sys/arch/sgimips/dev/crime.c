/*	$NetBSD: crime.c,v 1.36.12.1 2014/08/20 00:03:22 tls Exp $	*/

/*
 * Copyright (c) 2004 Christopher SEKIYA
 * Copyright (c) 2000 Soren S. Jorvang
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
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
 * O2 CRIME
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: crime.c,v 1.36.12.1 2014/08/20 00:03:22 tls Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/cpu.h>

#include <machine/locore.h>
#include <machine/autoconf.h>
#include <sys/bus.h>
#include <machine/intr.h>
#include <machine/machtype.h>
#include <machine/sysconf.h>

#include <sgimips/dev/crimevar.h>
#include <sgimips/dev/crimereg.h>
#include <sgimips/mace/macevar.h>

#include "locators.h"

#define DISABLE_CRIME_WATCHDOG

static int	crime_match(device_t, struct cfdata *, void *);
static void	crime_attach(device_t, device_t, void *);
static void	crime_mem_reset(void);
static void	crime_cpu_reset(void);
static void	crime_bus_reset(void);
void		crime_watchdog_reset(void);
void		crime_watchdog_disable(void);
void		crime_intr(vaddr_t, uint32_t, uint32_t);
void		*crime_intr_establish(int, int, int (*)(void *), void *);

static bus_space_tag_t crm_iot;
static bus_space_handle_t crm_ioh;

CFATTACH_DECL_NEW(crime, sizeof(struct crime_softc),
    crime_match, crime_attach, NULL, NULL);

#define CRIME_NINTR 32 	/* XXX */

struct {
	int	(*func)(void *);
	void	*arg;
} crime[CRIME_NINTR];

static int
crime_match(device_t parent, struct cfdata *match, void *aux)
{

	/*
	 * The CRIME is in the O2.
	 */
	if (mach_type == MACH_SGI_IP32)
		return 1;

	return 0;
}

static void
crime_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	struct cpu_info * const ci = curcpu();
	struct crime_softc *sc = device_private(self);
	uint64_t crm_id;
	uint64_t baseline, endline;
	uint32_t startctr, endctr, cps;

	sc->sc_dev = self;
	crm_iot = SGIMIPS_BUS_SPACE_CRIME;

	if (bus_space_map(crm_iot, ma->ma_addr, 0 /* XXX */,
	    BUS_SPACE_MAP_LINEAR, &crm_ioh))
		panic("%s: can't map I/O space", __func__);

	crm_id = bus_space_read_8(crm_iot, crm_ioh, CRIME_REV);

	aprint_naive(": system ASIC");

	switch ((crm_id & CRIME_ID_IDBITS) >> CRIME_ID_IDSHIFT) {
	case 0x0b:
		aprint_normal(": rev 1.5");
		break;

	case 0x0a:
		if ((crm_id >> 32) == 0)
			aprint_normal(": rev 1.1");
		else if ((crm_id >> 32) == 1)
			aprint_normal(": rev 1.3");
		else
			aprint_normal(": rev 1.4");
		break;

	case 0x00:
		aprint_normal(": Petty CRIME");
		break;

	default:
		aprint_normal(": Unknown CRIME");
		break;
	}

	aprint_normal(" (CRIME_ID: %" PRIu64 ")\n", crm_id);

	/* reset CRIME CPU & memory error registers */
	crime_mem_reset();
	crime_cpu_reset();

	crime_watchdog_disable();

#define CRIME_TIMER_FREQ	66666666	/* crime clock is 66.7MHz */

	baseline = bus_space_read_8(crm_iot, crm_ioh, CRIME_TIME)
	    & CRIME_TIME_MASK;
	startctr = mips3_cp0_count_read();

	/* read both cp0 and crime counters for 100ms */
	do {
		endline = bus_space_read_8(crm_iot, crm_ioh, CRIME_TIME)
		    & CRIME_TIME_MASK;
		endctr = mips3_cp0_count_read();
	} while (endline - baseline < (CRIME_TIMER_FREQ / 10));

	cps = (endctr - startctr) * 10;
	ci->ci_cpu_freq = cps;
	ci->ci_cctr_freq = cps;
	if (mips_options.mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT)
		ci->ci_cpu_freq *= 2;
	ci->ci_cycles_per_hz = (cps + (hz / 2)) / hz;
	ci->ci_divisor_delay = (cps + (1000000 / 2)) / 1000000;

	/* Turn on memory error and crime error interrupts.
	   All others turned on as devices are registered. */
	bus_space_write_8(crm_iot, crm_ioh, CRIME_INTMASK,
	    CRIME_INT_MEMERR |
	    CRIME_INT_CRMERR |
	    CRIME_INT_VICE |
	    CRIME_INT_VID_OUT |
	    CRIME_INT_VID_IN2 |
	    CRIME_INT_VID_IN1);
	bus_space_write_8(crm_iot, crm_ioh, CRIME_INTSTAT, 0);
	bus_space_write_8(crm_iot, crm_ioh, CRIME_SOFTINT, 0);
	bus_space_write_8(crm_iot, crm_ioh, CRIME_HARDINT, 0);

	platform.bus_reset = crime_bus_reset;
	platform.watchdog_reset = crime_watchdog_reset;
	platform.watchdog_disable = crime_watchdog_disable;
	platform.watchdog_enable = crime_watchdog_reset;
	platform.intr_establish = crime_intr_establish;
	platform.intr0 = crime_intr;
}

/*
 * XXX: sharing interrupts?
 */

void *
crime_intr_establish(int irq, int level, int (*func)(void *), void *arg)
{

	if (irq < 16)
		return mace_intr_establish(irq, level, func, arg);

	if (crime[irq].func != NULL)
		return NULL;	/* panic("Cannot share CRIME interrupts!"); */

	crime[irq].func = func;
	crime[irq].arg = arg;

	crime_intr_mask(irq);

	return (void *)&crime[irq];
}

void
crime_intr(vaddr_t pc, uint32_t status, uint32_t ipending)
{
	uint64_t crime_intmask;
	uint64_t crime_intstat;
	uint64_t crime_ipending;
	uint64_t address, stat;
	int i;

	crime_intmask = bus_space_read_8(crm_iot, crm_ioh, CRIME_INTMASK);
	crime_intstat = bus_space_read_8(crm_iot, crm_ioh, CRIME_INTSTAT);
	crime_ipending = (crime_intstat & crime_intmask);

	if (crime_ipending & 0xffff)
		mace_intr(crime_ipending & 0xffff);

	if (crime_ipending & 0xffff0000) {
	/*
	 * CRIME interrupts for CPU and memory errors
	 */
		if (crime_ipending & CRIME_INT_MEMERR) {
			address = bus_space_read_8(crm_iot, crm_ioh,
			    CRIME_MEM_ERROR_ADDR);
			stat = bus_space_read_8(crm_iot, crm_ioh,
			    CRIME_MEM_ERROR_STAT);
			printf("crime: memory error address %" PRIu64 
			    " status %" PRIu64 "\n", address << 2, stat);
			crime_mem_reset();
		}

		if (crime_ipending & CRIME_INT_CRMERR) {
			stat = bus_space_read_8(crm_iot, crm_ioh,
			    CRIME_CPU_ERROR_STAT);
			address = bus_space_read_8(crm_iot, crm_ioh,
			    CRIME_CPU_ERROR_ADDR) << 2;
			printf("crime: cpu error %" PRIu64 " at address %"
			       PRIu64 "\n", stat, address);
			crime_cpu_reset();
		}
	}

	crime_ipending &= ~0xffff;

	if (crime_ipending) {
		for (i = 16; i < CRIME_NINTR; i++) {
			if ((crime_ipending & (1 << i)) &&
			    crime[i].func != NULL)
				(*crime[i].func)(crime[i].arg);
		}
	}
}

void
crime_intr_mask(unsigned int intr)
{
	uint64_t mask;

	mask = bus_space_read_8(crm_iot, crm_ioh, CRIME_INTMASK);
	mask |= (1 << intr);
	bus_space_write_8(crm_iot, crm_ioh, CRIME_INTMASK, mask);
}

void
crime_intr_unmask(unsigned int intr)
{
	uint64_t mask;

	mask = bus_space_read_8(crm_iot, crm_ioh, CRIME_INTMASK);
	mask &= ~(1 << intr);
	bus_space_write_8(crm_iot, crm_ioh, CRIME_INTMASK, mask);
}

void
crime_mem_reset(void)
{

	bus_space_write_8(crm_iot, crm_ioh, CRIME_MEM_ERROR_STAT, 0);
}

void
crime_cpu_reset(void)
{

	bus_space_write_8(crm_iot, crm_ioh, CRIME_CPU_ERROR_STAT, 0);
}

void
crime_bus_reset(void)
{

	crime_mem_reset();
	crime_cpu_reset();
}

void
crime_watchdog_reset(void)
{

#ifdef DISABLE_CRIME_WATCHDOG
	bus_space_write_8(crm_iot, crm_ioh, CRIME_WATCHDOG, 0);
#else
	/* enable watchdog timer, clear it */
	bus_space_write_8(crm_iot, crm_ioh,
		CRIME_CONTROL, CRIME_CONTROL_DOG_ENABLE);
	bus_space_write_8(crm_iot, crm_ioh, CRIME_WATCHDOG, 0);
#endif
}

void
crime_watchdog_disable(void)
{
	uint64_t reg;

	bus_space_write_8(crm_iot, crm_ioh, CRIME_WATCHDOG, 0);
	reg = bus_space_read_8(crm_iot, crm_ioh, CRIME_CONTROL)
	    & ~CRIME_CONTROL_DOG_ENABLE;
	bus_space_write_8(crm_iot, crm_ioh, CRIME_CONTROL, reg);
}

void
crime_reboot(void)
{

	bus_space_write_8(crm_iot, crm_ioh,  CRIME_CONTROL,
	    CRIME_CONTROL_HARD_RESET);
	for (;;)
		;
}
