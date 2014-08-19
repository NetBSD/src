/*	$NetBSD: imxpwm.c,v 1.1.10.2 2014/08/20 00:02:46 tls Exp $	*/

/*
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
__KERNEL_RCSID(0, "$NetBSD: imxpwm.c,v 1.1.10.2 2014/08/20 00:02:46 tls Exp $");

#include "opt_imx.h"
#include "locators.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <arm/imx/imxpwmreg.h>
#include <arm/imx/imxpwmvar.h>

static inline uint32_t
imxpwm_read(struct imxpwm_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, o);
}

static inline void
imxpwm_write(struct imxpwm_softc *sc, bus_size_t o, uint32_t v)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, o, v);
}

static int
imxpwm_intr(void *arg)
{
	struct imxpwm_softc *sc = arg;
	uint32_t sts = imxpwm_read(sc, PWM_SR);

	imxpwm_write(sc, PWM_SR, sts);

	if ((sts & SR_ROV) && (sc->sc_handler != NULL))
		sc->sc_handler(sc->sc_cookie);

	return 1;
}

void
imxpwm_attach_common(struct imxpwm_softc *sc)
{
	uint32_t reg;
	int div;

	if (sc->sc_handler != NULL) {
		sc->sc_ih = intr_establish(sc->sc_intr, IPL_BIO, IST_LEVEL,
		    imxpwm_intr, sc);

		reg = IR_RIE;
		imxpwm_write(sc, PWM_IR, reg);
	}

	if (sc->sc_hz <= 0)
		sc->sc_hz = IMXPWM_DEFAULT_HZ;
	div = 0;
	do {
		div++;
		sc->sc_cycle = sc->sc_freq / div / sc->sc_hz;
	} while (sc->sc_cycle > 0xffff);

	imxpwm_write(sc, PWM_PR, __SHIFTIN(sc->sc_cycle - 2, PR_PERIOD));

	reg = 0;
	reg |= __SHIFTIN(CLKSRC_IPG_CLK, CR_CLKSRC);
	reg |= __SHIFTIN(div - 1, CR_PRESCALER);
	reg |= CR_EN;
	imxpwm_write(sc, PWM_CR, reg);
}

int
imxpwm_set_pwm(struct imxpwm_softc *sc, int duty)
{
	if (duty < 0 || IMXPWM_DUTY_MAX < duty)
		return EINVAL;

	sc->sc_duty = duty;

	uint16_t reg = sc->sc_cycle * sc->sc_duty / IMXPWM_DUTY_MAX;
	if (reg != 0)
		reg -= 1;
	imxpwm_write(sc, PWM_SAR, __SHIFTIN(reg, SAR_SAMPLE));

	return 0;
}
