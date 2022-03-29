/* $NetBSD: gpiopps.c,v 1.3 2022/03/29 22:10:42 pgoyette Exp $ */

/*
 * Copyright (c) 2016 Brad Spencer <brad@anduin.eldar.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gpiopps.c,v 1.3 2022/03/29 22:10:42 pgoyette Exp $");

/*
 * GPIO interface to the pps subsystem for ntp support.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bitops.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/timepps.h>

#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>

#define	GPIOPPS_NPINS		2

struct gpiopps_softc {
	device_t		sc_dev;
	void *			sc_gpio;
	struct gpio_pinmap	sc_map;
	int			_map[GPIOPPS_NPINS];
	struct {
		char		sc_intrstr[128];
		void *		sc_ih;
		int		sc_irqmode;
	} sc_intrs[GPIOPPS_NPINS];
	int			sc_assert_val;
	int			sc_npins;
	struct pps_state	sc_pps_state;
	bool			sc_functional;
	bool			sc_busy;
};

#define	GPIOPPS_FLAGS_ASSERT_NEG_EDGE	0x01
#define	GPIOPPS_FLAGS_NO_DOUBLE_EDGE	0x02

static int	gpiopps_match(device_t, cfdata_t, void *);
static void	gpiopps_attach(device_t, device_t, void *);
static int	gpiopps_detach(device_t, int);

CFATTACH_DECL_NEW(gpiopps, sizeof(struct gpiopps_softc),
		  gpiopps_match, gpiopps_attach,
		  gpiopps_detach, NULL /*activate*/);

extern struct cfdriver gpiopps_cd;

