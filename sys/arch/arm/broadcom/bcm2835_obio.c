/*	$NetBSD: bcm2835_obio.c,v 1.22.2.4 2017/08/28 17:51:30 skrll Exp $	*/

/*-
 * Copyright (c) 2012, 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_obio.c,v 1.22.2.4 2017/08/28 17:51:30 skrll Exp $");

#include "locators.h"
#include "obio.h"

#include "opt_bcm283x.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835var.h>
#include <arm/broadcom/bcm_amba.h>

#include <arm/cortex/mpcore_var.h>
#include <arm/cortex/gtmr_var.h>

struct obio_softc {
	device_t		sc_dev;
	bus_dma_tag_t		sc_dmat;
	struct arm32_dma_range	sc_dmarange[1];
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_base;
	bus_size_t		sc_size;
};

static bool obio_attached;

/* prototypes */
static int	obio_match(device_t, cfdata_t, void *);
static void	obio_attach(device_t, device_t, void *);
static int	obio_print(void *, const char *);

/* attach structures */
CFATTACH_DECL_NEW(obio, sizeof(struct obio_softc),
	obio_match, obio_attach, NULL, NULL);

/*
 * List of port-specific devices to attach to the AMBA AXI bus.
 */
static const struct ambadev_locators bcm2835_ambadev_locs[] = {
	{
		/* Interrupt controller */
		.ad_name = "icu",
		.ad_addr = BCM2835_ARMICU_BASE,
		.ad_size = BCM2835_ARMICU_SIZE,
		.ad_intr = -1,
	},
#if defined(BCM2836)
	{
		/* GTMR */
		.ad_name = "armgtmr",
		.ad_intr = BCM2836_INT_CNTVIRQ_CPUN(0),
	},
#endif
	{
		/* Mailbox */
		.ad_name = "bcmmbox",
		.ad_addr = BCM2835_ARMMBOX_BASE,
		.ad_size = BCM2835_ARMMBOX_SIZE,
		.ad_intr = BCM2835_INT_ARMMAILBOX
	},
#if !defined(BCM2836)
	{
		/* System Timer */
		.ad_name = "bcmtmr",
		.ad_addr = BCM2835_STIMER_BASE,
		.ad_size = BCM2835_STIMER_SIZE,
		.ad_intr = BCM2835_INT_TIMER3,
	},
#endif
	{
		/* VCHIQ */
		.ad_name = "bcmvchiq",
		.ad_addr = BCM2835_VCHIQ_BASE,
		.ad_size = BCM2835_VCHIQ_SIZE,
		.ad_intr = BCM2835_INT_ARMDOORBELL0,
	},
	{
		/* Power Management, Reset controller and Watchdog registers */
		.ad_name = "bcmpm",
		.ad_addr = BCM2835_PM_BASE,
		.ad_size = BCM2835_PM_SIZE,
		.ad_intr = -1,
	},
	{
		/* DMA Controller */
		.ad_name = "bcmdmac",
		.ad_addr = BCM2835_DMA0_BASE,
		.ad_size = BCM2835_DMA0_SIZE,
		.ad_intr = -1,
	},
	{
		/* Random number generator */
		.ad_name = "bcmrng",
		.ad_addr = BCM2835_RNG_BASE,
		.ad_size = BCM2835_RNG_SIZE,
		.ad_intr = -1,
	},
	{
		/* Uart 0 */
		.ad_name = "plcom",
		.ad_addr = BCM2835_UART0_BASE,
		.ad_size = BCM2835_UART0_SIZE,
		.ad_intr = BCM2835_INT_UART0,
	},
	{
		/* AUX UART */
		.ad_name = "com",
		.ad_addr = BCM2835_AUX_UART_BASE,
		.ad_size = BCM2835_AUX_UART_SIZE,
		.ad_intr = BCM2835_INT_AUX,
	},
	{
		/* Framebuffer */
		.ad_name = "fb",
		.ad_addr = 0,
		.ad_size = 0,
		.ad_intr = -1,
	},
	{
		/* SD host interface */
		.ad_name = "sdhost",
		.ad_addr = BCM2835_SDHOST_BASE,
		.ad_size = BCM2835_SDHOST_SIZE,
		.ad_intr = BCM2835_INT_SDHOST,
	},
	{
		/* eMMC interface */
		.ad_name = "emmc",
		.ad_addr = BCM2835_EMMC_BASE,
		.ad_size = BCM2835_EMMC_SIZE,
		.ad_intr = BCM2835_INT_EMMC,
	},
	{
		/* DesignWare_OTG USB controller */
		.ad_name = "dwctwo",
		.ad_addr = BCM2835_USB_BASE,
		.ad_size = BCM2835_USB_SIZE,
		.ad_intr = BCM2835_INT_USB,
	},
	{
		.ad_name = "bcmspi",
		.ad_addr = BCM2835_SPI0_BASE,
		.ad_size = BCM2835_SPI0_SIZE,
		.ad_intr = BCM2835_INT_SPI0,
	},
	{
		.ad_name = "bcmbsc",
		.ad_addr = BCM2835_BSC0_BASE,
		.ad_size = BCM2835_BSC_SIZE,
		.ad_intr = BCM2835_INT_BSC,
	},
	{
		.ad_name = "bcmbsc",
		.ad_addr = BCM2835_BSC1_BASE,
		.ad_size = BCM2835_BSC_SIZE,
		.ad_intr = BCM2835_INT_BSC,
	},
	{
		/* gpio */
		.ad_name = "bcmgpio",
		.ad_addr = BCM2835_GPIO_BASE,
		.ad_size = BCM2835_GPIO_SIZE,
		.ad_intr = -1,
	},
	{
		/* gpio */
		.ad_name = "bcmgpio",
		.ad_addr = BCM2835_GPIO_BASE,
		.ad_size = BCM2835_GPIO_SIZE,
		.ad_intr = -1,
	},
	{
		/* Clock Manager */
		.ad_name = "bcmcm",
		.ad_addr = BCM2835_CM_BASE,
		.ad_size = BCM2835_CM_SIZE,
		.ad_intr = -1,
	},
	{
		/* PWM Controller */
		.ad_name = "bcmpwm",
		.ad_addr = BCM2835_PWM_BASE,
		.ad_size = BCM2835_PWM_SIZE,
		.ad_intr = -1,
	},
	{
		/* Terminator */
		.ad_name = NULL,
	}
};


