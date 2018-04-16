/* $NetBSD: omap3_ehci.c,v 1.12.14.1 2018/04/16 01:59:53 pgoyette Exp $ */

/*-
 * Copyright (c) 2010-2012 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: omap3_ehci.c,v 1.12.14.1 2018/04/16 01:59:53 pgoyette Exp $");

#include "locators.h"

#include "opt_omap.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/gpio.h>

#include <machine/intr.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#include <arm/omap/omap_var.h>
#include <arm/omap/omap2_gpio.h>
#include <arm/omap/omap2_obioreg.h>
#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/omap2_reg.h>
#include <arm/omap/omap3_usbtllreg.h>
#include <arm/omap/omap3_uhhreg.h>

/* CORE_CM */
#define	CORE_CM_BASE		(OMAP2_CM_BASE + 0x0a00)
#define	CORE_CM_SIZE		0x2000

/*  CORE_CM registers */
#define CM_FCLKEN1_CORE		0x00
#define CM_FCLKEN3_CORE		0x08
#define  EN_USBTLL		4	/* USB TLL clock enable */
	 /* also valid for CM_ICLKEN3_CORE */
#define CM_ICLKEN1_CORE		0x10
#define CM_ICLKEN3_CORE		0x18
#define CM_IDLEST1_CORE		0x20
#define CM_IDLEST3_CORE		0x28
#define CM_AUTOIDLE1_CORE	0x30
#define CM_AUTOIDLE3_CORE	0x38
#define  AUTO_USBTLL		4	/* USB TLL auto enable/disable */
#define CM_CLKSEL_CORE		0x40
#define CM_CLKSTCTRL_CORE	0x48
#define CM_CLKSTST_CORE		0x4c

/* Clock_Control_Reg_CM */
#define	CCR_CM_BASE		(OMAP2_CM_BASE + 0x0d00)
#define	CCR_CM_SIZE		0x800

/*  DPLL clock control registers */
#define	CM_CLKEN2_PLL		0x04
#define	CM_IDLEST2_CKGEN 	0x24
#define  ST_PERIPH2_CLK		1	/* DPLL5 is locked */
#define	CM_CLKSEL4_PLL		0x4c
#define	CM_CLKSEL5_PLL		0x50
#define	CM_AUTOIDLE2_PLL	0x34
#define  AUTO_PERIPH2_DPLL	1	/* DPLL5 automatic control */

/* USBHOST_CM */
#define USBHOST_CM_BASE 	(OMAP2_CM_BASE + 0x1400)
#define USBHOST_CM_SIZE 	0x2000

/*  USBHOST_CM registers */
#define CM_FCLKEN_USBHOST	0x00
#define  EN_USBHOST1		 1	/* USB HOST 48 MHz clock enable */
#define  EN_USBHOST2		 2	/* USB HOST 120 MHz clock enable */
#define CM_ICLKEN_USBHOST	0x10
#define  EN_USBHOST		 1	/* USB HOST clock enable */
#define CM_IDLEST_USBHOST	0x20
#define CM_AUTOIDLE_USBHOST	0x30
#define CM_SLEEPDEP_USBHOST	0x44
#define CM_CLKSTCTRL_USBHOST	0x48
#define CM_CLKSTST_USBHOST	0x4c

/* USBTLL module */
#ifdef OMAP_3XXX
#define	USBTLL_BASE		0x48062000
#elif defined(OMAP4) || defined(OMAP5)
#define	USBTLL_BASE		0x4a062000
#endif
#define	USBTLL_SIZE		0x1000

/* HS USB HOST module */
#ifdef OMAP_3XXX
#define	UHH_BASE		0x48064000
#elif defined(OMAP4) || defined(OMAP5)
#define	UHH_BASE		0x4a064000
#endif
#define	UHH_SIZE		0x1000

enum omap3_ehci_port_mode {
	OMAP3_EHCI_PORT_MODE_NONE,
	OMAP3_EHCI_PORT_MODE_PHY,
	OMAP3_EHCI_PORT_MODE_TLL,
	OMAP3_EHCI_PORT_MODE_HSIC,
};

