/*	$Id: at91pio.c,v 1.4.2.2 2013/01/16 05:32:45 yamt Exp $	*/
/*	$NetBSD: at91pio.c,v 1.4.2.2 2013/01/16 05:32:45 yamt Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy. All rights reserved.
 *
 * Based on arch/arm/ep93xx/epgpio.c,
 * Copyright (c) 2005 HAMAJIMA Katsuomi. All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: at91pio.c,v 1.4.2.2 2013/01/16 05:32:45 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/bus.h>
#include <machine/intr.h>
#include <dev/gpio/gpiovar.h>
#include <arm/at91/at91var.h>
#include <arm/at91/at91reg.h>
#include <arm/at91/at91pioreg.h>
#include <arm/at91/at91piovar.h>
#include "gpio.h"
#if NGPIO > 0
#include <sys/gpio.h>
#endif
#include "locators.h"

#ifdef AT91PIO_DEBUG
int at91pio_debug = AT91PIO_DEBUG;
#define DPRINTFN(n,x)	if (at91pio_debug>(n)) printf x;
#else
#define DPRINTFN(n,x)
#endif

#define	AT91PIO_NMAXPORTS	4
#define	AT91PIO_NPINS		32

struct intr_req {
	int			(*ireq_func)(void *);
	void			*ireq_arg;
	int			ireq_ipl;
};

#define	PIO_READ(_sc, _reg)		bus_space_read_4((_sc)->sc_iot, (_sc)->sc_ioh, (_reg))
#define	PIO_WRITE(_sc, _reg, _val)	bus_space_write_4((_sc)->sc_iot, (_sc)->sc_ioh, (_reg), (_val))

struct at91pio_softc {
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	int			sc_pid;
#if NGPIO > 0
	struct gpio_chipset_tag	gpio_chipset;
	gpio_pin_t		pins[AT91PIO_NPINS];
#endif
	int			irq;
	void			*ih;
	struct intr_req		ireq[AT91PIO_NPINS];
};

static int at91pio_match(device_t, cfdata_t, void *);
static void at91pio_attach(device_t, device_t, void *);

#if NGPIO > 0
static int at91piobus_print(void *, const char *);
static int at91pio_pin_read(void *, int);
static void at91pio_pin_write(void *, int, int);
static void at91pio_pin_ctl(void *, int, int);
#endif

static int at91pio_search(device_t, cfdata_t, const int *, void *);
static int at91pio_print(void *, const char *);

static int at91pio_intr(void* arg);

CFATTACH_DECL_NEW(at91pio, sizeof(struct at91pio_softc),
	      at91pio_match, at91pio_attach, NULL, NULL);

static struct at91pio_softc *at91pio_softc[AT91_PIO_COUNT];

struct at91pio_softc *at91pio_sc(at91pio_port port)
{
	if (port < AT91_PIO_COUNT)
		return at91pio_softc[port];
	return NULL;
}


static int
at91pio_match(device_t parent, cfdata_t match, void *aux)
{
	if (strcmp(match->cf_name, "at91pio") == 0)
		return 2;
	return 0;
}

static void
at91pio_attach(device_t parent, device_t self, void *aux)
{
	struct at91pio_softc *sc = device_private(self);
	struct at91bus_attach_args *sa = aux;
#if NGPIO > 0
	struct gpiobus_attach_args gba;
	uint32_t psr, osr, pin;
	int j, n;
#endif
	printf("\n");
	sc->sc_iot = sa->sa_iot;
	sc->sc_pid = sa->sa_pid;

	if (bus_space_map(sa->sa_iot, sa->sa_addr,
			  sa->sa_size, 0, &sc->sc_ioh)){
		printf("%s: Cannot map registers", device_xname(self));
		return;
	}

	/* save descriptor: */
	at91pio_port p = at91_pio_port(sa->sa_pid);
	if (p < AT91_PIO_COUNT && !at91pio_softc[p])
		at91pio_softc[p] = sc;

	/* make sure peripheral is enabled: */
	at91_peripheral_clock(sc->sc_pid, 1);

	/* initialize ports (disable interrupts) */
	PIO_WRITE(sc, PIO_IDR, -1);

