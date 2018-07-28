/* $NetBSD: exynos_pwm.c,v 1.1.2.2 2018/07/28 04:37:29 pgoyette Exp $ */

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

__KERNEL_RCSID(1, "$NetBSD: exynos_pwm.c,v 1.1.2.2 2018/07/28 04:37:29 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/pwm/pwmvar.h>

#include <dev/fdt/fdtvar.h>

#define	TCFG0			0x00
#define	TCFG1			0x04
#define	TCON			0x08
#define	 _TCON_OFF(n)		((n) == 0 ? 0 : (((n) + 1) * 4))
#define	 TCON_START(n)		__BIT(_TCON_OFF(n) + 0)
#define	 TCON_UPDATE(n)		__BIT(_TCON_OFF(n) + 1)
#define	 TCON_OUTINV(n)		__BIT(_TCON_OFF(n) + 2)
#define	 TCON_AUTO_RELOAD(n)	__BIT(_TCON_OFF(n) + 3)
#define	 TCON_DEADZONE_EN(n)	__BIT(_TCON_OFF(n) + 4)
#define	TCNTB(n)		(0x0c + (n) * 12)
#define	TCMPB(n)		(0x10 + (n) * 12)
#define	TCNTO(n)		(0x14 + (n) * 12)
#define	TINT_CSTAT		0x44

#define	PWM_NTIMERS		5

static const char * const compatible[] = {
	"samsung,exynos4210-pwm",
	NULL
};

struct exynos_pwm_softc;

struct exynos_pwm_timer {
	u_int			timer_index;
	struct pwm_controller	timer_pwm;
};

struct exynos_pwm_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct exynos_pwm_timer	sc_timer[PWM_NTIMERS];
	u_int			sc_timermask;

	u_int			sc_clkfreq;
};

#define	PWM_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	PWM_WRITE(sc, reg, val)		\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
exynos_pwm_enable(pwm_tag_t pwm, bool enable)
{
	struct exynos_pwm_timer * const timer = pwm->pwm_priv;
	struct exynos_pwm_softc * const sc = device_private(pwm->pwm_dev);
	uint32_t tcon;

	if (enable) {
		tcon = PWM_READ(sc, TCON);
		tcon &= ~TCON_START(timer->timer_index);
		tcon |= TCON_UPDATE(timer->timer_index);
		PWM_WRITE(sc, TCON, tcon);
		tcon &= ~TCON_UPDATE(timer->timer_index);
		tcon |= TCON_START(timer->timer_index);
		tcon |= TCON_AUTO_RELOAD(timer->timer_index);
		PWM_WRITE(sc, TCON, tcon);
	} else {
		tcon = PWM_READ(sc, TCON);
		tcon &= ~TCON_AUTO_RELOAD(timer->timer_index);
		PWM_WRITE(sc, TCON, tcon);
	}

	return 0;
}

static int
exynos_pwm_get_config(pwm_tag_t pwm, struct pwm_config *conf)
{
	struct exynos_pwm_timer * const timer = pwm->pwm_priv;
	struct exynos_pwm_softc * const sc = device_private(pwm->pwm_dev);
	uint32_t tcon, tcntb, tcmpb;

	tcon = PWM_READ(sc, TCON);
	tcntb = PWM_READ(sc, TCNTB(timer->timer_index));
	tcmpb = PWM_READ(sc, TCMPB(timer->timer_index));

	conf->polarity = (tcon & TCON_OUTINV(timer->timer_index)) ? PWM_ACTIVE_HIGH : PWM_ACTIVE_LOW;
	conf->period = (u_int)(((uint64_t)tcntb * 1000000000) / sc->sc_clkfreq);
	conf->duty_cycle = (u_int)(((uint64_t)tcmpb * 1000000000) / sc->sc_clkfreq);

	return 0;
}

