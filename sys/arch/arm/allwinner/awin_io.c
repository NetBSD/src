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

__KERNEL_RCSID(1, "$NetBSD: awin_io.c,v 1.11 2014/09/07 17:49:39 jmcneill Exp $");

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
#define	REQ	AWINIO_REQUIRED

static const struct awin_locators awin_locators[] = {
	{ "awinicu", OFFANDSIZE(INTC), NOPORT, NOINTR, A10|REQ },
	{ "awingpio", OFFANDSIZE(PIO), NOPORT, NOINTR, AANY|REQ },
	{ "awindma", OFFANDSIZE(DMA), NOPORT, AWIN_IRQ_DMA, AANY|REQ },
	{ "awintmr", OFFANDSIZE(TMR), NOPORT, AWIN_IRQ_TMR0, A10 },
	{ "com", OFFANDSIZE(UART0), 0, AWIN_IRQ_UART0, AANY },
	{ "com", OFFANDSIZE(UART1), 1, AWIN_IRQ_UART1, AANY },
	{ "com", OFFANDSIZE(UART2), 2, AWIN_IRQ_UART2, AANY },
	{ "com", OFFANDSIZE(UART3), 3, AWIN_IRQ_UART3, AANY },
	{ "com", OFFANDSIZE(UART4), 4, AWIN_IRQ_UART4, AANY },
	{ "com", OFFANDSIZE(UART5), 5, AWIN_IRQ_UART5, AANY },
	{ "com", OFFANDSIZE(UART6), 6, AWIN_IRQ_UART6, AANY },
	{ "com", OFFANDSIZE(UART7), 7, AWIN_IRQ_UART7, AANY },
	{ "awinwdt", OFFANDSIZE(TMR), NOPORT, NOINTR, AANY },
	{ "awinrtc", OFFANDSIZE(TMR), NOPORT, NOINTR, AANY },
	{ "awinusb", OFFANDSIZE(USB1), 0, NOINTR, AANY },
	{ "awinusb", OFFANDSIZE(USB2), 1, NOINTR, AANY },
	{ "awinmmc", OFFANDSIZE(SDMMC0), 0, AWIN_IRQ_SDMMC0, AANY },
	{ "awinmmc", OFFANDSIZE(SDMMC1), 1, AWIN_IRQ_SDMMC1, AANY },
	{ "awinmmc", OFFANDSIZE(SDMMC2), 2, AWIN_IRQ_SDMMC2, AANY },
	{ "awinmmc", OFFANDSIZE(SDMMC3), 3, AWIN_IRQ_SDMMC3, AANY },
	{ "awinmmc", OFFANDSIZE(SDMMC1), 4, AWIN_IRQ_SDMMC1, AANY },
	{ "ahcisata", OFFANDSIZE(SATA), NOPORT, AWIN_IRQ_SATA, AANY },
	{ "awiniic", OFFANDSIZE(TWI0), 0, AWIN_IRQ_TWI0, AANY },
	{ "awiniic", OFFANDSIZE(TWI1), 1, AWIN_IRQ_TWI1, AANY },
	{ "awiniic", OFFANDSIZE(TWI2), 2, AWIN_IRQ_TWI2, AANY },
	{ "awiniic", OFFANDSIZE(TWI3), 3, AWIN_IRQ_TWI3, AANY },
	{ "awiniic", OFFANDSIZE(TWI4), 4, AWIN_IRQ_TWI4, AANY },
	{ "spi", OFFANDSIZE(SPI0), 0, AWIN_IRQ_SPI0, AANY },
	{ "spi", OFFANDSIZE(SPI1), 1, AWIN_IRQ_SPI1, AANY },
	{ "spi", OFFANDSIZE(SPI2), 1, AWIN_IRQ_SPI2, AANY },
	{ "spi", OFFANDSIZE(SPI3), 3, AWIN_IRQ_SPI3, AANY },
	{ "awe", OFFANDSIZE(EMAC), NOPORT, AWIN_IRQ_EMAC, AANY },
	{ "awge", AWIN_GMAC_OFFSET, AWIN_GMAC_SIZE, NOPORT, AWIN_IRQ_GMAC, A20 },
	{ "awincrypto", OFFANDSIZE(SS), NOPORT, AWIN_IRQ_SS, AANY },
	{ "awinac", OFFANDSIZE(AC), NOPORT, AWIN_IRQ_AC, AANY },
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
	const bool a10_p = CPU_ID_CORTEX_A8_P(curcpu()->ci_arm_cpuid);
	const bool a20_p = CPU_ID_CORTEX_A7_P(curcpu()->ci_arm_cpuid);
	prop_dictionary_t dict = device_properties(self);

	sc->sc_dev = self;

	sc->sc_bst = &awin_bs_tag;
	sc->sc_a4x_bst = &awin_a4x_bs_tag;
	sc->sc_bsh = awin_core_bsh;
	sc->sc_dmat = &awin_dma_tag;
	sc->sc_coherent_dmat = &awin_coherent_dma_tag;

	bus_space_subregion(sc->sc_bst, sc->sc_bsh, AWIN_CCM_OFFSET, 0x1000,
	    &sc->sc_ccm_bsh);

	aprint_naive("\n");
	aprint_normal("\n");

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
		}

		struct awinio_attach_args aio = {
			.aio_loc = *loc,
			.aio_core_bst = sc->sc_bst,
			.aio_core_a4x_bst = sc->sc_a4x_bst,
			.aio_core_bsh = sc->sc_bsh,
			.aio_ccm_bsh = sc->sc_ccm_bsh,
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
