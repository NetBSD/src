/*	$NetBSD: imxpcie.c,v 1.4 2021/08/07 16:18:45 thorpej Exp $	*/

/*
 * Copyright (c) 2019  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * i.MX6 On-Chip PCI Express Controller
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imxpcie.c,v 1.4 2021/08/07 16:18:45 thorpej Exp $");

#include "opt_pci.h"
#include "opt_fdt.h"

#include "pci.h"
#include "locators.h"

#define	_INTR_PRIVATE

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/extent.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/kmem.h>

#include <machine/frame.h>
#include <arm/cpufunc.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>
#include <dev/clk/clk_backend.h>

#include <arm/imx/imxpciereg.h>
#include <arm/imx/imxpcievar.h>
#include <arm/nxp/imx6_iomuxreg.h>

#define PCIE_CONF_LOCK(s)	(s) = disable_interrupts(I32_bit)
#define PCIE_CONF_UNLOCK(s)	restore_interrupts((s))

#define PCIE_READ(sc, reg)					\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, reg)
#define PCIE_WRITE(sc, reg, val)				\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, reg, val)

static void imxpcie_init(pci_chipset_tag_t, void *);
static void imxpcie_setup(struct imxpcie_softc * const);

static void imxpcie_attach_hook(device_t, device_t, struct pcibus_attach_args *);
static int imxpcie_bus_maxdevs(void *, int);
static pcitag_t imxpcie_make_tag(void *, int, int, int);
static void imxpcie_decompose_tag(void *, pcitag_t, int *, int *, int *);
static pcireg_t imxpcie_conf_read(void *, pcitag_t, int);
static void imxpcie_conf_write(void *, pcitag_t, int, pcireg_t);
#ifdef __HAVE_PCI_CONF_HOOK
static int imxpcie_conf_hook(void *, int, int, int, pcireg_t);
#endif
static void imxpcie_conf_interrupt(void *, int, int, int, int, int *);

static int imxpcie_intr_map(const struct pci_attach_args *, pci_intr_handle_t *);
static const char *imxpcie_intr_string(void *, pci_intr_handle_t, char *, size_t);
const struct evcnt *imxpcie_intr_evcnt(void *, pci_intr_handle_t);
static void * imxpcie_intr_establish(void *, pci_intr_handle_t, int,
    int (*)(void *), void *, const char *);
static void imxpcie_intr_disestablish(void *, void *);

static int
imxpcie_linkup_status(struct imxpcie_softc *sc)
{
	return PCIE_READ(sc, PCIE_PL_DEBUG1) & PCIE_PL_DEBUG1_XMLH_LINK_UP;
}

static int
imxpcie_valid_device(struct imxpcie_softc *sc, int bus, int dev)
{
	if (bus != 0 && !imxpcie_linkup_status(sc))
		return 0;
	if (bus <= 1 && dev > 0)
		return 0;

	return 1;
}

static int
imxpcie_init_phy(struct imxpcie_softc *sc)
{
	uint32_t v;

	/* initialize IOMUX */
	v = sc->sc_gpr_read(sc, IOMUX_GPR12);
	v &= ~IOMUX_GPR12_APP_LTSSM_ENABLE;
	sc->sc_gpr_write(sc, IOMUX_GPR12, v);

	v &= ~IOMUX_GPR12_LOS_LEVEL;
	v |= __SHIFTIN(9, IOMUX_GPR12_LOS_LEVEL);
	sc->sc_gpr_write(sc, IOMUX_GPR12, v);

	v = 0;
	v |= __SHIFTIN(0x7f, IOMUX_GPR8_PCS_TX_SWING_LOW);
	v |= __SHIFTIN(0x7f, IOMUX_GPR8_PCS_TX_SWING_FULL);
	v |= __SHIFTIN(20, IOMUX_GPR8_PCS_TX_DEEMPH_GEN2_6DB);
	v |= __SHIFTIN(20, IOMUX_GPR8_PCS_TX_DEEMPH_GEN2_3P5DB);
	v |= __SHIFTIN(20, IOMUX_GPR8_PCS_TX_DEEMPH_GEN1);
	sc->sc_gpr_write(sc, IOMUX_GPR8, v);

	v = sc->sc_gpr_read(sc, IOMUX_GPR12);
	v &= ~IOMUX_GPR12_DEVICE_TYPE;
	v |= IOMUX_GPR12_DEVICE_TYPE_PCIE_RC;
	sc->sc_gpr_write(sc, IOMUX_GPR12, v);

	return 0;
}

