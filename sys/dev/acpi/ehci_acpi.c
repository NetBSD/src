/* $NetBSD: ehci_acpi.c,v 1.8 2021/12/22 21:45:02 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ehci_acpi.c,v 1.8 2021/12/22 21:45:02 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>
#include <dev/acpi/acpi_usb.h>

static const struct device_compatible_entry compat_data[] = {
	/* EHCI-compliant USB controller without standard debug */
	{ .compat = "PNP0D20" },

	/* EHCI-compliant USB controller with standard debug */
	{ .compat = "PNP0D25" },

	DEVICE_COMPAT_EOL
};

struct ehci_acpi_softc {
	struct ehci_softc	sc_ehci;
	ACPI_HANDLE		sc_handle;
};

static int	ehci_acpi_match(device_t, cfdata_t, void *);
static void	ehci_acpi_attach(device_t, device_t, void *);

static void	ehci_acpi_init(struct ehci_softc *);

CFATTACH_DECL2_NEW(ehci_acpi, sizeof(struct ehci_acpi_softc),
	ehci_acpi_match, ehci_acpi_attach, NULL,
	ehci_activate, NULL, ehci_childdet);

static int
ehci_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
ehci_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct ehci_acpi_softc * const asc = device_private(self);
	struct ehci_softc * const sc = &asc->sc_ehci;
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	ACPI_STATUS rv;
	int error;
	void *ih;

	asc->sc_handle = aa->aa_node->ad_handle;

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_revision = USBREV_2_0;
	sc->sc_flags = EHCIF_ETTF;
	sc->sc_vendor_init = ehci_acpi_init;

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

	sc->sc_size = mem->ar_length;
	sc->iot = aa->aa_memt;
	error = bus_space_map(sc->iot, mem->ar_base, mem->ar_length, 0, &sc->ioh);
	if (error) {
		aprint_error_dev(self, "couldn't map registers\n");
		goto done;
	}

	/* Disable interrupts */
	sc->sc_offs = EREAD1(sc, EHCI_CAPLENGTH);
	EOWRITE4(sc, EHCI_USBINTR, 0);

	const uint32_t hccparams = EREAD4(sc, EHCI_HCCPARAMS);
	if (EHCI_HCC_64BIT(hccparams)) {
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
	    IPL_USB, true, ehci_intr, sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto done;
	}

	error = ehci_init(sc);
	if (error) {
		aprint_error_dev(self, "init failed, error = %d\n", error);
		acpi_intr_disestablish(ih);
		goto done;
	}

	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint, CFARGS_NONE);

done:
	acpi_resource_cleanup(&res);
}

static void
ehci_acpi_init(struct ehci_softc *sc)
{
	struct ehci_acpi_softc * const asc = (struct ehci_acpi_softc *)sc;

	acpi_usb_post_reset(asc->sc_handle);
}
