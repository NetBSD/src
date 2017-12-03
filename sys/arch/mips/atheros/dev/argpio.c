/* $NetBSD: argpio.c,v 1.6.12.1 2017/12/03 11:36:26 jdolecek Exp $ */

/*-
 * Copyright (c) 2006 Garrett D'Amore
 * All rights reserved.
 *
 * Written by Garrett D'Amore.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: argpio.c,v 1.6.12.1 2017/12/03 11:36:26 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/types.h>

#include <mips/atheros/include/arbusvar.h>

#include <dev/gpio/gpiovar.h>
#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <mips/atheros/dev/argpioreg.h>

/*
 * General Plan:
 *
 * Register GPIOs for all pins that are _not_ associated with the reset
 * pin.  (Possibly also not the sytem LED.)
 */

struct argpio_softc {
	device_t		sc_dev;
	struct gpio_chipset_tag	sc_gc;
	gpio_pin_t		sc_pins[ARGPIO_NPINS];
	int			sc_npins;
	bus_space_tag_t		sc_st;
	bus_space_handle_t	sc_sh;
	bus_size_t		sc_size;
	int			sc_caps;
	struct sysmon_pswitch	sc_resetbtn;
	void			*sc_ih;
	int			sc_rstpin;
	int			sc_ledpin;
};

static int argpio_match(device_t, cfdata_t, void *);
static void argpio_attach(device_t, device_t, void *);
static int argpio_intr(void *);
static void argpio_reset_pressed(void *);
static void argpio_ctl(void *, int, int);
static void argpio_write(void *, int, int);
static int argpio_read(void *, int);

CFATTACH_DECL_NEW(argpio, sizeof (struct argpio_softc), argpio_match,
    argpio_attach, NULL, NULL);

#define	INPUT(pin)	(1 << (pin))		/* input bit */
#define	INTR(pin)	(1 << ((pin) + 8))	/* interrupt bit */
#define	SERIAL(pin)	(1 << ((pin) + 16))	/* serial mux bit */

#define	GETREG(sc, o)		bus_space_read_4(sc->sc_st, sc->sc_sh, o)
#define	PUTREG(sc, o, v)	bus_space_write_4(sc->sc_st, sc->sc_sh, o, v)
#define	FLUSH(sc)		bus_space_barrier(sc->sc_st, sc->sc_sh, \
				0, 12, BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE)

int
argpio_match(device_t parent, cfdata_t match, void *aux)
{
	struct arbus_attach_args *aa = aux;

	return ((strcmp(aa->aa_name, "argpio") == 0) ? 1 : 0);
}