static int
exynos_pwm_set_config(pwm_tag_t pwm, const struct pwm_config *conf)
{
	struct exynos_pwm_timer * const timer = pwm->pwm_priv;
	struct exynos_pwm_softc * const sc = device_private(pwm->pwm_dev);
	uint32_t tcon, tcntb, tcmpb;

	tcon = PWM_READ(sc, TCON);
	if (conf->polarity == PWM_ACTIVE_HIGH)
		tcon |= TCON_OUTINV(timer->timer_index);
	else
		tcon &= ~TCON_OUTINV(timer->timer_index);
	PWM_WRITE(sc, TCON, tcon);

	tcntb = conf->period / (1000000000 / sc->sc_clkfreq);
	tcmpb = conf->duty_cycle / (1000000000 / sc->sc_clkfreq);
	if (tcmpb == 0)
		tcmpb = 1;
	tcmpb = tcntb - tcmpb;

	PWM_WRITE(sc, TCNTB(timer->timer_index), tcntb - 1);
	PWM_WRITE(sc, TCMPB(timer->timer_index), tcmpb - 1);

	tcon = PWM_READ(sc, TCON);
	tcon |= TCON_UPDATE(timer->timer_index);
	tcon |= TCON_AUTO_RELOAD(timer->timer_index);
	PWM_WRITE(sc, TCON, tcon);

	tcon &= ~TCON_UPDATE(timer->timer_index);
	PWM_WRITE(sc, TCON, tcon);

	return 0;
}

static pwm_tag_t
exynos_pwm_get_tag(device_t dev, const void *data, size_t len)
{
	struct exynos_pwm_softc * const sc = device_private(dev);
	struct exynos_pwm_timer *timer;
	const u_int *pwm = data;
	struct pwm_config conf;

	if (len != 16)
		return NULL;

	const u_int index = be32toh(pwm[1]);
	if (index >= 32 || (sc->sc_timermask & __BIT(index)) == 0)
		return NULL;

	const u_int period = be32toh(pwm[2]);
	const u_int polarity = be32toh(pwm[3]);

	timer = &sc->sc_timer[index];

	/* Set initial timer polarity and period from specifier */
	exynos_pwm_get_config(&timer->timer_pwm, &conf);
	conf.period = period;
	conf.polarity = polarity ? PWM_ACTIVE_LOW : PWM_ACTIVE_HIGH;
	exynos_pwm_set_config(&timer->timer_pwm, &conf);

	return &timer->timer_pwm;
}

static struct fdtbus_pwm_controller_func exynos_pwm_funcs = {
	.get_tag = exynos_pwm_get_tag
};

static int
exynos_pwm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
exynos_pwm_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_pwm_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const u_int *data;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	int error, len, n;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	clk = fdtbus_clock_get(phandle, "timers");
	if (clk == NULL || clk_enable(clk) != 0) {
		aprint_error(": couldn't enable timers clock\n");
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
	for (n = 0; n < PWM_NTIMERS; n++) {
		sc->sc_timer[n].timer_index = n;
		sc->sc_timer[n].timer_pwm.pwm_enable = exynos_pwm_enable;
		sc->sc_timer[n].timer_pwm.pwm_get_config = exynos_pwm_get_config;
		sc->sc_timer[n].timer_pwm.pwm_set_config = exynos_pwm_set_config;
		sc->sc_timer[n].timer_pwm.pwm_dev = self;
		sc->sc_timer[n].timer_pwm.pwm_priv = &sc->sc_timer[n];
		sc->sc_timermask |= __BIT(n);
	}

	data = fdtbus_get_prop(phandle, "samsung,pwm-outputs", &len);
	if (data) {
		sc->sc_timermask = 0;
		for (n = 0; n < len / 4; n++) {
			sc->sc_timermask |= __BIT(be32toh(data[n]));
		}
	}

	aprint_naive("\n");
	aprint_normal(": PWM\n");

	fdtbus_register_pwm_controller(self, phandle,
	    &exynos_pwm_funcs);
}

CFATTACH_DECL_NEW(exynos_pwm, sizeof(struct exynos_pwm_softc),
	exynos_pwm_match, exynos_pwm_attach, NULL, NULL);
