/*	$NetBSD: imx6_i2c.c,v 1.2.16.2 2017/12/03 11:35:53 jdolecek Exp $	*/

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

/*
 * i.MX6 I2C Controller
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx6_i2c.c,v 1.2.16.2 2017/12/03 11:35:53 jdolecek Exp $");

#include "opt_imx.h"

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/param.h>

#include <arm/imx/imx6var.h>
#include <arm/imx/imx6_reg.h>
#include <arm/imx/imx6_ccmvar.h>
#include <arm/imx/imxi2cvar.h>

/* ARGSUSED */
int
imxi2c_match(device_t parent __unused, struct cfdata *match __unused, void *aux)
{
	struct axi_attach_args *aa = aux;

	switch (aa->aa_addr) {
	case IMX6_AIPS2_BASE + AIPS2_I2C1_BASE:
	case IMX6_AIPS2_BASE + AIPS2_I2C2_BASE:
	case IMX6_AIPS2_BASE + AIPS2_I2C3_BASE:
		return 1;
	}

	return 0;
}

/* ARGSUSED */
void
imxi2c_attach(device_t parent __unused, device_t self, void *aux)
{
	struct axi_attach_args *aa = aux;

	if (aa->aa_size <= 0)
		aa->aa_size = I2C_SIZE;

	imxi2c_set_freq(self, imx6_get_clock(IMX6CLK_PERCLK), 400000);
	imxi2c_attach_common(parent, self,
	    aa->aa_iot, aa->aa_addr, aa->aa_size, aa->aa_irq, 0);
}