static int
imxpcie_phy_wait_ack(struct imxpcie_softc *sc, int ack)
{
	uint32_t v;
	int timeout;

	for (timeout = 10; timeout > 0; --timeout) {
		v = PCIE_READ(sc, PCIE_PL_PHY_STATUS);
		if (!!(v & PCIE_PL_PHY_STATUS_ACK) == !!ack)
			return 0;
		delay(1);
	}

	return -1;
}

static int
imxpcie_phy_addr(struct imxpcie_softc *sc, uint32_t addr)
{
	uint32_t v;

	v = __SHIFTIN(addr, PCIE_PL_PHY_CTRL_DATA);
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, v);

	v |= PCIE_PL_PHY_CTRL_CAP_ADR;
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, v);

	if (imxpcie_phy_wait_ack(sc, 1))
		return -1;

	v = __SHIFTIN(addr, PCIE_PL_PHY_CTRL_DATA);
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, v);

	if (imxpcie_phy_wait_ack(sc, 0))
		return -1;

	return 0;
}

static int
imxpcie_phy_write(struct imxpcie_softc *sc, uint32_t addr, uint16_t data)
{
	/* write address */
	if (imxpcie_phy_addr(sc, addr) != 0)
		return -1;

	/* store data */
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, __SHIFTIN(data, PCIE_PL_PHY_CTRL_DATA));

	/* assert CAP_DAT and wait ack */
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, __SHIFTIN(data, PCIE_PL_PHY_CTRL_DATA) | PCIE_PL_PHY_CTRL_CAP_DAT);
	if (imxpcie_phy_wait_ack(sc, 1))
		return -1;

	/* deassert CAP_DAT and wait ack */
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, __SHIFTIN(data, PCIE_PL_PHY_CTRL_DATA));
	if (imxpcie_phy_wait_ack(sc, 0))
		return -1;

	/* assert WR and wait ack */
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, PCIE_PL_PHY_CTRL_WR);
	if (imxpcie_phy_wait_ack(sc, 1))
		return -1;

	/* deassert WR and wait ack */
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, __SHIFTIN(data, PCIE_PL_PHY_CTRL_DATA));
	if (imxpcie_phy_wait_ack(sc, 0))
		return -1;

	/* done */
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, 0);

	return 0;
}

static int
imxpcie_phy_read(struct imxpcie_softc *sc, uint32_t addr)
{
	uint32_t v;

	/* write address */
	if (imxpcie_phy_addr(sc, addr) != 0)
		return -1;

	/* assert RD */
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, PCIE_PL_PHY_CTRL_RD);
	if (imxpcie_phy_wait_ack(sc, 1))
		return -1;

	/* read data */
	v = __SHIFTOUT(PCIE_READ(sc, PCIE_PL_PHY_STATUS),
	    PCIE_PL_PHY_STATUS_DATA);

	/* deassert RD */
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, 0);
	if (imxpcie_phy_wait_ack(sc, 0))
		return -1;

	return v;
}

static int
imxpcie_assert_core_reset(struct imxpcie_softc *sc)
{
	uint32_t gpr1 = sc->sc_gpr_read(sc, IOMUX_GPR1);

	if (sc->sc_have_sw_reset) {
		gpr1 |= IOMUX_GPR1_PCIE_SW_RST;
		sc->sc_gpr_write(sc, IOMUX_GPR1, gpr1);
	} else {
		uint32_t gpr12 = sc->sc_gpr_read(sc, IOMUX_GPR12);

		/* already enabled by bootloader */
		if ((gpr1 & IOMUX_GPR1_REF_SSP_EN) &&
		    (gpr12 & IOMUX_GPR12_APP_LTSSM_ENABLE)) {
			uint32_t v = PCIE_READ(sc, PCIE_PL_PFLR);
			v &= ~PCIE_PL_PFLR_LINK_STATE;
			v |= PCIE_PL_PFLR_FORCE_LINK;
			PCIE_WRITE(sc, PCIE_PL_PFLR, v);

			gpr12 &= ~IOMUX_GPR12_APP_LTSSM_ENABLE;
			sc->sc_gpr_write(sc, IOMUX_GPR12, gpr12);
		}
	}

	gpr1 |= IOMUX_GPR1_TEST_POWERDOWN;
	sc->sc_gpr_write(sc, IOMUX_GPR1, gpr1);
	gpr1 &= ~IOMUX_GPR1_REF_SSP_EN;
	sc->sc_gpr_write(sc, IOMUX_GPR1, gpr1);

	return 0;
}

