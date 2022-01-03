/* $NetBSD: eqos_acpi.c,v 1.1 2022/01/03 17:19:41 jmcneill Exp $ */

/*-
 * Copyright (c) 2022 Jared McNeill <jmcneill@invisible.ca>
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

#include "opt_net_mpsafe.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: eqos_acpi.c,v 1.1 2022/01/03 17:19:41 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <sys/rndsource.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include <dev/ic/dwc_eqos_var.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>

#ifdef NET_MPSAFE
#define	EQOS_INTR_MPSAFE	true
#else
#define	EQOS_INTR_MPSAFE	false
#endif

#define	CSR_RATE_RGMII		125000000

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "snps,dwmac-4.20a" },	/* DT link */
	DEVICE_COMPAT_EOL
};

static int	eqos_acpi_match(device_t, cfdata_t, void *);
static void	eqos_acpi_attach(device_t, device_t, void *);

static void	eqos_acpi_init_props(struct eqos_softc *, ACPI_HANDLE);

CFATTACH_DECL_NEW(eqos_acpi, sizeof(struct eqos_softc),
    eqos_acpi_match, eqos_acpi_attach, NULL, NULL);

static int
eqos_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
eqos_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct eqos_softc * const sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	ACPI_HANDLE handle = aa->aa_node->ad_handle;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	ACPI_STATUS rv;
	int error;
	void *ih;

	sc->sc_dev = self;

	rv = acpi_resource_parse(sc->sc_dev, handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	mem = acpi_res_mem(&res, 0);
	if (mem == NULL) {
		aprint_error_dev(self, "couldn't find mem resource\n");
		goto done;
	}

	irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		aprint_error_dev(self, "couldn't find irq resource\n");
		goto done;
	}

	sc->sc_bst = aa->aa_memt;
	error = bus_space_map(sc->sc_bst, mem->ar_base, mem->ar_length,
	    0, &sc->sc_bsh);
	if (error != 0) {
		aprint_error_dev(self, "couldn't map registers\n");
		goto done;
	}
	sc->sc_dmat = BUS_DMA_TAG_VALID(aa->aa_dmat64) ?
	    aa->aa_dmat64 : aa->aa_dmat;
	sc->sc_phy_id = MII_PHY_ANY;
	sc->sc_csr_clock = CSR_RATE_RGMII;

	eqos_acpi_init_props(sc, handle);

	aprint_normal("%s", device_xname(self));
	if (eqos_attach(sc) != 0)
		goto done;

        ih = acpi_intr_establish(self, (uint64_t)(uintptr_t)handle, IPL_NET,
	    EQOS_INTR_MPSAFE, eqos_intr, sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto done;
	}

done:
	acpi_resource_cleanup(&res);
}

static void
eqos_acpi_init_props(struct eqos_softc *sc, ACPI_HANDLE handle)
{
	prop_dictionary_t prop = device_properties(sc->sc_dev);
	ACPI_STATUS rv;
	ACPI_INTEGER ival;
#if notyet
	ACPI_HANDLE axi;
	char *sval;
#endif

	/* Defaults */
	prop_dictionary_set_uint(prop, "snps,wr_osr_lmt", 4);
	prop_dictionary_set_uint(prop, "snps,rd_osr_lmt", 8);

	/* Get settings from DSD properties */
	rv = acpi_dsd_integer(handle, "snps,mixed-burst", &ival);
	if (ACPI_SUCCESS(rv) && ival) {
		prop_dictionary_set_bool(prop, "snps,mixed-burst", true);
	}
	rv = acpi_dsd_integer(handle, "snps,tso", &ival);
	if (ACPI_SUCCESS(rv) && ival) {
		prop_dictionary_set_bool(prop, "snps,tso", true);
	}

#if notyet
	rv = acpi_dsd_string(handle, "snps,axi-config", &sval);
	if (ACPI_SUCCESS(rv)) {
		rv = AcpiGetHandle(handle, sval, &axi);
		if (ACPI_SUCCESS(rv)) {
			rv = acpi_dsd_integer(axi, "snps,wr_osr_lmt", &ival);
			if (ACPI_SUCCESS(rv) && ival > 0) {
				prop_dictionary_set_uint(prop,
				    "snps,wr_osr_lmt", ival);
			}
			rv = acpi_dsd_integer(axi, "snps,rd_osr_lmt", &ival);
			if (ACPI_SUCCESS(rv) && ival > 0) {
				prop_dictionary_set_uint(prop,
				    "snps,rd_osr_lmt", ival);
			}
		}
		kmem_strfree(sval);
	}
#endif
}
