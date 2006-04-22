/*	$NetBSD: gb225.c,v 1.4.6.1 2006/04/22 11:37:23 simonb Exp $ */

/*
 * Copyright (c) 2002, 2003  Genetec corp.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec corp.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec corp. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORP. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORP.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <arm/cpufunc.h>

#include <arm/mainbus/mainbus.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <arm/sa11x0/sa11x0_var.h>
#include <evbarm/g42xxeb/g42xxeb_reg.h>
#include <evbarm/g42xxeb/g42xxeb_var.h>
#include <evbarm/g42xxeb/gb225reg.h>
#include <evbarm/g42xxeb/gb225var.h>

#include "locators.h"

#define OPIO_INTR	G42XXEB_INT_EXT3


#define DEBOUNCE_COUNT	2
#define DEBOUNCE_TICKS	(hz/20)			/* 50ms */

/* prototypes */
static int	opio_match(struct device *, struct cfdata *, void *);
static void	opio_attach(struct device *, struct device *, void *);
static int 	opio_search(struct device *, struct cfdata *,
			    const int *, void *);
static int	opio_print(void *, const char *);
#ifdef OPIO_INTR
static int 	opio_intr( void *arg );
static void	opio_debounce(void *arg);
#endif


/* attach structures */
CFATTACH_DECL(opio, sizeof(struct opio_softc), opio_match, opio_attach,
    NULL, NULL);

/*
 * int opio_print(void *aux, const char *name)
 * print configuration info for children
 */

static int
opio_print(void *aux, const char *name)
{
	struct obio_attach_args *oba = (struct obio_attach_args*)aux;

	if (oba->oba_addr != OPIOCF_ADDR_DEFAULT)
                aprint_normal(" addr 0x%lx", oba->oba_addr);
        return (UNCONF);
}

int
opio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct obio_softc *psc = (struct obio_softc *)parent;
	uint16_t optid;

	optid = bus_space_read_2(psc->sc_iot, psc->sc_obioreg_ioh,
	    G42XXEB_OPTBRDID);

	return optid == 0x01;
}

void
opio_attach(struct device *parent, struct device *self, void *aux)
{
	struct opio_softc *sc = (struct opio_softc*)self;
	struct obio_softc *bsc = (struct obio_softc *)parent;
	struct obio_attach_args *oba = aux;
	uint32_t reg;
	int i;
	bus_space_tag_t iot;
	bus_space_handle_t  memctl_ioh = bsc->sc_memctl_ioh;

	iot = oba->oba_iot;
	sc->sc_iot = iot;
	sc->sc_memctl_ioh = memctl_ioh;

	/* use 16bit access for CS4, 32bit for CS5 */
	reg = bus_space_read_4(iot, memctl_ioh, MEMCTL_MSC2);
	reg |= MSC_RBW;
	reg &= ~(MSC_RBW<<16);
	/* XXX: set access timing */
	bus_space_write_4(iot, memctl_ioh, MEMCTL_MSC2, reg);

	/* Set alternative function for GPIO pings 48..57 on PXA2X0 */
#if 0
	reg = bus_space_read_4(iot, csc->saip.sc_gpioh, GPIO_GPDR1);
	bus_space_write_4(iot, csc->saip.sc_gpioh, GPIO_GPDR1,
	    (reg & ~0x03ff0000) | 0x00ff0000);
	reg = bus_space_read_4(iot, csc->saip.sc_gpioh, GPIO_GAFR1_U);
	bus_space_write_4(iot, csc->saip.sc_gpioh, GPIO_GAFR1_U,
	    (reg & ~0x000fffff) | 0x0005aaaa );
#else
	for (i=47; i <= 55; ++i)
		pxa2x0_gpio_set_function(i, GPIO_ALT_FN_2_OUT);
	pxa2x0_gpio_set_function(56, GPIO_ALT_FN_1_IN);
	pxa2x0_gpio_set_function(57, GPIO_ALT_FN_1_IN);
#endif

	/* Enable bus switch for option board */
	reg = bus_space_read_2(iot, bsc->sc_obioreg_ioh, G42XXEB_EXTCTRL);
	bus_space_write_2(iot, bsc->sc_obioreg_ioh, G42XXEB_EXTCTRL, reg | 3);

	/* Map on-board FPGA registers */
	if( bus_space_map( iot, GB225_PLDREG_BASE, GB225_PLDREG_SIZE,
	    0, &(sc->sc_ioh) ) ){
		aprint_error("%s: can't map FPGA registers\n", self->dv_xname);
	}

	aprint_normal("\n");

	callout_init(&sc->sc_callout);

	for (i=0; i < N_OPIO_INTR; ++i) {
		sc->sc_intr[i].func = NULL;
		sc->sc_intr[i].reported_state = 0xff;
		sc->sc_intr[i].last_state = 0xff;
	}

#ifdef OPIO_INTR
	sc->sc_ih = obio_intr_establish(bsc, OPIO_INTR, IPL_BIO,
	    IST_EDGE_FALLING, opio_intr, sc);
#endif

#ifdef DEBUG
	printf("%s: CF_DET=%x PCMCIA_DET=%x\n",
	    self->dv_xname,
	    bus_space_read_1(sc->sc_iot, sc->sc_ioh, GB225_CFDET),
	    bus_space_read_1(sc->sc_iot, sc->sc_ioh, GB225_PCMCIADET));
#endif

	/*
	 *  Attach each devices
	 */
	config_search_ia(opio_search, self, "opio", NULL);
}

