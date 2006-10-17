/*	$NetBSD: gxio.c,v 1.2 2006/10/17 17:06:22 kiyohara Exp $ */
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gxio.c,v 1.2 2006/10/17 17:06:22 kiyohara Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <sys/systm.h>

#include <machine/bootconfig.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <evbarm/gumstix/gumstixvar.h>

#include "locators.h"


static int gxiomatch(struct device *, struct cfdata *, void *);
static void gxioattach(struct device *, struct device *, void *);
static int gxiosearch(struct device *, struct cfdata *, const int *, void *);
static int gxioprint(void *, const char *);

static void gxio_config_gpio(struct gxio_softc *);
static void cfstix_config(struct gxio_softc *);
static void etherstix_config(struct gxio_softc *);
static void netcf_config(struct gxio_softc *);
static void netduo_config(struct gxio_softc *);
static void netmmc_config(struct gxio_softc *);

CFATTACH_DECL(
    gxio, sizeof(struct gxio_softc), gxiomatch, gxioattach, NULL, NULL);

char hirose60p[MAX_BOOT_STRING]; 
char busheader[MAX_BOOT_STRING]; 

struct gxioconf {
	const char *name;
	void (*config)(struct gxio_softc *);
	uint32_t searchdrv;
};

static const struct gxioconf hirose60p_conf[] = {
	{ NULL }
};
static const struct gxioconf busheader_conf[] = {
	{ "cfstix",		cfstix_config },
	{ "etherstix",		etherstix_config },
	{ "netcf",		netcf_config },
	{ "netduo",		netduo_config },
	{ "netmmc",		netmmc_config },
	{ NULL }
};


/* ARGSUSED */
static int
gxiomatch(struct device *parent, struct cfdata *match, void *aux)
{
	bus_space_tag_t iot = &pxa2x0_bs_tag;
	bus_space_handle_t ioh;

	if (bus_space_map(iot,
	    PXA2X0_MEMCTL_BASE, PXA2X0_MEMCTL_SIZE, 0, &ioh))
		return (0);
	bus_space_unmap(iot, ioh, PXA2X0_MEMCTL_SIZE);

	/* nothing */
	return (1);
}

/* ARGSUSED */
static void
gxioattach(struct device *parent, struct device *self, void *aux)
{
	struct gxio_softc *sc = (struct gxio_softc *)self;

	printf("\n");

	sc->sc_iot = &pxa2x0_bs_tag;

	if (bus_space_map(sc->sc_iot,
	    PXA2X0_MEMCTL_BASE, PXA2X0_MEMCTL_SIZE, 0, &sc->sc_ioh))
		return;

	/* configuration for GPIO */
	gxio_config_gpio(sc);

	/*
	 *  Attach each gumstix expantion devices
	 */
	config_search_ia(gxiosearch, self, "gxio", NULL);
}

/* ARGSUSED */
static int
gxiosearch(
    struct device *parent, struct cfdata *cf, const int *ldesc, void *aux)
{
	struct gxio_softc *sc = (struct gxio_softc *)parent;
	struct gxio_attach_args gxa;

	gxa.gxa_sc = sc;
	gxa.gxa_iot = sc->sc_iot;
	gxa.gxa_addr = cf->cf_loc[GXIOCF_ADDR];
	gxa.gxa_gpirq = cf->cf_loc[GXIOCF_GPIRQ];

	if (config_match(parent, cf, &gxa))
		config_attach(parent, cf, &gxa, gxioprint);

	return (0);
}

/* ARGSUSED */
static int
gxioprint(void *aux, const char *name)
{
	struct gxio_attach_args *gxa = (struct gxio_attach_args *)aux;

	if (gxa->gxa_addr != GXIOCF_ADDR_DEFAULT)
		printf(" addr 0x%lx", gxa->gxa_addr);
	if (gxa->gxa_gpirq > 0)
		printf(" gpirq %d", gxa->gxa_gpirq);
	return (UNCONF);
}


/*
 * configure for expansion boards.
 */
