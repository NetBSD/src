/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
#define USBH_PRIVATE

#include "locators.h"
#include "ohci.h"
#include "ehci.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: awin_usb.c,v 1.17.2.6 2016/02/12 16:43:38 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#if NOHCI > 0
#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>
#endif

#if NEHCI > 0
#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>
#endif

#include <dev/pci/pcidevs.h>

struct awinusb_softc {
	device_t usbsc_dev;
	bus_dma_tag_t usbsc_dmat;
	bus_space_tag_t usbsc_bst;
	bus_space_handle_t usbsc_ehci_bsh;
	bus_space_handle_t usbsc_ohci_bsh;
	bus_space_handle_t usbsc_usb0_phy_csr_bsh;
	u_int usbsc_number;
	struct awin_gpio_pindata usbsc_drv_pin;
	struct awin_gpio_pindata usbsc_restrict_pin;

	device_t usbsc_ohci_dev;
	device_t usbsc_ehci_dev;
	void *usbsc_ohci_ih;
	void *usbsc_ehci_ih;
};

struct awinusb_attach_args {
	const char *usbaa_name;
	bus_dma_tag_t usbaa_dmat;
	bus_space_tag_t usbaa_bst;
	bus_space_handle_t usbaa_bsh;
	bus_space_handle_t usbaa_ccm_bsh;
	bus_size_t usbaa_size;
	int usbaa_port;
};

#if NOHCI > 0
static const int awinusb_ohci_irqs[2] = { AWIN_IRQ_USB3, AWIN_IRQ_USB4 };
static const int awinusb_ohci_irqs_a31[2] = { AWIN_A31_IRQ_USB3,
					      AWIN_A31_IRQ_USB4 };
static const int awinusb_ohci_irqs_a80[3] = { AWIN_A80_IRQ_USB_OHCI0,
					      -1,
					      AWIN_A80_IRQ_USB_OHCI2 };

#ifdef OHCI_DEBUG
#define OHCI_DPRINTF(x)	if (ohcidebug) printf x
extern int ohcidebug;
#else
#define OHCI_DPRINTF(x)
#endif

static int ohci_awinusb_match(device_t, cfdata_t, void *);
static void ohci_awinusb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ohci_awinusb, sizeof(struct ohci_softc),
	ohci_awinusb_match, ohci_awinusb_attach, NULL, NULL);

static int
ohci_awinusb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinusb_attach_args * const usbaa = aux;

	if (strcmp(cf->cf_name, usbaa->usbaa_name))
		return 0;

	return 1;
}

static void
ohci_awinusb_attach(device_t parent, device_t self, void *aux)
{
	struct awinusb_softc * const usbsc = device_private(parent);
	struct ohci_softc * const sc = device_private(self);
	struct awinusb_attach_args * const usbaa = aux;
	int irq;

	sc->sc_dev = self;

	sc->iot = usbaa->usbaa_bst;
	sc->ioh = usbaa->usbaa_bsh;
	sc->sc_size = usbaa->usbaa_size;
	sc->sc_bus.ub_dmatag = usbaa->usbaa_dmat;
	sc->sc_bus.ub_hcpriv = sc;

	//sc->sc_id_vendor = PCI_VENDOR_ALLWINNER;
	strlcpy(sc->sc_vendor, "Allwinner", sizeof(sc->sc_vendor));

	aprint_naive(": OHCI USB controller\n");
	aprint_normal(": OHCI USB controller\n");

	int error = ohci_init(sc);
	if (error) {
		aprint_error_dev(self, "init failed, error=%d\n", error);
		return;
	}

	/* Attach usb device. */
	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);

	switch (awin_chip_id()) {
	case AWIN_CHIP_ID_A80:
		irq = awinusb_ohci_irqs_a80[usbaa->usbaa_port];
		break;
	case AWIN_CHIP_ID_A31:
		irq = awinusb_ohci_irqs_a31[usbaa->usbaa_port];
		break;
	default:
		irq = awinusb_ohci_irqs[usbaa->usbaa_port];
		break;
	}

	usbsc->usbsc_ohci_ih = intr_establish(irq, IPL_VM,
	    IST_LEVEL, ohci_intr, sc);
	if (usbsc->usbsc_ohci_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     irq);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", irq);
}
#endif /* NOHCI > 0 */

