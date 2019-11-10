/*      $NetBSD: sa1111.c,v 1.25 2019/11/10 21:16:24 chs Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * TODO:
 *   - introduce bus abstraction to support SA-1101
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sa1111.c,v 1.25 2019/11/10 21:16:24 chs Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/uio.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <arm/sa11x0/sa11x0_reg.h>
#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa11x0_gpioreg.h>
#include <arm/sa11x0/sa1111_reg.h>
#include <arm/sa11x0/sa1111_var.h>

#include "locators.h"

static int	sa1111_print(void *, const char *);

static void	sacc_intr_calculatemasks(struct sacc_softc *);
static void	sacc_intr_setpolarity(sacc_chipset_tag_t *, int , int);

#ifdef INTR_DEBUG
#define DPRINTF(arg)	printf arg
#else
#define DPRINTF(arg)
#endif

int
sacc_probe(device_t parent, cfdata_t match, void *aux)
{
	struct sa11x0_attach_args *sa = aux;
	bus_space_handle_t ioh;
	uint32_t skid;

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0, &ioh))
		return 0;

	skid = bus_space_read_4(sa->sa_iot, ioh, SACCSBI_SKID);
	bus_space_unmap(sa->sa_iot, ioh, sa->sa_size);

	if ((skid & 0xffffff00) != 0x690cc200)
		return 0;

	return 1;
}


int
sa1111_search(device_t parent, cfdata_t cf, const int *ldesc,
    void *aux)
{
	struct sa1111_attach_args aa;

	aa.sa_addr = cf->cf_loc[SACCCF_ADDR];
	aa.sa_size = cf->cf_loc[SACCCF_SIZE];
	aa.sa_intr = cf->cf_loc[SACCCF_INTR];
#if 0
	aa.sa_membase = cf->cf_loc[SACCCF_MEMBASE];
	aa.sa_memsize = cf->cf_loc[SACCCF_MEMSIZE];
#endif

        if (config_match(parent, cf, &aa) > 0)
                config_attach(parent, cf, &aa, sa1111_print);

        return 0;
}

static int
sa1111_print(void *aux, const char *name)
{

	return UNCONF;
}


void *
sacc_intr_establish(sacc_chipset_tag_t *ic, int irq, int type, int level,
    int (*ih_fun)(void *), void *ih_arg)
{
	int s;
	struct sacc_softc *sc = (struct sacc_softc *)ic;
	struct sacc_intrhand **p, *ih;

	ih = malloc(sizeof *ih, M_DEVBUF, M_WAITOK);

	if (irq < 0 || irq > SACCIC_LEN ||
	    !(type == IST_EDGE_RAISE || type == IST_EDGE_FALL))
		panic("sacc_intr_establish: bogus irq or type");

	if (sc->sc_intrhand[irq] == NULL) {
		sacc_intr_setpolarity(ic, irq, type);
		sc->sc_intrtype[irq] = type;
	} else if (sc->sc_intrtype[irq] != type)
		/* XXX we should be able to share raising and
		 * falling edge intrs */
		panic("sacc_intr_establish: type must be unique");

	/* install intr handler */
	/* map interrupt level to appropriate softinterrupt level */
	level = SOFTINT_SERIAL;
	ih->ih_soft = softint_establish(level, (void (*)(void *)) ih_fun,
					 ih_arg);
	ih->ih_irq = irq;
	ih->ih_next = NULL;

	s = splhigh();
	for (p = &sc->sc_intrhand[irq]; *p; p = &(*p)->ih_next)
		continue;

	*p = ih;

	sacc_intr_calculatemasks(sc);
	splx(s);

	return ih;
}

void
sacc_intr_disestablish(sacc_chipset_tag_t *ic, void *arg)
{
	int irq, s;
	struct sacc_softc *sc = (struct sacc_softc *)ic;
	struct sacc_intrhand *ih, **p;

	ih = (struct sacc_intrhand *)arg;
	irq = ih->ih_irq;

#ifdef DIAGNOSTIC
	if (irq < 0 || irq > SACCIC_LEN)
		panic("sacc_intr_disestablish: bogus irq");
#endif

	s = splhigh();

	for (p = &sc->sc_intrhand[irq];; p = &(*p)->ih_next) {
		if (*p == NULL)
			panic("sacc_intr_disestablish: handler not registered");
		if (*p == ih)
			break;
	}
	*p = (*p)->ih_next;

	sacc_intr_calculatemasks(sc);
	splx(s);

	free(ih, M_DEVBUF);
}

static void
sacc_intr_setpolarity(sacc_chipset_tag_t *ic, int irq, int type)
{
	struct sacc_softc *sc = (struct sacc_softc *)ic;
	int s;
	uint32_t pol, mask;
	int addr;

	if (irq >= 32) {
		addr = SACCIC_INTPOL1;
		irq -= 32;
	} else
		addr = SACCIC_INTPOL0;

	mask = (1 << irq);

	s = splhigh();
	pol = bus_space_read_4(sc->sc_iot, sc->sc_ioh, addr);
	if (type == IST_EDGE_RAISE)
		pol &= ~mask;
	else
		pol |= mask;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, addr, pol);
	splx(s);
}

static void
sacc_intr_calculatemasks(struct sacc_softc *sc)
{
	int irq;

	sc->sc_imask.lo = 0;
	sc->sc_imask.hi = 0;
	for (irq = 0; irq < 32; irq++)
		if (sc->sc_intrhand[irq])
			sc->sc_imask.lo |= (1 << irq);
	for (irq = 0; irq < SACCIC_LEN - 32; irq++)
		if (sc->sc_intrhand[irq + 32])
			sc->sc_imask.hi |= (1 << irq);


	/* XXX this should not be done here */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTEN0,
			  sc->sc_imask.lo);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTEN1,
			  sc->sc_imask.hi);
	DPRINTF(("sacc_intr_calculatemasks: %x %x\n", sc->sc_imask.lo,
	    sc->sc_imask.hi));
}