static dev_type_open(gpioppsopen);
static dev_type_close(gpioppsclose);
static dev_type_ioctl(gpioppsioctl);
const struct cdevsw gpiopps_cdevsw = {
	.d_open = gpioppsopen,
	.d_close = gpioppsclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = gpioppsioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

static int
gpiopps_match(device_t parent, cfdata_t cf, void *aux)
{
	struct gpio_attach_args *ga = aux;
	int bits;

	if (strcmp(ga->ga_dvname, cf->cf_name))
		return (0);
	
	if (ga->ga_offset == -1)
		return (0);

	/* One or 2 pins (unspecified, assume 1) */
	bits = gpio_npins(ga->ga_mask);
	if (bits > 2)
		return (0);

	return (1);
}

static void
gpiopps_attach(device_t parent, device_t self, void *aux)
{
	struct gpiopps_softc *sc = device_private(self);
	struct gpio_attach_args *ga = aux;
	int flags, intrcaps, npins;
	int assert_edge = GPIO_INTR_POS_EDGE;
	int clear_edge  = GPIO_INTR_NEG_EDGE;
	int mask = ga->ga_mask;

	sc->sc_dev = self;
	sc->sc_assert_val = GPIO_PIN_HIGH;

	/* Map pins */
	sc->sc_gpio = ga->ga_gpio;
	sc->sc_map.pm_map = sc->_map;

	/* Determine our pin configuation. */
	npins = gpio_npins(mask);
	if (npins == 0) {
		npins = 1;
		mask = 0x1;
	}

	/*
	 * Here's the different pin configurations we handle:
	 *
	 * 1 pin, single-edge capable pin -- interrupt on single-edge,
	 * only trigger ASSERT signal.
	 *
	 * 1 pin, double-edge capable pin -- interrupt on double-edge,
	 * trigger ASSERT and CLEAR signals, unless 0x2 is set in ga_flags,
	 * in which case we degrade to ASSERT only.
	 *
	 * 2 pins -- pin #0 is ASSERT signal, pin #1 is CLEAR signal.
	 *
	 * If 0x1 is set in ga_flags, ASSERT is negative edge, otherwise
	 * assert is positive edge.
	 */
	if (npins < 1 || npins > 2) {
		aprint_error(": invalid pin configuration\n");
		return;
	}
	if (ga->ga_flags & GPIOPPS_FLAGS_ASSERT_NEG_EDGE) {
		assert_edge = GPIO_INTR_NEG_EDGE;
		clear_edge  = GPIO_INTR_POS_EDGE;
		sc->sc_assert_val = GPIO_PIN_LOW;
	}

	if (gpio_pin_map(sc->sc_gpio, ga->ga_offset, mask,
			 &sc->sc_map)) {
		aprint_error(": can't map pins\n");
		return;
	}
	sc->sc_npins = npins;

	aprint_normal("\n");

	if (sc->sc_npins == 2) {
		intrcaps = gpio_pin_intrcaps(sc->sc_gpio, &sc->sc_map, 0);
		if ((intrcaps & assert_edge) == 0) {
			aprint_error_dev(sc->sc_dev,
			    "%s edge interrupt not supported for ASSERT\n",
			    assert_edge == GPIO_INTR_POS_EDGE ? "positive"
			    				      : "negative");
			gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
			return;
		}
		sc->sc_intrs[0].sc_irqmode = assert_edge;
		if (!gpio_intr_str(sc->sc_gpio, &sc->sc_map, 0,
				   sc->sc_intrs[0].sc_irqmode,
				   sc->sc_intrs[0].sc_intrstr,
				   sizeof(sc->sc_intrs[0].sc_intrstr))) {
			aprint_error_dev(self,
			    "failed to decode ASSERT interrupt\n");
			gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
			return;
		}
		flags = gpio_pin_get_conf(sc->sc_gpio, &sc->sc_map, 0);
		flags = (flags & ~(GPIO_PIN_OUTPUT|GPIO_PIN_INOUT)) |
		    GPIO_PIN_INPUT;
		if (!gpio_pin_set_conf(sc->sc_gpio, &sc->sc_map, 0, flags)) {
			aprint_error_dev(sc->sc_dev,
			    "ASSERT pin not capable of input\n");
			gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
			return;
		}

		intrcaps = gpio_pin_intrcaps(sc->sc_gpio, &sc->sc_map, 1);
		if ((intrcaps & clear_edge) == 0) {
			aprint_error_dev(sc->sc_dev,
			    "%s edge interrupt not supported for CLEAR\n",
			    clear_edge == GPIO_INTR_POS_EDGE ? "positive"
			    				     : "negative");
			gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
			return;
		}
		sc->sc_intrs[1].sc_irqmode = clear_edge;
		if (!gpio_intr_str(sc->sc_gpio, &sc->sc_map, 1,
				   sc->sc_intrs[1].sc_irqmode,
				   sc->sc_intrs[1].sc_intrstr,
				   sizeof(sc->sc_intrs[1].sc_intrstr))) {
			aprint_error_dev(self,
			    "failed to decode CLEAR interrupt\n");
			gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
			return;
		}
		flags = gpio_pin_get_conf(sc->sc_gpio, &sc->sc_map, 1);
		flags = (flags & ~(GPIO_PIN_OUTPUT|GPIO_PIN_INOUT)) |
		    GPIO_PIN_INPUT;
		if (!gpio_pin_set_conf(sc->sc_gpio, &sc->sc_map, 1, flags)) {
			aprint_error_dev(sc->sc_dev,
			    "CLEAR pin not capable of input\n");
			gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
			return;
		}

		aprint_normal_dev(self, "ASSERT interrupting on %s\n",
				  sc->sc_intrs[0].sc_intrstr);
		aprint_normal_dev(self, "CLEAR interrupting on %s\n",
				  sc->sc_intrs[1].sc_intrstr);
	} else {
		intrcaps = gpio_pin_intrcaps(sc->sc_gpio, &sc->sc_map, 0);
		bool double_edge = false;
		if ((intrcaps & GPIO_INTR_DOUBLE_EDGE) &&
		    (ga->ga_flags & GPIOPPS_FLAGS_NO_DOUBLE_EDGE) == 0) {
			sc->sc_intrs[0].sc_irqmode = GPIO_INTR_DOUBLE_EDGE;
			double_edge = true;
		} else if (intrcaps & assert_edge) {
			sc->sc_intrs[0].sc_irqmode = assert_edge;
		} else {
			aprint_error_dev(sc->sc_dev,
			    "%s edge interrupt not supported for ASSERT\n",
			    assert_edge == GPIO_INTR_POS_EDGE ? "positive"
			    				      : "negative");
			gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
			return;
		}
		if (!gpio_intr_str(sc->sc_gpio, &sc->sc_map, 0,
				   sc->sc_intrs[0].sc_irqmode,
				   sc->sc_intrs[0].sc_intrstr,
				   sizeof(sc->sc_intrs[0].sc_intrstr))) {
			aprint_error_dev(self,
			    "failed to decode interrupt\n");
			gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
			return;
		}
		flags = gpio_pin_get_conf(sc->sc_gpio, &sc->sc_map, 0);
		flags = (flags & ~(GPIO_PIN_OUTPUT|GPIO_PIN_INOUT)) |
		    GPIO_PIN_INPUT;
		if (!gpio_pin_set_conf(sc->sc_gpio, &sc->sc_map, 0, flags)) {
			aprint_error_dev(sc->sc_dev,
			    "ASSERT%s pin not capable of input\n",
			    double_edge ? "+CLEAR" : "");
			gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
			return;
		}

		aprint_normal_dev(self, "ASSERT%s interrupting on %s\n",
				  double_edge ? "+CLEAR" : "",
				  sc->sc_intrs[0].sc_intrstr);
	}

	/* Interrupt will be registered when device is opened for use. */

	sc->sc_functional = true;
}

static int
gpiopps_assert_intr(void *arg)
{
	struct gpiopps_softc *sc = arg;

	mutex_spin_enter(&timecounter_lock);
	pps_capture(&sc->sc_pps_state);
	pps_event(&sc->sc_pps_state, PPS_CAPTUREASSERT);
	mutex_spin_exit(&timecounter_lock);

	return (1);
}

static int
gpiopps_clear_intr(void *arg)
{
	struct gpiopps_softc *sc = arg;

	mutex_spin_enter(&timecounter_lock);
	pps_capture(&sc->sc_pps_state);
	pps_event(&sc->sc_pps_state, PPS_CAPTURECLEAR);
	mutex_spin_exit(&timecounter_lock);

	return (1);
}

static int
gpiopps_double_intr(void *arg)
{
	struct gpiopps_softc *sc = arg;
	int val = gpio_pin_read(sc->sc_gpio, &sc->sc_map, 0);

	if (val == sc->sc_assert_val)
		return (gpiopps_assert_intr(arg));
	return (gpiopps_clear_intr(arg));
}

static void
gpiopps_disable_interrupts(struct gpiopps_softc *sc)
{
	int i;

	for (i = 0; i < GPIOPPS_NPINS; i++) {
		if (sc->sc_intrs[i].sc_ih != NULL) {
			gpio_intr_disestablish(sc->sc_gpio,
					       sc->sc_intrs[i].sc_ih);
			sc->sc_intrs[i].sc_ih = NULL;
		}
	}
}

static void
gpiopps_reset(struct gpiopps_softc *sc)
{
	mutex_spin_enter(&timecounter_lock);
	sc->sc_pps_state.ppsparam.mode = 0;
	sc->sc_busy = false;
	mutex_spin_exit(&timecounter_lock);
}

static int
gpiopps_detach(device_t self, int flags)
{
	struct gpiopps_softc *sc = device_private(self);

	if (!sc->sc_functional) {
		/* Attach failed, no work to do; resources already released. */
		return (0);
	}

	if (sc->sc_busy)
		return (EBUSY);
	
	/*
	 * Clear the handler and disable the interrupt.
	 * NOTE: This should never be true, because we
	 * register the interrupt handler at open, and
	 * remove it at close.  We keep this as a backstop.
	 */
	gpiopps_disable_interrupts(sc);

	/* Release the pin. */
	gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);

	return (0);
}

