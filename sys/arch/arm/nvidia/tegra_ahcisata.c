/* $NetBSD: tegra_ahcisata.c,v 1.11.2.2 2017/12/03 11:35:54 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_ahcisata.c,v 1.11.2.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/ata/atavar.h>
#include <dev/ic/ahcisatavar.h>

#include <arm/nvidia/tegra_var.h>
#include <arm/nvidia/tegra_pmcreg.h>
#include <arm/nvidia/tegra_ahcisatareg.h>
#include <arm/nvidia/tegra_xusbpad.h>

#include <dev/fdt/fdtvar.h>

#define TEGRA_AHCISATA_OFFSET	0x7000

static int	tegra_ahcisata_match(device_t, cfdata_t, void *);
static void	tegra_ahcisata_attach(device_t, device_t, void *);

struct tegra_ahcisata_softc {
	struct ahci_softc	sc;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	void			*sc_ih;
	struct clk		*sc_clk_sata;
	struct clk		*sc_clk_sata_oob;
	struct clk		*sc_clk_cml1;
	struct clk		*sc_clk_pll_e;
	struct fdtbus_reset	*sc_rst_sata;
	struct fdtbus_reset	*sc_rst_sata_oob;
	struct fdtbus_reset	*sc_rst_sata_cold;

	struct tegra_gpio_pin	*sc_pin_power;
};

static const char * const tegra_ahcisata_supplies[] = {
    "hvdd-supply",
    "vddio-supply",
    "avdd-supply",
    "target-5v-supply",
    "target-12v-supply"
};

static void	tegra_ahcisata_init(struct tegra_ahcisata_softc *);
static int	tegra_ahcisata_init_clocks(struct tegra_ahcisata_softc *);

CFATTACH_DECL_NEW(tegra_ahcisata, sizeof(struct tegra_ahcisata_softc),
	tegra_ahcisata_match, tegra_ahcisata_attach, NULL, NULL);

static int
tegra_ahcisata_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "nvidia,tegra124-ahci", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_ahcisata_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_ahcisata_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t ahci_addr, sata_addr;
	bus_size_t ahci_size, sata_size;
	struct fdtbus_regulator *reg;
	char intrstr[128];
	int error, n;

	if (fdtbus_get_reg(phandle, 0, &ahci_addr, &ahci_size) != 0) {
		aprint_error(": couldn't get ahci registers\n");
		return;
	}
	if (fdtbus_get_reg(phandle, 1, &sata_addr, &sata_size) != 0) {
		aprint_error(": couldn't get sata registers\n");
		return;
	}
	sc->sc_clk_sata = fdtbus_clock_get(phandle, "sata");
	if (sc->sc_clk_sata == NULL) {
		aprint_error(": couldn't get clock sata\n");
		return;
	}
	sc->sc_clk_sata_oob = fdtbus_clock_get(phandle, "sata-oob");
	if (sc->sc_clk_sata_oob == NULL) {
		aprint_error(": couldn't get clock sata-oob\n");
		return;
	}
	sc->sc_clk_cml1 = fdtbus_clock_get(phandle, "cml1");
	if (sc->sc_clk_cml1 == NULL) {
		aprint_error(": couldn't get clock cml1\n");
		return;
	}
	sc->sc_clk_pll_e = fdtbus_clock_get(phandle, "pll_e");
	if (sc->sc_clk_pll_e == NULL) {
		aprint_error(": couldn't get clock pll_e\n");
		return;
	}
	sc->sc_rst_sata = fdtbus_reset_get(phandle, "sata");
	if (sc->sc_rst_sata == NULL) {
		aprint_error(": couldn't get reset sata\n");
		return;
	}
	sc->sc_rst_sata_oob = fdtbus_reset_get(phandle, "sata-oob");
	if (sc->sc_rst_sata_oob == NULL) {
		aprint_error(": couldn't get reset sata-oob\n");
		return;
	}
	sc->sc_rst_sata_cold = fdtbus_reset_get(phandle, "sata-cold");
	if(sc->sc_rst_sata_cold == NULL) {
		aprint_error(": couldn't get reset sata-cold\n");
		return;
	}

	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, sata_addr, sata_size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map sata registers: %d\n", error);
		return;
	}

	sc->sc.sc_atac.atac_dev = self;
	sc->sc.sc_dmat = faa->faa_dmat;
	sc->sc.sc_ahcit = faa->faa_bst;
	sc->sc.sc_ahcis = ahci_size;
	error = bus_space_map(sc->sc.sc_ahcit, ahci_addr, ahci_size, 0,
	    &sc->sc.sc_ahcih);
	if (error) {
		aprint_error(": couldn't map ahci registers: %d\n", error);
		return;
	}
	sc->sc.sc_ahci_quirks = AHCI_QUIRK_SKIP_RESET;

	aprint_naive("\n");
	aprint_normal(": SATA\n");

	for (n = 0; n < __arraycount(tegra_ahcisata_supplies); n++) {
		const char *supply = tegra_ahcisata_supplies[n];
		reg = fdtbus_regulator_acquire(phandle, supply);
		if (reg == NULL) {
			aprint_error_dev(self, "couldn't acquire %s\n", supply);
			continue;
		}
		if (fdtbus_regulator_enable(reg) != 0) {
			aprint_error_dev(self, "couldn't enable %s\n", supply);
		}
		fdtbus_regulator_release(reg);
	}

	if (tegra_ahcisata_init_clocks(sc) != 0)
		return;

	tegra_xusbpad_sata_enable();

	tegra_ahcisata_init(sc);

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_BIO, 0,
	    ahci_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	ahci_attach(&sc->sc);
}

static void
tegra_ahcisata_init(struct tegra_ahcisata_softc *sc)
{
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;

	const u_int gen1_tx_amp = 0x18;
	const u_int gen1_tx_peak = 0x04;
	const u_int gen2_tx_amp = 0x18;
	const u_int gen2_tx_peak = 0x0a;

	/* Set RX idle detection source and disable RX idle detection interrupt */
	tegra_reg_set_clear(bst, bsh, TEGRA_SATA_AUX_MISC_CNTL_1_REG,
	    TEGRA_SATA_AUX_MISC_CNTL_1_AUX_OR_CORE_IDLE_STATUS_SEL, 0);
	tegra_reg_set_clear(bst, bsh, TEGRA_SATA_AUX_RX_STAT_INT_REG,
	    TEGRA_SATA_AUX_RX_STAT_INT_SATA_RX_STAT_INT_DISABLE, 0);

	/* Prevent automatic OOB sequence when coming out of reset */
	tegra_reg_set_clear(bst, bsh, TEGRA_SATA_AUX_MISC_CNTL_1_REG,
	    0, TEGRA_SATA_AUX_MISC_CNTL_1_OOB_ON_POR);

	/* Disable device sleep */
	tegra_reg_set_clear(bst, bsh, TEGRA_SATA_AUX_MISC_CNTL_1_REG,
	    0, TEGRA_SATA_AUX_MISC_CNTL_1_SDS_SUPPORT);

	/* Enable IFPS device block */
	tegra_reg_set_clear(bst, bsh, TEGRA_SATA_CONFIGURATION_REG,
	    TEGRA_SATA_CONFIGURATION_EN_FPCI, 0);

	/* PHY config */
	bus_space_write_4(bst, bsh, TEGRA_T_SATA0_INDEX_REG,
	    TEGRA_T_SATA0_INDEX_CH1);
	tegra_reg_set_clear(bst, bsh, TEGRA_T_SATA0_CHX_PHY_CTRL1_GEN1_REG,
	    __SHIFTIN(gen1_tx_amp, TEGRA_T_SATA0_CHX_PHY_CTRL1_GEN1_TX_AMP) |
	    __SHIFTIN(gen1_tx_peak, TEGRA_T_SATA0_CHX_PHY_CTRL1_GEN1_TX_PEAK),
	    TEGRA_T_SATA0_CHX_PHY_CTRL1_GEN1_TX_AMP |
	    TEGRA_T_SATA0_CHX_PHY_CTRL1_GEN1_TX_PEAK);
	tegra_reg_set_clear(bst, bsh, TEGRA_T_SATA0_CHX_PHY_CTRL1_GEN2_REG,
	    __SHIFTIN(gen2_tx_amp, TEGRA_T_SATA0_CHX_PHY_CTRL1_GEN2_TX_AMP) |
	    __SHIFTIN(gen2_tx_peak, TEGRA_T_SATA0_CHX_PHY_CTRL1_GEN2_TX_PEAK),
	    TEGRA_T_SATA0_CHX_PHY_CTRL1_GEN2_TX_AMP |
	    TEGRA_T_SATA0_CHX_PHY_CTRL1_GEN2_TX_PEAK);
	bus_space_write_4(bst, bsh, TEGRA_T_SATA0_CHX_PHY_CTRL11_REG,
	    __SHIFTIN(0x2800, TEGRA_T_SATA0_CHX_PHY_CTRL11_GEN2_RX_EQ));
	bus_space_write_4(bst, bsh, TEGRA_T_SATA0_CHX_PHY_CTRL2_REG,
	    __SHIFTIN(0x23, TEGRA_T_SATA0_CHX_PHY_CTRL2_CDR_CNTL_GEN1));
	bus_space_write_4(bst, bsh, TEGRA_T_SATA0_INDEX_REG, 0);

	/* Backdoor update the programming interface field and class code */
	tegra_reg_set_clear(bst, bsh, TEGRA_T_SATA0_CFG_SATA_REG,
	    TEGRA_T_SATA0_CFG_SATA_BACKDOOR_PROG_IF_EN, 0);
	bus_space_write_4(bst, bsh, TEGRA_T_SATA0_BKDOOR_CC_REG,
	    __SHIFTIN(0x1016, TEGRA_T_SATA0_BKDOOR_CC_CLASS_CODE) |
	    __SHIFTIN(0x1, TEGRA_T_SATA0_BKDOOR_CC_PROG_IF));
	tegra_reg_set_clear(bst, bsh, TEGRA_T_SATA0_CFG_SATA_REG,
	    0, TEGRA_T_SATA0_CFG_SATA_BACKDOOR_PROG_IF_EN);

	/* Enable access and bus mastering */
	tegra_reg_set_clear(bst, bsh, TEGRA_T_SATA0_CFG1_REG,
	    TEGRA_T_SATA0_CFG1_SERR |
	    TEGRA_T_SATA0_CFG1_BUS_MASTER |
	    TEGRA_T_SATA0_CFG1_MEM_SPACE |
	    TEGRA_T_SATA0_CFG1_IO_SPACE,
	    0);

	/* MMIO setup */
	bus_space_write_4(bst, bsh, TEGRA_SATA_FPCI_BAR5_REG,
	    __SHIFTIN(0x10000, TEGRA_SATA_FPCI_BAR_START));
	bus_space_write_4(bst, bsh, TEGRA_T_SATA0_CFG9_REG,
	    __SHIFTIN(0x8000, TEGRA_T_SATA0_CFG9_BASE_ADDRESS));

	/* Enable interrupts */
	tegra_reg_set_clear(bst, bsh, TEGRA_SATA_INTR_MASK_REG,
	    TEGRA_SATA_INTR_MASK_IP_INT, 0);
}

