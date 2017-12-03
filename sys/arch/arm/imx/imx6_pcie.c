/*	$NetBSD: imx6_pcie.c,v 1.5.2.2 2017/12/03 11:35:53 jdolecek Exp $	*/

/*
 * Copyright (c) 2016  Genetec Corporation.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: imx6_pcie.c,v 1.5.2.2 2017/12/03 11:35:53 jdolecek Exp $");

#include "opt_pci.h"

#include "pci.h"
#include "imxgpio.h"
#include "locators.h"

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
#include <sys/gpio.h>

#include <machine/frame.h>
#include <arm/cpufunc.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <arm/imx/imxgpioreg.h>
#include <arm/imx/imxgpiovar.h>
#include <arm/imx/imx6var.h>
#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6_pciereg.h>
#include <arm/imx/imx6_iomuxreg.h>
#include <arm/imx/imx6_ccmreg.h>
#include <arm/imx/imx6_ccmvar.h>

static int imx6pcie_match(device_t, cfdata_t, void *);
static void imx6pcie_attach(device_t, device_t, void *);

#define IMX6_PCIE_MEM_BASE	0x01000000
#define IMX6_PCIE_MEM_SIZE	0x00f00000 /* 15MB */
#define IMX6_PCIE_ROOT_BASE	0x01f00000
#define IMX6_PCIE_ROOT_SIZE	0x00080000 /* 512KB */
#define IMX6_PCIE_IO_BASE	0x01f80000
#define IMX6_PCIE_IO_SIZE	0x00010000 /* 64KB */

struct imx6pcie_ih {
	int (*ih_handler)(void *);
	void *ih_arg;
	int ih_ipl;
	TAILQ_ENTRY(imx6pcie_ih) ih_entry;
};

struct imx6pcie_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_space_handle_t sc_root_ioh;
	bus_dma_tag_t sc_dmat;

	struct arm32_pci_chipset sc_pc;

	TAILQ_HEAD(, imx6pcie_ih) sc_intrs;

	void *sc_ih;
	kmutex_t sc_lock;
	u_int sc_intrgen;

	int32_t sc_gpio_reset;
	int32_t sc_gpio_reset_active;
	int32_t sc_gpio_pwren;
	int32_t sc_gpio_pwren_active;
};

#define PCIE_CONF_LOCK(s)	(s) = disable_interrupts(I32_bit)
#define PCIE_CONF_UNLOCK(s)	restore_interrupts((s))

#define PCIE_READ(sc, reg)					\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, reg)
#define PCIE_WRITE(sc, reg, val)				\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, reg, val)

static int imx6pcie_intr(void *);
static void imx6pcie_init(pci_chipset_tag_t, void *);
static void imx6pcie_setup(struct imx6pcie_softc * const);

static void imx6pcie_attach_hook(device_t, device_t,
				       struct pcibus_attach_args *);
static int imx6pcie_bus_maxdevs(void *, int);
static pcitag_t imx6pcie_make_tag(void *, int, int, int);
static void imx6pcie_decompose_tag(void *, pcitag_t, int *, int *, int *);
static pcireg_t imx6pcie_conf_read(void *, pcitag_t, int);
static void imx6pcie_conf_write(void *, pcitag_t, int, pcireg_t);
#ifdef __HAVE_PCI_CONF_HOOK
static int imx6pcie_conf_hook(void *, int, int, int, pcireg_t);
#endif
static void imx6pcie_conf_interrupt(void *, int, int, int, int, int *);

static int imx6pcie_intr_map(const struct pci_attach_args *,
				    pci_intr_handle_t *);
static const char *imx6pcie_intr_string(void *, pci_intr_handle_t,
					  char *, size_t);
const struct evcnt *imx6pcie_intr_evcnt(void *, pci_intr_handle_t);
static void * imx6pcie_intr_establish(void *, pci_intr_handle_t,
					 int, int (*)(void *), void *);
static void imx6pcie_intr_disestablish(void *, void *);

CFATTACH_DECL_NEW(imxpcie, sizeof(struct imx6pcie_softc),
    imx6pcie_match, imx6pcie_attach, NULL, NULL);

