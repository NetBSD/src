/*	$NetBSD: sc16is7xx.c,v 1.4 2025/12/01 14:56:03 brad Exp $	*/

/*
 * Copyright (c) 2025 Brad Spencer <brad@anduin.eldar.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "opt_fdt.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sc16is7xx.c,v 1.4 2025/12/01 14:56:03 brad Exp $");

/* Common driver for the frontend to the NXP SC16IS7xx UART bridge */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/workqueue.h>

#ifdef FDT
#include <dev/fdt/fdtvar.h>
#endif

#include <dev/ic/sc16is7xxreg.h>
#include <dev/ic/sc16is7xxvar.h>
#include <dev/ic/sc16is7xx_ttyvar.h>

const struct device_compatible_entry sc16is7xx_compat_data[] = {
	{.compat = "nxp,sc16is740"},
	{.compat = "nxp,sc16is741"},
	{.compat = "nxp,sc16is750"},
	{.compat = "nxp,sc16is752"},
	{.compat = "nxp,sc16is760"},
	{.compat = "nxp,sc16is762"},

	DEVICE_COMPAT_EOL
};

void sc16is7xx_attach(struct sc16is7xx_sc *);
static int sc16is7xx_verify_poll(SYSCTLFN_ARGS);
static int sc16is7xx_verify_freq_sysctl(SYSCTLFN_ARGS);
void sc16is7xx_thread(void *);

/* Artifical interrupts and the like */

static void
sc16is7xx_comintr(struct sc16is7xx_sc *sc)
{
	struct sc16is7xx_tty_softc *csc;

	for (int i = 0; i <= 1; i++){
		if (sc->sc_ttydevchannel[i] != NULL) {
			csc = device_private(sc->sc_ttydevchannel[i]);
			if (csc != NULL)
				comintr(&csc->sc_com);
		}
	}
}

void
sc16is7xx_thread(void *arg)
{
	struct sc16is7xx_sc *sc = arg;

	while (sc->sc_thread_state != SC16IS7XX_THREAD_EXIT) {
		mutex_enter(&sc->sc_thread_mutex);
		if (sc->sc_thread_state == SC16IS7XX_THREAD_PAUSED ||
		    sc->sc_thread_state == SC16IS7XX_THREAD_STALLED ||
		    sc->sc_thread_state == SC16IS7XX_THREAD_STOPPED)
			cv_wait(&sc->sc_threadvar, &sc->sc_thread_mutex);
		mutex_exit(&sc->sc_thread_mutex);

		mutex_enter(&sc->sc_thread_mutex);
		if (sc->sc_thread_state == SC16IS7XX_THREAD_RUNNING) {
			mutex_exit(&sc->sc_thread_mutex);
			sc16is7xx_comintr(sc);
		} else {
			mutex_exit(&sc->sc_thread_mutex);
		}

		mutex_enter(&sc->sc_thread_mutex);
		if (sc->sc_thread_state == SC16IS7XX_THREAD_RUNNING)
			cv_timedwait(&sc->sc_threadvar, &sc->sc_thread_mutex,
			    mstohz(sc->sc_poll));
		mutex_exit(&sc->sc_thread_mutex);
		
	}
	kthread_exit(0);
}

#ifdef FDT
/* This song and dance is needed because:

   sc16is7xx_intr is entered in a hard interrupt context.  It is not
   allowed to call workqueue_enqueue() and can't call comintr()
   because that might need to talk to the I2C or SPI bus, allocate
   memory, or otherwise wait.

   sc16is7xx_softintr wasn't able to call comintr() directly as that
   resulted in a panic.

   Hence..  sc16is7xx_comintr() and then comintr() needed to be
   entered from a thread or a workqueue worker.
*/

static void
sc16is7xx_wq(struct work *wk, void *arg)
{
	struct sc16is7xx_sc *sc = arg;

	sc16is7xx_comintr(sc);
}

