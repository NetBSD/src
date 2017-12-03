/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: awin_io.c,v 1.8.12.3 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/locore.h>
#include <arm/mainbus/mainbus.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

static int awinio_match(device_t, cfdata_t, void *);
static void awinio_attach(device_t, device_t, void *);

static struct awinio_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_tag_t sc_a4x_bst;
	bus_space_handle_t sc_bsh;
	bus_space_handle_t sc_ccm_bsh;
	bus_space_handle_t sc_a80_usb_bsh;
	bus_space_handle_t sc_a80_core2_bsh;
	bus_space_handle_t sc_a80_rcpus_bsh;
	bus_dma_tag_t sc_dmat;
	bus_dma_tag_t sc_coherent_dmat;
} awinio_sc;

CFATTACH_DECL_NEW(awin_io, 0,
	awinio_match, awinio_attach, NULL, NULL);

static int
awinio_match(device_t parent, cfdata_t cf, void *aux)
{
	if (awinio_sc.sc_dev != NULL)
		return 0;

	return 1;
}

static int
awinio_print(void *aux, const char *pnp)
{
	const struct awinio_attach_args * const aio = aux;

	if (aio->aio_loc.loc_port != AWINIOCF_PORT_DEFAULT)
		aprint_normal(" port %d", aio->aio_loc.loc_port);

	return QUIET;
}

#define	OFFANDSIZE(n) AWIN_##n##_OFFSET, (AWIN_##n##_OFFSET < 0x20000 ? 0x1000 : 0x400)
#define	NOPORT	AWINIOCF_PORT_DEFAULT
#define	NOINTR	AWINIO_INTR_DEFAULT
#define	AANY	0
#define	A10	AWINIO_ONLY_A10
#define	A20	AWINIO_ONLY_A20
#define	A31	AWINIO_ONLY_A31
#define	A80	AWINIO_ONLY_A80
#define	REQ	AWINIO_REQUIRED

