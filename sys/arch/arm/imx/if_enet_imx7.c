/*	$NetBSD: if_enet_imx7.c,v 1.2.4.2 2017/12/03 11:35:53 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: if_enet_imx7.c,v 1.2.4.2 2017/12/03 11:35:53 jdolecek Exp $");

#include "locators.h"
#include "imxccm.h"
#include "imxocotp.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <arm/imx/imx7var.h>
#include <arm/imx/imx7reg.h>
#include <arm/imx/imx7_ccmreg.h>
#include <arm/imx/imx7_ccmvar.h>
#include <arm/imx/imx7_ocotpreg.h>
#include <arm/imx/imx7_ocotpvar.h>
#include <arm/imx/if_enetreg.h>
#include <arm/imx/if_enetvar.h>

static void get_mac_from_ocotp(struct enet_softc *, device_t self,
    const char *);

int
enet_match(device_t parent __unused, struct cfdata *match __unused, void *aux)
{
	struct axi_attach_args *aa;

	aa = aux;

	switch (aa->aa_addr) {
	case (IMX7_AIPS_BASE + AIPS3_ENET1_BASE):
		return 1;
	case (IMX7_AIPS_BASE + AIPS3_ENET2_BASE):
		return 1;
	}

	return 0;
}

void
enet_attach(device_t parent, device_t self, void *aux)
{
	struct enet_softc *sc;
	struct axi_attach_args *aa;

	aa = aux;
	sc = device_private(self);

	if (aa->aa_size == AXICF_SIZE_DEFAULT)
		aa->aa_size = AIPS_ENET_SIZE;

	sc->sc_imxtype = 7;	/* i.MX7 */
	sc->sc_rgmii = 1;

#if NIMXCCM > 0
	/* PLL power up */
	if (imx7_pll_power(CCM_ANALOG_PLL_ENET, 1) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't enable CCM_ANALOG_PLL_ENET\n");
		return;
	}
	sc->sc_pllclock = imx7_get_clock(IMX7CLK_ENET_PLL);
#else
	sc->sc_pllclock = 1000000000;
#endif

	switch (aa->aa_addr) {
	case (IMX7_AIPS_BASE + AIPS3_ENET1_BASE):
		sc->sc_unit = 0;
		get_mac_from_ocotp(sc, self, "enet1-ocotp-mac");
		break;

	case (IMX7_AIPS_BASE + AIPS3_ENET2_BASE):
		sc->sc_unit = 1;
		get_mac_from_ocotp(sc, self, "enet2-ocotp-mac");
		break;
	}

	enet_attach_common(self, aa->aa_iot, aa->aa_dmat, aa->aa_addr,
	    aa->aa_size, aa->aa_irq);
}

static void
get_mac_from_ocotp(struct enet_softc *sc, device_t self, const char *key)
{
#if NIMXOCOTP > 0
	char ocotp_mac[12];
	prop_dictionary_t dict;
	const char *ocotp_mac_layout;
	uint32_t eaddr;
	int i;

#define HTOI(c)	\
	((((c) >= '0') && ((c) <= '9')) ? (c) - '0' :	\
	(((c) >= 'a') && ((c) <= 'f')) ? (c) - 'a' + 10: -1)

	/*
	 * some board has illegal layout of macaddr in OCOTP.
	 * so you can configure sequence of macaddr by device property.
	 *
	 *  OCOTP[MAC_ADDR0]: [0][1][2][3]
	 *  OCOTP[MAC_ADDR1]: [4][5][6][7]
	 *  OCOTP[MAC_ADDR2]: [8][9][a][b]
	 *
	 *  default layout, ENET1 macaddr is [6,7,0,1,2,3]
	 *                  ENET2 macaddr is [8,9,a,b,4,5]
	 */
	dict = device_properties(self);
	if (!prop_dictionary_get_cstring_nocopy(dict, key, &ocotp_mac_layout)) {
		if (sc->sc_unit == 0)
			ocotp_mac_layout = "670123";
		else if (sc->sc_unit == 1)
			ocotp_mac_layout = "89ab45";
		else
			return;
	}

	eaddr = imxocotp_read(OCOTP_MAC_ADDR0);
	ocotp_mac[0] = eaddr >> 24;
	ocotp_mac[1] = eaddr >> 16;
	ocotp_mac[2] = eaddr >> 8;
	ocotp_mac[3] = eaddr;
	eaddr = imxocotp_read(OCOTP_MAC_ADDR1);
	ocotp_mac[4] = eaddr >> 24;
	ocotp_mac[5] = eaddr >> 16;
	ocotp_mac[6] = eaddr >> 8;
	ocotp_mac[7] = eaddr;
	eaddr = imxocotp_read(OCOTP_MAC_ADDR2);
	ocotp_mac[8] = eaddr >> 24;
	ocotp_mac[9] = eaddr >> 16;
	ocotp_mac[10] = eaddr >> 8;
	ocotp_mac[11] = eaddr;

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		int offs;
		offs = HTOI(ocotp_mac_layout[i]);
		KASSERT((offs >= 0) && (offs < 12));
		sc->sc_enaddr[i] = ocotp_mac[offs];
	}
#endif
}
