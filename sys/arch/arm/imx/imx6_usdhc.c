/*	$NetBSD: imx6_usdhc.c,v 1.1 2014/09/25 05:05:28 ryo Exp $ */

/*-
 * Copyright (c) 2012  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx6_usdhc.c,v 1.1 2014/09/25 05:05:28 ryo Exp $");

#include "opt_imx.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/pmf.h>

#include <machine/intr.h>

#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6var.h>
#include <arm/imx/imx6_ccmvar.h>
#include <arm/imx/imx6_ccmreg.h>

struct sdhc_axi_softc {
	struct sdhc_softc  sc_sdhc;
	/* we have only one slot */
	struct sdhc_host *sc_hosts[1];

	void *sc_ih;
};

static int sdhc_match(device_t, cfdata_t, void *);
static void sdhc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sdhc_axi, sizeof(struct sdhc_axi_softc),
    sdhc_match, sdhc_attach, NULL, NULL);

static int
sdhc_match(device_t parent, cfdata_t cf, void *aux)
{

	struct axi_attach_args *aa = aux;

	switch (aa->aa_addr) {
	case IMX6_AIPS2_BASE + AIPS2_USDHC1_BASE:
	case IMX6_AIPS2_BASE + AIPS2_USDHC2_BASE:
	case IMX6_AIPS2_BASE + AIPS2_USDHC3_BASE:
	case IMX6_AIPS2_BASE + AIPS2_USDHC4_BASE:
		return 1;
	}

	return 0;
}

static void
sdhc_attach(device_t parent, device_t self, void *aux)
{
	struct sdhc_axi_softc *sc = device_private(self);
	struct axi_attach_args *aa = aux;
	bus_space_tag_t iot = aa->aa_iot;
	bus_space_handle_t ioh;
	u_int perclk = 0, v;

	sc->sc_sdhc.sc_dev = self;

	sc->sc_sdhc.sc_dmat = aa->aa_dmat;

	if (bus_space_map(iot, aa->aa_addr, AIPS2_USDHC_SIZE, 0, &ioh)) {
		aprint_error_dev(self, "can't map\n");
		return;
	}

	aprint_normal(": Ultra Secured Digial Host Controller\n");
	aprint_naive("\n");
	sc->sc_sdhc.sc_host = sc->sc_hosts;

	switch (aa->aa_addr) {
	case IMX6_AIPS2_BASE + AIPS2_USDHC1_BASE:
		v = imx6_ccm_read(CCM_CCGR6);
		imx6_ccm_write(CCM_CCGR6, v | CCM_CCGR6_USDHC1_CLK_ENABLE(3));
		perclk = imx6_get_clock(IMX6CLK_USDHC1_CLK_ROOT);
		break;
	case IMX6_AIPS2_BASE + AIPS2_USDHC2_BASE:
		v = imx6_ccm_read(CCM_CCGR6);
		imx6_ccm_write(CCM_CCGR6, v | CCM_CCGR6_USDHC2_CLK_ENABLE(3));
		perclk = imx6_get_clock(IMX6CLK_USDHC2_CLK_ROOT);
		break;
	case IMX6_AIPS2_BASE + AIPS2_USDHC3_BASE:
		v = imx6_ccm_read(CCM_CCGR6);
		imx6_ccm_write(CCM_CCGR6, v | CCM_CCGR6_USDHC3_CLK_ENABLE(3));
		perclk = imx6_get_clock(IMX6CLK_USDHC3_CLK_ROOT);
		break;
	case IMX6_AIPS2_BASE + AIPS2_USDHC4_BASE:
		v = imx6_ccm_read(CCM_CCGR6);
		imx6_ccm_write(CCM_CCGR6, v | CCM_CCGR6_USDHC4_CLK_ENABLE(3));
		perclk = imx6_get_clock(IMX6CLK_USDHC4_CLK_ROOT);
		break;
	}

	sc->sc_sdhc.sc_clkbase = perclk / 1000;
	sc->sc_sdhc.sc_flags |=
	    SDHC_FLAG_USDHC |
	    SDHC_FLAG_NO_PWR0 |
	    SDHC_FLAG_HAVE_DVS |
	    SDHC_FLAG_32BIT_ACCESS |
	    SDHC_FLAG_ENHANCED;

	sc->sc_ih = intr_establish(aa->aa_irq, IPL_SDMMC, IST_LEVEL,
	    sdhc_intr, &sc->sc_sdhc);

	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "can't establish interrupt\n");
		return;
	}

	if (sdhc_host_found(&sc->sc_sdhc, iot, ioh, AIPS2_USDHC_SIZE)) {
		aprint_error_dev(self, "can't initialize host\n");
		return;
	}

	if (!pmf_device_register1(self, sdhc_suspend, sdhc_resume,
		sdhc_shutdown)) {
		aprint_error_dev(self,
		    "can't establish power hook\n");
	}
}
