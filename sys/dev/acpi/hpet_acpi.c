/* $NetBSD: hpet_acpi.c,v 1.3.18.1 2009/05/13 17:19:10 jym Exp $ */

/*
 * Copyright (c) 2006 Nicolas Joly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
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
__KERNEL_RCSID(0, "$NetBSD: hpet_acpi.c,v 1.3.18.1 2009/05/13 17:19:10 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <sys/time.h>
#include <sys/timetc.h>

#include <dev/ic/hpetvar.h>

static int	hpet_acpi_match(device_t, cfdata_t, void *);
static void	hpet_acpi_attach(device_t, device_t, void *);


CFATTACH_DECL_NEW(hpet_acpi, sizeof(struct hpet_softc), hpet_acpi_match,
    hpet_acpi_attach, NULL, NULL);

/*
 * Supported device IDs
 */

static const char * const hpet_acpi_ids[] = {
	"PNP0103",
	NULL
};

/*
 * hpet_acpi_match: autoconf(9) match routine
 */
static int
hpet_acpi_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, hpet_acpi_ids);
}

static void
hpet_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct hpet_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	struct acpi_mem *mem;
	ACPI_STATUS rv;

	/* parse resources */
	rv = acpi_resource_parse(self, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	/* find our mem registers */
	mem = acpi_res_mem(&res, 0);
	if (mem == NULL) {
		aprint_error_dev(self,
		    "unable to find mem register resource\n");
		goto out;
	}

	sc->sc_memt = aa->aa_memt;
	if (bus_space_map(sc->sc_memt, mem->ar_base, mem->ar_length,
		    0, &sc->sc_memh)) {
		aprint_error_dev(self, "can't map mem space\n");
		goto out;
	}

	hpet_attach_subr(self);

 out:
	acpi_resource_cleanup(&res);
}