static const uint32_t uhh_map[3][4] = {
#if defined(OMAP4) || defined(OMAP5)
	{
		[OMAP3_EHCI_PORT_MODE_NONE] =
		    __SHIFTIN(UHH_HOSTCONFIG_PMODE_UTMI, UHH_HOSTCONFIG_P1_MODE),
		[OMAP3_EHCI_PORT_MODE_PHY] =
		    __SHIFTIN(UHH_HOSTCONFIG_PMODE_ULPI_PHY, UHH_HOSTCONFIG_P1_MODE),
		[OMAP3_EHCI_PORT_MODE_TLL] =
		    __SHIFTIN(UHH_HOSTCONFIG_PMODE_UTMI, UHH_HOSTCONFIG_P1_MODE),
		[OMAP3_EHCI_PORT_MODE_HSIC] =
		    __SHIFTIN(UHH_HOSTCONFIG_PMODE_HSIC, UHH_HOSTCONFIG_P1_MODE),
	}, {
		[OMAP3_EHCI_PORT_MODE_NONE] =
		    __SHIFTIN(UHH_HOSTCONFIG_PMODE_UTMI, UHH_HOSTCONFIG_P2_MODE),
		[OMAP3_EHCI_PORT_MODE_PHY] =
		    __SHIFTIN(UHH_HOSTCONFIG_PMODE_ULPI_PHY, UHH_HOSTCONFIG_P2_MODE),
		[OMAP3_EHCI_PORT_MODE_TLL] =
		    __SHIFTIN(UHH_HOSTCONFIG_PMODE_UTMI, UHH_HOSTCONFIG_P2_MODE),
		[OMAP3_EHCI_PORT_MODE_HSIC] =
		    __SHIFTIN(UHH_HOSTCONFIG_PMODE_HSIC, UHH_HOSTCONFIG_P2_MODE),
	}, {
		[OMAP3_EHCI_PORT_MODE_NONE] =
		    __SHIFTIN(UHH_HOSTCONFIG_PMODE_UTMI, UHH_HOSTCONFIG_P3_MODE),
		[OMAP3_EHCI_PORT_MODE_PHY] =
		    __SHIFTIN(UHH_HOSTCONFIG_PMODE_ULPI_PHY, UHH_HOSTCONFIG_P3_MODE),
		[OMAP3_EHCI_PORT_MODE_TLL] =
		    __SHIFTIN(UHH_HOSTCONFIG_PMODE_UTMI, UHH_HOSTCONFIG_P3_MODE),
		[OMAP3_EHCI_PORT_MODE_HSIC] =
		    __SHIFTIN(UHH_HOSTCONFIG_PMODE_HSIC, UHH_HOSTCONFIG_P3_MODE),
	}
#else
	{
		[OMAP3_EHCI_PORT_MODE_NONE] = UHH_HOSTCONFIG_P1_ULPI_BYPASS,
		[OMAP3_EHCI_PORT_MODE_PHY]  = 0,
		[OMAP3_EHCI_PORT_MODE_TLL]  = UHH_HOSTCONFIG_P1_ULPI_BYPASS,
		[OMAP3_EHCI_PORT_MODE_HSIC] = UHH_HOSTCONFIG_P1_ULPI_BYPASS,
	}, {
		[OMAP3_EHCI_PORT_MODE_NONE] = UHH_HOSTCONFIG_P2_ULPI_BYPASS,
		[OMAP3_EHCI_PORT_MODE_PHY]  = 0,
		[OMAP3_EHCI_PORT_MODE_TLL]  = UHH_HOSTCONFIG_P2_ULPI_BYPASS,
		[OMAP3_EHCI_PORT_MODE_HSIC] = UHH_HOSTCONFIG_P2_ULPI_BYPASS,
	}, {
		[OMAP3_EHCI_PORT_MODE_NONE] = UHH_HOSTCONFIG_P3_ULPI_BYPASS,
		[OMAP3_EHCI_PORT_MODE_PHY]  = 0,
		[OMAP3_EHCI_PORT_MODE_TLL]  = UHH_HOSTCONFIG_P3_ULPI_BYPASS,
		[OMAP3_EHCI_PORT_MODE_HSIC] = UHH_HOSTCONFIG_P3_ULPI_BYPASS,
	},
#endif
};

struct omap3_ehci_softc {
	ehci_softc_t	sc;
	void		*sc_ih;

	bus_space_handle_t sc_ioh_usbtll;
	bus_size_t	sc_usbtll_size;

	bus_space_handle_t sc_ioh_uhh;
	bus_size_t	sc_uhh_size;

	uint16_t	sc_nports;
	bool		sc_phy_reset;
	struct {
		enum omap3_ehci_port_mode mode;
		int gpio;
		bool value;
		bool extclk;
	} sc_portconfig[3];
	struct {
		uint16_t m, n, m2;
	} sc_dpll5;
};

#ifdef OMAP_3XXX
static void	omap3_dpll5_init(struct omap3_ehci_softc *);
static void	omap3_usbhost_init(struct omap3_ehci_softc *, int);
#endif
#if defined(OMAP4) || defined(OMAP5)
static void	omap4_usbhost_init(struct omap3_ehci_softc *, int);
#endif
static void	usbtll_reset(struct omap3_ehci_softc *);
static void	usbtll_power(struct omap3_ehci_softc *, bool);
static void	usbtll_init(struct omap3_ehci_softc *, int);
static void	uhh_power(struct omap3_ehci_softc *, bool);
static void	uhh_portconfig(struct omap3_ehci_softc *);

#define	USBTLL_READ4(sc, reg)		bus_space_read_4((sc)->sc.iot, (sc)->sc_ioh_usbtll, (reg))
#define	USBTLL_WRITE4(sc, reg, v)	bus_space_write_4((sc)->sc.iot, (sc)->sc_ioh_usbtll, (reg), (v))
#define	UHH_READ4(sc, reg)	bus_space_read_4((sc)->sc.iot, (sc)->sc_ioh_uhh, (reg))
#define	UHH_WRITE4(sc, reg, v)	bus_space_write_4((sc)->sc.iot, (sc)->sc_ioh_uhh, (reg), (v))

