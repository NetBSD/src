/*	$NetBSD: exynos_usb.c,v 1.7.6.2 2014/08/20 00:02:47 tls Exp $	*/

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

__KERNEL_RCSID(1, "$NetBSD: exynos_usb.c,v 1.7.6.2 2014/08/20 00:02:47 tls Exp $");

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

#define EHCI_OFFSET	(0)
#define OHCI_OFFSET	(1*EXYNOS_BLOCK_SIZE)
#define DEVLINK_OFFSET	(2*EXYNOS_BLOCK_SIZE)	/* Exynos5 */
#define USB2PHY_OFFSET	(3*EXYNOS_BLOCK_SIZE)

struct exynos_usb_softc {
	device_t	 sc_self;

	/* keep our tags here */
	bus_dma_tag_t	 sc_dmat;
	bus_space_tag_t  sc_bst;

	bus_space_handle_t sc_ehci_bsh;
	bus_space_handle_t sc_ohci_bsh;
	bus_space_handle_t sc_usb2phy_bsh;

	bus_space_handle_t sc_pmuregs_bsh;

	device_t	 sc_ohci_dev;
	device_t	 sc_ehci_dev;

	int		 sc_irq;
	void		*sc_intrh;
} exynos_usb_sc;

struct exynos_usb_attach_args {
	const char *name;
};

static struct exynos_gpio_pinset uhost_pwr_pinset = {
	.pinset_group = "ETC6",
	.pinset_func  = 0,
	.pinset_mask  = __BIT(5) | __BIT(6),
};


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
	bus_size_t pmu_offset;

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

	bus_space_subregion(sc->sc_bst, exyoaa->exyo_core_bsh,
		loc->loc_offset + EHCI_OFFSET, EXYNOS_BLOCK_SIZE,
		&sc->sc_ehci_bsh);
	bus_space_subregion(sc->sc_bst, exyoaa->exyo_core_bsh,
		loc->loc_offset + OHCI_OFFSET, EXYNOS_BLOCK_SIZE,
		&sc->sc_ohci_bsh);
	bus_space_subregion(sc->sc_bst, exyoaa->exyo_core_bsh,
		loc->loc_offset + USB2PHY_OFFSET, EXYNOS_BLOCK_SIZE,
		&sc->sc_usb2phy_bsh);

#ifdef EXYNOS4
	if (IS_EXYNOS4_P())
		pmu_offset = EXYNOS4_PMU_OFFSET;
#endif
#ifdef EXYNOS5
	if (IS_EXYNOS5_P())
		pmu_offset = EXYNOS5_ALIVE_OFFSET;
#endif

	KASSERT(pmu_offset);
	bus_space_subregion(sc->sc_bst, exyoaa->exyo_core_bsh,
		pmu_offset, EXYNOS_BLOCK_SIZE,
		&sc->sc_pmuregs_bsh);

	aprint_naive("\n");
	aprint_normal("\n");

	/* power up USB subsystem XXX PWREN not working yet */
	exynos_gpio_pinset_acquire(&uhost_pwr_pinset);
	exynos_gpio_pinset_to_pindata(&uhost_pwr_pinset, 5, &XuhostPWREN);
	exynos_gpio_pinset_to_pindata(&uhost_pwr_pinset, 6, &XuhostOVERCUR);

	/* enable power and set Xuhost OVERCUR to inactive by pulling it up */
	exynos_gpio_pindata_ctl(&XuhostPWREN, GPIO_PIN_PULLUP);
	exynos_gpio_pindata_ctl(&XuhostOVERCUR, GPIO_PIN_PULLUP);
	DELAY(80000);

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

	/* init USB phys */
	exynos_usb_phy_init(sc);

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

static void
exynos_usb2_set_isolation(struct exynos_usb_softc *sc, bool on)
{
	int val;

	val = on ? PMU_PHY_DISABLE : PMU_PHY_ENABLE;
	if (IS_EXYNOS4X12_P()) {
		bus_space_write_4(sc->sc_bst, sc->sc_pmuregs_bsh,
			EXYNOS_PMU_USB_PHY_CTRL, val);
		bus_space_write_4(sc->sc_bst, sc->sc_pmuregs_bsh,
			EXYNOS_PMU_USB_HSIC_1_PHY_CTRL, val);
		bus_space_write_4(sc->sc_bst, sc->sc_pmuregs_bsh,
			EXYNOS_PMU_USB_HSIC_2_PHY_CTRL, val);
	} else {
		bus_space_write_4(sc->sc_bst, sc->sc_pmuregs_bsh,
			EXYNOS_PMU_USBDEV_PHY_CTRL, val);
		bus_space_write_4(sc->sc_bst, sc->sc_pmuregs_bsh,
			EXYNOS_PMU_USBHOST_PHY_CTRL, val);
	}
}


#ifdef EXYNOS4
static void
exynos4_usb2phy_enable(struct exynos_usb_softc *sc)
{
	uint32_t phypwr, rstcon, clkreg;

	/* disable phy isolation */
	exynos_usb2_set_isolation(sc, false);

	/* write clock value */
	clkreg = 5;	/* 24 Mhz */
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
exynos5_usb2phy_enable(struct exynos_usb_softc *sc)
{
	/* disable phy isolation */
	exynos_usb2_set_isolation(sc, false);

	aprint_error_dev(sc->sc_self, "%s not implemented\n", __func__);
}
#endif


static void
exynos_usb_phy_init(struct exynos_usb_softc *sc)
{
#ifdef EXYNOS4
	if (IS_EXYNOS4_P())
		exynos4_usb2phy_enable(sc);
#endif
#ifdef EXYNOS5
	if (IS_EXYNOS5_P())
		exynos5_usb2phy_enable(sc);
	/* TBD: USB3 phy init */
#endif
}

