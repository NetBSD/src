/*	$NetBSD: epgpio.c,v 1.3 2009/08/03 06:57:09 he Exp $	*/

/*
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
__KERNEL_RCSID(0, "$NetBSD: epgpio.c,v 1.3 2009/08/03 06:57:09 he Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>
#include <arm/ep93xx/ep93xxvar.h> 
#include <arm/ep93xx/epsocvar.h> 
#include <arm/ep93xx/epgpioreg.h>
#include <arm/ep93xx/epgpiovar.h>
#include "opt_ep93xx_gpio_mask.h"
#include "gpio.h"
#include "locators.h"

#ifdef EPGPIO_DEBUG
int epgpio_debug = EPGPIO_DEBUG;
#define DPRINTFN(n,x)	if (epgpio_debug>(n)) printf x;
#else
#define DPRINTFN(n,x)
#endif

#define	EPGPIO_NPORTS	8
#define	EPGPIO_NPINS	8

struct port_info {
	struct epgpio_softc	*sc;
	int			unit;
#if NGPIO > 0
	struct gpio_chipset_tag	gpio_chipset;
	gpio_pin_t		pins[EPGPIO_NPINS];
	int			gpio_mask;
	int			gpio_npins;
#endif
	bus_size_t		pxdr;
	bus_size_t		pxddr;
	bus_size_t		xinten;
	bus_size_t		xinttype1;
	bus_size_t		xinttype2;
	bus_size_t		xeoi;
	bus_size_t		xdb;
};

struct intr_req {
	int		irq;
	int		(*ih_func)(void *);
	int		(*ireq_func)(void *);
	void		*ireq_arg;
	void		*cookie;
};

struct epgpio_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct port_info	sc_port[EPGPIO_NPORTS];
	struct intr_req		sc_ireq_combine;
	struct intr_req		sc_ireq_f[EPGPIO_NPINS];
};

static int epgpio_match(struct device *, struct cfdata *, void *);
static void epgpio_attach(struct device *, struct device *, void *);

#if NGPIO > 0
static int epgpiobus_print(void *, const char *);
static int epgpio_pin_read(void *, int);
static void epgpio_pin_write(void *, int, int);
static void epgpio_pin_ctl(void *, int, int);
#endif

static int epgpio_search(struct device *, struct cfdata *, const int *, void *);
static int epgpio_print(void *, const char *);

static int epgpio_intr_combine(void* arg);
static int epgpio_intr_f(void* arg, int);
static int epgpio_intr_0(void* arg);
static int epgpio_intr_1(void* arg);
static int epgpio_intr_2(void* arg);
static int epgpio_intr_3(void* arg);
static int epgpio_intr_4(void* arg);
static int epgpio_intr_5(void* arg);
static int epgpio_intr_6(void* arg);
static int epgpio_intr_7(void* arg);

static void epgpio_bit_set(struct epgpio_softc *, bus_size_t, int);
static void epgpio_bit_clear(struct epgpio_softc *, bus_size_t, int);

CFATTACH_DECL(epgpio, sizeof(struct epgpio_softc),
	      epgpio_match, epgpio_attach, NULL, NULL);

static int
epgpio_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 2;
}

static void
epgpio_attach(struct device *parent, struct device *self, void *aux)
{
	struct epgpio_softc *sc = (struct epgpio_softc*)self;
	struct epsoc_attach_args *sa = aux;
	struct port_info *pi;
#if NGPIO > 0
	struct gpiobus_attach_args gba;
	int dir, val;
	int i, j, pin;
#endif

	printf("\n");
	sc->sc_iot = sa->sa_iot;

	if (bus_space_map(sa->sa_iot, sa->sa_addr,
			  sa->sa_size, 0, &sc->sc_ioh)){
		printf("%s: Cannot map registers", self->dv_xname);
		return;
	}

	/* PORT A */
	pi = &sc->sc_port[0];
	pi->unit = 0;
	pi->sc = sc;
	pi->pxdr = EP93XX_GPIO_PADR;
	pi->pxddr = EP93XX_GPIO_PADDR;
	pi->xinten = EP93XX_GPIO_AIntEn;
	pi->xinttype1 = EP93XX_GPIO_AIntType1;
	pi->xinttype2 = EP93XX_GPIO_AIntType2;
	pi->xeoi = EP93XX_GPIO_AEOI;
	pi->xdb = EP93XX_GPIO_ADB;
#if NGPIO > 0
	pi->gpio_mask = EPGPIO_PORT_A_MASK;