/* Table 23-55 "EHCI Registers Mapping Summary" */
#define EHCI_INSNREG00		0x90
#define EHCI_INSNREG01		0x94
#define EHCI_INSNREG02		0x98
#define EHCI_INSNREG03		0x9c
#define EHCI_INSNREG04		0xa0
#define EHCI_INSNREG05_ULPI	0xa4
#define  ULPI_CONTROL		(1 << 31)
#define  ULPI_PORTSEL_SHIFT	24
#define  ULPI_PORTSEL_MASK	0xf
#define  ULPI_OPSEL_SHIFT	22
#define  ULPI_OPSEL_MASK	0x3
#define  ULPI_OPSEL_WRITE	0x2
#define	 ULPI_OPSEL_READ	0x3
#define  ULPI_REGADD_SHIFT	16
#define  ULPI_REGADD_MASK	0x3f
#define  ULPI_WRDATA_SHIFT	0
#define  ULPI_WRDATA_MASK	0xff
#define EHCI_INSNREG05_ITMI	0xa4

static void
omap3_ehci_phy_reset(struct omap3_ehci_softc *sc, unsigned int port)
{
	uint32_t reg, v;
	int retry = 1000;

	reg = ULPI_FUNCTION_CTRL_RESET |
	      (5 << ULPI_REGADD_SHIFT) |
	      (ULPI_OPSEL_WRITE << ULPI_OPSEL_SHIFT) |
	      (((port + 1) & ULPI_PORTSEL_MASK) << ULPI_PORTSEL_SHIFT) |
	      ULPI_CONTROL;
	EOWRITE4(&sc->sc, EHCI_INSNREG05_ULPI, reg);

	v = EOREAD4(&sc->sc, EHCI_INSNREG05_ULPI);
	while (v & (1 << 31) && --retry > 0) {
		delay(10000);
		v = EOREAD4(&sc->sc, EHCI_INSNREG05_ULPI);
	}
	if (retry == 0)
		aprint_error_dev(sc->sc.sc_dev, "phy reset timeout, port = %d\n", port);
}

static void
omap3_ehci_find_companions(struct omap3_ehci_softc *sc)
{
	device_t dv;
	deviter_t di;

	sc->sc.sc_ncomp = 0;
	for (dv = deviter_first(&di, DEVITER_F_ROOT_FIRST);
	     dv != NULL;
	     dv = deviter_next(&di)) {
		if (device_is_a(dv, "ohci") &&
		    device_parent(dv) == device_parent(sc->sc.sc_dev)) {
#ifdef OMAP3_EHCI_DEBUG
			aprint_normal("  adding companion '%s'\n", device_xname(dv));
#endif
			sc->sc.sc_comps[sc->sc.sc_ncomp++] = dv;
		}
	}
	deviter_release(&di);
}

static void
omap3_ehci_attach1(device_t self)
{
	struct omap3_ehci_softc *sc = device_private(self);
	int err;

	for (u_int i = 0; sc->sc_phy_reset && i < sc->sc_nports; i++) {
		if (sc->sc_portconfig[i].gpio != -1) {
			if (!omap2_gpio_has_pin(sc->sc_portconfig[i].gpio)) {
				aprint_error_dev(self, "WARNING: "
				    "gpio pin %d not available\n",
				    sc->sc_portconfig[i].gpio);
				continue;
			}
			omap2_gpio_ctl(sc->sc_portconfig[i].gpio, GPIO_PIN_OUTPUT);
			omap2_gpio_write(sc->sc_portconfig[i].gpio,
			    !sc->sc_portconfig[i].value); // off
			delay(10);
		}
	}

	usbtll_power(sc, true);
	usbtll_init(sc, 3);

	uhh_power(sc, true);
	uhh_portconfig(sc);

	for (u_int i = 0; i < sc->sc_nports; i++) {
		if (sc->sc_portconfig[i].mode == OMAP3_EHCI_PORT_MODE_PHY) {
			omap3_ehci_phy_reset(sc, i);
		}
	}

	delay(10);

	for (u_int i = 0; sc->sc_phy_reset && i < sc->sc_nports; i++) {
		if (sc->sc_portconfig[i].gpio != -1) {
			if (!omap2_gpio_has_pin(sc->sc_portconfig[i].gpio))
				continue;
			omap2_gpio_ctl(sc->sc_portconfig[i].gpio, GPIO_PIN_OUTPUT);
			omap2_gpio_write(sc->sc_portconfig[i].gpio,
			    sc->sc_portconfig[i].value);
			delay(10);
		}
	}

	omap3_ehci_find_companions(sc);

	err = ehci_init(&sc->sc);
	if (err) {
		aprint_error_dev(self, "init failed, error = %d\n", err);
		return;
	}

	sc->sc.sc_child = config_found(self, &sc->sc.sc_bus, usbctlprint);
}

