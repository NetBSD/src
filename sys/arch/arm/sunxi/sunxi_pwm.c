/* $NetBSD: sunxi_pwm.c,v 1.1.2.2 2018/05/21 04:35:59 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: sunxi_pwm.c,v 1.1.2.2 2018/05/21 04:35:59 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/pwm/pwmvar.h>

#include <dev/fdt/fdtvar.h>

#define	PWM_CH_CTRL		0x00
#define	 PWM0_RDY		__BIT(28)
#define	 PWM0_BYPASS		__BIT(9)
#define	 PWM_CH0_PUL_START	__BIT(8)
#define	 PWM_CHANNEL0_MODE	__BIT(7)
#define	 SCLK_CH0_GATING	__BIT(6)
#define	 PWM_CH0_ACT_STA	__BIT(5)
#define	 PWM_CH0_EN		__BIT(4)
#define	 PWM_CH0_PRESCAL	__BITS(3,0)
#define	PWM_CH0_PERIOD		0x04
#define	 PWM_CH0_ENTIRE_CYS	__BITS(31,16)
#define	 PWM_CH0_ENTIRE_ACT_CYS	__BITS(15,0)

enum sunxi_pwm_type {
	PWM_A64 = 1,
};

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun50i-a64-pwm",	PWM_A64 },
	{ NULL }
};

struct sunxi_pwm_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct pwm_controller	sc_pwm;
	struct pwm_config	sc_conf;

	u_int			sc_clkfreq;
};

#define	PWM_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	PWM_WRITE(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static pwm_tag_t
sunxi_pwm_get_tag(device_t dev, const void *data, size_t len)
{
	struct sunxi_pwm_softc * const sc = device_private(dev);
	const u_int *pwm = data;

	if (len != 16)
		return NULL;

	const u_int index = be32toh(pwm[1]);
	if (index != 0)
		return NULL;

	const u_int period = be32toh(pwm[2]);
	const u_int polarity = be32toh(pwm[3]);

	sc->sc_conf.period = period;
	sc->sc_conf.polarity = polarity ? PWM_ACTIVE_LOW : PWM_ACTIVE_HIGH;

	return &sc->sc_pwm;
}

static struct fdtbus_pwm_controller_func sunxi_pwm_funcs = {
	.get_tag = sunxi_pwm_get_tag
};

static int
sunxi_pwm_enable(pwm_tag_t pwm, bool enable)
{
	struct sunxi_pwm_softc * const sc = device_private(pwm->pwm_dev);
	uint32_t ctrl, octrl;

	octrl = ctrl = PWM_READ(sc, PWM_CH_CTRL);
	if (enable)
		ctrl |= (PWM_CH0_EN | SCLK_CH0_GATING);
	else
		ctrl &= ~(PWM_CH0_EN | SCLK_CH0_GATING);

	if (ctrl != octrl)
		PWM_WRITE(sc, PWM_CH_CTRL, ctrl);

	return 0;
}

static int
sunxi_pwm_get_config(pwm_tag_t pwm, struct pwm_config *conf)
{
	struct sunxi_pwm_softc * const sc = device_private(pwm->pwm_dev);
	uint32_t ctrl, ch_period;

	ctrl = PWM_READ(sc, PWM_CH_CTRL);
	ch_period = PWM_READ(sc, PWM_CH0_PERIOD);

	const uint64_t rate = sc->sc_clkfreq / 120;
	const u_int cycles = __SHIFTOUT(ch_period, PWM_CH0_ENTIRE_CYS) + 1;
	const u_int act_cycles = __SHIFTOUT(ch_period, PWM_CH0_ENTIRE_ACT_CYS);

	conf->polarity = (ctrl & PWM_CH0_ACT_STA) ? PWM_ACTIVE_HIGH : PWM_ACTIVE_LOW;
	conf->period = (u_int)(((uint64_t)cycles * 1000000000) / rate);
	conf->duty_cycle = (u_int)(((uint64_t)act_cycles * 1000000000) / rate);

	return 0;
}

static int
sunxi_pwm_set_config(pwm_tag_t pwm, const struct pwm_config *conf)
{
	struct sunxi_pwm_softc * const sc = device_private(pwm->pwm_dev);
	uint32_t ctrl, ch_period;

	ctrl = PWM_READ(sc, PWM_CH_CTRL);
	if (ctrl & PWM0_RDY)
		return EBUSY;

	ctrl &= ~PWM0_BYPASS;		/* Prescaler /120 = 200 kHz */
	ctrl &= ~PWM_CH0_PRESCAL;
	ctrl |= __SHIFTIN(0, PWM_CH0_PRESCAL);

	ctrl &= ~PWM_CHANNEL0_MODE;	/* Cycle mode */
	if (conf->polarity == PWM_ACTIVE_HIGH)
		ctrl |= PWM_CH0_ACT_STA;
	else
		ctrl &= ~PWM_CH0_ACT_STA;

	const uint64_t rate = sc->sc_clkfreq / 120;
	const u_int cycles = (u_int)((conf->period * rate) / 1000000000);
	const u_int act_cycles = (u_int)((conf->duty_cycle * rate) / 1000000000);

	ch_period = __SHIFTIN(cycles - 1, PWM_CH0_ENTIRE_CYS);
	ch_period |= __SHIFTIN(act_cycles, PWM_CH0_ENTIRE_ACT_CYS);

	PWM_WRITE(sc, PWM_CH0_PERIOD, ch_period);
	PWM_WRITE(sc, PWM_CH_CTRL, ctrl);

	sc->sc_conf = *conf;

	return 0;
}

static int
sunxi_pwm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_pwm_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_pwm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	clk = fdtbus_clock_get_index(phandle, 0);
	if (clk == NULL) {
		aprint_error(": couldn't get clock\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_clkfreq = clk_get_rate(clk);
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": PWM\n");

	sc->sc_pwm.pwm_enable = sunxi_pwm_enable;
	sc->sc_pwm.pwm_get_config = sunxi_pwm_get_config;
	sc->sc_pwm.pwm_set_config = sunxi_pwm_set_config;
	sc->sc_pwm.pwm_dev = self;

	fdtbus_register_pwm_controller(self, phandle,
	    &sunxi_pwm_funcs);
}

CFATTACH_DECL_NEW(sunxi_pwm, sizeof(struct sunxi_pwm_softc),
	sunxi_pwm_match, sunxi_pwm_attach, NULL, NULL);