#endif
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, pi->xinten, 0);
	/* PORT B */
	pi = &sc->sc_port[1];
	pi->unit = 1;
	pi->sc = sc;
	pi->pxdr = EP93XX_GPIO_PBDR;
	pi->pxddr = EP93XX_GPIO_PBDDR;
	pi->xinten = EP93XX_GPIO_BIntEn;
	pi->xinttype1 = EP93XX_GPIO_BIntType1;
	pi->xinttype2 = EP93XX_GPIO_BIntType2;
	pi->xeoi = EP93XX_GPIO_BEOI;
	pi->xdb = EP93XX_GPIO_BDB;
#if NGPIO > 0
	pi->gpio_mask = EPGPIO_PORT_B_MASK;
#endif
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, pi->xinten, 0);
	/* PORT C */
	pi = &sc->sc_port[2];
	pi->unit = 2;
	pi->sc = sc;
	pi->pxdr = EP93XX_GPIO_PCDR;
	pi->pxddr = EP93XX_GPIO_PCDDR;
	pi->xinten = pi->xinttype1 = pi->xinttype2 = pi->xeoi = pi->xdb = -1;
#if NGPIO > 0
	pi->gpio_mask = EPGPIO_PORT_C_MASK;
#endif
	/* PORT D */
	pi = &sc->sc_port[3];
	pi->unit = 3;
	pi->sc = sc;
	pi->pxdr = EP93XX_GPIO_PDDR;
	pi->pxddr = EP93XX_GPIO_PDDDR;
	pi->xinten = pi->xinttype1 = pi->xinttype2 = pi->xeoi = pi->xdb = -1;
#if NGPIO > 0
	pi->gpio_mask = EPGPIO_PORT_D_MASK;
#endif
	/* PORT E */
	pi = &sc->sc_port[4];
	pi->unit = 4;
	pi->sc = sc;
	pi->pxdr = EP93XX_GPIO_PEDR;
	pi->pxddr = EP93XX_GPIO_PEDDR;
	pi->xinten = pi->xinttype1 = pi->xinttype2 = pi->xeoi = pi->xdb = -1;
#if NGPIO > 0
	pi->gpio_mask = EPGPIO_PORT_E_MASK;
#endif
	/* PORT F */
	pi = &sc->sc_port[5];
	pi->unit = 5;
	pi->sc = sc;
	pi->pxdr = EP93XX_GPIO_PFDR;
	pi->pxddr = EP93XX_GPIO_PFDDR;
	pi->xinten = EP93XX_GPIO_FIntEn;
	pi->xinttype1 = EP93XX_GPIO_FIntType1;
	pi->xinttype2 = EP93XX_GPIO_FIntType2;
	pi->xeoi = EP93XX_GPIO_FEOI;
	pi->xdb = EP93XX_GPIO_FDB;
#if NGPIO > 0
	pi->gpio_mask = EPGPIO_PORT_F_MASK;
#endif
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, pi->xinten, 0);
	/* PORT G */
	pi = &sc->sc_port[6];
	pi->unit = 6;
	pi->sc = sc;
	pi->pxdr = EP93XX_GPIO_PGDR;
	pi->pxddr = EP93XX_GPIO_PGDDR;
	pi->xinten = pi->xinttype1 = pi->xinttype2 = pi->xeoi = pi->xdb = -1;
#if NGPIO > 0
	pi->gpio_mask = EPGPIO_PORT_G_MASK;
#endif
	/* PORT H */
	pi = &sc->sc_port[7];
	pi->unit = 7;
	pi->sc = sc;
	pi->pxdr = EP93XX_GPIO_PHDR;
	pi->pxddr = EP93XX_GPIO_PHDDR;
	pi->xinten = pi->xinttype1 = pi->xinttype2 = pi->xeoi = pi->xdb = -1;
#if NGPIO > 0
	pi->gpio_mask = EPGPIO_PORT_H_MASK;