void
gxio_config_gpio(struct gxio_softc *sc)
{
	int i, rv;

	for (i = 0; i < strlen(hirose60p); i++)
		hirose60p[i] = tolower(hirose60p[i]);
	for (i = 0; hirose60p_conf[i].name != NULL; i++) {
		rv = strncmp(hirose60p, hirose60p_conf[i].name,
		    strlen(hirose60p_conf[i].name) + 1);
		if (rv == 0) {
			hirose60p_conf[i].config(sc);
			break;
		}
	}

	for (i = 0; i < strlen(busheader); i++)
		busheader[i] = tolower(busheader[i]);
	for (i = 0; busheader_conf[i].name != NULL; i++) {
		rv = strncmp(busheader, busheader_conf[i].name,
		    strlen(busheader_conf[i].name) + 1);
		if (rv == 0) {
			busheader_conf[i].config(sc);
			break;
		}
	}
}

static void
cfstix_config(struct gxio_softc *sc)
{
	u_int gpio, npoe_fn;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MEMCTL_MECR,
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, MEMCTL_MECR) & ~MECR_NOS);

	pxa2x0_gpio_set_function(8, GPIO_OUT | GPIO_SET);	/* RESET */
	delay(50);
	pxa2x0_gpio_set_function(8, GPIO_OUT | GPIO_CLR);

	pxa2x0_gpio_set_function(4, GPIO_IN); 		     /* nBVD1/nSTSCHG */
	pxa2x0_gpio_set_function(27, GPIO_IN);			/* ~INPACK */
	pxa2x0_gpio_set_function(11, GPIO_IN);			/* nPCD1 */
	pxa2x0_gpio_set_function(26, GPIO_IN);			/* PRDY/IRQ */

	for (gpio = 48, npoe_fn = 0; gpio <= 53 ; gpio++)
		npoe_fn |= pxa2x0_gpio_get_function(gpio);
	npoe_fn &= GPIO_SET;

	pxa2x0_gpio_set_function(48, GPIO_ALT_FN_2_OUT | npoe_fn); /* nPOE */
	pxa2x0_gpio_set_function(49, GPIO_ALT_FN_2_OUT);	/* nPWE */
	pxa2x0_gpio_set_function(50, GPIO_ALT_FN_2_OUT);	/* nPIOR */
	pxa2x0_gpio_set_function(51, GPIO_ALT_FN_2_OUT);	/* nPIOW */
	pxa2x0_gpio_set_function(52, GPIO_ALT_FN_2_OUT);	/* nPCE1 */
	pxa2x0_gpio_set_function(54, GPIO_ALT_FN_2_OUT);	/* pSKTSEL */
	pxa2x0_gpio_set_function(55, GPIO_ALT_FN_2_OUT);	/* nPREG */
	pxa2x0_gpio_set_function(56, GPIO_ALT_FN_1_IN);		/* nPWAIT */
	pxa2x0_gpio_set_function(57, GPIO_ALT_FN_1_IN);		/* nIOIS16 */
}

/* ARGSUSED */
static void
etherstix_config(struct gxio_softc *sc)
{

	pxa2x0_gpio_set_function(49, GPIO_ALT_FN_2_OUT);	/* nPWE */
	pxa2x0_gpio_set_function(15, GPIO_ALT_FN_2_OUT);	/* nCS 1 */
	pxa2x0_gpio_set_function(80, GPIO_OUT | GPIO_SET);	/* RESET 1 */
	delay(1);
	pxa2x0_gpio_set_function(80, GPIO_OUT | GPIO_CLR);
	delay(50000);
}

static void
netcf_config(struct gxio_softc *sc)
{

	etherstix_config(sc);
	cfstix_config(sc);
}

static void
netduo_config(struct gxio_softc *sc)
{

	etherstix_config(sc);

	pxa2x0_gpio_set_function(78, GPIO_ALT_FN_2_OUT);	/* nCS 2 */
	pxa2x0_gpio_set_function(52, GPIO_OUT | GPIO_SET);	/* RESET 2 */
	delay(1);
	pxa2x0_gpio_set_function(52, GPIO_OUT | GPIO_CLR);
	delay(50000);
}

static void
netmmc_config(struct gxio_softc *sc)
{

	etherstix_config(sc);

	/* mmc not yet... */
}