static int
imx6pcie_linkup_status(struct imx6pcie_softc *sc)
{
	return PCIE_READ(sc, PCIE_PL_DEBUG1) & PCIE_PL_DEBUG1_XMLH_LINK_UP;
}

static int
imx6pcie_valid_device(struct imx6pcie_softc *sc, int bus, int dev)
{
	if (bus != 0 && !imx6pcie_linkup_status(sc))
		return 0;
	if (bus <= 1 && dev > 0)
		return 0;

	return 1;
}

static void
imx6pcie_clock_enable(struct imx6pcie_softc *sc)
{
	uint32_t v;

	v = imx6_ccm_analog_read(CCM_ANALOG_MISC1);
	v &= ~CCM_ANALOG_MISC1_LVDS_CLK1_IBEN;
	v &= ~CCM_ANALOG_MISC1_LVDS_CLK1_SRC;
	v |= CCM_ANALOG_MISC1_LVDS_CLK1_OBEN;
	v |= CCM_ANALOG_MISC1_LVDS_CLK1_SRC_SATA;
	imx6_ccm_analog_write(CCM_ANALOG_MISC1, v);

	/* select PCIe clock source from axi */
	v = imx6_ccm_read(CCM_CBCMR);
	v &= ~CCM_CBCMR_PCIE_AXI_CLK_SEL;
	imx6_ccm_write(CCM_CBCMR, v);

	/* AHCISATA clock enable */
	v = imx6_ccm_read(CCM_CCGR5);
	v |= CCM_CCGR5_100M_CLK_ENABLE(3);
	imx6_ccm_write(CCM_CCGR5, v);

	/* PCIe clock enable */
	v = imx6_ccm_read(CCM_CCGR4);
	v |= CCM_CCGR4_125M_ROOT_ENABLE(3);
	imx6_ccm_write(CCM_CCGR4, v);

	/* PLL power up */
	if (imx6_pll_power(CCM_ANALOG_PLL_ENET, 1,
		CCM_ANALOG_PLL_ENET_ENABLE_125M |
		CCM_ANALOG_PLL_ENET_ENABLE_100M) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't enable CCM_ANALOG_PLL_ENET\n");
		return;
	}
}

static int
imx6pcie_init_phy(struct imx6pcie_softc *sc)
{
	uint32_t v;

	/* initialize IOMUX */
	v = iomux_read(IOMUX_GPR12);
	v &= ~IOMUX_GPR12_APP_LTSSM_ENABLE;
	iomux_write(IOMUX_GPR12, v);

	v &= ~IOMUX_GPR12_DEVICE_TYPE;
	v |= IOMUX_GPR12_DEVICE_TYPE_PCIE_RC;
	iomux_write(IOMUX_GPR12, v);

	v &= ~IOMUX_GPR12_LOS_LEVEL;
	v |= __SHIFTIN(9, IOMUX_GPR12_LOS_LEVEL);
	iomux_write(IOMUX_GPR12, v);

	v = 0;
	v |= __SHIFTIN(0x7f, IOMUX_GPR8_PCS_TX_SWING_LOW);
	v |= __SHIFTIN(0x7f, IOMUX_GPR8_PCS_TX_SWING_FULL);
	v |= __SHIFTIN(20, IOMUX_GPR8_PCS_TX_DEEMPH_GEN2_6DB);
	v |= __SHIFTIN(0, IOMUX_GPR8_PCS_TX_DEEMPH_GEN2_3P5DB);
	v |= __SHIFTIN(0, IOMUX_GPR8_PCS_TX_DEEMPH_GEN1);
	iomux_write(IOMUX_GPR8, v);

	return 0;
}

static int
imx6pcie_phy_wait_ack(struct imx6pcie_softc *sc, int ack)
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
imx6pcie_phy_addr(struct imx6pcie_softc *sc, uint32_t addr)
{
	uint32_t v;

	v = __SHIFTIN(addr, PCIE_PL_PHY_CTRL_DATA);
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, v);

	v |= PCIE_PL_PHY_CTRL_CAP_ADR;
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, v);

	if (imx6pcie_phy_wait_ack(sc, 1))
		return -1;

	v = __SHIFTIN(addr, PCIE_PL_PHY_CTRL_DATA);
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, v);

	if (imx6pcie_phy_wait_ack(sc, 0))
		return -1;

	return 0;
}

