/*	$NetBSD: gxio.c,v 1.3.8.1 2007/07/11 19:58:41 mjf Exp $ */
/*
 * Copyright (C) 2005, 2006, 2007 WIDE Project and SOUM Corporation.
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
__KERNEL_RCSID(0, "$NetBSD: gxio.c,v 1.3.8.1 2007/07/11 19:58:41 mjf Exp $");

#include "opt_gxio.h"

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


struct gpioconf {
	int pin;
	u_int value;
};
struct gxioconf {
	const char *name;
	void (*config)(void);
};

static int gxiomatch(struct device *, struct cfdata *, void *);
static void gxioattach(struct device *, struct device *, void *);
static int gxiosearch(struct device *, struct cfdata *, const int *, void *);
static int gxioprint(void *, const char *);

void gxio_config_pin(void);
void gxio_config_busheader(char *);
static void gxio_config_gpio(const struct gxioconf *, char *);
static void cfstix_config(void);
static void etherstix_config(void);
static void netcf_config(void);
static void netduommc_config(void);
static void netduo_config(void);
static void netmmc_config(void);
static void wifistix_cf_config(void);

CFATTACH_DECL(
    gxio, sizeof(struct gxio_softc), gxiomatch, gxioattach, NULL, NULL);

char busheader[MAX_BOOT_STRING]; 

struct gpioconf gpioconf[] = {
	/* Bluetooth module configuration */
	{  7, GPIO_OUT | GPIO_SET },	/* power on */
	{ 12, GPIO_ALT_FN_1_OUT },	/* 32kHz out. required by SingleStone */

	/* AC97 configuration */
#if 1
	/* this configuration set by pxaacu_attach()::pxa2x0_ac97.c */
#else
	/* Don't reorder */
	{ 31, GPIO_ALT_FN_2_OUT },	/* SYNC */
	{ 30, GPIO_ALT_FN_2_OUT },	/* SDATA_OUT */
	{ 28, GPIO_ALT_FN_1_IN },	/* BITCLK */
	{ 29, GPIO_ALT_FN_1_IN },	/* SDATA_IN0 */
#endif

	/* FFUART configuration : is connected only TXD/RXD */
	{ 34, GPIO_ALT_FN_1_IN },	/* FFRXD */
	{ 39, GPIO_ALT_FN_2_OUT },	/* FFTXD */

#ifndef GXIO_BLUETOOTH_ON_HWUART
	/* BTUART configuration */
	{ 42, GPIO_ALT_FN_1_IN },	/* BTRXD */
	{ 43, GPIO_ALT_FN_2_OUT },	/* BTTXD */
	{ 44, GPIO_ALT_FN_1_IN },	/* BTCST */
	{ 45, GPIO_ALT_FN_2_OUT },	/* BTRST */
#else
	/* HWUART configuration */
	{ 42, GPIO_ALT_FN_3_IN },	/* HWRXD */
	{ 43, GPIO_ALT_FN_3_OUT },	/* HWTXD */
	{ 44, GPIO_ALT_FN_3_IN },	/* HWCST */
	{ 45, GPIO_ALT_FN_3_OUT },	/* HWRST */
#endif

	/* STUART configuration : is connected only TXD/RXD */
	{ 46, GPIO_ALT_FN_2_IN },	/* RXD */
	{ 47, GPIO_ALT_FN_1_OUT },	/* TXD */

#ifndef GXIO_BLUETOOTH_ON_HWUART
	/* HWUART configuration */
	{ 48, GPIO_ALT_FN_1_OUT },	/* HWTXD */
	{ 49, GPIO_ALT_FN_1_IN },	/* HWRXD */
	{ 50, GPIO_ALT_FN_1_IN },	/* HWCTS */
	{ 51, GPIO_ALT_FN_1_OUT },	/* HWRTS */
#endif

	{ -1 }
};

static const struct gxioconf busheader_conf[] = {
	{ "cfstix",		cfstix_config },
	{ "etherstix",		etherstix_config },
	{ "netcf",		netcf_config },
	{ "netduo-mmc",		netduommc_config },
	{ "netduo",		netduo_config },
	{ "netmmc",		netmmc_config },
	{ "wifistix-cf",	wifistix_cf_config },
	{ "wifistix",		cfstix_config },
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
	struct gxio_softc *sc = device_private(self);

	aprint_normal("\n");
	aprint_naive("\n");

	sc->sc_iot = &pxa2x0_bs_tag;

	if (bus_space_map(sc->sc_iot,
	    PXA2X0_MEMCTL_BASE, PXA2X0_MEMCTL_SIZE, 0, &sc->sc_ioh))
		return;

	/*
	 *  Attach each gumstix expansion of busheader side devices
	 */
	config_search_ia(gxiosearch, self, "gxio", NULL);
}

