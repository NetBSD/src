/*	$NetBSD: gpio_opb.c,v 1.2 2005/08/26 13:19:37 drochner Exp $	*/

/*
 * Copyright (c) 2004 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/pio.h>

#include <powerpc/ibm4xx/dcr405gp.h>
#include <powerpc/ibm4xx/dev/opbvar.h>
#include <powerpc/ibm4xx/dev/gpioreg.h>
#include <powerpc/ibm4xx/dev/gpiovar.h>

struct gpio_softc {
	struct device		sc_dev;		/* device generic */
	struct gpio_controller	sc_gpio;	/* GPIO controller */
        u_long			sc_addr;	/* GPIO controller address */
};

static int	gpio_print(void *, const char *);
static int	gpio_search(struct device *, struct cfdata *,
			const int *ldesc, void *aux);
static int	gpio_match(struct device *, struct cfdata *, void *);
static void	gpio_attach(struct device *, struct device *, void *);

static int	gpio_or_read(void *, int);
static int	gpio_tcr_read(void *, int);
static int	gpio_odr_read(void *, int);
static int	gpio_ir_read(void *, int);
static void	gpio_or_write(void *arg, int addr, int bit);
static void	gpio_tcr_write(void *arg, int addr, int bit);
static void	gpio_odr_write(void *arg, int addr, int bit);

static int	gpio_read_bit(void *, int, int);
static void	gpio_write_bit(void *, int, int, int);
static uint32_t	gpio_read(void *, int);
static void	gpio_write(void *, int, uint32_t);

CFATTACH_DECL(gpio, sizeof(struct gpio_softc),
    gpio_match, gpio_attach, NULL, NULL);

static int
gpio_print(void *aux, const char *pnp)
{
	struct gpio_attach_args *gaa = aux;

	aprint_normal(" addr GPIO#%d", gaa->ga_addr);

	return (UNCONF);
}

static int
gpio_search(struct device *parent, struct cfdata *cf,
	const int *ldesc, void *aux)
{
	struct gpio_softc *sc = (void *)parent;
	struct gpio_attach_args gaa;

	gaa.ga_tag = &sc->sc_gpio;
	gaa.ga_addr = cf->cf_loc[GPIOCF_ADDR];

	if (config_match(parent, cf, &gaa) > 0)
		config_attach(parent, cf, &gaa, gpio_print);

	return (0);
}

static int
gpio_match(struct device *parent, struct cfdata *cf, void *args)
{
	struct opb_attach_args *oaa = args;

	if (strcmp(oaa->opb_name, cf->cf_name) != 0)
		return (0);

	return (1);
}

static void
gpio_attach(struct device *parent, struct device *self, void *aux)
{
	struct gpio_softc *sc = (struct gpio_softc *)self;
	struct opb_attach_args *oaa = aux;

	aprint_naive(": GPIO controller\n");
	aprint_normal(": On-Chip GPIO controller\n");

	sc->sc_addr = oaa->opb_addr;
	sc->sc_gpio.cookie = sc;
	sc->sc_gpio.io_or_read = gpio_or_read;
	sc->sc_gpio.io_tcr_read = gpio_tcr_read;
	sc->sc_gpio.io_odr_read = gpio_odr_read;
	sc->sc_gpio.io_ir_read = gpio_ir_read;
	sc->sc_gpio.io_or_write = gpio_or_write;
	sc->sc_gpio.io_tcr_write = gpio_tcr_write;
	sc->sc_gpio.io_odr_write = gpio_odr_write;

	(void) config_search_ia(gpio_search, self, "gpio", NULL);
}

static int
gpio_or_read(void *arg, int addr)
{
	return (gpio_read_bit(arg, GPIO_OR, addr));
}

static int
gpio_tcr_read(void *arg, int addr)
{
	return (gpio_read_bit(arg, GPIO_TCR, addr));
}

static int
gpio_odr_read(void *arg, int addr)
{
	return (gpio_read_bit(arg, GPIO_ODR, addr));
}

static int
gpio_ir_read(void *arg, int addr)
{
	gpio_write_bit(arg, GPIO_ODR, addr, 0);
	gpio_write_bit(arg, GPIO_TCR, addr, 0);
	return (gpio_read_bit(arg, GPIO_ODR, addr));
}

static void
gpio_or_write(void *arg, int addr, int bit)
{
	gpio_write_bit(arg, GPIO_ODR, addr, 0);
	gpio_write_bit(arg, GPIO_TCR, addr, 1);
	gpio_write_bit(arg, GPIO_OR, addr, bit);
}

static void
gpio_tcr_write(void *arg, int addr, int bit)
{
	gpio_write_bit(arg, GPIO_TCR, addr, bit);
}

static void
gpio_odr_write(void *arg, int addr, int bit)
{
	gpio_write_bit(arg, GPIO_ODR, addr, bit);
}

static int
gpio_read_bit(void *arg, int offset, int addr)
{
	uint32_t rv = gpio_read(arg, offset);
	uint32_t mask = GPIO_SBIT(addr);

	return ((rv & mask) >> GPIO_SHIFT(addr));
}

void
gpio_write_bit(void *arg, int offset, int addr, int bit)
{
	uint32_t rv = gpio_read(arg, offset);
	uint32_t mask = GPIO_SBIT(addr);

	rv = rv & ~mask;
	rv = rv | ((bit << GPIO_SHIFT(addr)) & mask);
	gpio_write(arg, offset, rv);
}

static uint32_t
gpio_read(void *arg, int offset)
{
	struct gpio_softc *sc = arg;
	uint32_t rv;

        rv = inl(sc->sc_addr + offset);

	return rv;
}

static void
gpio_write(void *arg, int offset, uint32_t out)
{
	struct gpio_softc *sc = arg;

        outl((sc->sc_addr + offset), out);
}