static const struct awin_locators awin_locators[] = {
	{ "awinicu", OFFANDSIZE(INTC), NOPORT, NOINTR, A10|REQ },
	{ "awingpio", OFFANDSIZE(PIO), NOPORT, AWIN_IRQ_PIO, A10|A20|REQ },
	{ "awingpio", OFFANDSIZE(PIO), NOPORT, NOINTR, A31|REQ },
	{ "awingpio", OFFANDSIZE(A80_PIO), NOPORT, NOINTR, A80|REQ },
	{ "awindma", OFFANDSIZE(DMA), NOPORT, AWIN_IRQ_DMA, A10|A20 },
	{ "awindma", OFFANDSIZE(DMA), NOPORT, AWIN_A31_IRQ_DMA, A31 },
	{ "awindma", OFFANDSIZE(A80_DMA), NOPORT, AWIN_A80_IRQ_DMA, A80 },
	{ "awintmr", OFFANDSIZE(TMR), NOPORT, AWIN_IRQ_TMR0, A10 },
	{ "awincnt", OFFANDSIZE(CPUCFG), NOPORT, NOINTR, A20 },
	{ "awincnt", OFFANDSIZE(A31_CPUCFG), NOPORT, NOINTR, A31 },
	{ "com", OFFANDSIZE(UART0), 0, AWIN_IRQ_UART0, A10|A20 },
	{ "com", OFFANDSIZE(UART1), 1, AWIN_IRQ_UART1, A10|A20 },
	{ "com", OFFANDSIZE(UART2), 2, AWIN_IRQ_UART2, A10|A20 },
	{ "com", OFFANDSIZE(UART3), 3, AWIN_IRQ_UART3, A10|A20 },
	{ "com", OFFANDSIZE(UART4), 4, AWIN_IRQ_UART4, A10|A20 },
	{ "com", OFFANDSIZE(UART5), 5, AWIN_IRQ_UART5, A10|A20 },
	{ "com", OFFANDSIZE(UART6), 6, AWIN_IRQ_UART6, A10|A20 },
	{ "com", OFFANDSIZE(UART7), 7, AWIN_IRQ_UART7, A10|A20 },
	{ "com", OFFANDSIZE(UART0), 0, AWIN_A31_IRQ_UART0, A31 },
	{ "com", OFFANDSIZE(A80_UART0), 0, AWIN_A80_IRQ_UART0, A80 },
	{ "awinmp", OFFANDSIZE(MP), NOPORT, AWIN_A31_IRQ_MP, A31 },
	{ "awindebe", AWIN_DE_BE0_OFFSET, 0x1000, 0, NOINTR, A20|A31 },
	{ "awindebe", AWIN_DE_BE1_OFFSET, 0x1000, 1, NOINTR, A20|A31 },
	{ "awindebe", AWIN_A80_DE_BE0_OFFSET, 0x1000, 0, NOINTR, A80 },
	{ "awindebe", AWIN_A80_DE_BE1_OFFSET, 0x1000, 1, NOINTR, A80 },
	{ "awindebe", AWIN_A80_DE_BE2_OFFSET, 0x1000, 2, NOINTR, A80 },
	{ "awintcon", OFFANDSIZE(LCD0), 0, NOINTR, A20|A31 },
	{ "awintcon", OFFANDSIZE(LCD1), 1, NOINTR, A20|A31 },
	{ "awintcon", OFFANDSIZE(A80_LCD0), 0, NOINTR, A80 },
	{ "awinhdmi", OFFANDSIZE(HDMI), NOPORT, AWIN_IRQ_HDMI0, A20 },
	{ "awinhdmi", OFFANDSIZE(HDMI), NOPORT, AWIN_A31_IRQ_HDMI, A31 },
	{ "awinhdmi", OFFANDSIZE(A80_HDMI), NOPORT, AWIN_A80_IRQ_HDMI, A80 },
	{ "awinwdt", OFFANDSIZE(TMR), NOPORT, NOINTR, A10|A20|A31 },
	{ "awinwdt", OFFANDSIZE(A80_TIMER), NOPORT, NOINTR, A80 },
	{ "awinrtc", OFFANDSIZE(TMR), NOPORT, NOINTR, A10|A20 },
	{ "awinrtc", OFFANDSIZE(A31_RTC), NOPORT, NOINTR, A31 },
	{ "awinusb", OFFANDSIZE(USB1), 0, NOINTR, A10|A20 },
	{ "awinusb", OFFANDSIZE(USB2), 1, NOINTR, A10|A20 },
	{ "awinusb", OFFANDSIZE(A31_USB1), 0, NOINTR, A31 },
	{ "awinusb", OFFANDSIZE(A31_USB2), 1, NOINTR, A31 },
	{ "awinusb", OFFANDSIZE(A80_USB0), 0, NOINTR, A80 },
	{ "awinusb", OFFANDSIZE(A80_USB1), 1, NOINTR, A80 },
	{ "awinusb", OFFANDSIZE(A80_USB2), 2, NOINTR, A80 },
	{ "motg", OFFANDSIZE(USB0), NOPORT, AWIN_IRQ_USB0, A10|A20 },
	{ "motg", OFFANDSIZE(A31_USB0), NOPORT, AWIN_A31_IRQ_USB0, A31 },
	{ "awinmmc", OFFANDSIZE(SDMMC0), 0, AWIN_IRQ_SDMMC0, A10|A20 },
	{ "awinmmc", OFFANDSIZE(SDMMC1), 1, AWIN_IRQ_SDMMC1, A10|A20 },
	{ "awinmmc", OFFANDSIZE(SDMMC2), 2, AWIN_IRQ_SDMMC2, A10|A20 },
	{ "awinmmc", OFFANDSIZE(SDMMC3), 3, AWIN_IRQ_SDMMC3, A10|A20 },
	{ "awinmmc", OFFANDSIZE(SDMMC1), 4, AWIN_IRQ_SDMMC1, A10|A20 },
	{ "awinmmc", OFFANDSIZE(SDMMC0), 0, AWIN_A31_IRQ_SDMMC0, A31 },
	{ "awinmmc", OFFANDSIZE(SDMMC1), 1, AWIN_A31_IRQ_SDMMC1, A31 },
	{ "awinmmc", OFFANDSIZE(SDMMC2), 2, AWIN_A31_IRQ_SDMMC2, A31 },
	{ "awinmmc", OFFANDSIZE(SDMMC3), 3, AWIN_A31_IRQ_SDMMC3, A31 },
	{ "awinmmc", OFFANDSIZE(SDMMC0), 0, AWIN_A80_IRQ_SDMMC0, A80 },
	{ "awinmmc", OFFANDSIZE(SDMMC1), 1, AWIN_A80_IRQ_SDMMC1, A80 },
	{ "awinmmc", OFFANDSIZE(SDMMC2), 2, AWIN_A80_IRQ_SDMMC2, A80 },
	{ "awinmmc", OFFANDSIZE(SDMMC3), 3, AWIN_A80_IRQ_SDMMC3, A80 },
	{ "ahcisata", OFFANDSIZE(SATA), NOPORT, AWIN_IRQ_SATA, A10|A20 },
	{ "awiniic", OFFANDSIZE(TWI0), 0, AWIN_IRQ_TWI0, A10|A20 },
	{ "awiniic", OFFANDSIZE(TWI1), 1, AWIN_IRQ_TWI1, A10|A20 },
	{ "awiniic", OFFANDSIZE(TWI2), 2, AWIN_IRQ_TWI2, A10|A20 },
	{ "awiniic", OFFANDSIZE(TWI3), 3, AWIN_IRQ_TWI3, A10|A20 },
	{ "awiniic", OFFANDSIZE(TWI4), 4, AWIN_IRQ_TWI4, A10|A20 },
	{ "awiniic", OFFANDSIZE(TWI0), 0, AWIN_A31_IRQ_TWI0, A31 },
	{ "awiniic", OFFANDSIZE(TWI1), 1, AWIN_A31_IRQ_TWI1, A31 },
	{ "awiniic", OFFANDSIZE(TWI2), 2, AWIN_A31_IRQ_TWI2, A31 },
	{ "awiniic", OFFANDSIZE(TWI3), 3, AWIN_A31_IRQ_TWI3, A31 },
	{ "awiniic", OFFANDSIZE(A80_TWI0), 0, AWIN_A80_IRQ_TWI0, A80 },
	{ "awiniic", OFFANDSIZE(A80_TWI1), 1, AWIN_A80_IRQ_TWI1, A80 },
	{ "awiniic", OFFANDSIZE(A80_TWI2), 2, AWIN_A80_IRQ_TWI2, A80 },
	{ "awiniic", OFFANDSIZE(A80_TWI3), 3, AWIN_A80_IRQ_TWI3, A80 },
	{ "awiniic", OFFANDSIZE(A80_TWI4), 4, AWIN_A80_IRQ_TWI4, A80 },
	{ "awintve", OFFANDSIZE(TVE0), 0, AWIN_IRQ_TVE, A20 },
	{ "awinp2wi", OFFANDSIZE(A31_P2WI), NOPORT, AWIN_A31_IRQ_P2WI, A31 },
	{ "awinp2wi", OFFANDSIZE(A80_RSB), NOPORT, AWIN_A80_IRQ_R_RSB, A80 },
	{ "spi", OFFANDSIZE(SPI0), 0, AWIN_IRQ_SPI0, AANY },
	{ "spi", OFFANDSIZE(SPI1), 1, AWIN_IRQ_SPI1, AANY },
	{ "spi", OFFANDSIZE(SPI2), 1, AWIN_IRQ_SPI2, AANY },
	{ "spi", OFFANDSIZE(SPI3), 3, AWIN_IRQ_SPI3, AANY },
	{ "awe", OFFANDSIZE(EMAC), NOPORT, AWIN_IRQ_EMAC, A10|A20 },
	{ "awge", OFFANDSIZE(GMAC), NOPORT, AWIN_IRQ_GMAC, A20 },
	{ "awge", OFFANDSIZE(A31_GMAC), NOPORT, AWIN_A31_IRQ_GMAC, A31 },
	{ "awge", OFFANDSIZE(A80_GMAC), NOPORT, AWIN_A80_IRQ_EMAC, A80 },
	{ "awincrypto", OFFANDSIZE(SS), NOPORT, AWIN_IRQ_SS, AANY },
	{ "awinac", OFFANDSIZE(AC), NOPORT, AWIN_IRQ_AC, A10|A20 },
	{ "awinac", OFFANDSIZE(AC), NOPORT, AWIN_A31_IRQ_AC, A31 },
	{ "awindaudio", OFFANDSIZE(A80_DAUDIO1), 1, AWIN_A80_IRQ_R_DAUDIO, A80 },
	{ "awinhdmiaudio", OFFANDSIZE(HDMI), NOPORT, NOINTR, A20 },
	{ "awinhdmiaudio", OFFANDSIZE(HDMI), NOPORT, NOINTR, A31 },
	{ "awinnand", OFFANDSIZE(NFC), NOPORT, AWIN_IRQ_NAND, A10|A20 },
	{ "awinir", OFFANDSIZE(IR0), 0, AWIN_IRQ_IR0, A10|A20 },
	{ "awinir", OFFANDSIZE(IR1), 1, AWIN_IRQ_IR1, A10|A20 },
	{ "awinir", OFFANDSIZE(A31_CIR), NOPORT, AWIN_A31_IRQ_CIR, A31 },
	{ "awinir", OFFANDSIZE(A80_CIR), NOPORT, AWIN_A80_IRQ_R_CIR, A80 },
	{ "awinlradc", OFFANDSIZE(LRADC), NOPORT, AWIN_IRQ_LRADC, A20 },
	{ "awincan", OFFANDSIZE(CAN), NOPORT, AWIN_IRQ_CAN, A20 },
};

