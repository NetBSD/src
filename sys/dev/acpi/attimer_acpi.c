/* $NetBSD: attimer_acpi.c,v 1.2.6.1 2006/04/22 11:38:46 simonb Exp $ */

/*
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juan Romero Pardines <xtraeme@NetBSD.org> and Quentin Garnier.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
 * ACPI attimer(4) attachment based on lpt_acpi.c by Jared D. McNeill.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: attimer_acpi.c,v 1.2.6.1 2006/04/22 11:38:46 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/bus.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/ic/attimervar.h>

static int	attimer_acpi_match(struct device *, struct cfdata *, void *);
static void	attimer_acpi_attach(struct device *, struct device *, void *);

CFATTACH_DECL(attimer_acpi, sizeof(struct attimer_softc), attimer_acpi_match,
    attimer_acpi_attach, NULL, NULL);

/*
 * Supported device IDs
 */

static const char * const attimer_acpi_ids[] = {
	"PNP0100",	/* AT Timer */
	NULL
};

/*
 * attimer_acpi_match: autoconf(9) match routine
 */
static int
attimer_acpi_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, attimer_acpi_ids);
}

/*
 * attimer_acpi_attach: autoconf(9) attach routine
 */
static void
attimer_acpi_attach(struct device *parent, struct device *self, void *aux)
{
	struct attimer_softc *sc = (struct attimer_softc *)self;
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	struct acpi_io *io;
	ACPI_STATUS rv;

	aprint_naive(": AT Timer\n");
	aprint_normal(": AT Timer\n");

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
		goto out;
	}

	sc->sc_iot = aa->aa_iot;
	if (bus_space_map(sc->sc_iot, io->ar_base, io->ar_length,
		    0, &sc->sc_ioh)) {
		aprint_error("%s: can't map i/o space\n", sc->sc_dev.dv_xname);
		goto out;
	}

	attimer_attach(sc);

 out:
	acpi_resource_cleanup(&res);
}
