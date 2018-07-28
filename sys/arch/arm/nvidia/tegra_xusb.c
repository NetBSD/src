/* $NetBSD: tegra_xusb.c,v 1.12.4.1 2018/07/28 04:37:28 pgoyette Exp $ */

/*
 * Copyright (c) 2016 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "locators.h"
#include "opt_tegra.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_xusb.c,v 1.12.4.1 2018/07/28 04:37:28 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>
#include <arm/nvidia/tegra_xusbpad.h>
#include <arm/nvidia/tegra_xusbreg.h>
#include <arm/nvidia/tegra_pmcreg.h>

#include <dev/pci/pcireg.h>

#include <dev/fdt/fdtvar.h>

#include <dev/firmload.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/xhcireg.h>
#include <dev/usb/xhcivar.h>

#ifdef TEGRA_XUSB_DEBUG
int tegra_xusb_debug = 1;
#else
int tegra_xusb_debug = 0;
#endif

#define DPRINTF(...)	if (tegra_xusb_debug) device_printf(__VA_ARGS__)

static int	tegra_xusb_match(device_t, cfdata_t, void *);
static void	tegra_xusb_attach(device_t, device_t, void *);
static void	tegra_xusb_mountroot(device_t);

static int	tegra_xusb_intr_mbox(void *);

#ifdef TEGRA124_XUSB_BIN_STATIC
extern const char _binary_tegra124_xusb_bin_size[];
extern const char _binary_tegra124_xusb_bin_start[];
#endif

#ifdef TEGRA210_XUSB_BIN_STATIC
extern const char _binary_tegra210_xusb_bin_size[];
extern const char _binary_tegra210_xusb_bin_start[];
#endif

enum xusb_type {
	XUSB_T124 = 1,
	XUSB_T210
};

static const struct of_compat_data compat_data[] = {
	{ "nvidia,tegra124-xusb",		XUSB_T124 },
	{ "nvidia,tegra210-xusb",		XUSB_T210 },
	{ NULL }
};

struct fw_dma {
	bus_dmamap_t            map;
	void *                  addr;
	bus_dma_segment_t       segs[1];
	int                     nsegs;
	size_t                  size;
};

struct tegra_xusb_softc {
	struct xhci_softc	sc_xhci;
	int			sc_phandle;
	bus_space_handle_t	sc_bsh_xhci;
	bus_space_handle_t	sc_bsh_fpci;
	bus_space_handle_t	sc_bsh_ipfs;
	void			*sc_ih;
	void			*sc_ih_mbox;
	struct fw_dma		sc_fw_dma;
	struct clk		*sc_clk_ss_src;
	enum xusb_type		sc_type;

	bool			sc_scale_ss_clock;
};

static uint32_t	csb_read_4(struct tegra_xusb_softc * const, bus_size_t);
static void	csb_write_4(struct tegra_xusb_softc * const, bus_size_t,
    uint32_t);
	
static void	tegra_xusb_init(struct tegra_xusb_softc * const);
static int	tegra_xusb_open_fw(struct tegra_xusb_softc * const);
static int	tegra_xusb_load_fw(struct tegra_xusb_softc * const, void *,
    size_t);
static void	tegra_xusb_init_regulators(struct tegra_xusb_softc * const);

static int	xusb_mailbox_send(struct tegra_xusb_softc * const, uint32_t);

CFATTACH_DECL_NEW(tegra_xusb, sizeof(struct tegra_xusb_softc),
	tegra_xusb_match, tegra_xusb_attach, NULL, NULL);

static int
tegra_xusb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

#define tegra_xusb_attach_check(sc, cond, fmt, ...)			\
    do {								\
	if (cond) {							\
		aprint_error_dev(sc->sc_dev, fmt, ## __VA_ARGS__);	\
		return;							\
	}								\
    } while (0)

static void
tegra_xusb_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_xusb_softc * const psc = device_private(self);
	struct xhci_softc * const sc = &psc->sc_xhci;
	struct fdt_attach_args * const faa = aux;
	bool wait_for_root = true;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	struct fdtbus_reset *rst;
	struct fdtbus_phy *phy;
	struct clk *clk;
	uint32_t rate;
	int error, n;

	aprint_naive("\n");
	aprint_normal(": XUSB\n");

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	sc->sc_bus.ub_hcpriv = sc;
	sc->sc_bus.ub_dmatag = faa->faa_dmat;
	sc->sc_quirks = XHCI_DEFERRED_START;
	psc->sc_phandle = faa->faa_phandle;
	psc->sc_type = of_search_compatible(faa->faa_phandle, compat_data)->data;

	switch (psc->sc_type) {
	case XUSB_T124:
		psc->sc_scale_ss_clock = true;
		break;
	default:
		psc->sc_scale_ss_clock = false;
		break;
	}

	if (fdtbus_get_reg_byname(faa->faa_phandle, "hcd", &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	error = bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh);
	if (error) {
		aprint_error(": couldn't map %#" PRIx64 ": %d",
		    (uint64_t)addr, error);
		return;
	}
	DPRINTF(sc->sc_dev, "mapped %#" PRIx64 "\n", (uint64_t)addr);

	if (fdtbus_get_reg_byname(faa->faa_phandle, "fpci", &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	error = bus_space_map(sc->sc_iot, addr, size, 0, &psc->sc_bsh_fpci);
	if (error) {
		aprint_error(": couldn't map %#" PRIx64 ": %d",
		    (uint64_t)addr, error);
		return;
	}
	DPRINTF(sc->sc_dev, "mapped %#" PRIx64 "\n", (uint64_t)addr);

	if (fdtbus_get_reg_byname(faa->faa_phandle, "ipfs", &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	error = bus_space_map(sc->sc_iot, addr, size, 0, &psc->sc_bsh_ipfs);
	if (error) {
		aprint_error(": couldn't map %#" PRIx64 ": %d",
		    (uint64_t)addr, error);
		return;
	}
	DPRINTF(sc->sc_dev, "mapped %#" PRIx64 "\n", (uint64_t)addr);

	if (!fdtbus_intr_str(faa->faa_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	psc->sc_ih = fdtbus_intr_establish(faa->faa_phandle, 0, IPL_USB,
	    FDT_INTR_MPSAFE, xhci_intr, sc);
	if (psc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	if (!fdtbus_intr_str(faa->faa_phandle, 1, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	psc->sc_ih_mbox = fdtbus_intr_establish(faa->faa_phandle, 1, IPL_VM,
	    FDT_INTR_MPSAFE, tegra_xusb_intr_mbox, psc);
	if (psc->sc_ih_mbox == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	/* Enable PHYs */
	for (n = 0; (phy = fdtbus_phy_get_index(faa->faa_phandle, n)) != NULL; n++)
		if (fdtbus_phy_enable(phy, true) != 0)
			aprint_error_dev(self, "failed to enable PHY #%d\n", n);

	/* Enable XUSB power rails */

	tegra_pmc_power(PMC_PARTID_XUSBC, true);	/* Host/USB2.0 */
	tegra_pmc_remove_clamping(PMC_PARTID_XUSBC);
	tegra_pmc_power(PMC_PARTID_XUSBA, true);	/* SuperSpeed */
	tegra_pmc_remove_clamping(PMC_PARTID_XUSBA);

	/* Enable XUSB clocks */

	clk = fdtbus_clock_get(faa->faa_phandle, "pll_e");
	rate = clk_get_rate(clk);
	error = clk_enable(clk); /* XXX set frequency */
	DPRINTF(sc->sc_dev, "rate %u error %d\n", rate, error);
	tegra_xusb_attach_check(sc, error, "failed to enable pll_e clock");

	clk = fdtbus_clock_get(faa->faa_phandle, "xusb_host_src");
	rate = clk_get_rate(clk);
	DPRINTF(sc->sc_dev, "rate %u error %d\n", rate, error);
	error = clk_set_rate(clk, 102000000);
	tegra_xusb_attach_check(sc, error, "failed to set xusb_host_src clock rate");

	rate = clk_get_rate(clk);
	error = clk_enable(clk); /* XXX set frequency */
	DPRINTF(sc->sc_dev, "rate %u error %d\n", rate, error);
	tegra_xusb_attach_check(sc, error, "failed to enable xusb_host_src clock");

	clk = fdtbus_clock_get(faa->faa_phandle, "xusb_falcon_src");
	rate = clk_get_rate(clk);
	DPRINTF(sc->sc_dev, "rate %u error %d\n", rate, error);
	error = clk_set_rate(clk, 204000000);
	tegra_xusb_attach_check(sc, error, "failed to set xusb_falcon_src clock rate");

	rate = clk_get_rate(clk);
	error = clk_enable(clk);
	DPRINTF(sc->sc_dev, "rate %u error %d\n", rate, error);
	tegra_xusb_attach_check(sc, error, "failed to enable xusb_falcon_src clock");

	clk = fdtbus_clock_get(faa->faa_phandle, "xusb_host");
	rate = clk_get_rate(clk);
	error = clk_enable(clk); /* XXX set frequency */
	DPRINTF(sc->sc_dev, "rate %u error %d\n", rate, error);

	clk = fdtbus_clock_get(faa->faa_phandle, "xusb_ss");
	rate = clk_get_rate(clk);
	error = clk_enable(clk); /* XXX set frequency */
	DPRINTF(sc->sc_dev, "xusb_ss rate %u error %d\n", rate, error);
	tegra_xusb_attach_check(sc, error, "failed to enable xusb_ss clock");

	psc->sc_clk_ss_src = fdtbus_clock_get(faa->faa_phandle, "xusb_ss_src");
	tegra_xusb_attach_check(sc, psc->sc_clk_ss_src == NULL,
		"failed to get xusb_ss_src clock");

	if (psc->sc_scale_ss_clock) {
		rate = clk_get_rate(psc->sc_clk_ss_src);
		DPRINTF(sc->sc_dev, "xusb_ss_src rate %u\n", rate);
		error = clk_set_rate(psc->sc_clk_ss_src, 2000000);
		rate = clk_get_rate(psc->sc_clk_ss_src);
		DPRINTF(sc->sc_dev, "xusb_ss_src rate %u error %d\n", rate, error);
		tegra_xusb_attach_check(sc, error, "failed to get xusb_ss_src clock rate");

		rate = clk_get_rate(psc->sc_clk_ss_src);
		DPRINTF(sc->sc_dev, "ss_src rate %u\n", rate);
		tegra_xusb_attach_check(sc, error, "failed to set xusb_ss_src clock rate");

		error = clk_set_rate(psc->sc_clk_ss_src, 120000000);
		rate = clk_get_rate(psc->sc_clk_ss_src);
		DPRINTF(sc->sc_dev, "ss_src rate %u error %d\n", rate, error);
		tegra_xusb_attach_check(sc, error, "failed to get xusb_ss_src clock rate");
	}

	rate = clk_get_rate(psc->sc_clk_ss_src);
	error = clk_enable(psc->sc_clk_ss_src);
	DPRINTF(sc->sc_dev, "ss_src rate %u error %d\n", rate, error);
	tegra_xusb_attach_check(sc, error, "failed to enable xusb_ss_src clock");

