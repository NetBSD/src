/*	$NetBSD: imx6_usb.c,v 1.2.4.2 2018/06/25 07:25:39 pgoyette Exp $	*/

/*
 * Copyright (c) 2012  Genetec Corporation.  All rights reserved.
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx6_usb.c,v 1.2.4.2 2018/06/25 07:25:39 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>

#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6var.h>
#include <arm/imx/imx6_ccmreg.h>
#include <arm/imx/imx6_ccmvar.h>
#include <arm/imx/imxusbvar.h>

#include "locators.h"

static int imxusbc_search(device_t, cfdata_t, const int *, void *);
static int imxusbc_print(void *, const char *);

int
imxusbc_attach_common(device_t parent, device_t self, bus_space_tag_t iot)
{
	struct imxusbc_softc *sc;
	uint32_t v;

	sc = device_private(self);
	sc->sc_iot = iot;

	/* Map entire USBOH registers.  Host controller drivers
	 * re-use subregions of this. */
	if (bus_space_map(iot, IMX6_AIPS2_BASE + AIPS2_USBOH_BASE, /* XXX */
	    AIPS2_USBOH_SIZE, 0, &sc->sc_ioh))
		return -1;

	/* USBOH3 clock enable */
	v = imx6_ccm_read(CCM_CCGR6);
	imx6_ccm_write(CCM_CCGR6, v | __SHIFTIN(3, CCM_CCGR6_USBOH3_CLK_ENABLE));

	/* attach OTG/EHCI host controllers */
	config_search_ia(imxusbc_search, self, "imxusbc", NULL);

	return 0;
}

static int
imxusbc_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct imxusbc_softc *sc;
	struct imxusbc_attach_args aa;

	sc = device_private(parent);
	aa.aa_iot = sc->sc_iot;
	aa.aa_ioh = sc->sc_ioh;
	aa.aa_dmat = &arm_generic_dma_tag;
	aa.aa_unit = cf->cf_loc[IMXUSBCCF_UNIT];
	aa.aa_irq = cf->cf_loc[IMXUSBCCF_IRQ];

	if (config_match(parent, cf, &aa) > 0)
		config_attach(parent, cf, &aa, imxusbc_print);

	return 0;
}

/* ARGSUSED */
static int
imxusbc_print(void *aux, const char *name __unused)
{
	struct imxusbc_attach_args *iaa;

	iaa = (struct imxusbc_attach_args *)aux;

	aprint_normal(" unit %d intr %d", iaa->aa_unit, iaa->aa_irq);
	return UNCONF;
}
