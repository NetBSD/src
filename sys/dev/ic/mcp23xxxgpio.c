/*      $NetBSD: mcp23xxxgpio.c,v 1.2 2022/01/17 19:38:14 thorpej Exp $	*/

/*-
 * Copyright (c) 2014, 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank Kardel, and by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: mcp23xxxgpio.c,v 1.2 2022/01/17 19:38:14 thorpej Exp $");

/* 
 * Driver for Microchip serial I/O expansers:
 *
 *	MCP23008	8-bit, I2C interface
 *	MCP23S08	8-bit, SPI interface
 *	MCP23017	16-bit, I2C interface
 *	MCP23S17	16-bit, SPI interface
 *	MCP23018	16-bit (open-drain outputs), I2C interface
 *	MCP23S18	16-bit (open-drain outputs), SPI interface
 *
 * Data sheet:
 *
 *	https://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf
 */

#include "gpio.h"

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <dev/ic/mcp23xxxgpioreg.h>
#include <dev/ic/mcp23xxxgpiovar.h>

#define	PIN_BANK(p)	((p) / MCPGPIO_PINS_PER_BANK)
#define	PIN_EPIN(p)	((p) % MCPGPIO_PINS_PER_BANK)

static uint8_t
mcpgpio_regaddr(struct mcpgpio_softc *sc, uint8_t bank, uint8_t reg)
{

	if (sc->sc_variant->type == MCPGPIO_TYPE_23x08) {
		return reg;
	}
	if (sc->sc_iocon & IOCON_BANK) {
		return REGADDR_BANK1(bank & 1, reg);
	}
	return REGADDR_BANK0(bank & 1, reg);
}

static const char *
mcpgpio_regname(uint8_t reg)
{
	static const char * const regnames[] = {
		[REG_IODIR]	=	"IODIR",
		[REG_IPOL]	=	"IPOL",
		[REG_GPINTEN]	=	"GPINTEN",
		[REG_DEFVAL]	=	"DEFVAL",
		[REG_INTCON]	=	"INTCON",
		[REG_IOCON]	=	"IOCON",
		[REG_GPPU]	=	"GPPU",
		[REG_INTF]	=	"INTF",
		[REG_INTCAP]	=	"INTCAP",
		[REG_GPIO]	=	"GPIO",
		[REG_OLAT]	=	"OLAT",
	};
	KASSERT(reg <= REG_OLAT);
	return regnames[reg];
}

static const char *
mcpgpio_bankname(struct mcpgpio_softc *sc, uint8_t bank)
{
	static const char * const banknames[] = { "A", "B" };

	if (sc->sc_variant->type == MCPGPIO_TYPE_23x08) {
		return "";
	}
	return banknames[bank & 1];
}

static int
mcpgpio__lock(struct mcpgpio_softc *sc, const char *fn)
{
	int error;

	error = sc->sc_accessops->lock(sc);
	if (__predict_false(error != 0)) {
		aprint_error_dev(sc->sc_dev,
		    "%s: unable to lock device, error=%d\n", fn, error);
	}
	return error;
}

#define	mcpgpio_lock(sc)						\
	mcpgpio__lock((sc), __func__)

static void
mcpgpio_unlock(struct mcpgpio_softc *sc)
{
	sc->sc_accessops->unlock(sc);
}

static int
mcpgpio__read(struct mcpgpio_softc *sc, const char *fn,
    uint8_t bank, uint8_t reg, uint8_t *valp)
{
	int error;
	uint8_t regaddr = mcpgpio_regaddr(sc, bank, reg);

	error = sc->sc_accessops->read(sc, bank, regaddr, valp);
	if (__predict_false(error != 0)) {
		aprint_error_dev(sc->sc_dev,
		    "%s: unable to read %s%s[0x%02x], error=%d\n", fn,
		    mcpgpio_regname(reg), mcpgpio_bankname(sc, bank),
		    regaddr, error);
	}
	return error;
}

#define	mcpgpio_read(sc, b, r, v)					\
	mcpgpio__read((sc), __func__, (b), (r), (v))

static int
mcpgpio__write(struct mcpgpio_softc *sc, const char *fn,
    uint8_t bank, uint8_t reg, uint8_t val)
{
	int error;
	uint8_t regaddr = mcpgpio_regaddr(sc, bank, reg);

	error = sc->sc_accessops->write(sc, bank, regaddr, val);
	if (__predict_false(error != 0)) {
		aprint_error_dev(sc->sc_dev,
		    "%s: unable to write %s%s[0x%02x], error=%d\n", fn,
		    mcpgpio_regname(reg), mcpgpio_bankname(sc, bank),
		    regaddr, error);
	}
	return error;
}

#define	mcpgpio_write(sc, b, r, v)					\
	mcpgpio__write((sc), __func__, (b), (r), (v))

#if NGPIO > 0
/* GPIO support functions */
static int
mcpgpio_gpio_pin_read(void *arg, int pin)
{
	struct mcpgpio_softc *sc = arg;
	uint8_t data;
	int val;
	int error;

	KASSERT(pin >= 0 && pin < sc->sc_npins);

	const uint8_t bank = PIN_BANK(pin);
	const uint8_t epin = PIN_EPIN(pin);

	error = mcpgpio_lock(sc);
	if (__predict_false(error != 0)) {
		return GPIO_PIN_LOW;
	}
	error = mcpgpio_read(sc, bank, REG_GPIO, &data);
	if (error) {
		data = 0;
	}
	mcpgpio_unlock(sc);

	val = data & __BIT(epin) ? GPIO_PIN_HIGH : GPIO_PIN_LOW;

	return val;
}