#if 0
	clk = fdtbus_clock_get(faa->faa_phandle, "xusb_hs_src");
	error = 0;
	rate = clk_get_rate(clk);
	DPRINTF(sc->sc_dev, "rate %u error %d\n", rate, error);
#endif

	clk = fdtbus_clock_get(faa->faa_phandle, "xusb_fs_src");
	rate = clk_get_rate(clk);
	error = clk_enable(clk); /* XXX set frequency */
	DPRINTF(sc->sc_dev, "rate %u error %d\n", rate, error);
	tegra_xusb_attach_check(sc, error, "failed to enable xusb_fs_src clock");

	rst = fdtbus_reset_get(faa->faa_phandle, "xusb_host");
	fdtbus_reset_deassert(rst);

	rst = fdtbus_reset_get(faa->faa_phandle, "xusb_src");
	fdtbus_reset_deassert(rst);

	rst = fdtbus_reset_get(faa->faa_phandle, "xusb_ss");
	fdtbus_reset_deassert(rst);

	DELAY(1);

	tegra_xusb_init_regulators(psc);

	tegra_xusb_init(psc);

#if defined(TEGRA124_XUSB_BIN_STATIC)
	if (psc->sc_type == XUSB_T124)
		wait_for_root = false;
#endif
#if defined(TEGRA210_XUSB_BIN_STATIC)
	if (psc->sc_type == XUSB_T210)
		wait_for_root = false;
