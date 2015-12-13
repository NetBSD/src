/* $NetBSD: tegra_usbphy.c,v 1.3 2015/12/13 17:39:19 jmcneill Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_usbphy.c,v 1.3 2015/12/13 17:39:19 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>
#include <arm/nvidia/tegra_usbreg.h>

#include <dev/fdt/fdtvar.h>

/* XXX */
static int
tegra_usbphy_addr2port(bus_addr_t addr)
{
	switch (addr) {
	case TEGRA_AHB_A2_BASE + TEGRA_USB1_OFFSET:
		return 0;
	case TEGRA_AHB_A2_BASE + TEGRA_USB2_OFFSET:
		return 1;
	case TEGRA_AHB_A2_BASE + TEGRA_USB3_OFFSET:
		return 2;
	default:
		return -1;
	}
}

static int	tegra_usbphy_match(device_t, cfdata_t, void *);
static void	tegra_usbphy_attach(device_t, device_t, void *);

struct tegra_usbphy_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;
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

static int	tegra_usbphy_parse_properties(struct tegra_usbphy_softc *);
static void	tegra_usbphy_utmip_init(struct tegra_usbphy_softc *);

CFATTACH_DECL_NEW(tegra_usbphy, sizeof(struct tegra_usbphy_softc),
	tegra_usbphy_match, tegra_usbphy_attach, NULL, NULL);

static int
tegra_usbphy_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "nvidia,tegra124-usb-phy", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_usbphy_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_usbphy_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct fdtbus_regulator *reg;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}
	sc->sc_port = tegra_usbphy_addr2port(addr);

	aprint_naive("\n");
	aprint_normal(": USB PHY%d\n", sc->sc_port + 1);

	if (tegra_usbphy_parse_properties(sc) != 0)
		return;

	tegra_car_periph_usb_enable(sc->sc_port);
	delay(2);

	tegra_usbphy_utmip_init(sc);

	reg = fdtbus_regulator_acquire(faa->faa_phandle, "vbus-supply");
	if (reg) {
		const uint32_t v = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    TEGRA_EHCI_PHY_VBUS_SENSORS_REG);
		if ((v & TEGRA_EHCI_PHY_VBUS_SENSORS_A_VBUS_VLD_STS) == 0) {
			fdtbus_regulator_enable(reg);
		} else {
			aprint_normal_dev(self, "VBUS input active\n");
		}
        }
}

static int
tegra_usbphy_parse_properties(struct tegra_usbphy_softc *sc)
{
	const int phandle = sc->sc_phandle;
	const int plen = sizeof(u_int);
	u_int val;

#define PROPGET(k, v)							\
do {									\
	if (OF_getprop(phandle, (k), &val, plen) != plen) {		\
		aprint_error_dev(sc->sc_dev,				\
		    "missing property '%s'\n", (k));			\
		return EIO;						\
	}								\
	*(v) = be32toh(val);						\
} while (0)

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
tegra_usbphy_utmip_init(struct tegra_usbphy_softc *sc)
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
		aprint_error_dev(sc->sc_dev, "PHY clock is not valid\n");
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
