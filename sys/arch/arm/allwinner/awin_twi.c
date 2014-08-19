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

__KERNEL_RCSID(1, "$NetBSD: awin_twi.c,v 1.3.12.2 2014/08/20 00:02:44 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/gttwsivar.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

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

CFATTACH_DECL_NEW(awin_twi, sizeof(struct awin_twi_softc),
	awin_twi_match, awin_twi_attach, NULL, NULL);

static int
awin_twi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

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

	if (!awin_gpio_pinset_available(&awin_twi_pinsets[loc->loc_port]))
		return 0;

	return 1;
}

static void
awin_twi_attach(device_t parent, device_t self, void *aux)
{
	struct awin_twi_softc * const asc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	bus_space_handle_t bsh;

	awin_twi_ports |= __BIT(loc->loc_port);

	/*
	 * Acquite the PIO pins needed for the TWI port.
	 */
	awin_gpio_pinset_acquire(&awin_twi_pinsets[loc->loc_port]);

	/*
	 * Get a bus space handle for this TWI port.
	 */
	bus_space_subregion(aio->aio_core_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &bsh);

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