static int
tegra_ahcisata_init_clocks(struct tegra_ahcisata_softc *sc)
{
	device_t self = sc->sc.sc_atac.atac_dev;
	int error;

	/* Assert resets */
	fdtbus_reset_assert(sc->sc_rst_sata);
	fdtbus_reset_assert(sc->sc_rst_sata_cold);

	/* Set SATA_OOB clock source to 204MHz */
	error = clk_set_rate(sc->sc_clk_sata_oob, 204000000);
	if (error) {
		aprint_error_dev(self, "couldn't set sata-oob rate: %d\n",
		    error);
		return error;
	}

	/* Set SATA clock source to 102MHz */
	error = clk_set_rate(sc->sc_clk_sata, 102000000);
	if (error) {
		aprint_error_dev(self, "couldn't set sata rate: %d\n", error);
		return error;
	}

	/* Ungate SAX partition in the PMC */
	tegra_pmc_power(PMC_PARTID_SAX, true);
	delay(20);

	/* Remove clamping from SAX partition in the PMC */
	tegra_pmc_remove_clamping(PMC_PARTID_SAX);
	delay(20);

	/* Un-gate clocks and enable CML clock for SATA */
	error = clk_enable(sc->sc_clk_sata);
	if (error) {
		aprint_error_dev(self, "couldn't enable sata: %d\n", error);
		return error;
	}
	error = clk_enable(sc->sc_clk_sata_oob);
	if (error) {
		aprint_error_dev(self, "couldn't enable sata-oob: %d\n", error);
		return error;
	}
	error = clk_enable(sc->sc_clk_cml1);
	if (error) {
		aprint_error_dev(self, "couldn't enable cml1: %d\n", error);
		return error;
	}

	/* De-assert resets */
	fdtbus_reset_deassert(sc->sc_rst_sata);
	fdtbus_reset_deassert(sc->sc_rst_sata_cold);

	return 0;
}