static void
mcpgpio_gpio_pin_write(void *arg, int pin, int value)
{
	struct mcpgpio_softc *sc = arg;
	uint8_t data;
	int error;

	KASSERT(pin >= 0 && pin < sc->sc_npins);

	const uint8_t bank = PIN_BANK(pin);
	const uint8_t epin = PIN_EPIN(pin);

	error = mcpgpio_lock(sc);
	if (__predict_false(error != 0)) {
		return;
	}

	error = mcpgpio_read(sc, bank, REG_OLAT, &data);
	if (__predict_true(error == 0)) {
		if (value == GPIO_PIN_HIGH) {
			data |= __BIT(epin);
		} else {
			data &= ~__BIT(epin);
		}
		(void) mcpgpio_write(sc, bank, REG_OLAT, data);
	}

	mcpgpio_unlock(sc);
}

static void
mcpgpio_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct mcpgpio_softc *sc = arg;
	uint8_t iodir, ipol, gppu;
	int error;

	KASSERT(pin >= 0 && pin < sc->sc_npins);

	const uint8_t bank = PIN_BANK(pin);
	const uint8_t epin = PIN_EPIN(pin);
	const uint8_t bit = __BIT(epin);

	error = mcpgpio_lock(sc);
	if (__predict_false(error != 0)) {
		return;
	}

	if ((error = mcpgpio_read(sc, bank, REG_IODIR, &iodir)) != 0 ||
	    (error = mcpgpio_read(sc, bank, REG_IPOL, &ipol)) != 0 ||
	    (error = mcpgpio_read(sc, bank, REG_GPPU, &gppu)) != 0) {
		return;
	}

	if (flags & (GPIO_PIN_OUTPUT|GPIO_PIN_INPUT)) {
		if ((flags & GPIO_PIN_INPUT) || !(flags & GPIO_PIN_OUTPUT)) {
			/* for safety INPUT will override output */
			iodir |= bit;
		} else {
			iodir &= ~bit;
		}
	}

	if (flags & GPIO_PIN_INVIN) {
		ipol |= bit;
	} else {
		ipol &= ~bit;
	}

	if (flags & GPIO_PIN_PULLUP) {
		gppu |= bit;
	} else {
		gppu &= ~bit;
	}

	(void) mcpgpio_write(sc, bank, REG_IODIR, iodir);
	(void) mcpgpio_write(sc, bank, REG_IPOL, ipol);
	(void) mcpgpio_write(sc, bank, REG_GPPU, gppu);

	mcpgpio_unlock(sc);
} 
#endif /* NGPIO > 0 */

void
mcpgpio_attach(struct mcpgpio_softc *sc)
{
	int error;

	KASSERT(sc->sc_variant != NULL);

	/*
	 * The SPI front-end provides the logical pin count to
	 * deal with muliple chips on one chip select.
	 */
	if (sc->sc_npins == 0) {
		sc->sc_npins = sc->sc_variant->type == MCPGPIO_TYPE_23x08
		    ? MCP23x08_GPIO_NPINS : MCP23x17_GPIO_NPINS;
	}
	sc->sc_gpio_pins =
	    kmem_zalloc(sc->sc_npins * sizeof(*sc->sc_gpio_pins), KM_SLEEP);

	/*
	 * Perform the basic setup of the device.  We program the IOCON
	 * register once for each bank, even though the data sheet is
	 * not clear that this is strictly necessary.
	 */
	if (mcpgpio_lock(sc) != 0) {
		return;
	}
	error = mcpgpio_write(sc, 0, REG_IOCON, sc->sc_iocon);
	if (error == 0 && sc->sc_variant->type != MCPGPIO_TYPE_23x08) {
		error = mcpgpio_write(sc, 1, REG_IOCON, sc->sc_iocon);
	}
	mcpgpio_unlock(sc);
	if (error) {
		return;
	}

	/* XXX FDT glue. */

#if NGPIO > 0
	struct gpiobus_attach_args gba;
	int pin_output_caps;
	int i;

	pin_output_caps = sc->sc_variant->type == MCPGPIO_TYPE_23x18
	    ? GPIO_PIN_OPENDRAIN : GPIO_PIN_PUSHPULL;

	for (i = 0; i < sc->sc_npins; i++) {
		sc->sc_gpio_pins[i].pin_num = i;
		sc->sc_gpio_pins[i].pin_caps = GPIO_PIN_INPUT |
			GPIO_PIN_OUTPUT |
			pin_output_caps |
			GPIO_PIN_PULLUP |
			GPIO_PIN_INVIN;

		/* read initial state */
		sc->sc_gpio_pins[i].pin_state =
			mcpgpio_gpio_pin_read(sc, i);
	}

	/* create controller tag */
	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = mcpgpio_gpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = mcpgpio_gpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = mcpgpio_gpio_pin_ctl;

	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = sc->sc_npins;

	config_found(sc->sc_dev, &gba, gpiobus_print,
	    CFARGS(.devhandle = device_handle(sc->sc_dev)));
#else
	aprint_normal_dev(sc->sc_dev, "no GPIO configured in kernel");
#endif
}