#endif

	if (wait_for_root)
		config_mountroot(sc->sc_dev, tegra_xusb_mountroot);
	else
		tegra_xusb_mountroot(sc->sc_dev);
}

static void
tegra_xusb_mountroot(device_t self)
{
	struct tegra_xusb_softc * const psc = device_private(self);
	struct xhci_softc * const sc = &psc->sc_xhci;
	const bus_space_tag_t bst = sc->sc_iot;
	const bus_space_handle_t ipfsh = psc->sc_bsh_ipfs;
	struct clk *clk;
	struct fdtbus_reset *rst;
	uint32_t rate;
	uint32_t val;
	int error;

	DPRINTF(sc->sc_dev, "%s()\n", __func__);

	val = bus_space_read_4(bst, ipfsh, 0x0);
	DPRINTF(sc->sc_dev, "%s ipfs 0x0 = 0x%x\n", __func__, val);

	if (tegra_xusb_open_fw(psc) != 0)
		return;
	DPRINTF(sc->sc_dev, "post fw\n");

	tegra_xusbpad_xhci_enable();

	clk = fdtbus_clock_get(psc->sc_phandle, "xusb_falcon_src");
	rate = clk_get_rate(clk);
	error = clk_enable(clk);
	DPRINTF(sc->sc_dev, "rate %u error %d\n", rate, error);

	clk = fdtbus_clock_get(psc->sc_phandle, "xusb_host_src");
	rate = clk_get_rate(clk);
	error = clk_enable(clk);
	DPRINTF(sc->sc_dev, "rate %u error %d\n", rate, error);

	val = bus_space_read_4(bst, ipfsh, 0x0);
	DPRINTF(sc->sc_dev, "%s ipfs 0x0 = 0x%x\n", __func__, val);

	rst = fdtbus_reset_get(psc->sc_phandle, "xusb_host");
	fdtbus_reset_deassert(rst);

	rst = fdtbus_reset_get(psc->sc_phandle, "xusb_src");
	fdtbus_reset_deassert(rst);

	rst = fdtbus_reset_get(psc->sc_phandle, "xusb_ss");
	fdtbus_reset_deassert(rst);

	val = csb_read_4(psc, XUSB_CSB_FALCON_CPUCTL_REG);
	DPRINTF(sc->sc_dev, "XUSB_FALC_CPUCTL 0x%x\n", val);

	val = bus_space_read_4(bst, psc->sc_bsh_fpci, PCI_USBREV)
	    & PCI_USBREV_MASK;
	switch (val) {
	case PCI_USBREV_3_0:
		sc->sc_bus.ub_revision = USBREV_3_0;
		break;
	case PCI_USBREV_3_1:
		sc->sc_bus.ub_revision = USBREV_3_1;
		break;
	default:
		if (val < PCI_USBREV_3_0) {
			aprint_error_dev(self, "Unknown revision (%02x)\n", val);
			sc->sc_bus.ub_revision = USBREV_UNKNOWN;
		} else {
			/* Default to the latest revision */
			aprint_normal_dev(self,
			    "Unknown revision (%02x). Set to 3.1.\n", val);
			sc->sc_bus.ub_revision = USBREV_3_1;
		}
		break;
	}

	error = xhci_init(sc);
	if (error) {
		aprint_error_dev(self, "init failed, error=%d\n", error);
		return;
	}

	sc->sc_child = config_found(self, &sc->sc_bus, usbctlprint);

	sc->sc_child2 = config_found(self, &sc->sc_bus2, usbctlprint);

	xhci_start(sc);

	error = xusb_mailbox_send(psc, 0x01000000);
	if (error) {
		aprint_error_dev(self, "send failed, error=%d\n", error);
	}
}