static void
sc16is7xx_softintr(void *arg)
{
	struct sc16is7xx_sc *sc = arg;

	/* This is a little strange,  See workqueue(9) about "A work must
	 * not be enqueued again until the callback is called by the
	 * workqueue framework."
	 *
	 * If one tries to use a variable from the stack here you will panic if
	 * there are a number of interrupts coming in quickly.  kmem_alloc()
	 * also panic'ed.  This is running in a interrupt context, although it
	 * is soft and kmem(9) didn't like that.
	 *
	 * There could be a lot of interrupts going on if the there is a lot to
	 * receive or transmit.
	 *
	 * It may be possble to get clever and send a couple of work items, one
	 * for each possible channel. */

	workqueue_enqueue(sc->sc_wq, (struct work *)&sc->sc_frequency, NULL);
}

static int
sc16is7xx_intr(void *arg)
{
	struct sc16is7xx_sc *sc = arg;

	softint_schedule(sc->sc_sih);

	return 1;
}
#endif

/* GPIO */

static uint32_t
sc16is7xx_to_gpio_flags(struct sc16is7xx_sc *sc, int pin, int nc,
    uint8_t iocontrol, uint8_t iodir)
{
	int f = 0;

	if (pin <= 3) {
		if (nc == 2) {
			if (iocontrol & SC16IS7XX_IOCONTROL_3_0)
				f = GPIO_PIN_ALT0;
		}
		if (f == 0) {
			if (iodir & (1 << pin))
				f = GPIO_PIN_OUTPUT;
			else
				f = GPIO_PIN_INPUT;
		}
	} else {
		if (iocontrol & SC16IS7XX_IOCONTROL_7_4)
			f = GPIO_PIN_ALT0;
		else if (iodir & (1 << pin))
			f = GPIO_PIN_OUTPUT;
		else
			f = GPIO_PIN_INPUT;
	}

	return f;
}

static int
sc16is7xx_gpio_pin_read(void *arg, int pin)
{
	struct sc16is7xx_sc *sc = arg;
	uint8_t r;
	int rr = GPIO_PIN_LOW, error;

	error = sc->sc_funcs->read_reg(sc, SC16IS7XX_REGISTER_IOSTATE, 1, &r, 1);
	if (!error && (r & (1 << pin)))
		rr = GPIO_PIN_HIGH;

	return rr;
}

static void
sc16is7xx_gpio_pin_write(void *arg, int pin, int value)
{
	struct sc16is7xx_sc *sc = arg;
	uint8_t r;
	int error;

	error = sc->sc_funcs->read_reg(sc, SC16IS7XX_REGISTER_IOSTATE, 1, &r, 1);
	if (!error) {
		if (value)
			r |= (1 << pin);
		else
			r &= ~(1 << pin);
		error = sc->sc_funcs->write_reg(sc, SC16IS7XX_REGISTER_IOSTATE, 1, &r, 1);
	}
}

static void
sc16is7xx_gpio_ctl_alt0(struct sc16is7xx_sc *sc, uint8_t bank_mask,
    int low_pin, int high_pin, uint32_t flags)
{
	int error;
	uint8_t iocontrol;

	error = sc->sc_funcs->read_reg(sc, SC16IS7XX_REGISTER_IOCONTROL, 1, &iocontrol, 1);
	if (!error) {
		iocontrol |= bank_mask;
		error = sc->sc_funcs->write_reg(sc, SC16IS7XX_REGISTER_IOCONTROL, 1, &iocontrol, 1);
		if (!error) {
			for (int i = low_pin; i <= high_pin; i++)
				sc->sc_gpio_pins[i].pin_flags = flags;
		}
	}
}

