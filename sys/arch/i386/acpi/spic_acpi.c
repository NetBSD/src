/*	$NetBSD: spic_acpi.c,v 1.11 2004/04/11 10:36:45 kochi Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net).
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
__KERNEL_RCSID(0, "$NetBSD: spic_acpi.c,v 1.11 2004/04/11 10:36:45 kochi Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/callout.h>

#include <machine/bus.h>

#include <dev/ic/spicvar.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>

struct spic_acpi_softc {
	struct spic_softc sc_spic;	/* spic device */

	struct acpi_devnode *sc_node;	/* our ACPI devnode */

	void *sc_ih;
};

static const char * const spic_acpi_ids[] = {
	"SNY6001",
	NULL
};

int	spic_acpi_match(struct device *, struct cfdata *, void *);
void	spic_acpi_attach(struct device *, struct device *, void *);

CFATTACH_DECL(spic_acpi, sizeof(struct spic_acpi_softc),
    spic_acpi_match, spic_acpi_attach, NULL, NULL);


int
spic_acpi_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return (0);

	return (acpi_match_hid(aa->aa_node->ad_devinfo, spic_acpi_ids));
}

void
spic_acpi_attach(struct device *parent, struct device *self, void *aux)
{
	struct spic_acpi_softc *sc = (void *) self;
	struct acpi_attach_args *aa = aux;
	struct acpi_io *io;
	struct acpi_irq *irq;
	struct acpi_resources res;

	ACPI_STATUS rv;

	printf(": Sony Programmable I/O Controller\n");

	sc->sc_node = aa->aa_node;

	/* Parse our resources. */
	rv = acpi_resource_parse(&sc->sc_spic.sc_dev, sc->sc_node->ad_handle,
	    "_CRS", &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	sc->sc_spic.sc_iot = aa->aa_iot;
	io = acpi_res_io(&res, 0);
	if (io == NULL) {
		printf("%s: unable to find io resource\n",
		    sc->sc_spic.sc_dev.dv_xname);
		goto out;
	}
	if (bus_space_map(sc->sc_spic.sc_iot, io->ar_base, io->ar_length,
	    0, &sc->sc_spic.sc_ioh) != 0) {
		printf("%s: unable to map data register\n",
		    sc->sc_spic.sc_dev.dv_xname);
		goto out;
	}
	irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		printf("%s: unable to find irq resource\n",
		    sc->sc_spic.sc_dev.dv_xname);
		/* XXX unmap */
		goto out;
	}
#if 0
	sc->sc_ih = isa_intr_establish(NULL, irq->ar_irq,
	    IST_EDGE, IPL_TTY, spic_intr, sc);
#endif

	spic_attach(&sc->sc_spic);
 out:
	acpi_resource_cleanup(&res);
}
