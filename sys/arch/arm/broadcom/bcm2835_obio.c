/*	$NetBSD: bcm2835_obio.c,v 1.5.4.3 2013/01/23 00:05:41 yamt Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_obio.c,v 1.5.4.3 2013/01/23 00:05:41 yamt Exp $");

#include "locators.h"
#include "obio.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm_amba.h>

struct obio_softc {
	device_t		sc_dev;
	bus_dma_tag_t		sc_dmat;
	struct arm32_dma_range	sc_dmarange;
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
	{
		/* Mailbox */
		.ad_name = "bcmmbox",
		.ad_addr = BCM2835_ARMMBOX_BASE,
		.ad_size = BCM2835_ARMMBOX_SIZE,
		.ad_intr = -1, /* BCM2835_INT_ARMMAILBOX */
	},
	{
		/* System Timer */
		.ad_name = "bcmtmr",
		.ad_addr = BCM2835_STIMER_BASE,
		.ad_size = BCM2835_STIMER_SIZE,
		.ad_intr = BCM2835_INT_TIMER3,
	},
	{
		/* Power Management, Reset controller and Watchdog registers */
		.ad_name = "bcmpm",
		.ad_addr = BCM2835_PM_BASE,
		.ad_size = BCM2835_PM_SIZE,
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
		/* Framebuffer */
		.ad_name = "fb",
		.ad_addr = 0,
		.ad_size = 0,
		.ad_intr = -1,
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
		.ad_name = "dotg",
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
		/* Terminator */
		.ad_name = NULL,
	}
};


static int
obio_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

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

	sc->sc_dmarange.dr_sysbase = 0;
	sc->sc_dmarange.dr_busbase = 0xc0000000;	/* 0x40000000 if L2 */
	sc->sc_dmarange.dr_len = physmem * PAGE_SIZE;
	bcm2835_bus_dma_tag._ranges = &sc->sc_dmarange;
	bcm2835_bus_dma_tag._nranges = 1;

	aprint_normal("\n");

	/* Set up the attach args. */
	aaa.aaa_iot = &bcm2835_bs_tag;
	aaa.aaa_dmat = sc->sc_dmat;

	for (; ad->ad_name != NULL; ad++) {
		aprint_debug("dev=%s[%u], addr=%lx:+%lx\n",
		    ad->ad_name, ad->ad_instance, ad->ad_addr, ad->ad_size);
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
