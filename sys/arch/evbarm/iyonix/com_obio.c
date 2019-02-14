/*	$NetBSD: com_obio.c,v 1.1 2019/02/14 21:47:52 macallan Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
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
__KERNEL_RCSID(0, "$NetBSD: com_obio.c,v 1.1 2019/02/14 21:47:52 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <sys/bus.h>

#include <arm/xscale/i80321var.h>

#include <evbarm/iyonix/obiovar.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

struct com_obio_softc {
	struct com_softc sc_com;

	void *sc_ih;
};

int	com_obio_match(device_t, cfdata_t , void *);
void	com_obio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_obio, sizeof(struct com_obio_softc),
    com_obio_match, com_obio_attach, NULL, NULL);

int
com_obio_match(device_t parent, cfdata_t cf, void *aux)
{
	/* We take it on faith that the device is there. */
	return (1);
}

void
com_obio_attach(device_t parent, device_t self, void *aux)
{
	struct obio_attach_args *oba = aux;
	struct com_obio_softc *osc = device_private(self);
	struct com_softc *sc = &osc->sc_com;
	bus_space_handle_t ioh;
	int error;

	sc->sc_dev = self;
	sc->sc_frequency = COM_FREQ;
	sc->sc_hwflags = COM_HW_NO_TXPRELOAD;
	error = bus_space_map(oba->oba_st, oba->oba_addr, 8, 0, &ioh);
	com_init_regs(&sc->sc_regs, oba->oba_st, ioh, oba->oba_addr);

	if (error) {
		aprint_error(": failed to map registers: %d\n", error);
		return;
	}

	com_attach_subr(sc);

	osc->sc_ih = i80321_intr_establish(oba->oba_irq, IPL_SERIAL,
	    comintr, sc);
	if (osc->sc_ih == NULL)
		aprint_error_dev(self,
		    "unable to establish interrupt at irq %d\n", oba->oba_irq);
}
