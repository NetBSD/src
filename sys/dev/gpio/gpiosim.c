/* $NetBSD: gpiosim.c,v 1.23.6.1 2023/11/26 12:13:19 bouyer Exp $ */
/*      $OpenBSD: gpiosim.c,v 1.1 2008/11/23 18:46:49 mbalmer Exp $	*/

/*
 * Copyright (c) 2007 - 2011, 2013 Marc Balmer <marc@msys.ch>
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* 64 bit wide GPIO simulator  */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/ioccom.h>
#include <dev/gpio/gpiovar.h>
#include <sys/callout.h>
#include <sys/workqueue.h>

#include "gpiosim.h"
#include "ioconf.h"

#define	GPIOSIM_NPINS	64

struct gpiosim_irq {
	int (*sc_gpio_irqfunc)(void *);
	void *sc_gpio_irqarg;
	int sc_gpio_irqmode;
	bool sc_gpio_irqtriggered;
};

struct gpiosim_softc {
	device_t		sc_dev;
	device_t		sc_gdev;	/* gpio that attaches here */
	uint64_t		sc_state;
	struct gpio_chipset_tag	sc_gpio_gc;
	gpio_pin_t		sc_gpio_pins[GPIOSIM_NPINS];
        struct gpiosim_irq      sc_gpio_irqs[GPIOSIM_NPINS];

	struct sysctllog	*sc_log;
        struct workqueue        *sc_wq;
        callout_t               sc_co;
        bool                    sc_co_init;
	bool			sc_co_running;
        int                     sc_ms;
        kmutex_t 		sc_intr_mutex;
};

static int	gpiosim_match(device_t, cfdata_t, void *);
static void	gpiosim_attach(device_t, device_t, void *);
static int	gpiosim_detach(device_t, int);
static int	gpiosim_sysctl(SYSCTLFN_PROTO);
static int	gpiosim_ms_sysctl(SYSCTLFN_PROTO);

static int	gpiosim_pin_read(void *, int);
static void	gpiosim_pin_write(void *, int, int);
static void	gpiosim_pin_ctl(void *, int, int);

static void *   gpiosim_intr_establish(void *, int, int, int,
    int (*)(void *), void *);
static void     gpiosim_intr_disestablish(void *, void *);
static bool     gpiosim_gpio_intrstr(void *, int, int, char *, size_t);

void            gpiosim_wq(struct work *,void *);
void            gpiosim_co(void *);

CFATTACH_DECL_NEW(gpiosim, sizeof(struct gpiosim_softc), gpiosim_match,
    gpiosim_attach, gpiosim_detach, NULL);

int gpiosim_work;

#ifndef GPIOSIM_MS
#define GPIOSIM_MS 1000
#endif

static int
gpiosim_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

void
gpiosimattach(int num __unused)
{
	cfdata_t cf;
	int n, err;

	err = config_cfattach_attach(gpiosim_cd.cd_name, &gpiosim_ca);
	if (err)
		printf("%s: unable to register cfattach\n", gpiosim_cd.cd_name);

	for (n = 0; n < NGPIOSIM; n++) {
		cf = malloc(sizeof(*cf), M_DEVBUF, M_WAITOK);
		cf->cf_name = "gpiosim";
		cf->cf_atname = "gpiosim";
		cf->cf_unit = n;
		cf->cf_fstate = FSTATE_NOTFOUND;
		config_attach_pseudo(cf);
	}
}