#if NEHCI > 0
#ifdef EHCI_DEBUG
#define EHCI_DPRINTF(x)	if (ehcidebug) printf x
extern int ehcidebug;
#else
#define EHCI_DPRINTF(x)
#endif

static int ehci_awinusb_match(device_t, cfdata_t, void *);
static void ehci_awinusb_attach(device_t, device_t, void *);

static const int awinusb_ehci_irqs[2] = { AWIN_IRQ_USB1, AWIN_IRQ_USB2 };
static const int awinusb_ehci_irqs_a31[2] = { AWIN_A31_IRQ_USB1,
					      AWIN_A31_IRQ_USB2 };
static const int awinusb_ehci_irqs_a80[3] = { AWIN_A80_IRQ_USB_EHCI0,
					      AWIN_A80_IRQ_USB_EHCI1,
					      AWIN_A80_IRQ_USB_EHCI2 };

CFATTACH_DECL_NEW(ehci_awinusb, sizeof(struct ehci_softc),
	ehci_awinusb_match, ehci_awinusb_attach, NULL, NULL);

static int
ehci_awinusb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinusb_attach_args * const usbaa = aux;

	if (strcmp(cf->cf_name, usbaa->usbaa_name))
		return 0;

	return 1;
}

static void
ehci_awinusb_attach(device_t parent, device_t self, void *aux)
{
	struct awinusb_softc * const usbsc = device_private(parent);
	struct ehci_softc * const sc = device_private(self);
	struct awinusb_attach_args * const usbaa = aux;
	int irq;

	sc->sc_dev = self;

	sc->iot = usbaa->usbaa_bst;
	sc->ioh = usbaa->usbaa_bsh;
	sc->sc_size = usbaa->usbaa_size;
	sc->sc_bus.ub_dmatag = usbaa->usbaa_dmat;
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_revision = USBREV_2_0;
	sc->sc_ncomp = 0;
	if (usbsc->usbsc_ohci_dev != NULL) {
		sc->sc_comps[sc->sc_ncomp++] = usbsc->usbsc_ohci_dev;
	}

	//sc->sc_id_vendor = PCI_VENDOR_ALLWINNER;
	strlcpy(sc->sc_vendor, "Allwinner", sizeof(sc->sc_vendor));

	aprint_naive(": EHCI USB controller\n");
	aprint_normal(": EHCI USB controller\n");

	int error = ehci_init(sc);
	if (error) {
		aprint_error_dev(self, "init failed, error=%d\n", error);
		return;
	}
	/* Attach usb device. */
	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);

	switch (awin_chip_id()) {
	case AWIN_CHIP_ID_A80:
		irq = awinusb_ehci_irqs_a80[usbaa->usbaa_port];
		break;
	case AWIN_CHIP_ID_A31:
		irq = awinusb_ehci_irqs_a31[usbaa->usbaa_port];
		break;
	default:
		irq = awinusb_ehci_irqs[usbaa->usbaa_port];
		break;
	}

	usbsc->usbsc_ehci_ih = intr_establish(irq, IPL_VM,
	    IST_LEVEL | IST_MPSAFE, ehci_intr, sc);
	if (usbsc->usbsc_ehci_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     irq);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", irq);
}
#endif /* NEHCI > 0 */