static int
imxpcie_deassert_core_reset(struct imxpcie_softc *sc)
{
	int error;

	error = clk_enable(sc->sc_clk_pcie);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable pcie: %d\n", error);
		return error;
	}

	if (sc->sc_ext_osc) {
		error = clk_enable(sc->sc_clk_pcie_ext);
		if (error) {
			aprint_error_dev(sc->sc_dev, "couldn't enable ext: %d\n", error);
			return error;
		}
	} else {
		error = clk_enable(sc->sc_clk_pcie_bus);
		if (error) {
			aprint_error_dev(sc->sc_dev, "couldn't enable pcie_bus: %d\n",
			    error);
			return error;
		}
	}

	error = clk_enable(sc->sc_clk_pcie_phy);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable pcie_ref: %d\n", error);
		return error;
	}

	uint32_t gpr1 = sc->sc_gpr_read(sc, IOMUX_GPR1);

	delay(50 * 1000);

	gpr1 &= ~IOMUX_GPR1_TEST_POWERDOWN;
	sc->sc_gpr_write(sc, IOMUX_GPR1, gpr1);
	delay(10);
	gpr1 |= IOMUX_GPR1_REF_SSP_EN;
	sc->sc_gpr_write(sc, IOMUX_GPR1, gpr1);

	delay(50 * 1000);

	/* Reset */
	if (sc->sc_reset != NULL)
		sc->sc_reset(sc);

	if (sc->sc_have_sw_reset) {
		gpr1 &= ~IOMUX_GPR1_PCIE_SW_RST;
		sc->sc_gpr_write(sc, IOMUX_GPR1, gpr1);
		delay(200);
	}

	uint64_t rate;
	if (sc->sc_ext_osc)
		rate = clk_get_rate(sc->sc_clk_pcie_ext);
	else
		rate = clk_get_rate(sc->sc_clk_pcie_phy);
	aprint_normal_dev(sc->sc_dev, "PCIe ref clk %d MHz\n", (int)(rate / 1000 / 1000));

	int mult;
	int div;
	if (rate == 100000000) {
		mult = 25;
		div = 0;
	} else if (rate == 125000000) {
		mult = 40;
		div = 1;
	} else if (rate == 200000000) {
		mult = 25;
		div = 1;
	} else {
		return -1;
	}

	uint32_t val;
	val = imxpcie_phy_read(sc, PCIE_PHY_MPLL_OVRD_IN_LO);
	val &= ~MPLL_MULTIPLIER;
	val |= __SHIFTIN(mult, MPLL_MULTIPLIER);
	val |= MPLL_MULTIPLIER_OVRD;
	imxpcie_phy_write(sc, PCIE_PHY_MPLL_OVRD_IN_LO, val);

	val = imxpcie_phy_read(sc, PCIE_PHY_ATEOVRD);
	val &= ~REF_CLKDIV2;
	val |= __SHIFTIN(div, REF_CLKDIV2);
	val |= ATEOVRD_EN;
	imxpcie_phy_write(sc, PCIE_PHY_ATEOVRD, val);

	return 0;
}

