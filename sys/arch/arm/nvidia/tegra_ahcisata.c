/* $NetBSD: tegra_ahcisata.c,v 1.7 2015/10/15 09:04:35 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_ahcisata.c,v 1.7 2015/10/15 09:04:35 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/ata/atavar.h>
#include <dev/ic/ahcisatavar.h>

#include <arm/nvidia/tegra_var.h>
#include <arm/nvidia/tegra_ahcisatareg.h>

#define TEGRA_AHCISATA_OFFSET	0x7000

static int	tegra_ahcisata_match(device_t, cfdata_t, void *);
static void	tegra_ahcisata_attach(device_t, device_t, void *);

struct tegra_ahcisata_softc {
	struct ahci_softc	sc;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	void			*sc_ih;

	struct tegra_gpio_pin	*sc_pin_power;
};

static void	tegra_ahcisata_init(struct tegra_ahcisata_softc *);

CFATTACH_DECL_NEW(tegra_ahcisata, sizeof(struct tegra_ahcisata_softc),
	tegra_ahcisata_match, tegra_ahcisata_attach, NULL, NULL);

static int
tegra_ahcisata_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_ahcisata_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_ahcisata_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;
	prop_dictionary_t prop = device_properties(self);
	const char *pin;

	sc->sc_bst = tio->tio_bst;
	bus_space_subregion(tio->tio_bst, tio->tio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	sc->sc.sc_atac.atac_dev = self;
	sc->sc.sc_dmat = tio->tio_dmat;
	sc->sc.sc_ahcit = tio->tio_bst;
	sc->sc.sc_ahcis = loc->loc_size - TEGRA_AHCISATA_OFFSET;
	bus_space_subregion(tio->tio_bst, tio->tio_bsh,
	    loc->loc_offset + TEGRA_AHCISATA_OFFSET,
	    loc->loc_size - TEGRA_AHCISATA_OFFSET, &sc->sc.sc_ahcih);
	sc->sc.sc_ahci_quirks = AHCI_QUIRK_SKIP_RESET;

	aprint_naive("\n");
	aprint_normal(": SATA\n");

	if (prop_dictionary_get_cstring_nocopy(prop, "power-gpio", &pin)) {
		sc->sc_pin_power = tegra_gpio_acquire(pin, GPIO_PIN_OUTPUT);
		if (sc->sc_pin_power)
			tegra_gpio_write(sc->sc_pin_power, 1);
	}

	tegra_car_periph_sata_enable();

	tegra_xusbpad_sata_enable();

	tegra_ahcisata_init(sc);

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_BIO, IST_LEVEL,
	    ahci_intr, &sc->sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

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
