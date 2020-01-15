/*	$NetBSD: netwalker_spi.c,v 1.4 2020/01/15 10:25:47 skrll Exp $	*/

/*-
 * Copyright (c) 2009  Genetec Corporation.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: netwalker_spi.c,v 1.4 2020/01/15 10:25:47 skrll Exp $");

#include "opt_imxspi.h"

#define	_INTR_PRIVATE

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/gpio.h>

#include <arm/imx/imx51reg.h>
#include <arm/imx/imx51var.h>
#include <arm/imx/imx51_ccmvar.h>
#include <arm/imx/imx51_ccmreg.h>
#include <arm/imx/imx51_iomuxreg.h>
#include <arm/imx/imxgpiovar.h>
#include <arm/imx/imxspivar.h>
#include <arm/imx/imxspireg.h>

struct imx51spi_softc {
	struct imxspi_softc	sc_spi; /* Must be first */

	struct spi_chipset_tag	sc_tag;
};

CFATTACH_DECL_NEW(spi_netwalker, sizeof(struct imx51spi_softc),
    imxspi_match, imxspi_attach, NULL, NULL);

static int
imxspi_cs_enable(void *arg, int slave)
{
	switch (slave) {
	case 0:
		imxgpio_data_write(GPIO_NO(4, 24), GPIO_PIN_LOW);
		imxgpio_set_direction(GPIO_NO(4, 24), GPIO_PIN_OUTPUT);
		break;
	case 1:
		imxgpio_data_write(GPIO_NO(4, 25), GPIO_PIN_LOW);
		imxgpio_set_direction(GPIO_NO(4, 25), GPIO_PIN_OUTPUT);
		break;
	case 2:
		imxgpio_data_write(GPIO_NO(3, 0), GPIO_PIN_LOW);
		imxgpio_set_direction(GPIO_NO(3, 0), GPIO_PIN_OUTPUT);
		break;
	}

	return 0;
}

static int
imxspi_cs_disable(void *arg, int slave)
{
	switch (slave) {
	case 0:
		imxgpio_data_write(GPIO_NO(4, 24), GPIO_PIN_HIGH);
		imxgpio_set_direction(GPIO_NO(4, 24), GPIO_PIN_INPUT);
		break;
	case 1:
		imxgpio_data_write(GPIO_NO(4, 25), GPIO_PIN_HIGH);
		imxgpio_set_direction(GPIO_NO(4, 25), GPIO_PIN_INPUT);
		break;
	case 2:
		imxgpio_data_write(GPIO_NO(3, 0), GPIO_PIN_HIGH);
		imxgpio_set_direction(GPIO_NO(3, 0), GPIO_PIN_INPUT);
		break;
	}

	return 0;
}

int
imxspi_match(device_t parent, cfdata_t cf, void *aux)
{
	if (strcmp(cf->cf_name, "imxspi") == 0)
		return 1;
	if (cf->cf_unit != 0)
		return 1;

	return 0;
}

void
imxspi_attach(device_t parent, device_t self, void *aux)
{
	struct imx51spi_softc *isc = device_private(self);
	struct imxspi_softc *sc = &isc->sc_spi;
	struct axi_attach_args *aa = aux;
	int cf_flags = device_cfdata(self)->cf_flags;
	bus_addr_t addr;
	bus_size_t size;

	addr = aa->aa_addr;
	size = aa->aa_size;
	if (size <= 0)
		size = SPI_SIZE;

	isc->sc_tag.cookie = sc;

	switch (device_unit(self)) {
	case 0:
		/* CS 0 GPIO setting */
		imxgpio_data_write(GPIO_NO(4, 24), GPIO_PIN_HIGH);
		imxgpio_set_direction(GPIO_NO(4, 24), GPIO_PIN_INPUT);

		/* CS 1 GPIO setting */
		imxgpio_data_write(GPIO_NO(4, 25), GPIO_PIN_HIGH);
		imxgpio_set_direction(GPIO_NO(4, 25), GPIO_PIN_INPUT);

		/* CS 2 */
		/* OJ6SH-T25 Shutdown */
		imxgpio_data_write(GPIO_NO(3, 14), GPIO_PIN_LOW);
		imxgpio_set_direction(GPIO_NO(3, 14), GPIO_PIN_OUTPUT);

		/* CS 2 GPIO setting */
		imxgpio_data_write(GPIO_NO(3, 0), GPIO_PIN_HIGH);
		imxgpio_set_direction(GPIO_NO(3, 0), GPIO_PIN_INPUT);

		isc->sc_tag.spi_cs_enable = imxspi_cs_enable;
		isc->sc_tag.spi_cs_disable = imxspi_cs_disable;
		break;
	}

	sc->sc_iot = aa->aa_iot;
	sc->sc_enhanced = cf_flags;

	sc->sc_nslaves = IMXSPINSLAVES;
	sc->sc_freq = imx51_get_clock(IMX51CLK_CSPI_CLK_ROOT);
	sc->sc_tag = &isc->sc_tag;

	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev, "couldn't map registers\n");
		return;
	}

	/* enable device interrupts */
	sc->sc_ih = intr_establish(aa->aa_irq, IPL_BIO, IST_LEVEL,
	    imxspi_intr, sc);

	imxspi_attach_common(self);
}