static void
sc16is7xx_gpio_ctl_inout(struct sc16is7xx_sc *sc, uint8_t bank_mask,
    int low_pin, int high_pin, int a_pin, uint32_t flags)
{
	int error;
	uint8_t iocontrol, iodir;

	error = sc->sc_funcs->read_reg(sc, SC16IS7XX_REGISTER_IOCONTROL, 1, &iocontrol, 1);
	if (!error) {
		iocontrol &= ~bank_mask;
		error = sc->sc_funcs->write_reg(sc, SC16IS7XX_REGISTER_IOCONTROL, 1, &iocontrol, 1);
		if (!error) {
			error = sc->sc_funcs->read_reg(sc, SC16IS7XX_REGISTER_IODIR, 1, &iodir, 1);
			if (flags & GPIO_PIN_OUTPUT)
				iodir |= (1 << a_pin);
			if (flags & GPIO_PIN_INPUT)
				iodir &= ~(1 << a_pin);
			error = sc->sc_funcs->write_reg(sc, SC16IS7XX_REGISTER_IODIR, 1, &iodir, 1);
			if (!error) {
				for (int i = low_pin; i <= high_pin; i++)
					sc->sc_gpio_pins[i].pin_flags = sc16is7xx_to_gpio_flags(sc, i, sc->sc_num_channels, iocontrol, iodir);
			}
		}
	}
}

static void
sc16is7xx_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct sc16is7xx_sc *sc = arg;
	uint8_t bank_mask;
	int low_pin, high_pin;

	if (pin <= 3) {
		bank_mask = SC16IS7XX_IOCONTROL_3_0;
		low_pin = 0;
		high_pin = 3;
	} else {
		bank_mask = SC16IS7XX_IOCONTROL_7_4;
		low_pin = 4;
		high_pin = SC16IS7XX_NPINS - 1;
	}

	if (flags & GPIO_PIN_ALT0) {
		sc16is7xx_gpio_ctl_alt0(sc, bank_mask, low_pin, high_pin, flags);
	} else {
		sc16is7xx_gpio_ctl_inout(sc, bank_mask, low_pin, high_pin, pin, flags);
	}
}
/* sysctl */

int
sc16is7xx_verify_poll(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (t < 1)
		return EINVAL;

	*(int *)rnode->sysctl_data = t;

	return 0;
}

int
sc16is7xx_verify_freq_sysctl(SYSCTLFN_ARGS)
{
	struct sc16is7xx_sc *sc;
	struct sc16is7xx_tty_softc *csc;
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	sc = node.sysctl_data;
	t = sc->sc_frequency;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (t < 1)
		return EINVAL;

	sc->sc_frequency = t;

	for (int i = 0; i <= 1; i++){
		if (sc->sc_ttydevchannel[i] != NULL) {
			csc = device_private(sc->sc_ttydevchannel[i]);
			if (csc != NULL)
				csc->sc_com.sc_frequency = sc->sc_frequency;
		}
	}

	return 0;
}

static int
sc16is7xx_sysctl_init(struct sc16is7xx_sc *sc)
{
	int error;
	const struct sysctlnode *cnode;
	int sysctlroot_num;

	if ((error = sysctl_createv(&sc->sc_sc16is7xx_log, 0, NULL, &cnode,
		    0, CTLTYPE_NODE, device_xname(sc->sc_dev),
		    SYSCTL_DESCR("sc16ix7xx controls"), NULL, 0, NULL, 0, CTL_HW,
		    CTL_CREATE, CTL_EOL)) != 0)
		return error;

	sysctlroot_num = cnode->sysctl_num;

	if ((error = sysctl_createv(&sc->sc_sc16is7xx_log, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT, "frequency",
		    SYSCTL_DESCR("Frequency of the oscillator in Hz"), sc16is7xx_verify_freq_sysctl, 0,
		    (void *)sc, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
		    CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sc16is7xx_log, 0, NULL, &cnode,
		    CTLFLAG_READWRITE, CTLTYPE_INT, "poll",
		    SYSCTL_DESCR("In polling mode, how often to check the status register in ms"), sc16is7xx_verify_poll, 0,
		    &sc->sc_poll, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
		    CTL_EOL)) != 0)
		return error;

#define __DEBUGGING_THE_KTHREAD 1
#ifdef __DEBUGGING_THE_KTHREAD
	if ((error = sysctl_createv(&sc->sc_sc16is7xx_log, 0, NULL, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_INT, "thread_state",
		    SYSCTL_DESCR("Kernel thread state"), NULL, 0,
		    (int *)&sc->sc_thread_state, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
		    CTL_EOL)) != 0)
		return error;

	if ((error = sysctl_createv(&sc->sc_sc16is7xx_log, 0, NULL, &cnode,
		    CTLFLAG_READONLY, CTLTYPE_INT, "de_count",
		    SYSCTL_DESCR("Disable / Enable count"), NULL, 0,
		    &sc->sc_de_count, 0, CTL_HW, sysctlroot_num, CTL_CREATE,
		    CTL_EOL)) != 0)
		return error;
#endif

	return 0;
}
/* attach, detach and such */

