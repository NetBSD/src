/* $NetBSD: fdc_sableio.c,v 1.1.2.2 2001/01/08 14:56:10 bouyer Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: fdc_sableio.c,v 1.1.2.2 2001/01/08 14:56:10 bouyer Exp $");

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

int	fdc_sableio_match(struct device *, struct cfdata *, void *);
void	fdc_sableio_attach(struct device *, struct device *, void *);

struct fdc_sableio_softc {
	struct	fdc_softc sc_fdc;	/* real "fdc" softc */

	bus_space_handle_t sc_baseioh;	/* base I/O handle */
};

struct cfattach fdc_sableio_ca = {
	sizeof(struct fdc_sableio_softc), fdc_sableio_match, fdc_sableio_attach
};

int
fdc_sableio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct sableio_attach_args *sa = aux;

	/* Always present. */
	if (strcmp(sa->sa_name, match->cf_driver->cd_name) == 0)
		return (1);

	return (0);
}

void
fdc_sableio_attach(struct device *parent, struct device *self, void *aux)
{
	struct fdc_softc *fdc = (void *) self;
	struct fdc_sableio_softc *sfdc = (void *) self;
	struct sableio_attach_args *sa = aux;
	const char *intrstr;

	printf("\n");

	fdc->sc_iot = sa->sa_iot;
	fdc->sc_ic = sa->sa_ic;
	fdc->sc_drq = sa->sa_drq;

	if (bus_space_map(fdc->sc_iot, sa->sa_ioaddr, 6 /* FDC_NPORT */, 0,
	    &sfdc->sc_baseioh)) {
		printf("%s: unable to map I/O space\n", fdc->sc_dev.dv_xname);
		return;
	}

	if (bus_space_subregion(fdc->sc_iot, sfdc->sc_baseioh, 2, 4,
	    &fdc->sc_ioh)) {
		printf("%s: unable to subregion I/O space\n",
		    fdc->sc_dev.dv_xname);
		return;
	}

	if (bus_space_map(fdc->sc_iot, sa->sa_ioaddr + fdctl + 2, 1, 0,
	    &fdc->sc_fdctlioh)) {
		printf("%s: unable to map CTL I/O space\n",
		    fdc->sc_dev.dv_xname);
		return;
	}

	intrstr = pci_intr_string(sa->sa_pc, sa->sa_sableirq[0]);
	fdc->sc_ih = pci_intr_establish(sa->sa_pc, sa->sa_sableirq[0],
	    IPL_BIO, fdcintr, fdc);
	if (fdc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt",
		    fdc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", fdc->sc_dev.dv_xname, intrstr);

	fdcattach(fdc);
}