static int
omap3_ehci_match(device_t parent, cfdata_t match, void *opaque)
{
#ifdef OMAP3
	struct obio_attach_args *obio = opaque;
#endif

#ifdef OMAP_3XXX
	if (obio->obio_addr == EHCI1_BASE_OMAP3)
		return 1;
#endif
#if defined(OMAP4) || defined(OMAP5)
	if (obio->obio_addr == EHCI1_BASE_OMAP4)
		return 1;
#endif

	return 0;
}

static enum omap3_ehci_port_mode
omap3_ehci_get_port_mode(prop_dictionary_t prop, const char *key)
{
	const char *s = NULL;
	enum omap3_ehci_port_mode mode = OMAP3_EHCI_PORT_MODE_NONE;

	if (prop_dictionary_get_cstring_nocopy(prop, key, &s) && s != NULL) {
		if (strcmp(s, "phy") == 0) {
			mode = OMAP3_EHCI_PORT_MODE_PHY;
#ifdef OMAP_3XXX
		} else if (strcmp(s, "tll") == 0) {
			mode = OMAP3_EHCI_PORT_MODE_TLL;
#endif
#if defined(OMAP4) || defined(OMAP5)
		} else if (strcmp(s, "hsic") == 0) {
			mode = OMAP3_EHCI_PORT_MODE_HSIC;
#endif
		} else if (strcmp(s, "none") == 0) {
			mode = OMAP3_EHCI_PORT_MODE_NONE;
		} else {
			panic("%s: unknown port mode %s", __func__, s);
		}
	}

	return mode;
}

static int
omap3_ehci_get_port_gpio(prop_dictionary_t prop, const char *key)
{
	int16_t gpio;

	if (prop_dictionary_get_int16(prop, key, &gpio) == false)
		gpio = -1;

	return gpio;
}

static void
omap3_ehci_parse_properties(struct omap3_ehci_softc *sc, prop_dictionary_t prop)
{
	prop_dictionary_get_uint16(prop, "nports", &sc->sc_nports);
	prop_dictionary_get_bool(prop, "phy-reset", &sc->sc_phy_reset);
	sc->sc_portconfig[0].mode = omap3_ehci_get_port_mode(prop, "port0-mode");
	sc->sc_portconfig[0].gpio = omap3_ehci_get_port_gpio(prop, "port0-gpio");
	prop_dictionary_get_bool(prop, "port0-gpioval", &sc->sc_portconfig[0].value);
#if defined(OMAP4) || defined(OMAP5)
	prop_dictionary_get_bool(prop, "port0-extclk", &sc->sc_portconfig[0].extclk);
#endif
	if (sc->sc_nports > 1) {
		sc->sc_portconfig[1].mode = omap3_ehci_get_port_mode(prop, "port1-mode");
		sc->sc_portconfig[1].gpio = omap3_ehci_get_port_gpio(prop, "port1-gpio");
		prop_dictionary_get_bool(prop, "port1-gpioval", &sc->sc_portconfig[1].value);
#if defined(OMAP4) || defined(OMAP5)
		prop_dictionary_get_bool(prop, "port1-extclk", &sc->sc_portconfig[1].extclk);
#endif
	}
	if (sc->sc_nports > 2) {
		sc->sc_portconfig[2].mode = omap3_ehci_get_port_mode(prop, "port2-mode");
		sc->sc_portconfig[2].gpio = omap3_ehci_get_port_gpio(prop, "port2-gpio");
		prop_dictionary_get_bool(prop, "port2-gpioval", &sc->sc_portconfig[2].value);
#if defined(OMAP4) || defined(OMAP5)
		prop_dictionary_get_bool(prop, "port2-extclk", &sc->sc_portconfig[2].extclk);
#endif
	}

#ifdef OMAP_3XXX
	prop_dictionary_get_uint16(prop, "dpll5-m", &sc->sc_dpll5.m);
	prop_dictionary_get_uint16(prop, "dpll5-n", &sc->sc_dpll5.n);
	prop_dictionary_get_uint16(prop, "dpll5-m2", &sc->sc_dpll5.m2);
#endif

#ifdef OMAP3_EHCI_DEBUG
	printf("  GPIO PHY reset: %d\n", sc->sc_phy_reset);
	printf("  Port #0: mode %d, gpio %d\n", sc->sc_portconfig[0].mode, sc->sc_portconfig[0].gpio);
	printf("  Port #1: mode %d, gpio %d\n", sc->sc_portconfig[1].mode, sc->sc_portconfig[1].gpio);
	printf("  Port #2: mode %d, gpio %d\n", sc->sc_portconfig[2].mode, sc->sc_portconfig[2].gpio);
#ifdef OMAP_3XXX
	printf("  DPLL5: m=%d n=%d m2=%d\n", sc->sc_dpll5.m, sc->sc_dpll5.n, sc->sc_dpll5.m2);
#endif
#endif
}