static int
awinio_find(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	const struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	const int port = cf->cf_loc[AWINIOCF_PORT];

	if (strcmp(cf->cf_name, loc->loc_name)
	    || (port != AWINIOCF_PORT_DEFAULT && port != loc->loc_port))
		return 0;

	return config_match(parent, cf, aux);
}

static void
awinio_attach(device_t parent, device_t self, void *aux)
{
	struct awinio_softc * const sc = &awinio_sc;
	uint16_t chip_id = awin_chip_id();
	const char *chip_name = awin_chip_name();
	const bool a10_p = chip_id == AWIN_CHIP_ID_A10;
	const bool a20_p = chip_id == AWIN_CHIP_ID_A20;
	const bool a31_p = chip_id == AWIN_CHIP_ID_A31;
	const bool a80_p = chip_id == AWIN_CHIP_ID_A80;
	prop_dictionary_t dict = device_properties(self);

	sc->sc_dev = self;

	sc->sc_bst = &armv7_generic_bs_tag;
	sc->sc_a4x_bst = &armv7_generic_a4x_bs_tag;
	sc->sc_bsh = awin_core_bsh;
	sc->sc_dmat = &awin_dma_tag;
	sc->sc_coherent_dmat = &awin_coherent_dma_tag;

	switch (awin_chip_id()) {
#ifdef ALLWINNER_A80
	case AWIN_CHIP_ID_A80:
		sc->sc_a80_core2_bsh = awin_core2_bsh;
		sc->sc_a80_rcpus_bsh = awin_rcpus_bsh;
		bus_space_subregion(sc->sc_bst, sc->sc_bsh,
		    AWIN_A80_CCU_SCLK_OFFSET, 0x1000, &sc->sc_ccm_bsh);
		bus_space_map(sc->sc_bst, AWIN_A80_USB_PBASE,
		    AWIN_A80_USB_SIZE, 0, &sc->sc_a80_usb_bsh);
		break;
#endif
	default:
		bus_space_subregion(sc->sc_bst, sc->sc_bsh, AWIN_CCM_OFFSET,
		    0x1000, &sc->sc_ccm_bsh);
		break;
	}

	aprint_naive("\n");
	aprint_normal(": %s (0x%04x)\n", chip_name, chip_id);

	const struct awin_locators * const eloc =
	    awin_locators + __arraycount(awin_locators);
	for (const struct awin_locators *loc = awin_locators; loc < eloc; loc++) {
		char prop_name[31];
		bool skip;
		if (loc->loc_port == AWINIOCF_PORT_DEFAULT) {
			snprintf(prop_name, sizeof(prop_name),
			    "no-%s", loc->loc_name);
		} else {
			snprintf(prop_name, sizeof(prop_name),
			    "no-%s-%d", loc->loc_name, loc->loc_port);
		}
		if (prop_dictionary_get_bool(dict, prop_name, &skip) && skip)
			continue;

		if (loc->loc_flags & AWINIO_ONLY) {
			if (a10_p && !(loc->loc_flags & AWINIO_ONLY_A10))
				continue;
			if (a20_p && !(loc->loc_flags & AWINIO_ONLY_A20))
				continue;
			if (a31_p && !(loc->loc_flags & AWINIO_ONLY_A31))
				continue;
			if (a80_p && !(loc->loc_flags & AWINIO_ONLY_A80))
				continue;
		}

		struct awinio_attach_args aio = {
			.aio_loc = *loc,
			.aio_core_bst = sc->sc_bst,
			.aio_core_a4x_bst = sc->sc_a4x_bst,
			.aio_core_bsh = sc->sc_bsh,
			.aio_ccm_bsh = sc->sc_ccm_bsh,
			.aio_a80_usb_bsh = sc->sc_a80_usb_bsh,
			.aio_a80_core2_bsh = sc->sc_a80_core2_bsh,
			.aio_a80_rcpus_bsh = sc->sc_a80_rcpus_bsh,
			.aio_dmat = sc->sc_dmat,
			.aio_coherent_dmat = sc->sc_coherent_dmat,
		};
		cfdata_t cf = config_search_ia(awinio_find,
		    sc->sc_dev, "awinio", &aio);
		if (cf == NULL) {
			if (loc->loc_flags & AWINIO_REQUIRED)
				panic("%s: failed to find %s!", __func__,
				    loc->loc_name);
			continue;
		}
		config_attach(sc->sc_dev, cf, &aio, awinio_print);
	}
}
