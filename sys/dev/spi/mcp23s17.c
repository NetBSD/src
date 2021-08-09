/*      $NetBSD: mcp23s17.c,v 1.4.2.1 2021/08/09 00:30:09 thorpej Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank Kardel.
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
__KERNEL_RCSID(0, "$NetBSD: mcp23s17.c,v 1.4.2.1 2021/08/09 00:30:09 thorpej Exp $");

/* 
 * Driver for Microchip MCP23S17 GPIO
 *
 * see: http://ww1.microchip.com/downloads/en/DeviceDoc/21952b.pdf
 */

#include "gpio.h"
#include "opt_fdt.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/types.h>
#include <sys/gpio.h>
#include <sys/sysctl.h>

#include <dev/gpio/gpiovar.h>

#include <dev/spi/spivar.h>
#include <dev/spi/mcp23s17.h>

#ifdef FDT
#include <dev/fdt/fdtvar.h>
#endif /* FDT */

/* #define MCP23S17_DEBUG */
#ifdef MCP23S17_DEBUG
int mcp23S17debug = 3;
#define DPRINTF(l, x)	do { if (l <= mcp23S17debug) { printf x; } } while (0)
#else
#define DPRINTF(l, x)
#endif

struct mcp23s17gpio_softc {
	device_t                sc_dev;
	struct spi_handle      *sc_sh; 
	uint8_t                 sc_ha;   /* hardware address */
	uint8_t                 sc_bank; /* addressing scheme */
	struct gpio_chipset_tag	sc_gpio_gc;
	gpio_pin_t		sc_gpio_pins[MCP23x17_GPIO_NPINS];
};

static int	mcp23s17gpio_match(device_t, cfdata_t, void *);
static void	mcp23s17gpio_attach(device_t, device_t, void *);

static void     mcp23s17gpio_write(struct mcp23s17gpio_softc *, uint8_t, uint8_t);

#if NGPIO > 0
static uint8_t  mcp23s17gpio_read(struct mcp23s17gpio_softc *, uint8_t);

static int      mcp23s17gpio_gpio_pin_read(void *, int);
static void     mcp23s17gpio_gpio_pin_write(void *, int, int);
static void     mcp23s17gpio_gpio_pin_ctl(void *, int, int);
#endif

CFATTACH_DECL_NEW(mcp23s17gpio, sizeof(struct mcp23s17gpio_softc),
		  mcp23s17gpio_match, mcp23s17gpio_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "mcp,mcp23s17" },
	{ .compat = "microchip,mcp23s17" },

#if 0	/* We should also add support for these: */
	{ .compat = "mcp,mcp23s08" },
	{ .compat = "microchip,mcp23s08" },

	{ .compat = "microchip,mcp23s18" },
#endif

	DEVICE_COMPAT_EOL
};

static int
mcp23s17gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct spi_attach_args *sa = aux;

	return spi_compatible_match(sa, cf, compat_data);
}

#ifdef FDT
static bool
mcp23s17gpio_ha_fdt(struct mcp23s17gpio_softc *sc)
{
	devhandle_t devhandle = device_handle(sc->sc_dev);
	int phandle = devhandle_to_of(devhandle);
	uint32_t mask;
	int count;

	/*
	 * The number of devices sharing this chip select,
	 * along with their assigned addresses, is encoded
	 * in the "microchip,spi-present-mask" property.
	 *
	 * N.B. we also check for "mcp,spi-present-mask" if
	 * the first one isn't present (it's a deprecated
	 * property that may be present in older device trees).
	 */
	if (of_getprop_uint32(phandle, "microchip,spi-present-mask",
			      &mask) != 0 ||
	    of_getprop_uint32(phandle, "mcp,spi-present-mask",
			      &mask) != 0) {
		aprint_error(
		    ": missing \"microchip,spi-present-mask\" property\n");
		return false;
	}

	/*
	 * If we ever support the mcp23s08, then only bits 0-3 are valid
	 * on that device.
	 */
	if (mask == 0 || mask > __BITS(0,7)) {
		aprint_error(
		    ": invalid \"microchip,spi-present-mask\" property\n");
		return false;
	}

	count = popcount32(mask);
	if (count > 1) {
		/*
		 * XXX We only support a single chip on this chip
		 * select for now.
		 */
		aprint_error(": unsupported %d-chip configuration\n", count);
		return false;
	}

	sc->sc_ha = ffs(mask) - 1;
	KASSERT(sc->sc_ha >= 0 && sc->sc_ha <= 7);

	return true;
}
#endif /* FDT */