static void
awin_usb_phy_write(struct awinusb_softc *usbsc, u_int bit_addr, u_int bits,
	u_int len)
{
	bus_space_tag_t bst = usbsc->usbsc_bst;
	bus_space_handle_t bsh = usbsc->usbsc_usb0_phy_csr_bsh;
	uint32_t clk = AWIN_USB0_PHY_CTL_CLK0 << usbsc->usbsc_number;

	uint32_t v = bus_space_read_4(bst, bsh, 0);

	KASSERT((v & AWIN_USB0_PHY_CTL_CLK0) == 0);
	KASSERT((v & AWIN_USB0_PHY_CTL_CLK1) == 0);
	KASSERT((v & AWIN_USB0_PHY_CTL_CLK2) == 0);

	v &= ~AWIN_USB0_PHY_CTL_ADDR;
	v &= ~AWIN_USB0_PHY_CTL_DAT;

	v |= __SHIFTIN(bit_addr, AWIN_USB0_PHY_CTL_ADDR);

	/*
	 * Bitbang the data to the phy, bit by bit, incrementing bit address
	 * as we go.
	 */
	for (; len > 0; bit_addr++, bits >>= 1, len--) {
		v |= __SHIFTIN(bits & 1, AWIN_USB0_PHY_CTL_DAT);
		bus_space_write_4(bst, bsh, 0, v);
		delay(1);
		bus_space_write_4(bst, bsh, 0, v | clk);
		delay(1);
		bus_space_write_4(bst, bsh, 0, v);
		delay(1);
		v += __LOWEST_SET_BIT(AWIN_USB0_PHY_CTL_ADDR);
		v &= ~AWIN_USB0_PHY_CTL_DAT;
	}
}

static int awinusb_match(device_t, cfdata_t, void *);
static void awinusb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(awin_usb, sizeof(struct awinusb_softc),
	awinusb_match, awinusb_attach, NULL, NULL);

static int awinusb_ports;

static const char awinusb_drvpin_names[3][8] = { "usb1drv", "usb2drv", "usb3drv" };
static const char awinusb_restrictpin_names[3][13] = { "usb1restrict", "usb2restrict", "usb3restrict" };

