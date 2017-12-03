/*	$NetBSD: imx6_ahcisata.c,v 1.6.2.2 2017/12/03 11:35:53 jdolecek Exp $	*/

/*
 * Copyright (c) 2014 Ryo Shimizu <ryo@nerv.org>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx6_ahcisata.c,v 1.6.2.2 2017/12/03 11:35:53 jdolecek Exp $");

#include "locators.h"
#include "opt_imx.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6var.h>
#include <arm/imx/imx6_ahcisatareg.h>
#include <arm/imx/imx6_iomuxreg.h>
#include <arm/imx/imx6_ccmreg.h>
#include <arm/imx/imx6_ccmvar.h>

#include <dev/ata/atavar.h>
#include <dev/ic/ahcisatavar.h>

struct imx_ahci_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	void *sc_ih;

	struct ahci_softc sc_ahcisc;
};

static int imx6_ahcisata_match(device_t, cfdata_t, void *);
static void imx6_ahcisata_attach(device_t, device_t, void *);
static int imx6_ahcisata_detach(device_t, int);

static int ixm6_ahcisata_init(struct imx_ahci_softc *);
static int imx6_ahcisata_phy_ctrl(struct imx_ahci_softc *, uint32_t, int);
static int imx6_ahcisata_phy_addr(struct imx_ahci_softc *, uint32_t);
static int imx6_ahcisata_phy_write(struct imx_ahci_softc *, uint32_t, uint16_t);
static int imx6_ahcisata_phy_read(struct imx_ahci_softc *, uint32_t);

CFATTACH_DECL_NEW(imx6_ahcisata, sizeof(struct imx_ahci_softc),
    imx6_ahcisata_match, imx6_ahcisata_attach, imx6_ahcisata_detach, NULL);

static int
imx6_ahcisata_match(device_t parent, cfdata_t match, void *aux)
{
	struct axi_attach_args * const aa = aux;

	if (aa->aa_addr != IMX6_SATA_BASE)
		return 0;

	/* i.MX6 Solo/SoloLite/DualLite has no SATA interface */
	switch (IMX6_CHIPID_MAJOR(imx6_chip_id())) {
	case CHIPID_MAJOR_IMX6SL:
	case CHIPID_MAJOR_IMX6DL:
	case CHIPID_MAJOR_IMX6SOLO:
	case CHIPID_MAJOR_IMX6UL:
		return 0;
	default:
		break;
	}

	return 1;
}

static void
imx6_ahcisata_attach(device_t parent, device_t self, void *aux)
{
	struct imx_ahci_softc *sc;
	struct ahci_softc *ahci_sc;
	struct axi_attach_args *aa;

	aa = aux;
	sc = device_private(self);
	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;

	if (aa->aa_size == AXICF_SIZE_DEFAULT)
		aa->aa_size = IMX6_SATA_SIZE;

	aprint_naive("\n");
	aprint_normal(": AHCI Controller\n");

	if (bus_space_map(aa->aa_iot, aa->aa_addr, aa->aa_size, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(self, "cannot map registers\n");
		return;
	}

	if (ixm6_ahcisata_init(sc) != 0) {
		aprint_error_dev(self, "couldn't init ahci\n");
		return;
	}

	ahci_sc = &sc->sc_ahcisc;
	ahci_sc->sc_atac.atac_dev = sc->sc_dev;
	ahci_sc->sc_ahci_ports = 1;
	ahci_sc->sc_dmat = aa->aa_dmat;
	ahci_sc->sc_ahcis = aa->aa_size;
	ahci_sc->sc_ahcit = sc->sc_iot;
	ahci_sc->sc_ahcih = sc->sc_ioh;

	sc->sc_ih = intr_establish(aa->aa_irq, IPL_BIO, IST_LEVEL,
	    ahci_intr, ahci_sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "unable to establish interrupt\n");
		return;
	}

	ahci_attach(ahci_sc);
}

static int
imx6_ahcisata_detach(device_t self, int flags)
{
	struct imx_ahci_softc *sc;
	struct ahci_softc *ahci_sc;
	int rv;

	sc = device_private(self);
	ahci_sc = &sc->sc_ahcisc;

	rv = ahci_detach(ahci_sc, flags);
	if (rv)
		return rv;

	if (sc->sc_ih) {
		intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}

	if (ahci_sc->sc_ahcis) {
		bus_space_unmap(ahci_sc->sc_ahcit, ahci_sc->sc_ahcih,
		    ahci_sc->sc_ahcis);
		ahci_sc->sc_ahcis = 0;
		ahci_sc->sc_ahcit = 0;
		ahci_sc->sc_ahcih = 0;
	}

	return 0;
}

static int
imx6_ahcisata_phy_ctrl(struct imx_ahci_softc *sc, uint32_t bitmask, int on)
{
	uint32_t v;
	int timeout;

	v = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SATA_P0PHYCR);
	if (on)
		v |= bitmask;
	else
		v &= ~bitmask;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_P0PHYCR, v);

	for (timeout = 5000; timeout > 0; --timeout) {
		v = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SATA_P0PHYSR);
		if (!!(v & SATA_P0PHYSR_CR_ACK) == !!on)
			break;
		delay(100);
	}

	if (timeout > 0)
		return 0;

	return -1;
}

static int
imx6_ahcisata_phy_addr(struct imx_ahci_softc *sc, uint32_t addr)
{
	delay(100);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_P0PHYCR, addr);

	if (imx6_ahcisata_phy_ctrl(sc, SATA_P0PHYCR_CR_CAP_ADDR, 1) != 0)
		return -1;
	if (imx6_ahcisata_phy_ctrl(sc, SATA_P0PHYCR_CR_CAP_ADDR, 0) != 0)
		return -1;

	return 0;
}

