/*	$NetBSD: imx7_ocotp.c,v 1.1.18.2 2017/12/03 11:35:53 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: imx7_ocotp.c,v 1.1.18.2 2017/12/03 11:35:53 jdolecek Exp $");

#include "locators.h"

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/param.h>

#include <arm/imx/imx7var.h>
#include <arm/imx/imx7reg.h>
#include <arm/imx/imx7_ocotpreg.h>
#include <arm/imx/imx7_ocotpvar.h>

struct imxocotp_softc {
	device_t sc_dev;

	bus_addr_t sc_addr;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

static struct imxocotp_softc *ocotp_softc;

static int imxocotp_match(device_t, struct cfdata *, void *);
static void imxocotp_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(imxocotp, sizeof(struct imxocotp_softc),
    imxocotp_match, imxocotp_attach, NULL, NULL);

/* ARGSUSED */
static int
imxocotp_match(device_t parent __unused, struct cfdata *match __unused,
               void *aux)
{
	struct axi_attach_args *aa;

	aa = aux;

	if (ocotp_softc != NULL)
		return 0;

	switch (aa->aa_addr) {
	case (IMX7_AIPS_BASE + AIPS1_OCOTP_CTRL_BASE):
		return 1;
	}

	return 0;
}

/* ARGSUSED */
static void
imxocotp_attach(device_t parent __unused, device_t self, void *aux)
{
	struct imxocotp_softc *sc;
	struct axi_attach_args *aa;
	uint32_t v;

	aa = aux;
	sc = device_private(self);
	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->sc_addr = aa->aa_addr;

	if (aa->aa_size == AXICF_SIZE_DEFAULT)
		aa->aa_size = AIPS2_OCOTP_CTRL_SIZE;

	aprint_naive("\n");
	aprint_normal(": On-Chip OTP Controller\n");
	if (bus_space_map(sc->sc_iot, sc->sc_addr, aa->aa_size, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(self, "Cannot map registers\n");
		return;
	}
	ocotp_softc = sc;

	v = imxocotp_read(OCOTP_VERSION);
	aprint_normal_dev(self, "OCOTP_VERSION %d.%d.%d\n",
	    (v >> 24) & 0xff, (v >> 16) & 0xff, v & 0xffff);

	return;
}

uint32_t
imxocotp_read(uint32_t addr)
{
	struct imxocotp_softc *sc;

	if ((sc = ocotp_softc) == NULL)
		return 0;

	if (addr > AIPS2_OCOTP_CTRL_SIZE)
		return 0;

	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, addr);
}
