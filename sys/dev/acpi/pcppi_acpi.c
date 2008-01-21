/* $NetBSD: pcppi_acpi.c,v 1.1.8.4 2008/01/21 09:42:33 yamt Exp $ */

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
 * ACPI pcppi(4) attachment based on lpt_acpi.c by Jared D. McNeill.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcppi_acpi.c,v 1.1.8.4 2008/01/21 09:42:33 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <sys/bus.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/isa/pcppivar.h>

static int	pcppi_acpi_match(device_t, struct cfdata *, void *);
static void	pcppi_acpi_attach(device_t, device_t, void *);

struct pcppi_acpi_softc {
	struct pcppi_softc sc_pcppi;
};

CFATTACH_DECL(pcppi_acpi, sizeof(struct pcppi_acpi_softc), pcppi_acpi_match,
    pcppi_acpi_attach, pcppi_detach, NULL);

/*
 * Supported device IDs
 */

static const char * const pcppi_acpi_ids[] = {
	"PNP0800",	/* AT-style speaker sound */
	NULL
};

/*
 * pcppi_acpi_match: autoconf(9) match routine
 */
static int
pcppi_acpi_match(device_t parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, pcppi_acpi_ids);
}

/*
 * pcppi_acpi_attach: autoconf(9) attach routine
 */
static void
pcppi_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct pcppi_acpi_softc *asc = device_private(self);
	struct pcppi_softc *sc = &asc->sc_pcppi;
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	struct acpi_io *io;
	ACPI_STATUS rv;

	aprint_naive("\n");
	aprint_normal("\n");

	/* parse resources */
	rv = acpi_resource_parse(&sc->sc_dv, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	/* find our i/o registers */
	io = acpi_res_io(&res, 0);
	if (io == NULL) {
		aprint_error("%s: unable to find i/o register resource\n",
		    sc->sc_dv.dv_xname);
		goto out;
	}

	sc->sc_iot = aa->aa_iot;
	sc->sc_size = io->ar_length;
	if (bus_space_map(sc->sc_iot, io->ar_base, sc->sc_size,
		    0, &sc->sc_ppi_ioh)) {
		aprint_error("%s: can't map i/o space\n", sc->sc_dv.dv_xname);
		goto out;
	}

	pcppi_attach(sc);

 out:
	acpi_resource_cleanup(&res);
}