static void
omap3_ehci_attach(device_t parent, device_t self, void *opaque)
{
	struct omap3_ehci_softc *sc = device_private(self);
	struct obio_attach_args *obio = opaque;
	int rv;

	sc->sc.sc_dev = self;
	sc->sc.sc_bus.ub_hcpriv = sc;

	aprint_naive("\n");
	aprint_normal(": OMAP USB controller\n");

	omap3_ehci_parse_properties(sc, device_properties(self));

	sc->sc.iot = obio->obio_iot;
	rv = bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size, 0, &sc->sc.ioh);
	if (rv) {
		aprint_error_dev(self, "couldn't map memory space\n");
		return;
	}
	sc->sc.sc_size = obio->obio_size;
	rv = bus_space_map(obio->obio_iot, USBTLL_BASE, USBTLL_SIZE, 0, &sc->sc_ioh_usbtll);
	if (rv) {
		aprint_error_dev(self, "couldn't map usbtll memory space\n");
		return;
	}
	sc->sc_usbtll_size = USBTLL_SIZE;
	rv = bus_space_map(obio->obio_iot, UHH_BASE, UHH_SIZE, 0, &sc->sc_ioh_uhh);
	if (rv) {
		aprint_error_dev(self, "couldn't map uhh memory space\n");
		return;
	}
	sc->sc_uhh_size = UHH_SIZE;
	sc->sc.sc_bus.ub_dmatag = obio->obio_dmat;
	sc->sc.sc_bus.ub_revision = USBREV_2_0;

#ifdef OMAP_3XXX
	omap3_dpll5_init(sc);

	omap3_usbhost_init(sc, 1);
#endif /* OMAP_3XXX */
#if defined(OMAP4) || defined(OMAP5)
	omap4_usbhost_init(sc, 1);
#endif

	sc->sc.sc_offs = EREAD1(&sc->sc, EHCI_CAPLENGTH);

	//EOWRITE4(&sc->sc, EHCI_INSNREG04, 1 << 5);

	sc->sc_ih = intr_establish(obio->obio_intr, IPL_USB, IST_LEVEL, ehci_intr, &sc->sc);

	//config_interrupts(self, omap3_ehci_attach1);
	omap3_ehci_attach1(self);
}

static int
omap3_ehci_detach(device_t self, int flags)
{
	struct omap3_ehci_softc *sc = device_private(self);
	int rv;

	rv = ehci_detach(&sc->sc, flags);
	if (rv)
		return rv;

	EOWRITE2(&sc->sc, EHCI_USBINTR, 0);
	EOREAD2(&sc->sc, EHCI_USBINTR);

	if (sc->sc_ih) {
		intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}

	usbtll_power(sc, false);

	if (sc->sc_usbtll_size) {
		bus_space_unmap(sc->sc.iot, sc->sc_ioh_usbtll, sc->sc_usbtll_size);
		sc->sc_usbtll_size = 0;
	}

	if (sc->sc_uhh_size) {
		bus_space_unmap(sc->sc.iot, sc->sc_ioh_uhh, sc->sc_uhh_size);
		sc->sc_uhh_size = 0;
	}

	if (sc->sc.sc_size) {
		bus_space_unmap(sc->sc.iot, sc->sc.ioh, sc->sc.sc_size);
		sc->sc.sc_size = 0;
	}

	return 0;
}

#ifdef OMAP_3XXX
static void
omap3_dpll5_init(struct omap3_ehci_softc *sc)
{
	bus_space_tag_t iot = sc->sc.iot;
	bus_space_handle_t ioh;
	int err;

	if (sc->sc_dpll5.m == 0 || sc->sc_dpll5.n == 0 || sc->sc_dpll5.m2 == 0)
		return;

	err = bus_space_map(iot, CCR_CM_BASE, CCR_CM_SIZE, 0, &ioh);
	if (err)
                panic("%s: cannot map CCR_CM_BASE at %#x, error %d\n",
                        __func__, CCR_CM_BASE, err);

	/* set the multiplier and divider values for the desired CLKOUT freq */
	uint32_t m = sc->sc_dpll5.m;
	uint32_t n = sc->sc_dpll5.n;
	/* set the corresponding output dividers */
	uint32_t m2 = sc->sc_dpll5.m2;

	KASSERTMSG(479900000 <= 2 * m * (omap_sys_clk / ((n + 1) * m2)),
	    "m=%u n=%u m2=%u freq=%u",
	    m, n, m2, 2 * m * (omap_sys_clk / ((n + 1) * m2)));
	KASSERTMSG(2 * m * (omap_sys_clk / ((n + 1) * m2)) <= 480100000,
	    "m=%u n=%u m2=%u freq=%u",
	    m, n, m2, 2 * m * (omap_sys_clk / ((n + 1) * m2)));

	/* 4.7.6.2
	 * In the DPLL programming sequence, the DPLL_FREQSEL must be
	 * programmed before the new Multiplier factor M and the Divider
	 * factor N are programmed so that the new value is taken into
	 * account during current DPLL relock.
	 */
	bus_space_write_4(iot, ioh, CM_CLKEN2_PLL, (0x4 << 4) | 0x7);

	bus_space_write_4(iot, ioh, CM_CLKSEL4_PLL, (m << 8) | n);
	bus_space_write_4(iot, ioh, CM_CLKSEL5_PLL, m2);

	/*
	 * Put DPLL5 into low power stop mode when the 120MHz clock
	 * is not required (restarted automatically)
	 */
	bus_space_write_4(iot, ioh, CM_AUTOIDLE2_PLL, AUTO_PERIPH2_DPLL);

	bus_space_unmap(iot, ioh, CCR_CM_SIZE);
}

