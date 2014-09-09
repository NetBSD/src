/*	$NetBSD: exynos_usb.c,v 1.9 2014/09/09 21:26:47 reinoud Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk.
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

#include "locators.h"
#include "ohci.h"
#include "ehci.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: exynos_usb.c,v 1.9 2014/09/09 21:26:47 reinoud Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/intr.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

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

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_var.h>
#include <arm/samsung/exynos_io.h>

struct exynos_usb_softc {
	device_t	 sc_self;

	/* keep our tags here */
	bus_dma_tag_t	 sc_dmat;
	bus_space_tag_t  sc_bst;

	bus_space_handle_t sc_ehci_bsh;
	bus_space_handle_t sc_ohci_bsh;
	bus_space_handle_t sc_usb2phy_bsh;

	bus_space_handle_t sc_sysregs_bsh;
	bus_space_handle_t sc_pmuregs_bsh;

	device_t	 sc_ohci_dev;
	device_t	 sc_ehci_dev;

	int		 sc_irq;
	void		*sc_intrh;
} exynos_usb_sc;

struct exynos_usb_attach_args {
	const char *name;
};


#ifdef EXYNOS4
static struct exynos_gpio_pinset e4_uhost_pwr_pinset = {
	.pinset_group = "ETC6",
	.pinset_func  = 0,
	.pinset_mask  = __BIT(6) | __BIT(7),
};
#endif


#ifdef EXYNOS5
static struct exynos_gpio_pinset e5_uhost_pwr_pinset = {
	.pinset_group = "ETC6",
	.pinset_func  = 0,
	.pinset_mask  = __BIT(5) | __BIT(6),
};
#endif


static int exynos_usb_intr(void *arg);
static void exynos_usb_phy_init(struct exynos_usb_softc *sc);


static int	exynos_usb_match(device_t, cfdata_t, void *);
static void	exynos_usb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(exyo_usb, 0,
    exynos_usb_match, exynos_usb_attach, NULL, NULL);


static int
exynos_usb_match(device_t parent, cfdata_t cf, void *aux)
{
	/* there can only be one */
	if (exynos_usb_sc.sc_self)
		return 0;

	return 1;
}


static void
exynos_usb_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_usb_softc * const sc = &exynos_usb_sc;
	struct exyo_attach_args *exyoaa = (struct exyo_attach_args *) aux;
	struct exyo_locators *loc = &exyoaa->exyo_loc;
	struct exynos_gpio_pindata XuhostOVERCUR;
	struct exynos_gpio_pindata XuhostPWREN;
	bus_size_t ehci_offset, ohci_offset, usb2phy_offset;
	bus_size_t pmu_offset, sysreg_offset;

	/* no locators expected */
	KASSERT(loc->loc_port == EXYOCF_PORT_DEFAULT);
	KASSERT(loc->loc_intr != EXYOCF_INTR_DEFAULT);

	/* copy our device handle */
	sc->sc_self = self;
	sc->sc_irq  = loc->loc_intr;

	/* get our bushandles */
	sc->sc_bst  = exyoaa->exyo_core_bst;
	sc->sc_dmat = exyoaa->exyo_dmat;
//	sc->sc_dmat = exyoaa->exyo_coherent_dmat;

#ifdef EXYNOS4
	if (IS_EXYNOS4_P()) {
		ehci_offset    = EXYNOS4_USB2_HOST_EHCI_OFFSET;
		ohci_offset    = EXYNOS4_USB2_HOST_OHCI_OFFSET;
		usb2phy_offset = EXYNOS4_USB2_HOST_PHYCTRL_OFFSET;
		sysreg_offset  = EXYNOS4_SYSREG_OFFSET;
		pmu_offset     = EXYNOS4_PMU_OFFSET;
	}
#endif
#ifdef EXYNOS5
	if (IS_EXYNOS5_P()) {
		ehci_offset    = EXYNOS5_USB2_HOST_EHCI_OFFSET;
		ohci_offset    = EXYNOS5_USB2_HOST_OHCI_OFFSET;
		usb2phy_offset = EXYNOS5_USB2_HOST_PHYCTRL_OFFSET;
		sysreg_offset  = EXYNOS5_SYSREG_OFFSET;
		pmu_offset     = EXYNOS5_PMU_OFFSET;
	}