#if NGPIO > 0
	/* initialize and attach gpio(4) */
	psr = PIO_READ(sc, PIO_PSR);	// only ports 
	osr = PIO_READ(sc, PIO_OSR);
	pin = PIO_READ(sc, PIO_PDSR);
	psr &= ~at91_gpio_mask(sc->sc_pid);
	for (j = n = 0; j < AT91PIO_NPINS; j++) {
		sc->pins[n].pin_num = j;
		if (psr & (1 << j))
			sc->pins[n].pin_caps = (GPIO_PIN_INPUT
						| GPIO_PIN_OUTPUT
						| GPIO_PIN_OPENDRAIN // @@@ not all pins
						| GPIO_PIN_PUSHPULL
						| GPIO_PIN_PULLUP);
		else
			sc->pins[n].pin_caps = 0;

		if (osr & (1 << j))
			sc->pins[n].pin_flags = GPIO_PIN_OUTPUT;
		else
			sc->pins[n].pin_flags = GPIO_PIN_INPUT;
		if (pin & (1 << j))
			sc->pins[n].pin_state = GPIO_PIN_HIGH;
		else
			sc->pins[n].pin_state = GPIO_PIN_LOW;
		n++;
	}
	sc->gpio_chipset.gp_cookie = sc;
	sc->gpio_chipset.gp_pin_read = at91pio_pin_read;
	sc->gpio_chipset.gp_pin_write = at91pio_pin_write;
	sc->gpio_chipset.gp_pin_ctl = at91pio_pin_ctl;
	gba.gba_gc = &sc->gpio_chipset;
	gba.gba_pins = sc->pins;
	gba.gba_npins = n;
	config_found_ia(self, "gpiobus", &gba, at91piobus_print);
#endif

	/* attach device */
	config_search_ia(at91pio_search, self, "at91pio", at91pio_print);
}

#if NGPIO > 0
static int
at91piobus_print(void *aux, const char *name)
{
	struct gpiobus_attach_args *gba = aux;
	struct at91pio_softc *sc = (struct at91pio_softc *)gba->gba_gc->gp_cookie;

	gpiobus_print(aux, name);
	aprint_normal(": port %s (mask %08"PRIX32")",
		      at91_peripheral_name(sc->sc_pid),
		      at91_gpio_mask(sc->sc_pid));

	return (UNCONF);
}
#endif


static int
at91pio_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct at91pio_softc *sc = device_private(parent);
	struct at91pio_attach_args paa;

	paa.paa_sc = sc;
	paa.paa_iot = sc->sc_iot;
	paa.paa_pid = cf->cf_loc[AT91PIOCF_PID];
	paa.paa_bit = cf->cf_loc[AT91PIOCF_BIT];

	if (config_match(parent, cf, &paa) > 0)
		config_attach(parent, cf, &paa, at91pio_print);

	return 0;
}

static int
at91pio_print(void *aux, const char *name)
{
	struct at91pio_attach_args *paa = aux;

	aprint_normal(":");
	if (paa->paa_pid > -1)
		aprint_normal(" port %s", at91_peripheral_name(paa->paa_pid));
	if (paa->paa_bit > -1)
		aprint_normal(" bit %d", paa->paa_bit);

	return (UNCONF);
}

int
at91pio_read(struct at91pio_softc *sc, int bit)
{
#if NGPIO > 0
	sc->pins[bit].pin_caps = 0;
#endif
	return (PIO_READ(sc, PIO_PDSR) >> bit) & 1;
}

void
at91pio_set(struct at91pio_softc *sc, int bit)
{
#if NGPIO > 0
	sc->pins[bit].pin_caps = 0;
#endif
	PIO_WRITE(sc, PIO_SODR, (1U << bit));
}

void
at91pio_clear(struct at91pio_softc *sc, int bit)
{
#if NGPIO > 0
	sc->pins[bit].pin_caps = 0;
#endif
	PIO_WRITE(sc, PIO_CODR, (1U << bit));
}

void
at91pio_in(struct at91pio_softc *sc, int bit)
{
#if NGPIO > 0
	sc->pins[bit].pin_caps = 0;
#endif
	PIO_WRITE(sc, PIO_ODR, (1U << bit));
}

void
at91pio_out(struct at91pio_softc *sc, int bit)
{
#if NGPIO > 0
	sc->pins[bit].pin_caps = 0;
#endif
	PIO_WRITE(sc, PIO_OER, (1U << bit));
}

void at91pio_per(struct at91pio_softc *sc, int bit, int perab)
{
#if NGPIO > 0
	sc->pins[bit].pin_caps = 0;
#endif
	switch (perab) {
	case -1:
		PIO_WRITE(sc, PIO_PER, (1U << bit));
		break;
	case 0:
		PIO_WRITE(sc, PIO_ASR, (1U << bit));
		PIO_WRITE(sc, PIO_PDR, (1U << bit));
		break;
	case 1:
		PIO_WRITE(sc, PIO_BSR, (1U << bit));
		PIO_WRITE(sc, PIO_PDR, (1U << bit));
		break;
	default:
		panic("%s: perab is invalid: %i", __FUNCTION__, perab);
		break;
	}
}