static void
omap3_usbhost_init(struct omap3_ehci_softc *sc, int enable)
{
	bus_space_tag_t iot = sc->sc.iot;
        bus_space_handle_t ioh;
        uint32_t r;
        int err;

	/*
	 * USBHOST
	 */
        err = bus_space_map(iot, USBHOST_CM_BASE, USBHOST_CM_SIZE, 0, &ioh);
        if (err)
                panic("%s: cannot map USBHOST_CM_BASE at %#x, error %d\n",
                        __func__, USBHOST_CM_BASE, err);

        r = bus_space_read_4(iot, ioh, CM_FCLKEN_USBHOST);
        if (enable)
                r |= (EN_USBHOST1 | EN_USBHOST2);
        else
                r &= ~(EN_USBHOST1 | EN_USBHOST2);
        bus_space_write_4(iot, ioh, CM_FCLKEN_USBHOST, r);

        r = bus_space_read_4(iot, ioh, CM_ICLKEN_USBHOST);
        if (enable)
		r |= EN_USBHOST;
        else
                r &= ~EN_USBHOST;
        bus_space_write_4(iot, ioh, CM_ICLKEN_USBHOST, r);

        bus_space_unmap(iot, ioh, USBHOST_CM_SIZE);

	delay(10000);

	/*
	 * USBTLL
	 */
	err = bus_space_map(iot, CORE_CM_BASE, CORE_CM_SIZE, 0, &ioh);
	if (err)
		panic("%s: cannot map CORE_CM_BASE a5 %#x, error %d\n",
			__func__, CORE_CM_BASE, err);

	r = bus_space_read_4(iot, ioh, CM_FCLKEN3_CORE);
	if (enable)
		r |= EN_USBTLL;
	else
		r &= ~EN_USBTLL;
	bus_space_write_4(iot, ioh, CM_FCLKEN3_CORE, r);

	r = bus_space_read_4(iot, ioh, CM_ICLKEN3_CORE);
	if (enable)
		r |= EN_USBTLL;
	else
		r &= ~EN_USBTLL;
	bus_space_write_4(iot, ioh, CM_ICLKEN3_CORE, r);

	r = bus_space_read_4(iot, ioh, CM_AUTOIDLE3_CORE);
	if (enable)
		r &= ~AUTO_USBTLL;
	else
		r |= AUTO_USBTLL;
	bus_space_write_4(iot, ioh, CM_AUTOIDLE3_CORE, r);

	bus_space_unmap(iot, ioh, CORE_CM_SIZE);

	delay(10000);

#undef USBHOST_CM_SIZE
#undef USBHOST_CM_BASE
#undef CORE_CM_SIZE
#undef CORE_CM_BASE
}
#endif /* OMAP_3XXX */