static int
sc16is7xx_print(void *aux, const char *pnp)
{
	struct sc16is7xx_tty_attach_args *aa = aux;

	if (pnp)
		aprint_normal("%s", pnp);
	aprint_normal(" channel %d", aa->aa_channel);
	return (UNCONF);
}

void
sc16is7xx_attach(struct sc16is7xx_sc *sc)
{
	int error;
	char chip_type[SC16IS7XX_TYPE_STRINGLEN];
	int num_channels;
	int num_gpio;
	uint8_t buf[1];
	uint8_t iocontrol_reg;
	struct sc16is7xx_tty_attach_args aa;
	int reset_count = 0;

	aprint_normal("\n");

	sc->sc_frequency = SC16IS7XX_DEFAULT_FREQUENCY;
	sc->sc_poll = SC16IS7XX_DEFAULT_POLL;
	sc->sc_thread_state = SC16IS7XX_THREAD_GPIO;
	sc->sc_thread = NULL;
	sc->sc_wq = NULL;
	sc->sc_ih = NULL;
	sc->sc_sih = NULL;
	sc->sc_de_count = 0;

	mutex_init(&sc->sc_thread_mutex, MUTEX_DEFAULT, IPL_SOFTSERIAL);
	cv_init(&sc->sc_threadvar, "sc16is_cv");

	if ((error = sc16is7xx_sysctl_init(sc)) != 0) {
		aprint_error_dev(sc->sc_dev, "Can't setup sysctl tree (%d)\n", error);
		goto out;
	}

	/* Reset of the chip is a little odd.  Setting the SRESET bit is the
	 * only write that will NACK.  So, an expected error will result in
	 * that case.  Just ignore the error and read a few times to make sure
	 * that the reset is done.  After the reset, one must delay at least
	 * 3us before trying to talk to the chip again. */
	error = sc->sc_funcs->read_reg(sc, SC16IS7XX_REGISTER_IOCONTROL, 0, &iocontrol_reg, 1);
	if (!error) {
		iocontrol_reg |= SC16IS7XX_IOCONTROL_SRESET;
		sc->sc_funcs->write_reg(sc, SC16IS7XX_REGISTER_IOCONTROL, 0, &iocontrol_reg, 1);
		delay(5);
		do {
			error = sc->sc_funcs->read_reg(sc, SC16IS7XX_REGISTER_IOCONTROL, 0, &iocontrol_reg, 1);
			if (!error) {
				if (iocontrol_reg & SC16IS7XX_IOCONTROL_SRESET) {
					delay(2);
				}
				reset_count++;
			}
		} while ((iocontrol_reg & SC16IS7XX_IOCONTROL_SRESET) &&
		    (reset_count < 100) &&
		    (!error));
	}
	if (!error) {
		if (iocontrol_reg & SC16IS7XX_IOCONTROL_SRESET) {
			aprint_error_dev(sc->sc_dev,
			    "Chip did not reset in time.  reset_count=%d\n", reset_count);
		}
	} else {
		aprint_error_dev(sc->sc_dev,
		    "Error reseting chip: error=%d, reset_count=%d\n",
		    error, reset_count);
	}

	/* After a reset, the LCR register will be 0x1d.  If this isn't the
	 * case with channel 1, then channel 1 does not exist and this is a
	 * single UART chip, that is a SC16IS740, SC16IS750 or SC16IS760 and
	 * not a SC16IS752 or SC16IS762.
	 *
	 * There does not appear to be a way to distinguish a SC16IS740 /
	 * SC16IS741 from a SC16IS750 / SC16IS760.  The GPIO registers exist in
	 * both varients and appear to behave the same.  Obviously the physical
	 * pins are missing from the SC16IS74x branch of the family.  A bit
	 * more detail is available if you have a system with FDT.
	 *
	 * */

	num_channels = 1;
	num_gpio = SC16IS7XX_NPINS;
	strncpy(chip_type, "UNKNOWN", SC16IS7XX_TYPE_STRINGLEN);

	error = sc->sc_funcs->read_reg(sc, SC16IS7XX_REGISTER_LCR, 1, buf, 1);
	if (!error) {
		if (buf[0] == 0x1d) {
			strncpy(chip_type, "SC16IS752/SC16IS762", SC16IS7XX_TYPE_STRINGLEN);
			num_channels = 2;
		} else {
			strncpy(chip_type, "SC16IS740/SC16IS741/SC16IS750/SC16IS760", SC16IS7XX_TYPE_STRINGLEN);
		}
	}
	sc->sc_num_channels = num_channels;

	aprint_normal_dev(sc->sc_dev, "NXP %s\n", chip_type);

	sc->sc_ttydevchannel[0] = sc->sc_ttydevchannel[1] = NULL;

	bool use_polling = true;

#ifdef FDT

	error = workqueue_create(&sc->sc_wq, device_xname(sc->sc_dev),
	    sc16is7xx_wq, sc, PRI_SOFTSERIAL, IPL_SOFTSERIAL, WQ_MPSAFE);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "Could not create workqueue: %d\n",
		    error);
	}
	if (!error &&
	    devhandle_type(device_handle(sc->sc_dev)) == DEVHANDLE_TYPE_OF) {
		char intrstr[128];

		sc->sc_phandle = devhandle_to_of(device_handle(sc->sc_dev));

		if (!of_hasprop(sc->sc_phandle, "gpio-controller")) {
			num_gpio = 0;

			/* If there is an indication that the GPIO is not
			 * desired, turn the pins into modem control pins. */
			error = sc->sc_funcs->read_reg(sc, SC16IS7XX_REGISTER_IOCONTROL, 1, &iocontrol_reg, 1);
			if (error)
				goto out;
			if (num_channels == 2)
				iocontrol_reg |= SC16IS7XX_IOCONTROL_7_4 | SC16IS7XX_IOCONTROL_3_0;
			else
				iocontrol_reg |= SC16IS7XX_IOCONTROL_7_4;	/* No strictly correct
										 * for the SC16IS74x */
			error = sc->sc_funcs->write_reg(sc, SC16IS7XX_REGISTER_IOCONTROL, 1, &iocontrol_reg, 1);
			if (error)
				goto out;
		}
		if (fdtbus_intr_str(sc->sc_phandle, 0,
		    intrstr, sizeof(intrstr))) {

			aprint_normal_dev(sc->sc_dev, "interrupting on %s\n", intrstr);

			sc->sc_ih = fdtbus_intr_establish(sc->sc_phandle, 0, IPL_VM, 0,
			    sc16is7xx_intr, sc);

			if (sc->sc_ih == NULL) {
				aprint_error_dev(sc->sc_dev,
				    "unable to establish interrupt\n");
			} else {
				sc->sc_sih =
				    softint_establish(SOFTINT_SERIAL, sc16is7xx_softintr, sc);
				if (sc->sc_sih == NULL) {
					aprint_error_dev(sc->sc_dev,
					    "unable to establish soft interrupt\n");
					fdtbus_intr_disestablish(sc->sc_phandle, sc->sc_ih);
					sc->sc_ih = NULL;
				} else {
					use_polling = false;
				}
			}
		}
		const u_int * cf;
		int len;
		cf = fdtbus_get_prop(sc->sc_phandle, "clock-frequency", &len);
		if (cf != NULL && len > 0) {
			sc->sc_frequency = be32toh(cf[0]);
		}
	}

