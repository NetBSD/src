/*	$NetBSD: imx6_pcie.c,v 1.3 2019/08/19 03:45:51 hkenken Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: imx6_pcie.c,v 1.3 2019/08/19 03:45:51 hkenken Exp $");

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
#include <sys/extent.h>
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
#include <arm/imx/imx6var.h>
#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6_iomuxreg.h>
#include <arm/imx/imx6_ccmreg.h>
#include <arm/imx/imx6_ccmvar.h>

struct imxpcie_fdt_softc {
	struct imxpcie_softc sc_imxpcie;

	struct fdtbus_gpio_pin	*sc_pin_reset;
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

static const char * const compatible[] = {
	"fsl,imx6q-pcie",
	NULL
};

static int
imx6_pcie_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
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

	sc->sc_clk_pcie_axi = fdtbus_clock_get(phandle, "pcie");
	if (sc->sc_clk_pcie_axi == NULL) {
		aprint_error(": couldn't get clock pcie_axi\n");
		return;
	}
	sc->sc_clk_lvds1_gate = fdtbus_clock_get(phandle, "pcie_bus");
	if (sc->sc_clk_lvds1_gate == NULL) {
		aprint_error(": couldn't get clock lvds1_gate\n");
		return;
	}
	sc->sc_clk_pcie_ref = fdtbus_clock_get(phandle, "pcie_phy");
	if (sc->sc_clk_pcie_ref == NULL) {
		aprint_error(": couldn't get clock pcie_ref\n");
		return;
	}

	TAILQ_INIT(&sc->sc_intrs);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_VM,
	    FDT_INTR_MPSAFE, imxpcie_intr, sc);
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
	struct extent *ioext, *memext;
	int error;

	ioext = extent_create("pciio", IMX6_PCIE_IO_BASE,
	    IMX6_PCIE_IO_BASE + IMX6_PCIE_IO_SIZE - 1,
	    NULL, 0, EX_NOWAIT);
	memext = extent_create("pcimem", IMX6_PCIE_MEM_BASE,
	    IMX6_PCIE_MEM_BASE + IMX6_PCIE_MEM_SIZE - 1,
	    NULL, 0, EX_NOWAIT);

	error = pci_configure_bus(&sc->sc_pc, ioext, memext, NULL, 0,
	    arm_dcache_align);

	extent_destroy(ioext);
	extent_destroy(memext);

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
