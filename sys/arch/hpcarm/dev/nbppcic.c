/*	$NetBSD: nbppcic.c,v 1.1 2011/08/06 03:53:40 kiyohara Exp $ */
/*
 * Copyright (c) 2011 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <arm/xscale/pxa2x0_pcic.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>


#define HAVE_CARD(r)	(!((r) & GPIO_SET))


static int nbppcic_match(device_t, cfdata_t, void *);
static void nbppcic_attach(device_t, device_t, void *);

static u_int nbppcic_read(struct pxapcic_socket *, int);
static void nbppcic_write(struct pxapcic_socket *, int, u_int);
static void nbppcic_set_power(struct pxapcic_socket *, int);
static void nbppcic_clear_intr(struct pxapcic_socket *);
static void *nbppcic_intr_establish(struct pxapcic_socket *, int,
				    int (*)(void *), void *);
static void nbppcic_intr_disestablish(struct pxapcic_socket *, void *);

static void nbppcic_socket_setup(struct pxapcic_socket *);

CFATTACH_DECL_NEW(nbppcic, sizeof(struct pxapcic_softc),
    nbppcic_match, nbppcic_attach, NULL, NULL);

static struct pxapcic_tag nbppcic_functions = {
	nbppcic_read,
	nbppcic_write,
	nbppcic_set_power,
	nbppcic_clear_intr,
	nbppcic_intr_establish,
	nbppcic_intr_disestablish,
};


static int
nbppcic_match(device_t parent, cfdata_t match, void *aux)
{
	struct pxaip_attach_args *pxa = aux;

	if (strcmp(pxa->pxa_name, match->cf_name) != 0 ||
	    !platid_match(&platid, &platid_mask_MACH_PSIONTEKLOGIX_NETBOOK_PRO))
		return 0;

	return	1;
}

static void
nbppcic_attach(device_t parent, device_t self, void *aux)
{
	struct pxapcic_softc *sc = device_private(self);
	struct pxaip_attach_args *pxa = aux;

	sc->sc_dev = self;
	sc->sc_iot = pxa->pxa_iot;

	sc->sc_nslots = 2;
	sc->sc_irqpin[0] = 71;		/* GPIO 71: PRDY1/~IRQ1 */
	sc->sc_irqcfpin[0] = 73;	/* GPIO 73: PCD1 */
	sc->sc_irqpin[1] = 77;		/* GPIO 77: PRDY2/~IRQ2 */
	sc->sc_irqcfpin[1] = 75;	/* GPIO 75: PCD2 */

	pxapcic_attach_common(sc, &nbppcic_socket_setup);
}

static u_int
nbppcic_read(struct pxapcic_socket *so, int which)
{
	struct pxapcic_softc *sc = so->sc;
	int reg;

	switch (which) {
	case PXAPCIC_CARD_STATUS:
		reg = pxa2x0_gpio_get_function(sc->sc_irqcfpin[so->socket]);
		return (HAVE_CARD(reg) ?
		    PXAPCIC_CARD_VALID : PXAPCIC_CARD_INVALID);

	case PXAPCIC_CARD_READY:
		reg = pxa2x0_gpio_get_function(sc->sc_irqpin[so->socket]);
		return (reg & GPIO_SET) ? 1 : 0;

	default:
		panic("%s: bogus register", __func__);
	}
	/* NOTREACHED */
}

static void
nbppcic_write(struct pxapcic_socket *so, int which, u_int arg)
{

	switch (which) {
	case PXAPCIC_CARD_POWER:
		/* To Be Done: ;-) */
		break;

	case PXAPCIC_CARD_RESET:
	{
		int reset_pin[2] = { 70, 74 };	/* GPIO 70, GPIO 74 */

		pxa2x0_gpio_set_function(reset_pin[so->socket],
		    GPIO_OUT | (arg ? GPIO_CLR : GPIO_SET));
		break;
	}

	default:
		panic("%s: bogus register", __func__);
	}
	/* NOTREACHED */
}

/* ARGSUSED */
static void
nbppcic_set_power(struct pxapcic_socket *so, int arg)
{

	if(arg != PXAPCIC_POWER_OFF && arg != PXAPCIC_POWER_3V)
		panic("%s: bogus arg\n", __func__);
}

/* ARGSUSED */
static void
nbppcic_clear_intr(struct pxapcic_socket *so)
{

	/* nothing to do */
}

static void *
nbppcic_intr_establish(struct pxapcic_socket *so, int level,
    int (* ih_fun)(void *), void *ih_arg)
{

	return pxa2x0_gpio_intr_establish(so->irqpin, IST_EDGE_FALLING,
	    level, ih_fun, ih_arg);
}

/* ARGSUSED */
static void
nbppcic_intr_disestablish(struct pxapcic_socket *so, void *ih)
{

	pxa2x0_gpio_intr_disestablish(ih);
}


static void
nbppcic_socket_setup(struct pxapcic_socket *so)
{

	so->power_capability = PXAPCIC_POWER_3V;
	so->pcictag_cookie = NULL;
	so->pcictag = &nbppcic_functions;
}