static void
gpiosim_attach(device_t parent, device_t self, void *aux)
{
	struct gpiosim_softc *sc = device_private(self);
	struct gpiobus_attach_args gba;
	const struct sysctlnode *node;
	int i;
	int error = 0;

	sc->sc_dev = self;

	printf("%s", device_xname(sc->sc_dev));

	/* initialize pin array */
	for (i = 0; i < GPIOSIM_NPINS; i++) {
		sc->sc_gpio_pins[i].pin_num = i;
		sc->sc_gpio_pins[i].pin_caps = GPIO_PIN_INPUT |
		    GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN |
		    GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN |
		    GPIO_PIN_INVIN | GPIO_PIN_INVOUT;

		/* Set up what interrupt types are allowed */
		sc->sc_gpio_pins[i].pin_intrcaps =
		    GPIO_INTR_POS_EDGE |
		    GPIO_INTR_NEG_EDGE |
		    GPIO_INTR_DOUBLE_EDGE |
		    GPIO_INTR_HIGH_LEVEL |
		    GPIO_INTR_LOW_LEVEL |
		    GPIO_INTR_MPSAFE;
		sc->sc_gpio_irqs[i].sc_gpio_irqfunc = NULL;
		sc->sc_gpio_irqs[i].sc_gpio_irqarg = NULL;
		sc->sc_gpio_irqs[i].sc_gpio_irqmode = 0;
		sc->sc_gpio_irqs[i].sc_gpio_irqtriggered = false;

		/* read initial state */
		sc->sc_gpio_pins[i].pin_flags = GPIO_PIN_INPUT;}

	sc->sc_state = 0;
	sc->sc_ms = GPIOSIM_MS;
	sc->sc_co_init = false;

	mutex_init(&sc->sc_intr_mutex, MUTEX_DEFAULT, IPL_VM);

	/* create controller tag */
	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = gpiosim_pin_read;
	sc->sc_gpio_gc.gp_pin_write = gpiosim_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = gpiosim_pin_ctl;
        sc->sc_gpio_gc.gp_intr_establish = gpiosim_intr_establish;
        sc->sc_gpio_gc.gp_intr_disestablish = gpiosim_intr_disestablish;
	sc->sc_gpio_gc.gp_intr_str = gpiosim_gpio_intrstr;

	/* gba.gba_name = "gpio"; */
	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = GPIOSIM_NPINS;

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

        sysctl_createv(&sc->sc_log, 0, NULL, &node,
            0,
            CTLTYPE_NODE, device_xname(sc->sc_dev),
            SYSCTL_DESCR("GPIO simulator"),
            NULL, 0, NULL, 0,
            CTL_HW, CTL_CREATE, CTL_EOL);

        if (node == NULL) {
		aprint_error(": can't create sysctl node\n");
                return;
	}

        sysctl_createv(&sc->sc_log, 0, &node, NULL,
            CTLFLAG_READWRITE,
            CTLTYPE_QUAD, "value",
            SYSCTL_DESCR("Current GPIO simulator value"),
            gpiosim_sysctl, 0, (void *)sc, 0,
	    CTL_CREATE, CTL_EOL);

        sysctl_createv(&sc->sc_log, 0, &node, NULL,
            CTLFLAG_READWRITE,
            CTLTYPE_INT, "ms",
            SYSCTL_DESCR("Number of ms for level interrupts"),
            gpiosim_ms_sysctl, 0, &sc->sc_ms, 0,
	    CTL_CREATE, CTL_EOL);

	error = workqueue_create(&sc->sc_wq,"gsimwq",gpiosim_wq,sc,PRI_NONE,IPL_VM,WQ_MPSAFE);
	if (error != 0) {
		aprint_error(": can't create workqueue for interrupts\n");
                return;
	}

	callout_init(&sc->sc_co,CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_co,gpiosim_co, sc);
	sc->sc_co_running = false;
	sc->sc_co_init = true;

	aprint_normal(": simulating %d pins\n", GPIOSIM_NPINS);
	sc->sc_gdev = config_found(self, &gba, gpiobus_print, CFARGS_NONE);
}

static int
gpiosim_detach(device_t self, int flags)
{
	struct gpiosim_softc *sc = device_private(self);

	/* Detach the gpio driver that attached here */
	if (sc->sc_gdev != NULL)
		config_detach(sc->sc_gdev, 0);

	pmf_device_deregister(self);

	if (sc->sc_log != NULL) {
		sysctl_teardown(&sc->sc_log);
		sc->sc_log = NULL;
	}

	/* Destroy the workqueue, hope that it is empty */
	if (sc->sc_wq != NULL) {
		workqueue_destroy(sc->sc_wq);
	}

	sc->sc_co_running = false;

	/* Destroy any callouts */
	if (sc->sc_co_init) {
		callout_halt(&sc->sc_co,NULL);
		callout_destroy(&sc->sc_co);
	}
	return 0;
}