static int
tegra_xusb_intr_mbox(void *v)
{
	struct tegra_xusb_softc * const psc = v;
	struct xhci_softc * const sc = &psc->sc_xhci;
	const bus_space_tag_t bst = sc->sc_iot;
	const bus_space_handle_t fpcih = psc->sc_bsh_fpci;
	uint32_t val;
	uint32_t irv;
	uint32_t msg;
	int error;

	DPRINTF(sc->sc_dev, "%s()\n", __func__);

	irv = bus_space_read_4(bst, fpcih, T_XUSB_CFG_ARU_SMI_INTR_REG);
	DPRINTF(sc->sc_dev, "XUSB_CFG_ARU_SMI_INTR 0x%x\n", irv);
	bus_space_write_4(bst, fpcih, T_XUSB_CFG_ARU_SMI_INTR_REG, irv);

	if (irv & T_XUSB_CFG_ARU_SMI_INTR_FW_HANG)
		aprint_error_dev(sc->sc_dev, "firmware hang\n");

	msg = bus_space_read_4(bst, fpcih, T_XUSB_CFG_ARU_MAILBOX_DATA_OUT_REG);
	DPRINTF(sc->sc_dev, "XUSB_CFG_ARU_MBOX_DATA_OUT 0x%x\n", msg);

	val = bus_space_read_4(bst, fpcih, T_XUSB_CFG_ARU_MAILBOX_CMD_REG);
	DPRINTF(sc->sc_dev, "XUSB_CFG_ARU_MBOX_CMD 0x%x\n", val);
	val &= ~T_XUSB_CFG_ARU_MAILBOX_CMD_DEST_SMI;
	bus_space_write_4(bst, fpcih, T_XUSB_CFG_ARU_MAILBOX_CMD_REG, val);

	bool sendresp = true;
	u_int rate;

	const uint32_t data = __SHIFTOUT(msg, MAILBOX_DATA_DATA);
	const uint8_t type = __SHIFTOUT(msg, MAILBOX_DATA_TYPE);

	switch (type) {
	case 2:
	case 3:
		DPRINTF(sc->sc_dev, "FALC_CLOCK %u\n", data * 1000);
		break;
	case 4:
	case 5:
		if (psc->sc_scale_ss_clock) {
			DPRINTF(sc->sc_dev, "SSPI_CLOCK %u\n", data * 1000);
			rate = clk_get_rate(psc->sc_clk_ss_src);
			DPRINTF(sc->sc_dev, "rate of psc->sc_clk_ss_src %u\n",
			    rate);
			error = clk_set_rate(psc->sc_clk_ss_src, data * 1000);
			if (error != 0)
				goto clk_fail;
			rate = clk_get_rate(psc->sc_clk_ss_src);
			DPRINTF(sc->sc_dev,
			    "rate of psc->sc_clk_ss_src %u after\n", rate);
			if (data == (rate / 1000)) {
				msg = __SHIFTIN(128, MAILBOX_DATA_TYPE) |
				      __SHIFTIN(rate / 1000, MAILBOX_DATA_DATA);
			} else
clk_fail:	
				msg = __SHIFTIN(129, MAILBOX_DATA_TYPE) |
				      __SHIFTIN(rate / 1000, MAILBOX_DATA_DATA);
		} else {
			msg = __SHIFTIN(128, MAILBOX_DATA_TYPE) |
			      __SHIFTIN(data, MAILBOX_DATA_DATA);
		}
		xusb_mailbox_send(psc, msg);
		break;
	case 9:
		msg = __SHIFTIN(data, MAILBOX_DATA_DATA) |
		      __SHIFTIN(128, MAILBOX_DATA_TYPE);
		xusb_mailbox_send(psc, msg);
		break;
	case 6:
	case 128:
	case 129:
		sendresp = false;
		break;
	default:
		sendresp = false;
		break;
	}

	if (sendresp == false)
		bus_space_write_4(bst, fpcih, T_XUSB_CFG_ARU_MAILBOX_OWNER_REG,
		    MAILBOX_OWNER_NONE);

	return irv;
}