static int
imxpcie_wait_for_link(struct imxpcie_softc *sc)
{
#define LINKUP_RETRY	20000
	for (int retry = LINKUP_RETRY; retry > 0; --retry) {
		if (!imxpcie_linkup_status(sc)) {
			delay(10);
			continue;
		}

		uint32_t valid = imxpcie_phy_read(sc, PCIE_PHY_RX_ASIC_OUT) &
		    PCIE_PHY_RX_ASIC_OUT_VALID;
		uint32_t ltssm = __SHIFTOUT(PCIE_READ(sc, PCIE_PL_DEBUG0),
		    PCIE_PL_DEBUG0_XMLH_LTSSM_STATE);

		if ((ltssm == 0x0d) && !valid) {
			aprint_normal_dev(sc->sc_dev, "resetting PCIe phy\n");

			uint32_t v = imxpcie_phy_read(sc, PCIE_PHY_RX_OVRD_IN_LO);
			v |= PCIE_PHY_RX_OVRD_IN_LO_RX_PLL_EN_OVRD;
			v |= PCIE_PHY_RX_OVRD_IN_LO_RX_DATA_EN_OVRD;
			imxpcie_phy_write(sc, PCIE_PHY_RX_OVRD_IN_LO, v);

			delay(3000);

			v = imxpcie_phy_read(sc, PCIE_PHY_RX_OVRD_IN_LO);
			v &= ~PCIE_PHY_RX_OVRD_IN_LO_RX_PLL_EN_OVRD;
			v &= ~PCIE_PHY_RX_OVRD_IN_LO_RX_DATA_EN_OVRD;
			imxpcie_phy_write(sc, PCIE_PHY_RX_OVRD_IN_LO, v);
		}

		return 0;
	}

	aprint_error_dev(sc->sc_dev, "Link Up failed.\n");

	return -1;
}

static int
imxpcie_wait_for_changespeed(struct imxpcie_softc *sc)
{
#define CHANGESPEED_RETRY	200
	for (int retry = CHANGESPEED_RETRY; retry > 0; --retry) {
		uint32_t v = PCIE_READ(sc, PCIE_PL_G2CR);
		if (!(v & PCIE_PL_G2CR_DIRECTED_SPEED_CHANGE))
			return 0;
		delay(100);
	}

	aprint_error_dev(sc->sc_dev, "Speed change timeout.\n");

	return -1;
}

static void
imxpcie_linkup(struct imxpcie_softc *sc)
{
	uint32_t v;
	int ret;

	imxpcie_assert_core_reset(sc);
	imxpcie_init_phy(sc);
	imxpcie_deassert_core_reset(sc);

	imxpcie_setup(sc);

	/* GEN1 Operation */
	v = PCIE_READ(sc, PCIE_RC_LCR);
	v &= ~PCIE_RC_LCR_MAX_LINK_SPEEDS;
	v |= PCIE_RC_LCR_MAX_LINK_SPEEDS_GEN1;
	PCIE_WRITE(sc, PCIE_RC_LCR, v);

	/* Link Up */
	v = sc->sc_gpr_read(sc, IOMUX_GPR12);
	v |= IOMUX_GPR12_APP_LTSSM_ENABLE;
	sc->sc_gpr_write(sc, IOMUX_GPR12, v);

	ret = imxpcie_wait_for_link(sc);
	if (ret)
		goto error;

	/* Allow Gen2 mode */
	v = PCIE_READ(sc, PCIE_RC_LCR);
	v &= ~PCIE_RC_LCR_MAX_LINK_SPEEDS;
	v |= PCIE_RC_LCR_MAX_LINK_SPEEDS_GEN2;
	PCIE_WRITE(sc, PCIE_RC_LCR, v);

	/* Change speed */
	v = PCIE_READ(sc, PCIE_PL_G2CR);
	v |= PCIE_PL_G2CR_DIRECTED_SPEED_CHANGE;
	PCIE_WRITE(sc, PCIE_PL_G2CR, v);

	ret = imxpcie_wait_for_changespeed(sc);
	if (ret)
		goto error;

	ret = imxpcie_wait_for_link(sc);
	if (ret)
		goto error;

	v = PCIE_READ(sc, PCIE_RC_LCSR);
	aprint_normal_dev(sc->sc_dev, "LinkUp, Gen %d\n",
	    (int)__SHIFTOUT(v, PCIE_RC_LCSR_LINK_SPEED));

	return;

error:
	aprint_error_dev(sc->sc_dev, "PCIE_PL_DEBUG0,1 = %08x, %08x\n",
	    PCIE_READ(sc, PCIE_PL_DEBUG0), PCIE_READ(sc, PCIE_PL_DEBUG1));

	return;
}

