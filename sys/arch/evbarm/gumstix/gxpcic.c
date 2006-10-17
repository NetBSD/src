/*	$NetBSD: gxpcic.c,v 1.2 2006/10/17 17:06:22 kiyohara Exp $ */
/*
 * Copyright (C) 2005, 2006 WIDE Project and SOUM Corporation.
 * All rights reserved.
 *
 * Written by Takashi Kiyohara and Susumu Miki for WIDE Project and SOUM
 * Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the name of SOUM Corporation
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT and SOUM CORPORATION ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT AND SOUM CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2002, 2003, 2005  Genetec corp.  All rights reserved.
 *
 * PCMCIA/CF support for TWINTAIL (G4255EB)
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/malloc.h>
#include <uvm/uvm.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>

#include <arch/arm/xscale/pxa2x0var.h>
#include <arch/arm/xscale/pxa2x0reg.h>
#include <arch/arm/sa11x0/sa11xx_pcicvar.h>
#include <arch/evbarm/gumstix/gumstixvar.h>


#ifdef DEBUG
#define DPRINTF(arg)	printf arg
#else
#define DPRINTF(arg)
#endif

#define HAVE_CARD(r)	(!((r) & GPIO_SET))

#define GXIO_GPIO8_RESET	8
#define GXIO_GPIRQ11_nCD	11
#define GXIO_GPIO26_READY	26

struct gxpcic_softc;

struct gxpcic_socket {
	struct sapcic_socket ss;	/* inherit socket for sa11x0 pcic */
};

struct gxpcic_softc {
	struct sapcic_softc sc_pc;	/* inherit SA11xx pcic */

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	int sc_gpirq;

	struct gxpcic_socket sc_socket[2];
	int sc_cards;
};

static int  	gxpcic_match(struct device *, struct cfdata *, void *);
static void  	gxpcic_attach(struct device *, struct device *, void *);

static	int	gxpcic_card_detect(void *);
static	int	gxpcic_read(struct sapcic_socket *, int);
static	void	gxpcic_write(struct sapcic_socket *, int, int);
static	void	gxpcic_set_power(struct sapcic_socket *, int);
static	void	gxpcic_clear_intr(int);
static	void	*gxpcic_intr_establish(struct sapcic_socket *, int,
				       int (*)(void *), void *);
static	void	gxpcic_intr_disestablish(struct sapcic_socket *, void *);

CFATTACH_DECL(gxpcic, sizeof(struct gxpcic_softc),
    gxpcic_match, gxpcic_attach, NULL, NULL);

static struct sapcic_tag gxpcic_tag = {
	gxpcic_read,
	gxpcic_write,
	gxpcic_set_power,
	gxpcic_clear_intr,
	gxpcic_intr_establish,
	gxpcic_intr_disestablish,
};


static int
gxpcic_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct {
		int gpio;
		u_int fn;
	} gpiomodes[] = {
		{ 48, GPIO_ALT_FN_2_OUT },		/* nPOE */
		{ 49, GPIO_ALT_FN_2_OUT },		/* nPWE */
		{ 50, GPIO_ALT_FN_2_OUT },		/* nPIOR */
		{ 51, GPIO_ALT_FN_2_OUT },		/* nPIOW */
		{ 54, GPIO_ALT_FN_2_OUT },		/* pSKTSEL */
		{ 55, GPIO_ALT_FN_2_OUT },		/* nPREG */
		{ 56, GPIO_ALT_FN_1_IN },		/* nPWAIT */
		{ 57, GPIO_ALT_FN_1_IN },		/* nIOIS16 */
		{ -1 }
	};
	u_int reg;
	int i;

	for (i = 0; gpiomodes[i].gpio != -1; i++) {
		reg = pxa2x0_gpio_get_function(gpiomodes[i].gpio);
		if (GPIO_FN(reg) != GPIO_FN(gpiomodes[i].fn) ||
		    GPIO_FN_IS_OUT(reg) != GPIO_FN_IS_OUT(gpiomodes[i].fn))
			break;
	}
	if (gpiomodes[i].gpio != -1)
		return 0;

	return	1;	/* match */
}

static void
gxpcic_attach(struct device *parent, struct device *self, void *aux)
{
	struct gxio_softc *gxsc = (struct gxio_softc *)parent;
	struct gxpcic_softc *sc = (struct gxpcic_softc *)self;
	struct gxio_attach_args *gxa = aux;
	struct pcmciabus_attach_args paa;
	int mecr, val, i, n;

	printf("\n");

	sc->sc_iot = sc->sc_pc.sc_iot = gxa->gxa_iot;
	sc->sc_ioh = gxsc->sc_ioh;
	sc->sc_gpirq = gxa->gxa_gpirq;
	sc->sc_cards = 0;

	n = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MEMCTL_MECR) & MECR_NOS ?
	    2 : 1;
	for (i = 0; i < n; i++) {
		sc->sc_socket[i].ss.sc = &sc->sc_pc;
		sc->sc_socket[i].ss.socket = 0;
		sc->sc_socket[i].ss.pcictag_cookie = NULL;
		sc->sc_socket[i].ss.pcictag = &gxpcic_tag;
		sc->sc_socket[i].ss.event_thread = NULL;
		sc->sc_socket[i].ss.event = 0;
		sc->sc_socket[i].ss.laststatus = SAPCIC_CARD_INVALID;
		sc->sc_socket[i].ss.shutdown = 0;

		/* 3.3V only? */
		sc->sc_socket[i].ss.power_capability = SAPCIC_POWER_3V;

		bus_space_write_4(sc->sc_iot, sc->sc_ioh, MEMCTL_MCMEM(i),
		    MC_TIMING_VAL(10,10,30));
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, MEMCTL_MCATT(i),
		    MC_TIMING_VAL(10,10,30));
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, MEMCTL_MCIO(i),
		    MC_TIMING_VAL(6,6,17));

		paa.paa_busname = "pcmcia";
		paa.pct = (pcmcia_chipset_tag_t)&sa11x0_pcmcia_functions;
		paa.pch = (pcmcia_chipset_handle_t)&sc->sc_socket[i].ss;
		paa.iobase = 0;
		paa.iosize = 0x4000000;

		sc->sc_socket[i].ss.pcmcia =
		    (struct device *)config_found_ia(&sc->sc_pc.sc_dev,
		    "pcmciabus", &paa, NULL);

		/* interrupt for card insertion/removal */
		gxio_intr_establish(sc, GXIO_GPIRQ11_nCD, IST_EDGE_BOTH,
		    IPL_BIO, gxpcic_card_detect, &sc->sc_socket[i]);

		/* if card was insert then set CIT */
		val = pxa2x0_gpio_get_function(GXIO_GPIRQ11_nCD);
		if (HAVE_CARD(val)) {
			mecr = bus_space_read_4(
			    sc->sc_iot, sc->sc_ioh, MEMCTL_MECR);
			bus_space_write_4(sc->sc_iot,
			    sc->sc_ioh, MEMCTL_MECR, mecr | MECR_CIT);
		}

		/* schedule kthread creation */
		kthread_create(sapcic_kthread_create, &sc->sc_socket[i].ss);
	}

}