void
argpio_attach(device_t parent, device_t self, void *aux)
{
	struct argpio_softc *sc = device_private(self);
	struct arbus_attach_args *aa = aux;
	struct gpiobus_attach_args gba;
	prop_number_t	pn;
	int i;
	uint32_t reg;

	sc->sc_dev = self;
	sc->sc_st = aa->aa_bst;
	sc->sc_npins = ARGPIO_NPINS;
	sc->sc_size = aa->aa_size;
	sc->sc_ledpin = -1;
	sc->sc_rstpin = -1;

	if (bus_space_map(sc->sc_st, aa->aa_addr, sc->sc_size, 0,
		&sc->sc_sh) != 0) {
		printf(": unable to map registers!\n");
		return;
	}

	sc->sc_gc.gp_cookie = sc;
	sc->sc_gc.gp_pin_read = argpio_read;
	sc->sc_gc.gp_pin_write = argpio_write;
	sc->sc_gc.gp_pin_ctl = argpio_ctl;

	aprint_normal(": Atheros AR531X GPIO");
	pn = prop_dictionary_get(device_properties(sc->sc_dev), "reset-pin");
	if (pn != NULL) {
		KASSERT(prop_object_type(pn) == PROP_TYPE_NUMBER);
		sc->sc_rstpin = (int)prop_number_integer_value(pn);
		aprint_normal(", reset button pin %d", sc->sc_rstpin);
	}
	pn = prop_dictionary_get(device_properties(sc->sc_dev), "sysled-pin");
	if (pn != NULL) {
		KASSERT(prop_object_type(pn) == PROP_TYPE_NUMBER);
		sc->sc_ledpin = (int)prop_number_integer_value(pn);
		aprint_normal(", system led pin %d", sc->sc_ledpin);
	}

	aprint_normal("\n");

	if (sc->sc_ledpin) {
		sc->sc_ih = arbus_intr_establish(aa->aa_cirq, aa->aa_mirq,
		    argpio_intr, sc);
		if (sc->sc_ih == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "couldn't establish interrupt\n");
		}
	}

	if (sc->sc_ih) {
		sysmon_task_queue_init();

		sc->sc_resetbtn.smpsw_name = device_xname(sc->sc_dev);
		sc->sc_resetbtn.smpsw_type = PSWITCH_TYPE_RESET;
		if (sysmon_pswitch_register(&sc->sc_resetbtn) != 0) {
			aprint_normal_dev(sc->sc_dev,
			    "unable to register reset button\n");
		}
	}

	reg = GETREG(sc, GPIO_CR);

	for (i = 0; i < sc->sc_npins; i++) {
		gpio_pin_t	*pp;

		pp = &sc->sc_pins[i];

		if (i == sc->sc_rstpin) {
			/* configure as interrupt for reset */
			pp->pin_caps = GPIO_PIN_INPUT;
			reg &= ~SERIAL(i);
			reg |= INPUT(i);
			/* only if we were able to set up the handler, tho' */
			if (sc->sc_ih != NULL)
				reg |= INTR(i);

		} else if (i == sc->sc_ledpin) {
			/* configure as output for LED */
			pp->pin_caps = GPIO_PIN_OUTPUT;
			reg &= ~SERIAL(i);
			reg &= ~INPUT(i);
			reg &= ~INTR(i);

		} else {
			if (reg & SERIAL(i)) {
				/* pin multiplexed with serial bit */
				pp->pin_caps = 0;
			} else {
				pp->pin_caps = GPIO_PIN_INPUT |
				    GPIO_PIN_OUTPUT;
			}
		}
	}

	PUTREG(sc, GPIO_CR, reg);
	FLUSH(sc);

	gba.gba_gc = &sc->sc_gc;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = sc->sc_npins;
	config_found_ia(sc->sc_dev, "gpiobus", &gba, gpiobus_print);
}

void
argpio_ctl(void *arg, int pin, int flags)
{
	struct argpio_softc	*sc = arg;
	uint32_t		reg;

	reg = GETREG(sc, GPIO_CR);
	if (reg & (SERIAL(pin) | INTR(pin))) {
		printf("pin %d cannot be changed!\n", pin);
		/* don't allow changes to these bits */
		return;
	}
	if (flags & GPIO_PIN_INPUT) {
		reg |= INPUT(pin);
	} else {
		reg &= ~INPUT(pin);
	}
	
	PUTREG(sc, GPIO_CR, reg);
	FLUSH(sc);
}

void
argpio_write(void *arg, int pin, int value)
{
	struct argpio_softc	*sc = arg;
	uint32_t		reg;

	reg = GETREG(sc, GPIO_DO);
	if (value)
		reg &= ~(1 << pin);
	else
		reg |= (1 << pin);
	PUTREG(sc, GPIO_DO, reg);
	FLUSH(sc);
}

int
argpio_read(void *arg, int pin)
{
	struct argpio_softc	*sc = arg;

	return ((GETREG(sc, GPIO_DI) & (1 << pin)) ?
	    GPIO_PIN_HIGH : GPIO_PIN_LOW);
}

void
argpio_reset_pressed(void *arg)
{
	struct argpio_softc	*sc = arg;
	int			x;

	sysmon_pswitch_event(&sc->sc_resetbtn, PSWITCH_EVENT_PRESSED);

	/* reenable the interrupt */
	x = splhigh();
	PUTREG(sc, GPIO_CR,
	    GETREG(sc, GPIO_CR) | INTR(sc->sc_rstpin));
	splx(x);
}

int
argpio_intr(void  *arg)
{
	struct argpio_softc	*sc = arg;

	if (sc->sc_rstpin < 0)
		return 0;
	
	/* this is an edge triggered interrupt, so disable it for now */
	PUTREG(sc, GPIO_CR, GETREG(sc, GPIO_CR) & ~INTR(sc->sc_rstpin));

	/* no other interrupt on this, so we have to claim it */
	sysmon_task_queue_sched(0, argpio_reset_pressed, sc);

	return 1;
}