#endif
	KASSERT(ehci_offset);

	bus_space_subregion(sc->sc_bst, exyoaa->exyo_core_bsh,
		ehci_offset, EXYNOS_BLOCK_SIZE,
		&sc->sc_ehci_bsh);
	bus_space_subregion(sc->sc_bst, exyoaa->exyo_core_bsh,
		ohci_offset, EXYNOS_BLOCK_SIZE,
		&sc->sc_ohci_bsh);
	bus_space_subregion(sc->sc_bst, exyoaa->exyo_core_bsh,
		usb2phy_offset, EXYNOS_BLOCK_SIZE,
		&sc->sc_usb2phy_bsh);

	bus_space_subregion(sc->sc_bst, exyoaa->exyo_core_bsh,
		sysreg_offset, EXYNOS_BLOCK_SIZE,
		&sc->sc_sysregs_bsh);
	bus_space_subregion(sc->sc_bst, exyoaa->exyo_core_bsh,
		pmu_offset, EXYNOS_BLOCK_SIZE,
		&sc->sc_pmuregs_bsh);

	aprint_naive("\n");
	aprint_normal("\n");

	/* power up USB subsystem */
#ifdef EXYNOS4
	if (IS_EXYNOS4_P()) {
		exynos_gpio_pinset_acquire(&e4_uhost_pwr_pinset);
		exynos_gpio_pinset_to_pindata(&e4_uhost_pwr_pinset, 6, &XuhostPWREN);
		exynos_gpio_pinset_to_pindata(&e4_uhost_pwr_pinset, 7, &XuhostOVERCUR);
	}
#endif
#ifdef EXYNOS5
	if (IS_EXYNOS5_P()) {
		exynos_gpio_pinset_acquire(&e5_uhost_pwr_pinset);
		exynos_gpio_pinset_to_pindata(&e5_uhost_pwr_pinset, 5, &XuhostPWREN);
		exynos_gpio_pinset_to_pindata(&e5_uhost_pwr_pinset, 6, &XuhostOVERCUR);
	}
#endif

	/* enable power and set Xuhost OVERCUR to inactive by pulling it up */
	exynos_gpio_pindata_ctl(&XuhostPWREN, GPIO_PIN_PULLUP);
	exynos_gpio_pindata_ctl(&XuhostOVERCUR, GPIO_PIN_PULLUP);
	DELAY(80000);

	/* init USB phys */
	exynos_usb_phy_init(sc);

	/*
	 * Disable interrupts
	 */
#if NOHCI > 0
	bus_space_write_4(sc->sc_bst, sc->sc_ohci_bsh,
	    OHCI_INTERRUPT_DISABLE, OHCI_ALL_INTRS);
#endif
#if NEHCI > 0
	bus_size_t caplength = bus_space_read_1(sc->sc_bst,
	    sc->sc_ehci_bsh, EHCI_CAPLENGTH);
	bus_space_write_4(sc->sc_bst, sc->sc_ehci_bsh,
	    caplength + EHCI_USBINTR, 0);
#endif

	/* claim shared interrupt for OHCI/EHCI */
	sc->sc_intrh = intr_establish(sc->sc_irq,
		IPL_USB, IST_LEVEL, exynos_usb_intr, sc);
	if (!sc->sc_intrh) {
		aprint_error(": unable to establish interrupt at irq %d\n",
			sc->sc_irq);
		/* disable? TBD */
		return;
	}
	aprint_normal_dev(sc->sc_self, "USB2 host interrupting on irq %d\n",
		sc->sc_irq);

#if NOHCI > 0
	/* attach OHCI */
	struct exynos_usb_attach_args usb_ohci = {
		.name = "ohci",
	};
	sc->sc_ohci_dev = config_found(self, &usb_ohci, NULL);
#endif

#if NEHCI > 0
	/* attach EHCI */
	struct exynos_usb_attach_args usb_ehci = {
		.name = "ehci",
	};
	sc->sc_ehci_dev = config_found(self, &usb_ehci, NULL);
#endif
}


