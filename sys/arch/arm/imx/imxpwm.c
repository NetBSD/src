/*	$NetBSD: imxpwm.c,v 1.2 2020/05/20 05:10:42 hkenken Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: imxpwm.c,v 1.2 2020/05/20 05:10:42 hkenken Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/clk/clk_backend.h>

#include <arm/imx/imxpwmreg.h>
#include <arm/imx/imxpwmvar.h>

#include <dev/pwm/pwmvar.h>

#define	PWM_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define	PWM_WRITE(sc, reg, val)		\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

int
imxpwm_intr(void *arg)
{
	struct imxpwm_softc *sc = arg;

	uint32_t sts = PWM_READ(sc, PWM_SR);

	if (sts & PWM_SR_ROV) {
		if (sc->sc_handler != NULL)
			sc->sc_handler(sc->sc_cookie);
	}

	PWM_WRITE(sc, PWM_SR, sts);

	return 1;
}

static int
imxpwm_enable(pwm_tag_t pwm, bool enable)
{
	struct imxpwm_softc * const sc = device_private(pwm->pwm_dev);
	uint32_t cr, ocr;

	ocr = cr = PWM_READ(sc, PWM_CR);
	if (enable)
		cr |= PWM_CR_EN;
	else
		cr &= ~PWM_CR_EN;

	if (cr != ocr)
		PWM_WRITE(sc, PWM_CR, cr);

	return 0;
}

static int
imxpwm_get_config(pwm_tag_t pwm, struct pwm_config *conf)
{
	struct imxpwm_softc * const sc = device_private(pwm->pwm_dev);
	uint32_t cr, sar, pr;

	cr = PWM_READ(sc, PWM_CR);
	sar = PWM_READ(sc, PWM_SAR);
	pr = PWM_READ(sc, PWM_PR);

	const int div		= __SHIFTOUT(cr, PWM_CR_PRESCALER) + 1;
	const int polarity	= __SHIFTOUT(cr, PWM_CR_POUTC);
	const uint64_t rate	= sc->sc_freq / div;
	const u_int cycles	=  __SHIFTOUT(pr, PWM_PR_PERIOD) + 2;
	const u_int act_cycles	= __SHIFTOUT(sar, PWM_SAR_SAMPLE);

	conf->polarity	 = polarity ? PWM_ACTIVE_HIGH : PWM_ACTIVE_LOW;
	conf->period	 = (u_int)(((uint64_t)cycles * 1000000000) / rate);
	conf->duty_cycle = (u_int)(((uint64_t)act_cycles * 1000000000) / rate);

	return 0;
}

static int
imxpwm_set_config(pwm_tag_t pwm, const struct pwm_config *conf)
{
	struct imxpwm_softc * const sc = device_private(pwm->pwm_dev);
	uint32_t cr, sar, pr;

	if (conf->period == 0)
		return EINVAL;
	uint64_t rate;
	u_int cycles;
	int div = 0;
	do {
		div++;
		rate = sc->sc_freq / div;
		cycles = (u_int)((conf->period * rate) / 1000000000);
	} while (cycles > 0xffff);
	pr = __SHIFTIN(cycles - 2, PWM_PR_PERIOD);

	cr = PWM_READ(sc, PWM_CR);
	cr &= ~PWM_CR_PRESCALER;
	cr |= __SHIFTIN(div - 1, PWM_CR_PRESCALER);
	cr &= ~PWM_CR_POUTC;
	if (conf->polarity == PWM_ACTIVE_LOW)
		cr |= __SHIFTIN(1, PWM_CR_POUTC);

	u_int act_cycles = (u_int)((conf->duty_cycle * rate) / 1000000000);
	sar = __SHIFTIN(act_cycles, PWM_PR_PERIOD);

	PWM_WRITE(sc, PWM_SAR, sar);
	PWM_WRITE(sc, PWM_PR, pr);
	PWM_WRITE(sc, PWM_CR, cr);

	sc->sc_conf = *conf;

	return 0;
}

void
imxpwm_attach_common(struct imxpwm_softc *sc)
{
	uint32_t reg;
	int error;

	if (sc->sc_handler != NULL) {
		reg = PWM_IR_RIE;
		PWM_WRITE(sc, PWM_IR, reg);
	}

	if (sc->sc_clk) {
		error = clk_enable(sc->sc_clk);
		if (error != 0) {
			aprint_error(": couldn't enable clk\n");
			return;
		}
	}

	reg = PWM_READ(sc, PWM_CR);
	reg &= PWM_CR_CLKSRC;
	reg |= __SHIFTIN(CLKSRC_IPG_CLK, PWM_CR_CLKSRC);
	PWM_WRITE(sc, PWM_CR, reg);

	sc->sc_pwm.pwm_enable	  = imxpwm_enable;
	sc->sc_pwm.pwm_get_config = imxpwm_get_config;
	sc->sc_pwm.pwm_set_config = imxpwm_set_config;
	sc->sc_pwm.pwm_dev	  = sc->sc_dev;

	/* Set default settings */
	struct pwm_config conf = {
		.period = 1000000,
		.duty_cycle = 0,
		.polarity = PWM_ACTIVE_HIGH,
	};
	pwm_set_config(&sc->sc_pwm, &conf);
}