static int
gxpcic_card_detect(void *arg)
{
	struct gxpcic_socket *socket = arg;
	struct gxpcic_softc *sc = (struct gxpcic_softc *)socket->ss.sc;
	int sock_no = socket->ss.socket;
	int mecr, last, val, s;

	s = splbio();
	last = sc->sc_cards;
	val = pxa2x0_gpio_get_function(GXIO_GPIRQ11_nCD);
	if (HAVE_CARD(val)) {
		sc->sc_cards |= 1<<sock_no;
		/* if it is the first card, turn on expansion memory control. */
		if (last == 0) {
			mecr = bus_space_read_4(
			    sc->sc_iot, sc->sc_ioh, MEMCTL_MECR);
			bus_space_write_4(sc->sc_iot,
			    sc->sc_ioh, MEMCTL_MECR, mecr | MECR_CIT);
		}
	} else {
		sc->sc_cards &= ~(1<<sock_no);
		/* if we loast all cards, turn off expansion memory control. */
		if (sc->sc_cards == 0) {
			mecr = bus_space_read_4(
			    sc->sc_iot, sc->sc_ioh, MEMCTL_MECR);
			bus_space_write_4(sc->sc_iot,
			    sc->sc_ioh, MEMCTL_MECR, mecr & ~MECR_CIT);
		}
	}
	splx(s);

	DPRINTF(("%s: card %d %s\n", sc->sc_pc.sc_dev.dv_xname, sock_no,
	    HAVE_CARD(val) ? "inserted" : "removed"));

	sapcic_intr(arg);

	return 1;
}

/* ARGSUSED */
static int
gxpcic_read(struct sapcic_socket *so, int which)
{
	int reg;

	switch (which) {
	case SAPCIC_STATUS_CARD:
		reg = pxa2x0_gpio_get_function(GXIO_GPIRQ11_nCD);
		return (HAVE_CARD(reg) ?
		    SAPCIC_CARD_VALID : SAPCIC_CARD_INVALID);

	case SAPCIC_STATUS_READY:
		reg = pxa2x0_gpio_get_function(GXIO_GPIO26_READY);
		return (reg & GPIO_SET ? 1 : 0);

	default:
		panic("%s: bogus register", __FUNCTION__);
	}
}

/* ARGSUSED */
static void
gxpcic_write(struct sapcic_socket *so, int which, int arg)
{

	switch (which) {
	case SAPCIC_CONTROL_RESET:
#if 0	/* XXXX: Our PXA2x0 no necessary ??? */
		if (arg)
			pxa2x0_gpio_set_function(GXIO_GPIO8_RESET, GPIO_SET);
		else
			pxa2x0_gpio_set_function(GXIO_GPIO8_RESET, GPIO_CLR);
#endif
		break;

	case SAPCIC_CONTROL_LINEENABLE:
		break;

	case SAPCIC_CONTROL_WAITENABLE:
		break;

	case SAPCIC_CONTROL_POWERSELECT:
		break;

	default:
		panic("%s: bogus register", __FUNCTION__);
	}
}

static void
gxpcic_set_power(struct sapcic_socket *__so, int arg)
{

	if(arg != SAPCIC_POWER_OFF && arg != SAPCIC_POWER_3V)
		panic("%s: bogus arg\n", __FUNCTION__);

	/* 3.3V only? */
}

/* ARGSUSED */
static void
gxpcic_clear_intr(int arg)
{

	/* nothing to do */
}

static void *
gxpcic_intr_establish(struct sapcic_socket *so, int level,
    int (* ih_fun)(void *), void *ih_arg)
{
	__attribute__((unused))struct gxpcic_softc *sc =
	    (struct gxpcic_softc *)so->sc;
	int gpirq;

	gpirq = GXIO_GPIO26_READY;

	DPRINTF(("%s: card %d gpio %d\n",
	    sc->sc_pc.sc_dev.dv_xname, so->socket, gpirq));

	return gxio_intr_establish(sc, gpirq, IST_EDGE_FALLING, level,
	    ih_fun, ih_arg);
}

static void
gxpcic_intr_disestablish(struct sapcic_socket *so, void *ih)
{
	__attribute__((unused))struct gxpcic_softc *sc =
	    (struct gxpcic_softc *)so->sc;

	gxio_intr_disestablish(sc, ih);
}