static int
exynos_usb_intr(void *arg)
{
	struct exynos_usb_softc *sc = (struct exynos_usb_softc *) arg;
	void *private;
	int ret = 0;

#if NEHCI > 0
	private = device_private(sc->sc_ehci_dev);
	if (private)
		ret = ehci_intr(private);
#endif
	/* XXX should we always deliver to ohci even if ehci takes it? */
//	if (ret)
//		return ret;
#if NOHCI > 0
	private = device_private(sc->sc_ohci_dev);
	if (private)
		ret = ohci_intr(private);
#endif

	return ret;
}


#if NOHCI > 0
static int	exynos_ohci_match(device_t, cfdata_t, void *);
static void	exynos_ohci_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ohci_exyousb, sizeof(struct ohci_softc),
    exynos_ohci_match, exynos_ohci_attach, NULL, NULL);


static int
exynos_ohci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct exynos_usb_attach_args *euaa = aux;

	if (strcmp(euaa->name, "ohci"))
		return 0;

	return 1;
}


static void
exynos_ohci_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_usb_softc *usbsc = &exynos_usb_sc;
	struct ohci_softc *sc = device_private(self);
	int r;

	sc->sc_dev = self;
	sc->iot = usbsc->sc_bst;
	sc->ioh = usbsc->sc_ohci_bsh;
	sc->sc_size = EXYNOS_BLOCK_SIZE;
	sc->sc_bus.dmatag = usbsc->sc_dmat;
	sc->sc_bus.hci_private = sc;

	strlcpy(sc->sc_vendor, "exynos", sizeof(sc->sc_vendor));

	aprint_naive(": OHCI USB controller\n");
	aprint_normal(": OHCI USB controller\n");

	/* attach */
	r = ohci_init(sc);
	if (r != USBD_NORMAL_COMPLETION) {
		aprint_error_dev(self, "init failed, error = %d\n", r);
		/* disable : TBD */
		return;
	}
	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);
	aprint_normal_dev(sc->sc_dev, "interrupting on irq %d\n",
		usbsc->sc_irq);
}
#endif


#if NEHCI > 0
static int	exynos_ehci_match(device_t, cfdata_t, void *);
static void	exynos_ehci_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ehci_exyousb, sizeof(struct ehci_softc),
    exynos_ehci_match, exynos_ehci_attach, NULL, NULL);


static int
exynos_ehci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct exynos_usb_attach_args *euaa = aux;

	if (strcmp(euaa->name, "ehci"))
		return 0;

	return 1;
}


static void
exynos_ehci_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_usb_softc *usbsc = &exynos_usb_sc;
	struct ehci_softc *sc = device_private(self);
	int r;

	sc->sc_dev = self;
	sc->iot = usbsc->sc_bst;
	sc->ioh = usbsc->sc_ehci_bsh;
	sc->sc_size = EXYNOS_BLOCK_SIZE;
	sc->sc_bus.dmatag = usbsc->sc_dmat;
	sc->sc_bus.hci_private = sc;
	sc->sc_bus.usbrev = USBREV_2_0;
	sc->sc_ncomp = 0;
	if (usbsc->sc_ohci_dev != NULL)
		sc->sc_comps[sc->sc_ncomp++] = usbsc->sc_ohci_dev;

	strlcpy(sc->sc_vendor, "exynos", sizeof(sc->sc_vendor));

	aprint_naive(": EHCI USB controller\n");
	aprint_normal(": EHCI USB controller\n");

	/* attach */
	r = ehci_init(sc);
	if (r != USBD_NORMAL_COMPLETION) {
		aprint_error_dev(self, "init failed, error = %d\n", r);
		/* disable : TBD */
		return;
	}
	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);
	aprint_normal_dev(sc->sc_dev, "interrupting on irq %d\n",
		usbsc->sc_irq);
}
#endif


/*
 * USB Phy init
 */

