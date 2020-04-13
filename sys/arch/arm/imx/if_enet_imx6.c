/*	$NetBSD: if_enet_imx6.c,v 1.3.8.1 2020/04/13 08:03:34 martin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: if_enet_imx6.c,v 1.3.8.1 2020/04/13 08:03:34 martin Exp $");

#include "locators.h"
#include "imxccm.h"
#include "imxocotp.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <arm/imx/imx6var.h>
#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6_ccmreg.h>
#include <arm/imx/imx6_ccmvar.h>
#include <arm/imx/imx6_iomuxreg.h>
#include <arm/imx/imx6_ocotpreg.h>
#include <arm/imx/imx6_ocotpvar.h>
#include <arm/imx/if_enetreg.h>
#include <arm/imx/if_enetvar.h>

CFATTACH_DECL_NEW(enet, sizeof(struct enet_softc),
    enet_match, enet_attach, NULL, NULL);

static int enet_init_clocks(struct enet_softc *);

int
enet_match(device_t parent __unused, struct cfdata *match __unused, void *aux)
{
	struct axi_attach_args *aa;

	aa = aux;

	switch (aa->aa_addr) {
	case (IMX6_AIPS2_BASE + AIPS2_ENET_BASE):
		return 1;
	case (IMX6_AIPS1_BASE + AIPS1_ENET2_BASE):
		if (IMX6_CHIPID_MAJOR(imx6_chip_id()) == CHIPID_MAJOR_IMX6UL)
			return 1;
		break;
	}

	return 0;
}

void
enet_attach(device_t parent, device_t self, void *aux)
{
	struct enet_softc *sc = device_private(self);
	struct axi_attach_args *aa = aux;
#if NIMXOCOTP > 0
	uint32_t eaddr;
#endif

	if (aa->aa_size == AXICF_SIZE_DEFAULT)
		aa->aa_size = AIPS_ENET_SIZE;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->sc_dmat = aa->aa_dmat;

	sc->sc_imxtype = 6;	/* i.MX6 */
	sc->sc_phyid = MII_PHY_ANY;
	if (IMX6_CHIPID_MAJOR(imx6_chip_id()) == CHIPID_MAJOR_IMX6UL)
		sc->sc_rgmii = 0;
	else
		sc->sc_rgmii = 1;

	switch (aa->aa_addr) {
	case (IMX6_AIPS2_BASE + AIPS2_ENET_BASE):
		sc->sc_unit = 0;
		break;
	case (IMX6_AIPS1_BASE + AIPS1_ENET2_BASE):
		sc->sc_unit = 1;
		break;
	}

#if NIMXOCOTP > 0
	/* get mac-address from OCOTP */
	eaddr = imxocotp_read(OCOTP_MAC1);
	sc->sc_enaddr[0] = eaddr >> 8;
	sc->sc_enaddr[1] = eaddr;
	eaddr = imxocotp_read(OCOTP_MAC0);
	sc->sc_enaddr[2] = eaddr >> 24;
	sc->sc_enaddr[3] = eaddr >> 16;
	sc->sc_enaddr[4] = eaddr >> 8;
	sc->sc_enaddr[5] = eaddr + sc->sc_unit;
#endif

	if (IMX6_CHIPID_MAJOR(imx6_chip_id()) == CHIPID_MAJOR_IMX6UL) {
		uint32_t v;

		v = iomux_read(IMX6UL_IOMUX_GPR1);
		switch (sc->sc_unit) {
		case 0:
			v |= IMX6UL_IOMUX_GPR1_ENET1_TX_CLK_DIR;
			v &= ~IMX6UL_IOMUX_GPR1_ENET1_CLK_SEL;
			break;
		case 1:
			v |= IMX6UL_IOMUX_GPR1_ENET2_TX_CLK_DIR;
			v &= ~IMX6UL_IOMUX_GPR1_ENET2_CLK_SEL;
			break;
		}
		iomux_write(IMX6UL_IOMUX_GPR1, v);
	}

	sc->sc_clk_ipg = imx6_get_clock("enet");
	if (sc->sc_clk_ipg == NULL) {
		aprint_error(": couldn't get clock ipg\n");
		return;
	}
	sc->sc_clk_enet = imx6_get_clock("enet");
	if (sc->sc_clk_enet == NULL) {
		aprint_error(": couldn't get clock enet\n");
		return;
	}
	sc->sc_clk_enet_ref = imx6_get_clock("enet_ref");
	if (sc->sc_clk_enet_ref == NULL) {
		aprint_error(": couldn't get clock enet_ref\n");
		return;
	}
	if (enet_init_clocks(sc) != 0) {
		aprint_error_dev(self, "couldn't init clocks\n");
		return;
	}

	sc->sc_clock = clk_get_rate(sc->sc_clk_ipg);

	if (bus_space_map(sc->sc_iot, aa->aa_addr, aa->aa_size, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(self, "cannot map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Gigabit Ethernet Controller\n");

	/* setup interrupt handlers */
	if ((sc->sc_ih = intr_establish(aa->aa_irq, IPL_NET,
	    IST_LEVEL, enet_intr, sc)) == NULL) {
		aprint_error_dev(self, "unable to establish interrupt\n");
		goto failure;
	}

	if (enet_attach_common(self) != 0)
		goto failure;

	return;

failure:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, aa->aa_size);
	return;
}

static int
enet_init_clocks(struct enet_softc *sc)
{
	int error;

	error = clk_enable(sc->sc_clk_ipg);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable ipg: %d\n", error);
		return error;
	}
	error = clk_enable(sc->sc_clk_enet);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable enet: %d\n", error);
		return error;
	}
	error = clk_enable(sc->sc_clk_enet_ref);
	if (error) {
		aprint_error_dev(sc->sc_dev, "couldn't enable enet-ref: %d\n", error);
		return error;
	}

	return 0;
}