static void
mcp23s17gpio_attach(device_t parent, device_t self, void *aux)
{
	struct mcp23s17gpio_softc *sc;
	struct spi_attach_args *sa;
	int error;
#if NGPIO > 0
	int i;
	struct gpiobus_attach_args gba;
#endif

	sa = aux;
	sc = device_private(self);
	sc->sc_dev = self;
	sc->sc_sh = sa->sa_handle;
	sc->sc_bank = 0;

	/*
	 * The MCP23S17 can multiplex multiple chips on the same
	 * chip select.
	 *
	 * If we got here using indirect configuration, our kernel
	 * config file directive has our address.  Otherwise, we need
	 * to consult the device tree used for direct configuration.
	 */
	if (sa->sa_name == NULL) {
		sc->sc_ha = device_cfdata(sc->sc_dev)->cf_flags & 0x7;
	} else {
		devhandle_t devhandle = device_handle(self);

		switch (devhandle_type(devhandle)) {
#ifdef FDT
		case DEVHANDLE_TYPE_OF:
			if (! mcp23s17gpio_ha_fdt(sc)) {
				/* Error alredy displayed. */
				return;
			}
			break;
#endif /* FDT */
		default:
			aprint_error(": unsupported device handle type\n");
			return;
		}
	}

	aprint_naive(": GPIO\n");	
	aprint_normal(": MCP23S17 GPIO (ha=%d)\n", sc->sc_ha);

	/* run at 10MHz */
	error = spi_configure(sa->sa_handle, SPI_MODE_0, 10000000);
	if (error) {
		aprint_error_dev(self, "spi_configure failed (error = %d)\n",
		    error);
		return;
	}

	DPRINTF(1, ("%s: initialize (HAEN|SEQOP)\n", device_xname(sc->sc_dev)));

	/* basic setup */
	mcp23s17gpio_write(sc, MCP23x17_IOCONA(sc->sc_bank),
	    MCP23x17_IOCON_HAEN|MCP23x17_IOCON_SEQOP);

	/* XXX Hook up to FDT GPIO. */

#if NGPIO > 0
	for (i = 0; i < MCP23x17_GPIO_NPINS; i++) {
		sc->sc_gpio_pins[i].pin_num = i;
		sc->sc_gpio_pins[i].pin_caps = GPIO_PIN_INPUT |
			GPIO_PIN_OUTPUT |
			GPIO_PIN_PUSHPULL | GPIO_PIN_TRISTATE |
			GPIO_PIN_PULLUP |
			GPIO_PIN_INVIN;
		
		/* read initial state */
		sc->sc_gpio_pins[i].pin_state =
			mcp23s17gpio_gpio_pin_read(sc, i);
	}

	/* create controller tag */
	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = mcp23s17gpio_gpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = mcp23s17gpio_gpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = mcp23s17gpio_gpio_pin_ctl;
	
	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = MCP23x17_GPIO_NPINS;

	config_found(self, &gba, gpiobus_print, CFARGS_NONE);
#else
	aprint_normal_dev(sc->sc_dev, "no GPIO configured in kernel");
#endif
}

#if NGPIO > 0
static uint8_t
mcp23s17gpio_read(struct mcp23s17gpio_softc *sc, uint8_t addr)
{
	uint8_t buf[2];
	uint8_t val = 0;
	int rc;

	buf[0] = MCP23x17_OP_READ(sc->sc_ha);
	buf[1] = addr;

	rc = spi_send_recv(sc->sc_sh, 2, buf, 1, &val);
	if (rc != 0)
	{
		aprint_normal_dev(sc->sc_dev, "SPI send_recv failed rc=%d\n", rc); 
	}

	DPRINTF(3, ("%s: read(0x%02x) @0x%02x->0x%02x\n", device_xname(sc->sc_dev), buf[0], addr, val));
	return val;
}
#endif

