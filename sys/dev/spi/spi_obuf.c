/*	$Id: spi_obuf.c,v 1.1.2.1 2007/11/10 02:57:04 matt Exp $	*/
/*	$NetBSD: spi_obuf.c,v 1.1.2.1 2007/11/10 02:57:04 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: spi_obuf.c,v 1.1.2.1 2007/11/10 02:57:04 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/lock.h>
#include <dev/spi/spivar.h>
#include <dev/gpio/gpiovar.h>
#include "gpio.h"
#if NGPIO > 0
#include <sys/gpio.h>
#endif
#include "locators.h"

#ifdef SPI_OBUF_DEBUG
int spi_obuf_debug = SPI_OBUF_DEBUG;
#define DPRINTFN(n,x)		if (spi_obuf_debug>(n)) printf x;
#else
#define DPRINTFN(n,x)
#endif

#define	SPI_OBUF_NPINS		16

struct spi_obuf_softc {
	struct device		sc_dev;
	struct spi_handle	*sc_sh;

#if NGPIO > 0
	struct gpio_chipset_tag	sc_gpio_chipset;
	gpio_pin_t		sc_pins[SPI_OBUF_NPINS];
#endif

	u_int32_t		sc_pinstate;
};

static int spi_obuf_match(struct device *, struct cfdata *, void *);
static void spi_obuf_attach(struct device *, struct device *, void *);

#if NGPIO > 0
static int spi_obufbus_print(void *, const char *);
static int spi_obuf_pin_read(void *, int);
static void spi_obuf_pin_write(void *, int, int);
static void spi_obuf_pin_ctl(void *, int, int);
#endif

#if 0
static int spi_obuf_search(struct device *, struct cfdata *, const int *, void *);
static int spi_obuf_print(void *, const char *);
#endif

CFATTACH_DECL(spi_obuf, sizeof(struct spi_obuf_softc),
	      spi_obuf_match, spi_obuf_attach, NULL, NULL);

static int
spi_obuf_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct spi_attach_args *sa = aux;

	if (spi_configure(sa->sa_handle, SPI_MODE_0, 10000000))
		return 0;

	return 2;
}

static void
spi_obuf_attach(struct device *parent, struct device *self, void *aux)
{
	struct spi_obuf_softc *sc = (struct spi_obuf_softc*)self;
	struct spi_attach_args *sa = aux;
#if NGPIO > 0
	struct gpiobus_attach_args gba;
	int n;
#endif

	aprint_naive(": output buffer\n");
	aprint_normal(": 74HC595 or compatible shift register(s)\n");

	sc->sc_sh = sa->sa_handle;
	sc->sc_pinstate = 0;

	if (spi_send(sc->sc_sh, 4, (const void *)&sc->sc_pinstate) != 0) {
		// @@@ failed @@@
		printf("spi_send() failed!\n");
	}

#if NGPIO > 0
	/* initialize and attach gpio(4) */
	for (n = 0; n < SPI_OBUF_NPINS; n++) {
		sc->sc_pins[n].pin_num = n;
		sc->sc_pins[n].pin_caps = (GPIO_PIN_OUTPUT
					   | GPIO_PIN_PUSHPULL);
		sc->sc_pins[n].pin_flags = GPIO_PIN_OUTPUT | GPIO_PIN_LOW;
	}

	sc->sc_gpio_chipset.gp_cookie = sc;
	sc->sc_gpio_chipset.gp_pin_read = spi_obuf_pin_read;
	sc->sc_gpio_chipset.gp_pin_write = spi_obuf_pin_write;
	sc->sc_gpio_chipset.gp_pin_ctl = spi_obuf_pin_ctl;
	gba.gba_gc = &sc->sc_gpio_chipset;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = SPI_OBUF_NPINS;
	config_found_ia(self, "gpiobus", &gba, spi_obufbus_print);
#endif

	/* attach device */
//	config_search_ia(spi_obuf_search, self, "spi_obuf", spi_obuf_print);
}

#if NGPIO > 0
static int
spi_obufbus_print(void *aux, const char *name)
{
//	struct gpiobus_attach_args *gba = aux;
//	struct spi_obuf_softc *sc = (struct spi_obuf_softc*)gba->gba_gc->gp_cookie;

	gpiobus_print(aux, name);

	return (UNCONF);
}
#endif

