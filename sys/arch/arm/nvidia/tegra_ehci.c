/* $NetBSD: tegra_ehci.c,v 1.8 2015/10/21 10:43:09 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_ehci.c,v 1.8 2015/10/21 10:43:09 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#include <arm/nvidia/tegra_var.h>
#include <arm/nvidia/tegra_ehcireg.h>

#define TEGRA_EHCI_REG_OFFSET	0x100

static int	tegra_ehci_match(device_t, cfdata_t, void *);
static void	tegra_ehci_attach(device_t, device_t, void *);

static void	tegra_ehci_init(struct ehci_softc *);

struct tegra_ehci_softc {
	struct ehci_softc	sc;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	void			*sc_ih;
	u_int			sc_port;

	struct tegra_gpio_pin	*sc_pin_vbus;
	uint8_t			sc_hssync_start_delay;
	uint8_t			sc_idle_wait_delay;
	uint8_t			sc_elastic_limit;
	uint8_t			sc_term_range_adj;
	uint8_t			sc_xcvr_setup;
	uint8_t			sc_xcvr_lsfslew;
	uint8_t			sc_xcvr_lsrslew;
	uint8_t			sc_hssquelch_level;
	uint8_t			sc_hsdiscon_level;
	uint8_t			sc_xcvr_hsslew;
};

static int	tegra_ehci_parse_properties(struct tegra_ehci_softc *);
static void	tegra_ehci_utmip_init(struct tegra_ehci_softc *);
static int	tegra_ehci_port_status(struct ehci_softc *sc, uint32_t v,
		    int i);

CFATTACH_DECL2_NEW(tegra_ehci, sizeof(struct tegra_ehci_softc),
	tegra_ehci_match, tegra_ehci_attach, NULL,
	ehci_activate, NULL, ehci_childdet);

static int
tegra_ehci_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_ehci_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_ehci_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;
	prop_dictionary_t prop = device_properties(self);
	const char *pin;
	int error;

	sc->sc_bst = tio->tio_bst;
	bus_space_subregion(tio->tio_bst, tio->tio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);
	sc->sc_port = loc->loc_port;

	sc->sc.sc_dev = self;
	sc->sc.sc_bus.hci_private = &sc->sc;
	sc->sc.sc_bus.dmatag = tio->tio_dmat;
	sc->sc.sc_bus.usbrev = USBREV_2_0;
	sc->sc.sc_ncomp = 0;
	sc->sc.sc_flags = EHCIF_ETTF;
	sc->sc.sc_id_vendor = 0x10de;
	strlcpy(sc->sc.sc_vendor, "Tegra", sizeof(sc->sc.sc_vendor));
	sc->sc.sc_size = loc->loc_size;
	sc->sc.iot = tio->tio_bst;
	bus_space_subregion(tio->tio_bst, tio->tio_bsh,
	    loc->loc_offset + TEGRA_EHCI_REG_OFFSET,
	    loc->loc_size - TEGRA_EHCI_REG_OFFSET, &sc->sc.ioh);
	sc->sc.sc_vendor_init = tegra_ehci_init;
	sc->sc.sc_vendor_port_status = tegra_ehci_port_status;

	aprint_naive("\n");
	aprint_normal(": USB%d\n", loc->loc_port + 1);

	if (tegra_ehci_parse_properties(sc) != 0)
		return;

	tegra_car_periph_usb_enable(sc->sc_port);
	delay(2);

	tegra_ehci_utmip_init(sc);

	if (prop_dictionary_get_cstring_nocopy(prop, "vbus-gpio", &pin)) {
		const uint32_t v = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    TEGRA_EHCI_PHY_VBUS_SENSORS_REG);
		if ((v & TEGRA_EHCI_PHY_VBUS_SENSORS_A_VBUS_VLD_STS) == 0) {
			sc->sc_pin_vbus = tegra_gpio_acquire(pin,
			    GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN);
			if (sc->sc_pin_vbus)
				tegra_gpio_write(sc->sc_pin_vbus, 1);
		} else {
			aprint_normal_dev(self, "VBUS input active\n");
		}
        }

	sc->sc.sc_offs = EREAD1(&sc->sc, EHCI_CAPLENGTH);

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_USB, IST_LEVEL,
	    ehci_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

	error = ehci_init(&sc->sc);
	if (error != USBD_NORMAL_COMPLETION) {
		aprint_error_dev(self, "init failed, error = %d\n", error);
		return;
	}

	sc->sc.sc_child = config_found(self, &sc->sc.sc_bus, usbctlprint);
}

static int
tegra_ehci_parse_properties(struct tegra_ehci_softc *sc)
{
#define PROPGET(k, v)	\
	if (prop_dictionary_get_uint8(prop, (k), (v)) == false) {	\
		aprint_error_dev(sc->sc.sc_dev,				\
		    "missing property '%s'\n", (k));			\
		return EIO;						\
	}

	prop_dictionary_t prop = device_properties(sc->sc.sc_dev);

	PROPGET("nvidia,hssync-start-delay", &sc->sc_hssync_start_delay);
	PROPGET("nvidia,idle-wait-delay", &sc->sc_idle_wait_delay);
	PROPGET("nvidia,elastic-limit", &sc->sc_elastic_limit);
	PROPGET("nvidia,term-range-adj", &sc->sc_term_range_adj);
	PROPGET("nvidia,xcvr-setup", &sc->sc_xcvr_setup);
	PROPGET("nvidia,xcvr-lsfslew", &sc->sc_xcvr_lsfslew);
	PROPGET("nvidia,xcvr-lsrslew", &sc->sc_xcvr_lsrslew);
	PROPGET("nvidia,hssquelch-level", &sc->sc_hssquelch_level);
	PROPGET("nvidia,hsdiscon-level", &sc->sc_hsdiscon_level);
	PROPGET("nvidia,xcvr-hsslew", &sc->sc_xcvr_hsslew);

	return 0;
#undef PROPGET
}

static void
tegra_ehci_init(struct ehci_softc *esc)
{
	struct tegra_ehci_softc * const sc = device_private(esc->sc_dev);
	uint32_t usbmode;

	usbmode = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
	    TEGRA_EHCI_USBMODE_REG);

	const u_int cm = __SHIFTOUT(usbmode, TEGRA_EHCI_USBMODE_CM);
	if (cm != TEGRA_EHCI_USBMODE_CM_HOST) {
		aprint_verbose_dev(esc->sc_dev, "switching to host mode\n");
		usbmode &= ~TEGRA_EHCI_USBMODE_CM;
		usbmode |= __SHIFTIN(TEGRA_EHCI_USBMODE_CM_HOST,
				     TEGRA_EHCI_USBMODE_CM);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    TEGRA_EHCI_USBMODE_REG, usbmode);
	}

	/* Parallel transceiver select */
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh,
	    TEGRA_EHCI_HOSTPC1_DEVLC_REG,
	    __SHIFTIN(TEGRA_EHCI_HOSTPC1_DEVLC_PTS_UTMI,
		      TEGRA_EHCI_HOSTPC1_DEVLC_PTS),
	    TEGRA_EHCI_HOSTPC1_DEVLC_PTS |
	    TEGRA_EHCI_HOSTPC1_DEVLC_STS);

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, TEGRA_EHCI_TXFILLTUNING_REG,
	    __SHIFTIN(0x10, TEGRA_EHCI_TXFILLTUNING_TXFIFOTHRES));
}