static int
gpioppsopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct gpiopps_softc *sc;
	int error = EIO;

	sc = device_lookup_private(&gpiopps_cd, minor(dev));
	if (sc == NULL)
		return (ENXIO);

	if (!sc->sc_functional)
		return (EIO);

	mutex_spin_enter(&timecounter_lock);
	
	if (sc->sc_busy) {
		mutex_spin_exit(&timecounter_lock);
		return (0);
	}

	memset(&sc->sc_pps_state, 0, sizeof(sc->sc_pps_state));
	sc->sc_pps_state.ppscap = PPS_CAPTUREASSERT;
	if (sc->sc_npins == 2 ||
	    sc->sc_intrs[0].sc_irqmode == GPIO_INTR_DOUBLE_EDGE)
	    	sc->sc_pps_state.ppscap |= PPS_CAPTURECLEAR;
	pps_init(&sc->sc_pps_state);
	sc->sc_busy = true;

	mutex_spin_exit(&timecounter_lock);

	if (sc->sc_npins == 2) {
		sc->sc_intrs[0].sc_ih = gpio_intr_establish(sc->sc_gpio,
		    &sc->sc_map, 0, IPL_VM,
		    sc->sc_intrs[0].sc_irqmode | GPIO_INTR_MPSAFE,
		    gpiopps_assert_intr, sc);
		if (sc->sc_intrs[0].sc_ih == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "unable to establish ASSERT interrupt on %s\n",
			    sc->sc_intrs[0].sc_intrstr);
			goto out;
		}

		sc->sc_intrs[1].sc_ih = gpio_intr_establish(sc->sc_gpio,
		    &sc->sc_map, 1, IPL_VM,
		    sc->sc_intrs[1].sc_irqmode | GPIO_INTR_MPSAFE,
		    gpiopps_clear_intr, sc);
		if (sc->sc_intrs[1].sc_ih == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "unable to establish CLEAR interrupt on %s\n",
			    sc->sc_intrs[0].sc_intrstr);
			gpio_intr_disestablish(sc->sc_gpio,
					       sc->sc_intrs[0].sc_ih);
			goto out;
		}
	} else {
		bool double_edge =
		    sc->sc_intrs[0].sc_irqmode == GPIO_INTR_DOUBLE_EDGE;
		sc->sc_intrs[0].sc_ih = gpio_intr_establish(sc->sc_gpio,
		    &sc->sc_map, 0, IPL_VM,
		    sc->sc_intrs[0].sc_irqmode | GPIO_INTR_MPSAFE,
		    double_edge ? gpiopps_double_intr
				: gpiopps_assert_intr, sc);
		if (sc->sc_intrs[0].sc_ih == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "unable to establish ASSERT%s interrupt on %s\n",
			    double_edge ? "+CLEAR" : "",
			    sc->sc_intrs[0].sc_intrstr);
			goto out;
		}
	}

	error = 0;

 out:
	if (error) {
		gpiopps_disable_interrupts(sc);
		gpiopps_reset(sc);
	}
	return (error);
}