#if 0
static int
spi_obuf_search(struct device *parent, struct cfdata *cf,
	      const int *ldesc, void *aux)
{
	struct spi_obuf_softc *sc = (struct spi_obuf_softc*)parent;
	struct spi_obuf_attach_args ga;

	ga.ga_gc = sc;
	ga.ga_iot = sc->sc_iot;
	ga.ga_port = cf->cf_loc[SPI_OBUFCF_PORT];
	ga.ga_bit = cf->cf_loc[SPI_OBUFCF_BIT];

	if (config_match(parent, cf, &ga) > 0)
		config_attach(parent, cf, &ga, spi_obuf_print);

	return 0;
}

static int
spi_obuf_print(void *aux, const char *name)
{
	struct spi_obuf_attach_args *ga = (struct spi_obuf_attach_args*)aux;
	struct spi_obuf_softc *sc = (struct spi_obuf_softc*)ga->ga_gc;

	aprint_normal(":");
	if (ga->ga_port > -1)
		aprint_normal(" port %c", sc->sc_port[ga->ga_port].unit+'A');
	if (ga->ga_bit > -1)
		aprint_normal(" bit %d", ga->ga_bit);

	return (UNCONF);
}
#endif

#if 0
int
spi_obuf_read(struct spi_obuf_softc *sc, int bit)
{
#if NGPIO > 0
	sc->sc_pins[bit].pin_caps = 0;
#endif
	return (sc->sc_pinstate >> bit) & 1
}

void
spi_obuf_set(struct spi_obuf_softc *sc, spi_obuf_port port, int bit)
{
	struct port_info *pi = &sc->sc_port[port];

#if NGPIO > 0
	pi->pins[bit].pin_caps = 0;
#endif
	GPIO_WRITE(pi, PIO_SODR, (1U << bit));
}

void
spi_obuf_clear(struct spi_obuf_softc *sc, spi_obuf_port port, int bit)
{
	struct port_info *pi = &sc->sc_port[port];

#if NGPIO > 0
	pi->pins[bit].pin_caps = 0;
#endif
	GPIO_WRITE(pi, PIO_CODR, (1U << bit));
}

void
spi_obuf_in(struct spi_obuf_softc *sc, spi_obuf_port port, int bit)
{
	struct port_info *pi = &sc->sc_port[port];

#if NGPIO > 0
	pi->pins[bit].pin_caps = 0;
#endif
	GPIO_WRITE(pi, PIO_ODR, (1U << bit));
}

void
spi_obuf_out(struct spi_obuf_softc *sc, spi_obuf_port port, int bit)
{
	struct port_info *pi = &sc->sc_port[port];

#if NGPIO > 0
	pi->pins[bit].pin_caps = 0;
#endif
	GPIO_WRITE(pi, PIO_OER, (1U << bit));
}
#endif


#if NGPIO > 0
static int
spi_obuf_pin_read(void *arg, int pin)
{
	struct spi_obuf_softc *sc = arg;
	pin %= SPI_OBUF_NPINS;
	return (sc->sc_pinstate >> pin) & 1;
}

static void
spi_obuf_pin_write(void *arg, int pin, int val)
{
	struct spi_obuf_softc *sc = arg;
	u_int32_t sr;

	pin %= SPI_OBUF_NPINS;
	if (!sc->sc_pins[pin].pin_caps)
		return;

	if (val)
		sc->sc_pinstate |= 1U << pin;
	else
		sc->sc_pinstate &= ~(1U << pin);

	sr = htobe32(sc->sc_pinstate);
	if (spi_send(sc->sc_sh, 4, (const void *)&sr) != 0) {
		// @@@ do what? @@@
	}
}
	

static void
spi_obuf_pin_ctl(void *arg, int pin, int flags)
{
	struct spi_obuf_softc *sc = arg;

	pin %= SPI_OBUF_NPINS;
	if (!sc->sc_pins[pin].pin_caps)
		return;

	// hmm, I think there's nothing to do
}
#endif