#endif

	/* PORT A & B */
	sc->sc_ireq_combine.irq = EP93XX_GPIO_INTR;
	sc->sc_ireq_combine.ih_func = epgpio_intr_combine;
	/* PORT F */
	sc->sc_ireq_f[0].irq = EP93XX_GPIO0_INTR;
	sc->sc_ireq_f[0].ih_func = epgpio_intr_0;
	sc->sc_ireq_f[1].irq = EP93XX_GPIO1_INTR;
	sc->sc_ireq_f[1].ih_func = epgpio_intr_1;
	sc->sc_ireq_f[2].irq = EP93XX_GPIO2_INTR;
	sc->sc_ireq_f[2].ih_func = epgpio_intr_2;
	sc->sc_ireq_f[3].irq = EP93XX_GPIO3_INTR;
	sc->sc_ireq_f[3].ih_func = epgpio_intr_3;
	sc->sc_ireq_f[4].irq = EP93XX_GPIO4_INTR;
	sc->sc_ireq_f[4].ih_func = epgpio_intr_4;
	sc->sc_ireq_f[5].irq = EP93XX_GPIO5_INTR;
	sc->sc_ireq_f[5].ih_func = epgpio_intr_5;
	sc->sc_ireq_f[6].irq = EP93XX_GPIO6_INTR;
	sc->sc_ireq_f[6].ih_func = epgpio_intr_6;
	sc->sc_ireq_f[7].irq = EP93XX_GPIO7_INTR;
	sc->sc_ireq_f[7].ih_func = epgpio_intr_7;

#if NGPIO > 0
	/* initialize and attach gpio(4) */
	for (i = 0; i < EPGPIO_NPORTS; i++) {
		pi = &sc->sc_port[i];
		/*
		 * If this port is completely disabled for gpio attachment,
		 * then skip it.
		 */
		if (pi->gpio_mask == 0x00)
			continue;

		dir = bus_space_read_4(sc->sc_iot, sc->sc_ioh, pi->pxddr) & 0xff;
		val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, pi->pxdr) & 0xff;

		/*
		 * pin_num doesn't seem to be used for anything in the GPIO
		 * code.  So we're going to use it to refer to the REAL pin
		 * on the port.  Just to keep things straight below:
		 *
		 * pin - The pin number as seen by the GPIO code
		 * j   - The ACTUAL pin on the port
		 */

		for (j = 0, pin = 0; j < EPGPIO_NPINS; j++) {
			if (pi->gpio_mask & (1 << j)) {
				pi->pins[pin].pin_num = j;
				pi->pins[pin].pin_caps = (GPIO_PIN_INPUT
							| GPIO_PIN_OUTPUT);
				if((dir >> j) & 0x01)
					pi->pins[pin].pin_flags =
							GPIO_PIN_OUTPUT;
				else
					pi->pins[pin].pin_flags =
						GPIO_PIN_INPUT;
				if((val >> j) & 0x01)
					pi->pins[pin].pin_state = GPIO_PIN_HIGH;
				else
					pi->pins[pin].pin_state = GPIO_PIN_LOW;
				pin++;
			}
		}
		pi->gpio_chipset.gp_cookie = pi;
		pi->gpio_chipset.gp_pin_read = epgpio_pin_read;
		pi->gpio_chipset.gp_pin_write = epgpio_pin_write;
		pi->gpio_chipset.gp_pin_ctl = epgpio_pin_ctl;
		gba.gba_gc = &pi->gpio_chipset;
		gba.gba_pins = pi->pins;
		gba.gba_npins = pin;
		config_found_ia(self, "gpiobus", &gba, epgpiobus_print);
	}
#endif

	/* attach device */
	config_search_ia(epgpio_search, self, "epgpio", epgpio_print);
}

#if NGPIO > 0
static int
epgpiobus_print(void *aux, const char *name)
{
	struct gpiobus_attach_args *gba = aux;
	struct port_info *pi = (struct port_info *)gba->gba_gc->gp_cookie;

	gpiobus_print(aux, name);
	aprint_normal(": port %c", pi->unit+'A');

	return (UNCONF);
}
#endif


static int
epgpio_search(struct device *parent, struct cfdata *cf,
	      const int *ldesc, void *aux)
{
	struct epgpio_softc *sc = (struct epgpio_softc*)parent;
	struct epgpio_attach_args ga;

	ga.ga_gc = sc;
	ga.ga_iot = sc->sc_iot;
	ga.ga_port = cf->cf_loc[EPGPIOCF_PORT];
	ga.ga_bit1 = cf->cf_loc[EPGPIOCF_BIT1];
	ga.ga_bit2 = cf->cf_loc[EPGPIOCF_BIT2];

	if (config_match(parent, cf, &ga) > 0)
		config_attach(parent, cf, &ga, epgpio_print);

	return 0;
}

