/*	$NetBSD: octeon_xhci.c,v 1.9 2022/09/29 07:00:46 skrll Exp $ */
/*	$OpenBSD: octxhci.c,v 1.4 2019/09/29 04:32:23 visa Exp $	*/

/*
 * Copyright (c) 2017 Visa Hankala
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Driver for OCTEON USB3 controller bridge.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <mips/cavium/octeonvar.h>

#include <mips/cavium/dev/octeon_xhcireg.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/xhcireg.h>
#include <dev/usb/xhcivar.h>

#include <dev/fdt/fdtvar.h>

#define XCTL_RD_8(sc, reg) \
	bus_space_read_8((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define XCTL_WR_8(sc, reg, val) \
	bus_space_write_8((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

struct octxhci_softc {
	struct xhci_softc	sc_xhci;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct fdtbus_gpio_pin	*sc_power_gpio;
	int			sc_unit;
};

static int	octxhci_match(device_t, cfdata_t, void *);
static void	octxhci_attach(device_t, device_t, void *);

static int	octxhci_dwc3_init(struct xhci_softc *);
static void	octxhci_uctl_init(struct octxhci_softc *, uint64_t, uint64_t);

static void	octxhci_bus_io_init(bus_space_tag_t, void *);

static struct mips_bus_space octxhci_bus_tag;

CFATTACH_DECL_NEW(octxhci, sizeof(struct octxhci_softc),
    octxhci_match, octxhci_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "cavium,octeon-7130-usb-uctl" },
	DEVICE_COMPAT_EOL
};

int
octxhci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

void
octxhci_attach(device_t parent, device_t self, void *aux)
{
	struct octxhci_softc *osc = device_private(self);
	struct xhci_softc *sc = &osc->sc_xhci;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const char *clock_type_hs;
	const char *clock_type_ss;
	u_int clock_freq, clock_sel;
	char intrstr[128];
	int child, error;
	bus_addr_t addr;
	bus_size_t size;
	void *ih;

	osc->sc_iot = faa->faa_bst;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get bridge registers\n");
		return;
	}
	if (bus_space_map(osc->sc_iot, addr, size, 0, &osc->sc_ioh) != 0) {
		aprint_error(": couldn't map bridge registers\n");
		return;
	}
	osc->sc_power_gpio = fdtbus_gpio_acquire(phandle, "power",
	    GPIO_PIN_OUTPUT);
	osc->sc_unit = (addr >> 24) & 0x1;

	octxhci_bus_io_init(&octxhci_bus_tag, NULL);

	sc->sc_dev = self;
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_dmatag = faa->faa_dmat;
	sc->sc_iot = &octxhci_bus_tag;
	sc->sc_ios = size;

	child = of_find_bycompat(phandle, "synopsys,dwc3");
	if (child == -1) {
		aprint_error(": couldn't find dwc3 child node\n");
		return;
	}
	if (fdtbus_get_reg(child, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get xhci registers\n");
		return;
	}
	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map xhci registers\n");
		return;
	}

	if (of_getprop_uint32(phandle, "refclk-frequency", &clock_freq) != 0) {
		aprint_error(": couldn't get refclk-frequency property\n");
		return;
	}
	clock_type_hs = fdtbus_get_string(phandle, "refclk-type-hs");
	if (clock_type_hs == NULL) {
		aprint_error(": couldn't get refclk-type-hs property\n");
		return;
	}
	clock_type_ss = fdtbus_get_string(phandle, "refclk-type-ss");
	if (clock_type_ss == NULL) {
		aprint_error(": couldn't get refclk-type-ss property\n");
		return;
	}

	clock_sel = 0;
	if (strcmp(clock_type_ss, "dlmc_ref_clk1") == 0)
		clock_sel |= 1;
	if (strcmp(clock_type_hs, "pll_ref_clk") == 0)
		clock_sel |= 2;

	octxhci_uctl_init(osc, clock_freq, clock_sel);

	if (octxhci_dwc3_init(sc) != 0) {
		/* Error message has been printed already. */
		return;
	}

	if (!fdtbus_intr_str(child, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	ih = fdtbus_intr_establish(child, 0, IPL_USB, FDT_INTR_MPSAFE,
	    xhci_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sc->sc_bus.ub_revision = USBREV_3_0;
	error = xhci_init(sc);
	if (error != 0) {
		aprint_error_dev(self, "init failed, error = %d\n", error);
		return;
	}

	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint, CFARGS_NONE);
	sc->sc_child2 = config_found(self, &sc->sc_bus2, usbctlprint,
	    CFARGS_NONE);
}

void
octxhci_uctl_init(struct octxhci_softc *sc, uint64_t clock_freq,
    uint64_t clock_sel)
{
	static const uint32_t clock_divs[] = { 1, 2, 4, 6, 8, 16, 24, 32 };
	uint64_t i, val;
	uint64_t ioclock = octeon_ioclock_speed();
	uint64_t mpll_mult;
	uint64_t refclk_fsel;
#if notyet
	int output_sel;
#endif

	/*
	 * Put the bridge controller, USB core, PHY, and clock divider
	 * into reset.
	 */
	val = XCTL_RD_8(sc, XCTL_CTL);
	val |= XCTL_CTL_UCTL_RST;
	val |= XCTL_CTL_UAHC_RST;
	val |= XCTL_CTL_UPHY_RST;
	XCTL_WR_8(sc, XCTL_CTL, val);
	val = XCTL_RD_8(sc, XCTL_CTL);
	val |= XCTL_CTL_CLKDIV_RST;
	XCTL_WR_8(sc, XCTL_CTL, val);

	/* Select IO clock divisor. */
	for (i = 0; i < __arraycount(clock_divs); i++) {
		if (ioclock / clock_divs[i] < 300000000)
			break;
	}

	/* Update the divisor and enable the clock. */
	val = XCTL_RD_8(sc, XCTL_CTL);
	val &= ~XCTL_CTL_CLKDIV_SEL;
	val |= (i << XCTL_CTL_CLKDIV_SEL_SHIFT) & XCTL_CTL_CLKDIV_SEL;
	val |= XCTL_CTL_CLK_EN;
	XCTL_WR_8(sc, XCTL_CTL, val);

	/* Take the clock divider out of reset. */
	val = XCTL_RD_8(sc, XCTL_CTL);
	val &= ~XCTL_CTL_CLKDIV_RST;
	XCTL_WR_8(sc, XCTL_CTL, val);

	/* Select the reference clock. */
	switch (clock_freq) {
	case 50000000:
		refclk_fsel = 0x07;
		mpll_mult = 0x32;
		break;
	case 125000000:
		refclk_fsel = 0x07;
		mpll_mult = 0x28;
		break;
	case 100000000:
	default:
		if (clock_sel < 2)
			refclk_fsel = 0x27;
		else
			refclk_fsel = 0x07;
		mpll_mult = 0x19;
		break;
	}

	/* Set the clock and power up PHYs. */
	val = XCTL_RD_8(sc, XCTL_CTL);
	val &= ~XCTL_CTL_REFCLK_SEL;
	val |= clock_sel << XCTL_CTL_REFCLK_SEL_SHIFT;
	val &= ~XCTL_CTL_REFCLK_DIV2;
	val &= ~XCTL_CTL_REFCLK_FSEL;
	val |= refclk_fsel << XCTL_CTL_REFCLK_FSEL_SHIFT;
	val &= ~XCTL_CTL_MPLL_MULT;
	val |= mpll_mult << XCTL_CTL_MPLL_MULT_SHIFT;
	val |= XCTL_CTL_SSC_EN;
	val |= XCTL_CTL_REFCLK_SSP_EN;
	val |= XCTL_CTL_SSPOWER_EN;
	val |= XCTL_CTL_HSPOWER_EN;
	XCTL_WR_8(sc, XCTL_CTL, val);

	delay(100);

	/* Take the bridge out of reset. */
	val = XCTL_RD_8(sc, XCTL_CTL);
	val &= ~XCTL_CTL_UCTL_RST;
	XCTL_WR_8(sc, XCTL_CTL, val);

	delay(100);

#if notyet
	if (sc->sc_power_gpio[0] != 0) {
		if (sc->sc_unit == 0)
			output_sel = GPIO_CONFIG_MD_USB0_VBUS_CTRL;
		else
			output_sel = GPIO_CONFIG_MD_USB1_VBUS_CTRL;
		gpio_controller_config_pin(sc->sc_power_gpio,
		    GPIO_CONFIG_OUTPUT | output_sel);

		/* Enable port power control. */
		val = XCTL_RD_8(sc, XCTL_HOST_CFG);
		val |= XCTL_HOST_CFG_PPC_EN;
		if (sc->sc_power_gpio[2] & GPIO_ACTIVE_LOW)
			val &= ~XCTL_HOST_CFG_PPC_ACTIVE_HIGH_EN;
		else
			val |= XCTL_HOST_CFG_PPC_ACTIVE_HIGH_EN;
		XCTL_WR_8(sc, XCTL_HOST_CFG, val);
	} else {
		/* Disable port power control. */
		val = XCTL_RD_8(sc, XCTL_HOST_CFG);
		val &= ~XCTL_HOST_CFG_PPC_EN;
		XCTL_WR_8(sc, XCTL_HOST_CFG, val);
	}
#else
	/* Disable port power control. */
	val = XCTL_RD_8(sc, XCTL_HOST_CFG);
	val &= ~XCTL_HOST_CFG_PPC_EN;
	XCTL_WR_8(sc, XCTL_HOST_CFG, val);
#endif

	/* Enable host-only mode. */
	val = XCTL_RD_8(sc, XCTL_CTL);
	val &= ~XCTL_CTL_DRD_MODE;
	XCTL_WR_8(sc, XCTL_CTL, val);

	delay(100);

	/* Take the USB core out of reset. */
	val = XCTL_RD_8(sc, XCTL_CTL);
	val &= ~XCTL_CTL_UAHC_RST;
	XCTL_WR_8(sc, XCTL_CTL, val);

	delay(100);

	val = XCTL_RD_8(sc, XCTL_CTL);
	val |= XCTL_CTL_CSCLK_EN;
	XCTL_WR_8(sc, XCTL_CTL, val);

	/* Take the PHY out of reset. */
	val = XCTL_RD_8(sc, XCTL_CTL);
	val &= ~XCTL_CTL_UPHY_RST;
	XCTL_WR_8(sc, XCTL_CTL, val);
	(void)XCTL_RD_8(sc, XCTL_CTL);

	/* Fix endianness. */
	val = XCTL_RD_8(sc, XCTL_SHIM_CFG);
	val &= ~XCTL_SHIM_CFG_CSR_BYTE_SWAP;
	val &= ~XCTL_SHIM_CFG_DMA_BYTE_SWAP;
	val |= __SHIFTIN(XCTL_SHIM_ENDIAN_BIG, XCTL_SHIM_CFG_DMA_BYTE_SWAP);
	val |= __SHIFTIN(XCTL_SHIM_ENDIAN_BIG, XCTL_SHIM_CFG_CSR_BYTE_SWAP);
	XCTL_WR_8(sc, XCTL_SHIM_CFG, val);
	(void)XCTL_RD_8(sc, XCTL_SHIM_CFG);
}

int
octxhci_dwc3_init(struct xhci_softc *sc)
{
	bus_space_handle_t ioh = sc->sc_ioh;
	uint32_t rev;
	uint32_t val;

	val = bus_space_read_4(sc->sc_iot, ioh, DWC3_GSNPSID);
	if ((val & 0xffff0000u) != 0x55330000u) {
		aprint_error(": no DWC3 core (DWC3_GSNPSID=%08x)\n", val);
		return EIO;
	}
	rev = val & 0xffffu;
	aprint_normal(": DWC3 rev 0x%04x\n", rev);

	val = bus_space_read_4(sc->sc_iot, ioh, DWC3_GUSB3PIPECTL(0));
	val &= ~DWC3_GUSB3PIPECTL_UX_EXIT_PX;
	val |= DWC3_GUSB3PIPECTL_SUSPHY;
	bus_space_write_4(sc->sc_iot, ioh, DWC3_GUSB3PIPECTL(0), val);

	val = bus_space_read_4(sc->sc_iot, ioh, DWC3_GUSB2PHYCFG(0));
	val |= DWC3_GUSB2PHYCFG_SUSPHY;
	bus_space_write_4(sc->sc_iot, ioh, DWC3_GUSB2PHYCFG(0), val);

	/* Set the controller into host mode. */
	val = bus_space_read_4(sc->sc_iot, ioh, DWC3_GCTL);
	val &= ~DWC3_GCTL_PRTCAP_MASK;
	val |= DWC3_GCTL_PRTCAP_HOST;
	bus_space_write_4(sc->sc_iot, ioh, DWC3_GCTL, val);

	val = bus_space_read_4(sc->sc_iot, ioh, DWC3_GCTL);
	val &= ~DWC3_GCTL_SCALEDOWN_MASK;
	val &= ~DWC3_GCTL_DISSCRAMBLE;
	if (rev >= DWC3_REV_210A && rev <= DWC3_REV_250A)
		val |= DWC3_GCTL_DSBLCLKGTNG | DWC3_GCTL_SOFITPSYNC;
	else
		val &= ~DWC3_GCTL_DSBLCLKGTNG;
	bus_space_write_4(sc->sc_iot, ioh, DWC3_GCTL, val);

	return 0;
}

/* ---- bus_space(9) */
#define CHIP			octxhci
#define CHIP_IO
#define	CHIP_LITTLE_ENDIAN

#define CHIP_W1_BUS_START(v)	0x0000000000000000ULL
#define CHIP_W1_BUS_END(v)	0x7fffffffffffffffULL
#define CHIP_W1_SYS_START(v)	0x8000000000000000ULL
#define CHIP_W1_SYS_END(v)	0xffffffffffffffffULL

#include <mips/mips/bus_space_alignstride_chipdep.c>