static const bus_size_t awinusb_dram_hpcr_regs[2] = {
	AWIN_DRAM_HPCR_USB1_REG,
	AWIN_DRAM_HPCR_USB2_REG,
};
static const uint32_t awinusb_ahb_gating[2] = {
#if NOHCI > 0
	AWIN_AHB_GATING0_USB_OHCI0 |
#endif
	AWIN_AHB_GATING0_USB_EHCI0,
#if NOHCI > 0
	AWIN_AHB_GATING0_USB_OHCI1 |
#endif
	AWIN_AHB_GATING0_USB_EHCI1,
};
static const uint32_t awinusb_ahb_gating_a31[2] = {
#if NOHCI > 0
	AWIN_A31_AHB_GATING0_USB_OHCI0 |
#endif
	AWIN_A31_AHB_GATING0_USB_EHCI0,
#if NOHCI > 0
	AWIN_A31_AHB_GATING0_USB_OHCI1 |
#endif
	AWIN_A31_AHB_GATING0_USB_EHCI1,
};
static const uint32_t awinusb_usb_clk_set[2] = {
#if NOHCI > 0
	AWIN_USB_CLK_OHCI0_ENABLE |
#endif
	AWIN_USB_CLK_USBPHY_ENABLE|AWIN_USB_CLK_PHY1_ENABLE,
#if NOHCI > 0
	AWIN_USB_CLK_OHCI1_ENABLE |
#endif
	AWIN_USB_CLK_USBPHY_ENABLE|AWIN_USB_CLK_PHY2_ENABLE,
};
static const uint32_t awinusb_usb_clk_set_a31[2] = {
#if NOHCI > 0
	AWIN_A31_USB_CLK_OHCI0_ENABLE |
#endif
	AWIN_A31_USB_CLK_USBPHY0_ENABLE |
	AWIN_A31_USB_CLK_USBPHY1_ENABLE |
	AWIN_A31_USB_CLK_PHY1_ENABLE,
#if NOHCI > 0
	AWIN_A31_USB_CLK_OHCI1_ENABLE |
#endif
	AWIN_A31_USB_CLK_USBPHY0_ENABLE |
	AWIN_A31_USB_CLK_USBPHY2_ENABLE |
	AWIN_A31_USB_CLK_PHY2_ENABLE,
};
static const uint32_t awinusb_usb_ahb_reset_a31[2] = {
#if NOHCI > 0
	AWIN_A31_AHB_RESET0_USBOHCI0_RST |
#endif
#if NEHCI > 0
	AWIN_A31_AHB_RESET0_USBEHCI0_RST |
#endif
	0,
#if NOHCI > 0
	AWIN_A31_AHB_RESET0_USBOHCI1_RST |
#endif
#if NEHCI > 0
	AWIN_A31_AHB_RESET0_USBEHCI1_RST |
#endif
	0,
};
static const uint32_t awinusb_usb_scr_a80[3] = {
	AWIN_A80_USBPHY_HCI_SCR_HCI0_AHB_GATING |
#if NOHCI > 0
	AWIN_A80_USBPHY_HCI_SCR_OHCI0_SCLK_GATING |
#endif
	AWIN_A80_USBPHY_HCI_SCR_HCI0_RST,
	AWIN_A80_USBPHY_HCI_SCR_HCI1_AHB_GATING |
	AWIN_A80_USBPHY_HCI_SCR_HCI1_RST,
	AWIN_A80_USBPHY_HCI_SCR_HCI2_AHB_GATING |
#if NOHCI > 0
	AWIN_A80_USBPHY_HCI_SCR_OHCI2_SCLK_GATING |
#endif
	AWIN_A80_USBPHY_HCI_SCR_HCI2_RST,
};
static const uint32_t awinusb_usb_pcr_a80[3] = {
	AWIN_A80_USBPHY_HCI_PCR_SCLK_GATING_HCI0_PHY |
	AWIN_A80_USBPHY_HCI_PCR_HCI0_PHY_RST,
	AWIN_A80_USBPHY_HCI_PCR_480M_GATING_HCI1_HSIC |
	AWIN_A80_USBPHY_HCI_PCR_12M_GATING_HCI1_HSIC |
	AWIN_A80_USBPHY_HCI_PCR_HCI1_HSIC_RST,
	AWIN_A80_USBPHY_HCI_PCR_SCLK_GATING_HCI2_UTMIPHY |
	AWIN_A80_USBPHY_HCI_PCR_HCI2_UTMIPHY_RST,
};

int
awinusb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio __diagused = aux;
	const struct awin_locators * const loc __diagused = &aio->aio_loc;

	KASSERT(loc->loc_port != AWINIOCF_PORT_DEFAULT);
	KASSERT(!strcmp(cf->cf_name, loc->loc_name));
	KASSERT(cf->cf_loc[AWINIOCF_PORT] == AWINIOCF_PORT_DEFAULT
	    || cf->cf_loc[AWINIOCF_PORT] == loc->loc_port);
	KASSERT((awinusb_ports & __BIT(loc->loc_port)) == 0);

	return 1;
}