static int
imx6pcie_phy_write(struct imx6pcie_softc *sc, uint32_t addr, uint16_t data)
{
	/* write address */
	if (imx6pcie_phy_addr(sc, addr) != 0)
		return -1;

	/* store data */
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, __SHIFTIN(data, PCIE_PL_PHY_CTRL_DATA));

	/* assert CAP_DAT and wait ack */
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, __SHIFTIN(data, PCIE_PL_PHY_CTRL_DATA) | PCIE_PL_PHY_CTRL_CAP_DAT);
	if (imx6pcie_phy_wait_ack(sc, 1))
		return -1;

	/* deassert CAP_DAT and wait ack */
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, __SHIFTIN(data, PCIE_PL_PHY_CTRL_DATA));
	if (imx6pcie_phy_wait_ack(sc, 0))
		return -1;

	/* assert WR and wait ack */
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, PCIE_PL_PHY_CTRL_WR);
	if (imx6pcie_phy_wait_ack(sc, 1))
		return -1;

	/* deassert WR and wait ack */
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, __SHIFTIN(data, PCIE_PL_PHY_CTRL_DATA));
	if (imx6pcie_phy_wait_ack(sc, 0))
		return -1;

	/* done */
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, 0);

	return 0;
}

static int
imx6pcie_phy_read(struct imx6pcie_softc *sc, uint32_t addr)
{
	uint32_t v;

	/* write address */
	if (imx6pcie_phy_addr(sc, addr) != 0)
		return -1;

	/* assert RD */
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, PCIE_PL_PHY_CTRL_RD);
	if (imx6pcie_phy_wait_ack(sc, 1))
		return -1;

	/* read data */
	v = __SHIFTOUT(PCIE_READ(sc, PCIE_PL_PHY_STATUS),
	    PCIE_PL_PHY_STATUS_DATA);

	/* deassert RD */
	PCIE_WRITE(sc, PCIE_PL_PHY_CTRL, 0);
	if (imx6pcie_phy_wait_ack(sc, 0))
		return -1;

	return v;
}

static int
imx6pcie_assert_core_reset(struct imx6pcie_softc *sc)
{
	uint32_t gpr1;
	uint32_t gpr12;

	gpr1 = iomux_read(IOMUX_GPR1);
	gpr12 = iomux_read(IOMUX_GPR12);

	/* already enabled by bootloader */
	if ((gpr1 & IOMUX_GPR1_REF_SSP_EN) &&
	    (gpr12 & IOMUX_GPR12_APP_LTSSM_ENABLE)) {
		uint32_t v = PCIE_READ(sc, PCIE_PL_PFLR);
		v &= ~PCIE_PL_PFLR_LINK_STATE;
		v |= PCIE_PL_PFLR_FORCE_LINK;
		PCIE_WRITE(sc, PCIE_PL_PFLR, v);

		gpr12 &= ~IOMUX_GPR12_APP_LTSSM_ENABLE;
		iomux_write(IOMUX_GPR12, gpr12);
	}

#if defined(IMX6DQP)
	gpr1 |= IOMUX_GPR1_PCIE_SW_RST;
	iomux_write(IOMUX_GPR1, gpr1);
#endif

	gpr1 |= IOMUX_GPR1_TEST_POWERDOWN;
	iomux_write(IOMUX_GPR1, gpr1);
	gpr1 &= ~IOMUX_GPR1_REF_SSP_EN;
	iomux_write(IOMUX_GPR1, gpr1);

	return 0;
}

