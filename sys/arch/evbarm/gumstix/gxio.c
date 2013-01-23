/*	$NetBSD: gxio.c,v 1.18.8.2 2013/01/23 00:05:46 yamt Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: gxio.c,v 1.18.8.2 2013/01/23 00:05:46 yamt Exp $");

#include "opt_cputypes.h"
#include "opt_gumstix.h"
#include "opt_gxio.h"
#if defined(OVERO)
#include "opt_omap.h"
#endif

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>

#include <sys/systm.h>

#include <machine/bootconfig.h>

#if defined(OVERO)
#include <arm/omap/omap2_gpmcreg.h>
#include <arm/omap/omap2_reg.h>
#if defined(OMAP3530)
#include <arm/omap/omap2_intr.h>
#endif
#include <arm/omap/omap_var.h>
#endif
#if defined(CPU_XSCALE_PXA270) || defined(CPU_XSCALE_PXA250)
#include <arm/xscale/pxa2x0cpu.h>
#endif
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <evbarm/gumstix/gumstixreg.h>
#include <evbarm/gumstix/gumstixvar.h>

#include "ioconf.h"
#include "locators.h"


struct gxioconf {
	const char *name;
	void (*config)(void);
};

#if defined(GUMSTIX)
static int gxiomatch(device_t, cfdata_t, void *);
static void gxioattach(device_t, device_t, void *);
static int gxiosearch(device_t, cfdata_t, const int *, void *);
static int gxioprint(void *, const char *);

CFATTACH_DECL_NEW(gxio, sizeof(struct gxio_softc),
    gxiomatch, gxioattach, NULL, NULL);
#endif

void gxio_config_pin(void);
void gxio_config_expansion(char *);
static void gxio_config_gpio(const struct gxioconf *, char *);
#if defined(GUMSTIX)
static void basix_config(void);
static void cfstix_config(void);
static void etherstix_config(void);
static void netcf_config(void);
static void netcf_vx_config(void);
static void netduommc_config(void);
static void netduo_config(void);
static void netmicrosd_config(void);
static void netwifimicrosd_config(void);
static void netmmc_config(void);
static void wifistix_config(void);
static void wifistix_cf_config(void);
#elif defined(OVERO)
static void eth0_config(void);
static void eth1_config(void);
static void chestnut_config(void);
static void tobi_config(void);
static void tobiduo_config(void);
#endif

#if defined(CPU_XSCALE_PXA250)
static struct pxa2x0_gpioconf pxa255dep_gpioconf[] = {
	/* Bluetooth module configuration */
	{  7, GPIO_OUT | GPIO_SET },	/* power on */
	{ 12, GPIO_ALT_FN_1_OUT },	/* 32kHz out. required by SingleStone */

	/* AC97 configuration */
	{ 29, GPIO_ALT_FN_1_IN },	/* SDATA_IN0 */

	/* FFUART configuration */
	{ 35, GPIO_ALT_FN_1_IN },	/* CTS */
	{ 41, GPIO_ALT_FN_2_OUT },	/* RTS */

#ifndef GXIO_BLUETOOTH_ON_HWUART
	/* BTUART configuration */
	{ 44, GPIO_ALT_FN_1_IN },	/* BTCTS */
	{ 45, GPIO_ALT_FN_2_OUT },	/* BTRTS */
#else
	/* HWUART configuration */
	{ 42, GPIO_ALT_FN_3_IN },	/* HWRXD */
	{ 43, GPIO_ALT_FN_3_OUT },	/* HWTXD */
	{ 44, GPIO_ALT_FN_3_IN },	/* HWCTS */
	{ 45, GPIO_ALT_FN_3_OUT },	/* HWRTS */
#endif

#ifndef GXIO_BLUETOOTH_ON_HWUART
	/* HWUART configuration */
	{ 48, GPIO_ALT_FN_1_OUT },	/* HWTXD */
	{ 49, GPIO_ALT_FN_1_IN },	/* HWRXD */
	{ 50, GPIO_ALT_FN_1_IN },	/* HWCTS */
	{ 51, GPIO_ALT_FN_1_OUT },	/* HWRTS */
#endif

	{ -1 }
};
#endif
#if defined(CPU_XSCALE_PXA270)
static struct pxa2x0_gpioconf verdexdep_gpioconf[] = {
	/* Bluetooth module configuration */
	{   9, GPIO_ALT_FN_3_OUT },	/* CHOUT<0> */
	{  12, GPIO_OUT | GPIO_SET },

