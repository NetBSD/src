/*      $NetBSD: sa1111.c,v 1.1.2.4 2001/04/21 17:53:35 bouyer Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 *   - implement IPL in intr handler
 *   - separate machine specific attach code
 *   - introduce bus abstraction to support SA1101
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/uio.h>

#include <machine/bus.h>
#include <hpcarm/sa11x0/sa11x0_reg.h>
#include <hpcarm/sa11x0/sa11x0_var.h>
#include <hpcarm/sa11x0/sa11x0_gpioreg.h>
#include <hpcarm/sa11x0/sa1111_reg.h>
#include <hpcarm/sa11x0/sa1111_var.h>

static	int	sacc_match(struct device *, struct cfdata *, void *);
static	void	sacc_attach(struct device *, struct device *, void *);
static	int	sa1111_search(struct device *, struct cfdata *, void *);
static	int	sa1111_print(void *, const char *);

static int	sacc_intr_dispatch(void *);
static void	sacc_stray_interrupt(struct sacc_softc *, int);
static void	sacc_intr_calculatemasks(struct sacc_softc *);
static void	sacc_intr_setpolarity(sacc_chipset_tag_t *, int , int);

struct cfattach sacc_ca = {
	sizeof(struct sacc_softc), sacc_match, sacc_attach
};

#ifdef INTR_DEBUG
#define DPRINTF(arg)	printf arg
#else
#define DPRINTF(arg)
#endif

static int
sacc_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (0);
}

static void
sacc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	int i;
	struct sacc_softc *sc = (struct sacc_softc *)self;
	struct sa11x0_softc *psc = (struct sa11x0_softc *)parent;
	struct sa11x0_attach_args *sa = aux;

	sc->sc_iot = sa->sa_iot;
	sc->sc_piot = psc->sc_iot;
	sc->sc_gpioh = psc->sc_gpioh;
	sc->sc_gpiomask = 2;	/* XXX jornada 720 */

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0,
			  &sc->sc_ioh)) {
		printf("%s: unable to map registers\n", sc->sc_dev.dv_xname);
		return;
	}

	for(i = 0; i < SACCIC_LEN; i++)
		sc->sc_intrhand[i] = NULL;

	/* initialize SA1111 interrupt controller */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTEN0, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTEN1, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTTSTSEL, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  SACCIC_INTSTATCLR0, 0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  SACCIC_INTSTATCLR1, 0xffffffff);

	/* connect to SA1110's GPIO intr */
#ifdef notyet
	sa11x0_cascade_intr_establish(0, xx, 1, sacc_intr_probe, sc);
#endif
	sa11x0_intr_establish(0, 1 /* XXX pdattach->sacc->irq */,
	    1, IPL_BIO, sacc_intr_dispatch, sc);

	/*
	 *  Attach each devices
	 */
	config_search(sa1111_search, self, sa1111_print);
}

static int
sa1111_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
        if ((*cf->cf_attach->ca_match)(parent, cf, NULL) > 0)
                config_attach(parent, cf, NULL, sa1111_print);

        return 0;
}

static int
sa1111_print(aux, name)
	void *aux;
	const char *name;
{
	printf("\n");
	return (UNCONF);
}

static int
sacc_intr_dispatch(arg)
	void *arg;
{
	int i, handled;
	u_int32_t mask;
	struct sacc_intrvec intstat;
	struct sacc_softc *sc = arg;
	struct sacc_intrhand *ih;

	intstat.lo =
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTSTATCLR0);
	intstat.hi =
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTSTATCLR1);
	DPRINTF(("sacc_intr_dispatch: %x %x\n", intstat.lo, intstat.hi));

	/* process the intrs which have the highest IPL */
#ifdef notyet
	intstat.lo &= sc->sc_imask[xx].lo;
	intstat.hi &= sc->sc_imask[xx].hi;
