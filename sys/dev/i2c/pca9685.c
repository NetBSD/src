/* $NetBSD: pca9685.c,v 1.6 2021/01/27 02:29:48 thorpej Exp $ */

/*-
 * Copyright (c) 2018, 2019 Jason R. Thorpe
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
__KERNEL_RCSID(0, "$NetBSD: pca9685.c,v 1.6 2021/01/27 02:29:48 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mutex.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/pca9685reg.h>

#include <dev/pwm/pwmvar.h>

#include <dev/fdt/fdtvar.h>

/*
 * Special channel number used to indicate that we want to set the
 * pulse mode for all channels on this controller.
 */
#define	PCA9685_ALL_CHANNELS	PCA9685_NCHANNELS

struct pcapwm_channel {
	struct pwm_controller	ch_controller;
	struct pwm_config	ch_conf;
	u_int			ch_number;
};

struct pcapwm_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;

	/*
	 * Locking order is:
	 *	pcapwm mutex -> i2c bus
	 */
	kmutex_t	sc_lock;

	/*
	 * The PCA9685 only has a single pre-scaler, so the configured
	 * PWM frequency / period is shared by all channels.
	 */
	u_int		sc_period;	/* nanoseconds */
	u_int		sc_clk_freq;
	bool		sc_ext_clk;
	bool		sc_invert;	/* "invert" property specified */
	bool		sc_open_drain;	/* "open-drain" property specified */

	/*
	 * +1 because we treat channel "16" as the all-channels
	 * pseudo-channel.
	 */
	struct pcapwm_channel sc_channels[PCA9685_NCHANNELS+1];
};

static int	pcapwm_pwm_enable(struct pwm_controller *, bool);
static int	pcapwm_pwm_get_config(struct pwm_controller *,
		    struct pwm_config *);
static int	pcapwm_pwm_set_config(struct pwm_controller *,
		    const struct pwm_config *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "nxp,pca9685-pwm" },
	DEVICE_COMPAT_EOL
};

static int
pcapwm_read1(struct pcapwm_softc * const sc, uint8_t reg, uint8_t *valp)
{

	return iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
			sc->sc_addr, &reg, sizeof(reg),
			valp, sizeof(*valp), 0);
}

static int
pcapwm_write1(struct pcapwm_softc * const sc, uint8_t reg, uint8_t val)
{

	return iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP,
			sc->sc_addr, &reg, sizeof(reg),
			&val, sizeof(val), 0);
}

static int
pcapwm_read_LEDn(struct pcapwm_softc * const sc, uint8_t reg, uint8_t *buf,
		 size_t buflen)
{

	/* We rely on register auto-increment being enabled. */
	return iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP,
			sc->sc_addr, &reg, sizeof(reg),
			buf, buflen, 0);
}

static int
pcapwm_write_LEDn(struct pcapwm_softc * const sc, uint8_t reg, uint8_t *buf,
		  size_t buflen)
{

	/* We rely on register auto-increment being enabled. */
	return iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP,
			sc->sc_addr, &reg, sizeof(reg),
			buf, buflen, 0);
}

static int
pcapwm_program_channel(struct pcapwm_softc * const sc,
		       struct pcapwm_channel * const chan,
		       uint16_t on_tick, uint16_t off_tick)
{
	const uint8_t reg = chan->ch_number == PCA9685_ALL_CHANNELS ?
	    PCA9685_ALL_LED_ON_L : PCA9685_LEDx_ON_L(chan->ch_number);
	uint8_t regs[4];
	int error;

	regs[0] = (uint8_t)(on_tick & 0xff);
	regs[1] = (uint8_t)((on_tick >> 8) & 0xff);
	regs[2] = (uint8_t)(off_tick & 0xff);
	regs[3] = (uint8_t)((off_tick >> 8) & 0xff);

	error = iic_acquire_bus(sc->sc_i2c, 0);
	if (error) {
		device_printf(sc->sc_dev,
		    "program_channel: failed to acquire I2C bus\n");
		return error;
	}

	error = pcapwm_write_LEDn(sc, reg, regs, sizeof(regs));

	iic_release_bus(sc->sc_i2c, 0);

	return error;
}