static void
tegra_xusb_init_regulators(struct tegra_xusb_softc * const psc)
{
	const char * supply_names[] = {
		"dvddio-pex-supply",
		"hvddio-pex-supply",
		"avdd-usb-supply",
		"avdd-pll-utmip-supply",
		"avdd-pll-uerefe-supply",
		"dvdd-usb-ss-pll-supply",
		"hvdd-usb-ss-pll-e-supply"
	};
	device_t dev = psc->sc_xhci.sc_dev;
	const int phandle = psc->sc_phandle;
	struct fdtbus_regulator *reg;
	int n, error;

	for (n = 0; n < __arraycount(supply_names); n++) {
		if (!of_hasprop(phandle, supply_names[n]))
			continue;
		reg = fdtbus_regulator_acquire(phandle, supply_names[n]);
		if (reg == NULL) {
			aprint_error_dev(dev, "couldn't acquire supply '%s'\n",
			    supply_names[n]);
			continue;
		}
		error = fdtbus_regulator_enable(reg);
		if (error != 0)
			aprint_error_dev(dev, "couldn't enable supply '%s': %d\n",
			    supply_names[n], error);
	}
}

static void
tegra_xusb_init(struct tegra_xusb_softc * const psc)
{
	struct xhci_softc * const sc = &psc->sc_xhci;
	const bus_space_tag_t bst = sc->sc_iot;
	const bus_space_handle_t ipfsh = psc->sc_bsh_ipfs;
	const bus_space_handle_t fpcih = psc->sc_bsh_fpci;

	DPRINTF(sc->sc_dev, "%s()\n", __func__);

	DPRINTF(sc->sc_dev, "%s ipfs 0x0 = 0x%x\n", __func__,
	    bus_space_read_4(bst, ipfsh, 0x0));

	DPRINTF(sc->sc_dev, "%s ipfs 0x40 = 0x%x\n", __func__,
	    bus_space_read_4(bst, ipfsh, 0x40));

	DPRINTF(sc->sc_dev, "%s ipfs 0x80 = 0x%x\n", __func__,
	    bus_space_read_4(bst, ipfsh, 0x80));
	/* FPCI_BAR0_START and FPCI_BAR0_ACCESS_TYPE */
	bus_space_write_4(bst, ipfsh, 0x80, 0x00100000);
	DPRINTF(sc->sc_dev, "%s ipfs 0x80 = 0x%x\n", __func__,
	    bus_space_read_4(bst, ipfsh, 0x80));

	DPRINTF(sc->sc_dev, "%s ipfs 0x180 = 0x%x\n", __func__,
	    bus_space_read_4(bst, ipfsh, 0x180));
	/* EN_FPCI */
	tegra_reg_set_clear(bst, ipfsh, 0x180, 1, 0);
	DPRINTF(sc->sc_dev, "%s ipfs 0x180 = 0x%x\n", __func__,
	    bus_space_read_4(bst, ipfsh, 0x180));

	DPRINTF(sc->sc_dev, "%s fpci PCI_COMMAND_STATUS_REG = 0x%x\n",
	    __func__, bus_space_read_4(bst, fpcih, PCI_COMMAND_STATUS_REG));
	tegra_reg_set_clear(bst, fpcih, PCI_COMMAND_STATUS_REG,
	    PCI_COMMAND_MASTER_ENABLE|PCI_COMMAND_MEM_ENABLE, 0x0);
	DPRINTF(sc->sc_dev, "%s fpci PCI_COMMAND_STATUS_REG = 0x%x\n",
	    __func__, bus_space_read_4(bst, fpcih, PCI_COMMAND_STATUS_REG));

	DPRINTF(sc->sc_dev, "%s fpci PCI_BAR0 = 0x%x\n", __func__,
	    bus_space_read_4(bst, fpcih, PCI_BAR0));
	/* match FPCI BAR0 to above */
	bus_space_write_4(bst, fpcih, PCI_BAR0, 0x10000000);
	DPRINTF(sc->sc_dev, "%s fpci PCI_BAR0 = 0x%x\n", __func__,
	    bus_space_read_4(bst, fpcih, PCI_BAR0));
	
	DPRINTF(sc->sc_dev, "%s ipfs 0x188 = 0x%x\n", __func__,
	    bus_space_read_4(bst, ipfsh, 0x188));
	tegra_reg_set_clear(bst, ipfsh, 0x188, __BIT(16), 0);
	DPRINTF(sc->sc_dev, "%s ipfs 0x188 = 0x%x\n", __func__,
	    bus_space_read_4(bst, ipfsh, 0x188));

	DPRINTF(sc->sc_dev, "%s fpci 0x1bc = 0x%x\n", __func__,
	    bus_space_read_4(bst, fpcih, 0x1bc));
	bus_space_write_4(bst, fpcih, 0x1bc, 0x80);
	DPRINTF(sc->sc_dev, "%s fpci 0x1bc = 0x%x\n", __func__,
	    bus_space_read_4(bst, fpcih, 0x1bc));
}

static int
fw_dma_alloc(struct tegra_xusb_softc * const psc, size_t size, size_t align,
    struct fw_dma * const p)
{
	struct xhci_softc * const sc = &psc->sc_xhci;
	const bus_dma_tag_t dmat = sc->sc_bus.ub_dmatag;
	int err;

