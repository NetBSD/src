/*	$NetBSD: imx6_usb.c,v 1.8 2023/05/04 17:09:45 bouyer Exp $	*/

/*-
 * Copyright (c) 2019 Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
__KERNEL_RCSID(0, "$NetBSD: imx6_usb.c,v 1.8 2023/05/04 17:09:45 bouyer Exp $");

#include "opt_fdt.h"

#include <locators.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#include <arm/nxp/imx6_usbreg.h>
#include <arm/imx/imxusbvar.h>

#include <dev/fdt/fdtvar.h>

struct imxusbc_fdt_softc {
	struct imxusbc_softc sc_imxusbc; /* Must be first */

	int sc_phandle;
};

static int imx6_usb_match(device_t, struct cfdata *, void *);
static void imx6_usb_attach(device_t, device_t, void *);
static int imx6_usb_init_clocks(struct imxusbc_softc *);
static void imx6_usb_init(struct imxehci_softc *, uintptr_t);
static void init_otg(struct imxehci_softc *);
static void init_h1(struct imxehci_softc *);
static int imxusbc_print(void *, const char *);
static void *imx6_usb_intr_establish(struct imxehci_softc *, uintptr_t);

/* attach structures */
CFATTACH_DECL_NEW(imxusbc_fdt, sizeof(struct imxusbc_fdt_softc),
    imx6_usb_match, imx6_usb_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx6q-usb", .value = 1 },
	{ .compat = "fsl,imx6sx-usb", .value = 2 },
	{ .compat = "fsl,imx7d-usb", .value = 1 },
	DEVICE_COMPAT_EOL
};

static int
imx6_usb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx6_usb_attach(device_t parent, device_t self, void *aux)
{
	struct imxusbc_fdt_softc *ifsc = device_private(self);
	struct imxusbc_softc *sc = &ifsc->sc_imxusbc;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_space_handle_t bsh;
	bus_addr_t addr;
	bus_size_t size;
	int error;
	struct fdtbus_regulator *reg;

	aprint_naive("\n");
	aprint_normal("\n");

	ifsc->sc_phandle = phandle;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get imxusbc registers\n");
		return;
	}

	error = bus_space_map(bst, addr, size, 0, &bsh);
	if (error) {
		aprint_error(": couldn't map %#" PRIxBUSADDR ": %d", addr, error);
		return;
	}

	sc->sc_clk = fdtbus_clock_get_index(phandle, 0);
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't get clock\n");
		return;
	}

	sc->sc_init_md_hook = imx6_usb_init;
	sc->sc_intr_establish_md_hook = imx6_usb_intr_establish;
	sc->sc_setup_md_hook = NULL;
	sc->sc_md_hook_data = of_compatible_lookup(phandle, compat_data)->value;

	sc->sc_dev = self;
	sc->sc_iot = bst;
	sc->sc_ioh = bsh;
	sc->sc_ehci_size = size;
	sc->sc_ehci_offset = 0;

	struct fdt_phandle_data data;
	error = fdtbus_get_phandle_with_data(phandle, "fsl,usbmisc",
	    "#index-cells", 0, &data);
	if (error) {
		aprint_error(": couldn't get usbmisc property\n");
		return;
	}
	int unit = be32toh(data.values[0]);

	if (fdtbus_get_reg(data.phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get usbmisc registers\n");
		return;
	}
	error = bus_space_map(bst, addr, size, 0, &bsh);
	if (error) {
		aprint_error(": couldn't map usbmisc registers: %d\n", error);
		return;
	}
	sc->sc_ioh_usbnc = bsh;

	if (imx6_usb_init_clocks(sc) != 0) {
		aprint_error_dev(self, "couldn't init clocks\n");
		return;
	}

	/* attach OTG/EHCI host controllers */
	struct imxusbc_attach_args iaa;
	iaa.aa_iot = sc->sc_iot;
	iaa.aa_ioh = sc->sc_ioh;
	iaa.aa_dmat = faa->faa_dmat;
	iaa.aa_unit = unit;
	iaa.aa_irq = IMXUSBCCF_IRQ_DEFAULT;
	config_found(self, &iaa, imxusbc_print,
	    CFARGS_NONE);

	if (of_hasprop(phandle, "vbus-supply")) {
		reg = fdtbus_regulator_acquire(phandle, "vbus-supply");
		if (reg == NULL) {
			aprint_error_dev(self,
			    "couldn't acquire vbus-supply\n");
		} else {
			error = fdtbus_regulator_enable(reg);
			if (error != 0) {
				aprint_error_dev(self,
				    "couldn't enable vbus-supply\n");
			}
		}
	} else {
		aprint_verbose_dev(self, "no regulator\n");
	}
	return;
}