#endif

	if (use_polling) {
		aprint_normal_dev(sc->sc_dev, "polling for interrupts\n");
		sc->sc_thread_state = SC16IS7XX_THREAD_STOPPED;
	}

	for (int i = 0; i < num_channels; i++){
		aa.aa_channel = i;

		sc->sc_ttydevchannel[i] = config_found(sc->sc_dev, &aa, sc16is7xx_print,
		    CFARGS(.submatch = config_stdsubmatch,
		    .iattr = "sc16is7xxbus"));

	}

	if (num_gpio > 0) {
		struct gpiobus_attach_args gba;
		uint8_t iodir_reg;
		int c = 3;

		if (num_channels == 2)
			c = -1;

		error = sc->sc_funcs->read_reg(sc, SC16IS7XX_REGISTER_IOCONTROL, 1, &iocontrol_reg, 1);
		if (error)
			goto out;
		error = sc->sc_funcs->read_reg(sc, SC16IS7XX_REGISTER_IODIR, 1, &iodir_reg, 1);
		if (error)
			goto out;

		for (int i = 0; i < num_gpio; i++){
			sc->sc_gpio_pins[i].pin_num = i;
			sc->sc_gpio_pins[i].pin_caps = GPIO_PIN_INPUT;
			sc->sc_gpio_pins[i].pin_caps |= GPIO_PIN_OUTPUT;
			if (i > c)
				sc->sc_gpio_pins[i].pin_caps |= GPIO_PIN_ALT0;
			sc->sc_gpio_pins[i].pin_flags =
			    sc16is7xx_to_gpio_flags(sc, i, num_channels, iocontrol_reg, iodir_reg);
			sc->sc_gpio_pins[i].pin_intrcaps = 0;
			snprintf(sc->sc_gpio_pins[i].pin_defname, 4, "GP%d", i);
		}

		sc->sc_gpio_gc.gp_cookie = sc;
		sc->sc_gpio_gc.gp_pin_read = sc16is7xx_gpio_pin_read;
		sc->sc_gpio_gc.gp_pin_write = sc16is7xx_gpio_pin_write;
		sc->sc_gpio_gc.gp_pin_ctl = sc16is7xx_gpio_pin_ctl;

		gba.gba_gc = &sc->sc_gpio_gc;
		gba.gba_pins = sc->sc_gpio_pins;
		gba.gba_npins = SC16IS7XX_NPINS;

		sc->sc_gpio_dev = config_found(sc->sc_dev, &gba, gpiobus_print,
		    CFARGS(.iattr = "gpiobus"));
	}

