/* $NetBSD: plcom_obio.c,v 1.1 2014/08/10 05:47:38 matt Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

__KERNEL_RCSID(1, "$NetBSD: plcom_obio.c,v 1.1 2014/08/10 05:47:38 matt Exp $");

/* Interface to plcom (PL011) serial driver. */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/intr.h>
#include <sys/bus.h>

#include <sys/termios.h>
#include <evbarm/dev/plcomreg.h>
#include <evbarm/dev/plcomvar.h>

#include <aarch64/locore.h>
#include <evbarm64/a64emul/obio_var.h>

static int plcom_obio_match(device_t, cfdata_t, void *);
static void plcom_obio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(plcom_obio, sizeof(struct plcom_softc),
    plcom_obio_match, plcom_obio_attach, NULL, NULL);

static int
plcom_obio_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct obio_attach_args * const oba = aux;

	if (strcmp(oba->oba_name, cf->cf_name) != 0)
		return 0;

	return 1;
}

void
plcom_obio_attach(device_t parent, device_t self, void *aux)
{
	struct plcom_softc * const sc = device_private(self);
	const struct obio_attach_args * const oba = aux;

	sc->sc_dev = self;
	sc->sc_frequency = 24000000;
	sc->sc_hwflags = PLCOM_HW_TXFIFO_DISABLE;
	sc->sc_swflags = 0;
	sc->sc_set_mcr = NULL;
	sc->sc_set_mcr_arg = NULL;

	sc->sc_pi.pi_type = PLCOM_TYPE_PL011;
	sc->sc_pi.pi_flags = PLC_FLAG_32BIT_ACCESS;
	sc->sc_pi.pi_iot = oba->oba_bst;
	sc->sc_pi.pi_iobase = oba->oba_addr;

	if (!plcom_is_console(oba->oba_bst, oba->oba_addr, &sc->sc_pi.pi_ioh)
	    && bus_space_map(oba->oba_bst, oba->oba_addr, PL011COM_UART_SIZE,
			0, &sc->sc_pi.pi_ioh)) {
		aprint_error_dev(self, "unable to map device\n");
		return;
	}

	plcom_attach_subr(sc);

	void * const ih = intr_establish(oba->oba_intr, IPL_SERIAL, IST_LEVEL,
	    plcomintr, sc);
	if (ih == NULL)
		aprint_error_dev(self, "cannot install interrupt handler");
}