static int
imx6_ahcisata_phy_write(struct imx_ahci_softc *sc, uint32_t addr,
                        uint16_t data)
{
	if (imx6_ahcisata_phy_addr(sc, addr) != 0)
		return -1;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_P0PHYCR, data);

	if (imx6_ahcisata_phy_ctrl(sc, SATA_P0PHYCR_CR_CAP_DATA, 1) != 0)
		return -1;
	if (imx6_ahcisata_phy_ctrl(sc, SATA_P0PHYCR_CR_CAP_DATA, 0) != 0)
		return -1;

	if ((addr == SATA_PHY_CLOCK_RESET) && data) {
		/* we can't check ACK after RESET */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_P0PHYCR,
		    data | SATA_P0PHYCR_CR_WRITE);
		return 0;
	}

	if (imx6_ahcisata_phy_ctrl(sc, SATA_P0PHYCR_CR_WRITE, 1) != 0)
		return -1;
	if (imx6_ahcisata_phy_ctrl(sc, SATA_P0PHYCR_CR_WRITE, 0) != 0)
		return -1;

	return 0;
}

static int
imx6_ahcisata_phy_read(struct imx_ahci_softc *sc, uint32_t addr)
{
	uint32_t v;

	if (imx6_ahcisata_phy_addr(sc, addr) != 0)
		return -1;

	if (imx6_ahcisata_phy_ctrl(sc, SATA_P0PHYCR_CR_READ, 1) != 0)
		return -1;

	v = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SATA_P0PHYSR);

	if (imx6_ahcisata_phy_ctrl(sc, SATA_P0PHYCR_CR_READ, 0) != 0)
		return -1;

	return SATA_P0PHYSR_CR_DATA_OUT(v);
}

static int
ixm6_ahcisata_init(struct imx_ahci_softc *sc)
{
	uint32_t v;
	int timeout, pllstat;

	/* AHCISATA clock enable */
	v = imx6_ccm_read(CCM_CCGR5);
	imx6_ccm_write(CCM_CCGR5, v | CCM_CCGR5_100M_CLK_ENABLE(3));

	/* PLL power up */
	if (imx6_pll_power(CCM_ANALOG_PLL_ENET, 1,
		CCM_ANALOG_PLL_ENET_ENABLE_100M) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't enable CCM_ANALOG_PLL_ENET\n");
		return -1;
	}
	v = imx6_ccm_analog_read(CCM_ANALOG_PLL_ENET);
	v |= CCM_ANALOG_PLL_ENET_ENABLE_100M;
	imx6_ccm_analog_write(CCM_ANALOG_PLL_ENET, v);

	v = iomux_read(IOMUX_GPR13);
	/* clear */
	v &= ~(IOMUX_GPR13_SATA_PHY_8(7) |
	    IOMUX_GPR13_SATA_PHY_7(0x1f) |
	    IOMUX_GPR13_SATA_PHY_6(7) |
	    IOMUX_GPR13_SATA_SPEED(1) |
	    IOMUX_GPR13_SATA_PHY_5(1) |
	    IOMUX_GPR13_SATA_PHY_4(7) |
	    IOMUX_GPR13_SATA_PHY_3(0xf) |
	    IOMUX_GPR13_SATA_PHY_2(0x1f) |
	    IOMUX_GPR13_SATA_PHY_1(1) |
	    IOMUX_GPR13_SATA_PHY_0(1));
	/* setting */
	v |= IOMUX_GPR13_SATA_PHY_8(5) |	/* Rx 3.0db */
	    IOMUX_GPR13_SATA_PHY_7(0x12) |	/* Rx SATA2m */
	    IOMUX_GPR13_SATA_PHY_6(3) |		/* Rx DPLL mode */
	    IOMUX_GPR13_SATA_SPEED(1) |		/* 3.0GHz */
	    IOMUX_GPR13_SATA_PHY_5(0) |		/* SpreadSpectram */
	    IOMUX_GPR13_SATA_PHY_4(4) |		/* Tx Attenuation 9/16 */
	    IOMUX_GPR13_SATA_PHY_3(0) |		/* Tx Boost 0db */
	    IOMUX_GPR13_SATA_PHY_2(0x11) |	/* Tx Level 1.104V */
	    IOMUX_GPR13_SATA_PHY_1(1);		/* PLL clock enable */
	iomux_write(IOMUX_GPR13, v);

	/* phy reset */
	if (imx6_ahcisata_phy_write(sc, SATA_PHY_CLOCK_RESET,
	    SATA_PHY_CLOCK_RESET_RST) < 0) {
		aprint_error_dev(sc->sc_dev, "cannot reset PHY\n");
		return -1;
	}

	for (timeout = 50; timeout > 0; --timeout) {
		delay(100);
		pllstat = imx6_ahcisata_phy_read(sc, SATA_PHY_LANE0_OUT_STAT);
		if (pllstat < 0) {
			aprint_error_dev(sc->sc_dev,
			    "cannot read LANE0 status\n");
			break;
		}
		if (pllstat & SATA_PHY_LANE0_OUT_STAT_RX_PLL_STATE)
			break;
	}
	if (timeout <= 0)
		return -1;

	/* Support Staggered Spin-up */
	v = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SATA_CAP);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_CAP, v | SATA_CAP_SSS);

	/* Ports Implmented. must set 1 */
	v = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SATA_PI);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_PI, v | SATA_PI_PI);

	/* set 1ms-timer = AHB clock / 1000 */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SATA_TIMER1MS,
	    imx6_get_clock(IMX6CLK_AHB) / 1000);

	return 0;
}
