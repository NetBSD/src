/*	$NetBSD: com_obio.c,v 1.1.10.2 2006/05/24 15:47:54 tron Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: com_obio.c,v 1.1.10.2 2006/05/24 15:47:54 tron Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <machine/bus.h>

#include <arm/xscale/i80321var.h>

#include <evbarm/hdl_g/obiovar.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

struct com_obio_softc {
	struct com_softc sc_com;

	void *sc_ih;
};

int	com_obio_match(struct device *, struct cfdata *, void *);
void	com_obio_attach(struct device *, struct device *, void *);

CFATTACH_DECL(com_obio, sizeof(struct com_obio_softc),
    com_obio_match, com_obio_attach, NULL, NULL);

int
com_obio_match(struct device *parent, struct cfdata *cf, void *aux)
{

	/* We take it on faith that the device is there. */
	return 1;
}

void
com_obio_attach(struct device *parent, struct device *self, void *aux)
{
	struct obio_attach_args *oba = aux;
	struct com_obio_softc *osc = (void *) self;
	struct com_softc *sc = &osc->sc_com;
	int error;

	sc->sc_iot = oba->oba_st;
	sc->sc_iobase = oba->oba_addr;
	sc->sc_frequency = COM_FREQ;
	sc->sc_hwflags = COM_HW_NO_TXPRELOAD;
	error = bus_space_map(sc->sc_iot, oba->oba_addr, 8, 0, &sc->sc_ioh);

	if (error) {
		aprint_error(": failed to map registers: %d\n", error);
		return;
	}

	com_attach_subr(sc);

	osc->sc_ih = i80321_intr_establish(oba->oba_irq, IPL_SERIAL,
	    comintr, sc);
	if (osc->sc_ih == NULL)
		aprint_error("%s: unable to establish interrupt at irq %d\n",
		    device_xname(&sc->sc_dev), oba->oba_irq);
}
