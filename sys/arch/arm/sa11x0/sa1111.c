/*      $NetBSD: sa1111.c,v 1.10 2003/07/15 00:24:50 lukem Exp $	*/

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
 *   - separate machine specific attach code
 *   - introduce bus abstraction to support SA1101
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sa1111.c,v 1.10 2003/07/15 00:24:50 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/uio.h>

#include <machine/bus.h>
#ifdef hpcarm
#include <machine/platid.h>
#include <machine/platid_mask.h>
#endif

#include <arm/sa11x0/sa11x0_reg.h>
#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa11x0_gpioreg.h>
#include <arm/sa11x0/sa1111_reg.h>
#include <arm/sa11x0/sa1111_var.h>

static	int	sacc_probe(struct device *, struct cfdata *, void *);
static	void	sacc_attach(struct device *, struct device *, void *);
static	int	sa1111_search(struct device *, struct cfdata *, void *);
static	int	sa1111_print(void *, const char *);

static void	sacc_intr_calculatemasks(struct sacc_softc *);
static void	sacc_intr_setpolarity(sacc_chipset_tag_t *, int , int);
int		sacc_intr(void *);

#ifndef hpcarm
void *softintr_establish(int, int (*)(void *), void *);
void softintr_schedule(void *);
#endif

#ifdef hpcarm
struct platid_data sacc_platid_table[] = {
	{ &platid_mask_MACH_HP_JORNADA_720, (void *)1 },
	{ &platid_mask_MACH_HP_JORNADA_720JP, (void *)1 },
	{ NULL, NULL }
};
#endif

CFATTACH_DECL(sacc, sizeof(struct sacc_softc),
    sacc_probe, sacc_attach, NULL, NULL);

#ifdef INTR_DEBUG
#define DPRINTF(arg)	printf arg
#else
#define DPRINTF(arg)
#endif

static int
sacc_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct sa11x0_attach_args *sa = aux;
	bus_space_handle_t ioh;
	u_int32_t skid;

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0, &ioh))
		return (0);

	skid = bus_space_read_4(sa->sa_iot, ioh, SACCSBI_SKID);
	bus_space_unmap(sa->sa_iot, ioh, sa->sa_size);

	if ((skid & 0xffffff00) != 0x690cc200)
		return (0);

	return (1);
}

static void
sacc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	int i, gpiopin;
	u_int32_t skid;
	struct sacc_softc *sc = (struct sacc_softc *)self;
	struct sa11x0_softc *psc = (struct sa11x0_softc *)parent;
	struct sa11x0_attach_args *sa = aux;
#ifdef hpcarm
	struct platid_data *p;
#endif

	printf("\n");

	sc->sc_iot = sa->sa_iot;
	sc->sc_piot = psc->sc_iot;
	sc->sc_gpioh = psc->sc_gpioh;
#ifdef hpcarm
	if ((p = platid_search_data(&platid, sacc_platid_table)) == NULL)
		return;

	gpiopin = (int) p->data;
#else
	gpiopin = sa->sa_gpio;
#endif
	sc->sc_gpiomask = 1 << gpiopin;

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0,
			  &sc->sc_ioh)) {
		printf("%s: unable to map registers\n", sc->sc_dev.dv_xname);
		return;
	}

	skid = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACCSBI_SKID);

	printf("%s: SA1111 rev %d.%d\n", sc->sc_dev.dv_xname,
	       (skid & 0xf0) >> 3, skid & 0xf);

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
	sa11x0_intr_establish(0, gpiopin, 1, IPL_SERIAL, sacc_intr, sc);

	/*
	 *  Attach each devices
	 */
	config_search(sa1111_search, self, NULL);
}

static int
sa1111_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
        if (config_match(parent, cf, NULL) > 0)
                config_attach(parent, cf, NULL, sa1111_print);

        return 0;
}

static int
sa1111_print(aux, name)
	void *aux;
	const char *name;
{
	return (UNCONF);
}

int
sacc_intr(arg)
	void *arg;
{
	int i;
	u_int32_t mask;
	struct sacc_intrvec intstat;
	struct sacc_softc *sc = arg;
#ifdef hpcarm
	struct sacc_intrhand *ih;
#endif

	intstat.lo =
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTSTATCLR0);
	intstat.hi =
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTSTATCLR1);
	DPRINTF(("sacc_intr_dispatch: %x %x\n", intstat.lo, intstat.hi));

	/* clear SA1110's GPIO intr status */
	bus_space_write_4(sc->sc_piot, sc->sc_gpioh,
			  SAGPIO_EDR, sc->sc_gpiomask);

	for(i = 0, mask = 1; i < 32; i++, mask <<= 1)
		if (intstat.lo & mask) {
			/*
			 * Clear intr status before calling intr handlers.
			 * This cause stray interrupts, but clearing
			 * after calling intr handlers cause intr lossage.
			 */
			bus_space_write_4(sc->sc_iot, sc->sc_ioh,
					  SACCIC_INTSTATCLR0, 1 << i);

#ifdef hpcarm
			for(ih = sc->sc_intrhand[i]; ih; ih = ih->ih_next)
				softintr_schedule(ih->ih_soft);
#endif
		}
	for(i = 0, mask = 1; i < SACCIC_LEN - 32; i++, mask <<= 1)
		if (intstat.hi & mask) {
			bus_space_write_4(sc->sc_iot, sc->sc_ioh,
					  SACCIC_INTSTATCLR1, 1 << i);
#ifdef hpcarm
			for(ih = sc->sc_intrhand[i + 32]; ih; ih = ih->ih_next)
				softintr_schedule(ih->ih_soft);
#endif
		}
	return 1;
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

	if (sc->sc_intrhand[irq] == NULL) {
		sacc_intr_setpolarity(ic, irq, type);
		sc->sc_intrtype[irq] = type;
	} else if (sc->sc_intrtype[irq] != type)
		/* XXX we should be able to share raising and
		 * falling edge intrs */
		panic("sacc_intr_establish: type must be unique");

	/* install intr handler */
#ifdef hpcarm
	ih->ih_soft = softintr_establish(level, (void (*)(void *)) ih_fun,
					 ih_arg);
#endif
	ih->ih_irq = irq;
	ih->ih_next = NULL;

	s = splhigh();
	for(p = &sc->sc_intrhand[irq]; *p; p = &(*p)->ih_next)
		;

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

	for(p = &sc->sc_intrhand[irq];; p = &(*p)->ih_next) {
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

	sc->sc_imask.lo = 0;
	sc->sc_imask.hi = 0;
	for(irq = 0; irq < 32; irq++)
		if (sc->sc_intrhand[irq])
			sc->sc_imask.lo |= (1 << irq);
	for(irq = 0; irq < SACCIC_LEN - 32; irq++)
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
