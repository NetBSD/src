/* $NetBSD: gpioiic.c,v 1.2 2009/12/06 22:33:44 dyoung Exp $ */
/*	$OpenBSD: gpioiic.c,v 1.8 2008/11/24 12:12:12 mbalmer Exp $	*/

/*
 * Copyright (c) 2006 Alexander Yurchenko <grange@openbsd.org>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gpioiic.c,v 1.2 2009/12/06 22:33:44 dyoung Exp $");

/*
 * I2C bus bit-banging through GPIO pins.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/rwlock.h>

#include <dev/gpio/gpiovar.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>

#define GPIOIIC_PIN_SDA		0
#define GPIOIIC_PIN_SCL		1
#define GPIOIIC_NPINS		2

#define GPIOIIC_SDA		0x01
#define GPIOIIC_SCL		0x02

struct gpioiic_softc {
	void *			sc_gpio;
	struct gpio_pinmap	sc_map;
	int			_map[GPIOIIC_NPINS];

	struct i2c_controller	sc_i2c_tag;
	device_t		sc_i2c_dev;
	krwlock_t		sc_i2c_lock;

	int			sc_sda;
	int			sc_scl;
};

int		gpioiic_match(device_t, cfdata_t, void *);
void		gpioiic_attach(device_t, device_t, void *);
int		gpioiic_detach(device_t, int);

int		gpioiic_i2c_acquire_bus(void *, int);
void		gpioiic_i2c_release_bus(void *, int);
int		gpioiic_i2c_send_start(void *, int);
int		gpioiic_i2c_send_stop(void *, int);
int		gpioiic_i2c_initiate_xfer(void *, i2c_addr_t, int);
int		gpioiic_i2c_read_byte(void *, u_int8_t *, int);
int		gpioiic_i2c_write_byte(void *, u_int8_t, int);

void		gpioiic_bb_set_bits(void *, u_int32_t);
void		gpioiic_bb_set_dir(void *, u_int32_t);
u_int32_t	gpioiic_bb_read_bits(void *);

CFATTACH_DECL_NEW(gpioiic, sizeof(struct gpioiic_softc),
	gpioiic_match, gpioiic_attach, gpioiic_detach, NULL);

extern struct cfdriver gpioiic_cd;

static const struct i2c_bitbang_ops gpioiic_bbops = {
	gpioiic_bb_set_bits,
	gpioiic_bb_set_dir,
	gpioiic_bb_read_bits,
	{ GPIOIIC_SDA, GPIOIIC_SCL, GPIOIIC_SDA, 0 }
};

int
gpioiic_match(device_t parent, cfdata_t cf, void *aux)
{
	struct gpio_attach_args *ga = aux;

	if (strcmp(ga->ga_dvname, cf->cf_name))
		return 0;

	if (ga->ga_offset == -1)
		return 0;

	/* Check that we have enough pins */
	if (gpio_npins(ga->ga_mask) != GPIOIIC_NPINS) {
		aprint_debug("%s: invalid pin mask 0x%02x\n", cf->cf_name,
		    ga->ga_mask);
		return 0;
	}
	return 1;
}

void
gpioiic_attach(struct device *parent, struct device *self, void *aux)
{
	struct gpioiic_softc *sc = device_private(self);
	struct gpio_attach_args *ga = aux;
	struct i2cbus_attach_args iba;
	int caps;

	/* Map pins */
	sc->sc_gpio = ga->ga_gpio;
	sc->sc_map.pm_map = sc->_map;
	if (gpio_pin_map(sc->sc_gpio, ga->ga_offset, ga->ga_mask,
	    &sc->sc_map)) {
		aprint_error(": can't map pins\n");
		return;
	}

	/* Configure SDA pin */
	caps = gpio_pin_caps(sc->sc_gpio, &sc->sc_map, GPIOIIC_PIN_SDA);
	if (!(caps & GPIO_PIN_OUTPUT)) {
		aprint_error(": SDA pin is unable to drive output\n");
		goto fail;
	}
	if (!(caps & GPIO_PIN_INPUT)) {
		aprint_error(": SDA pin is unable to read input\n");
		goto fail;
	}
	aprint_normal(": SDA[%d]", sc->sc_map.pm_map[GPIOIIC_PIN_SDA]);
	sc->sc_sda = GPIO_PIN_OUTPUT;
	if (caps & GPIO_PIN_OPENDRAIN) {
		aprint_normal(" open-drain");
		sc->sc_sda |= GPIO_PIN_OPENDRAIN;
	} else if ((caps & GPIO_PIN_PUSHPULL) && (caps & GPIO_PIN_TRISTATE)) {
		aprint_normal(" push-pull tri-state");
		sc->sc_sda |= GPIO_PIN_PUSHPULL;
	}
	if (caps & GPIO_PIN_PULLUP) {
		aprint_normal(" pull-up");
		sc->sc_sda |= GPIO_PIN_PULLUP;
	}
	gpio_pin_ctl(sc->sc_gpio, &sc->sc_map, GPIOIIC_PIN_SDA, sc->sc_sda);

	/* Configure SCL pin */
	caps = gpio_pin_caps(sc->sc_gpio, &sc->sc_map, GPIOIIC_PIN_SCL);
	if (!(caps & GPIO_PIN_OUTPUT)) {
		aprint_error(": SCL pin is unable to drive output\n");
		goto fail;
	}
	aprint_normal(", SCL[%d]", sc->sc_map.pm_map[GPIOIIC_PIN_SCL]);
	sc->sc_scl = GPIO_PIN_OUTPUT;
	if (caps & GPIO_PIN_OPENDRAIN) {
		aprint_normal(" open-drain");
		sc->sc_scl |= GPIO_PIN_OPENDRAIN;
		if (caps & GPIO_PIN_PULLUP) {
			aprint_normal(" pull-up");
			sc->sc_scl |= GPIO_PIN_PULLUP;
		}
	} else if (caps & GPIO_PIN_PUSHPULL) {
		aprint_normal(" push-pull");
		sc->sc_scl |= GPIO_PIN_PUSHPULL;
	}
	gpio_pin_ctl(sc->sc_gpio, &sc->sc_map, GPIOIIC_PIN_SCL, sc->sc_scl);

	aprint_normal("\n");

	/* Attach I2C bus */
	rw_init(&sc->sc_i2c_lock);
	sc->sc_i2c_tag.ic_cookie = sc;
	sc->sc_i2c_tag.ic_acquire_bus = gpioiic_i2c_acquire_bus;
	sc->sc_i2c_tag.ic_release_bus = gpioiic_i2c_release_bus;
	sc->sc_i2c_tag.ic_send_start = gpioiic_i2c_send_start;
	sc->sc_i2c_tag.ic_send_stop = gpioiic_i2c_send_stop;
	sc->sc_i2c_tag.ic_initiate_xfer = gpioiic_i2c_initiate_xfer;
	sc->sc_i2c_tag.ic_read_byte = gpioiic_i2c_read_byte;
	sc->sc_i2c_tag.ic_write_byte = gpioiic_i2c_write_byte;
	sc->sc_i2c_tag.ic_exec = NULL;

	memset(&iba, 0, sizeof(iba));
	iba.iba_type = I2C_TYPE_SMBUS;
	iba.iba_tag = &sc->sc_i2c_tag;
	sc->sc_i2c_dev = config_found(self, &iba, iicbus_print);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error("%s: could not establish power handler\n",
		    device_xname(self));
	return;

fail:
	gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
}