static int
imxusbc_print(void *aux, const char *name __unused)
{
	struct imxusbc_attach_args *iaa;

	iaa = (struct imxusbc_attach_args *)aux;

	aprint_normal(" unit %d", iaa->aa_unit);
	return UNCONF;
}


static int
imx6_usb_init_clocks(struct imxusbc_softc *sc)
{
	int error;

	error = clk_enable(sc->sc_clk);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable: %d\n", error);
		return error;
	}

	return 0;
}

static void
imx6_usb_init(struct imxehci_softc *sc, uintptr_t data)
{
	int notg = data;

	switch (sc->sc_unit) {
	case 0:	/* OTG controller */
		init_otg(sc);
		break;
	case 1:	/* EHCI Host 1 */
		if (notg >= 2)
			init_otg(sc);
		else
			init_h1(sc);
		break;
	case 2:	/* EHCI Host 2 */
	case 3:	/* EHCI Host 3 */
	default:
		aprint_error_dev(sc->sc_dev, "unit %d not supported\n",
		    sc->sc_unit);
	}
}

static void
init_otg(struct imxehci_softc *sc)
{
	struct imxusbc_softc *usbc = sc->sc_usbc;
	uint32_t v;

	sc->sc_iftype = IMXUSBC_IF_UTMI_WIDE;

	imxehci_reset(sc);

	v = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh_usbnc, USBNC_USB_OTG_CTRL);
	v |= USBNC_USB_OTG_CTRL_WKUP_VBUS_EN;
	v |= USBNC_USB_OTG_CTRL_OVER_CUR_DIS;
	v |= USBNC_USB_OTG_CTRL_PWR_POL;
	v &= ~USBNC_USB_OTG_CTRL_UTMI_ON_CLOCK;
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh_usbnc, USBNC_USB_OTG_CTRL, v);
}

static void
init_h1(struct imxehci_softc *sc)
{
	struct imxusbc_softc *usbc = sc->sc_usbc;
	uint32_t v;

	sc->sc_iftype = IMXUSBC_IF_UTMI_WIDE;

	v = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh_usbnc, USBNC_USB_UH1_CTRL);
	v |= USBNC_USB_UH1_CTRL_OVER_CUR_POL;
	v |= USBNC_USB_UH1_CTRL_OVER_CUR_DIS;
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh_usbnc, USBNC_USB_UH1_CTRL, v);

	/* do reset */
	imxehci_reset(sc);

	/* set mode */
	v = bus_space_read_4(usbc->sc_iot, usbc->sc_ioh, USBC_UH1_USBMODE);
	v &= ~USBC_UH_USBMODE_CM;
	v |= __SHIFTIN(USBC_UH_USBMODE_CM, USBC_UH_USBMODE_CM_HOST_CONTROLLER);
	bus_space_write_4(usbc->sc_iot, usbc->sc_ioh, USBC_UH1_USBMODE, v);
}

static void *
imx6_usb_intr_establish(struct imxehci_softc *sc, uintptr_t data)
{
	struct imxusbc_fdt_softc *ifsc = (struct imxusbc_fdt_softc *)sc->sc_usbc;
	ehci_softc_t *hsc = &sc->sc_hsc;
	void *ih;

	char intrstr[128];
	if (!fdtbus_intr_str(ifsc->sc_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(sc->sc_dev, "failed to decode interrupt\n");
		return NULL;
	}
	ih = fdtbus_intr_establish_xname(ifsc->sc_phandle, 0, IPL_USB,
	    FDT_INTR_MPSAFE, ehci_intr, hsc, device_xname(sc->sc_dev));
	if (ih == NULL) {
		aprint_error_dev(sc->sc_dev, "failed to establish interrupt on %s\n",
		    intrstr);
		return NULL;
	}
	aprint_normal_dev(sc->sc_dev, "interrupting on %s\n", intrstr);

	return ih;
}
