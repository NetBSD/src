/* $NetBSD: dwcwdt_acpi.c,v 1.1 2023/04/16 16:55:01 jmcneill Exp $ */

/*-
 * Copyright (c) 2023 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: dwcwdt_acpi.c,v 1.1 2023/04/16 16:55:01 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/wdog.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/ic/dwc_wdt_var.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "snps,dw-wdt" },
	DEVICE_COMPAT_EOL
};

static int
dwcwdt_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
dwcwdt_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct dwcwdt_softc * const sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	ACPI_HANDLE handle = aa->aa_node->ad_handle;
	struct acpi_resources res;
	struct acpi_mem *mem;
	ACPI_INTEGER ival;
	ACPI_STATUS rv;

	rv = acpi_resource_parse(sc->sc_dev, handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv)) {
		return;
	}

	mem = acpi_res_mem(&res, 0);
	if (mem == NULL) {
		aprint_error_dev(self, "couldn't find mem resource\n");
		goto done;
	}

	sc->sc_dev = self;
	sc->sc_bst = aa->aa_memt;
	if (bus_space_map(sc->sc_bst, mem->ar_base, mem->ar_length,
	    0, &sc->sc_bsh) != 0) {
		aprint_error_dev(self, "couldn't map registers\n");
		goto done;
	}
	if (ACPI_SUCCESS(acpi_dsd_integer(handle, "clock-frequency", &ival))) {
		sc->sc_clkrate = ival;
	}

	dwcwdt_init(sc);

done:
	acpi_resource_cleanup(&res);
}

CFATTACH_DECL_NEW(dwcwdt_acpi, sizeof(struct dwcwdt_softc),
    dwcwdt_acpi_match, dwcwdt_acpi_attach, NULL, NULL);
