/* $NetBSD: tegra124_xusbpad.c,v 1.4 2019/10/13 06:11:31 skrll Exp $ */

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

#include "opt_tegra.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra124_xusbpad.c,v 1.4 2019/10/13 06:11:31 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_xusbpad.h>
#include <arm/nvidia/tegra124_xusbpadreg.h>
#include <arm/nvidia/tegra_var.h>

#include <dev/fdt/fdtvar.h>

#define TEGRA_FUSE_SKU_CALIB_REG 0xf0

static int	tegra124_xusbpad_match(device_t, cfdata_t, void *);
static void	tegra124_xusbpad_attach(device_t, device_t, void *);
static void	tegra124_xusbpad_sata_enable(device_t);
static void	tegra124_xusbpad_xhci_enable(device_t);

static const struct tegra_xusbpad_ops tegra124_xusbpad_ops = {
	.sata_enable = tegra124_xusbpad_sata_enable,
	.xhci_enable = tegra124_xusbpad_xhci_enable,
};

struct tegra124_xusbpad_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};

#ifdef TEGRA_XUSBPAD_DEBUG
static void	padregdump(void);
#endif

CFATTACH_DECL_NEW(tegra124_xusbpad, sizeof(struct tegra124_xusbpad_softc),
	tegra124_xusbpad_match, tegra124_xusbpad_attach, NULL, NULL);

static int
tegra124_xusbpad_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] =
	    { "nvidia,tegra124-xusb-padctl", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra124_xusbpad_attach(device_t parent, device_t self, void *aux)
{
	struct tegra124_xusbpad_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#" PRIxBUSADDR ": %d", addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": XUSB PADCTL\n");

	tegra_xusbpad_register(self, &tegra124_xusbpad_ops);
}

static void
tegra124_xusbpad_sata_enable(device_t dev)
{
	struct tegra124_xusbpad_softc * const sc = device_private(dev);
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	int retry;

	tegra_reg_set_clear(bst, bsh, XUSB_PADCTL_USB3_PAD_MUX_REG,
	    __SHIFTIN(XUSB_PADCTL_USB3_PAD_MUX_SATA_PAD_LANE0_SATA,
		      XUSB_PADCTL_USB3_PAD_MUX_SATA_PAD_LANE0) |
	    XUSB_PADCTL_USB3_PAD_MUX_FORCE_SATA_PAD_IDDQ_DISABLE_MASK0,
	    XUSB_PADCTL_USB3_PAD_MUX_SATA_PAD_LANE0);

	tegra_reg_set_clear(bst, bsh, XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1_REG,
	    0,
	    XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1_IDDQ |
	    XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1_IDDQ_OVRD);
	tegra_reg_set_clear(bst, bsh, XUSB_PADCTL_IOPHY_PLL_S0_CTL1_REG,
	    0,
	    XUSB_PADCTL_IOPHY_PLL_S0_CTL1_IDDQ |
	    XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PWR_OVRD);
	tegra_reg_set_clear(bst, bsh, XUSB_PADCTL_IOPHY_PLL_S0_CTL1_REG,
	    XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL1_MODE, 0);
	tegra_reg_set_clear(bst, bsh, XUSB_PADCTL_IOPHY_PLL_S0_CTL1_REG,
	    XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL_RST, 0);

	for (retry = 1000; retry > 0; retry--) {
		const uint32_t v = bus_space_read_4(bst, bsh,
		    XUSB_PADCTL_IOPHY_PLL_S0_CTL1_REG);
		if (v & XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL1_LOCKDET)
			break;
		delay(100);
	}
	if (retry == 0) {
		printf("WARNING: SATA PHY power-on failed\n");
	}
}

#ifdef TEGRA_XUSBPAD_DEBUG
static void
padregdump(void)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t i;
	u_int j;

	tegra124_xusbpad_get_bs(&bst, &bsh);

	for (i = 0x000; i < 0x160; ) {
		printf("0x%03jx:", (uintmax_t)i);
		for (j = 0; i < 0x160 && j < 0x10; j += 4, i += 4) {
			printf(" %08x", bus_space_read_4(bst, bsh, i));
		}
		printf("\n");
	}
}
#endif