static void
tegra_ehci_utmip_init(struct tegra_ehci_softc *sc)
{
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	int retry;

	/* Put UTMIP PHY into reset before programming UTMIP config registers */
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_SUSP_CTRL_REG,
	    TEGRA_EHCI_SUSP_CTRL_UTMIP_RESET, 0);

	/* Enable UTMIP PHY mode */
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_SUSP_CTRL_REG,
	    TEGRA_EHCI_SUSP_CTRL_UTMIP_PHY_ENB, 0);

	/* Stop crystal clock */
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_MISC_CFG1_REG,
	    0, TEGRA_EHCI_UTMIP_MISC_CFG1_PHY_XTAL_CLOCKEN);
	delay(1);

	/* Clear session status */
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_PHY_VBUS_SENSORS_REG,
	    0,
	    TEGRA_EHCI_PHY_VBUS_SENSORS_B_VLD_SW_VALUE |
	    TEGRA_EHCI_PHY_VBUS_SENSORS_B_VLD_SW_EN);

	/* PLL configuration */
	tegra_car_utmip_init();

	/* Transceiver configuration */
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_XCVR_CFG0_REG,
	    __SHIFTIN(4, TEGRA_EHCI_UTMIP_XCVR_CFG0_SETUP) |
	    __SHIFTIN(3, TEGRA_EHCI_UTMIP_XCVR_CFG0_SETUP_MSB) |
	    __SHIFTIN(sc->sc_xcvr_hsslew,
		      TEGRA_EHCI_UTMIP_XCVR_CFG0_HSSLEW_MSB),
	    TEGRA_EHCI_UTMIP_XCVR_CFG0_SETUP |
	    TEGRA_EHCI_UTMIP_XCVR_CFG0_SETUP_MSB |
	    TEGRA_EHCI_UTMIP_XCVR_CFG0_HSSLEW_MSB);
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_XCVR_CFG1_REG,
	    __SHIFTIN(sc->sc_term_range_adj,
		      TEGRA_EHCI_UTMIP_XCVR_CFG1_TERM_RANGE_ADJ),
	    TEGRA_EHCI_UTMIP_XCVR_CFG1_TERM_RANGE_ADJ);

	if (sc->sc_port == 0) {
		tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_BIAS_CFG0_REG,
		    TEGRA_EHCI_UTMIP_BIAS_CFG0_HSDISCON_LEVEL_MSB |
		    __SHIFTIN(sc->sc_hsdiscon_level,
			      TEGRA_EHCI_UTMIP_BIAS_CFG0_HSDISCON_LEVEL),
		    TEGRA_EHCI_UTMIP_BIAS_CFG0_HSDISCON_LEVEL); 
	}

	/* Misc config */
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_MISC_CFG0_REG,
	    0,
	    TEGRA_EHCI_UTMIP_MISC_CFG0_SUSPEND_EXIT_ON_EDGE);

	/* BIAS cell power down lag */
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_BIAS_CFG1_REG,
	    __SHIFTIN(5, TEGRA_EHCI_UTMIP_BIAS_CFG1_PDTRK_COUNT),
	    TEGRA_EHCI_UTMIP_BIAS_CFG1_PDTRK_COUNT);

	/* Debounce config */
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_DEBOUNCE_CFG0_REG,
	    __SHIFTIN(0x7530, TEGRA_EHCI_UTMIP_DEBOUNCE_CFG0_A),
	    TEGRA_EHCI_UTMIP_DEBOUNCE_CFG0_A);

	/* Transmit signal preamble config */
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_TX_CFG0_REG,
	    TEGRA_EHCI_UTMIP_TX_CFG0_FS_PREAMBLE_J, 0);

	/* Power-down battery charger circuit */
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_BAT_CHRG_CFG0_REG,
	    TEGRA_EHCI_UTMIP_BAT_CHRG_CFG0_PD_CHRG, 0);

	/* Select low speed bias method */
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_XCVR_CFG0_REG,
	    0, TEGRA_EHCI_UTMIP_XCVR_CFG0_LSBIAS_SEL);

	/* High speed receive config */
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_HSRX_CFG0_REG,
	    __SHIFTIN(sc->sc_idle_wait_delay,
		      TEGRA_EHCI_UTMIP_HSRX_CFG0_IDLE_WAIT) |
	    __SHIFTIN(sc->sc_elastic_limit,
		      TEGRA_EHCI_UTMIP_HSRX_CFG0_ELASTIC_LIMIT),
	    TEGRA_EHCI_UTMIP_HSRX_CFG0_IDLE_WAIT |
	    TEGRA_EHCI_UTMIP_HSRX_CFG0_ELASTIC_LIMIT);
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_HSRX_CFG1_REG,
	    __SHIFTIN(sc->sc_hssync_start_delay,
		      TEGRA_EHCI_UTMIP_HSRX_CFG1_SYNC_START_DLY),
	    TEGRA_EHCI_UTMIP_HSRX_CFG1_SYNC_START_DLY);

	/* Start crystal clock */
	delay(1);
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_MISC_CFG1_REG,
	    TEGRA_EHCI_UTMIP_MISC_CFG1_PHY_XTAL_CLOCKEN, 0);

	/* Clear port PLL powerdown status */
	tegra_car_utmip_enable(sc->sc_port);

	/* Bring UTMIP PHY out of reset */
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_SUSP_CTRL_REG,
	    0, TEGRA_EHCI_SUSP_CTRL_UTMIP_RESET);
	for (retry = 100000; retry > 0; retry--) {
		const uint32_t susp = bus_space_read_4(bst, bsh,
		    TEGRA_EHCI_SUSP_CTRL_REG);
		if (susp & TEGRA_EHCI_SUSP_CTRL_PHY_CLK_VALID)
			break;
		delay(1);
	}
	if (retry == 0) {
		aprint_error_dev(sc->sc.sc_dev, "PHY clock is not valid\n");
		return;
	}

	/* Disable ICUSB transceiver */
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_ICUSB_CTRL_REG,
	    0,
	    TEGRA_EHCI_ICUSB_CTRL_ENB1);

	/* Power up UTMPI transceiver */
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_XCVR_CFG0_REG,
	    0,
	    TEGRA_EHCI_UTMIP_XCVR_CFG0_PD_POWERDOWN |
	    TEGRA_EHCI_UTMIP_XCVR_CFG0_PD2_POWERDOWN |
	    TEGRA_EHCI_UTMIP_XCVR_CFG0_PDZI_POWERDOWN);
	tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_XCVR_CFG1_REG,
	    0,
	    TEGRA_EHCI_UTMIP_XCVR_CFG1_PDDISC_POWERDOWN |
	    TEGRA_EHCI_UTMIP_XCVR_CFG1_PDCHRP_POWERDOWN |
	    TEGRA_EHCI_UTMIP_XCVR_CFG1_PDDR_POWERDOWN);

	if (sc->sc_port == 0) {
		tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_BIAS_CFG0_REG,
		    0, TEGRA_EHCI_UTMIP_BIAS_CFG0_BIASPD);
		delay(25);
		tegra_reg_set_clear(bst, bsh, TEGRA_EHCI_UTMIP_BIAS_CFG1_REG,
		    0, TEGRA_EHCI_UTMIP_BIAS_CFG1_PDTRK_POWERDOWN);
	}
}

static int
tegra_ehci_port_status(struct ehci_softc *ehci_sc, uint32_t v, int i)
{
	struct tegra_ehci_softc * const sc = device_private(ehci_sc->sc_dev);
	bus_space_tag_t iot = sc->sc_bst;
	bus_space_handle_t ioh = sc->sc_bsh;

	i &= ~(UPS_HIGH_SPEED|UPS_LOW_SPEED);

	uint32_t val = bus_space_read_4(iot, ioh,
	    TEGRA_EHCI_HOSTPC1_DEVLC_REG);

	switch (__SHIFTOUT(val, TEGRA_EHCI_HOSTPC1_DEVLC_PSPD)) {
	case TEGRA_EHCI_HOSTPC1_DEVLC_PSPD_FS:
		i |= UPS_FULL_SPEED;
		break;
	case TEGRA_EHCI_HOSTPC1_DEVLC_PSPD_LS:
		i |= UPS_LOW_SPEED;
		break;
	case TEGRA_EHCI_HOSTPC1_DEVLC_PSPD_HS:
	default:
		i |= UPS_HIGH_SPEED;
		break;
	}
	return i;
}
