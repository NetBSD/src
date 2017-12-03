/* $NetBSD: lpt_pnpbios.c,v 1.12.12.2 2017/12/03 11:36:18 jdolecek Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: lpt_pnpbios.c,v 1.12.12.2 2017/12/03 11:36:18 jdolecek Exp $");

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

#include <dev/ic/lptvar.h>

struct lpt_pnpbios_softc {
	struct	lpt_softc sc_lpt;
};

int lpt_pnpbios_match(device_t, cfdata_t, void *);
void lpt_pnpbios_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(lpt_pnpbios, sizeof(struct lpt_pnpbios_softc),
    lpt_pnpbios_match, lpt_pnpbios_attach, NULL, NULL);

int
lpt_pnpbios_match(device_t parent, cfdata_t match, void *aux)
{
	struct pnpbiosdev_attach_args *aa = aux;

	if (strcmp(aa->idstr, "PNP0400") &&
	    strcmp(aa->idstr, "PNP0401"))
		return (0);

	return (1);
}

void
lpt_pnpbios_attach(device_t parent, device_t self, void *aux)
{
	struct lpt_pnpbios_softc *psc = device_private(self);
	struct lpt_softc *sc = &psc->sc_lpt;
	struct pnpbiosdev_attach_args *aa = aux;

	sc->sc_dev = self;

       /* Lest someone attach a parallel-port SCSI adapter etc:
	  this is really ISA DMA under the covers: clamp max transfer size */
        self->dv_maxphys = MIN(parent->dv_maxphys, 64 * 1024);

	aprint_naive("\n");
	if (pnpbios_io_map(aa->pbt, aa->resc, 0, &sc->sc_iot, &sc->sc_ioh)) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	aprint_normal("\n");
	pnpbios_print_devres(self, aa);

	lpt_attach_subr(sc);

	sc->sc_ih = pnpbios_intr_establish(aa->pbt, aa->resc, 0, IPL_TTY,
					    lptintr, sc);
}
