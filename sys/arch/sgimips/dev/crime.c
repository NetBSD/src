/*	$NetBSD: crime.c,v 1.13 2003/10/05 15:38:08 tsutsui Exp $	*/

/*
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
 *          NetBSD Project.  See http://www.netbsd.org/ for
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
__KERNEL_RCSID(0, "$NetBSD: crime.c,v 1.13 2003/10/05 15:38:08 tsutsui Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/machtype.h>

#include <sgimips/dev/crimevar.h>
#include <sgimips/dev/crimereg.h>

#include "locators.h"

static int	crime_match(struct device *, struct cfdata *, void *);
static void	crime_attach(struct device *, struct device *, void *);

struct crime_softc *crime_sc; /* only one per machine, okay to be global */

CFATTACH_DECL(crime, sizeof(struct crime_softc),
    crime_match, crime_attach, NULL, NULL);

#define CRIME_NINTR 32 	/* XXX */

struct {
	int	(*func)(void *);
	void	*arg;
} crime[CRIME_NINTR];

static int
crime_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

	/*
	 * The CRIME is in the O2.
	 */
	if (mach_type == MACH_SGI_IP32)
		return (1);

	return (0);
}

static void
crime_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct crime_softc *sc = (struct crime_softc *)self;
	struct mainbus_attach_args *ma = aux;
	u_int64_t crm_id;

	crime_sc = sc;

	sc->iot = SGIMIPS_BUS_SPACE_HPC;

	if (bus_space_map(sc->iot, ma->ma_addr, 0 /* XXX */,
	    BUS_SPACE_MAP_LINEAR, &sc->ioh))
		panic("crime_attach: can't map I/O space");

	crm_id = bus_space_read_8(sc->iot, sc->ioh, CRIME_REV);

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

	/* Turn on memory error and crime error interrupts.
	   All others turned on as devices are registered. */
	bus_space_write_8(sc->iot, sc->ioh, CRIME_INTMASK,
	    CRIME_INT_MEMERR |
	    CRIME_INT_CRMERR |
	    CRIME_INT_VICE |
	    CRIME_INT_VID_OUT |
	    CRIME_INT_VID_IN2 |
	    CRIME_INT_VID_IN1);
	bus_space_write_8(sc->iot, sc->ioh, CRIME_INTSTAT, 0);
	bus_space_write_8(sc->iot, sc->ioh, CRIME_SOFTINT, 0);
	bus_space_write_8(sc->iot, sc->ioh, CRIME_HARDINT, 0);
}

/*
 * XXX: sharing interrupts?
 */

void *
crime_intr_establish(irq, type, level, func, arg)
	int irq;
	int type;
	int level;
	int (*func)(void *);
	void *arg;
{
	if (crime[irq].func != NULL)
		return NULL;	/* panic("Cannot share CRIME interrupts!"); */

	crime[irq].func = func;
	crime[irq].arg = arg;

	crime_intr_mask(irq);

	return (void *)&crime[irq];
}

void
crime_intr(pendmask)
	u_int pendmask;
{
	int i;

	for (i = 0; i < CRIME_NINTR; i++) {
		if ((pendmask & (1 << i)) && crime[i].func != NULL)
			(*crime[i].func)(crime[i].arg);
	}
}

void
crime_intr_mask(unsigned int intr)
{
	u_int64_t mask;

	mask = bus_space_read_8(crime_sc->iot, crime_sc->ioh, CRIME_INTMASK);
	mask |= (1 << intr);
	bus_space_write_8(crime_sc->iot, crime_sc->ioh, CRIME_INTMASK, mask);
}
