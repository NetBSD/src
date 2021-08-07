/* $NetBSD: xhci_acpi.c,v 1.13 2021/08/07 16:19:09 thorpej Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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
__KERNEL_RCSID(0, "$NetBSD: xhci_acpi.c,v 1.13 2021/08/07 16:19:09 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/xhcireg.h>
#include <dev/usb/xhcivar.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>
#include <dev/acpi/acpi_usb.h>

static const struct device_compatible_entry compat_data[] = {
	/* XHCI-compliant USB controller without standard debug */
	{ .compat = "PNP0D10" },

	/* XHCI-compliant USB controller with standard debug */
	{ .compat = "PNP0D15" },

	/* DesignWare Dual Role SuperSpeed USB controller */
	{ .compat = "808622B7", },

	DEVICE_COMPAT_EOL
};

struct xhci_acpi_softc {
	struct xhci_softc	sc_xhci;
	ACPI_HANDLE		sc_handle;
};

static int	xhci_acpi_match(device_t, cfdata_t, void *);
static void	xhci_acpi_attach(device_t, device_t, void *);

static void	xhci_acpi_init(struct xhci_softc *);

CFATTACH_DECL2_NEW(xhci_acpi, sizeof(struct xhci_acpi_softc),
	xhci_acpi_match, xhci_acpi_attach, NULL,
	xhci_activate, NULL, xhci_childdet);

static int
xhci_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
xhci_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct xhci_acpi_softc * const asc = device_private(self);
	struct xhci_softc * const sc = &asc->sc_xhci;
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	uint32_t hccparams;
	ACPI_STATUS rv;
	int error;
	void *ih;

	asc->sc_handle = aa->aa_node->ad_handle;

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_revision = USBREV_3_0;
	sc->sc_quirks = 0;
	sc->sc_vendor_init = xhci_acpi_init;

	rv = acpi_resource_parse(sc->sc_dev, asc->sc_handle, "_CRS",
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

	sc->sc_ios = mem->ar_length;
	sc->sc_iot = aa->aa_memt;
	error = bus_space_map(sc->sc_iot, mem->ar_base, mem->ar_length, 0, &sc->sc_ioh);
	if (error) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}

	hccparams = bus_space_read_4(sc->sc_iot, sc->sc_ioh, XHCI_HCCPARAMS);
	if (XHCI_HCC_AC64(hccparams)) {
		aprint_verbose_dev(self, "64-bit DMA");
		if (BUS_DMA_TAG_VALID(aa->aa_dmat64)) {
			aprint_verbose("\n");
			sc->sc_bus.ub_dmatag = aa->aa_dmat64;
		} else {
			aprint_verbose(" - limited\n");
			sc->sc_bus.ub_dmatag = aa->aa_dmat;
		}
	} else {
		aprint_verbose_dev(self, "32-bit DMA\n");
		sc->sc_bus.ub_dmatag = aa->aa_dmat;
	}

	ih = acpi_intr_establish(self,
	    (uint64_t)(uintptr_t)aa->aa_node->ad_handle,
	    IPL_USB, true, xhci_intr, sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		return;
	}

	error = xhci_init(sc);
	if (error) {
		aprint_error_dev(self, "init failed, error = %d\n", error);
		return;
	}

	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint, CFARGS_NONE);
	sc->sc_child2 = config_found(self, &sc->sc_bus2, usbctlprint,
	    CFARGS_NONE);

done:
	acpi_resource_cleanup(&res);
}

static void
xhci_acpi_init(struct xhci_softc *sc)
{
	struct xhci_acpi_softc * const asc = (struct xhci_acpi_softc *)sc;

	acpi_usb_post_reset(asc->sc_handle);
}
