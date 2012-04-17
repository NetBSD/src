/* $NetBSD: nhpow.c,v 1.1.6.2 2012/04/17 00:06:50 yamt Exp $ */

/*-
 * Copyright (c) 2012 Frank Wille.
 * All rights reserved.
 *
 * Written by Frank Wille for The NetBSD Project.
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

/*
 * NH230/231 power and LED control, button handling
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nhpow.c,v 1.1.6.2 2012/04/17 00:06:50 yamt Exp $");
#include "gpio.h"

#include <sys/param.h>
#include <sys/device.h>
#if NGPIO > 0
#include <sys/gpio.h>
#endif
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>

#if NGPIO > 0
#include <dev/gpio/gpiovar.h>
#endif
#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <machine/autoconf.h>

static int  nhpow_match(device_t, cfdata_t, void *);
static void nhpow_attach(device_t, device_t, void *);
static void nhpow_reboot(int);
static int nhpow_sysctl_fan(SYSCTLFN_PROTO);
static int hwintr(void *);
static void guarded_pbutton(void *);
static void sched_sysmon_pbutton(void *);

static void nhgpio_pin_write(void *, int, int);
#if NGPIO > 0
static int nhgpio_pin_read(void *, int);
static void nhgpio_pin_ctl(void *, int, int);
#endif

#define NHGPIO_PINS		8

/* write gpio */
#define NHGPIO_WRITE(sc,x)	bus_space_write_1(sc->sc_iot, sc->sc_ioh, 0, x)
#define NHGPIO_POWEROFF		0x01
#define NHGPIO_RESET		0x02
#define NHGPIO_STATUS_LED_OFF	0x04
#define NHGPIO_FAN_HIGH		0x08
#define NHGPIO_USB1_LED_OFF	0x40
#define NHGPIO_USB2_LED_OFF	0x80

/* read gpio */
#define NHGPIO_READ(sc)		bus_space_read_1(sc->sc_iot, sc->sc_ioh, 0)
#define NHGPIO_POWERBUTTON	0x01
#define NHGPIO_RESETBUTTON	0x02
#define NHGPIO_VERSIONMASK	0xf0
#define NHGPIO_VERSIONSHIFT	4

struct nhpow_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	callout_t		sc_ch_pbutton;
	struct sysmon_pswitch	sc_sm_pbutton;
	int			sc_sysctl_fan;
#if NGPIO > 0
	struct gpio_chipset_tag	sc_gpio_gc;
	gpio_pin_t		sc_gpio_pins[8];
#endif
	uint8_t			sc_gpio_wrstate;
};

CFATTACH_DECL_NEW(nhpow, sizeof(struct nhpow_softc),
    nhpow_match, nhpow_attach, NULL, NULL);

static int found = 0;

extern struct cfdriver nhpow_cd;
extern void (*md_reboot)(int);

static int
nhpow_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (found != 0 || strcmp(ma->ma_name, nhpow_cd.cd_name) != 0)
		return 0;

	return 1;
}

static void
nhpow_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
#if NGPIO > 0
	struct gpiobus_attach_args gba;
