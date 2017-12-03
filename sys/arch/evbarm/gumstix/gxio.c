/*	$NetBSD: gxio.c,v 1.18.18.2 2017/12/03 11:36:04 jdolecek Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: gxio.c,v 1.18.18.2 2017/12/03 11:36:04 jdolecek Exp $");

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

#include <arm/omap/omap2_gpmcreg.h>
#if defined(OMAP2)
#include <arm/omap/omap2_reg.h>
#if defined(OMAP3530)
#include <arm/omap/omap2_intr.h>
#endif
#endif
#include <arm/omap/omap_var.h>
#include <arm/omap/ti_iicreg.h>
#include <arm/omap/tifbvar.h>
#if defined(CPU_XSCALE)
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

void gxio_config(void);
void gxio_config_expansion(const char *);
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
static void dvi_config(void);
static void lcd_config(char);
static void header_40pin_config(int);

static void chestnut_config(void);
static void gallop_config(void);
static void summit_config(void);
static void tobi_config(void);
static void tobiduo_config(void);
#elif defined(DUOVERO)
static void ehci_config(void);

static void parlor_config(void);
#elif defined(PEPPER)
static void lcd_config(void);
static void pepper43_config(void);

static void pepper_config(void);
static void c_config(void);
static void dvi_config(void);
static void r_config(void);
#endif
#if defined(OVERO) || defined(DUOVERO)
struct omap_mux_conf;
static void smsh_config(struct omap_mux_conf *, int, int);
#endif
#if defined(OVERO) || defined(DUOVERO) || defined(PEPPER)
static void __udelay(unsigned int);
#endif
#if defined(PEPPER)
static int read_i2c_device(const vaddr_t, uint16_t, uint8_t, int, uint8_t *);
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

#elif defined(OMAP2)

struct omap_mux_conf {
	int offset;
	uint32_t value;
/* OMAP3/4 register values */
#define WAKEUPEVENT		(1 << 15)
#define WAKEUPENABLE		(1 << 14)
#define OFFMODEPULLTYPESELECT	(1 << 13)
#define OFFMODEPULLUDENABLE	(1 << 12)
#define OFFMODEOUTVALUE		(1 << 11)
#define OFFMODEOUTENABLE	(1 << 10)
#define OFFMODEENABLE		(1 << 9)
#define INPUTENABLE		(1 << 8)
#define PULLTYPESELECT		(1 << 4)
#define PULLUDENABLE		(1 << 3)
#define MUXMODE(n)		((n) & 0x7)

/* Sitara AM3xxx register values */
#define SLEWCTRL		(1 << 6)
#define RXACTIVE		(1 << 5)
#define PUTYPESEL		(1 << 4)
#define PUDEN			(1 << 3)
#define MMODE(n)		((n) & 0x7)
};
struct omap_gpio_conf {
	int pin;
	enum {
		conf_input = -1,
		conf_output_0,
		conf_output_1,
	} conf;
};

static void gxio_omap_mux_config(const struct omap_mux_conf []);
static int gxio_omap_mux_config_address(const char *, unsigned long,
					const struct omap_mux_conf[],
					const struct omap_mux_conf[]);
static void gxio_omap_gpio_config(const struct omap_gpio_conf[]);
void gxio_omap_gpio_write(int, int);