static int
pcapwm_inspect_channel(struct pcapwm_softc * const sc,
		       struct pcapwm_channel * const chan,
		       uint16_t *on_tickp, uint16_t *off_tickp)
{
	const uint8_t reg = chan->ch_number == PCA9685_ALL_CHANNELS ?
	    PCA9685_ALL_LED_ON_L : PCA9685_LEDx_ON_L(chan->ch_number);
	uint8_t regs[4];
	int error;

	error = iic_acquire_bus(sc->sc_i2c, 0);
	if (error) {
		device_printf(sc->sc_dev,
		    "inspect_channel: failed to acquire I2C bus\n");
		return error;
	}

	error = pcapwm_read_LEDn(sc, reg, regs, sizeof(regs));

	iic_release_bus(sc->sc_i2c, 0);

	if (error) {
		return error;
	}

	*on_tickp = regs[0] | (((uint16_t)regs[1]) << 8);
	*off_tickp = regs[2] | (((uint16_t)regs[3]) << 8);

	return 0;
}

static struct pcapwm_channel *
pcapwm_lookup_channel(struct pcapwm_softc * const sc, u_int index)
{

	if (index > PCA9685_ALL_CHANNELS)
		return NULL;

	return &sc->sc_channels[index];
}

static void
pcapwm_init_channel(struct pcapwm_softc * const sc, u_int index)
{
	struct pcapwm_channel * const chan = pcapwm_lookup_channel(sc, index);

	KASSERT(chan != NULL);

	chan->ch_number = index;

	chan->ch_controller.pwm_enable = pcapwm_pwm_enable;
	chan->ch_controller.pwm_get_config = pcapwm_pwm_get_config;
	chan->ch_controller.pwm_set_config = pcapwm_pwm_set_config;

	chan->ch_controller.pwm_dev = sc->sc_dev;
	chan->ch_controller.pwm_priv = chan;
}

static pwm_tag_t
pcapwm_get_tag(device_t dev, const void *data, size_t len)
{
	struct pcapwm_softc * const sc = device_private(dev);
	const u_int *pwm = data;

	/* #pwm-cells == 2 in the PCA9685 DT bindings. */
	if (len != 12)
		return NULL;

	/* Channel 16 is the special call-channels channel. */
	const u_int index = be32toh(pwm[1]);
	struct pcapwm_channel * const chan = pcapwm_lookup_channel(sc, index);
	if (chan == NULL)
		return NULL;

	const u_int period = be32toh(pwm[2]);

	mutex_enter(&sc->sc_lock);

	/*
	 * XXX Should we reflect the value of the "invert" property in
	 * pwm_config::polarity?  I'm thinking not, but...
	 */
	chan->ch_conf.period = period;
	chan->ch_conf.polarity = PWM_ACTIVE_HIGH;

	mutex_exit(&sc->sc_lock);

	return &chan->ch_controller;
}

static struct fdtbus_pwm_controller_func pcapwm_pwm_funcs = {
	.get_tag = pcapwm_get_tag,
};

static int
pcapwm_pwm_enable(pwm_tag_t pwm, bool enable)
{
	struct pcapwm_softc * const sc = device_private(pwm->pwm_dev);
	struct pcapwm_channel * const chan = pwm->pwm_priv;
	int error;

	if (enable) {
		/* Set whatever is programmed for the channel. */
		error = pwm_set_config(pwm, &chan->ch_conf);
		if (error) {
			device_printf(sc->sc_dev,
			    "enable: unable to set config for channel %u\n",
			    chan->ch_number);
		}
		return error;
	}

	mutex_enter(&sc->sc_lock);

	error = pcapwm_program_channel(sc, chan, 0, PCA9685_PWM_TICKS);
	if (error) {
		device_printf(sc->sc_dev,
		    "disable: unable to program channel %u\n",
		    chan->ch_number);
	}

	mutex_exit(&sc->sc_lock);

	return error;
}