	p->size = size;
	err = bus_dmamem_alloc(dmat, p->size, align, 0, p->segs,
	    sizeof(p->segs) / sizeof(p->segs[0]), &p->nsegs, BUS_DMA_NOWAIT);
	if (err)
		return err;
	err = bus_dmamem_map(dmat, p->segs, p->nsegs, p->size, &p->addr,
	    BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (err)
		goto free;
	err = bus_dmamap_create(dmat, p->size, 1, p->size, 0, BUS_DMA_NOWAIT,
	    &p->map);
	if (err)
		goto unmap;
	err = bus_dmamap_load(dmat, p->map, p->addr, p->size, NULL,
	    BUS_DMA_NOWAIT);
	if (err)
		goto destroy;

	return 0;

destroy:
	bus_dmamap_destroy(dmat, p->map);
unmap:
	bus_dmamem_unmap(dmat, p->addr, p->size);
free:
	bus_dmamem_free(dmat, p->segs, p->nsegs);

	return err;
}

#if !defined(TEGRA124_XUSB_BIN_STATIC)
static void
fw_dma_free(struct tegra_xusb_softc * const psc, struct fw_dma * const p)
{
	const struct xhci_softc * const sc = &psc->sc_xhci;
	const bus_dma_tag_t dmat = sc->sc_bus.ub_dmatag;

	bus_dmamap_unload(dmat, p->map);
	bus_dmamap_destroy(dmat, p->map);
	bus_dmamem_unmap(dmat, p->addr, p->size);
	bus_dmamem_free(dmat, p->segs, p->nsegs);
}
#endif

#define FWHEADER_BOOT_CODETAG 8
#define FWHEADER_BOOT_CODESIZE 12
#define FWHEADER_FWIMG_LEN 100
#define FWHEADER__LEN 256

static int
tegra_xusb_open_fw(struct tegra_xusb_softc * const psc)
{
	struct xhci_softc * const sc = &psc->sc_xhci;
	firmware_handle_t fw;
	size_t firmware_size = 0;
	void *firmware_image;
	const char *fw_path = NULL;
	void *fw_static = NULL;
	int error;

	switch (psc->sc_type) {
	case XUSB_T124:
#if defined(TEGRA124_XUSB_BIN_STATIC)
		firmware_size = (uintptr_t)&_binary_tegra124_xusb_bin_size;
		fw_static = _binary_tegra124_xusb_bin_start;
#else
		fw_path = "nvidia/tegra124";
#endif
		break;
	case XUSB_T210:
#if defined(TEGRA210_XUSB_BIN_STATIC)
		firmware_size = (uintptr_t)&_binary_tegra210_xusb_bin_size;
		fw_static = _binary_tegra210_xusb_bin_start;
#else
		fw_path = "nvidia/tegra210";
#endif
		break;
	default:
		return EINVAL;
	}

	if (fw_path != NULL) {
		error = firmware_open(fw_path, "xusb.bin", &fw);
		if (error != 0) {
			aprint_error_dev(sc->sc_dev,
			    "couldn't load firmware from %s/xusb.bin: %d\n",
			    fw_path, error);
			return error;
		}
		firmware_size = firmware_get_size(fw);
	}

	error = fw_dma_alloc(psc, firmware_size, PAGE_SIZE,
	    &psc->sc_fw_dma);
	if (error != 0)
		return error;
	firmware_image = psc->sc_fw_dma.addr;

	if (fw_path != NULL) {
		error = firmware_read(fw, 0, firmware_image, firmware_size);
		if (error != 0) {
			fw_dma_free(psc, &psc->sc_fw_dma);
			firmware_close(fw);
			return error;
		}
		firmware_close(fw);
	} else {
		memcpy(firmware_image, fw_static, firmware_size);
	}

	return tegra_xusb_load_fw(psc, firmware_image, firmware_size);
}

static int
tegra_xusb_load_fw(struct tegra_xusb_softc * const psc, void *firmware_image,
    size_t firmware_size)
{
	struct xhci_softc * const sc = &psc->sc_xhci;
	const uint8_t *header;

	header = firmware_image;

	const uint32_t fwimg_len = le32dec(&header[FWHEADER_FWIMG_LEN]);
	const uint32_t boot_codetag = le32dec(&header[FWHEADER_BOOT_CODETAG]);
	const uint32_t boot_codesize = le32dec(&header[FWHEADER_BOOT_CODESIZE]);

	if (fwimg_len != firmware_size)
		aprint_error_dev(sc->sc_dev, "fwimg_len mismatch %u != %zu\n",
		    fwimg_len, firmware_size);

	bus_dmamap_sync(sc->sc_bus.ub_dmatag, psc->sc_fw_dma.map, 0,
	    firmware_size, BUS_DMASYNC_PREWRITE);

	DPRINTF(sc->sc_dev, "XUSB_FALC_CPUCTL 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_FALCON_CPUCTL_REG));
	DPRINTF(sc->sc_dev, "XUSB_CSB_MP_ILOAD_BASE_LO 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_MEMPOOL_ILOAD_BASE_LO_REG));

	DPRINTF(sc->sc_dev, "XUSB_CSB_MP_ILOAD_ATTR 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_MEMPOOL_ILOAD_ATTR_REG));
	csb_write_4(psc, XUSB_CSB_MEMPOOL_ILOAD_ATTR_REG,
	    fwimg_len);
	DPRINTF(sc->sc_dev, "XUSB_CSB_MP_ILOAD_ATTR 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_MEMPOOL_ILOAD_ATTR_REG));

	const uint64_t fwbase = psc->sc_fw_dma.map->dm_segs[0].ds_addr +
	    FWHEADER__LEN;

	csb_write_4(psc, XUSB_CSB_MEMPOOL_ILOAD_BASE_HI_REG, fwbase >> 32);
	csb_write_4(psc, XUSB_CSB_MEMPOOL_ILOAD_BASE_LO_REG, fwbase);
	DPRINTF(sc->sc_dev, "XUSB_CSB_MP_ILOAD_BASE_LO 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_MEMPOOL_ILOAD_BASE_LO_REG));
	DPRINTF(sc->sc_dev, "XUSB_CSB_MP_ILOAD_BASE_HI 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_MEMPOOL_ILOAD_BASE_HI_REG));

	DPRINTF(sc->sc_dev, "XUSB_CSB_MP_APMAP 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_MEMPOOL_APMAP_REG));
	csb_write_4(psc, XUSB_CSB_MEMPOOL_APMAP_REG,
	    XUSB_CSB_MEMPOOL_APMAP_BOOTPATH);
	DPRINTF(sc->sc_dev, "XUSB_CSB_MP_APMAP 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_MEMPOOL_APMAP_REG));

	DPRINTF(sc->sc_dev, "XUSB_CSB_MP_L2IMEMOP_TRIG 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_MEMPOOL_L2IMEMOP_TRIG_REG));
	csb_write_4(psc, XUSB_CSB_MEMPOOL_L2IMEMOP_TRIG_REG,
	    __SHIFTIN(ACTION_L2IMEM_INVALIDATE_ALL,
		XUSB_CSB_MEMPOOL_L2IMEMOP_TRIG_ACTION));
	DPRINTF(sc->sc_dev, "XUSB_CSB_MP_L2IMEMOP_TRIG 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_MEMPOOL_L2IMEMOP_TRIG_REG));

	const u_int code_tag_blocks =
	    howmany(boot_codetag, IMEM_BLOCK_SIZE);
	const u_int code_size_blocks =
	    howmany(boot_codesize, IMEM_BLOCK_SIZE);
	const u_int code_blocks = code_tag_blocks + code_size_blocks;

	DPRINTF(sc->sc_dev, "XUSB_CSB_MP_L2IMEMOP_SIZE 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_MEMPOOL_L2IMEMOP_SIZE_REG));
	csb_write_4(psc, XUSB_CSB_MEMPOOL_L2IMEMOP_SIZE_REG,
	    __SHIFTIN(code_tag_blocks,
		XUSB_CSB_MEMPOOL_L2IMEMOP_SIZE_SRC_OFFSET) |
	    __SHIFTIN(code_size_blocks,
		XUSB_CSB_MEMPOOL_L2IMEMOP_SIZE_SRC_COUNT));
	DPRINTF(sc->sc_dev, "XUSB_CSB_MP_L2IMEMOP_SIZE 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_MEMPOOL_L2IMEMOP_SIZE_REG));

	DPRINTF(sc->sc_dev, "XUSB_CSB_MP_L2IMEMOP_TRIG 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_MEMPOOL_L2IMEMOP_TRIG_REG));
	csb_write_4(psc, XUSB_CSB_MEMPOOL_L2IMEMOP_TRIG_REG,
	    __SHIFTIN(ACTION_L2IMEM_LOAD_LOCKED_RESULT,
		XUSB_CSB_MEMPOOL_L2IMEMOP_TRIG_ACTION));
	DPRINTF(sc->sc_dev, "XUSB_CSB_MP_L2IMEMOP_TRIG 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_MEMPOOL_L2IMEMOP_TRIG_REG));

	DPRINTF(sc->sc_dev, "XUSB_FALC_IMFILLCTL 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_FALCON_IMFILLCTL_REG));
	csb_write_4(psc, XUSB_CSB_FALCON_IMFILLCTL_REG, code_size_blocks);
	DPRINTF(sc->sc_dev, "XUSB_FALC_IMFILLCTL 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_FALCON_IMFILLCTL_REG));

	DPRINTF(sc->sc_dev, "XUSB_FALC_IMFILLRNG1 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_FALCON_IMFILLRNG1_REG));
	csb_write_4(psc, XUSB_CSB_FALCON_IMFILLRNG1_REG,
	    __SHIFTIN(code_tag_blocks, XUSB_CSB_FALCON_IMFILLRNG1_TAG_LO) |
	    __SHIFTIN(code_blocks, XUSB_CSB_FALCON_IMFILLRNG1_TAG_HI));
	DPRINTF(sc->sc_dev, "XUSB_FALC_IMFILLRNG1 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_FALCON_IMFILLRNG1_REG));

	DPRINTF(sc->sc_dev, "XUSB_FALC_DMACTL 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_FALCON_DMACTL_REG));
	csb_write_4(psc, XUSB_CSB_FALCON_DMACTL_REG, 0);
	DPRINTF(sc->sc_dev, "XUSB_FALC_DMACTL 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_FALCON_DMACTL_REG));

	DPRINTF(sc->sc_dev, "XUSB_FALC_BOOTVEC 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_FALCON_BOOTVEC_REG));
	csb_write_4(psc, XUSB_CSB_FALCON_BOOTVEC_REG,
	    boot_codetag);
	DPRINTF(sc->sc_dev, "XUSB_FALC_BOOTVEC 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_FALCON_BOOTVEC_REG));

	DPRINTF(sc->sc_dev, "XUSB_FALC_CPUCTL 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_FALCON_CPUCTL_REG));
	csb_write_4(psc, XUSB_CSB_FALCON_CPUCTL_REG,
	    XUSB_CSB_FALCON_CPUCTL_STARTCPU);
	DPRINTF(sc->sc_dev, "XUSB_FALC_CPUCTL 0x%x\n",
	    csb_read_4(psc, XUSB_CSB_FALCON_CPUCTL_REG));

	return 0;
}

static uint32_t
csb_read_4(struct tegra_xusb_softc * const psc, bus_size_t csb_offset)
{
	const uint32_t range = __SHIFTOUT(csb_offset, XUSB_CSB_RANGE);
	const bus_size_t offset = __SHIFTOUT(csb_offset, XUSB_CSB_OFFSET);
	const bus_space_tag_t bst = psc->sc_xhci.sc_iot;
	const bus_space_handle_t fpcih = psc->sc_bsh_fpci;

	bus_space_write_4(bst, fpcih, T_XUSB_CFG_ARU_C11_CSBRANGE_REG, range);
	return bus_space_read_4(bst, fpcih, T_XUSB_CFG_CSB_BASE_ADDR + offset);
}

static void
csb_write_4(struct tegra_xusb_softc * const psc, bus_size_t csb_offset,
    uint32_t value)
{
	const uint32_t range = __SHIFTOUT(csb_offset, XUSB_CSB_RANGE);
	const bus_size_t offset = __SHIFTOUT(csb_offset, XUSB_CSB_OFFSET);
	const bus_space_tag_t bst = psc->sc_xhci.sc_iot;
	const bus_space_handle_t fpcih = psc->sc_bsh_fpci;

	bus_space_write_4(bst, fpcih, T_XUSB_CFG_ARU_C11_CSBRANGE_REG, range);
	bus_space_write_4(bst, fpcih, T_XUSB_CFG_CSB_BASE_ADDR + offset, value);
}

static int
xusb_mailbox_send(struct tegra_xusb_softc * const psc, uint32_t msg)
{
	struct xhci_softc * const sc = &psc->sc_xhci;
	const bus_space_tag_t bst = psc->sc_xhci.sc_iot;
	const bus_space_handle_t fpcih = psc->sc_bsh_fpci;
	uint32_t val;
	bool wait = false;

	const uint8_t type = __SHIFTOUT(msg, MAILBOX_DATA_TYPE);

	if (!(type == 128 || type == 129)) {
		val = bus_space_read_4(bst, fpcih,
		    T_XUSB_CFG_ARU_MAILBOX_OWNER_REG);
		DPRINTF(sc->sc_dev, "XUSB_CFG_ARU_MBOX_OWNER 0x%x\n",
		    val);
		if (val != MAILBOX_OWNER_NONE) {
			return EBUSY;
		}

		bus_space_write_4(bst, fpcih, T_XUSB_CFG_ARU_MAILBOX_OWNER_REG,
		    MAILBOX_OWNER_SW);

		val = bus_space_read_4(bst, fpcih,
		    T_XUSB_CFG_ARU_MAILBOX_OWNER_REG);
		DPRINTF(sc->sc_dev, "XUSB_CFG_ARU_MBOX_OWNER 0x%x\n",
		    val);
		if (val != MAILBOX_OWNER_SW) {
			return EBUSY;
		}

		wait = true;
	}

	bus_space_write_4(bst, fpcih, T_XUSB_CFG_ARU_MAILBOX_DATA_IN_REG, msg);

	tegra_reg_set_clear(bst, fpcih, T_XUSB_CFG_ARU_MAILBOX_CMD_REG,
	    T_XUSB_CFG_ARU_MAILBOX_CMD_INT_EN |
	    T_XUSB_CFG_ARU_MAILBOX_CMD_DEST_FALCON, 0);

	if (wait) {

		for (u_int i = 0; i < 2500; i++) {
			val = bus_space_read_4(bst, fpcih,
			    T_XUSB_CFG_ARU_MAILBOX_OWNER_REG);
			DPRINTF(sc->sc_dev,
			    "XUSB_CFG_ARU_MBOX_OWNER 0x%x\n", val);
			if (val == MAILBOX_OWNER_NONE) {
				break;
			}
			DELAY(10);
		}

		val = bus_space_read_4(bst, fpcih,
		    T_XUSB_CFG_ARU_MAILBOX_OWNER_REG);
		DPRINTF(sc->sc_dev,
		    "XUSB_CFG_ARU_MBOX_OWNER 0x%x\n", val);
		if (val != MAILBOX_OWNER_NONE) {
			aprint_error_dev(sc->sc_dev,
			    "timeout, XUSB_CFG_ARU_MBOX_OWNER 0x%x\n", val);
		}
	}

	return 0;
}