int
gpioiic_detach(struct device *self, int flags)
{
	struct gpioiic_softc *sc = device_private(self);
	int rv = 0;

	if (sc->sc_i2c_dev != NULL)
		rv = config_detach(sc->sc_i2c_dev, flags);

	if (!rv) {
		gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
		pmf_device_deregister(self);
	}
	return rv;
}

int
gpioiic_i2c_acquire_bus(void *cookie, int flags)
{
	struct gpioiic_softc *sc = cookie;

	if (flags & I2C_F_POLL)
		return 0;

	rw_enter(&sc->sc_i2c_lock, RW_WRITER);
	return 1;
}

void
gpioiic_i2c_release_bus(void *cookie, int flags)
{
	struct gpioiic_softc *sc = cookie;

	if (flags & I2C_F_POLL)
		return;

	rw_exit(&sc->sc_i2c_lock);
}

int
gpioiic_i2c_send_start(void *cookie, int flags)
{
	return i2c_bitbang_send_start(cookie, flags, &gpioiic_bbops);
}

int
gpioiic_i2c_send_stop(void *cookie, int flags)
{
	return i2c_bitbang_send_stop(cookie, flags, &gpioiic_bbops);
}

int
gpioiic_i2c_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{
	return i2c_bitbang_initiate_xfer(cookie, addr, flags, &gpioiic_bbops);
}

int
gpioiic_i2c_read_byte(void *cookie, u_int8_t *bytep, int flags)
{
	return i2c_bitbang_read_byte(cookie, bytep, flags, &gpioiic_bbops);
}

int
gpioiic_i2c_write_byte(void *cookie, u_int8_t byte, int flags)
{
	return i2c_bitbang_write_byte(cookie, byte, flags, &gpioiic_bbops);
}

void
gpioiic_bb_set_bits(void *cookie, u_int32_t bits)
{
	struct gpioiic_softc *sc = cookie;

	gpio_pin_write(sc->sc_gpio, &sc->sc_map, GPIOIIC_PIN_SDA,
	    bits & GPIOIIC_SDA ? GPIO_PIN_HIGH : GPIO_PIN_LOW);
	gpio_pin_write(sc->sc_gpio, &sc->sc_map, GPIOIIC_PIN_SCL,
	    bits & GPIOIIC_SCL ? GPIO_PIN_HIGH : GPIO_PIN_LOW);
}

void
gpioiic_bb_set_dir(void *cookie, u_int32_t bits)
{
	struct gpioiic_softc *sc = cookie;
	int sda = sc->sc_sda;

	sda &= ~(GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_TRISTATE);
	sda |= (bits & GPIOIIC_SDA ? GPIO_PIN_OUTPUT : GPIO_PIN_INPUT);
	if ((sda & GPIO_PIN_PUSHPULL) && !(bits & GPIOIIC_SDA))
		sda |= GPIO_PIN_TRISTATE;
	if (sc->sc_sda != sda) {
		sc->sc_sda = sda;
		gpio_pin_ctl(sc->sc_gpio, &sc->sc_map, GPIOIIC_PIN_SDA,
		    sc->sc_sda);
	}
}

u_int32_t
gpioiic_bb_read_bits(void *cookie)
{
	struct gpioiic_softc *sc = cookie;

	return gpio_pin_read(sc->sc_gpio, &sc->sc_map,
	    GPIOIIC_PIN_SDA) == GPIO_PIN_HIGH ? GPIOIIC_SDA : 0;
}