/* XXX 5422 not handled since its unknown how it handles this XXX*/
static void
exynos_usb2_set_isolation(struct exynos_usb_softc *sc, bool on)
{
	uint32_t en_mask, regval;
	bus_addr_t reg;

	/* enable PHY */
	reg = EXYNOS_PMU_USB_PHY_CTRL;

	if (IS_EXYNOS5_P() || IS_EXYNOS4410_P()) {
		/* set usbhost mode */
		regval = on ? 0 : USB20_PHY_HOST_LINK_EN;
		bus_space_write_4(sc->sc_bst, sc->sc_sysregs_bsh,
			EXYNOS5_SYSREG_USB20_PHY_TYPE, regval);
		reg = EXYNOS_PMU_USBHOST_PHY_CTRL;
	}

	/* do enable PHY */
	en_mask = PMU_PHY_ENABLE;
	regval = bus_space_read_4(sc->sc_bst, sc->sc_pmuregs_bsh, reg);
	regval = on ? regval & ~en_mask : regval | en_mask;

	bus_space_write_4(sc->sc_bst, sc->sc_pmuregs_bsh,
		reg, regval);

	if (IS_EXYNOS4X12_P()) {
		bus_space_write_4(sc->sc_bst, sc->sc_pmuregs_bsh,
			EXYNOS_PMU_USB_HSIC_1_PHY_CTRL, regval);
		bus_space_write_4(sc->sc_bst, sc->sc_pmuregs_bsh,
			EXYNOS_PMU_USB_HSIC_2_PHY_CTRL, regval);
	}
}


#ifdef EXYNOS4
static void
exynos4_usb2phy_enable(struct exynos_usb_softc *sc)
{
	uint32_t phypwr, rstcon, clkreg;

	/* write clock value */
	clkreg = FSEL_CLKSEL_24M;
	bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHYCLK, clkreg);

	/* set device and host to normal */
	phypwr = bus_space_read_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHYPWR);

	/* enable analog, enable otg, unsleep phy0 (host) */
	phypwr &= ~PHYPWR_NORMAL_MASK_PHY0;
	bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHYPWR, phypwr);

	if (IS_EXYNOS4X12_P()) {
		/* enable hsic0 (host), enable hsic1 and phy1 (otg) */
		phypwr = bus_space_read_4(sc->sc_bst, sc->sc_usb2phy_bsh,
			USB_PHYPWR);
		phypwr &= ~(PHYPWR_NORMAL_MASK_HSIC0 |
			    PHYPWR_NORMAL_MASK_HSIC1 |
			    PHYPWR_NORMAL_MASK_PHY1);
		bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
			USB_PHYPWR, phypwr);
	}

	/* reset both phy and link of device */
	rstcon = bus_space_read_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_RSTCON);
	rstcon |= RSTCON_DEVPHY_SWRST;
	bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_RSTCON, rstcon);
	DELAY(10000);
	rstcon &= ~RSTCON_DEVPHY_SWRST;
	bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_RSTCON, rstcon);

	if (IS_EXYNOS4X12_P()) {
		/* reset both phy and link of host */
		rstcon = bus_space_read_4(sc->sc_bst, sc->sc_usb2phy_bsh,
			USB_RSTCON);
		rstcon |= RSTCON_HOSTPHY_SWRST | RSTCON_HOSTPHYLINK_SWRST;
		bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
			USB_RSTCON, rstcon);
		DELAY(10000);
		rstcon &= ~(RSTCON_HOSTPHY_SWRST | RSTCON_HOSTPHYLINK_SWRST);
		bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
			USB_RSTCON, rstcon);
	}

	/* wait for everything to be initialized */
	DELAY(80000);
}
#endif


