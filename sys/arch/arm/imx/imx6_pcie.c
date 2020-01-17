/*	$NetBSD: imx6_pcie.c,v 1.14.2.1 2020/01/17 21:47:24 ad Exp $	*/

/*
 * Copyright (c) 2016  Genetec Corporation.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * i.MX6 On-Chip PCI Express Controller
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx6_pcie.c,v 1.14.2.1 2020/01/17 21:47:24 ad Exp $");

#include "opt_pci.h"

#include "locators.h"
#include "imxgpio.h"
#include "pci.h"

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

struct imx6_pcie_softc {
	struct imxpcie_softc sc_imxpcie;

	int32_t sc_gpio_reset;
	int32_t sc_gpio_reset_active;
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

CFATTACH_DECL_NEW(imx6_pcie, sizeof(struct imx6_pcie_softc),
    imx6_pcie_match, imx6_pcie_attach, NULL, NULL);

static int
imx6_pcie_match(device_t parent, cfdata_t cf, void *aux)
{
	struct axi_attach_args * const aa = aux;

	/* i.MX6 SoloLite/UltraLight has no PCIe controller */
	switch (IMX6_CHIPID_MAJOR(imx6_chip_id())) {
	case CHIPID_MAJOR_IMX6SL:
	case CHIPID_MAJOR_IMX6UL:
		return 0;
	default:
		break;
	}

	switch (aa->aa_addr) {
	case (IMX6_PCIE_BASE):
		return 1;
	default:
		break;
	}

	return 0;
}

static void
imx6_pcie_attach(device_t parent, device_t self, void *aux)
{
	struct imx6_pcie_softc * const ipsc = device_private(self);
	struct imxpcie_softc * const sc = &ipsc->sc_imxpcie;
	struct axi_attach_args * const aa = aux;

	aprint_naive("\n");
	aprint_normal(": PCI Express Controller\n");

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->sc_dmat = aa->aa_dmat;
	sc->sc_root_addr = IMX6_PCIE_ROOT_BASE;
	sc->sc_root_size = IMX6_PCIE_ROOT_SIZE;
	sc->sc_cookie = ipsc;
	sc->sc_pci_netbsd_configure = imx6_pcie_configure;
	sc->sc_gpr_read = imx6_pcie_gpr_read;
	sc->sc_gpr_write = imx6_pcie_gpr_write;
	sc->sc_reset = imx6_pcie_reset;

	if (aa->aa_size == AXICF_SIZE_DEFAULT)
		aa->aa_size = IMX6_PCIE_SIZE;

	if (bus_space_map(sc->sc_iot, aa->aa_addr, aa->aa_size, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(self, "Cannot map registers\n");
		return;
	}

	imx6_set_gpio(self, "imxpcie-reset-gpio", &ipsc->sc_gpio_reset,
	    &ipsc->sc_gpio_reset_active, GPIO_PIN_OUTPUT);

	sc->sc_clk_pcie = imx6_get_clock("pcie_axi");
	if (sc->sc_clk_pcie == NULL) {
		aprint_error(": couldn't get clock pcie\n");
		return;
	}
	sc->sc_clk_pcie_bus = imx6_get_clock("lvds1_gate");
	if (sc->sc_clk_pcie_bus == NULL) {
		aprint_error(": couldn't get clock pcie_bus\n");
		return;
	}
	sc->sc_clk_pcie_phy = imx6_get_clock("pcie_ref_125m");
	if (sc->sc_clk_pcie_phy == NULL) {
		aprint_error(": couldn't get clock pcie_phy\n");
		return;
	}

	TAILQ_INIT(&sc->sc_intrs);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	sc->sc_ih = intr_establish(aa->aa_irq, IPL_VM, IST_LEVEL,
	    imxpcie_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %d\n",
		    aa->aa_irq);
		return;
	}
	aprint_normal_dev(self, "interrupting on %d\n", aa->aa_irq);

	imxpcie_attach_common(sc);
}

static void
imx6_pcie_configure(void *cookie)
{
	struct imx6_pcie_softc *ipsc = cookie;
	struct imxpcie_softc *sc = &ipsc->sc_imxpcie;

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
	return iomux_read(reg);
}

static void
imx6_pcie_gpr_write(void *cookie, uint32_t reg, uint32_t val)
{
	iomux_write(reg, val);
}

static void
imx6_pcie_reset(void *cookie)
{
	struct imx6_pcie_softc *ipsc = cookie;

#if NIMXGPIO > 0
	if (ipsc->sc_gpio_reset >= 0) {
		imxgpio_data_write(ipsc->sc_gpio_reset, ipsc->sc_gpio_reset_active);
		delay(100 * 1000);
		imxgpio_data_write(ipsc->sc_gpio_reset, !ipsc->sc_gpio_reset_active);
	}
#endif
}
