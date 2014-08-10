/*	$NetBSD: imx51_pwm.c,v 1.1.6.2 2014/08/10 06:53:51 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: imx51_pwm.c,v 1.1.6.2 2014/08/10 06:53:51 tls Exp $");

#include "locators.h"
#include "opt_imx.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <arm/imx/imxpwmvar.h>
#include <arm/imx/imx51reg.h>
#include <arm/imx/imx51var.h>
#include <arm/imx/imx51_ccmvar.h>

int
imxpwm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct axi_attach_args *aa = aux;

	switch (aa->aa_addr) {
	case PWM1_BASE:
	case PWM2_BASE:
		return 1;
	}

	return 0;
}

void
imxpwm_attach(struct imxpwm_softc *sc, void *aux)
{
	struct axi_attach_args *aa = aux;

	if (aa->aa_size == AXICF_SIZE_DEFAULT)
		aa->aa_size = PWM_SIZE;
	sc->sc_iot = aa->aa_iot;
	sc->sc_intr = aa->aa_irq;
	sc->sc_freq = imx51_get_clock(IMX51CLK_IPG_CLK_ROOT);
	if (bus_space_map(aa->aa_iot, aa->aa_addr, aa->aa_size, 0, &sc->sc_ioh))
		panic("%s: couldn't map", device_xname(sc->sc_dev));
	imxpwm_attach_common(sc);
}