static void
mcp23s17gpio_write(struct mcp23s17gpio_softc *sc, uint8_t addr, uint8_t val)
{
	uint8_t buf[3];
	int rc;

	buf[0] = MCP23x17_OP_WRITE(sc->sc_ha);
	buf[1] = addr;
	buf[2] = val;

	rc = spi_send(sc->sc_sh, 3, buf);
	if (rc != 0)
	{
		aprint_normal_dev(sc->sc_dev, "SPI send failed rc=%d\n", rc); 
	}
	DPRINTF(3, ("%s: write(0x%02x) @0x%02x<-0x%02x\n", device_xname(sc->sc_dev), buf[0], addr, val));
}

#if NGPIO > 0
/* GPIO support functions */
static int
mcp23s17gpio_gpio_pin_read(void *arg, int pin)
{
	struct mcp23s17gpio_softc *sc = arg;
	int epin = pin & MCP23x17_GPIO_NPINS_MASK;
	uint8_t data;
	uint8_t addr;
	int val;

	if (epin < 8) {
		addr = MCP23x17_GPIOA(sc->sc_bank);
	} else {
		addr = MCP23x17_GPIOB(sc->sc_bank);
		epin = pin - 8;
	}

	data = mcp23s17gpio_read(sc, addr);

	val = data & (1 << epin) ? GPIO_PIN_HIGH : GPIO_PIN_LOW;

	DPRINTF(2, ("%s: gpio_read pin %d->%d\n", device_xname(sc->sc_dev), pin, (val == GPIO_PIN_HIGH)));
	
	return val;
}

static void
mcp23s17gpio_gpio_pin_write(void *arg, int pin, int value)
{
	struct mcp23s17gpio_softc *sc = arg;
	int epin = pin & MCP23x17_GPIO_NPINS_MASK;
	uint8_t data;
	uint8_t addr;

	if (epin < 8) {
		addr = MCP23x17_OLATA(sc->sc_bank);
	} else {
		addr = MCP23x17_OLATB(sc->sc_bank);
		epin = pin - 8;
	}

	data = mcp23s17gpio_read(sc, addr);

	if (value == GPIO_PIN_HIGH) {
		data |= 1 << epin;
	} else {
		data &= ~(1 << epin);
	}

	mcp23s17gpio_write(sc, addr, data);

	DPRINTF(2, ("%s: gpio_write pin %d<-%d\n", device_xname(sc->sc_dev), pin, (value == GPIO_PIN_HIGH)));
}

static void
mcp23s17gpio_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct mcp23s17gpio_softc *sc = arg;
	int epin = pin & MCP23x17_GPIO_NPINS_MASK;
	uint8_t bit;
	uint8_t port;
	uint8_t data;

	if (epin < 8) {
		port = 0;
	} else {
		port = 1;
		epin = pin - 8;
	}

	bit = 1 << epin;

	DPRINTF(2, ("%s: gpio_ctl pin %d flags 0x%x\n", device_xname(sc->sc_dev), pin, flags));

	if (flags & (GPIO_PIN_OUTPUT|GPIO_PIN_INPUT)) {
		data = mcp23s17gpio_read(sc, MCP23x17_IODIR(sc->sc_bank, port));
		if ((flags & GPIO_PIN_INPUT) || !(flags & GPIO_PIN_OUTPUT)) {
			/* for safety INPUT will override output */
			data |= bit;
		} else {
			data &= ~bit;
		}
		mcp23s17gpio_write(sc, MCP23x17_IODIR(sc->sc_bank, port), data);
	}

	data = mcp23s17gpio_read(sc, MCP23x17_IPOL(sc->sc_bank, port));
	if (flags & GPIO_PIN_INVIN) {
		data |= bit;
	} else {
		data &= ~bit;
	}
	mcp23s17gpio_write(sc, MCP23x17_IPOL(sc->sc_bank, port), data);

	data = mcp23s17gpio_read(sc, MCP23x17_GPPU(sc->sc_bank, port));
	if (flags & GPIO_PIN_PULLUP) {
		data |= bit;
	} else {
		data &= ~bit;
	}
	mcp23s17gpio_write(sc, MCP23x17_GPPU(sc->sc_bank, port), data);
} 
#endif /* NGPIO > 0 */
