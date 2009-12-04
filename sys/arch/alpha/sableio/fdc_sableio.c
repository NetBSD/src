/* $NetBSD: fdc_sableio.c,v 1.10 2009/12/04 11:13:04 njoly Exp $ */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: fdc_sableio.c,v 1.10 2009/12/04 11:13:04 njoly Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/buf.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/isa/fdreg.h>
#include <dev/isa/fdcvar.h>

#include <dev/pci/pcivar.h>

#include <alpha/sableio/sableiovar.h>

int	fdc_sableio_match(device_t, cfdata_t, void *);
void	fdc_sableio_attach(device_t, device_t, void *);

struct fdc_sableio_softc {
	struct	fdc_softc sc_fdc;	/* real "fdc" softc */

	bus_space_handle_t sc_baseioh;	/* base I/O handle */
};

CFATTACH_DECL_NEW(fdc_sableio, sizeof(struct fdc_sableio_softc),
    fdc_sableio_match, fdc_sableio_attach, NULL, NULL);

int
fdc_sableio_match(device_t parent, cfdata_t match, void *aux)
{
	struct sableio_attach_args *sa = aux;

	/* Always present. */
	if (strcmp(sa->sa_name, match->cf_name) == 0)
		return (1);

	return (0);
}

void
fdc_sableio_attach(device_t parent, device_t self, void *aux)
{
	struct fdc_sableio_softc *sfdc = device_private(self);
	struct fdc_softc *fdc = &sfdc->sc_fdc;
	struct sableio_attach_args *sa = aux;
	const char *intrstr;

	aprint_normal("\n");

	fdc->sc_dev = self;
	fdc->sc_iot = sa->sa_iot;
	fdc->sc_ic = sa->sa_ic;
	fdc->sc_drq = sa->sa_drq;

	if (bus_space_map(fdc->sc_iot, sa->sa_ioaddr, 6 /* FDC_NPORT */, 0,
	    &sfdc->sc_baseioh)) {
		aprint_error_dev(self, "unable to map I/O space\n");
		return;
	}

	if (bus_space_subregion(fdc->sc_iot, sfdc->sc_baseioh, 2, 4,
	    &fdc->sc_ioh)) {
		aprint_error_dev(self, "unable to subregion I/O space\n");
		return;
	}

	if (bus_space_map(fdc->sc_iot, sa->sa_ioaddr + fdctl + 2, 1, 0,
	    &fdc->sc_fdctlioh)) {
		aprint_error_dev(self, "unable to map CTL I/O space\n");
		return;
	}

	intrstr = pci_intr_string(sa->sa_pc, sa->sa_sableirq[0]);
	fdc->sc_ih = pci_intr_establish(sa->sa_pc, sa->sa_sableirq[0],
	    IPL_BIO, fdcintr, fdc);
	if (fdc->sc_ih == NULL) {
		aprint_error_dev(self, "unable to establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		return;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	fdcattach(fdc);
}