static int
epgpio_print(void *aux, const char *name)
{
	struct epgpio_attach_args *ga = (struct epgpio_attach_args*)aux;
	struct epgpio_softc *sc = (struct epgpio_softc*)ga->ga_gc;

	aprint_normal(":");
	if (ga->ga_port > -1)
		aprint_normal(" port %c", sc->sc_port[ga->ga_port].unit+'A');
	if (ga->ga_bit1 > -1)
		aprint_normal(" bit1 %d", ga->ga_bit1);
	if (ga->ga_bit2 > -1)
		aprint_normal(" bit2 %d", ga->ga_bit2);

	return (UNCONF);
}

int
epgpio_read(struct epgpio_softc *sc, epgpio_port port, int bit)
{
	struct port_info *pi = &sc->sc_port[port];

#if NGPIO > 0
	pi->pins[bit].pin_caps = 0;
#endif
	return (bus_space_read_4(sc->sc_iot, sc->sc_ioh, pi->pxdr) >> bit) & 1;
}

void
epgpio_set(struct epgpio_softc *sc, epgpio_port port, int bit)
{
	struct port_info *pi = &sc->sc_port[port];

#if NGPIO > 0
	pi->pins[bit].pin_caps = 0;
#endif
	epgpio_bit_set(sc, pi->pxdr, bit);
}

void
epgpio_clear(struct epgpio_softc *sc, epgpio_port port, int bit)
{
	struct port_info *pi = &sc->sc_port[port];

#if NGPIO > 0
	pi->pins[bit].pin_caps = 0;
#endif
	epgpio_bit_clear(sc, pi->pxdr, bit);
}

void
epgpio_in(struct epgpio_softc *sc, epgpio_port port, int bit)
{
	struct port_info *pi = &sc->sc_port[port];

#if NGPIO > 0
	pi->pins[bit].pin_caps = 0;
#endif
	epgpio_bit_clear(sc, pi->pxddr, bit);
}

void
epgpio_out(struct epgpio_softc *sc, epgpio_port port, int bit)
{
	struct port_info *pi = &sc->sc_port[port];

#if NGPIO > 0
	pi->pins[bit].pin_caps = 0;
#endif
	epgpio_bit_set(sc, pi->pxddr, bit);
}

void *
epgpio_intr_establish(struct epgpio_softc *sc, epgpio_port port, int bit,
		      int flag, int ipl, int (*ireq_func)(void *), void *arg) {
	struct port_info *pi;
	struct intr_req *intq;

	DPRINTFN(1, ("epgpio_intr_establish: port=%d, bit=%d, flag=%#x\n",port,bit,flag));

	if (bit < 0 || bit >= EPGPIO_NPINS)
		return 0;

	switch (port) {
	case PORT_A:
	case PORT_B:
		intq = &sc->sc_ireq_combine;
		break;
	case PORT_F:
		intq = &sc->sc_ireq_f[bit];
		break;
	default:
		return 0;
	};

	if (intq->ireq_func)
		return 0;	/* already used */

	intq->ireq_func = ireq_func;
	intq->ireq_arg = arg;

	pi = &sc->sc_port[port];
	epgpio_bit_clear(sc, pi->xinten, bit);
	epgpio_in(sc, port, bit);
#if NGPIO > 0
	pi->pins[bit].pin_caps = 0;
#endif

	if (flag & EDGE_TRIGGER)
		epgpio_bit_set(sc, pi->xinttype1, bit);
	else	/* LEVEL_SENSE */
		epgpio_bit_clear(sc, pi->xinttype1, bit);
	if (flag & RISING_EDGE)	/* or HIGH_LEVEL */
		epgpio_bit_set(sc, pi->xinttype2, bit);
	else	/* FALLING_EDGE or LOW_LEVEL */
		epgpio_bit_clear(sc, pi->xinttype2, bit);
	if (flag & DEBOUNCE)
		epgpio_bit_set(sc, pi->xdb, bit);
	else
		epgpio_bit_clear(sc, pi->xdb, bit);

	if (!intq->cookie)
		intq->cookie = ep93xx_intr_establish(intq->irq, ipl,
						     intq->ih_func, pi);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, pi->xeoi, 1 << bit);
	epgpio_bit_set(sc, pi->xinten, bit);
	return intq->cookie;
}