void
imxpcie_attach_common(struct imxpcie_softc * const sc)
{
	struct pcibus_attach_args pba;

	if (bus_space_map(sc->sc_iot, sc->sc_root_addr, sc->sc_root_size, 0,
		&sc->sc_root_ioh)) {
		aprint_error_dev(sc->sc_dev, "Cannot map root config\n");
		return;
	}

	imxpcie_linkup(sc);

	TAILQ_INIT(&sc->sc_intrs);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	imxpcie_init(&sc->sc_pc, sc);

	if (sc->sc_pci_netbsd_configure != NULL)
		sc->sc_pci_netbsd_configure(sc);

	memset(&pba, 0, sizeof(pba));
	pba.pba_flags = PCI_FLAGS_MEM_OKAY |
	    PCI_FLAGS_IO_OKAY;
	pba.pba_iot = sc->sc_iot;
	pba.pba_memt = sc->sc_iot;
	pba.pba_dmat = sc->sc_dmat;
	pba.pba_pc = &sc->sc_pc;
	pba.pba_bus = 0;

	config_found(sc->sc_dev, &pba, pcibusprint,
	    CFARGS(.devhandle = device_handle(sc->sc_dev)));
}

int
imxpcie_intr(void *priv)
{
	struct imxpcie_softc *sc = priv;
	struct imxpcie_ih *pcie_ih;

	for (int i = 0; i < 8; i++) {
		uint32_t v = PCIE_READ(sc, PCIE_PL_MSICIN_STATUS + i * 0xC);
		int bit;
		while ((bit = ffs(v) - 1) >= 0) {
			PCIE_WRITE(sc, PCIE_PL_MSICIN_STATUS + i * 0xC,
			    __BIT(bit));
			v &= ~__BIT(bit);
		}
	}

	mutex_enter(&sc->sc_lock);
	int rv = 0;
	const u_int lastgen = sc->sc_intrgen;
	TAILQ_FOREACH(pcie_ih, &sc->sc_intrs, ih_entry) {
		int (*callback)(void *) = pcie_ih->ih_handler;
		void *arg = pcie_ih->ih_arg;
		mutex_exit(&sc->sc_lock);
		rv += callback(arg);
		mutex_enter(&sc->sc_lock);
		if (lastgen != sc->sc_intrgen)
			break;
	}
	mutex_exit(&sc->sc_lock);

	return rv;
}

static void
imxpcie_setup(struct imxpcie_softc * const sc)
{
	uint32_t v;

	/* Setup RC */
	v = PCIE_READ(sc, PCIE_PL_PLCR);
	v &= ~PCIE_PL_PLCR_LINK_MODE_ENABLE;
	v |= __SHIFTIN(1, PCIE_PL_PLCR_LINK_MODE_ENABLE);
	PCIE_WRITE(sc, PCIE_PL_PLCR, v);

	v = PCIE_READ(sc, PCIE_PL_G2CR);
	v &= ~PCIE_PL_G2CR_PREDETERMINED_NUMBER_OF_LANES;
	v |= __SHIFTIN(1, PCIE_PL_G2CR_PREDETERMINED_NUMBER_OF_LANES);
	PCIE_WRITE(sc, PCIE_PL_G2CR, v);

	/* BARs */
	PCIE_WRITE(sc, PCI_BAR0, 0x00000004);
	PCIE_WRITE(sc, PCI_BAR1, 0x00000000);

	/* Interurupt pins */
	v = PCIE_READ(sc, PCI_INTERRUPT_REG);
	v &= ~(PCI_INTERRUPT_PIN_MASK << PCI_INTERRUPT_PIN_SHIFT);
	v |= PCI_INTERRUPT_PIN_A << PCI_INTERRUPT_PIN_SHIFT;
	PCIE_WRITE(sc, PCI_INTERRUPT_REG, v);

	/* Bus number */
	v = PCIE_READ(sc, PCI_BRIDGE_BUS_REG);
	v &= ~(PCI_BRIDGE_BUS_SUBORDINATE | PCI_BRIDGE_BUS_SECONDARY |
	    PCI_BRIDGE_BUS_PRIMARY);
	v |= PCI_BRIDGE_BUS_NUM_SUBORDINATE(1);
	v |= PCI_BRIDGE_BUS_NUM_SECONDARY(1);
	v |= PCI_BRIDGE_BUS_NUM_PRIMARY(0);
	PCIE_WRITE(sc, PCI_BRIDGE_BUS_REG, v);

	/* Command register */
	v = PCIE_READ(sc, PCI_COMMAND_STATUS_REG);
	v |= PCI_COMMAND_IO_ENABLE |
	    PCI_COMMAND_MEM_ENABLE |
	    PCI_COMMAND_MASTER_ENABLE |
	    PCI_COMMAND_SERR_ENABLE;
	PCIE_WRITE(sc, PCI_COMMAND_STATUS_REG, v);

	PCIE_WRITE(sc, PCI_CLASS_REG,
	    PCI_CLASS_CODE(PCI_CLASS_BRIDGE,
		PCI_SUBCLASS_BRIDGE_PCI,
		PCI_INTERFACE_BRIDGE_PCI_PCI));

	PCIE_WRITE(sc, PCIE_PL_IATUVR, 0);

	PCIE_WRITE(sc, PCIE_PL_IATURLBA, sc->sc_root_addr);
	PCIE_WRITE(sc, PCIE_PL_IATURUBA, 0);
	PCIE_WRITE(sc, PCIE_PL_IATURLA, sc->sc_root_addr + sc->sc_root_size);

	PCIE_WRITE(sc, PCIE_PL_IATURLTA, 0);
	PCIE_WRITE(sc, PCIE_PL_IATURUTA, 0);
	PCIE_WRITE(sc, PCIE_PL_IATURC1, PCIE_PL_IATURC1_TYPE_CFG0);
	PCIE_WRITE(sc, PCIE_PL_IATURC2, PCIE_PL_IATURC2_REGION_ENABLE);
}