void *
at91pio_intr_establish(struct at91pio_softc *sc, int bit,
			 int ipl, int (*ireq_func)(void *), void *arg)
{
	struct intr_req *ireq;

	DPRINTFN(1, ("at91pio_intr_establish: port=%s, bit=%d\n", at91_peripheral_name(sc->sc_pid), bit));

	if (bit < 0 || bit >= AT91PIO_NPINS)
		return 0;

	ireq = &sc->ireq[bit];

	if (ireq->ireq_func)	/* already used */
		return 0;

	ireq->ireq_func = ireq_func;
	ireq->ireq_arg = arg;
	ireq->ireq_ipl = ipl;

	PIO_WRITE(sc, PIO_IDR, (1U << bit));	/* disable interrupt for now */
	at91pio_in(sc, bit);			/* make sure pin is input */
#if NGPIO > 0
	sc->pins[bit].pin_caps = 0;
#endif
#if 0
	if (flag & EDGE_TRIGGER)
		at91pio_bit_set(sc, sc->xinttype1, bit);
	else	/* LEVEL_SENSE */
		at91pio_bit_clear(sc, sc->xinttype1, bit);
	if (flag & RISING_EDGE)	/* or HIGH_LEVEL */
		at91pio_bit_set(sc, sc->xinttype2, bit);
	else	/* FALLING_EDGE or LOW_LEVEL */
		at91pio_bit_clear(sc, sc->xinttype2, bit);
	if (flag & DEBOUNCE)
		PIO_WRITE(sc, PIO_IFER, (1U << bit));
	else
		PIO_WRITE(sc, PIO_IFDR, (1U << bit));
#endif

	if (!sc->ih) {
		// use IPL_BIO because we want lowest possible priority as
		// we really don't know what priority is going to be used by
		// the caller.. this is not really optimal but tell me a
		// better way
		sc->ih = at91_intr_establish(sc->sc_pid, IPL_BIO, INTR_HIGH_LEVEL,
					      at91pio_intr, sc);
	}

	//(void)PIO_READ(sc, PIO_ISR);	// clear interrupts
	PIO_WRITE(sc, PIO_IER, (1U << bit));	// enable interrupt

	return sc->ih;
}

void
at91pio_intr_disestablish(struct at91pio_softc *sc, int bit, void *cookie)
{
	struct intr_req *ireq;
	int i;

	DPRINTFN(1, ("at91pio_intr_disestablish: port=%s, bit=%d\n", at91_peripheral_name(sc->sc_pid), bit));

	if (bit < 0 || bit >= AT91PIO_NPINS)
		return;

if (cookie != sc->ih)
		return;

	ireq = &sc->ireq[bit];

	if (!ireq->ireq_func)
		return;

	PIO_WRITE(sc, PIO_IDR, (1U << bit));
	ireq->ireq_func = 0;
	ireq->ireq_arg = 0;

	for (i = 0; i < AT91PIO_NPINS; i++) {
		if (sc->ireq[i].ireq_func)
			break;
	}

	if (i >= AT91PIO_NPINS) {
		at91_intr_disestablish(sc->ih);
		sc->ih = 0;
	}
}

static int
at91pio_intr(void *arg)
{
	struct at91pio_softc *sc = arg;
	int bit;
	uint32_t isr;

	isr = (PIO_READ(sc, PIO_ISR) & PIO_READ(sc, PIO_IMR));
	if (!isr)
		return 0;

	do {
		bit = ffs(isr) - 1;
		isr &= ~(1U << bit);
#ifdef	DIAGNOSTIC
		if (bit < 0)
			panic("%s: isr is zero (0x%X)", __FUNCTION__, isr);
#endif
		if (sc->ireq[bit].ireq_func) {
			int s = _splraise(sc->ireq[bit].ireq_ipl);
			(*sc->ireq[bit].ireq_func)(sc->ireq[bit].ireq_arg);
			splx(s);
		}
	} while (isr);

	return 1;
}


#if NGPIO > 0
static int
at91pio_pin_read(void *arg, int pin)
{
	struct at91pio_softc *sc = arg;

	pin %= AT91PIO_NPINS;
	if (!sc->pins[pin].pin_caps)
		return 0; /* EBUSY? */

	return (PIO_READ(sc, PIO_PDSR) >> pin) & 1;
}

static void
at91pio_pin_write(void *arg, int pin, int val)
{
	struct at91pio_softc *sc = arg;

	pin %= AT91PIO_NPINS;
	if (!sc->pins[pin].pin_caps)
		return;

	if (val)
		PIO_WRITE(sc, PIO_SODR, (1U << pin));
	else
		PIO_WRITE(sc, PIO_CODR, (1U << pin));
}

static void
at91pio_pin_ctl(void *arg, int pin, int flags)
{
	struct at91pio_softc *sc = arg;

	pin %= AT91PIO_NPINS;
	if (!sc->pins[pin].pin_caps)
		return;

	if (flags & GPIO_PIN_INPUT)
		PIO_WRITE(sc, PIO_ODR, (1U << pin));
	else if (flags & GPIO_PIN_OUTPUT)
		PIO_WRITE(sc, PIO_OER, (1U << pin));

	if (flags & GPIO_PIN_OPENDRAIN)
		PIO_WRITE(sc, PIO_MDER, (1U << pin));
	else if (flags & GPIO_PIN_PUSHPULL) 
		PIO_WRITE(sc, PIO_MDDR, (1U << pin));

	if (flags & GPIO_PIN_PULLUP)
		PIO_WRITE(sc, PIO_PUER, (1U << pin));
	else
		PIO_WRITE(sc, PIO_PUDR, (1U << pin));
}
#endif