static int
imx6pcie_deassert_core_reset(struct imx6pcie_softc *sc)
{
	uint32_t v;

	/* Power ON */
#if NIMXGPIO > 0
	if (sc->sc_gpio_pwren >= 0) {
		gpio_data_write(sc->sc_gpio_pwren, sc->sc_gpio_pwren_active);
		delay(100 * 1000);
		gpio_data_write(sc->sc_gpio_pwren, !sc->sc_gpio_pwren_active);
	}
#endif

	imx6pcie_clock_enable(sc);

	v = iomux_read(IOMUX_GPR1);

#if defined(IMX6DQP)
	v &= ~IOMUX_GPR1_PCIE_SW_RST;
	iomux_write(IOMUX_GPR1, v);
#endif

	delay(50 * 1000);

	v &= ~IOMUX_GPR1_TEST_POWERDOWN;
	iomux_write(IOMUX_GPR1, v);
	delay(10);
	v |= IOMUX_GPR1_REF_SSP_EN;
	iomux_write(IOMUX_GPR1, v);

	delay(50 * 1000);

	/* Reset */
#if NIMXGPIO > 0
	if (sc->sc_gpio_reset >= 0) {
		gpio_data_write(sc->sc_gpio_reset, sc->sc_gpio_reset_active);
		delay(100 * 1000);
		gpio_data_write(sc->sc_gpio_reset, !sc->sc_gpio_reset_active);
	}
#endif

	return 0;
}

static int
imx6pcie_wait_for_link(struct imx6pcie_softc *sc)
{
	uint32_t ltssm, valid, v;
	int retry;

#define LINKUP_RETRY	200
	for (retry = LINKUP_RETRY; retry > 0; --retry) {
		if (!imx6pcie_linkup_status(sc)) {
			delay(100);
			continue;
		}

		valid = imx6pcie_phy_read(sc, PCIE_PHY_RX_ASIC_OUT) &
		    PCIE_PHY_RX_ASIC_OUT_VALID;
		ltssm = __SHIFTOUT(PCIE_READ(sc, PCIE_PL_DEBUG0),
		    PCIE_PL_DEBUG0_XMLH_LTSSM_STATE);

		if ((ltssm == 0x0d) && !valid) {
			aprint_normal_dev(sc->sc_dev, "resetting PCIe phy\n");

			v = imx6pcie_phy_read(sc, PCIE_PHY_RX_OVRD_IN_LO);
			v |= PCIE_PHY_RX_OVRD_IN_LO_RX_PLL_EN_OVRD;
			v |= PCIE_PHY_RX_OVRD_IN_LO_RX_DATA_EN_OVRD;
			imx6pcie_phy_write(sc, PCIE_PHY_RX_OVRD_IN_LO, v);

			delay(3000);

			v = imx6pcie_phy_read(sc, PCIE_PHY_RX_OVRD_IN_LO);
			v &= ~PCIE_PHY_RX_OVRD_IN_LO_RX_PLL_EN_OVRD;
			v &= ~PCIE_PHY_RX_OVRD_IN_LO_RX_DATA_EN_OVRD;
			imx6pcie_phy_write(sc, PCIE_PHY_RX_OVRD_IN_LO, v);
		}

		return 0;
	}

	aprint_error_dev(sc->sc_dev, "Link Up failed.\n");

	return -1;
}

static int
imx6pcie_wait_for_changespeed(struct imx6pcie_softc *sc)
{
	uint32_t v;
	int retry;

#define CHANGESPEED_RETRY	200
	for (retry = CHANGESPEED_RETRY; retry > 0; --retry) {
		v = PCIE_READ(sc, PCIE_PL_G2CR);
		if (!(v & PCIE_PL_G2CR_DIRECTED_SPEED_CHANGE))
			return 0;
		delay(100);
	}

	aprint_error_dev(sc->sc_dev, "Speed change timeout.\n");

	return -1;
}