int
opio_search(struct device *parent, struct cfdata *cf,
	    const int *ldesc, void *aux)
{
	struct opio_softc *sc = (struct opio_softc *)parent;
	struct obio_attach_args oba;

	oba.oba_sc = sc;
        oba.oba_iot = sc->sc_iot;
        oba.oba_addr = cf->cf_loc[OPIOCF_ADDR];
        oba.oba_intr = cf->cf_loc[OPIOCF_INTR];

        if (config_match(parent, cf, &oba) > 0)
                config_attach(parent, cf, &oba, opio_print);

        return 0;
}

void *
opio_intr_establish(struct opio_softc *sc, int intr, int ipl,
    int (*func)(void *, int), void *arg)
{
	sc->sc_intr[intr].arg = arg;
	sc->sc_intr[intr].func = func;

	return &sc->sc_intr[intr];
}


#ifdef OPIO_INTR
/*
 * interrupt handler for option board. interrupt sources are:
 *   CF card insertion/removal.
 *   PCMCIA card insertion/removal.
 *   USB power failure.
 *   CF/PCMCIA power failure.
 * We need to debounce for CF/PCMCIA card insertion/removal signal.
 */
static int	
opio_intr( void *arg )
{
	struct opio_softc *sc = (struct opio_softc *)arg;
	struct obio_softc *bsc =
	    (struct obio_softc *)device_parent(&sc->sc_dev);

	/* avoid further interrupts while debouncing */
	obio_intr_mask(bsc, sc->sc_ih);

	printf("OPIO ");

	if (sc->sc_debounce == ST_STABLE) {
		/* start debounce timer */
		callout_reset(&sc->sc_callout, DEBOUNCE_TICKS, 
		    opio_debounce, sc);
		sc->sc_debounce = ST_BOUNCING;
	}

	return 1;
}


static int
do_debounce(struct opio_softc *sc, int intr, int val, int count)
{
	int s;


	struct opio_intr_handler *p = &sc->sc_intr[intr];

	if (p->last_state != val) {
		p->debounce_count = 0;
		p->last_state = val;
		return 1;
	}
	else if (p->debounce_count++ < count)
		return 1;
	else {
		/* debounce done. if status has changed, report it */
		if (p->reported_state != val) {
			p->reported_state = val;
			if (p->func) {
				s = _splraise(p->level);
				p->func(p->arg, val);
				splx(s);
			}
		}
		return 0;
	}
	/*NOTREACHED*/
}

static void
opio_debounce(void *arg)
{
	struct opio_softc *sc = arg;
	struct obio_softc *osc =
	    (struct obio_softc *)device_parent(&sc->sc_dev);
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int flag = 0;
	uint8_t reg;

	reg = bus_space_read_1(iot, ioh, GB225_CFDET) & CARDDET_DET;
	flag |= do_debounce(sc, OPIO_INTR_CF_INSERT, reg, DEBOUNCE_COUNT);

	reg = bus_space_read_1(iot, ioh, GB225_PCMCIADET) & CARDDET_DET;
	flag |= do_debounce(sc, OPIO_INTR_PCMCIA_INSERT, reg, DEBOUNCE_COUNT);

	reg = bus_space_read_1(iot, ioh, GB225_DEVERROR);

	flag |= do_debounce(sc, OPIO_INTR_USB_POWER, reg & DEVERROR_USB, 1);

	flag |= do_debounce(sc, OPIO_INTR_CARD_POWER, reg & DEVERROR_CARD, 1);


	if (flag) {
		/* start debounce timer */
		callout_reset(&sc->sc_callout, DEBOUNCE_TICKS, 
		    opio_debounce, sc);
	}
	else {
		sc->sc_debounce = ST_STABLE;
		obio_intr_unmask(osc, sc->sc_ih);
	}
}
#endif
