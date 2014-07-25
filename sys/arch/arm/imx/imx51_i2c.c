/*	$NetBSD: imx51_i2c.c,v 1.1 2014/07/25 07:07:47 hkenken Exp $	*/

/*
 * Copyright (c) 2012 Genetec Corporation.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: imx51_i2c.c,v 1.1 2014/07/25 07:07:47 hkenken Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include "opt_imx.h"

#include <arm/imx/imxi2cvar.h>
#include <arm/imx/imx51reg.h>
#include <arm/imx/imx51var.h>
#include <arm/imx/imx51_ccmvar.h>

int
imxi2c_match(device_t parent, cfdata_t cf, void *aux)
{
	if (strcmp(cf->cf_name, "imxi2c") == 0)
		return 1;

	return 0;
}

void
imxi2c_attach(device_t parent, device_t self, void *aux)
{
	struct axi_attach_args * aa = aux;
	struct imxi2c_softc *sc = device_private(self);
	struct i2cbus_attach_args iba;

	imxi2c_attach_common(parent, self,
	    aa->aa_iot, aa->aa_addr, aa->aa_size, aa->aa_irq, 0);

	imxi2c_set_freq(self, imx51_get_clock(IMX51CLK_PERCLK_ROOT), 400000);

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c;
	config_found_ia(sc->sc_dev, "i2cbus", &iba, iicbus_print);
}