void
imxpcie_init(pci_chipset_tag_t pc, void *priv)
{
	pc->pc_conf_v = priv;
	pc->pc_attach_hook = imxpcie_attach_hook;
	pc->pc_bus_maxdevs = imxpcie_bus_maxdevs;
	pc->pc_make_tag = imxpcie_make_tag;
	pc->pc_decompose_tag = imxpcie_decompose_tag;
	pc->pc_conf_read = imxpcie_conf_read;
	pc->pc_conf_write = imxpcie_conf_write;
#ifdef __HAVE_PCI_CONF_HOOK
	pc->pc_conf_hook = imxpcie_conf_hook;
#endif
	pc->pc_conf_interrupt = imxpcie_conf_interrupt;

	pc->pc_intr_v = priv;
	pc->pc_intr_map = imxpcie_intr_map;
	pc->pc_intr_string = imxpcie_intr_string;
	pc->pc_intr_evcnt = imxpcie_intr_evcnt;
	pc->pc_intr_establish = imxpcie_intr_establish;
	pc->pc_intr_disestablish = imxpcie_intr_disestablish;
}

static void
imxpcie_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
	/* nothing to do */
}

static int
imxpcie_bus_maxdevs(void *v, int busno)
{
	return 32;
}

static pcitag_t
imxpcie_make_tag(void *v, int b, int d, int f)
{
	return (b << 16) | (d << 11) | (f << 8);
}

static void
imxpcie_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{
	if (bp)
		*bp = (tag >> 16) & 0xff;
	if (dp)
		*dp = (tag >> 11) & 0x1f;
	if (fp)
		*fp = (tag >> 8) & 0x7;
}

/*
 * work around.
 * If there is no PCIe devices, DABT will be generated by read/write access to
 * config area, so replace original DABT handler with simple jump-back one.
 */
extern u_int data_abort_handler_address;
static bool data_abort_flag;
static void
imxpcie_data_abort_handler(trapframe_t *tf)
{
	data_abort_flag = true;
	tf->tf_pc += 0x4;
	return;
}

static pcireg_t
imxpcie_conf_read(void *v, pcitag_t tag, int offset)
{
	struct imxpcie_softc *sc = v;
	bus_space_handle_t bsh;
	int b, d, f;
	pcireg_t ret = -1;
	int s;

	imxpcie_decompose_tag(v, tag, &b, &d, &f);

	if ((unsigned int)offset >= PCI_EXTCONF_SIZE)
		return ret;
	if (!imxpcie_valid_device(sc, b, d))
		return ret;

	PCIE_WRITE(sc, PCIE_PL_IATUVR, 0);
	if (b < 2)
		PCIE_WRITE(sc, PCIE_PL_IATURC1, PCIE_PL_IATURC1_TYPE_CFG0);
	else
		PCIE_WRITE(sc, PCIE_PL_IATURC1, PCIE_PL_IATURC1_TYPE_CFG1);

	if (b == 0) {
		bsh = sc->sc_ioh;
	} else {
		PCIE_WRITE(sc, PCIE_PL_IATURLTA, tag << 8);
		bsh = sc->sc_root_ioh;
	}
	PCIE_READ(sc, PCIE_PL_IATURC2);

	PCIE_CONF_LOCK(s);

	u_int saved = data_abort_handler_address;
	data_abort_handler_address = (u_int)imxpcie_data_abort_handler;
	data_abort_flag = false;

	ret = bus_space_read_4(sc->sc_iot, bsh, offset & ~0x3);

	data_abort_handler_address = saved;

	PCIE_CONF_UNLOCK(s);

	if (data_abort_flag)
		ret = -1;

	return ret;
}