void
awinusb_attach(device_t parent, device_t self, void *aux)
{
	struct awinusb_softc * const usbsc = device_private(self);
	const struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	bus_space_handle_t usb_bsh;
	bool has_ohci = true;

	awinusb_ports |= __BIT(loc->loc_port);

	usbsc->usbsc_bst = aio->aio_core_bst;
	usbsc->usbsc_dmat = aio->aio_dmat;
	usbsc->usbsc_number = loc->loc_port + 1;

	usb_bsh = awin_chip_id() == AWIN_CHIP_ID_A80 ?
	    aio->aio_a80_usb_bsh : aio->aio_core_bsh;

	bus_space_subregion(usbsc->usbsc_bst, usb_bsh,
	    loc->loc_offset + AWIN_EHCI_OFFSET, AWIN_EHCI_SIZE,
	    &usbsc->usbsc_ehci_bsh);
	bus_space_subregion(usbsc->usbsc_bst, usb_bsh,
	    loc->loc_offset + AWIN_OHCI_OFFSET, AWIN_OHCI_SIZE,
	    &usbsc->usbsc_ohci_bsh);

	if (awin_chip_id() != AWIN_CHIP_ID_A80) {
		bus_space_subregion(usbsc->usbsc_bst, usb_bsh,
		    AWIN_USB0_OFFSET + AWIN_USB0_PHY_CTL_REG, 4,
		    &usbsc->usbsc_usb0_phy_csr_bsh);
	}

	if (awin_chip_id() == AWIN_CHIP_ID_A80 && loc->loc_port == 1) {
		has_ohci = false;
	}

	aprint_naive("\n");
	aprint_normal("\n");

	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		/* Enable USB PHY */
		awin_reg_set_clear(usbsc->usbsc_bst, aio->aio_ccm_bsh,
		    AWIN_USB_CLK_REG, awinusb_usb_clk_set_a31[loc->loc_port],
		    0);

		/* AHB gate enable */
		awin_reg_set_clear(usbsc->usbsc_bst, aio->aio_ccm_bsh,
		    AWIN_AHB_GATING0_REG,
		    AWIN_A31_AHB_GATING0_USB0 | awinusb_ahb_gating_a31[loc->loc_port],
		    0);

		/* Soft reset */
		awin_reg_set_clear(usbsc->usbsc_bst, aio->aio_ccm_bsh,
		    AWIN_A31_AHB_RESET0_REG, awinusb_usb_ahb_reset_a31[loc->loc_port], 0);
	} else if (awin_chip_id() == AWIN_CHIP_ID_A80) {
		/* Gate enable */
		awin_reg_set_clear(usbsc->usbsc_bst, aio->aio_ccm_bsh,
		    AWIN_A80_CCU_SCLK_BUS_CLK_GATING1_REG,
		    AWIN_A80_CCU_SCLK_BUS_CLK_GATING1_USB_HOST, 0);

		/* Enable USB PHY */
		awin_reg_set_clear(usbsc->usbsc_bst, usb_bsh,
		    AWIN_A80_USBPHY_OFFSET + AWIN_A80_USBPHY_HCI_PCR_REG,
		    awinusb_usb_pcr_a80[loc->loc_port], 0);
		awin_reg_set_clear(usbsc->usbsc_bst, usb_bsh,
		    AWIN_A80_USBPHY_OFFSET + AWIN_A80_USBPHY_HCI_SCR_REG,
		    awinusb_usb_scr_a80[loc->loc_port], 0);

		if (!has_ohci) {
			/* No OHCI for USB1, force EHCI mode */
			awin_reg_set_clear(usbsc->usbsc_bst, usb_bsh,
			    loc->loc_offset + AWIN_USB_PMU_IRQ_REG,
			    AWIN_USB_PMU_IRQ_EHCI_HS_FORCE |
			    AWIN_USB_PMU_IRQ_HSIC_CONNECT_DET |
			    AWIN_USB_PMU_IRQ_HSIC, 0);
		}
	} else {
		/*
		 * Access to the USB phy is off USB0 so make sure it's on.
		*/
		awin_reg_set_clear(usbsc->usbsc_bst, aio->aio_ccm_bsh,
		    AWIN_AHB_GATING0_REG,
		    AWIN_AHB_GATING0_USB0 | awinusb_ahb_gating[loc->loc_port],
		    0);

		/*
		 * Enable the USB phy for this port.
		 */
		awin_reg_set_clear(usbsc->usbsc_bst, aio->aio_ccm_bsh,
		    AWIN_USB_CLK_REG, awinusb_usb_clk_set[loc->loc_port], 0);
	}

	/*
	 * Allow USB DMA engine access to the DRAM.
	 */
	awin_reg_set_clear(usbsc->usbsc_bst, usb_bsh,
	    loc->loc_offset + AWIN_USB_PMU_IRQ_REG,
	    AWIN_USB_PMU_IRQ_AHB_INCR8 | AWIN_USB_PMU_IRQ_AHB_INCR4
	       | AWIN_USB_PMU_IRQ_AHB_INCRX | AWIN_USB_PMU_IRQ_ULPI_BYPASS, 0);

	if (awin_chip_id() == AWIN_CHIP_ID_A20) {
		awin_reg_set_clear(usbsc->usbsc_bst, aio->aio_core_bsh,
		    AWIN_DRAM_OFFSET + awinusb_dram_hpcr_regs[loc->loc_port],
		    AWIN_DRAM_HPCR_ACCESS_EN, 0);
	}

	/* initialize the USB phy */
	if (awin_chip_id() != AWIN_CHIP_ID_A80) {
		awin_usb_phy_write(usbsc, 0x20, 0x14, 5);
		awin_usb_phy_write(usbsc, 0x2a, 0x03, 2);
	}

	/*
	 * Now get the GPIO that enables the power to the port and
	 * turn it on.
	 */
	if (awin_gpio_pin_reserve(awinusb_drvpin_names[loc->loc_port],
		    &usbsc->usbsc_drv_pin)) {
		awin_gpio_pindata_write(&usbsc->usbsc_drv_pin, 1);
	} else {
		aprint_error_dev(self, "no power gpio found\n");
	}

	if (awin_gpio_pin_reserve(awinusb_restrictpin_names[loc->loc_port],
		    &usbsc->usbsc_restrict_pin)) {
		awin_gpio_pindata_write(&usbsc->usbsc_restrict_pin, 1);
	} else {
		aprint_debug_dev(self, "no restrict gpio found\n");
	}

	/*
	 * Disable interrupts
	 */