static int
gpioppsclose(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct gpiopps_softc *sc;

	sc = device_lookup_private(&gpiopps_cd, minor(dev));

	gpiopps_disable_interrupts(sc);
	gpiopps_reset(sc);

	return (0);
}

static int
gpioppsioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct gpiopps_softc *sc;
	int error = 0;

	sc = device_lookup_private(&gpiopps_cd, minor(dev));

	switch (cmd) {
	case PPS_IOC_CREATE:
	case PPS_IOC_DESTROY:
	case PPS_IOC_GETPARAMS:
	case PPS_IOC_SETPARAMS:
	case PPS_IOC_GETCAP:
	case PPS_IOC_FETCH:
	case PPS_IOC_KCBIND:
		mutex_spin_enter(&timecounter_lock);
		error = pps_ioctl(cmd, data, &sc->sc_pps_state);
		mutex_spin_exit(&timecounter_lock);
		break;
	
	default:
		error = EPASSTHROUGH;
	}

	return (error);
}

MODULE(MODULE_CLASS_DRIVER, gpiopps, "gpio");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
gpiopps_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;
#ifdef _MODULE
	int bmaj = -1, cmaj = -1;
#endif

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_gpiopps,
		    cfattach_ioconf_gpiopps, cfdata_ioconf_gpiopps);
		if (error) {
			aprint_error("%s: unable to init component\n",
			    gpiopps_cd.cd_name);
			return (error);
		}

		error = devsw_attach("gpiopps", NULL, &bmaj,
		    &gpiopps_cdevsw, &cmaj);
		if (error) {
			aprint_error("%s: unable to attach devsw\n",
			    gpiopps_cd.cd_name);
			config_fini_component(cfdriver_ioconf_gpiopps,
			    cfattach_ioconf_gpiopps, cfdata_ioconf_gpiopps);
		}
#endif
		return (error);
	case MODULE_CMD_FINI:
#ifdef _MODULE
		devsw_detach(NULL, &gpiopps_cdevsw);
		config_fini_component(cfdriver_ioconf_gpiopps,
		    cfattach_ioconf_gpiopps, cfdata_ioconf_gpiopps);
#endif
		return (0);
	default:
		return (ENOTTY);
	}
}
