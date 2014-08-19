/*	$NetBSD: imx51_spi.c,v 1.1.10.2 2014/08/20 00:02:46 tls Exp $	*/

/*-
 * Copyright (c) 2014  Genetec Corporation.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx51_spi.c,v 1.1.10.2 2014/08/20 00:02:46 tls Exp $");

#include "locators.h"
#include "opt_imx.h"
#include "opt_imxspi.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <arm/imx/imxspivar.h>
#include <arm/imx/imx51reg.h>
#include <arm/imx/imx51var.h>
#include <arm/imx/imx51_ccmvar.h>

struct imx51spi_softc {
	struct imxspi_softc sc_spi;
	struct spi_chipset_tag sc_tag;
};

CFATTACH_DECL_NEW(imx51_spi, sizeof(struct imx51spi_softc),
    imxspi_match, imxspi_attach, NULL, NULL);

static int
imxspi_cs_enable(void *arg, int slave)
{
	return 0;
}

static int
imxspi_cs_disable(void *arg, int slave)
{
	return 0;
}

int
imxspi_match(device_t parent, cfdata_t cf, void *aux)
{
	if (strcmp(cf->cf_name, "imxspi") == 0)
		return 1;

	return 0;
}

void
imxspi_attach(device_t parent, device_t self, void *aux)
{
	struct imx51spi_softc *sc = device_private(self);
	struct axi_attach_args *aa = aux;
	struct imxspi_attach_args saa;
	int cf_flags = device_cfdata(self)->cf_flags;

	sc->sc_tag.cookie = sc;
	sc->sc_tag.spi_cs_enable = imxspi_cs_enable;
	sc->sc_tag.spi_cs_disable = imxspi_cs_disable;

	saa.saa_iot = aa->aa_iot;
	saa.saa_addr = aa->aa_addr;
	saa.saa_size = aa->aa_size;
	saa.saa_irq = aa->aa_irq;
	saa.saa_enhanced = cf_flags;

	saa.saa_nslaves = IMXSPINSLAVES;
	saa.saa_freq = imx51_get_clock(IMX51CLK_CSPI_CLK_ROOT);
	saa.saa_tag = &sc->sc_tag;

	sc->sc_spi.sc_dev = self;

	imxspi_attach_common(parent, &sc->sc_spi, &saa);
}