static int
pcapwm_pwm_get_config(pwm_tag_t pwm, struct pwm_config *conf)
{
	struct pcapwm_softc * const sc = device_private(pwm->pwm_dev);
	struct pcapwm_channel * const chan = pwm->pwm_priv;
	uint16_t on_tick, off_tick;
	u_int duty_cycle;
	int error;

	mutex_enter(&sc->sc_lock);

	error = pcapwm_inspect_channel(sc, chan, &on_tick, &off_tick);
	if (error) {
		device_printf(sc->sc_dev,
		    "get_config: unable to inspect channel %u\n",
		        chan->ch_number);
		goto out;
	}

	if (on_tick & PCA9685_PWM_TICKS) {
		duty_cycle = chan->ch_conf.period;
	} else if (off_tick & PCA9685_PWM_TICKS) {
		duty_cycle = 0;
	} else {
		/*
		 * Compute the number of ticks, accounting for a non-zero
		 * on-tick (which the hardware can do, even if the software
		 * can't).
		 */
		int signed_off_tick = off_tick;
		signed_off_tick -= on_tick;
		if (signed_off_tick < 0)
			signed_off_tick += PCA9685_PWM_TICKS;
		const uint64_t nticks = signed_off_tick;
		duty_cycle = (u_int)((nticks * chan->ch_conf.period) /
		    PCA9685_PWM_TICKS);
	}

	*conf = chan->ch_conf;
	conf->duty_cycle = duty_cycle;

 out:
	mutex_exit(&sc->sc_lock);

	return error;
}

static int
pcapwm_pwm_set_config(pwm_tag_t pwm, const struct pwm_config *conf)
{
	struct pcapwm_softc * const sc = device_private(pwm->pwm_dev);
	struct pcapwm_channel * const chan = pwm->pwm_priv;
	int error = 0;

	mutex_enter(&sc->sc_lock);

	/* Only active-high is supported. */
	if (conf->polarity != PWM_ACTIVE_HIGH) {
		device_printf(sc->sc_dev,
		    "set_config: invalid polarity: %d\n", conf->polarity);
		error = EINVAL;
		goto out;
	}

	if (sc->sc_period != conf->period) {
		/*
		 * Formula for the prescale is:
		 *
		 *           (     clk_freq      )
		 *      round( ----------------- ) - 1
		 *           (  4096 * pwm_freq  )
		 *
		 * pwm_freq == 1000000000 / period.
		 *
		 * To do the rounding step, we scale the oscillator_freq
		 * by 100, check for the rounding condition, and then
		 * de-scale before the subtraction step.
		 */
		const u_int pwm_freq = 1000000000 / conf->period;
		u_int prescale = (sc->sc_clk_freq * 100) /
		    (PCA9685_PWM_TICKS * pwm_freq);
		if ((prescale % 100) >= 50)
			prescale += 100;
		prescale = (prescale / 100) - 1;
		if (prescale < PCA9685_PRESCALE_MIN ||
		    prescale > PCA9685_PRESCALE_MAX) {
			device_printf(sc->sc_dev,
			    "set_config: invalid period: %uns\n", conf->period);
			error = EINVAL;
			goto out;
		}

		error = iic_acquire_bus(sc->sc_i2c, 0);
		if (error) {
			device_printf(sc->sc_dev,
			    "set_config: unable to acquire I2C bus\n");
			goto out;
		}

		uint8_t mode1;
		error = pcapwm_read1(sc, PCA9685_MODE1, &mode1);
		if (error) {
			device_printf(sc->sc_dev,
			    "set_config: unable to read MODE1\n");
			goto out_release_i2c;
		}

		/* Disable the internal oscillator. */
		mode1 |= MODE1_SLEEP;
		error = pcapwm_write1(sc, PCA9685_MODE1, mode1);
		if (error) {
			device_printf(sc->sc_dev,
			    "set_config: unable to write MODE1\n");
			goto out_release_i2c;
		}

		/* Update the prescale register. */
		error = pcapwm_write1(sc, PCA9685_PRE_SCALE,
		    (uint8_t)(prescale & 0xff));
		if (error) {
			device_printf(sc->sc_dev,
			    "set_config: unable to write PRE_SCALE\n");
			goto out_release_i2c;
		}

		/*
		 * If we're using an external clock source, keep the
		 * internal oscillator turned off.
		 *
		 * XXX The datasheet is a little ambiguous about how this
		 * XXX is supposed to work -- on the same page it says to
		 * XXX perform this procedure, and also that PWM control of
		 * XXX the channels is not possible when the oscillator is
		 * XXX disabled.  I haven't tested this with an external
		 * XXX oscillator yet, so I don't know for sure.
		 */
		if (sc->sc_ext_clk) {
			mode1 |= MODE1_EXTCLK;
		} else {
			mode1 &= ~MODE1_SLEEP;
		}

		/*
		 * We rely on auto-increment for the PWM register updates.
		 */
		mode1 |= MODE1_AI;

		error = pcapwm_write1(sc, PCA9685_MODE1, mode1);
		if (error) {
			device_printf(sc->sc_dev,
			    "set_config: unable to write MODE1\n");
			goto out_release_i2c;
		}

		iic_release_bus(sc->sc_i2c, 0);

		if (sc->sc_ext_clk == false) {
			/* Wait for 500us for the clock to settle. */
			delay(500);
		}

		sc->sc_period = conf->period;
	}

	uint16_t on_tick, off_tick;

	/*
	 * The PWM framework doesn't support the phase-shift / start-delay
	 * feature of this chip, so all duty cycles start at 0 ticks.
	 */

	/*
	 * For full-on and full-off, use the magic FULL-{ON,OFF} values
	 * described in the data sheet.
	 */
	if (conf->duty_cycle == 0) {
		on_tick = 0;
		off_tick = PCA9685_PWM_TICKS;
	} else if (conf->duty_cycle == sc->sc_period) {
		on_tick = PCA9685_PWM_TICKS;
		off_tick = 0;
	} else {
		uint64_t ticks =
		    PCA9685_PWM_TICKS * (uint64_t)conf->duty_cycle;
		/* Scale up so we can check if we need to round. */
		ticks = (ticks * 100) / sc->sc_period;
		/* Round up. */
		if (ticks % 100)
			ticks += 100;
		ticks /= 100;
		if (ticks >= PCA9685_PWM_TICKS) {
			ticks = PCA9685_PWM_TICKS - 1;
		}

		on_tick = 0;
		off_tick = (u_int)ticks;
	}

	error = pcapwm_program_channel(sc, chan, on_tick, off_tick);
	if (error) {
		device_printf(sc->sc_dev,
		    "set_config: unable to program channel %u\n",
		    chan->ch_number);
		goto out;
	}

	chan->ch_conf = *conf;

 out:
	mutex_exit(&sc->sc_lock);

	return error;

 out_release_i2c:
	iic_release_bus(sc->sc_i2c, 0);
	goto out;
}