	/* LCD configuration */
	{  17, GPIO_IN },		/* backlight on */

	/* FFUART configuration */
	{  34, GPIO_ALT_FN_1_IN },	/* FFRXD */
	{  39, GPIO_ALT_FN_2_OUT },	/* FFTXD */

	/* BTUART configuration */
	{  42, GPIO_ALT_FN_1_IN },	/* BTRXD */
	{  43, GPIO_ALT_FN_2_OUT },	/* BTTXD */
	{  44, GPIO_ALT_FN_1_IN },	/* BTCTS */
	{  45, GPIO_ALT_FN_2_OUT },	/* BTRTS */

	/* AC97 configuration */
	{  29, GPIO_ALT_FN_1_IN },	/* SDATA_IN0 */

	{ -1 }
};
#endif

static const struct gxioconf busheader_conf[] = {
#if defined(GUMSTIX)
	{ "basix",		basix_config },
	{ "cfstix",		cfstix_config },
	{ "etherstix",		etherstix_config },
	{ "netcf",		netcf_config },
	{ "netcf-vx",		netcf_vx_config },
	{ "netduo-mmc",		netduommc_config },
	{ "netduo",		netduo_config },
	{ "netmicrosd",		netmicrosd_config },
	{ "netmicrosd-vx",	netmicrosd_config },
	{ "netwifimicrosd",	netwifimicrosd_config },
	{ "netmmc",		netmmc_config },
	{ "netpro-vx",		netwifimicrosd_config },
	{ "wifistix-cf",	wifistix_cf_config },
	{ "wifistix",		wifistix_config },
#elif defined(OVERO)
	{ "chestnut43",		chestnut_config },
	{ "tobi",		tobi_config },
	{ "tobi-duo",		tobiduo_config },
#endif
	{ NULL }
};

int gxpcic_gpio_reset;
struct gxpcic_slot_irqs gxpcic_slot_irqs[2] = { { 0, -1, -1 }, { 0, -1, -1 } };


#if defined(GUMSTIX)
/* ARGSUSED */
static int
gxiomatch(device_t parent, cfdata_t match, void *aux)
{

	struct pxaip_attach_args *pxa = aux;
	bus_space_tag_t iot = &pxa2x0_bs_tag;
	bus_space_handle_t ioh;

	if (strcmp(pxa->pxa_name, match->cf_name) != 0 ||
	    pxa->pxa_addr != PXAIPCF_ADDR_DEFAULT)
		 return 0;

	if (bus_space_map(iot,
	    PXA2X0_MEMCTL_BASE, PXA2X0_MEMCTL_SIZE, 0, &ioh))
		return 0;
	bus_space_unmap(iot, ioh, PXA2X0_MEMCTL_SIZE);

	/* nothing */
	return 1;
}

/* ARGSUSED */
static void
gxioattach(device_t parent, device_t self, void *aux)
{
	struct gxio_softc *sc = device_private(self);

	aprint_normal("\n");
	aprint_naive("\n");

	sc->sc_dev = self;
	sc->sc_iot = &pxa2x0_bs_tag;

	if (bus_space_map(sc->sc_iot,
	    PXA2X0_MEMCTL_BASE, PXA2X0_MEMCTL_SIZE, 0, &sc->sc_ioh))
		return;

	/*
	 *  Attach each gumstix(busheader)/overo expansion board devices.
	 */
	config_search_ia(gxiosearch, self, "gxio", NULL);
}

/* ARGSUSED */
static int
gxiosearch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct gxio_softc *sc = device_private(parent);
	struct gxio_attach_args gxa;

	gxa.gxa_sc = sc;
	gxa.gxa_iot = sc->sc_iot;
	gxa.gxa_addr = cf->cf_loc[GXIOCF_ADDR];
	gxa.gxa_gpirq = cf->cf_loc[GXIOCF_GPIRQ];

	if (config_match(parent, cf, &gxa))
		config_attach(parent, cf, &gxa, gxioprint);

	return 0;
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
	return UNCONF;
}
#endif


/*
 * configure for GPIO pin and expansion boards.
 */