static void
imx6pcie_linkup(struct imx6pcie_softc *sc)
{
	uint32_t v;
	int ret;

	imx6pcie_assert_core_reset(sc);
	imx6pcie_init_phy(sc);
	imx6pcie_deassert_core_reset(sc);

	imx6pcie_setup(sc);

	/* GEN1 Operation */
	v = PCIE_READ(sc, PCIE_RC_LCR);
	v &= ~PCIE_RC_LCR_MAX_LINK_SPEEDS;
	v |= PCIE_RC_LCR_MAX_LINK_SPEEDS_GEN1;
	PCIE_WRITE(sc, PCIE_RC_LCR, v);

	/* Link Up */
	v = iomux_read(IOMUX_GPR12);
	v |= IOMUX_GPR12_APP_LTSSM_ENABLE;
	iomux_write(IOMUX_GPR12, v);

	ret = imx6pcie_wait_for_link(sc);
	if (ret)
		goto error;

	/* Change speed */
	v = PCIE_READ(sc, PCIE_PL_G2CR);
	v |= PCIE_PL_G2CR_DIRECTED_SPEED_CHANGE;
	PCIE_WRITE(sc, PCIE_PL_G2CR, v);

	ret = imx6pcie_wait_for_changespeed(sc);
	if (ret)
		goto error;

	/* Allow Gen2 mode */
	v = PCIE_READ(sc, PCIE_RC_LCR);
	v &= ~PCIE_RC_LCR_MAX_LINK_SPEEDS;
	v |= PCIE_RC_LCR_MAX_LINK_SPEEDS_GEN2;
	PCIE_WRITE(sc, PCIE_RC_LCR, v);

	ret = imx6pcie_wait_for_link(sc);
	if (ret)
		goto error;

	v = PCIE_READ(sc, PCIE_RC_LCSR);
	aprint_normal_dev(sc->sc_dev, "LinkUp, Gen %d\n",
	    (int)__SHIFTOUT(v, PCIE_RC_LCSR_LINK_SPEED));

	return;

error:
	aprint_error_dev(sc->sc_dev,
	    "PCIE_PL_DEBUG0,1 = %08x, %08x\n",
	    PCIE_READ(sc, PCIE_PL_DEBUG0),
	    PCIE_READ(sc, PCIE_PL_DEBUG1));

	return;
}

static int
imx6pcie_match(device_t parent, cfdata_t cf, void *aux)
{
	struct axi_attach_args * const aa = aux;

	/* i.MX6 SoloLite/UltraLight has no PCIe controller */
	switch (IMX6_CHIPID_MAJOR(imx6_chip_id())) {
	case CHIPID_MAJOR_IMX6SL:
	case CHIPID_MAJOR_IMX6UL:
		return 0;
	default:
		break;
	}

	switch (aa->aa_addr) {
	case (IMX6_PCIE_BASE):
		return 1;
	}

	return 0;
}