static int
obio_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

bus_space_tag_t al_iot = &bcm2835_bs_tag;
bus_space_handle_t al_ioh;

static void
obio_attach(device_t parent, device_t self, void *aux)
{
	struct obio_softc *sc = device_private(self);
	const struct ambadev_locators *ad = bcm2835_ambadev_locs;
	struct amba_attach_args aaa;
	int locs[OBIOCF_NLOCS];

	if (obio_attached)
		return;

	obio_attached = true;

	sc->sc_dev = self;
	sc->sc_dmat = &bcm2835_bus_dma_tag;

	sc->sc_dmarange[0].dr_sysbase = 0;
#if defined(BCM2836)
	sc->sc_dmarange[0].dr_busbase = BCM2835_BUSADDR_CACHE_DIRECT;
#else
	sc->sc_dmarange[0].dr_busbase = BCM2835_BUSADDR_CACHE_COHERENT;
#endif
	sc->sc_dmarange[0].dr_len = physmem * PAGE_SIZE;
	bcm2835_bus_dma_tag._ranges = sc->sc_dmarange;
	bcm2835_bus_dma_tag._nranges = __arraycount(sc->sc_dmarange);

	aprint_normal("\n");

	/* Set up the attach args. */
	aaa.aaa_iot = &bcm2835_bs_tag;
	aaa.aaa_dmat = sc->sc_dmat;

#if defined(BCM2836)
	if (bus_space_map(al_iot, BCM2836_ARM_LOCAL_BASE, BCM2836_ARM_LOCAL_SIZE,
	    0, &al_ioh)) {
		aprint_error(": unable to map local space\n");
		return;
	}

	bus_space_write_4(al_iot, al_ioh, BCM2836_LOCAL_CONTROL, 0);
	bus_space_write_4(al_iot, al_ioh, BCM2836_LOCAL_PRESCALER, 0x80000000);
#endif

	for (; ad->ad_name != NULL; ad++) {
		aprint_debug("dev=%s[%u], addr=%lx:+%lx\n",
		    ad->ad_name, ad->ad_instance, ad->ad_addr, ad->ad_size);
		if (strcmp(ad->ad_name, "armgtmr") == 0) {
			struct mpcore_attach_args mpcaa = {
				.mpcaa_name = "armgtmr",
				.mpcaa_irq = ad->ad_intr,
			};

			config_found(self, &mpcaa, NULL);
			continue;
		}

		aaa.aaa_name = ad->ad_name;
		aaa.aaa_addr = ad->ad_addr;
		aaa.aaa_size = ad->ad_size;
		aaa.aaa_intr = ad->ad_intr;
		/* ad->ad_instance; */

		locs[OBIOCF_ADDR] = ad->ad_addr;
		locs[OBIOCF_SIZE] = ad->ad_size;
		locs[OBIOCF_INTR] = ad->ad_intr;

		config_found_sm_loc(self, "obio", locs, &aaa, obio_print,
		    config_stdsubmatch);
	}

	return;
}


static int
obio_print(void *aux, const char *name)
{
	struct amba_attach_args *aaa = aux;

	if (name)
		aprint_normal("%s at %s", aaa->aaa_name, name);
	else {
#if 0
		if (aaa->aaa_unit != OBIOCF_UNIT_DEFAULT)
			aprint_normal(" unit %d", aaa->aaa_unit);
		if (aaa->aaa_addr != OBIOCF_ADDR_DEFAULT) {
			aprint_normal(" addr 0x%lx", aaa->aaa_addr);
			if (aaa->aaa_size > 0)
				aprint_normal("-0x%lx",
				    aaa->aaa_addr + aaa->aaa_size - 1);
		}
#endif
		if (aaa->aaa_intr != OBIOCF_INTR_DEFAULT)
			aprint_normal(" intr %d", aaa->aaa_intr);
	}

	return UNCONF;
}

#ifdef MULTIPROCESSOR
void
bcm2836_cpu_hatch(struct cpu_info *ci)
{

	bcm2836mp_intr_init(ci);

	gtmr_init_cpu_clock(ci);
}
#endif