void
gxio_config_pin(void)
{
#if defined(CPU_XSCALE_PXA250)
	struct pxa2x0_gpioconf *gumstix_gpioconf[] = {
		pxa25x_com_ffuart_gpioconf,
		pxa25x_com_stuart_gpioconf,
#ifndef GXIO_BLUETOOTH_ON_HWUART
		pxa25x_com_btuart_gpioconf,
#endif
		pxa25x_com_hwuart_gpioconf,
		pxa25x_i2c_gpioconf,
		pxa25x_pxaacu_gpioconf,
		pxa255dep_gpioconf,
		NULL
	};
#endif
#if defined(CPU_XSCALE_PXA270)
	struct pxa2x0_gpioconf *verdex_gpioconf[] = {
		pxa27x_com_ffuart_gpioconf,
		pxa27x_com_stuart_gpioconf,
		pxa27x_com_btuart_gpioconf,
		pxa27x_i2c_gpioconf,
		pxa27x_pxaacu_gpioconf,
		pxa27x_pxamci_gpioconf,
		pxa27x_ohci_gpioconf,
		verdexdep_gpioconf,
		NULL
	};
#endif

	/* XXX: turn off for power of bluetooth module */
#if defined(CPU_XSCALE_PXA250)
	pxa2x0_gpio_set_function(7, GPIO_OUT | GPIO_CLR);
#elif defined(CPU_XSCALE_PXA270)
	pxa2x0_gpio_set_function(12, GPIO_OUT | GPIO_CLR);
#endif
	delay(100);

#if defined(CPU_XSCALE_PXA270) && defined(CPU_XSCALE_PXA250)
	pxa2x0_gpio_config(
	    (CPU_IS_PXA250) ? gumstix_gpioconf : verdex_gpioconf);
#elif defined(CPU_XSCALE_PXA270) || defined(CPU_XSCALE_PXA250)
#if defined(CPU_XSCALE_PXA270)
	pxa2x0_gpio_config(verdex_gpioconf);
#else
	pxa2x0_gpio_config(gumstix_gpioconf);
#endif
#endif
}