static void
imxpcie_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	struct imxpcie_softc *sc = v;
	bus_space_handle_t bsh;
	int b, d, f;
	int s;

	imxpcie_decompose_tag(v, tag, &b, &d, &f);

	if ((unsigned int)offset >= PCI_EXTCONF_SIZE)
		return;
	if (!imxpcie_valid_device(sc, b, d))
		return;

	PCIE_WRITE(sc, PCIE_PL_IATUVR, 0);
	if (b < 2)
		PCIE_WRITE(sc, PCIE_PL_IATURC1, PCIE_PL_IATURC1_TYPE_CFG0);
	else
		PCIE_WRITE(sc, PCIE_PL_IATURC1, PCIE_PL_IATURC1_TYPE_CFG1);

	if (b == 0) {
		bsh = sc->sc_ioh;
	} else {
		PCIE_WRITE(sc, PCIE_PL_IATURLTA, tag << 8);
		bsh = sc->sc_root_ioh;
	}
	PCIE_READ(sc, PCIE_PL_IATURC2);

	PCIE_CONF_LOCK(s);

	u_int saved = data_abort_handler_address;
	data_abort_handler_address = (u_int)imxpcie_data_abort_handler;

	bus_space_write_4(sc->sc_iot, bsh, offset & ~0x3, val);

	data_abort_handler_address = saved;

	PCIE_CONF_UNLOCK(s);

	return;
}

#ifdef __HAVE_PCI_CONF_HOOK
static int
imxpcie_conf_hook(void *v, int b, int d, int f, pcireg_t id)
{
	return PCI_CONF_DEFAULT;
}
#endif

static void
imxpcie_conf_interrupt(void *v, int bus, int dev, int ipin, int swiz,
    int *ilinep)
{
	/* nothing to do */
}

static int
imxpcie_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ih)
{
	if (pa->pa_intrpin == 0)
		return EINVAL;
	*ih = pa->pa_intrpin;
	return 0;
}

static const char *
imxpcie_intr_string(void *v, pci_intr_handle_t ih, char *buf, size_t len)
{
	if (ih == PCI_INTERRUPT_PIN_NONE)
		return NULL;

	snprintf(buf, len, "pci");

	return buf;
}

const struct evcnt *
imxpcie_intr_evcnt(void *v, pci_intr_handle_t ih)
{
	return NULL;
}

static void *
imxpcie_intr_establish(void *v, pci_intr_handle_t ih, int ipl,
    int (*callback)(void *), void *arg, const char *xname)
{
	struct imxpcie_softc *sc = v;
	struct imxpcie_ih *pcie_ih;

	if (ih == 0)
		return NULL;

	pcie_ih = kmem_alloc(sizeof(*pcie_ih), KM_SLEEP);
	pcie_ih->ih_handler = callback;
	pcie_ih->ih_arg = arg;
	pcie_ih->ih_ipl = ipl;

	mutex_enter(&sc->sc_lock);
	TAILQ_INSERT_TAIL(&sc->sc_intrs, pcie_ih, ih_entry);
	sc->sc_intrgen++;
	mutex_exit(&sc->sc_lock);

	return pcie_ih;
}

static void
imxpcie_intr_disestablish(void *v, void *vih)
{
	struct imxpcie_softc *sc = v;
	struct imxpcie_ih *pcie_ih = vih;

	mutex_enter(&sc->sc_lock);
	TAILQ_REMOVE(&sc->sc_intrs, pcie_ih, ih_entry);
	sc->sc_intrgen++;
	mutex_exit(&sc->sc_lock);

	kmem_free(pcie_ih, sizeof(*pcie_ih));
}