out:

	return;
}

int
sc16is7xx_detach(struct sc16is7xx_sc *sc, int flags)
{
	int err = 0;

	err = config_detach_children(sc->sc_dev, flags);

	mutex_enter(&sc->sc_thread_mutex);
	if (sc->sc_thread &&
	    sc->sc_thread_state != SC16IS7XX_THREAD_GPIO &&
	    sc->sc_thread_state != SC16IS7XX_THREAD_STOPPED &&
	    sc->sc_thread_state != SC16IS7XX_THREAD_EXIT) {
		sc->sc_thread_state = SC16IS7XX_THREAD_EXIT;
		cv_signal(&sc->sc_threadvar);
		mutex_exit(&sc->sc_thread_mutex);
		kthread_join(sc->sc_thread);
	} else {
		mutex_exit(&sc->sc_thread_mutex);
	}
#ifdef FDT
	if (sc->sc_ih != NULL)
		fdtbus_intr_disestablish(sc->sc_phandle, sc->sc_ih);

	if (sc->sc_sih != NULL)
		softint_disestablish(sc->sc_sih);
#endif

	sysctl_teardown(&sc->sc_sc16is7xx_log);

	cv_destroy(&sc->sc_threadvar);
	mutex_destroy(&sc->sc_thread_mutex);

	return err;
}