static int
gpiosim_sysctl(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct gpiosim_softc *sc;
	uint64_t val, error;
	uint64_t previous_val;
	int i;
	struct gpiosim_irq *irq;
	int t = 0;

	node = *rnode;
	sc = node.sysctl_data;

	node.sysctl_data = &val;

	val = sc->sc_state;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	mutex_enter(&sc->sc_intr_mutex);
	previous_val = sc->sc_state;
	sc->sc_state = val;
	for (i = 0; i < GPIOSIM_NPINS; i++) {
		irq = &sc->sc_gpio_irqs[i];
		/* Simulate edge interrupts ... */
		if ((previous_val & (1LL << i)) == 0 && (sc->sc_state & (1LL << i)) &&
		    irq->sc_gpio_irqfunc != NULL &&
		    (irq->sc_gpio_irqmode & (GPIO_INTR_POS_EDGE | GPIO_INTR_DOUBLE_EDGE))) {
			irq->sc_gpio_irqtriggered = true;
			t++;
		}
		if ((previous_val & (1LL << i)) && (sc->sc_state & (1LL << i)) == 0 &&
		    irq->sc_gpio_irqfunc != NULL &&
		    (irq->sc_gpio_irqmode & (GPIO_INTR_NEG_EDGE | GPIO_INTR_DOUBLE_EDGE))) {
			irq->sc_gpio_irqtriggered = true;
			t++;
		}
		/* Simulate level interrupts ... */
		if ((sc->sc_state & (1LL << i)) && irq->sc_gpio_irqfunc != NULL &&
		    (irq->sc_gpio_irqmode & GPIO_INTR_HIGH_LEVEL)) {
			irq->sc_gpio_irqtriggered = true;
		}
		if ((sc->sc_state & (1LL << i)) == 0 && irq->sc_gpio_irqfunc != NULL &&
		    (irq->sc_gpio_irqmode & GPIO_INTR_LOW_LEVEL)) {
			irq->sc_gpio_irqtriggered = true;
		}
		if ((sc->sc_state & (1LL << i)) && irq->sc_gpio_irqfunc != NULL &&
		    (irq->sc_gpio_irqmode & GPIO_INTR_LOW_LEVEL)) {
			irq->sc_gpio_irqtriggered = false;
		}
		if ((sc->sc_state & (1LL << i)) == 0 && irq->sc_gpio_irqfunc != NULL &&
		    (irq->sc_gpio_irqmode & GPIO_INTR_HIGH_LEVEL)) {
			irq->sc_gpio_irqtriggered = false;
		}
	}
	mutex_exit(&sc->sc_intr_mutex);

	if (t > 0) {
		workqueue_enqueue(sc->sc_wq,(struct work *)&gpiosim_work,NULL);
	}

	return 0;
}

int
gpiosim_ms_sysctl(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = *(int*)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	if (t < 1)
		return (EINVAL);

	*(int*)rnode->sysctl_data = t;

	return (0);
}

/* Interrupts though the read and write path are not simulated,
 * that is, an interrupt on the setting of an output or an
 * interrupt on a pin read.  It is not at all clear that it makes
 * any sense to do any of that, although real hardware in some cases
 * might trigger an interrupt on an output pin.
 */

static int
gpiosim_pin_read(void *arg, int pin)
{
	struct gpiosim_softc *sc = arg;

	if (sc->sc_state & (1LL << pin))
		return GPIO_PIN_HIGH;
	else
		return GPIO_PIN_LOW;
}

static void
gpiosim_pin_write(void *arg, int pin, int value)
{
	struct gpiosim_softc *sc = arg;

	if (value == 0)
		sc->sc_state &= ~(1LL << pin);
	else
		sc->sc_state |= (1LL << pin);
}

static void
gpiosim_pin_ctl(void *arg, int pin, int flags)
{
	struct gpiosim_softc *sc = arg;

	sc->sc_gpio_pins[pin].pin_flags = flags;
}

static void *
gpiosim_intr_establish(void *vsc, int pin, int ipl, int irqmode,
    int (*func)(void *), void *arg)
{
	struct gpiosim_softc * const sc = vsc;
	struct gpiosim_irq *irq;

	mutex_enter(&sc->sc_intr_mutex);
	irq = &sc->sc_gpio_irqs[pin];
	irq->sc_gpio_irqfunc = func;
	irq->sc_gpio_irqmode = irqmode;
	irq->sc_gpio_irqarg = arg;

	/* The first level interrupt starts the callout if it is not running */
	if (((irqmode & GPIO_INTR_HIGH_LEVEL) ||
	    (irqmode & GPIO_INTR_LOW_LEVEL)) &&
	    (sc->sc_co_running == false)) {
		callout_schedule(&sc->sc_co,mstohz(sc->sc_ms));
		sc->sc_co_running = true;
	}

	/* Level interrupts can start as soon as a IRQ handler is installed */
	if (((irqmode & GPIO_INTR_HIGH_LEVEL) && (sc->sc_state & (1LL << pin))) ||
	    ((irqmode & GPIO_INTR_LOW_LEVEL) && ((sc->sc_state & (1LL << pin)) == 0))) {
		irq->sc_gpio_irqtriggered = true;
	}

	mutex_exit(&sc->sc_intr_mutex);

	return(irq);
}

static void
gpiosim_intr_disestablish(void *vsc, void *ih)
{
	struct gpiosim_softc * const sc = vsc;
	struct gpiosim_irq *irq = ih;
	struct gpiosim_irq *lirq;
	int i;
	bool has_level = false;

	mutex_enter(&sc->sc_intr_mutex);
	irq->sc_gpio_irqfunc = NULL;
	irq->sc_gpio_irqmode = 0;
	irq->sc_gpio_irqarg = NULL;
	irq->sc_gpio_irqtriggered = false;

	/* Check for any level interrupts and stop the callout
	 * if there are none.
	 */
	for (i = 0;i < GPIOSIM_NPINS; i++) {
		lirq = &sc->sc_gpio_irqs[i];
		if (lirq->sc_gpio_irqmode & (GPIO_INTR_HIGH_LEVEL | GPIO_INTR_LOW_LEVEL)) {
			has_level = true;
			break;
		}
	}
	if (has_level == false) {
		sc->sc_co_running = false;
	}
	mutex_exit(&sc->sc_intr_mutex);
}