#endif
	const struct sysctlnode *rnode;
	struct sysctllog *clog;
	struct nhpow_softc *sc;
	int i;

	found = 1;
	sc = device_private(self);
	sc->sc_dev = self;

	/* map the first byte for GPIO */
	KASSERT(ma->ma_bst != NULL);
	sc->sc_iot = ma->ma_bst;
	i = bus_space_map(sc->sc_iot, 0, 1, 0, &sc->sc_ioh);
	if (i != 0) {
		aprint_error(": could not map error %d\n", i);
		return;
	}

	aprint_naive(": Power Button Manager\n");
	aprint_normal(": NH230/231 gpio board control, version %u\n",
	    (NHGPIO_READ(sc) & NHGPIO_VERSIONMASK) >> NHGPIO_VERSIONSHIFT);

	md_reboot = nhpow_reboot;		/* cpu_reboot() hook */
	callout_init(&sc->sc_ch_pbutton, 0);	/* power-button callout */

	/* establish button interrupt handler */
	intr_establish(I8259_ICU + 4, IST_EDGE_RISING, IPL_SCHED, hwintr, sc);
	aprint_normal_dev(self, "interrupting at irq %d\n", I8259_ICU + 4);

	/* register power button with sysmon */
	sysmon_task_queue_init();
	memset(&sc->sc_sm_pbutton, 0, sizeof(struct sysmon_pswitch));
	sc->sc_sm_pbutton.smpsw_name = device_xname(sc->sc_dev);
	sc->sc_sm_pbutton.smpsw_type = PSWITCH_TYPE_POWER;
	if (sysmon_pswitch_register(&sc->sc_sm_pbutton) != 0)
		aprint_error_dev(sc->sc_dev,
		    "unable to register power button with sysmon\n");

	/* create machdep.nhpow subtree for fan control */
	clog = NULL;
	sysctl_createv(&clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_EOL);
	sysctl_createv(&clog, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "nhpow", NULL,
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
	sysctl_createv(&clog, 0, &rnode, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, "fan",
	    SYSCTL_DESCR("Toggle high(1)/low(0) fan speed"),
	    nhpow_sysctl_fan, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	/* define initial output state */
	sc->sc_sysctl_fan = 0;
	sc->sc_gpio_wrstate = NHGPIO_USB1_LED_OFF | NHGPIO_USB2_LED_OFF;
	NHGPIO_WRITE(sc, sc->sc_gpio_wrstate);

#if NGPIO > 0
	/* initialize gpio pin array */
	for (i = 0; i < NHGPIO_PINS; i++) {
		sc->sc_gpio_pins[i].pin_num = i;
		sc->sc_gpio_pins[i].pin_caps = GPIO_PIN_INOUT;
		sc->sc_gpio_pins[i].pin_flags = GPIO_PIN_INOUT;
		sc->sc_gpio_pins[i].pin_state =
		    (sc->sc_gpio_wrstate & (1 << i)) ?
		    GPIO_PIN_HIGH : GPIO_PIN_LOW;
	}

	/* create controller tag and attach GPIO framework */
	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = nhgpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = nhgpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = nhgpio_pin_ctl;
	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = NHGPIO_PINS;
	config_found(self, &gba, gpiobus_print);
#endif
}

static void
nhgpio_pin_write(void *arg, int pin, int value)
{
	struct nhpow_softc *sc = arg;
	int p;

	KASSERT(sc != NULL);
	p = pin % NHGPIO_PINS;
	if (value)
		sc->sc_gpio_wrstate |= (1 << p);
	else
		sc->sc_gpio_wrstate &= ~(1 << p);
	NHGPIO_WRITE(sc, sc->sc_gpio_wrstate);
}

#if NGPIO > 0
static int
nhgpio_pin_read(void *arg, int pin)
{
	struct nhpow_softc *sc = arg;
	int p;

	KASSERT(sc != NULL);
	p = pin % NHGPIO_PINS;
	return (NHGPIO_READ(sc) >> p) & 1;
}

static void
nhgpio_pin_ctl(void *arg, int pin, int flags)
{

	/* nothing to control */
}
#endif

static void
nhpow_reboot(int howto)
{
	struct nhpow_softc *sc = device_lookup_private(&nhpow_cd, 0);

	if ((howto & RB_POWERDOWN) == RB_AUTOBOOT)
		NHGPIO_WRITE(sc, NHGPIO_STATUS_LED_OFF | NHGPIO_RESET);
	else
		NHGPIO_WRITE(sc, NHGPIO_STATUS_LED_OFF | NHGPIO_POWEROFF);

	tsleep(nhpow_reboot, PWAIT, "reboot", 0);
	/*NOTREACHED*/
}

static int
nhpow_sysctl_fan(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct nhpow_softc *sc;
	int error, t;

	sc = device_lookup_private(&nhpow_cd, 0);
	node = *rnode;
	t = sc->sc_sysctl_fan;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;
	if (t < 0 || t > 1)
		return EINVAL;

	if (sc->sc_sysctl_fan != t) {
		sc->sc_sysctl_fan = t;
		nhgpio_pin_write(sc, 3, t);	/* set new fan speed */
	}
	return 0;
}

static int
hwintr(void *arg)
{
	struct nhpow_softc *sc = arg;
	uint8_t buttons;

	callout_stop(&sc->sc_ch_pbutton);

	buttons = NHGPIO_READ(sc);
	if (!(buttons & NHGPIO_POWERBUTTON)) {
		/* power button, schedule 3 seconds poweroff guard time */
		callout_reset(&sc->sc_ch_pbutton, 3 * hz, guarded_pbutton, sc);
	}
	if (!(buttons & NHGPIO_RESETBUTTON)) {
		/* reset/setup button */
	}

	return 1;
}

static void
guarded_pbutton(void *arg)
{
	struct nhpow_softc *sc = arg;

	/* we're now in callout(9) context */
	if (!(NHGPIO_READ(sc) & NHGPIO_POWERBUTTON))
		sysmon_task_queue_sched(0, sched_sysmon_pbutton, sc);
}

static void
sched_sysmon_pbutton(void *arg)
{
	struct nhpow_softc *sc = arg;

	/* we're now in kthread(9) context */
	sysmon_pswitch_event(&sc->sc_sm_pbutton, PSWITCH_EVENT_PRESSED);
}
