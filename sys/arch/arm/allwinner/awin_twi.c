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

__KERNEL_RCSID(1, "$NetBSD: awin_twi.c,v 1.3.12.3 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/gttwsivar.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#define TWI_CCR_REG	0x14
#define TWI_CCR_CLK_M	__BITS(6,3)
#define TWI_CCR_CLK_N	__BITS(2,0)

static int awin_twi_match(device_t, cfdata_t, void *);
static void awin_twi_attach(device_t, device_t, void *);

struct awin_twi_softc {
	struct gttwsi_softc asc_sc;
	void *asc_ih;
};

static int awin_twi_ports;

static const struct awin_gpio_pinset awin_twi_pinsets[] = {
	[0] = { 'B', AWIN_PIO_PB_TWI0_FUNC, AWIN_PIO_PB_TWI0_PINS },
	[1] = { 'B', AWIN_PIO_PB_TWI1_FUNC, AWIN_PIO_PB_TWI1_PINS },
	[2] = { 'B', AWIN_PIO_PB_TWI2_FUNC, AWIN_PIO_PB_TWI2_PINS },
	[3] = { 'I', AWIN_PIO_PI_TWI3_FUNC, AWIN_PIO_PI_TWI3_PINS },
	[4] = { 'I', AWIN_PIO_PI_TWI4_FUNC, AWIN_PIO_PI_TWI4_PINS },
};

static const struct awin_gpio_pinset awin_twi_pinsets_a31[] = {
	[0] = { 'H', AWIN_A31_PIO_PH_TWI0_FUNC, AWIN_A31_PIO_PH_TWI0_PINS },
	[1] = { 'H', AWIN_A31_PIO_PH_TWI1_FUNC, AWIN_A31_PIO_PH_TWI1_PINS },
	[2] = { 'H', AWIN_A31_PIO_PH_TWI2_FUNC, AWIN_A31_PIO_PH_TWI2_PINS },
	[3] = { 'B', AWIN_A31_PIO_PB_TWI3_FUNC, AWIN_A31_PIO_PB_TWI3_PINS },
};

static const struct awin_gpio_pinset awin_twi_pinsets_a80[] = {
	[0] = { 'H', AWIN_A80_PIO_PH_TWI0_FUNC, AWIN_A80_PIO_PH_TWI0_PINS },
	[1] = { 'H', AWIN_A80_PIO_PH_TWI1_FUNC, AWIN_A80_PIO_PH_TWI1_PINS },
	[2] = { 'H', AWIN_A80_PIO_PH_TWI2_FUNC, AWIN_A80_PIO_PH_TWI2_PINS },
	[3] = { 'G', AWIN_A80_PIO_PG_TWI3_FUNC, AWIN_A80_PIO_PG_TWI3_PINS },
	[4] = { 'B', AWIN_A80_PIO_PB_TWI4_FUNC, AWIN_A80_PIO_PB_TWI4_PINS },
};

CFATTACH_DECL_NEW(awin_twi, sizeof(struct awin_twi_softc),
	awin_twi_match, awin_twi_attach, NULL, NULL);

static int
awin_twi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	unsigned int port = loc->loc_port;

	KASSERT(!strcmp(cf->cf_name, loc->loc_name));
	KASSERT(cf->cf_loc[AWINIOCF_PORT] == AWINIOCF_PORT_DEFAULT
	    || cf->cf_loc[AWINIOCF_PORT] == loc->loc_port);
	KASSERT((awin_twi_ports & __BIT(loc->loc_port)) == 0);

	/*
	 * We don't have alternative mappings so if one is requested
	 * fail the match.
	 */
	if (cf->cf_flags & 1)
		return 0;

	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		if (!awin_gpio_pinset_available(&awin_twi_pinsets_a31[port]))
			return 0;
	} else if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		if (!awin_gpio_pinset_available(&awin_twi_pinsets_a80[port]))
			return 0;
	} else {
		if (!awin_gpio_pinset_available(&awin_twi_pinsets[port]))
			return 0;
	}

	return 1;
}

static void
awin_twi_attach(device_t parent, device_t self, void *aux)
{
	struct awin_twi_softc * const asc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	prop_dictionary_t cfg = device_properties(self);
	bus_space_handle_t bsh;
	uint32_t ccr;

	awin_twi_ports |= __BIT(loc->loc_port);

	/*
	 * Acquire the PIO pins needed for the TWI port.
	 */
	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		awin_gpio_pinset_acquire(&awin_twi_pinsets_a31[loc->loc_port]);
	} else if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		awin_gpio_pinset_acquire(&awin_twi_pinsets_a80[loc->loc_port]);
	} else {
		awin_gpio_pinset_acquire(&awin_twi_pinsets[loc->loc_port]);
	}

	/*
	 * Clock gating, soft reset
	 */
	if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A80_CCU_SCLK_BUS_CLK_GATING4_REG,
		    AWIN_A80_CCU_SCLK_BUS_CLK_GATING4_TWI0 << loc->loc_port, 0);
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A80_CCU_SCLK_BUS_SOFT_RST4_REG,
		    AWIN_A80_CCU_SCLK_BUS_SOFT_RST4_TWI0 << loc->loc_port, 0);
	} else {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_APB1_GATING_REG,
		    AWIN_APB_GATING1_TWI0 << loc->loc_port, 0);
		if (awin_chip_id() == AWIN_CHIP_ID_A31) {
			awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
			    AWIN_A31_APB2_RESET_REG,
			    AWIN_A31_APB2_RESET_TWI0_RST << loc->loc_port, 0);
		}
	}

	/*
	 * Get a bus space handle for this TWI port.
	 */
	bus_space_subregion(aio->aio_core_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &bsh);

	/*
	 * A31/A80 quirk
	 */
	switch (awin_chip_id()) {
	case AWIN_CHIP_ID_A31:
	case AWIN_CHIP_ID_A80:
		prop_dictionary_set_bool(cfg, "iflg-rwc", true);
		break;
	}

	/*
	 * Set clock rate to 100kHz. From the datasheet:
	 *   For 100Khz standard speed 2Wire, CLK_N=2, CLK_M=11
	 *   F0=48M/2^2=12Mhz, F1=F0/(10*(11+1)) = 0.1Mhz
	 */
	ccr = __SHIFTIN(11, TWI_CCR_CLK_M) | __SHIFTIN(2, TWI_CCR_CLK_N);
	bus_space_write_4(aio->aio_core_bst, bsh, TWI_CCR_REG, ccr);

	/*
	 * Do the MI attach
	 */
	gttwsi_attach_subr(self, aio->aio_core_bst, bsh);

	/*
	 * Establish interrupt for it
	 */
	asc->asc_ih = intr_establish(loc->loc_intr, IPL_BIO, IST_LEVEL,
	    gttwsi_intr, &asc->asc_sc);
	if (asc->asc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		    loc->loc_intr);
                return;
        }
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

	/*
	 * Configure its children
	 */
	gttwsi_config_children(self);
}

struct i2c_controller *
awin_twi_get_controller(device_t dev)
{
	if (!device_is_a(dev, "awiniic"))
		return NULL;

	struct awin_twi_softc * const sc = device_private(dev);

	return &sc->asc_sc.sc_i2c;
}
