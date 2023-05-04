/*	$NetBSD: imx6_pcie.c,v 1.7 2023/05/04 13:29:33 bouyer Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: imx6_pcie.c,v 1.7 2023/05/04 13:29:33 bouyer Exp $");

#include "opt_pci.h"
#include "opt_fdt.h"

#include "pci.h"
#include "locators.h"

#define	_INTR_PRIVATE

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <machine/frame.h>
#include <arm/cpufunc.h>

#include <dev/fdt/fdtvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <arm/imx/imxpcievar.h>
#include <arm/imx/imxgpioreg.h>
#include <arm/imx/imxgpiovar.h>
#include <arm/nxp/imx6_iomuxreg.h>
#include <arm/nxp/imx6_ccmreg.h>
#include <arm/nxp/imx6_ccmvar.h>

struct imxpcie_fdt_softc {
	struct imxpcie_softc sc_imxpcie;

	struct fdtbus_gpio_pin	*sc_pin_reset;
	struct fdtbus_regulator	*sc_reg_vpcie;
};

static int imx6_pcie_match(device_t, cfdata_t, void *);
static void imx6_pcie_attach(device_t, device_t, void *);

static void imx6_pcie_configure(void *);
static uint32_t imx6_pcie_gpr_read(void *, uint32_t);
static void imx6_pcie_gpr_write(void *, uint32_t, uint32_t);
static void imx6_pcie_reset(void *);

#define IMX6_PCIE_MEM_BASE	0x01000000
#define IMX6_PCIE_MEM_SIZE	0x00f00000 /* 15MB */
#define IMX6_PCIE_ROOT_BASE	0x01f00000
#define IMX6_PCIE_ROOT_SIZE	0x00080000 /* 512KB */
#define IMX6_PCIE_IO_BASE	0x01f80000
#define IMX6_PCIE_IO_SIZE	0x00010000 /* 64KB */

CFATTACH_DECL_NEW(imxpcie_fdt, sizeof(struct imxpcie_fdt_softc),
    imx6_pcie_match, imx6_pcie_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx6q-pcie",	.value = false },
	{ .compat = "fsl,imx6qp-pcie",	.value = true },
	{ .compat = "fsl,imx6sx-pcie",	.value = true },
	DEVICE_COMPAT_EOL
};