/* ARGSUSED */
static int
gxiosearch(
    struct device *parent, struct cfdata *cf, const int *ldesc, void *aux)
{
	struct gxio_softc *sc = device_private(parent);
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
 * configure for GPIO pin and expansion boards.
 */
void
gxio_config_pin()
{
	int i;

	/* XXX: turn off for power of bluetooth module */
	pxa2x0_gpio_set_function(7, GPIO_OUT | GPIO_CLR);
	delay(100);

	for (i = 0; gpioconf[i].pin != -1; i++)
		pxa2x0_gpio_set_function(gpioconf[i].pin, gpioconf[i].value);
}

void
gxio_config_busheader(char *expansion)
{

	gxio_config_gpio(busheader_conf, expansion);
}

static void
gxio_config_gpio(const struct gxioconf *gxioconflist, char *expansion)
{
	int i, rv;

	for (i = 0; i < strlen(expansion); i++)
		expansion[i] = tolower(expansion[i]);
	for (i = 0; gxioconflist[i].name != NULL; i++) {
		rv = strncmp(expansion, gxioconflist[i].name,
		    strlen(gxioconflist[i].name) + 1);
		if (rv == 0) {
			gxioconflist[i].config();
			break;
		}
	}
}


static void
cfstix_config()
{
	u_int gpio, npoe_fn;

#if 1
	/* this configuration set by pxapcic_attach_common()::pxa2x0_pcic.c */
#else
	pxa2x0_gpio_set_function(11, GPIO_IN);		/* PCD1 */
	pxa2x0_gpio_set_function(26, GPIO_IN);		/* PRDY1/~IRQ1 */
#endif
	pxa2x0_gpio_set_function(4, GPIO_IN); 		/* BVD1/~STSCHG1 */

	for (gpio = 48, npoe_fn = 0; gpio <= 53 ; gpio++)
		npoe_fn |= pxa2x0_gpio_get_function(gpio);
	npoe_fn &= GPIO_SET;

	pxa2x0_gpio_set_function(48, GPIO_ALT_FN_2_OUT | npoe_fn); /* nPOE */
	pxa2x0_gpio_set_function(49, GPIO_ALT_FN_2_OUT);	/* nPWE */
	pxa2x0_gpio_set_function(50, GPIO_ALT_FN_2_OUT);	/* nPIOR */
	pxa2x0_gpio_set_function(51, GPIO_ALT_FN_2_OUT);	/* nPIOW */
	pxa2x0_gpio_set_function(52, GPIO_ALT_FN_2_OUT);	/* nPCE1 */
	pxa2x0_gpio_set_function(53, GPIO_ALT_FN_2_OUT);	/* nPCE2 */
	pxa2x0_gpio_set_function(54, GPIO_ALT_FN_2_OUT);	/* pSKTSEL */
	pxa2x0_gpio_set_function(55, GPIO_ALT_FN_2_OUT);	/* nPREG */
	pxa2x0_gpio_set_function(56, GPIO_ALT_FN_1_IN);		/* nPWAIT */
	pxa2x0_gpio_set_function(57, GPIO_ALT_FN_1_IN);		/* nIOIS16 */
}

static void
etherstix_config()
{

	pxa2x0_gpio_set_function(49, GPIO_ALT_FN_2_OUT);	/* nPWE */
	pxa2x0_gpio_set_function(15, GPIO_ALT_FN_2_OUT);	/* nCS 1 */
	pxa2x0_gpio_set_function(80, GPIO_OUT | GPIO_SET);	/* RESET 1 */
	delay(1);
	pxa2x0_gpio_set_function(80, GPIO_OUT | GPIO_CLR);
	delay(50000);
}

static void
netcf_config()
{

	etherstix_config();
	cfstix_config();
}

static void
netduommc_config()
{

	netduo_config();

	/* mmc not yet... */
}

static void
netduo_config()
{

	etherstix_config();

	pxa2x0_gpio_set_function(78, GPIO_ALT_FN_2_OUT);	/* nCS 2 */
	pxa2x0_gpio_set_function(52, GPIO_OUT | GPIO_SET);	/* RESET 2 */
	delay(1);
	pxa2x0_gpio_set_function(52, GPIO_OUT | GPIO_CLR);
	delay(50000);
}

static void
netmmc_config()
{

	etherstix_config();

	/* mmc not yet... */
}

static void
wifistix_cf_config()
{

#if 1
	/* this configuration set by pxapcic_attach_common()::pxa2x0_pcic.c */
#else
	pxa2x0_gpio_set_function(36, GPIO_IN);		/* PCD2 */
	pxa2x0_gpio_set_function(27, GPIO_IN);		/* PRDY2/~IRQ2 */
#endif
	pxa2x0_gpio_set_function(18, GPIO_IN); 		/* BVD2/~STSCHG2 */

	cfstix_config();
}