void
gxio_config_expansion(char *expansion)
{

	if (expansion == NULL) {
		printf("not specified 'expansion=' in the boot args.\n");
#ifdef GXIO_DEFAULT_EXPANSION
		printf("configure default expansion (%s)\n",
		    GXIO_DEFAULT_EXPANSION);
		expansion = __UNCONST(GXIO_DEFAULT_EXPANSION);
#else
		return;
#endif
	}
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


#if defined(GUMSTIX)

static void
basix_config(void)
{

	pxa2x0_gpio_set_function(8, GPIO_ALT_FN_1_OUT);		/* MMCCS0 */
	pxa2x0_gpio_set_function(53, GPIO_ALT_FN_1_OUT);	/* MMCCLK */
#if 0
	/* this configuration set by gxmci.c::pxamci_attach() */
	pxa2x0_gpio_set_function(11, GPIO_IN);			/* nSD_DETECT */
	pxa2x0_gpio_set_function(22, GPIO_IN);			/* nSD_WP */
#endif
}

static void
cfstix_config(void)
{
	u_int gpio, npoe_fn;
#if defined(CPU_XSCALE_PXA270) && defined(CPU_XSCALE_PXA250)
	int bvd = (CPU_IS_PXA250) ? 4 : 111;
#else
#if defined(CPU_XSCALE_PXA270)
	const int bvd = 111;
#else
	const int bvd = 4;
#endif
#endif

	if (CPU_IS_PXA250) {
		gxpcic_slot_irqs[0].valid = 1;
		gxpcic_slot_irqs[0].cd = 11;
		gxpcic_slot_irqs[0].prdy = 26;
		gxpcic_gpio_reset = 8;
	} else {
		gxpcic_slot_irqs[0].valid = 1;
		gxpcic_slot_irqs[0].cd = 104;
		gxpcic_slot_irqs[0].prdy = 96;
		gxpcic_gpio_reset = 97;
	}

#if 1
	/* PCD/PRDY set by pxa2x0_pcic.c::pxapcic_attach_common() */
#else
	pxa2x0_gpio_set_function(11, GPIO_IN);		/* PCD1 */
	pxa2x0_gpio_set_function(26, GPIO_IN);		/* PRDY1/~IRQ1 */
#endif
	pxa2x0_gpio_set_function(bvd, GPIO_IN); 	/* BVD1/~STSCHG1 */

	for (gpio = 48, npoe_fn = 0; gpio <= 53 ; gpio++)
		npoe_fn |= pxa2x0_gpio_get_function(gpio);
	npoe_fn &= GPIO_SET;

	pxa2x0_gpio_set_function(48, GPIO_ALT_FN_2_OUT | npoe_fn); /* nPOE */
	pxa2x0_gpio_set_function(49, GPIO_ALT_FN_2_OUT);	/* nPWE */
	pxa2x0_gpio_set_function(50, GPIO_ALT_FN_2_OUT);	/* nPIOR */
	pxa2x0_gpio_set_function(51, GPIO_ALT_FN_2_OUT);	/* nPIOW */
	if (CPU_IS_PXA250) {
		pxa2x0_gpio_set_function(52, GPIO_ALT_FN_2_OUT); /* nPCE1 */
		pxa2x0_gpio_set_function(53, GPIO_ALT_FN_2_OUT); /* nPCE2 */
		pxa2x0_gpio_set_function(54, GPIO_ALT_FN_2_OUT); /* pSKTSEL */
	} else {
		pxa2x0_gpio_set_function(102, GPIO_ALT_FN_1_OUT); /* nPCE1 */
		pxa2x0_gpio_set_function(105, GPIO_ALT_FN_1_OUT); /* nPCE2 */
		pxa2x0_gpio_set_function(79, GPIO_ALT_FN_1_OUT);  /* pSKTSEL */
	}
	pxa2x0_gpio_set_function(55, GPIO_ALT_FN_2_OUT);	/* nPREG */
	pxa2x0_gpio_set_function(56, GPIO_ALT_FN_1_IN);		/* nPWAIT */
	pxa2x0_gpio_set_function(57, GPIO_ALT_FN_1_IN);		/* nIOIS16 */
}

static void
etherstix_config(void)
{
	extern struct cfdata cfdata[];
#if defined(CPU_XSCALE_PXA270) && defined(CPU_XSCALE_PXA250)
	int rst = (CPU_IS_PXA250) ? 80 : 32;
	int irq = (CPU_IS_PXA250) ? 36 : 99;
#else
#if defined(CPU_XSCALE_PXA270)
	const int rst = 32, irq = 99;
#else
	const int rst = 80, irq = 36;
#endif
#endif
	int i;

	pxa2x0_gpio_set_function(49, GPIO_ALT_FN_2_OUT);	/* nPWE */
	pxa2x0_gpio_set_function(15, GPIO_ALT_FN_2_OUT);	/* nCS 1 */
	pxa2x0_gpio_set_function(rst, GPIO_OUT | GPIO_SET);	/* RESET 1 */
	delay(1);
	pxa2x0_gpio_set_function(rst, GPIO_OUT | GPIO_CLR);
	delay(50000);

	for (i = 0; cfdata[i].cf_name != NULL; i++)
		if (strcmp(cfdata[i].cf_name, "sm") == 0 &&
		    strcmp(cfdata[i].cf_atname, "sm_gxio") == 0 &&
		    cfdata[i].cf_loc[GXIOCF_ADDR] == 0x04000300 &&
		    cfdata[i].cf_loc[GXIOCF_GPIRQ] == GXIOCF_GPIRQ_DEFAULT)
			cfdata[i].cf_loc[GXIOCF_GPIRQ] = irq;
}

static void
netcf_config(void)
{

	etherstix_config();
	cfstix_config();
}

static void
netcf_vx_config(void)
{

	/*
	 * XXXX: More power is necessary for NIC and USB???
	 * (no document.  from Linux)
	 */

	pxa2x0_gpio_set_function(27, GPIO_IN);
	pxa2x0_gpio_set_function(107, GPIO_OUT | GPIO_CLR);
	pxa2x0_gpio_set_function(118, GPIO_ALT_FN_1_IN | GPIO_CLR);

	etherstix_config();
	cfstix_config();
	if (CPU_IS_PXA270) {
		/* Overwrite */
		gxpcic_slot_irqs[0].cd = 104;
		gxpcic_slot_irqs[0].prdy = 109;
		gxpcic_gpio_reset = 110;
	};
}

static void
netduommc_config(void)
{

	netduo_config();
	basix_config();
}

static void
netduo_config(void)
{

	etherstix_config();

	pxa2x0_gpio_set_function(78, GPIO_ALT_FN_2_OUT);	/* nCS 2 */
	pxa2x0_gpio_set_function(52, GPIO_OUT | GPIO_SET);	/* RESET 2 */
	delay(1);
	pxa2x0_gpio_set_function(52, GPIO_OUT | GPIO_CLR);
	delay(50000);
}

static void
netmicrosd_config(void)
{

	/* MicroSD(mci) always configure on PXA270 */

	pxa2x0_gpio_set_function(49, GPIO_ALT_FN_2_OUT);	/* nPWE */
	pxa2x0_gpio_set_function(15, GPIO_ALT_FN_2_OUT);	/* nCS 1 */
	pxa2x0_gpio_set_function(107, GPIO_OUT | GPIO_CLR);	/* RESET 1 */
	delay(hz / 2);
	pxa2x0_gpio_set_function(107, GPIO_OUT | GPIO_SET);
	delay(50000);
}

static void
netwifimicrosd_config(void)
{

	netmicrosd_config();

	cfstix_config();
	/* However use pxamci. */
	pxa2x0_gpio_set_function(111, GPIO_CLR | GPIO_ALT_FN_1_IN);
	/* Power to Marvell 88W8385 */
	pxa2x0_gpio_set_function(80, GPIO_OUT | GPIO_SET);
}

static void
netmmc_config(void)
{

	etherstix_config();
	basix_config();
}

static void
wifistix_config(void)
{

	cfstix_config();

	/* Power to Marvell 88W8385 */
	pxa2x0_gpio_set_function(80, GPIO_OUT | GPIO_SET);
}

static void
wifistix_cf_config(void)
{

	gxpcic_slot_irqs[1].valid = 1;
	gxpcic_slot_irqs[1].cd = 36;
	gxpcic_slot_irqs[1].prdy = 27;

#if 1
	/* this configuration set by pxa2x0_pcic.c::pxapcic_attach_common() */
#else
	pxa2x0_gpio_set_function(36, GPIO_IN);		/* PCD2 */
	pxa2x0_gpio_set_function(27, GPIO_IN);		/* PRDY2/~IRQ2 */
#endif
	pxa2x0_gpio_set_function(18, GPIO_IN); 		/* BVD2/~STSCHG2 */

	cfstix_config();

	/* Power to Marvell 88W8385 */
	pxa2x0_gpio_set_function(80, GPIO_OUT | GPIO_SET);
}

#elif defined(OVERO)

static void
eth0_config(void)
{
	extern struct cfdata cfdata[];
	cfdata_t cf = &cfdata[0];

	/*
	 * ETH0 connects via CS5.  It use GPIO 176 for IRQ.
	 * Also GPIO 64 is NRESET.
	 *
	 * Basically use current settings by U-Boot.
	 * However remap physical address to configured address.
	 */

	while (cf->cf_name != NULL) {
		if (strcmp(cf->cf_name, "smsh") == 0 &&
		    cf->cf_loc[GPMCCF_INTR] == PIC_MAXSOURCES + 176)
			break;
		cf++;
	}
	if (cf->cf_name == NULL ||
	    cf->cf_loc[GPMCCF_ADDR] == GPMCCF_ADDR_DEFAULT)
		return;

	ioreg_write(OVERO_GPMC_VBASE + GPMC_CONFIG7_5,
	    GPMC_CONFIG7_CSVALID |
	    GPMC_CONFIG7(GPMC_CONFIG7_MASK_16M, cf->cf_loc[GPMCCF_ADDR]));

	/*
	 * Maybe need NRESET and delay(9).
	 * However delay(9) needs to attach mputmr.
	 */
}

static void
eth1_config(void)
{
	extern struct cfdata cfdata[];
	cfdata_t cf = &cfdata[0];

	/*
	 * ETH1 connects via CS4.  It use GPIO 65 for IRQ.
	 *
	 * Basically use current settings by U-Boot.
	 * However remap physical address to configured address.
	 */

	while (cf->cf_name != NULL) {
		if (strcmp(cf->cf_name, "smsh") == 0 &&
		    cf->cf_loc[GPMCCF_INTR] == PIC_MAXSOURCES + 65)
			break;
		cf++;
	}
	if (cf->cf_name == NULL ||
	    cf->cf_loc[GPMCCF_ADDR] == GPMCCF_ADDR_DEFAULT)
		return;

	ioreg_write(OVERO_GPMC_VBASE + GPMC_CONFIG7_4,
	    GPMC_CONFIG7_CSVALID |
	    GPMC_CONFIG7(GPMC_CONFIG7_MASK_16M, cf->cf_loc[GPMCCF_ADDR]));

	/* ETH1 is sure to be reset with ETH0. */
}

static void
chestnut_config(void)
{

	eth0_config();
}

static void
tobi_config(void)
{

	eth0_config();
}

static void
tobiduo_config(void)
{

	eth0_config();
	eth1_config();
}
#endif