#if defined(OMAP4) || defined(OMAP5)
static void
omap4_usbhost_init(struct omap3_ehci_softc *sc, int enable)
{
	bus_space_tag_t iot = sc->sc.iot;
        bus_space_handle_t ioh;
	uint32_t val;
	int err __diagused;
#ifdef OMAP5
	bus_size_t off = OMAP5_CM_L3INIT_CORE;
#elif defined(OMAP4)
	bus_size_t off = OMAP4_CM_L3INIT_CORE;
#endif

	err = bus_space_map(iot, OMAP2_CM_BASE + off, 0x100, 0, &ioh);
	KASSERT(err == 0);

	val = bus_space_read_4(iot, ioh, OMAP4_CM_L3INIT_USB_HOST_HS_CLKCTRL);
	if (sc->sc_portconfig[0].mode != OMAP3_EHCI_PORT_MODE_NONE) {
		if (sc->sc_portconfig[0].extclk)
			val |= OMAP4_CM_L3INIT_USB_HOST_HS_CLKCTRL_CLKSEL_UTMI_P1;
		else
			val |= OMAP4_CM_L3INIT_USB_HOST_HS_CLKCTRL_OPTFCLKEN_UTMI_P1_CLK;
		if (sc->sc_portconfig[0].mode == OMAP3_EHCI_PORT_MODE_HSIC)
			val |= OMAP4_CM_L3INIT_USB_HOST_HS_CLKCTRL_OPTFCLKEN_HSIC60M_P1_CLK
			    |  OMAP4_CM_L3INIT_USB_HOST_HS_CLKCTRL_OPTFCLKEN_HSIC480M_P1_CLK;
	}
	if (sc->sc_nports > 1
	    && sc->sc_portconfig[1].mode != OMAP3_EHCI_PORT_MODE_NONE) {
		if (sc->sc_portconfig[1].extclk)
			val |= OMAP4_CM_L3INIT_USB_HOST_HS_CLKCTRL_CLKSEL_UTMI_P2;
		else
			val |= OMAP4_CM_L3INIT_USB_HOST_HS_CLKCTRL_OPTFCLKEN_UTMI_P2_CLK;
		if (sc->sc_portconfig[1].mode == OMAP3_EHCI_PORT_MODE_HSIC)
			val |= OMAP4_CM_L3INIT_USB_HOST_HS_CLKCTRL_OPTFCLKEN_HSIC60M_P2_CLK
			    |  OMAP4_CM_L3INIT_USB_HOST_HS_CLKCTRL_OPTFCLKEN_HSIC480M_P2_CLK;
	}
	if (sc->sc_nports > 2
	    && sc->sc_portconfig[2].mode != OMAP3_EHCI_PORT_MODE_NONE) {
		val |= OMAP4_CM_L3INIT_USB_HOST_HS_CLKCTRL_OPTFCLKEN_UTMI_P3_CLK;
		if (sc->sc_portconfig[2].mode == OMAP3_EHCI_PORT_MODE_HSIC)
			val |= OMAP4_CM_L3INIT_USB_HOST_HS_CLKCTRL_OPTFCLKEN_HSIC60M_P3_CLK
			    |  OMAP4_CM_L3INIT_USB_HOST_HS_CLKCTRL_OPTFCLKEN_HSIC480M_P3_CLK;
	}
	bus_space_write_4(iot, ioh, OMAP4_CM_L3INIT_USB_HOST_HS_CLKCTRL, val);

	val = bus_space_read_4(iot, ioh, OMAP4_CM_L3INIT_USB_TLL_HS_CLKCTRL);
	if (sc->sc_portconfig[0].mode != OMAP3_EHCI_PORT_MODE_NONE)
		val |= OMAP4_CM_L3INIT_USB_TLL_HS_CLKCTRL_USB_CH0_CLK;
	if (sc->sc_nports > 1
	    && sc->sc_portconfig[1].mode != OMAP3_EHCI_PORT_MODE_NONE)
		val |= OMAP4_CM_L3INIT_USB_TLL_HS_CLKCTRL_USB_CH1_CLK;
	if (sc->sc_nports > 2
	    && sc->sc_portconfig[2].mode != OMAP3_EHCI_PORT_MODE_NONE)
		val |= OMAP4_CM_L3INIT_USB_TLL_HS_CLKCTRL_USB_CH2_CLK;
	bus_space_write_4(iot, ioh, OMAP4_CM_L3INIT_USB_TLL_HS_CLKCTRL, val);

	bus_space_unmap(iot, ioh, 0x100);
}
#endif /* OMAP4 || OMAP5 */
static void
usbtll_reset(struct omap3_ehci_softc *sc)
{
	uint32_t v;
	int retry = 5000;

	USBTLL_WRITE4(sc, USBTLL_SYSCONFIG, USBTLL_SYSCONFIG_SOFTRESET);
	do {
		v = USBTLL_READ4(sc, USBTLL_SYSSTATUS);
		if (v & USBTLL_SYSSTATUS_RESETDONE)
			break;
		delay(10);
	} while (retry-- > 0);
	if (retry == 0)
		aprint_error_dev(sc->sc.sc_dev,
		    "reset timeout, status = 0x%08x\n", v);
}

static void
usbtll_power(struct omap3_ehci_softc *sc, bool on)
{
	uint32_t v;

	usbtll_reset(sc);

	if (on) {
		v = USBTLL_SYSCONFIG_ENAWAKEUP |
		    USBTLL_SYSCONFIG_AUTOIDLE |
		    USBTLL_SYSCONFIG_SIDLEMODE |
		    USBTLL_SYSCONFIG_CLOCKACTIVITY;
		USBTLL_WRITE4(sc, USBTLL_SYSCONFIG, v);
	}
}

static void
usbtll_init(struct omap3_ehci_softc *sc, int chmask)
{
	uint32_t v;

	v = USBTLL_READ4(sc, USBTLL_SHARED_CONF);
	v |= (USBTLL_SHARED_CONF_FCLK_IS_ON | (1 << 2)/*divration*/);
	v &= ~USBTLL_SHARED_CONF_USB_90D_DDR_EN;
	v &= ~USBTLL_SHARED_CONF_USB_180D_SDR_EN;
	USBTLL_WRITE4(sc, USBTLL_SHARED_CONF, v);

	for (u_int i = 0; i < sc->sc_nports; i++) {
		if (sc->sc_portconfig[i].mode == OMAP3_EHCI_PORT_MODE_NONE)
			continue;
		v = USBTLL_READ4(sc, USBTLL_CHANNEL_CONF(i));
		v &= ~(USBTLL_CHANNEL_CONF_ULPINOBITSTUFF|
		       USBTLL_CHANNEL_CONF_ULPIAUTOIDLE|
		       USBTLL_CHANNEL_CONF_ULPIDDRMODE);
		//v |= USBTLL_CHANNEL_CONF_FSLSMODE;
		v |= USBTLL_CHANNEL_CONF_CHANEN;
		USBTLL_WRITE4(sc, USBTLL_CHANNEL_CONF(i), v);
	}
}