static void
tegra124_xusbpad_xhci_enable(device_t dev)
{
	struct tegra124_xusbpad_softc * const sc = device_private(dev);
	const uint32_t skucalib = tegra_fuse_read(TEGRA_FUSE_SKU_CALIB_REG);
#ifdef TEGRA_XUSBPAD_DEBUG
	uint32_t val;
#endif

	if (sc == NULL) {
		aprint_error("%s: xusbpad driver not loaded\n", __func__);
		return;
	}


#ifdef TEGRA_XUSBPAD_DEBUG
	padregdump(void);
	printf("SKU CALIB 0x%x\n", skucalib);
#endif
	const uint32_t hcl[3] = {
		(skucalib >>  0) & 0x3f,
		(skucalib >> 15) & 0x3f,
		(skucalib >> 15) & 0x3f,
	};
	const uint32_t hic = (skucalib >> 13) & 3;
	const uint32_t hsl = (skucalib >> 11) & 3;
	const uint32_t htra = (skucalib >> 7) & 0xf;


#ifdef TEGRA_XUSBPAD_DEBUG
	val = bus_space_read_4(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_PAD_MUX_REG);
	device_printf(sc->sc_dev, "XUSB_PADCTL_USB2_PAD_MUX_REG is 0x%x\n", val);
	val = bus_space_read_4(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_PORT_CAP_REG);
	device_printf(sc->sc_dev, "XUSB_PADCTL_USB2_PORT_CAP_REG is 0x%x\n", val);
	val = bus_space_read_4(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_SS_PORT_MAP_REG);
	device_printf(sc->sc_dev, "XUSB_PADCTL_SS_PORT_MAP_REG is 0x%x\n", val);
#endif

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_PAD_MUX_REG, (0<<0)|(0<<2)|(1<<4));
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_PORT_CAP_REG, (1<<0)|(1<<4)|(1<<8));
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_SS_PORT_MAP_REG, (2<<0)|(7<<4));

	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_BIAS_PAD_CTL0_REG,
	    __SHIFTIN(hsl, XUSB_PADCTL_USB2_BIAS_PAD_CTL0_HS_SQUELCH_LEVEL) |
	    __SHIFTIN(XUSB_PADCTL_USB2_BIAS_PAD_CTL0_HS_DISCON_LEVEL_VAL,
		      XUSB_PADCTL_USB2_BIAS_PAD_CTL0_HS_DISCON_LEVEL),
	    XUSB_PADCTL_USB2_BIAS_PAD_CTL0_HS_SQUELCH_LEVEL |
	    XUSB_PADCTL_USB2_BIAS_PAD_CTL0_HS_DISCON_LEVEL);

	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh,
	    XUSB_PADCTL_USB2_OTG_PAD0_CTL0_REG,
	    __SHIFTIN(hcl[0],
		      XUSB_PADCTL_USB2_OTG_PAD_CTL0_HS_CURR_LEVEL) |
	    __SHIFTIN(XUSB_PADCTL_USB2_OTG_PAD_CTL0_HS_SLEW_VAL,
		      XUSB_PADCTL_USB2_OTG_PAD_CTL0_HS_SLEW) |
	    __SHIFTIN(XUSB_PADCTL_USB2_OTG_PAD_CTL0_LS_RSLEW_VAL(0),
		      XUSB_PADCTL_USB2_OTG_PAD_CTL0_LS_RSLEW),
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_HS_CURR_LEVEL |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_HS_SLEW |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_LS_RSLEW |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD2 |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD_ZI);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh,
	    XUSB_PADCTL_USB2_OTG_PAD1_CTL0_REG,
	    __SHIFTIN(hcl[1],
		      XUSB_PADCTL_USB2_OTG_PAD_CTL0_HS_CURR_LEVEL) |
	    __SHIFTIN(XUSB_PADCTL_USB2_OTG_PAD_CTL0_HS_SLEW_VAL,
		      XUSB_PADCTL_USB2_OTG_PAD_CTL0_HS_SLEW) |
	    __SHIFTIN(XUSB_PADCTL_USB2_OTG_PAD_CTL0_LS_RSLEW_VAL(1),
		      XUSB_PADCTL_USB2_OTG_PAD_CTL0_LS_RSLEW),
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_HS_CURR_LEVEL |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_HS_SLEW |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_LS_RSLEW |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD2 |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD_ZI);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh,
	    XUSB_PADCTL_USB2_OTG_PAD2_CTL0_REG,
	    __SHIFTIN(hcl[2],
		      XUSB_PADCTL_USB2_OTG_PAD_CTL0_HS_CURR_LEVEL) |
	    __SHIFTIN(XUSB_PADCTL_USB2_OTG_PAD_CTL0_HS_SLEW_VAL,
		      XUSB_PADCTL_USB2_OTG_PAD_CTL0_HS_SLEW) |
	    __SHIFTIN(XUSB_PADCTL_USB2_OTG_PAD_CTL0_LS_RSLEW_VAL(2),
		      XUSB_PADCTL_USB2_OTG_PAD_CTL0_LS_RSLEW),
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_HS_CURR_LEVEL |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_HS_SLEW |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_LS_RSLEW |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD2 |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD_ZI);

	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh,
	    XUSB_PADCTL_USB2_OTG_PAD0_CTL1_REG,
	    __SHIFTIN(htra,
		      XUSB_PADCTL_USB2_OTG_PAD_CTL1_TERM_RANGE_ADJ) |
	    __SHIFTIN(hic,
		      XUSB_PADCTL_USB2_OTG_PAD_CTL1_HS_IREF_CAP),
	    XUSB_PADCTL_USB2_OTG_PAD_CTL1_TERM_RANGE_ADJ |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL1_HS_IREF_CAP |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL1_PD_DR |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL1_PD_CHRP_FORCE_POWERUP |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL1_PD_DISC_FORCE_POWERUP);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh,
	    XUSB_PADCTL_USB2_OTG_PAD1_CTL1_REG,
	    __SHIFTIN(htra,
		      XUSB_PADCTL_USB2_OTG_PAD_CTL1_TERM_RANGE_ADJ) |
	    __SHIFTIN(hic,
		      XUSB_PADCTL_USB2_OTG_PAD_CTL1_HS_IREF_CAP),
	    XUSB_PADCTL_USB2_OTG_PAD_CTL1_TERM_RANGE_ADJ |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL1_HS_IREF_CAP |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL1_PD_DR |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL1_PD_CHRP_FORCE_POWERUP |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL1_PD_DISC_FORCE_POWERUP);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh,
	    XUSB_PADCTL_USB2_OTG_PAD2_CTL1_REG,
	    __SHIFTIN(htra,
		      XUSB_PADCTL_USB2_OTG_PAD_CTL1_TERM_RANGE_ADJ) |
	    __SHIFTIN(hic,
		      XUSB_PADCTL_USB2_OTG_PAD_CTL1_HS_IREF_CAP),
	    XUSB_PADCTL_USB2_OTG_PAD_CTL1_TERM_RANGE_ADJ |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL1_HS_IREF_CAP |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL1_PD_DR |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL1_PD_CHRP_FORCE_POWERUP |
	    XUSB_PADCTL_USB2_OTG_PAD_CTL1_PD_DISC_FORCE_POWERUP);

	//tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_BATTERY_CHRG_BIASPAD_REG, 0, 1); /* PD_OTG */
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_OTG_PAD0_CTL0_REG, 0, XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_OTG_PAD1_CTL0_REG, 0, XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_OTG_PAD2_CTL0_REG, 0, XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_OTG_PAD0_CTL0_REG, 0, XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD2);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_OTG_PAD1_CTL0_REG, 0, XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD2);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_OTG_PAD2_CTL0_REG, 0, XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD2);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_OTG_PAD0_CTL0_REG, 0, XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD_ZI);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_OTG_PAD1_CTL0_REG, 0, XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD_ZI);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_OTG_PAD2_CTL0_REG, 0, XUSB_PADCTL_USB2_OTG_PAD_CTL0_PD_ZI);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_OTG_PAD0_CTL1_REG, 0, XUSB_PADCTL_USB2_OTG_PAD_CTL1_PD_DR);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_OTG_PAD1_CTL1_REG, 0, XUSB_PADCTL_USB2_OTG_PAD_CTL1_PD_DR);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_OTG_PAD2_CTL1_REG, 0, XUSB_PADCTL_USB2_OTG_PAD_CTL1_PD_DR);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_BIAS_PAD_CTL0_REG, 0, XUSB_PADCTL_USB2_BIAS_PAD_CTL0_PD);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_BIAS_PAD_CTL0_REG, 0, XUSB_PADCTL_USB2_BIAS_PAD_CTL0_PD_TRK);

	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_ELPG_PROGRAM_REG,
	    0, XUSB_PADCTL_ELPG_PROGRAM_SSP0_ELPG_CLAMP_EN);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_ELPG_PROGRAM_REG,
	    0, XUSB_PADCTL_ELPG_PROGRAM_SSP0_ELPG_CLAMP_EN_EARLY);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_ELPG_PROGRAM_REG,
	    0, XUSB_PADCTL_ELPG_PROGRAM_SSP0_ELPG_VCORE_DOWN);

	DELAY(200);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_ELPG_PROGRAM_REG, 0,
	    XUSB_PADCTL_ELPG_PROGRAM_AUX_MUX_LP0_VCORE_DOWN);
	DELAY(200);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_ELPG_PROGRAM_REG, 0,
	    XUSB_PADCTL_ELPG_PROGRAM_AUX_MUX_LP0_CLAMP_EN_EARLY);
	DELAY(200);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_ELPG_PROGRAM_REG, 0,
	    XUSB_PADCTL_ELPG_PROGRAM_AUX_MUX_LP0_CLAMP_EN);
	DELAY(200);

	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_OC_DET_REG, 0,
	    XUSB_PADCTL_OC_DET_OC_DETECTED_VBUS_PAD2 |
	    XUSB_PADCTL_OC_DET_OC_DETECTED_VBUS_PAD1 |
	    XUSB_PADCTL_OC_DET_OC_DETECTED_VBUS_PAD0 |
	    XUSB_PADCTL_OC_DET_OC_DETECTED3 |
	    XUSB_PADCTL_OC_DET_OC_DETECTED2 |
	    XUSB_PADCTL_OC_DET_OC_DETECTED1 |
	    XUSB_PADCTL_OC_DET_OC_DETECTED0);
	tegra_reg_set_clear(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_OC_DET_REG,
	    XUSB_PADCTL_OC_DET_VBUS_ENABLE2 | 
	    XUSB_PADCTL_OC_DET_VBUS_ENABLE1 |
	    XUSB_PADCTL_OC_DET_VBUS_ENABLE0, 0);

#ifdef TEGRA_XUSBPAD_DEBUG
	val = bus_space_read_4(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_PAD_MUX_REG);
	device_printf(sc->sc_dev, "XUSB_PADCTL_USB2_PAD_MUX_REG is 0x%x\n", val);
	val = bus_space_read_4(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_USB2_PORT_CAP_REG);
	device_printf(sc->sc_dev, "XUSB_PADCTL_USB2_PORT_CAP_REG is 0x%x\n", val);
	val = bus_space_read_4(sc->sc_bst, sc->sc_bsh, XUSB_PADCTL_SS_PORT_MAP_REG);
	device_printf(sc->sc_dev, "XUSB_PADCTL_SS_PORT_MAP_REG is 0x%x\n", val);

	padregdump();
#endif
}
