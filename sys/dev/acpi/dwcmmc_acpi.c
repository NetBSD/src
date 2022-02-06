/* $NetBSD: dwcmmc_acpi.c,v 1.2 2022/02/06 15:48:12 jmcneill Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dwcmmc_acpi.c,v 1.2 2022/02/06 15:48:12 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/ic/dwc_mmc_var.h>                                                 
#include <dev/ic/dwc_mmc_reg.h>                                                 
#include <dev/sdmmc/sdmmcchip.h>                                                

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>

/*
 * FIXME: Remove separate compat_data arrays once acpi_compatible_lookup
 *	  grows support for DT link devices.
 */
static const struct device_compatible_entry rockchip_compat_data[] = {
	{ .compat = "rockchip,rk3288-dw-mshc" },
	DEVICE_COMPAT_EOL
};
static const struct device_compatible_entry compat_data[] = {
	{ .compat = "snps,dw-mshc" },
	DEVICE_COMPAT_EOL
};

static int	dwcmmc_acpi_match(device_t, cfdata_t, void *);
static void	dwcmmc_acpi_attach(device_t, device_t, void *);

static int	dwcmmc_acpi_init_props(struct dwc_mmc_softc *, ACPI_HANDLE);

CFATTACH_DECL_NEW(dwcmmc_acpi, sizeof(struct dwc_mmc_softc),
    dwcmmc_acpi_match, dwcmmc_acpi_attach, NULL, NULL);

static int
dwcmmc_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;
	int match;

	match = acpi_compatible_match(aa, rockchip_compat_data);
	if (!match) {
		match = acpi_compatible_match(aa, compat_data);
	}

	return match;
}

static void
dwcmmc_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct dwc_mmc_softc * const sc = device_private(self);
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
	if (ACPI_FAILURE(rv)) {
		return;
	}

	sc->sc_flags = DWC_MMC_F_USE_HOLD_REG | DWC_MMC_F_DMA;

	if (acpi_compatible_match(aa, rockchip_compat_data)) {
		sc->sc_ciu_div = 2;
		sc->sc_intr_cardmask = DWC_MMC_INT_SDIO_INT(8);
	}

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
	sc->sc_dmat = aa->aa_dmat;

	if (dwcmmc_acpi_init_props(sc, handle) != 0) {
		goto done;
	}

	if (dwc_mmc_init(sc) != 0) {
		goto done;
	}

        ih = acpi_intr_establish(self, (uint64_t)(uintptr_t)handle, IPL_NET,
	    false, dwc_mmc_intr, sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto done;
	}

done:
	acpi_resource_cleanup(&res);
}

static int
dwcmmc_acpi_init_props(struct dwc_mmc_softc *sc, ACPI_HANDLE handle)
{
	ACPI_INTEGER ival;
	bool bval;

	/* Defaults */
	sc->sc_fifo_depth = 0;
	sc->sc_clock_freq = UINT_MAX;
	sc->sc_bus_width = 4;

	/* Get settings from DSD properties */
	if (ACPI_SUCCESS(acpi_dsd_integer(handle, "fifo-depth", &ival))) {
		sc->sc_fifo_depth = ival;
	}
	if (ACPI_SUCCESS(acpi_dsd_integer(handle, "max-frequency", &ival))) {
		sc->sc_clock_freq = ival;
	}
	if (ACPI_SUCCESS(acpi_dsd_integer(handle, "bus-width", &ival))) {
		sc->sc_bus_width = ival;
	}
	bval = false;
	acpi_dsd_bool(handle, "non-removable", &bval);
	if (bval) {
		sc->sc_flags |= DWC_MMC_F_NON_REMOVABLE;
	}
	bval = false;
	acpi_dsd_bool(handle, "broken-cd", &bval);
	if (bval) {
		sc->sc_flags |= DWC_MMC_F_BROKEN_CD;
	}

	if (sc->sc_clock_freq == 0 || sc->sc_clock_freq == UINT_MAX) {
		aprint_error_dev(sc->sc_dev, "clock frequency not specified\n");
		return ENXIO;
	}

	return 0;
}