static void
imx6pcie_attach(device_t parent, device_t self, void *aux)
{
	struct imx6pcie_softc * const sc = device_private(self);
	struct axi_attach_args * const aa = aux;
	struct pcibus_attach_args pba;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->sc_dmat = aa->aa_dmat;

	if (aa->aa_size == AXICF_SIZE_DEFAULT)
		aa->aa_size = IMX6_PCIE_SIZE;

	aprint_naive("\n");
	aprint_normal(": PCI Express Controller\n");

	if (bus_space_map(sc->sc_iot, aa->aa_addr, aa->aa_size, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(self, "Cannot map registers\n");
		return;
	}
	if (bus_space_map(sc->sc_iot, IMX6_PCIE_ROOT_BASE,
	    IMX6_PCIE_ROOT_SIZE, 0, &sc->sc_root_ioh)) {
		aprint_error_dev(self, "Cannot map registers\n");
		return;
	}

	imx6_set_gpio(self, "imx6pcie-reset-gpio", &sc->sc_gpio_reset,
	    &sc->sc_gpio_reset_active, GPIO_DIR_OUT);
	imx6_set_gpio(self, "imx6pcie-pwren-gpio", &sc->sc_gpio_pwren,
	    &sc->sc_gpio_pwren_active, GPIO_DIR_OUT);

	imx6pcie_linkup(sc);

	TAILQ_INIT(&sc->sc_intrs);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	sc->sc_ih = intr_establish(aa->aa_irq, IPL_VM, IST_LEVEL,
	    imx6pcie_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %d\n",
		    aa->aa_irq);
		return;
	}
	aprint_normal_dev(self, "interrupting on %d\n", aa->aa_irq);

	imx6pcie_init(&sc->sc_pc, sc);

#ifdef PCI_NETBSD_CONFIGURE
	struct extent *ioext, *memext;
	int error;

	ioext = extent_create("pciio", IMX6_PCIE_IO_BASE,
	    IMX6_PCIE_IO_BASE + IMX6_PCIE_IO_SIZE - 1,
	    NULL, 0, EX_NOWAIT);
	memext = extent_create("pcimem", IMX6_PCIE_MEM_BASE,
	    IMX6_PCIE_MEM_BASE + IMX6_PCIE_MEM_SIZE - 1,
	    NULL, 0, EX_NOWAIT);

	error = pci_configure_bus(&sc->sc_pc, ioext, memext, NULL, 0,
	    arm_dcache_align);

	extent_destroy(ioext);
	extent_destroy(memext);

	if (error) {
		aprint_error_dev(self, "configuration failed (%d)\n",
		    error);
		return;
	}
#endif

	memset(&pba, 0, sizeof(pba));
	pba.pba_flags = PCI_FLAGS_MEM_OKAY |
			PCI_FLAGS_IO_OKAY;
	pba.pba_iot = sc->sc_iot;
	pba.pba_memt = sc->sc_iot;
	pba.pba_dmat = sc->sc_dmat;
	pba.pba_pc = &sc->sc_pc;
	pba.pba_bus = 0;

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

static int
imx6pcie_intr(void *priv)
{
	struct imx6pcie_softc *sc = priv;
	struct imx6pcie_ih *pcie_ih;
	int rv = 0;
	uint32_t v;

	for (int i = 0; i < 8; i++) {
		v = PCIE_READ(sc, PCIE_PL_MSICIN_STATUS + i * 0xC);
		int bit;
		while ((bit = ffs(v) - 1) >= 0) {
			PCIE_WRITE(sc, PCIE_PL_MSICIN_STATUS + i * 0xC,
			    __BIT(bit));
			v &= ~__BIT(bit);
		}
	}

	mutex_enter(&sc->sc_lock);
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
imx6pcie_setup(struct imx6pcie_softc * const sc)
{
	uint32_t v;

	/* Setup RC */

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
	v &= ~(PCI_BRIDGE_BUS_EACH_MASK << PCI_BRIDGE_BUS_SUBORDINATE_SHIFT |
	    PCI_BRIDGE_BUS_EACH_MASK << PCI_BRIDGE_BUS_SECONDARY_SHIFT |
	    PCI_BRIDGE_BUS_EACH_MASK << PCI_BRIDGE_BUS_PRIMARY_SHIFT);
	v |= PCI_BRIDGE_BUS_SUBORDINATE(1);
	v |= PCI_BRIDGE_BUS_SECONDARY(1);
	v |= PCI_BRIDGE_BUS_PRIMARY(0);
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
		PCI_SUBCLASS_BRIDGE_PCI, PCI_INTERFACE_BRIDGE_PCI_PCI));

	PCIE_WRITE(sc, PCIE_PL_IATUVR, 0);

	PCIE_WRITE(sc, PCIE_PL_IATURLBA, IMX6_PCIE_ROOT_BASE);
	PCIE_WRITE(sc, PCIE_PL_IATURUBA, 0);
	PCIE_WRITE(sc, PCIE_PL_IATURLA, IMX6_PCIE_ROOT_BASE + IMX6_PCIE_ROOT_SIZE);

	PCIE_WRITE(sc, PCIE_PL_IATURLTA, 0);
	PCIE_WRITE(sc, PCIE_PL_IATURUTA, 0);
	PCIE_WRITE(sc, PCIE_PL_IATURC1, PCIE_PL_IATURC1_TYPE_CFG0);
	PCIE_WRITE(sc, PCIE_PL_IATURC2, PCIE_PL_IATURC2_REGION_ENABLE);
}

void
imx6pcie_init(pci_chipset_tag_t pc, void *priv)
{
	pc->pc_conf_v = priv;
	pc->pc_attach_hook = imx6pcie_attach_hook;
	pc->pc_bus_maxdevs = imx6pcie_bus_maxdevs;
	pc->pc_make_tag = imx6pcie_make_tag;
	pc->pc_decompose_tag = imx6pcie_decompose_tag;
	pc->pc_conf_read = imx6pcie_conf_read;
	pc->pc_conf_write = imx6pcie_conf_write;
#ifdef __HAVE_PCI_CONF_HOOK
	pc->pc_conf_hook = imx6pcie_conf_hook;
#endif
	pc->pc_conf_interrupt = imx6pcie_conf_interrupt;

	pc->pc_intr_v = priv;
	pc->pc_intr_map = imx6pcie_intr_map;
	pc->pc_intr_string = imx6pcie_intr_string;
	pc->pc_intr_evcnt = imx6pcie_intr_evcnt;
	pc->pc_intr_establish = imx6pcie_intr_establish;
	pc->pc_intr_disestablish = imx6pcie_intr_disestablish;
}

