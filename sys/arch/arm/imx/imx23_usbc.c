/*	$NetBSD: imx23_usbc.c,v 1.3 2026/02/02 06:23:37 skrll Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yuri Honegger.
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

/*
 * USB controller driver for the imx23.
 */

#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
/* must be after the other USB includes */
#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#include <arm/fdt/arm_fdtvar.h>
#include <arm/imx/imxusbvar.h>
#include <arm/imx/imxusbreg.h>
#include <arm/imx/imx23var.h>

struct imx23_imxusbc_softc {
	struct imxusbc_softc sc_imxusbc; /* Must be first */
	int sc_phandle;
};

static int imx23_usbc_match(device_t, cfdata_t, void *);
static void imx23_usbc_attach(device_t, device_t, void *);

static void imx23_usbc_init(struct imxehci_softc *, uintptr_t);
static void * imx23_usbc_intr_establish(struct imxehci_softc *, uintptr_t);

CFATTACH_DECL_NEW(imxusbc, sizeof(struct imx23_imxusbc_softc),
		  imx23_usbc_match, imx23_usbc_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx23-usb" },
	DEVICE_COMPAT_EOL
};

static int
imx23_usbc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx23_usbc_attach(device_t parent, device_t self, void *aux)
{
	struct imx23_imxusbc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_phandle = phandle;

	sc->sc_imxusbc.sc_dev = self;
	sc->sc_imxusbc.sc_iot = faa->faa_bst;

	bus_addr_t addr;
	bus_size_t size;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get register address\n");
		return;
	}
	if (bus_space_map(faa->faa_bst, addr, size, 0,
			  &sc->sc_imxusbc.sc_ioh)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	/* Enable external USB chip. */
	struct fdtbus_regulator *vbus_reg =
	    fdtbus_regulator_acquire(phandle, "vbus-supply");
	if(vbus_reg == NULL){
		aprint_error(": couldn't get vbus regulator\n");
		return;
	}
	fdtbus_regulator_enable(vbus_reg);

	sc->sc_imxusbc.sc_ehci_size = IMXUSB_EHCI_SIZE;
	sc->sc_imxusbc.sc_ehci_offset = IMXUSB_EHCI_SIZE;
	sc->sc_imxusbc.sc_init_md_hook = imx23_usbc_init;
	sc->sc_imxusbc.sc_intr_establish_md_hook = imx23_usbc_intr_establish;
	sc->sc_imxusbc.sc_setup_md_hook = NULL;

	aprint_normal("\n");

	/* attach OTG/EHCI host controllers */
	struct imxusbc_attach_args iaa;
	iaa.aa_iot = sc->sc_imxusbc.sc_iot;
	iaa.aa_ioh = sc->sc_imxusbc.sc_ioh;
	iaa.aa_dmat = faa->faa_dmat;
	iaa.aa_unit = 0; /* only one unit on the imx23 */
	iaa.aa_irq = -1; /* we establish it directly from FDT in the hook */
	config_found(self, &iaa, NULL, CFARGS_NONE);
}

static void
imx23_usbc_init(struct imxehci_softc *sc, uintptr_t data)
{
	sc->sc_iftype = IMXUSBC_IF_UTMI;
}

static void *
imx23_usbc_intr_establish(struct imxehci_softc *sc, uintptr_t data)
{
	struct imx23_imxusbc_softc *ifsc = (struct imx23_imxusbc_softc *)
					     sc->sc_usbc;
	ehci_softc_t *hsc = &sc->sc_hsc;
	void *ih;

	char intrstr[128];
	if (!fdtbus_intr_str(ifsc->sc_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(sc->sc_dev, "failed to decode interrupt\n");
		return NULL;
	}
	ih = fdtbus_intr_establish_xname(ifsc->sc_phandle, 0, IPL_USB,
					 FDT_INTR_MPSAFE, ehci_intr, hsc,
					 device_xname(sc->sc_dev));
	if (ih == NULL) {
		aprint_error_dev(sc->sc_dev,
				 "failed to establish interrupt on %s\n",
				 intrstr);
		return NULL;
	}
	aprint_normal_dev(sc->sc_dev, "interrupting on %s\n", intrstr);

	return ih;
}
