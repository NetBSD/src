/* $NetBSD: com_pnpbios.c,v 1.15.54.1 2019/06/10 22:06:21 christos Exp $ */
/*
 * Copyright (c) 1999
 * 	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_pnpbios.c,v 1.15.54.1 2019/06/10 22:06:21 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/termios.h>

#include <sys/bus.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <i386/pnpbios/pnpbiosvar.h>

#include <dev/ic/comvar.h>

struct com_pnpbios_softc {
	struct	com_softc sc_com;
	void	*sc_ih;
};

int com_pnpbios_match(device_t, cfdata_t , void *);
void com_pnpbios_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_pnpbios, sizeof(struct com_pnpbios_softc),
    com_pnpbios_match, com_pnpbios_attach, NULL, NULL);

int
com_pnpbios_match(device_t parent, cfdata_t match, void *aux)
{
	struct pnpbiosdev_attach_args *aa = aux;

	if (strcmp(aa->idstr, "PNP0500") &&
	    strcmp(aa->idstr, "PNP0501") &&
	    strcmp(aa->idstr, "PNP0510") &&
	    strcmp(aa->idstr, "PNP0511"))
		return (0);

	return (1);
}

void
com_pnpbios_attach(device_t parent, device_t self, void *aux)
{
	struct com_pnpbios_softc *psc = device_private(self);
	struct com_softc *sc = &psc->sc_com;
	struct pnpbiosdev_attach_args *aa = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int iobase;

	sc->sc_dev = self;

	if (pnpbios_getiobase(aa->pbt, aa->resc, 0, &iot, &iobase)) {
		aprint_error(": can't get iobase\n");
		return;
	}

	if ((!com_is_console(iot, iobase, &ioh)) &&
	    pnpbios_io_map(aa->pbt, aa->resc, 0, &iot, &ioh)) { 	
		aprint_error(": can't map i/o space\n");
		return;
	}

	com_init_regs(&sc->sc_regs, iot, ioh, iobase);

	aprint_normal("\n");
	pnpbios_print_devres(self, aa);

	aprint_normal("%s", device_xname(self));

	/*
	 * if the chip isn't something we recognise skip it.
	 */
	if (com_probe_subr(&sc->sc_regs) == 0) {
		aprint_error(": com probe failed\n");
		return;
	}

	sc->sc_frequency = 115200 * 16;

	com_attach_subr(sc);

	psc->sc_ih = pnpbios_intr_establish(aa->pbt, aa->resc, 0, IPL_SERIAL,
					    comintr, sc);
}