static void
imx6pcie_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
	/* nothing to do */
}

static int
imx6pcie_bus_maxdevs(void *v, int busno)
{
	return 32;
}

static pcitag_t
imx6pcie_make_tag(void *v, int b, int d, int f)
{
	return (b << 16) | (d << 11) | (f << 8);
}

static void
imx6pcie_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
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
imx6pcie_data_abort_handler(trapframe_t *tf)
{
	data_abort_flag = true;
	tf->tf_pc += 0x4;
	return;
}

static pcireg_t
imx6pcie_conf_read(void *v, pcitag_t tag, int offset)
{
	struct imx6pcie_softc *sc = v;
	bus_space_handle_t bsh;
	int b, d, f;
	pcireg_t ret = -1;
	int s;

	imx6pcie_decompose_tag(v, tag, &b, &d, &f);

	if ((unsigned int)offset >= PCI_EXTCONF_SIZE)
		return ret;
	if (!imx6pcie_valid_device(sc, b, d))
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
	data_abort_handler_address = (u_int)imx6pcie_data_abort_handler;
	data_abort_flag = false;

	ret = bus_space_read_4(sc->sc_iot, bsh, offset & ~0x3);

	data_abort_handler_address = saved;

	PCIE_CONF_UNLOCK(s);

	if (data_abort_flag)
		ret = -1;

	return ret;
}

static void
imx6pcie_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	struct imx6pcie_softc *sc = v;
	bus_space_handle_t bsh;
	int b, d, f;
	int s;

	imx6pcie_decompose_tag(v, tag, &b, &d, &f);

	if ((unsigned int)offset >= PCI_EXTCONF_SIZE)
		return;
	if (!imx6pcie_valid_device(sc, b, d))
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
	data_abort_handler_address = (u_int)imx6pcie_data_abort_handler;

	bus_space_write_4(sc->sc_iot, bsh, offset & ~0x3, val);

	data_abort_handler_address = saved;

	PCIE_CONF_UNLOCK(s);

	return;
}

#ifdef __HAVE_PCI_CONF_HOOK
static int
imx6pcie_conf_hook(void *v, int b, int d, int f, pcireg_t id)
{
	return PCI_CONF_DEFAULT;
}
#endif

static void
imx6pcie_conf_interrupt(void *v, int bus, int dev, int ipin, int swiz,
    int *ilinep)
{
	/* nothing to do */
}

static int
imx6pcie_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ih)
{
	if (pa->pa_intrpin == 0)
		return EINVAL;
	*ih = pa->pa_intrpin;
	return 0;
}

static const char *
imx6pcie_intr_string(void *v, pci_intr_handle_t ih, char *buf, size_t len)
{
	if (ih == PCI_INTERRUPT_PIN_NONE)
		return NULL;

	strlcpy(buf, "pci", len);

	return buf;
}

const struct evcnt *
imx6pcie_intr_evcnt(void *v, pci_intr_handle_t ih)
{
	return NULL;
}

static void *
imx6pcie_intr_establish(void *v, pci_intr_handle_t ih, int ipl,
    int (*callback)(void *), void *arg)
{
	struct imx6pcie_softc *sc = v;
	struct imx6pcie_ih *pcie_ih;

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
imx6pcie_intr_disestablish(void *v, void *vih)
{
	struct imx6pcie_softc *sc = v;
	struct imx6pcie_ih *pcie_ih = vih;

	mutex_enter(&sc->sc_lock);
	TAILQ_REMOVE(&sc->sc_intrs, pcie_ih, ih_entry);
	sc->sc_intrgen++;
	mutex_exit(&sc->sc_lock);

	kmem_free(pcie_ih, sizeof(*pcie_ih));
}