static int
pcapwm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args * const ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, cf, compat_data, &match_result)) {
		return match_result;
	}
	
	/* This device is direct-config only. */

	return 0;
}

static void
pcapwm_attach(device_t parent, device_t self, void *aux)
{
	struct pcapwm_softc * const sc = device_private(self);
	struct i2c_attach_args * const ia = aux;
	struct clk *clk;
	const int phandle = (int)ia->ia_cookie;
	u_int index;
	int error;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	aprint_naive("\n");
	aprint_normal(": PCA9685 PWM controller\n");

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	for (index = 0; index <= PCA9685_ALL_CHANNELS; index++) {
		pcapwm_init_channel(sc, index);
	}

	clk = fdtbus_clock_get_index(phandle, 0);
	if (clk != NULL) {
		sc->sc_ext_clk = true;
		sc->sc_clk_freq = clk_get_rate(clk);
	} else {
		sc->sc_clk_freq = PCA9685_INTERNAL_FREQ;
	}

	if (of_hasprop(phandle, "invert")) {
		sc->sc_invert = true;
	}

	if (of_hasprop(phandle, "open-drain")) {
		sc->sc_open_drain = true;
	}

	/*
	 * XXX No DT bindings for the OUTNEx configurations in
	 * MODE2.
	 */

	error = iic_acquire_bus(sc->sc_i2c, 0);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to acquire I2C bus\n");
		return;
	}

	/*
	 * Set up the outputs.  We want the channel to update when
	 * we send the I2C "STOP" condition.
	 */
	uint8_t mode2;
	error = pcapwm_read1(sc, PCA9685_MODE2, &mode2);
	if (error == 0) {
		mode2 &= ~(MODE2_OUTDRV | MODE2_OCH | MODE2_INVRT);
		if (sc->sc_invert) {
			mode2 |= MODE2_INVRT;
		}
		if (sc->sc_open_drain == false) {
			mode2 |= MODE2_OUTDRV;
		}
		error = pcapwm_write1(sc, PCA9685_MODE2, mode2);
	}
	iic_release_bus(sc->sc_i2c, 0);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to configure MODE2\n");
		return;
	}

	fdtbus_register_pwm_controller(self, phandle,
	    &pcapwm_pwm_funcs);
}

CFATTACH_DECL_NEW(pcapwm, sizeof(struct pcapwm_softc),
    pcapwm_match, pcapwm_attach, NULL, NULL);
