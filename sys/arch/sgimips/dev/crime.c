/*	$NetBSD: crime.c,v 1.24 2007/04/16 23:31:04 jmcneill Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: crime.c,v 1.24 2007/04/16 23:31:04 jmcneill Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/machtype.h>
#include <machine/sysconf.h>

#include <sgimips/dev/crimevar.h>
#include <sgimips/dev/crimereg.h>
#include <sgimips/mace/macevar.h>

#include "locators.h"

#define CRIME_DISABLE_WATCHDOG

static int	crime_match(struct device *, struct cfdata *, void *);
static void	crime_attach(struct device *, struct device *, void *);
void		crime_bus_reset(void);
void		crime_watchdog_reset(void);
void		crime_watchdog_disable(void);
void		crime_intr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
void		*crime_intr_establish(int, int, int (*)(void *), void *);
extern	void  mace_intr(int); /* XXX */

static bus_space_tag_t crm_iot;
static bus_space_handle_t crm_ioh;

CFATTACH_DECL(crime, sizeof(struct crime_softc),
    crime_match, crime_attach, NULL, NULL);

#define CRIME_NINTR 32 	/* XXX */

struct {
	int	(*func)(void *);
	void	*arg;
} crime[CRIME_NINTR];

static int
crime_match(struct device *parent, struct cfdata *match, void *aux)
{

	/*
	 * The CRIME is in the O2.
	 */
	if (mach_type == MACH_SGI_IP32)
		return (1);

	return (0);
}

static void
crime_attach(struct device *parent, struct device *self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	u_int64_t crm_id;
	u_int64_t baseline;
	u_int32_t cps;

	crm_iot = SGIMIPS_BUS_SPACE_CRIME;

	if (bus_space_map(crm_iot, ma->ma_addr, 0 /* XXX */,
	    BUS_SPACE_MAP_LINEAR, &crm_ioh))
		panic("crime_attach: can't map I/O space");

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

	aprint_normal(" (CRIME_ID: %llx)\n", crm_id);

	/* reset CRIME CPU & memory error registers */
	crime_bus_reset();

	bus_space_write_8(crm_iot, crm_ioh, CRIME_WATCHDOG, 0);
	bus_space_write_8(crm_iot, crm_ioh, CRIME_CONTROL, 0);

	baseline = bus_space_read_8(crm_iot, crm_ioh, CRIME_TIME) & CRIME_TIME_MASK;
	cps = mips3_cp0_count_read();

	while (((bus_space_read_8(crm_iot, crm_ioh, CRIME_TIME) & CRIME_TIME_MASK)
		- baseline) < 50 * 1000000 / 15)
		continue;
	cps = mips3_cp0_count_read() - cps;
	cps = cps / 5;

	/* Counter on R4k/R4400/R4600/R5k counts at half the CPU frequency */
	curcpu()->ci_cpu_freq = cps * 2 * hz;
	curcpu()->ci_cycles_per_hz = curcpu()->ci_cpu_freq / (2 * hz);
	curcpu()->ci_divisor_delay = curcpu()->ci_cpu_freq / (2 * 1000000);
	MIPS_SET_CI_RECIPRICAL(curcpu());

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
crime_intr(u_int32_t status, u_int32_t cause, u_int32_t pc, u_int32_t ipending)
{
	u_int64_t crime_intmask;
	u_int64_t crime_intstat;
	u_int64_t crime_ipending;
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
			u_int64_t address =
				bus_space_read_8(crm_iot, crm_ioh, CRIME_MEM_ERROR_ADDR);
			u_int64_t status1 =
				bus_space_read_8(crm_iot, crm_ioh, CRIME_MEM_ERROR_STAT);
			printf("crime: memory error address %llx"
				" status %llx\n", address << 2, status1);
			crime_bus_reset();
		}

		if (crime_ipending & CRIME_INT_CRMERR) {
			u_int64_t stat =
				bus_space_read_8(crm_iot, crm_ioh, CRIME_CPU_ERROR_STAT);
				printf("crime: cpu error %llx at"
					" address %llx\n", stat,
					bus_space_read_8(crm_iot, crm_ioh, CRIME_CPU_ERROR_ADDR));
			crime_bus_reset();
		}
	}

	crime_ipending &= ~0xffff;

	if (crime_ipending)
		for (i = 16; i < CRIME_NINTR; i++) {
			if ((crime_ipending & (1 << i)) && crime[i].func != NULL)
				(*crime[i].func)(crime[i].arg);
		}
}

void
crime_intr_mask(unsigned int intr)
{
	u_int64_t mask;

	mask = bus_space_read_8(crm_iot, crm_ioh, CRIME_INTMASK);
	mask |= (1 << intr);
	bus_space_write_8(crm_iot, crm_ioh, CRIME_INTMASK, mask);
}

void
crime_intr_unmask(unsigned int intr)
{
	u_int64_t mask;

	mask = bus_space_read_8(crm_iot, crm_ioh, CRIME_INTMASK);
	mask &= ~(1 << intr);
	bus_space_write_8(crm_iot, crm_ioh, CRIME_INTMASK, mask);
}

void
crime_bus_reset(void)
{
	bus_space_write_8(crm_iot, crm_ioh, CRIME_CPU_ERROR_STAT, 0);
	bus_space_write_8(crm_iot, crm_ioh, CRIME_MEM_ERROR_STAT, 0);
}

void
crime_watchdog_reset(void)
{
#ifdef CRIME_WATCHDOG_DISABLE
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
	u_int64_t reg;

	bus_space_write_8(crm_iot, crm_ioh, CRIME_WATCHDOG, 0);
	reg = bus_space_read_8(crm_iot, crm_ioh, CRIME_CONTROL)
			& ~CRIME_CONTROL_DOG_ENABLE;
	bus_space_write_8(crm_iot, crm_ioh, CRIME_CONTROL, reg);
}