static bool
gpiosim_gpio_intrstr(void *vsc, int pin, int irqmode, char *buf, size_t buflen)
{

        if (pin < 0 || pin >= GPIOSIM_NPINS)
                return (false);

        snprintf(buf, buflen, "GPIO %d", pin);

        return (true);
}

/* The workqueue handles edge the simulation of edge interrupts */
void
gpiosim_wq(struct work *wk, void *arg)
{
	struct gpiosim_softc *sc = arg;
	struct gpiosim_irq *irq;
	int i;

	mutex_enter(&sc->sc_intr_mutex);
	for (i = 0; i < GPIOSIM_NPINS; i++) {
		irq = &sc->sc_gpio_irqs[i];
		if (irq->sc_gpio_irqtriggered &&
		    irq->sc_gpio_irqfunc != NULL &&
		    (irq->sc_gpio_irqmode & (GPIO_INTR_POS_EDGE | GPIO_INTR_NEG_EDGE | GPIO_INTR_DOUBLE_EDGE))) {
			(*irq->sc_gpio_irqfunc)(irq->sc_gpio_irqarg);
			irq->sc_gpio_irqtriggered = false;
		}
	}
	mutex_exit(&sc->sc_intr_mutex);
}

/* This runs as long as there are level interrupts to simulate */
void
gpiosim_co(void *arg)
{
	struct gpiosim_softc *sc = arg;
	struct gpiosim_irq *irq;
	int i;

	mutex_enter(&sc->sc_intr_mutex);
	for (i = 0; i < GPIOSIM_NPINS; i++) {
		irq = &sc->sc_gpio_irqs[i];
		if (irq->sc_gpio_irqtriggered &&
		    irq->sc_gpio_irqfunc != NULL &&
		    (irq->sc_gpio_irqmode & (GPIO_INTR_HIGH_LEVEL | GPIO_INTR_LOW_LEVEL))) {
			(*irq->sc_gpio_irqfunc)(irq->sc_gpio_irqarg);
		}
	}
	mutex_exit(&sc->sc_intr_mutex);

	if (sc->sc_co_running == true) {
		callout_schedule(&sc->sc_co,mstohz(sc->sc_ms));
	}
}


MODULE(MODULE_CLASS_DRIVER, gpiosim, "gpio");

#ifdef _MODULE
static const struct cfiattrdata gpiobus_iattrdata = {
	"gpiobus", 0, { { NULL, NULL, 0 },}
};
static const struct cfiattrdata *const gpiosim_attrs[] = {
	&gpiobus_iattrdata, NULL
};
CFDRIVER_DECL(gpiosim, DV_DULL, gpiosim_attrs);
extern struct cfattach gpiosim_ca;
static int gpiosimloc[] = {
	-1,
	-1,
	-1
};
static struct cfdata gpiosim_cfdata[] = {
	{
		.cf_name = "gpiosim",
		.cf_atname = "gpiosim",
		.cf_unit = 0,
		.cf_fstate = FSTATE_STAR,
		.cf_loc = gpiosimloc,
		.cf_flags = 0,
		.cf_pspec = NULL,
	},
	{ NULL, NULL, 0, FSTATE_NOTFOUND, NULL, 0, NULL }
};
#endif

static int
gpiosim_modcmd(modcmd_t cmd, void *opaque)
{
#ifdef _MODULE
	int error = 0;
#endif
	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_cfdriver_attach(&gpiosim_cd);
		if (error)
			return error;

		error = config_cfattach_attach(gpiosim_cd.cd_name,
		    &gpiosim_ca);
		if (error) {
			config_cfdriver_detach(&gpiosim_cd);
			aprint_error("%s: unable to register cfattach\n",
			    gpiosim_cd.cd_name);
			return error;
		}
		error = config_cfdata_attach(gpiosim_cfdata, 1);
		if (error) {
			config_cfattach_detach(gpiosim_cd.cd_name,
			    &gpiosim_ca);
			config_cfdriver_detach(&gpiosim_cd);
			aprint_error("%s: unable to register cfdata\n",
			    gpiosim_cd.cd_name);
			return error;
		}
		config_attach_pseudo(gpiosim_cfdata);
#endif
		return 0;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_cfdata_detach(gpiosim_cfdata);
		if (error)
			return error;

		config_cfattach_detach(gpiosim_cd.cd_name, &gpiosim_ca);
		config_cfdriver_detach(&gpiosim_cd);
#endif
		return 0;
	case MODULE_CMD_AUTOUNLOAD:
		/* no auto-unload */
		return EBUSY;
	default:
		return ENOTTY;
	}
}