void
epgpio_intr_disestablish(struct epgpio_softc *sc, epgpio_port port, int bit)
{
	struct port_info *pi;
	struct intr_req *intq;

	DPRINTFN(1, ("epgpio_intr_disestablish: port=%d, bit=%d\n",port,bit));

	if (bit < 0 || bit >= EPGPIO_NPINS)
		return;

	switch (port) {
	case PORT_A:
	case PORT_B:
		intq = &sc->sc_ireq_combine;
		break;
	case PORT_F:
		intq = &sc->sc_ireq_f[bit];
		break;
	default:
		return;
	};

	if (!intq->ireq_func)
		return;

	pi = &sc->sc_port[port];
	epgpio_bit_clear(sc, pi->xinten, bit);
	intq->ireq_func = 0;
	intq->ireq_arg = 0;
	ep93xx_intr_disestablish(intq->cookie);
	intq->cookie = 0;
}

static int
epgpio_intr_combine(void *arg)
{
	struct port_info *pi = arg;
	struct epgpio_softc *sc = pi->sc;
	struct intr_req *intq = &sc->sc_ireq_combine;
	int err = 0;

	DPRINTFN(1, ("epgpio_intr_combine\n"));

	if (intq->ireq_func)
		err = (*intq->ireq_func)(intq->ireq_arg);
	epgpio_bit_set(sc, pi->xeoi, 0xff);
	return err;
}

static int
epgpio_intr_f(void *arg, int bit)
{
	struct port_info *pi = arg;
	struct epgpio_softc *sc = pi->sc;
	struct intr_req *intq = &sc->sc_ireq_f[bit];
	int err = 0;

	DPRINTFN(1, ("epgpio_intr_%d\n", bit));

	if (intq->ireq_func)
		err = (*intq->ireq_func)(intq->ireq_arg);
	epgpio_bit_set(sc, pi->xeoi, bit);
	return err;
}

static int
epgpio_intr_0(void *arg)
{
	return epgpio_intr_f(arg, 0);
}

static int
epgpio_intr_1(void *arg)
{
	return epgpio_intr_f(arg, 1);
}

static int
epgpio_intr_2(void *arg)
{
	return epgpio_intr_f(arg, 2);
}

static int
epgpio_intr_3(void *arg)
{
	return epgpio_intr_f(arg, 3);
}

static int
epgpio_intr_4(void *arg)
{
	return epgpio_intr_f(arg, 4);
}

static int
epgpio_intr_5(void *arg)
{
	return epgpio_intr_f(arg, 5);
}

static int
epgpio_intr_6(void *arg)
{
	return epgpio_intr_f(arg, 6);
}

static int
epgpio_intr_7(void *arg)
{
	return epgpio_intr_f(arg, 7);
}

#if NGPIO > 0
static int
epgpio_pin_read(void *arg, int pin)
{
	struct port_info *pi = arg;
	struct epgpio_softc *sc = pi->sc;

	pin %= pi->gpio_npins;
	if (!pi->pins[pin].pin_caps)
		return 0; /* EBUSY? */

	return (bus_space_read_4(sc->sc_iot, sc->sc_ioh,
				 pi->pxdr) >> pi->pins[pin].pin_num) & 1;
}

static void
epgpio_pin_write(void *arg, int pin, int val)
{
	struct port_info *pi = arg;
	struct epgpio_softc *sc = pi->sc;

	pin %= pi->gpio_npins;
	if (!pi->pins[pin].pin_caps)
		return;

	if (val)
		epgpio_bit_set(sc, pi->pxdr, pi->pins[pin].pin_num);
	else
		epgpio_bit_clear(sc, pi->pxdr, pi->pins[pin].pin_num);
}

static void
epgpio_pin_ctl(void *arg, int pin, int flags)
{
	struct port_info *pi = arg;
	struct epgpio_softc *sc = pi->sc;

	pin %= pi->gpio_npins;
	if (!pi->pins[pin].pin_caps)
		return;

	if (flags & GPIO_PIN_INPUT)
		epgpio_bit_clear(sc, pi->pxddr, pi->pins[pin].pin_num);
	else if (flags & GPIO_PIN_OUTPUT)
		epgpio_bit_set(sc, pi->pxddr, pi->pins[pin].pin_num);
}
#endif

static void
epgpio_bit_set(struct epgpio_softc *sc, bus_size_t reg, int bit)
{
	int t = bus_space_read_4(sc->sc_iot, sc->sc_ioh, reg) & 0xff;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, reg, t | (1 << bit));
}

static void
epgpio_bit_clear(struct epgpio_softc *sc, bus_size_t reg, int bit)
{
	int t = bus_space_read_4(sc->sc_iot, sc->sc_ioh, reg) & 0xff;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, reg, t & ~(1 << bit));
}
