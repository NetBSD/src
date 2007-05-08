/* $NetBSD: ug_acpi.c,v 1.2 2007/05/08 17:17:14 xtraeme Exp $ */

/*
 * Copyright (c) 2007 Mihai Chelaru <kefren@netbsd.ro>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ug_acpi.c,v 1.2 2007/05/08 17:17:14 xtraeme Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/envsys.h>

#include <machine/bus.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/ic/ugreg.h>
#include <dev/ic/ugvar.h>

/* autoconf(9) functions */
static int	ug_acpi_match(struct device *, struct cfdata *, void *);
static void	ug_acpi_attach(struct device *, struct device *, void *);

CFATTACH_DECL(ug_acpi, sizeof(struct ug_softc), ug_acpi_match,
    ug_acpi_attach, NULL, NULL);

/*
 * Supported devices
 * XXX: only uGuru 2005 for now
 */

static const char* const ug_acpi_ids[] = {
	"ABT2005",	/* uGuru 2005 */
	NULL
};

static int
ug_acpi_match(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, ug_acpi_ids);
}

static void
ug_acpi_attach(struct device *parent, struct device *self, void *aux)
{
	struct ug_softc *sc = (struct ug_softc*)self;
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	struct acpi_io *io;
	bus_space_handle_t ioh;
	ACPI_STATUS rv;

	aprint_naive("\n");
	aprint_normal("\n");

	/* parse resources */
	rv = acpi_resource_parse(&sc->sc_dev, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	/* find our i/o registers */
	io = acpi_res_io(&res, 0);
	if (io == NULL) {
		aprint_error("%s: unable to find i/o register resource\n",
		    sc->sc_dev.dv_xname);
		acpi_resource_cleanup(&res);
		return;
	}

	if (bus_space_map(aa->aa_iot, io->ar_base, io->ar_length,
	    0, &ioh)) {
		aprint_error("%s: can't map i/o space\n",
		    sc->sc_dev.dv_xname);
		acpi_resource_cleanup(&res);
		return;
	}

	aprint_normal("%s", sc->sc_dev.dv_xname);

	sc->version = 2;	/* uGuru 2005 */
	sc->sc_ioh = ioh;
	sc->sc_iot = aa->aa_iot;
	ug2_attach(sc);

	acpi_resource_cleanup(&res);
}