#ifdef EXYNOS5
static void
exynos5410_usb2phy_enable(struct exynos_usb_softc *sc)
{
	uint32_t phyhost, phyotg, phyhsic, ehcictrl, ohcictrl;

	/* host configuration: */
	phyhost = bus_space_read_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHY_HOST_CTRL0);

	/* host phy reference clock; assumption its 24 MHz now */
	phyhost &= ~HOST_CTRL0_FSEL_MASK;
	phyhost |= __SHIFTIN(HOST_CTRL0_FSEL_MASK, FSEL_CLKSEL_24M);

	/* enable normal mode of operation */
	phyhost &= ~(HOST_CTRL0_FORCESUSPEND | HOST_CTRL0_FORCESLEEP);

	/* host phy reset */
	phyhost &= ~(HOST_CTRL0_PHY_SWRST | HOST_CTRL0_SIDDQ);
			
	/* host link reset */
	phyhost |= HOST_CTRL0_LINK_SWRST | HOST_CTRL0_UTMI_SWRST |
		HOST_CTRL0_COMMONON_N;

	/* do the reset */
	bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHY_HOST_CTRL0, phyhost);
	DELAY(10000);
	phyhost &= ~(HOST_CTRL0_LINK_SWRST | HOST_CTRL0_UTMI_SWRST);
	bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHY_HOST_CTRL0, phyhost);

	/* otg configuration: */
	phyotg = bus_space_read_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHY_OTG_SYS);

	/* otg phy refrence clock: assumption its 24 Mhz now */
	phyotg &= ~OTG_SYS_FSEL_MASK;
	phyotg |= __SHIFTIN(OTG_SYS_FSEL_MASK, FSEL_CLKSEL_24M);

	/* enable normal mode of operation */
	phyotg &= ~(OTG_SYS_FORCESUSPEND | OTG_SYS_FORCESLEEP |
		OTG_SYS_SIDDQ_UOTG | OTG_SYS_REFCLKSEL_MASK |
		OTG_SYS_COMMON_ON);

	/* OTG phy and link reset */
	phyotg |= OTG_SYS_PHY0_SWRST | OTG_SYS_PHYLINK_SWRST |
		OTG_SYS_OTGDISABLE | OTG_SYS_REFCLKSEL_MASK;

	/* do the reset */
	bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHY_OTG_SYS, phyotg);
	DELAY(10000);
	phyotg &= ~(OTG_SYS_PHY0_SWRST | OTG_SYS_LINK_SWRST_UOTG |
		OTG_SYS_PHYLINK_SWRST);
	bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHY_OTG_SYS, phyotg);

	/* HSIC phy configuration: */
	phyhsic = REFCLKDIV_12 | REFCLKSEL_HSIC_DEFAULT |
		HSIC_CTRL_PHY_SWRST;

	bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHY_HSIC_CTRL1, phyhsic);
	bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHY_HSIC_CTRL2, phyhsic);
	DELAY(10000);
	phyhsic &= ~HSIC_CTRL_PHY_SWRST;
	bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHY_HSIC_CTRL1, phyhsic);
	bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHY_HSIC_CTRL2, phyhsic);

	DELAY(80000);

	/* enable EHCI DMA burst: */
	ehcictrl = bus_space_read_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHY_HOST_EHCICTRL);
	ehcictrl |= HOST_EHCICTRL_ENA_INCRXALIGN |
		HOST_EHCICTRL_ENA_INCR4 | HOST_EHCICTRL_ENA_INCR8 |
		HOST_EHCICTRL_ENA_INCR16;
	bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHY_HOST_EHCICTRL, ehcictrl);

	/* suspend OHCI legacy (?) */
	ohcictrl = bus_space_read_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHY_HOST_OHCICTRL);
	ohcictrl |= HOST_OHCICTRL_SUSPLGCY;
	bus_space_write_4(sc->sc_bst, sc->sc_usb2phy_bsh,
		USB_PHY_HOST_OHCICTRL, ohcictrl);
	DELAY(10000);
}


static void
exynos5422_usb2phy_enable(struct exynos_usb_softc *sc)
{
	aprint_error_dev(sc->sc_self, "%s not implemented\n", __func__);
}
#endif


static void
exynos_usb_phy_init(struct exynos_usb_softc *sc)
{
	/* disable phy isolation */
	exynos_usb2_set_isolation(sc, false);

#ifdef EXYNOS4
	if (IS_EXYNOS4_P())
		exynos4_usb2phy_enable(sc);
#endif
#ifdef EXYNOS5
	if (IS_EXYNOS5410_P()) {
		exynos5410_usb2phy_enable(sc);
		/* TBD: USB3 phy init */
	}
	if (IS_EXYNOS5422_P()) {
		exynos5422_usb2phy_enable(sc);
		/* TBD: USB3 phy init */
	}
	/* XXX exynos5422 */
#endif
}