#endif
	for(i = 0, mask = 1; i < 32; i++, mask <<= 1)
		if (intstat.lo & mask) {
			/* clear SA1110's GPIO intr status */
			bus_space_write_4(sc->sc_piot, sc->sc_gpioh,
					  SAGPIO_EDR, sc->sc_gpiomask);
			/*
			 * Clear intr status before calling intr handlers.
			 * This cause stray interrupts, but clearing
			 * after calling intr handlers cause intr lossage.
			 */
			bus_space_write_4(sc->sc_iot, sc->sc_ioh,
					  SACCIC_INTSTATCLR0, 1 << i);

			handled = 0;
			for(ih = sc->sc_intrhand[i]; ih; ih = ih->ih_next)
				handled = handled |
				    ((ih->ih_fun)(ih->ih_arg) == 1);

			if (! handled)
				sacc_stray_interrupt(sc, i + 32);
		}
	for(i = 0, mask = 1; i < SACCIC_LEN - 32; i++, mask <<= 1)
		if (intstat.hi & mask) {
			/* clear SA1110's GPIO intr status */
			bus_space_write_4(sc->sc_piot, sc->sc_gpioh,
					  SAGPIO_EDR, sc->sc_gpiomask);
			bus_space_write_4(sc->sc_iot, sc->sc_ioh,
					  SACCIC_INTSTATCLR1, 1 << i);

			handled = 0;
			for(ih = sc->sc_intrhand[i + 32]; ih; ih = ih->ih_next)
				handled = handled |
				    ((ih->ih_fun)(ih->ih_arg) == 1);

			if (! handled)
				sacc_stray_interrupt(sc, i + 32);
		}
	return 1;
}

static void
sacc_stray_interrupt(sc, irq)
	struct sacc_softc *sc;
	int irq;
{
	DPRINTF(("sacc_stray_interrupt\n"));
}

void *
sacc_intr_establish(ic, irq, type, level, ih_fun, ih_arg)
	sacc_chipset_tag_t *ic;
	int irq, type, level;
	int (*ih_fun)(void *);
	void *ih_arg;
{
	int s;
	struct sacc_softc *sc = (struct sacc_softc *)ic;
	struct sacc_intrhand **p, *ih;

	/* no point in sleeping unless someone can free memory. */
	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("sacc_intr_establish: can't malloc handler info");

	if (irq < 0 || irq > SACCIC_LEN ||
	    ! (type == IST_EDGE_RAISE || type == IST_EDGE_FALL))
		panic("sacc_intr_establish: bogus irq or type");

	/* install intr handler */
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_irq = irq;
	ih->ih_type = type;
	ih->ih_next = NULL;

	s = splhigh();
	for(p = &sc->sc_intrhand[irq]; *p; p = &(*p)->ih_next)
		if ((*p)->ih_type != type)
			/* XXX we should be able to share raising and
			 * falling edge intrs */
			panic("sacc_intr_establish: type must be unique\n");

	if (sc->sc_intrhand[irq] == NULL)
		sacc_intr_setpolarity(ic, irq, type);

	*p = ih;

	sacc_intr_calculatemasks(sc);
	splx(s);

	return(ih);
}

void
sacc_intr_disestablish(ic, arg)
	sacc_chipset_tag_t *ic;
	void *arg;
{
	int irq;
	struct sacc_softc *sc = (struct sacc_softc *)ic;
	struct sacc_intrhand *ih, **p;

	ih = (struct sacc_intrhand *)arg;
	irq = ih->ih_irq;

#ifdef DIAGNOSTIC
	if (irq < 0 || irq > SACCIC_LEN)
		panic("sacc_intr_disestablish: bogus irq");
#endif

	for(p = &sc->sc_intrhand[irq];; p = &(*p)->ih_next) {
		if (*p == NULL)
			panic("sacc_intr_disestablish: handler not registered");
		if (*p == ih)
			break;
	}
	*p = (*p)->ih_next;
	free(ih, M_DEVBUF);
}

void
sacc_intr_setpolarity(ic, irq, type)
	sacc_chipset_tag_t *ic;
	int irq;
	int type;
{
	struct sacc_softc *sc = (struct sacc_softc *)ic;
	int s;
	u_int32_t pol, mask;
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

void
sacc_intr_calculatemasks(sc)
	struct sacc_softc *sc;
{
	int irq;

	sc->sc_imask[0].lo = 0;
	sc->sc_imask[0].hi = 0;
	for(irq = 0; irq < 32; irq++)
		if (sc->sc_intrhand[irq])
			sc->sc_imask[0].lo |= (1 << irq);
	for(irq = 0; irq < SACCIC_LEN - 32; irq++)
		if (sc->sc_intrhand[irq + 32])
			sc->sc_imask[0].hi |= (1 << irq);


	/* XXX this should not be done here */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTEN0,
			  sc->sc_imask[0].lo);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTEN1,
			  sc->sc_imask[0].hi);
	DPRINTF(("sacc_intr_calculatemasks: %x %x\n", sc->sc_imask[0].lo,
	    sc->sc_imask[0].hi));
}