#if NOHCI > 0
	if (has_ohci) {
		bus_space_write_4(usbsc->usbsc_bst, usbsc->usbsc_ohci_bsh,
		    OHCI_INTERRUPT_DISABLE, OHCI_ALL_INTRS);
	}
#endif
#if NEHCI > 0
	bus_size_t caplength = bus_space_read_1(usbsc->usbsc_bst,
	    usbsc->usbsc_ehci_bsh, EHCI_CAPLENGTH);
	bus_space_write_4(usbsc->usbsc_bst, usbsc->usbsc_ehci_bsh,
	    caplength + EHCI_USBINTR, 0);
#endif

#if NOHCI > 0
	if (has_ohci) {
		struct awinusb_attach_args usbaa_ohci = {
			.usbaa_name = "ohci",
			.usbaa_dmat = usbsc->usbsc_dmat,
			.usbaa_bst = usbsc->usbsc_bst,
			.usbaa_bsh = usbsc->usbsc_ohci_bsh,
			.usbaa_ccm_bsh = aio->aio_ccm_bsh,
			.usbaa_size = AWIN_OHCI_SIZE,
			.usbaa_port = loc->loc_port,
		};

		usbsc->usbsc_ohci_dev = config_found(self, &usbaa_ohci, NULL);
	}
#endif

#if NEHCI > 0
	struct awinusb_attach_args usbaa_ehci = {
		.usbaa_name = "ehci",
		.usbaa_dmat = usbsc->usbsc_dmat,
		.usbaa_bst = usbsc->usbsc_bst,
		.usbaa_bsh = usbsc->usbsc_ehci_bsh,
		.usbaa_ccm_bsh = aio->aio_ccm_bsh,
		.usbaa_size = AWIN_EHCI_SIZE,
		.usbaa_port = loc->loc_port,
	};

	config_found(self, &usbaa_ehci, NULL);
#endif
}