static void
uhh_power(struct omap3_ehci_softc *sc, bool on)
{
	int retry = 5000;

	uint32_t v;

	v = UHH_READ4(sc, UHH_REVISION);
	const int vers = UHH_REVISION_MAJOR(v);
	if (on) {
		v = UHH_READ4(sc, UHH_SYSCONFIG);
		if (vers >= UHH_REVISION_VERS2) {
			v &= ~UHH4_SYSCONFIG_STANDBYMODE;
			v |= UHH4_SYSCONFIG_STANDBYMODE_SMARTSTANDBY;
			v &= ~UHH4_SYSCONFIG_SIDLEMODE;
			v |= UHH4_SYSCONFIG_SIDLEMODE_SMARTIDLE;
		} else {
			v &= ~UHH3_SYSCONFIG_MIDLEMODE_MASK;
			v |= UHH3_SYSCONFIG_MIDLEMODE_SMARTSTANDBY;
			v &= ~UHH3_SYSCONFIG_SIDLEMODE_MASK;
			v |= UHH3_SYSCONFIG_SIDLEMODE_SMARTIDLE;
			v |= UHH3_SYSCONFIG_CLOCKACTIVITY;
			v |= UHH3_SYSCONFIG_ENAWAKEUP;
			v &= ~UHH3_SYSCONFIG_AUTOIDLE;
		}
		UHH_WRITE4(sc, UHH_SYSCONFIG, v);

		v = UHH_READ4(sc, UHH_SYSCONFIG);
	} else {
		v = UHH_READ4(sc, UHH_SYSCONFIG);
		if (vers >= UHH_REVISION_VERS2) {
			v |= UHH4_SYSCONFIG_SOFTRESET;
		} else {
			v |= UHH3_SYSCONFIG_SOFTRESET;
		}
		UHH_WRITE4(sc, UHH_SYSCONFIG, v);
		do {
			v = UHH_READ4(sc, UHH_SYSSTATUS);
			if (vers >= UHH_REVISION_VERS2) {
				if ((v & UHH4_SYSSTATUS_RESETDONE_ALL) == 0)
					break;
			} else {
				if (v & UHH3_SYSSTATUS_RESETDONE_ALL)
					break;
			}
			delay(10);
		} while (retry-- > 0);
		if (retry == 0)
			aprint_error_dev(sc->sc.sc_dev,
			    "reset timeout, status = 0x%08x\n", v);
	}
}

static void
uhh_portconfig(struct omap3_ehci_softc *sc)
{
	uint32_t v;

	v = UHH_READ4(sc, UHH_HOSTCONFIG);
	v |= UHH_HOSTCONFIG_ENA_INCR16;
	v |= UHH_HOSTCONFIG_ENA_INCR8;
	v |= UHH_HOSTCONFIG_ENA_INCR4;
	v |= UHH_HOSTCONFIG_APP_START_CLK;
	v &= ~UHH_HOSTCONFIG_ENA_INCR_ALIGN;

	if (sc->sc_portconfig[0].mode == OMAP3_EHCI_PORT_MODE_NONE)
		v &= ~UHH_HOSTCONFIG_P1_CONNECT_STATUS;
	if (sc->sc_nports > 1
	    && sc->sc_portconfig[1].mode == OMAP3_EHCI_PORT_MODE_NONE)
		v &= ~UHH_HOSTCONFIG_P2_CONNECT_STATUS;
	if (sc->sc_nports > 2
	    && sc->sc_portconfig[2].mode == OMAP3_EHCI_PORT_MODE_NONE)
		v &= ~UHH_HOSTCONFIG_P3_CONNECT_STATUS;

	v &= ~(UHH_HOSTCONFIG_P1_ULPI_BYPASS|UHH_HOSTCONFIG_P2_ULPI_BYPASS
	    |UHH_HOSTCONFIG_P3_ULPI_BYPASS);
	v &= ~(UHH_HOSTCONFIG_P1_MODE|UHH_HOSTCONFIG_P2_MODE
	    |UHH_HOSTCONFIG_P3_MODE);

	v |= uhh_map[0][sc->sc_portconfig[0].mode];
	if (sc->sc_nports > 1) {
		v |= uhh_map[1][sc->sc_portconfig[1].mode];
		if (sc->sc_nports > 2) {
			v |= uhh_map[2][sc->sc_portconfig[2].mode];
		}
	}

	UHH_WRITE4(sc, UHH_HOSTCONFIG, v);
}

CFATTACH_DECL2_NEW(omap3_ehci, sizeof(struct omap3_ehci_softc),
    omap3_ehci_match,
    omap3_ehci_attach,
    omap3_ehci_detach,
    ehci_activate,
    NULL,
    ehci_childdet
);
