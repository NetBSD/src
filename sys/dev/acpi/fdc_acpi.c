/* $NetBSD: fdc_acpi.c,v 1.2 2002/12/28 19:53:50 jmcneill Exp $ */

/*
 * Copyright (c) 2002 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * ACPI attachment for the PC Floppy Controller driver, based on
 * sys/arch/i386/pnpbios/fdc_pnpbios.c by Jason R. Thorpe
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdc_acpi.c,v 1.2 2002/12/28 19:53:50 jmcneill Exp $");

#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/queue.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/isa/fdcvar.h>

int	fdc_acpi_match(struct device *, struct cfdata *, void *);
void	fdc_acpi_attach(struct device *, struct device *, void *);

struct fdc_acpi_softc {
	struct fdc_softc sc_fdc;
	bus_space_handle_t sc_baseioh;
};

CFATTACH_DECL(fdc_acpi, sizeof(struct fdc_acpi_softc), fdc_acpi_match,
    fdc_acpi_attach, NULL, NULL);

/*
 * Supported device IDs
 */

static const char * const fdc_acpi_ids[] = {
	"PNP0700",	/* PC standard floppy disk controller */
#if 0 /* XXX do we support this? */
	"PNP0701",	/* Standard floppy controller for MS Device Bay Spec */
#endif
	NULL
};

/*
 * fdc_acpi_match: autoconf(9) match routine
 */
int
fdc_acpi_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;
	const char *id;
	int i;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	for (i = 0; (id = fdc_acpi_ids[i]) != NULL; ++i) {
		if (strcmp(aa->aa_node->ad_devinfo.HardwareId, id) == 0)
			return 1;
	}

	/* No matches found */
	return 0;
}

/*
 * fdc_acpi_attach: autoconf(9) attach routine
 */
void
fdc_acpi_attach(struct device *parent, struct device *self, void *aux)
{
	struct fdc_acpi_softc *asc = (struct fdc_acpi_softc *)self;
	struct fdc_softc *sc = &asc->sc_fdc;
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	struct acpi_io *io, *ctlio;
	struct acpi_irq *irq;
	struct acpi_drq *drq;
	ACPI_STATUS rv;

	printf("\n");

	sc->sc_ic = aa->aa_ic;

	/* parse resources */
	rv = acpi_resource_parse(&sc->sc_dev, aa->aa_node, &res,
	    &acpi_resource_parse_ops_default);
	if (rv != AE_OK) {
		printf("%s: unable to parse resources\n", sc->sc_dev.dv_xname);
		return;
	}

	/* find our i/o registers */
	io = acpi_res_io(&res, 0);
	if (io == NULL) {
		printf("%s: unable to find i/o register resource\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* find our IRQ */
	irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		printf("%s: unable to find irq resource\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* find our DRQ */
	drq = acpi_res_drq(&res, 0);
	if (drq == NULL) {
		printf("%s: unable to find drq resource\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_drq = drq->ar_drq;

	sc->sc_iot = aa->aa_iot;
	if (bus_space_map(sc->sc_iot, io->ar_base, io->ar_length,
		    0, &asc->sc_baseioh)) {
		printf("%s: can't map i/o space\n", sc->sc_dev.dv_xname);
		return;
	}

	switch (io->ar_length) {
	case 4:
		sc->sc_ioh = asc->sc_baseioh;
		break;
	case 6:
		if (bus_space_subregion(sc->sc_iot, asc->sc_baseioh, 2, 4,
		    &sc->sc_ioh)) {
			printf("%s: unable to subregion i/o space\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		break;
	default:
		printf("%s: unknown size: %d of io mapping\n",
		    sc->sc_dev.dv_xname, io->ar_length);
		return;
	}

	/*
	 * XXX: This is bad. If the pnpbios claims only 1 I/O range then it's
	 * omitting the controller I/O port. (One has to exist for there to
	 * be a working fdc). Just try and force the mapping in.
	 */
	ctlio = acpi_res_io(&res, 1);
	if (ctlio == NULL) {
		if (bus_space_map(sc->sc_iot, io->ar_base + io->ar_length + 1,
		    1, 0, &sc->sc_fdctlioh)) {
			printf("%s: unable to force map ctl i/o space\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		printf("%s: ctl io %x did't probe. Forced attach\n",
		    sc->sc_dev.dv_xname, io->ar_base + io->ar_length + 1);
	} else {
		if (bus_space_map(sc->sc_iot, ctlio->ar_base, ctlio->ar_length,
		    0, &sc->sc_fdctlioh)) {
			printf("%s: unable to map ctl i/o space\n",
			    sc->sc_dev.dv_xname);
			return;
		}
	}

	sc->sc_ih = isa_intr_establish(aa->aa_ic, irq->ar_irq, 0,
				       IPL_BIO, fdcintr, sc);

	fdcattach(sc);
}
