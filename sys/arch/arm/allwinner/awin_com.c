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

__KERNEL_RCSID(1, "$NetBSD: awin_com.c,v 1.5.12.3 2017/12/03 11:35:50 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/termios.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/ic/comvar.h>

static int awin_com_match(device_t, cfdata_t, void *);
static void awin_com_attach(device_t, device_t, void *);

struct awin_com_softc {
	struct com_softc asc_sc;
	void *asc_ih;
};

static const struct awin_gpio_pinset awin_com_pinsets[] = {
	{ 'B', AWIN_PIO_PB_UART0_FUNC, AWIN_PIO_PB_UART0_PINS },
	{ 'A', AWIN_PIO_PA_UART1_FUNC, AWIN_PIO_PA_UART1_PINS },
	{ 'I', AWIN_PIO_PI_UART2_FUNC, AWIN_PIO_PI_UART2_PINS },
	{ 'H', AWIN_PIO_PH_UART3_FUNC, AWIN_PIO_PH_UART3_PINS },
	{ 'H', AWIN_PIO_PH_UART4_FUNC, AWIN_PIO_PH_UART4_PINS },
	{ 'H', AWIN_PIO_PH_UART5_FUNC, AWIN_PIO_PH_UART5_PINS },
	{ 'I', AWIN_PIO_PI_UART6_FUNC, AWIN_PIO_PI_UART6_PINS },
	{ 'I', AWIN_PIO_PI_UART7_FUNC, AWIN_PIO_PI_UART7_PINS },
};

/* alternative pinnings */
static const struct awin_gpio_pinset awin_com_alt_pinsets[] = {
	{ 'F', AWIN_PIO_PF_UART0_FUNC, AWIN_PIO_PF_UART0_PINS },
	{ 0, 0, 0},
	{ 'A', AWIN_PIO_PA_UART2_FUNC, AWIN_PIO_PA_UART2_PINS },
	{ 'G', AWIN_PIO_PG_UART3_FUNC, AWIN_PIO_PG_UART3_PINS },
	{ 'G', AWIN_PIO_PG_UART4_FUNC, AWIN_PIO_PG_UART4_PINS },
	{ 'I', AWIN_PIO_PI_UART5_FUNC, AWIN_PIO_PI_UART5_PINS },
	{ 'A', AWIN_PIO_PA_UART6_FUNC, AWIN_PIO_PA_UART6_PINS },
	{ 'A', AWIN_PIO_PA_UART7_FUNC, AWIN_PIO_PA_UART7_PINS },
};

static const struct awin_gpio_pinset awin_com_pinsets_a31[] = {
	{ 'H', AWIN_A31_PIO_PH_UART0_FUNC, AWIN_A31_PIO_PH_UART0_PINS },
};

static const struct awin_gpio_pinset awin_com_pinsets_a80[] = {
	{ 'H', AWIN_A80_PIO_PH_UART0_FUNC, AWIN_A80_PIO_PH_UART0_PINS },
};

CFATTACH_DECL_NEW(awin_com, sizeof(struct awin_com_softc),
	awin_com_match, awin_com_attach, NULL, NULL);

static int awin_com_ports;

static int
awin_com_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	bus_space_tag_t iot = aio->aio_core_a4x_bst;
	bus_space_handle_t bsh;
	const struct awin_gpio_pinset *pinset;

	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		pinset = awin_com_pinsets_a31;
	} else if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		pinset = awin_com_pinsets_a80;
	} else {
		pinset = loc->loc_port + ((cf->cf_flags & 1) ?
		    awin_com_alt_pinsets : awin_com_pinsets);
	}

	KASSERT(!strcmp(cf->cf_name, loc->loc_name));
#if defined(ALLWINNER_A80)
	KASSERT(loc->loc_offset >= AWIN_A80_UART0_OFFSET);
	KASSERT(loc->loc_offset <= AWIN_A80_UART5_OFFSET);
#else
	KASSERT(loc->loc_offset >= AWIN_UART0_OFFSET);
	KASSERT(loc->loc_offset <= AWIN_UART7_OFFSET);
#endif
	KASSERT((loc->loc_offset & 0x3ff) == 0);
	KASSERT((awin_com_ports & __BIT(loc->loc_port)) == 0);
	KASSERT(cf->cf_loc[AWINIOCF_PORT] == AWINIOCF_PORT_DEFAULT
	    || cf->cf_loc[AWINIOCF_PORT] == loc->loc_port);

	if (!awin_gpio_pinset_available(pinset))
		return 0;

	if (com_is_console(iot, AWIN_CORE_PBASE + loc->loc_offset, NULL))
		return 1;

	awin_gpio_pinset_acquire(pinset);

	bus_space_subregion(iot, aio->aio_core_bsh,
	    loc->loc_offset / 4, loc->loc_size, &bsh);

	/*
	 * Clock gating, soft reset
	 */
	if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A80_CCU_SCLK_BUS_CLK_GATING4_REG,
		    AWIN_A80_CCU_SCLK_BUS_CLK_GATING4_UART0 << loc->loc_port, 0);
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A80_CCU_SCLK_BUS_SOFT_RST4_REG,
		    AWIN_A80_CCU_SCLK_BUS_SOFT_RST4_UART0 << loc->loc_port, 0);
	} else {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		   AWIN_APB1_GATING_REG,
		   AWIN_APB_GATING1_UART0 << loc->loc_port, 0);
		if (awin_chip_id() == AWIN_CHIP_ID_A31) {
			awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
			    AWIN_A31_APB2_RESET_REG,
			    AWIN_A31_APB2_RESET_UART0_RST << loc->loc_port, 0);
		}
	}

	const int rv = comprobe1(iot, bsh);

	awin_gpio_pinset_release(pinset);

	return rv;
}

static void
awin_com_attach(device_t parent, device_t self, void *aux)
{
	cfdata_t cf = device_cfdata(self);
	struct awin_com_softc * const asc = device_private(self);
	struct com_softc * const sc = &asc->asc_sc;
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	bus_space_tag_t iot = aio->aio_core_a4x_bst;
	const bus_addr_t iobase = AWIN_CORE_PBASE + loc->loc_offset;
	const struct awin_gpio_pinset *pinset;
	bus_space_handle_t ioh;

	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		pinset = awin_com_pinsets_a31;
	} else if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		pinset = awin_com_pinsets_a80;
	} else {
		pinset = loc->loc_port + ((cf->cf_flags & 1) ?
		    awin_com_alt_pinsets : awin_com_pinsets);
	}

	awin_com_ports |= __BIT(loc->loc_port);

	awin_gpio_pinset_acquire(pinset);

	sc->sc_dev = self;
	sc->sc_frequency = AWIN_UART_FREQ;
	sc->sc_type = COM_TYPE_NORMAL;

	if (com_is_console(iot, iobase, &ioh) == 0
	    && bus_space_subregion(iot, aio->aio_core_bsh,
		loc->loc_offset / 4, loc->loc_size, &ioh)) {
		panic(": can't map registers");
	}
	COM_INIT_REGS(sc->sc_regs, iot, ioh, iobase);

	com_attach_subr(sc);
	aprint_naive("\n");

	KASSERT(loc->loc_intr != AWINIO_INTR_DEFAULT);
	asc->asc_ih = intr_establish(loc->loc_intr, IPL_SERIAL,
	    IST_EDGE | IST_MPSAFE, comintr, sc);
	if (asc->asc_ih == NULL)
		panic("%s: failed to establish interrupt %d",
		    device_xname(self), loc->loc_intr);
}