static const struct omap_mux_conf overo_mux_i2c3_conf[] = {
	{ 0x1c2, MUXMODE(0) | INPUTENABLE },		/* i2c3_scl */
	{ 0x1c4, MUXMODE(0) | INPUTENABLE },		/* i2c3_sda */
	{ -1 }
};
static const struct omap_mux_conf overo_mux_mmchs2_conf[] = {
	{ 0x158,					/* mmc2_clk */
	  MUXMODE(0) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
	{ 0x15a,					/* mmc2_cmd */
	  MUXMODE(0) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
	{ 0x15c,					/* mmc2_dat0 */
	  MUXMODE(0) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
	{ 0x15e,					/* mmc2_dat1 */
	  MUXMODE(0) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
	{ 0x160,					/* mmc2_dat2 */
	  MUXMODE(0) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
	{ 0x162,					/* mmc2_dat3 */
	  MUXMODE(0) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
	{ -1 }
};
#if defined(OVERO) 
static const struct omap_mux_conf overo_mux_wireless_conf[] = {
	{ 0x0b4, MUXMODE(4) },				/* gpio_54:BT_nPOWERON*/
	{ 0x0bc, MUXMODE(4) | INPUTENABLE },		/* gpio_58: WIFI_IRQ */
	{ 0x19c, MUXMODE(4) },				/* gpio_164:BT_nRESET */
	{ 0x5e0, MUXMODE(4) },				/* gpio_16: W2W_nRESET*/
	{ -1 }
};

#elif defined(DUOVERO)
static const struct omap_mux_conf duovero_mux_led_conf[] = {
	{ 0x116, MUXMODE(3) },				/* GPIO 122 */
	{ -1 }
};
static const struct omap_mux_conf duovero_mux_button_conf[] = {
	{ 0x114,					/* GPIO 121 */
	  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
	{ -1 }
};

#elif defined(PEPPER)
static const struct omap_mux_conf pepper_mux_led_conf[] = {
	{ 0x850, MMODE(7) | PUDEN },			/* GPIO 52: Blue */
	{ 0x854, MMODE(7) | PUDEN },			/* GPIO 53: Red */
	{ -1 }
};
static const struct omap_mux_conf pepper_mux_button_conf[] = {
	{ 0x858, MMODE(7) | PUTYPESEL | RXACTIVE },	/* GPIO 54 */
	{ -1 }
};
static const struct omap_mux_conf pepper_mux_mmchs3_conf[] = {
	{ 0x844, MMODE(3) | PUTYPESEL | RXACTIVE },	/* MMC2_DAT0 */
	{ 0x848, MMODE(3) | PUTYPESEL | RXACTIVE },	/* MMC2_DAT1 */
	{ 0x84c, MMODE(3) | PUTYPESEL | RXACTIVE },	/* MMC2_DAT2 */
	{ 0x878, MMODE(3) | PUTYPESEL | RXACTIVE },	/* MMC2_DAT3 */
	{ 0x888, MMODE(3) | PUTYPESEL | RXACTIVE },	/* MMC2_CMD */
	{ 0x88c, MMODE(3) | PUTYPESEL | RXACTIVE },	/* MMC2_CLK */
	{ -1 }
};
static const struct omap_mux_conf pepper_mux_audio_codec_conf[] = {
	{ 0x840, MMODE(7) | PUDEN },			/* GPIO 48: #Reset */
	{ -1 }
};
#endif

#endif

static const struct gxioconf gxioconflist[] = {
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
	{ "gallop43",		gallop_config },
	{ "summit",		summit_config },
	{ "tobi",		tobi_config },
	{ "tobi-duo",		tobiduo_config },
#elif defined(DUOVERO)
	{ "parlor",		parlor_config },
#elif defined(PEPPER)
	{ "43c",		c_config },
	{ "43r",		r_config },
	{ "dvi",		dvi_config },
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


#if defined(GUMSTIX)
/*
 * configure for GPIO pin and expansion boards.
 */
void
gxio_config(void)
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
#elif defined(OVERO) || defined(DUOVERO) || defined(PEPPER)
static void
gxio_omap_mux_config(const struct omap_mux_conf mux_conf[])
{
#if defined(OVERO)
	const vaddr_t ctrlmod_base = OVERO_L4_CORE_VBASE + 0x2000;
#elif defined(DUOVERO)
	const vaddr_t ctrlmod_base = DUOVERO_L4_CM_VBASE;
#elif defined(PEPPER)
	const vaddr_t ctrlmod_base = PEPPER_PRCM_VBASE + 0x10000;
#endif
	int i;

	for (i = 0; mux_conf[i].offset != -1; i++)
#if !defined(TI_AM335X)
		ioreg16_write(ctrlmod_base + mux_conf[i].offset,
		    mux_conf[i].value);
#else
		ioreg_write(ctrlmod_base + mux_conf[i].offset,
		    mux_conf[i].value);
#endif
}

static int
gxio_omap_mux_config_address(const char *name, unsigned long address,
			     const struct omap_mux_conf mux_conf[],
			     const struct omap_mux_conf not_mux_conf[])
{
	extern struct cfdata cfdata[];
	cfdata_t cf = &cfdata[0];

	while (cf->cf_name != NULL) {
		if (strcmp(name, cf->cf_name) == 0 &&
		    address == cf->cf_loc[OBIOCF_ADDR]) {
			gxio_omap_mux_config(mux_conf);
			return 0;
		}
		cf++;
	}

	if (not_mux_conf == NULL)
		return -1;

	gxio_omap_mux_config(not_mux_conf);
	return 0;
}

#if defined(OVERO)
#define gpio_reg_read		ioreg_read
#define gpio_reg_write		ioreg_write
#elif defined(DUOVERO) || defined(PEPPER)
#define gpio_reg_read(a)	ioreg_read((a) + GPIO_SIZE2)
#define gpio_reg_write(a, v)	ioreg_write((a) + GPIO_SIZE2, (v))
#endif

static const vaddr_t gpio_bases[] = {
#if defined(OVERO)
#define OVERO_GPIO_VBASE(n) ((n) == 1 ? BASE(WAKEUP, n) : BASE(PERIPHERAL, n))
#define GPIO(n)		GPIO ## n ## _BASE_3530
#define BASE(a, n) \
	(OVERO_L4_ ## a ## _VBASE + (GPIO(n) - OMAP3530_L4_ ## a ## _BASE))

	GPIO1_BASE_3530,
	GPIO2_BASE_3530,
	GPIO3_BASE_3530,
	GPIO4_BASE_3530,
	GPIO5_BASE_3530,
	GPIO6_BASE_3530,

#elif defined(DUOVERO)
#define DUOVERO_GPIO_VBASE(n) ((n) == 1 ? BASE(WAKEUP, n) : BASE(PERIPHERAL, n))
#define GPIO(n)		GPIO ## n ## _BASE_4430
#define BASE(a, n) \
	(DUOVERO_L4_ ## a ## _VBASE + (GPIO(n) - OMAP4430_L4_ ## a ## _BASE))

	DUOVERO_GPIO_VBASE(1),
	DUOVERO_GPIO_VBASE(2),
	DUOVERO_GPIO_VBASE(3),
	DUOVERO_GPIO_VBASE(4),
	DUOVERO_GPIO_VBASE(5),

#elif defined(PEPPER)
#define PEPPER_GPIO_VBASE(n) ((n) == 0 ? WAKEUP(n) : PERIPHERAL(n))
#define GPIO(n)		GPIO ## n ## _BASE_TI_AM335X
#define WAKEUP(n)	(PEPPER_PRCM_VBASE + (GPIO(n) - OMAP2_CM_BASE))
#define PERIPHERAL(n) \
	(PEPPER_L4_PERIPHERAL_VBASE + (GPIO(n) - TI_AM335X_L4_PERIPHERAL_BASE))

	PEPPER_GPIO_VBASE(0),
	PEPPER_GPIO_VBASE(1),
	PEPPER_GPIO_VBASE(2),
	PEPPER_GPIO_VBASE(3),
#endif
};

static void
gxio_omap_gpio_config(const struct omap_gpio_conf gpio_conf[])
{
	vaddr_t gpio_base;
	int mask, i;

	for (i = 0; gpio_conf[i].pin != -1; i++) {
		gpio_base = gpio_bases[gpio_conf[i].pin / 32];
		mask = 1 << (gpio_conf[i].pin % 32);
		switch (gpio_conf[i].conf) {
		case conf_input:
			ioreg_write(gpio_base + GPIO_OE,
			    ioreg_read(gpio_base + GPIO_OE) | mask);
			break;
		case conf_output_0:
			ioreg_write(gpio_base + GPIO_OE,
			    ioreg_read(gpio_base + GPIO_OE) | ~mask);
#if 0
			ioreg_write(gpio_base + GPIO_CLEARDATAOUT, mask);
#else
			ioreg_write(gpio_base + GPIO_DATAOUT,
			    ioreg_read(gpio_base + GPIO_DATAOUT) & ~mask);
#endif
			break;
		case conf_output_1:
			ioreg_write(gpio_base + GPIO_OE,
			    ioreg_read(gpio_base + GPIO_OE) | ~mask);
#if 0
			ioreg_write(gpio_base + GPIO_SETDATAOUT, mask);
#else
			ioreg_write(gpio_base + GPIO_DATAOUT,
			    ioreg_read(gpio_base + GPIO_DATAOUT) | mask);
#endif
			break;
		}
	}
}

void
gxio_omap_gpio_write(int pin, int val)
{
	vaddr_t gpio_base;
	int mask;

	KASSERT(pin / 32 < __arraycount(gpio_bases));

	gpio_base = gpio_bases[pin / 32];
	mask = 1 << (pin % 32);
	if (val == 0)
		ioreg_write(gpio_base + GPIO_CLEARDATAOUT, mask);
	else
		ioreg_write(gpio_base + GPIO_SETDATAOUT, mask);
}

/*
 * configure for MUX, GPIO.
 */
void
gxio_config(void)
{
	const struct omap_mux_conf *mux_conf[] = {
#if defined(OVERO)
		overo_mux_i2c3_conf,
		overo_mux_mmchs2_conf,
		overo_mux_wireless_conf,
#elif defined(DUOVERO)
		duovero_mux_led_conf,
		duovero_mux_button_conf,
#elif defined(PEPPER)
		pepper_mux_led_conf,
		pepper_mux_button_conf,
		pepper_mux_mmchs3_conf,
		pepper_mux_audio_codec_conf,
#endif
	};
	const struct omap_gpio_conf gpio_conf[] = {
#if defined(OVERO)
		{  16, conf_output_0 },		/* Wireless: #Reset */
#elif defined(PEPPER)
		{  48, conf_output_0 },		/* Audio Codec: #Reset */
#endif
		{ -1 }
	};
	int i;

	for (i = 0; i < __arraycount(mux_conf); i++)
		gxio_omap_mux_config(mux_conf[i]);
	gxio_omap_gpio_config(gpio_conf);
}
#endif

static int
gxio_find_default_expansion(void)
{
#ifdef GXIO_DEFAULT_EXPANSION
	int i;

	/* Find out the default expansion */
	for (i = 0; gxioconflist[i].name != NULL; i++)
		if (strncasecmp(gxioconflist[i].name, GXIO_DEFAULT_EXPANSION,
		    strlen(gxioconflist[i].name) + 1) == 0)
			break;
	return gxioconflist[i].name == NULL ? -1 : i;
#else
	return -1;
#endif
}

void
gxio_config_expansion(const char *expansion)
{
	int i, d;

	d = gxio_find_default_expansion();

	/* Print information about expansions */
	printf("supported expansions:\n");
	for (i = 0; gxioconflist[i].name != NULL; i++)
		printf("  %s%s\n", gxioconflist[i].name,
		    i == d ? " (DEFAULT)" : "");

	
	if (expansion == NULL) {
		printf("not specified 'expansion=' in the boot args.\n");
		i = -1;
	} else {
		for (i = 0; gxioconflist[i].name != NULL; i++)
			if (strncasecmp(gxioconflist[i].name, expansion,
			    strlen(gxioconflist[i].name) + 1) == 0)
				break;
		if (gxioconflist[i].name == NULL) {
			printf("unknown expansion specified: %s\n", expansion);
			i = -1;
		}
	}

	/* Do some magic stuff for PEPPER */
#if defined(PEPPER)
	if (i < 0) {
		struct pepper_board_id {
			unsigned int device_vendor;
#define GUMSTIX_PEPPER          0x30000200	/* 1st gen */
#define GUMSTIX_PEPPER_DVI      0x31000200	/* DVI and newer */
			unsigned char revision;
			unsigned char content;
			char fab_revision[8];
			char env_var[16];
			char env_setting[64];
		} id;
		const vaddr_t i2c_base = PEPPER_PRCM_VBASE + 0xb000;
		const uint8_t eeprom = 0x50;
		const uint8_t len = sizeof(id);
		int rv;

		rv = read_i2c_device(i2c_base, eeprom, 0x00, len, (void *)&id);
		if (rv == 0)
			if (id.device_vendor == GUMSTIX_PEPPER) {
				printf("configure auto detected expansion"
				    " (pepper)\n");
				pepper_config();
				return;
			}
	}
#endif

	/*
	 * Now proceed to configure the default expansion if one was
	 * specified (and found) or return.
	 */
	const char *toconfigure;
	if (i < 0) {
#ifdef GXIO_DEFAULT_EXPANSION
		if (d == -1) {
			printf("default expansion (%s) not found\n", 
			    GXIO_DEFAULT_EXPANSION);
			return;
		}
		expansion = GXIO_DEFAULT_EXPANSION;
		i = d;
		toconfigure = "default";
#else
		return;
#endif
	} else
		toconfigure = "specified";

	printf("configure %s expansion (%s)\n", toconfigure, expansion);
	gxioconflist[i].config();
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
	/*
	 * ETH0 connects via CS5.  It use GPIO 176 for IRQ.
	 * Also GPIO 64 is NRESET.
	 */

	smsh_config(NULL, 176, 64);
}

static void
eth1_config(void)
{
	struct omap_mux_conf eth1_mux_conf[] = {
		{ 0x0d2, MUXMODE(4) | INPUTENABLE },
		{ -1 }
	};

	/*
	 * ETH1 connects via CS4.  It use GPIO 65 for IRQ.
	 */

	smsh_config(eth1_mux_conf, 65, 64);
}

static void
dvi_config(void)
{
	static const struct omap_mux_conf overo_mux_dvi_conf[] = {
		{ 0x0d4, MUXMODE(0) },				/* dss_pclk */
		{ 0x0d6, MUXMODE(0) },				/* dss_pclk */
		{ 0x0d8, MUXMODE(0) },				/* dss_pclk */
		{ 0x0da, MUXMODE(0) },				/* dss_pclk */
		{ 0x0dc, MUXMODE(0) },				/* dss_pclk */
		{ 0x0de, MUXMODE(0) },				/* dss_pclk */
		{ 0x0e0, MUXMODE(0) },				/* dss_pclk */
		{ 0x0e2, MUXMODE(0) },				/* dss_pclk */
		{ 0x0e4, MUXMODE(0) },				/* dss_pclk */
		{ 0x0e6, MUXMODE(0) },				/* dss_pclk */
		{ 0x0e8, MUXMODE(0) },				/* dss_pclk */
		{ 0x0ea, MUXMODE(0) },				/* dss_pclk */
		{ 0x0ec, MUXMODE(0) },				/* dss_pclk */
		{ 0x0ee, MUXMODE(0) },				/* dss_pclk */
		{ 0x0f0, MUXMODE(0) },				/* dss_pclk */
		{ 0x0f2, MUXMODE(0) },				/* dss_pclk */
		{ 0x0f4, MUXMODE(0) },				/* dss_pclk */
		{ 0x0f6, MUXMODE(0) },				/* dss_pclk */
		{ 0x0f8, MUXMODE(0) },				/* dss_pclk */
		{ 0x0fa, MUXMODE(0) },				/* dss_pclk */
		{ 0x0fc, MUXMODE(0) },				/* dss_pclk */
		{ 0x0fe, MUXMODE(0) },				/* dss_pclk */
		{ 0x100, MUXMODE(0) },				/* dss_pclk */
		{ 0x102, MUXMODE(0) },				/* dss_pclk */
		{ 0x104, MUXMODE(0) },				/* dss_pclk */
		{ 0x106, MUXMODE(0) },				/* dss_pclk */
		{ 0x108, MUXMODE(0) },				/* dss_pclk */
		{ 0x10a, MUXMODE(0) },				/* dss_pclk */
		{ -1 }
	};

	gxio_omap_mux_config(overo_mux_dvi_conf);
}

static void
lcd_config(char type)
{
	static const struct omap_mux_conf overo_mux_mcspi1_conf[] = {
		{ 0x1c8, MUXMODE(0) | INPUTENABLE },		/* mcspi1_clk */
		{ 0x1ca, MUXMODE(0) | INPUTENABLE },		/* mcspi1_simo*/
		{ 0x1cc, MUXMODE(0) | INPUTENABLE },		/* mcspi1_somi*/
		{ 0x1ce, MUXMODE(0) | INPUTENABLE },		/* mcspi1_cs0 */
		{ 0x1d0, MUXMODE(0) | INPUTENABLE },		/* mcspi1_cs1 */
		{ -1 }
	};
	static const struct omap_mux_conf overo_mux_ads7846_conf[] = {
		{ 0x138,			/* gpio_114: NPENIRQ */
		  MUXMODE(4) | PULLUDENABLE | INPUTENABLE },
		{ -1 }
	};
	static const struct omap_mux_conf overo_mux_lcd_conf[] = {
		{ 0x174, MUXMODE(4) },		/* gpio_144: DISPLAY_EN */
		{ 0x176, MUXMODE(4) },		/* gpio_145: Brightness */
		{ -1 }
	};

	static const struct omap_gpio_conf overo_gpio_lcd_conf[] = {
		{ 144, conf_output_0 },		/* DISPLAY_EN */
		{ 145, conf_output_0 },		/* Brightness */
		{ -1 }
	};

	dvi_config();
	if (type != 'C') {
		gxio_omap_mux_config(overo_mux_mcspi1_conf);
		gxio_omap_mux_config(overo_mux_ads7846_conf);
	}
	gxio_omap_mux_config(overo_mux_lcd_conf);

	gxio_omap_gpio_config(overo_gpio_lcd_conf);
}

enum {
	uart1_if_exists = 0,
	force_uart1
};
static void
header_40pin_config(int uart1)
{
	static const struct omap_mux_conf overo_mux_40pin_header_conf[] = {
		/*
		 *  1: GND
		 *  2: VCC_3.3
		 *  3: GPIO171_SPI1_CLK
		 *  4: GPIO114_SPI1_NIRQ
		 *  5: GPIO172_SPI1_MOSI
		 *  6: GPIO174_SPI1_CS0
		 *  7: GPIO173_SPI1_MISO
		 *  8: GPIO175_SPI1_CS1
		 *  9: GPIO151_RXD1
		 * 10: GPIO148_TXD1
		 * 11: SYS_EN
		 * 12: VBACKUP
		 * 13: GPIO0_WAKEUP
		 * 14: POWERON
		 * 15: GND
		 * 16: VCC_1.8
		 * 17: GPIO128_GPS_PPS
		 * 18: GPIO127_TS_IRQ
		 * 19: GPIO170_HDQ_1WIRE
		 * 20: GPIO163_IR_CTS3
		 * 21: GPIO165_IR_RXD3	(console)
		 * 22: GPIO166_IR_TXD3	(console)
		 * 23: GPIO184_SCL3	(system eeprom)
		 * 24: GPIO185_SDA3	(system eeprom)
		 * 25: GND
		 * 26: VCC_1.8
		 * 27: GPIO146_PWM11
		 * 28: GPIO145_PWM10
		 * 29: GPIO147_PWM8
		 * 30: GPIO144_PWM9
		 * 31: PWM0 (TPS65950)
		 * 32: PWM1 (TPS65950)
		 * 33: ADCIN7 (TPS65950)
		 * 34: ADCIN2 (TPS65950)
		 * 35: ADCIN6 (TPS65950)
		 * 36: ADCIN5 (TPS65950)
		 * 37: AGND (TPS65950)
		 * 38: ADCIN3 (TPS65950)
		 * 39: ADCIN4 (TPS65950)
		 * 40: VIN (TPS65950)
		 */

		{ 0x152, MUXMODE(4) | INPUTENABLE },		/* gpio_127 */
		{ 0x154, MUXMODE(4) | INPUTENABLE },		/* gpio_128 */
		{ 0x174, MUXMODE(4) | INPUTENABLE },		/* gpio_144 */
		{ 0x176, MUXMODE(4) | INPUTENABLE },		/* gpio_145 */
		{ 0x178, MUXMODE(4) | INPUTENABLE },		/* gpio_146 */
		{ 0x17a, MUXMODE(4) | INPUTENABLE },		/* gpio_147 */
		{ 0x19a, MUXMODE(4) | INPUTENABLE },		/* gpio_163 */
		{ -1 }
	};
	static const struct omap_mux_conf overo_mux_uart1_conf[] = {
		{ 0x17c, MUXMODE(0) },				/* uart1_tx */
		{ 0x182, MUXMODE(0) | INPUTENABLE },		/* uart1_rx */
		{ -1 }
	};
	static const struct omap_mux_conf overo_mux_no_uart1_conf[] = {
		{ 0x17c, MUXMODE(4) | INPUTENABLE },		/* gpio_148 */
		{ 0x182, MUXMODE(4) | INPUTENABLE },		/* gpio_151 */
		{ -1 }
	};
	static const struct omap_mux_conf overo_mux_hdq_conf[] = {
#if 0
		{ 0x1c4, MUXMODE(0) | ??? | INPUTENABLE },	/* hdq_sio */
#endif
		{ -1 }
	};
	static const struct omap_mux_conf overo_mux_no_hdq_conf[] = {
		{ 0x1c4, MUXMODE(4) | INPUTENABLE },		/* gpio_170 */
		{ -1 }
	};

	gxio_omap_mux_config(overo_mux_40pin_header_conf);
	if (uart1 == force_uart1)
		gxio_omap_mux_config(overo_mux_uart1_conf);
	else
		gxio_omap_mux_config_address("com", 0x4806a000,
		    overo_mux_uart1_conf, overo_mux_no_uart1_conf);
	gxio_omap_mux_config_address("hdq", 0x480b2000,
	    overo_mux_hdq_conf, overo_mux_no_hdq_conf);
}

static void
chestnut_config(void)
{
	static const struct omap_mux_conf chestnut_mux_conf[] = {
		{ 0x5ec, MUXMODE(4) },			/* gpio_22: LED (Blue)*/
		{ 0x5ee, MUXMODE(4) | INPUTENABLE },	/* gpio_23: Button */
		{ 0x5dc, MUXMODE(4) | INPUTENABLE },	/* gpio_14: Button */
		{ -1 }
	};

	eth0_config();
	lcd_config('R');

	header_40pin_config(uart1_if_exists);
	gxio_omap_mux_config(chestnut_mux_conf);
}

static void
gallop_config(void)
{
	static const struct omap_mux_conf gallop43_mux_conf[] = {
		{ 0x5ec, MUXMODE(4) },			/* gpio_22: LED (Blue)*/
		{ 0x5ee, MUXMODE(4) | INPUTENABLE },	/* gpio_23: Button */
		{ 0x5dc, MUXMODE(4) | INPUTENABLE },	/* gpio_14: Button */
		{ -1 }
	};

	lcd_config('R');

	header_40pin_config(force_uart1);
	gxio_omap_mux_config(gallop43_mux_conf);
}

static void
summit_config(void)
{

	dvi_config();

	header_40pin_config(uart1_if_exists);
}

static void
tobi_config(void)
{

	eth0_config();
	dvi_config();

	header_40pin_config(uart1_if_exists);
}

static void
tobiduo_config(void)
{

	eth0_config();
	eth1_config();
}

#elif defined(DUOVERO)

static void
ehci_config(void)
{
	uint32_t val;

#define SCRM_ALTCLKSRC		0xa110
#define   ALTCLKSRC_ENABLE_EXT		(1 << 3)
#define   ALTCLKSRC_ENABLE_INT		(1 << 2)
#define   ALTCLKSRC_MODE_MASK		(3 << 0)
#define   ALTCLKSRC_MODE_POWERDOWN	(0 << 0)
#define   ALTCLKSRC_MODE_ACTIVE		(1 << 0)
#define   ALTCLKSRC_MODE_BYPASS		(2 << 0)
#define SCRM_AUXCLK3		0xa31c
#define   AUXCLK3_CLKDIV(n)		(((n) - 1) << 16)
#define   AUXCLK3_CLKDIV_MASK		(0xf << 16)
#define   AUXCLK3_ENABLE		(1 << 8)
#define   AUXCLK3_SRCSELECT_MASK	(3 << 1)
#define   AUXCLK3_SRCSELECT_SYSCLK	(0 << 1)
#define   AUXCLK3_SRCSELECT_CORE	(1 << 1)
#define   AUXCLK3_SRCSELECT_PERDPLL	(2 << 1)
#define   AUXCLK3_SRCSELECT_ALTCLK	(3 << 1)
#define   AUXCLK3_POLARITY_LOW		(0 << 0)
#define   AUXCLK3_POLARITY_HIGH		(1 << 0)

	/* Use the 1/2 auxiliary clock #3 of system clock. */
	val = ioreg_read(DUOVERO_L4_WAKEUP_VBASE + SCRM_AUXCLK3);
	val &= ~(AUXCLK3_CLKDIV_MASK | AUXCLK3_SRCSELECT_MASK);
	val |= (AUXCLK3_CLKDIV(2) | AUXCLK3_ENABLE | AUXCLK3_SRCSELECT_SYSCLK);
	ioreg_write(DUOVERO_L4_WAKEUP_VBASE + SCRM_AUXCLK3, val);

	val = ioreg_read(DUOVERO_L4_WAKEUP_VBASE + SCRM_ALTCLKSRC);
	val &= ~ALTCLKSRC_MODE_MASK;
	val |= ALTCLKSRC_MODE_ACTIVE;
	val |= (ALTCLKSRC_ENABLE_EXT | ALTCLKSRC_ENABLE_INT);
	ioreg_write(DUOVERO_L4_WAKEUP_VBASE + SCRM_ALTCLKSRC, val);
}

static void
parlor_config(void)
{
#if 0
	static const struct omap_mux_conf parlor_mux_40pin_header_conf[] = {
		/*
		 *  1: GND
		 *  2: GND
		 *  3: MCSPI1_CLK or GPIO 134
		 *  4: MCSPI1_CS0 or GPIO 137
		 *  5: MCSPI1_SIMO or GPIO 136
		 *  6: MCSPI1_CS1 or GPIO 138
		 *  7: MCSPI1_SOMI or GPIO 135
		 *  8: MCSPI1_CS2 or GPIO 139
		 *  9: HDQ_SIO or GPIO 127
		 * 10: MCSPI1_CS3 or GPIO 140
		 * 11: SDMMC3_CMD or GPIO ???
		 * 12: I2C2_SCL or GPIO 128
		 * 13: SDMMC3_CLK or GPIO ???
		 * 14: I2C2_SDA or GPIO 129
		 * 15: UART2_TX or SDMMC3_DAT1 or GPIO 126
		 * 16: PMIC_PWM2 (TWL6030)
		 * 17: UART2_RX or SDMMC3_DAT0 or GPIO 125
		 * 18: PMIC_PWM1 (TWL6030)
		 * 19: BSP2_CLKX or GPIO 110
		 * 20: BSP2_FSX or GPIO 113
		 * 21: BSP2_DX or GPIO 112
		 * 22: BSP2_DR or GPIO 111
		 * 23: BSP2_CLKS or GPIO 118
		 * 24: FREF1
		 * 25: MCSPI4_SOMI or GPIO 153
		 * 26: PMIC_NRESWARN
		 * 27: MCSPI4_SIMO or GPIO 152
		 * 28: SYSEN
		 * 29: MCSPI4_CLK or GPIO 151
		 * 30: PWRON
		 * 31: MCSPI4_CS0 or GPIO 154
		 * 32: REGEN1
		 * 33: ADCIN3 (TWL6030)
		 * 34: VCC_1.0
		 * 35: ADCIN4_VREF (TWL6030)
		 * 36: VDD_VAUX2
		 * 37: ADCIN4 (TWL6030)
		 * 38: VCC_3.3
		 * 39: ADCIN5 (TWL6030)
		 * 40: V_BATT_5
		 */
		{ -1 }
	};
#endif
	static const struct omap_mux_conf parlor_mux_mcspi1_conf[] = {
#if 0
		{ 0x132,			/*  3: MCSPI1_CLK */
		  MUXMODE(0) | ??? },
		{ 0x138,			/*  4: MCSPI1_CS0 */
		  MUXMODE(0) | ??? },
		{ 0x136,			/*  5: MCSPI1_SIMO */
		  MUXMODE(0) | ??? },
		{ 0x13a,			/*  6: MCSPI1_CS1 */
		  MUXMODE(0) | ??? },
		{ 0x134,			/*  7: MCSPI1_SOMI */
		  MUXMODE(0) | ??? | INPUTENABLE },
		{ 0x13c,			/*  8: MCSPI1_CS2 */
		  MUXMODE(0) | ??? },
		{ 0x13e,			/* 10: MCSPI1_CS3 */
		  MUXMODE(0) | ??? },
#endif
		{ -1 }
	};
	static const struct omap_mux_conf parlor_mux_no_mcspi1_conf[] = {
		{ 0x132,			/*  3: GPIO 134 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x138,			/*  4: GPIO 137 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x136,			/*  5: GPIO 136 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x13a,			/*  6: GPIO 138 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x134,			/*  7: GPIO 135 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x13c,			/*  8: GPIO 139 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x13e,			/* 10: GPIO 140 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ -1 }
	};
	static const struct omap_mux_conf parlor_mux_hdq_conf[] = {
#if 0
		{ 0x120,			/*  9: HDQ_SIO */
		  MUXMODE(0) | ??? | INPUTENABLE },
#endif
		{ -1 }
	};
	static const struct omap_mux_conf parlor_mux_no_hdq_conf[] = {
		{ 0x120,			/*  9: GPIO_127 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ -1 }
	};
	static const struct omap_mux_conf parlor_mux_i2c2_conf[] = {
		{ 0x126,			/* 12: I2C2_SCL */
		  MUXMODE(0) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x128,			/* 14: I2C2_SDA */
		  MUXMODE(0) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ -1 }
	};
	static const struct omap_mux_conf parlor_mux_no_i2c2_conf[] = {
		{ 0x126,			/* 12: GPIO 128 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x128,			/* 14: GPIO 129 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ -1 }
	};
	static const struct omap_mux_conf parlor_mux_sdmmc3_conf[] = {
#if 0
11	SDMMC3_CMD	  DuoVero J2 A15 <- omap pin AG10
		  MUXMODE(1) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
13	SDMMC3_CLK	  DuoVero J2 A16 <- omap pin AE9
		  MUXMODE(1) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
#endif
		{ 0x11c,			/* 17: SDMMC3_DAT0 */
		  MUXMODE(1) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x11e,			/* 15: SDMMC3_DAT1 */
		  MUXMODE(1) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ -1 }
	};
	static const struct omap_mux_conf parlor_mux_uart2_conf[] = {
		{ 0x11c,			/* 17: UART2_RX */
		  MUXMODE(0) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x11e,
		  MUXMODE(0) | PULLUDENABLE },	/* 15: UART2_TX */
		{ -1 }
	};
	static const struct omap_mux_conf parlor_mux_no_uart2_conf[] = {
		{ 0x11c,			/* 17: GPIO 125 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x11e,			/* 15: GPIO 126 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ -1 }
	};
	static const struct omap_mux_conf parlor_mux_bsp2_conf[] = {
		{ 0x0f6,			/* 19: BSP2_CLKX */
		  MUXMODE(0) | INPUTENABLE },
		{ 0x0fc,			/* 20: BSP2_FSX */
		  MUXMODE(0) | INPUTENABLE },
		{ 0x0fa,			/* 21: BSP2_DX */
		  MUXMODE(0) | PULLUDENABLE },
		{ 0x0f8,			/* 22: BSP2_DR */
		  MUXMODE(0) | PULLUDENABLE | INPUTENABLE },
		{ 0x10e,			/* 23: BSP2_CLKS */
		  MUXMODE(0) | PULLUDENABLE | INPUTENABLE },
		{ -1 }
	};
	static const struct omap_mux_conf parlor_mux_no_bsp2_conf[] = {
		{ 0x0f6,			/* 19: GPIO 110 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x0fc,			/* 20: GPIO 113 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x0fa,			/* 21: GPIO 112 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x0f8,			/* 22: GPIO 111 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x10e,			/* 23: GPIO 118 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ -1 }
	};
	static const struct omap_mux_conf parlor_mux_mcspi4_conf[] = {
#if 0
		{ 0x158,			/* 25: MCSPI4_SOMI */
		  MUXMODE(0) | ??? | INPUTENABLE },
		{ 0x156,			/* 27: MCSPI4_SIMO */
		  MUXMODE(0) | ??? },
		{ 0x154,			/* 29: MCSPI4_CLK */
		  MUXMODE(0) | ??? },
		{ 0x15a,			/* 31: MCSPI4_CS0 */
		  MUXMODE(0) | ??? },
#endif
		{ -1 }
	};
	static const struct omap_mux_conf parlor_mux_no_mcspi4_conf[] = {
		{ 0x158,			/* 25: GPIO 153 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x156,			/* 27: GPIO 152 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x154,			/* 29: GPIO 151 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ 0x15a,			/* 31: GPIO 154 */
		  MUXMODE(3) | PULLUDENABLE | PULLTYPESELECT | INPUTENABLE },
		{ -1 }
	};

	/*
	 * ETH0 connects via CS5.  It use GPIO 44 for IRQ.
	 * Also GPIO 45 is NRESET.
	 */
	smsh_config(NULL, 44, 45);

	ehci_config();

	gxio_omap_mux_config_address("mcspi", 0x48098000,
	    parlor_mux_mcspi1_conf, parlor_mux_no_mcspi1_conf);
	gxio_omap_mux_config_address("hdq", 0x480b2000,
	    parlor_mux_hdq_conf, parlor_mux_no_hdq_conf);
	gxio_omap_mux_config_address("tiiic", 0x48072000,
	    parlor_mux_i2c2_conf, parlor_mux_no_i2c2_conf);
	if (gxio_omap_mux_config_address("sdhc", 0x480ad000,
				parlor_mux_sdmmc3_conf, NULL) != 0)
		gxio_omap_mux_config_address("com", 0x4806c000,
		    parlor_mux_uart2_conf, parlor_mux_no_uart2_conf);
	gxio_omap_mux_config_address("mcbsp", 0x49024000,
	    parlor_mux_bsp2_conf, parlor_mux_no_bsp2_conf);
	gxio_omap_mux_config_address("mcspi", 0x480ba000,
	    parlor_mux_mcspi4_conf, parlor_mux_no_mcspi4_conf);
}

#elif defined(PEPPER)

static void
lcd_config(void)
{
	static const struct tifb_panel_info panel_lcd = {
		.panel_tft = 1,
		.panel_mono = false,
		.panel_bpp = 24,

		.panel_pxl_clk = 18400000,
		.panel_width = 480,
		.panel_height = 272,
		.panel_hfp = 8,
		.panel_hbp = 4,
		.panel_hsw = 41,
		.panel_vfp = 4,
		.panel_vbp = 2,
		.panel_vsw = 10,
		.panel_invert_hsync = 0,
		.panel_invert_vsync = 0,

		.panel_ac_bias = 255,
		.panel_ac_bias_intrpt = 0,
		.panel_dma_burst_sz = 16,
		.panel_fdd = 0x80,
		.panel_sync_edge = 0,
		.panel_sync_ctrl = 1,
		.panel_tft_alt_mode = 0,
		.panel_invert_pxl_clk = 0,
	};
	static const struct omap_mux_conf pepper_mux_lcd_conf[] = {
		/*
		 * LCD_DATA[0-23] configures in tifb.c
		 */

		{ 0x8e0, MMODE(0) | PUDEN },		/* LCD_VSYNC */
		{ 0x8e4, MMODE(0) | PUDEN },		/* LCD_HSYNC */
		{ 0x8e8, MMODE(0) | PUDEN },		/* LCD_PCLK */
		{ 0x8ec, MMODE(0) | PUDEN },		/* LCD_AC_BIAS_EN */

		{ 0x86c, MMODE(7) | PUTYPESEL },	/* GPIO 59: Enable */
		{ -1 }
	};

	if (gxio_omap_mux_config_address("tifb", 0x4830e000,
					pepper_mux_lcd_conf, NULL) == 0) {
		extern const struct tifb_panel_info *tifb_panel_info;
		extern bool use_tps65217_wled;

		tifb_panel_info = &panel_lcd;
		use_tps65217_wled = true;
	}
}

static void
pepper43_config(void)
{
	static const struct omap_mux_conf pepper43_mux_wilink8_conf[] = {
		/* TI WiLink 8 */
		{ 0x800, MMODE(7) | PUTYPESEL },	/* GPIO 32: Bluetooth */
		{ 0x804, MMODE(7) | PUDEN | RXACTIVE },	/* GPIO 33: irq   */
		{ 0x860, MMODE(7) | PUTYPESEL },	/* GPIO 56: WiFi  */
		{ -1 }
	};
	static const struct omap_mux_conf pepper43_mux_i2c1_conf[] = {
		{ 0x968, MMODE(3) | PUTYPESEL | RXACTIVE },	/* I2C1_SDA */
		{ 0x96c, MMODE(3) | PUTYPESEL | RXACTIVE },	/* I2C1_SCL */
		{ -1 }
	};
	static const struct omap_mux_conf pepper43_mpu9150_conf[] = {
		/* MPU9150 at I2C1 */
		{ 0x808, MMODE(7) | PUDEN | RXACTIVE },	/* GPIO 34: IRQ */
		{ 0x898, MMODE(7) | PUDEN | RXACTIVE },	/* GPIO 68 */
		{ 0x870, MMODE(7) | PUDEN | RXACTIVE },	/* GPIO 30 */
		{ -1 }
	};
	static const struct omap_mux_conf pepper43_mux_20pin_header_conf[] = {
		{ 0x85c, MMODE(7) | PUDEN | RXACTIVE },	/*  1: GPIO 55 */
		{ 0x80c, MMODE(7) | PUDEN | RXACTIVE },	/*  2: GPIO 35 */
		{ 0x810, MMODE(7) | PUDEN | RXACTIVE },	/*  3: GPIO 36 */
		{ 0x814, MMODE(7) | PUDEN | RXACTIVE },	/*  4: GPIO 37 */
		{ 0x818, MMODE(7) | PUDEN | RXACTIVE },	/*  5: GPIO 38 */
		{ 0x81c, MMODE(7) | PUDEN | RXACTIVE },	/*  6: GPIO 39 */
		{ 0x87c, MMODE(7) | PUDEN | RXACTIVE },	/*  7: GPIO 61 */
		{ 0x880, MMODE(7) | PUDEN | RXACTIVE },	/*  8: GPIO 62 */
		{ 0x884, MMODE(7) | PUDEN | RXACTIVE },	/*  9: GPIO 63 */
		{ 0x9e4, MMODE(7) | PUDEN | RXACTIVE },	/* 10: GPIO 103 */
		{ 0x9e8, MMODE(7) | PUDEN | RXACTIVE },	/* 11: GPIO 104 */
		{ 0x9b0, MMODE(7) | PUDEN | RXACTIVE },	/* 12: GPIO 19 */
#if 0	/* UART3 or GPIO */
		{ 0x964, MMODE(7) | PUDEN | RXACTIVE },	/* 13: GPIO 7 */
		{ 0x960, MMODE(7) | PUDEN | RXACTIVE },	/* 14: GPIO 6 */
#endif
#if 0	/* UART2 or GPIO */
		{ 0x910, MMODE(7) | PUDEN | RXACTIVE },	/* 15: GPIO 98 */
		{ 0x90c, MMODE(7) | PUDEN | RXACTIVE },	/* 16: GPIO 97 */
#endif
							/* 17: VCC 5v */
							/* 18: VCC 3.3v */
							/* 19: GND */
							/* 20: GND */
		{ -1 }
	};
	static const struct omap_mux_conf pepper43_mux_uart2_conf[] = {
		{ 0x90c, MMODE(6) | PUTYPESEL | RXACTIVE },	/* UART2_RXD */
		{ 0x910, MMODE(6) | PUDEN },			/* UART2_TXD */
		{ -1 }
	};
	static const struct omap_mux_conf pepper43_mux_no_uart2_conf[] = {
		{ 0x90c, MMODE(7) | PUDEN | RXACTIVE },		/* GPIO 97 */
		{ 0x910, MMODE(7) | PUDEN | RXACTIVE },		/* GPIO 98 */
		{ -1 }
	};
	static const struct omap_mux_conf pepper43_mux_uart3_conf[] = {
		{ 0x960, MMODE(1) | PUTYPESEL | RXACTIVE },	/* UART3_RXD */
		{ 0x964, MMODE(1) | PUDEN },			/* UART3_TXD */
		{ -1 }
	};
	static const struct omap_mux_conf pepper43_mux_no_uart3_conf[] = {
		{ 0x960, MMODE(7) | PUDEN | RXACTIVE },		/* GPIO 6 */
		{ 0x964, MMODE(7) | PUDEN | RXACTIVE },		/* GPIO 7 */
		{ -1 }
	};

	static const struct omap_mux_conf *pepper43_mux_conf[] = {
		pepper43_mux_wilink8_conf,
		pepper43_mux_i2c1_conf,
		pepper43_mpu9150_conf,
		pepper43_mux_20pin_header_conf,
	};

	static const struct omap_gpio_conf pepper43_gpio_wl18xx_conf[] = {
		{  32, conf_output_0 },		/* #Reset: Bluetooth */
		{  56, conf_output_0 },		/* #Reset: WiFi */
		{ -1 }
	};
	int i;

	lcd_config();

	for (i = 0; i < __arraycount(pepper43_mux_conf); i++)
		gxio_omap_mux_config(pepper43_mux_conf[i]);
	gxio_omap_gpio_config(pepper43_gpio_wl18xx_conf);

#if 0
	ioreg_write(gpio1_base + GPIO_SIZE2 + GPIO_OE,	/* GPIO 52 (Blue) */
	    ioreg_read(gpio1_base + GPIO_SIZE2 + GPIO_OE) & ~(1 << 20));
	ioreg_write(gpio1_base + GPIO_SIZE2 + GPIO_DATAOUT,
	    ioreg_read(gpio1_base + GPIO_SIZE2 + GPIO_DATAOUT) | (1 << 20));
	ioreg_write(gpio1_base + GPIO_SIZE2 + GPIO_OE,	/* GPIO 53 (Red) */
	    ioreg_read(gpio1_base + GPIO_SIZE2 + GPIO_OE) & ~(1 << 21));
	ioreg_write(gpio1_base + GPIO_SIZE2 + GPIO_DATAOUT,
	    ioreg_read(gpio1_base + GPIO_SIZE2 + GPIO_DATAOUT) | (1 << 21));
#endif

	gxio_omap_mux_config_address("com", 0x48024000,
	    pepper43_mux_uart2_conf, pepper43_mux_no_uart2_conf);
	gxio_omap_mux_config_address("com", 0x481a6000,
	    pepper43_mux_uart3_conf, pepper43_mux_no_uart3_conf);
}

static void
pepper_config(void)
{
	static const struct omap_mux_conf pepper_mux_button2_conf[] = {
		{ 0x85c, MMODE(7) | PUTYPESEL | RXACTIVE },	/* GPIO 55 */
		{ -1 }
	};
	static const struct omap_mux_conf pepper_mux_i2c1_conf[] = {
		{ 0x90c, MMODE(3) | PUTYPESEL | RXACTIVE },	/* I2C1_SDA */
		{ 0x910, MMODE(3) | PUTYPESEL | RXACTIVE },	/* I2C1_SCL */
		{ -1 }
	};
	static const struct omap_mux_conf pepper_mux_wi2wi_conf[] = {
		{ 0x9b4, MMODE(3) | PUDEN },			/* CLKOUT2 */
		/* Wi2Wi */
		{ 0x860, MMODE(7) | PUTYPESEL },	/* GPIO 56: nReset */
		{ 0x870, MMODE(7) | PUTYPESEL },	/* GPIO 30: nPower */
		{ -1 }
	};
	static const struct omap_mux_conf pepper_mux_uart1_conf[] = {
		{ 0x978, MMODE(0) | PUTYPESEL | RXACTIVE },	/* UART1_CTSn */
		{ 0x97c, MMODE(0) },				/* UART1_RTSn */
		{ 0x980, MMODE(0) | PUTYPESEL | RXACTIVE },	/* UART1_RXD */
		{ 0x984, MMODE(0) },				/* UART1_TXD */
		{ -1 }
	};
	static const struct omap_mux_conf pepper_mux_no_uart1_conf[] = {
		{ 0x978, MMODE(7) | PUDEN | RXACTIVE },		/* GPIO 12 */
		{ 0x97c, MMODE(7) | PUDEN | RXACTIVE },		/* GPIO 13 */
		{ 0x980, MMODE(7) | PUDEN | RXACTIVE },		/* GPIO 14 */
		{ 0x984, MMODE(7) | PUDEN | RXACTIVE },		/* GPIO 15 */
		{ -1 }
	};
	static const struct omap_mux_conf *pepper_mux_conf[] = {
		pepper_mux_button2_conf,
		pepper_mux_i2c1_conf,
		pepper_mux_wi2wi_conf,
	};

	int i;

	lcd_config();

	for (i = 0; i < __arraycount(pepper_mux_conf); i++)
		gxio_omap_mux_config(pepper_mux_conf[i]);
	gxio_omap_mux_config_address("com", 0x48022000,
	    pepper_mux_uart1_conf, pepper_mux_no_uart1_conf);
}

static void
c_config(void)
{
	static const struct omap_mux_conf pepper43c_mux_ft5306_conf[] = {
		/* FT5306 at I2C2 */
		{ 0x9b4, MMODE(7) | PUDEN | RXACTIVE },		/* GPIO 20 */
		{ 0x95c, MMODE(7) | PUDEN },			/* GPIO 5 */
		{ -1 }
	};
	static const struct omap_mux_conf pepper43c_mux_i2c2_conf[] = {
		{ 0x950, MMODE(2) | PUTYPESEL | RXACTIVE },	/* I2C2_SDA */
		{ 0x954, MMODE(2) | PUTYPESEL | RXACTIVE },	/* I2C2_SCL */
		{ -1 }
	};
	static const struct omap_mux_conf *pepper43c_mux_conf[] = {
		pepper43c_mux_ft5306_conf,
		pepper43c_mux_i2c2_conf,
	};

	static const struct omap_gpio_conf pepper43c_gpio_ft5306_conf[] = {
		{   5, conf_output_0 },		/* #Reset */
		{ -1 }
	};
	int i;

	pepper43_config();

	for (i = 0; i < __arraycount(pepper43c_mux_conf); i++)
		gxio_omap_mux_config(pepper43c_mux_conf[i]);
	gxio_omap_gpio_config(pepper43c_gpio_ft5306_conf);
}

static void
dvi_config(void)
{
	/* XXXX: hmm... mismatch found in Linux's dts and pubs.gumstix.org... */

	extern struct cfdata cfdata[];
	extern const struct tifb_panel_info *tifb_panel_info;

	static const struct tifb_panel_info panel_dvi = {
		.panel_tft = 1,
		.panel_mono = false,
		.panel_bpp = 16,

		.panel_pxl_clk = 63500000,
		.panel_width = 1024,
		.panel_height = 768,
		.panel_hfp = 8,
		.panel_hbp = 4,
		.panel_hsw = 41,
		.panel_vfp = 4,
		.panel_vbp = 2,
		.panel_vsw = 10,
		.panel_invert_hsync = 0,
		.panel_invert_vsync = 0,

		.panel_ac_bias = 255,
		.panel_ac_bias_intrpt = 0,
		.panel_dma_burst_sz = 16,
		.panel_fdd = 0x80,
		.panel_sync_edge = 0,
		.panel_sync_ctrl = 1,
		.panel_invert_pxl_clk = 0,
	};
	cfdata_t cf = &cfdata[0];

	/* Disable wireless module. */
	while (cf->cf_name != NULL) {
		if (strcmp(cf->cf_name, "sdhc") == 0 &&
		    strcmp(cf->cf_atname, "mainbus") == 0 &&
		    cf->cf_loc[MAINBUSCF_BASE] == 0x47810000) {
			if (cf->cf_fstate == FSTATE_NOTFOUND)
				cf->cf_fstate = FSTATE_DNOTFOUND;
			else if (cf->cf_fstate == FSTATE_STAR)
				cf->cf_fstate = FSTATE_DSTAR;
		}
		cf++;
	}

	tifb_panel_info = &panel_dvi;
}

static void
r_config(void)
{
	static const struct omap_mux_conf pepper43r_mux_ads7846_conf[] = {
		/* ADS7846 at McSPI0 */
		{ 0x9b4, MMODE(7) | PUDEN | RXACTIVE },	/* GPIO 20: IRQ */
		{ -1 }
	};
	static const struct omap_mux_conf pepper43r_mux_spi0_conf[] = {
		{ 0x950, MMODE(0) | PUTYPESEL | RXACTIVE },	/* SPI0_SCLK */
		{ 0x954, MMODE(0) | PUTYPESEL | RXACTIVE },	/* SPI0_D0 */
		{ 0x958, MMODE(0) | PUTYPESEL | RXACTIVE },	/* SPI0_D1 */
		{ 0x95c, MMODE(0) | PUTYPESEL | RXACTIVE },	/* SPI0_CS0 */
		{ -1 }
	};
	static const struct omap_mux_conf *pepper43r_mux_conf[] = {
		pepper43r_mux_ads7846_conf,
		pepper43r_mux_spi0_conf,
	};
	int i;

	pepper43_config();

	for (i = 0; i < __arraycount(pepper43r_mux_conf); i++)
		gxio_omap_mux_config(pepper43r_mux_conf[i]);
}

#endif

#if defined(OVERO) || defined(DUOVERO)
static void
smsh_config(struct omap_mux_conf *smsh_mux_conf, int intr, int nreset)
{
	struct omap_gpio_conf smsh_gpio_conf[] = {
		{ intr,		conf_input },
		{ nreset,	conf_output_0 },
		{ -1 }
	};

	/*
	 * Basically use current settings by U-Boot.
	 * However remap physical address to configured address.
	 */

	if (smsh_mux_conf != NULL)
		gxio_omap_mux_config(smsh_mux_conf);
	gxio_omap_gpio_config(smsh_gpio_conf);
	__udelay(100000);
	gxio_omap_gpio_write(nreset, 1);
}
#endif

#if defined(OVERO) || defined(DUOVERO) || defined(PEPPER)
/*
 * The delay for configuration time.
 * This function use initialized timer by U-Boot.
 */
static void
__udelay(unsigned int usec)
{
#if defined(OVERO) || defined(DUOVERO)
#define V_SCLK			(26000000 >> 1)
#define TCRR			0x28
#elif defined(PEPPER)
#define V_SCLK			24000000
#define TCRR			0x3c
#endif
#define SYS_PTV			2
#define TIMER_CLOCK		(V_SCLK / (2 << SYS_PTV))

	const vaddr_t timer_base =
#if defined(OVERO)
	    OVERO_L4_PERIPHERAL_VBASE + 0x32000;
#elif defined(DUOVERO)
	    DUOVERO_L4_PERIPHERAL_VBASE + 0x32000;
#elif defined(PEPPER)
	    PEPPER_L4_PERIPHERAL_VBASE + 0x40000;
#endif
	long timo = usec * (TIMER_CLOCK / 1000) / 1000;
	uint32_t now, last;

	last = ioreg_read(timer_base + TCRR);
	while (timo > 0) {
		now = ioreg_read(timer_base + TCRR);
		if (last > now)
			timo -= __BITS(0, 31) - last + now + 1;
		else
			timo -= now - last;
		last = now;
	}
}
#endif

#if defined(PEPPER)
static int
read_i2c_device(const vaddr_t i2c_base, uint16_t sa, uint8_t addr, int len,
		uint8_t *buf)
{
	uint16_t v;
	int aok = 0, cnt = 0;

	ioreg16_write(i2c_base + OMAP2_I2C_IRQSTATUS, 0xffff);
	v = ioreg16_read(i2c_base + OMAP2_I2C_IRQSTATUS_RAW);
	while (v & I2C_IRQSTATUS_BB) {
		ioreg16_write(i2c_base + OMAP2_I2C_IRQSTATUS, v);
		__udelay(20);
		v = ioreg16_read(i2c_base + OMAP2_I2C_IRQSTATUS_RAW);
	}
	ioreg16_write(i2c_base + OMAP2_I2C_IRQSTATUS, 0xffff);

	ioreg16_write(i2c_base + OMAP2_I2C_SA, sa);
	ioreg16_write(i2c_base + OMAP2_I2C_CNT, sizeof(addr));
	ioreg16_write(i2c_base + OMAP2_I2C_CON,
	    I2C_CON_EN | I2C_CON_MST | I2C_CON_TRX | I2C_CON_STP | I2C_CON_STT);
	while (1 /*CONSTCOND*/) {
		__udelay(20);
		v = ioreg16_read(i2c_base + OMAP2_I2C_IRQSTATUS_RAW);
		if ((v & I2C_IRQSTATUS_XRDY) && aok == 0) {
			aok = 1;
			ioreg16_write(i2c_base + OMAP2_I2C_DATA, addr);
			ioreg16_write(i2c_base + OMAP2_I2C_IRQSTATUS,
			    I2C_IRQSTATUS_XRDY);
		}
		if (v & I2C_IRQSTATUS_ARDY) {
			ioreg16_write(i2c_base + OMAP2_I2C_IRQSTATUS,
			    I2C_IRQSTATUS_ARDY);
			break;
		}
	}

	ioreg16_write(i2c_base + OMAP2_I2C_SA, sa);
	ioreg16_write(i2c_base + OMAP2_I2C_CNT, len);
	ioreg16_write(i2c_base + OMAP2_I2C_CON,
	    I2C_CON_EN | I2C_CON_MST | I2C_CON_STP | I2C_CON_STT);
	while (1 /*CONSTCOND*/) {
		v = ioreg16_read(i2c_base + OMAP2_I2C_IRQSTATUS_RAW);
		if (v & I2C_IRQSTATUS_RRDY &&
		    cnt < len) {
			buf[cnt++] = ioreg16_read(i2c_base + OMAP2_I2C_DATA);
			ioreg16_write(i2c_base + OMAP2_I2C_IRQSTATUS,
			    I2C_IRQSTATUS_RRDY);
		}
		if (v & I2C_IRQSTATUS_ARDY) {
			ioreg16_write(i2c_base + OMAP2_I2C_IRQSTATUS,
			    I2C_IRQSTATUS_ARDY);
			break;
		}
	}
	ioreg16_write(i2c_base + OMAP2_I2C_IRQSTATUS, 0xffff);
	return 0;
}
#endif