static int
imx6_pcie_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx6_pcie_attach(device_t parent, device_t self, void *aux)
{
	struct imxpcie_fdt_softc * const ifsc = device_private(self);
	struct imxpcie_softc * const sc = &ifsc->sc_imxpcie;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;

	aprint_naive("\n");
	aprint_normal(": PCI Express Controller\n");

	sc->sc_dev = self;
	sc->sc_iot = bst;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_cookie = ifsc;
	sc->sc_pci_netbsd_configure = imx6_pcie_configure;
	sc->sc_gpr_read = imx6_pcie_gpr_read;
	sc->sc_gpr_write = imx6_pcie_gpr_write;
	sc->sc_reset = imx6_pcie_reset;
	sc->sc_have_sw_reset =
	    (bool)of_compatible_lookup(phandle, compat_data)->value;

	if (fdtbus_get_reg_byname(phandle, "dbi", &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh)) {
		aprint_error_dev(self, "Cannot map registers\n");
		return;
	}
	if (fdtbus_get_reg_byname(phandle, "config", &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	sc->sc_root_addr = addr;
	sc->sc_root_size = size;

	const int gpr_phandle = OF_finddevice("/soc/aips-bus/iomuxc-gpr");
	fdtbus_get_reg(gpr_phandle, 0, &addr, &size);
	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_gpr_ioh)) {
		aprint_error_dev(self, "Cannot map registers\n");
		return;
	}

	ifsc->sc_pin_reset = fdtbus_gpio_acquire(phandle, "reset-gpio",
	    GPIO_PIN_OUTPUT);
	if (!ifsc->sc_pin_reset) {
		aprint_error(": couldn't acquire reset gpio\n");
		return;
	}

	sc->sc_clk_pcie = fdtbus_clock_get(phandle, "pcie");
	if (sc->sc_clk_pcie == NULL) {
		aprint_error(": couldn't get clock pcie_axi\n");
		return;
	}
	sc->sc_clk_pcie_bus = fdtbus_clock_get(phandle, "pcie_bus");
	if (sc->sc_clk_pcie_bus == NULL) {
		aprint_error(": couldn't get clock lvds1_gate\n");
		return;
	}
	sc->sc_clk_pcie_phy = fdtbus_clock_get(phandle, "pcie_phy");
	if (sc->sc_clk_pcie_phy == NULL) {
		aprint_error(": couldn't get clock pcie_ref\n");
		return;
	}

	if (of_hasprop(phandle, "vpcie-supply")) {
		ifsc->sc_reg_vpcie = fdtbus_regulator_acquire(phandle, "vpcie-supply");
		if (ifsc->sc_reg_vpcie == NULL) {
			aprint_error(": couldn't acquire regulator\n");
			return;
		}
		aprint_normal_dev(self, "regulator On\n");
		fdtbus_regulator_enable(ifsc->sc_reg_vpcie);
	}

	if (of_hasprop(phandle, "ext_osc")) {
		aprint_normal_dev(self, "Use external OSC\n");
		sc->sc_ext_osc = true;

		sc->sc_clk_pcie_ext = fdtbus_clock_get(phandle, "pcie_ext");
		if (sc->sc_clk_pcie_ext == NULL) {
			aprint_error(": couldn't get clock pcie_ext\n");
			return;
		}
		sc->sc_clk_pcie_ext_src = fdtbus_clock_get(phandle, "pcie_ext_src");
		if (sc->sc_clk_pcie_ext_src == NULL) {
			aprint_error(": couldn't get clock pcie_ext_src\n");
			return;
		}
	} else {
		sc->sc_ext_osc = false;
		sc->sc_clk_pcie_ext = NULL;
		sc->sc_clk_pcie_ext_src = NULL;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_VM,
	    FDT_INTR_MPSAFE, imxpcie_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	imxpcie_attach_common(sc);
}

static void
imx6_pcie_configure(void *cookie)
{
	struct imxpcie_fdt_softc * const ifsc = cookie;
	struct imxpcie_softc * const sc = &ifsc->sc_imxpcie;

#ifdef PCI_NETBSD_CONFIGURE
	struct pciconf_resources *pcires = pciconf_resource_init();

	pciconf_resource_add(pcires, PCICONF_RESOURCE_IO,
	    IMX6_PCIE_IO_BASE, IMX6_PCIE_IO_SIZE);
	pciconf_resource_add(pcires, PCICONF_RESOURCE_MEM,
	    IMX6_PCIE_MEM_BASE, IMX6_PCIE_MEM_SIZE);

	int error = pci_configure_bus(&sc->sc_pc, pcires, 0, arm_dcache_align);

	pciconf_resource_fini(pcires);

	if (error) {
		aprint_error_dev(sc->sc_dev, "configuration failed (%d)\n",
		    error);
	}
#endif
}

static uint32_t
imx6_pcie_gpr_read(void *cookie, uint32_t reg)
{
	struct imxpcie_fdt_softc * const ifsc = cookie;
	struct imxpcie_softc * const sc = &ifsc->sc_imxpcie;
	return bus_space_read_4(sc->sc_iot, sc->sc_gpr_ioh, reg);
}

static void
imx6_pcie_gpr_write(void *cookie, uint32_t reg, uint32_t val)
{
	struct imxpcie_fdt_softc * const ifsc = cookie;
	struct imxpcie_softc * const sc = &ifsc->sc_imxpcie;
	bus_space_write_4(sc->sc_iot, sc->sc_gpr_ioh, reg, val);
}

static void
imx6_pcie_reset(void *cookie)
{
	struct imxpcie_fdt_softc * const ifsc = cookie;

	fdtbus_gpio_write(ifsc->sc_pin_reset, 1);
	delay(20 * 1000);
	fdtbus_gpio_write(ifsc->sc_pin_reset, 0);
}
