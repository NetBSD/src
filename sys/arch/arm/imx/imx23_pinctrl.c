/* $NetBSD: imx23_pinctrl.c,v 1.10 2026/02/02 09:51:40 yurix Exp $ */

/*
* Copyright (c) 2013 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Petri Laakso.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/gpio.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>
#include <dev/gpio/gpiovar.h>

#include <arm/imx/imx23_pinctrlreg.h>
#include <arm/imx/imx23var.h>

#define IMX23_NUM_GPIO_PINS 96
#define NUM_GPIO_BANKS 3

struct imx23_pinctrl_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_hdl;
	struct gpio_chipset_tag gc;
	gpio_pin_t pins[IMX23_NUM_GPIO_PINS];
	int bank_phandles[NUM_GPIO_BANKS];
};

struct imx23_pinctrl_fdt_pin {
	int pin_nr;
	bool pin_actlo;
};

static int	imx23_pinctrl_match(device_t, cfdata_t, void *);
static void	imx23_pinctrl_attach(device_t, device_t, void *);

static void     imx23_pinctrl_init(struct imx23_pinctrl_softc *);

static	int	imx23_pinctrl_gp_gc_open(void *, device_t);
static	void	imx23_pinctrl_gp_gc_close(void *, device_t);
static	int	imx23_pinctrl_gp_pin_read(void *, int);
static	void	imx23_pinctrl_gp_pin_write(void *, int, int);
static	void	imx23_pinctrl_gp_pin_ctl(void *, int, int);

static void 	*imx23_gpio_acquire(device_t, const void *, size_t, int);
static void 	imx23_gpio_release(device_t, void *);
static int 	imx23_gpio_read(device_t, void *, bool);
static void 	imx23_gpio_write(device_t, void *, int, bool);

static struct imx23_pinctrl_softc *_sc = NULL;

CFATTACH_DECL_NEW(imx23pctl, sizeof(struct imx23_pinctrl_softc),
		  imx23_pinctrl_match, imx23_pinctrl_attach, NULL, NULL);

#define GPIO_PIN_CAP (GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_INOUT | \
		GPIO_PIN_PULLUP | GPIO_PIN_SET)

/*
 * Supported capabilities for each GPIO pin.
 */
const static int pin_caps[IMX23_NUM_GPIO_PINS] = {
	/*
	 * HW_PINCTRL_MUXSEL0
	 */
	/* PIN 0 */
	GPIO_PIN_CAP,
	/* PIN 1 */
	GPIO_PIN_CAP,
	/* PIN 2 */
	GPIO_PIN_CAP,
	/* PIN 3 */
	GPIO_PIN_CAP,
	/* PIN 4 */
	GPIO_PIN_CAP,
	/* PIN 5 */
	GPIO_PIN_CAP,
	/* PIN 6 */
	GPIO_PIN_CAP,
	/* PIN 7 */
	GPIO_PIN_CAP,
	/* PIN 8 */
	GPIO_PIN_CAP,
	/* PIN 9 */
	GPIO_PIN_CAP,
	/* PIN 10 */
	GPIO_PIN_CAP,
	/* PIN 11 */
	GPIO_PIN_CAP,
	/* PIN 12 */
	GPIO_PIN_CAP,
	/* PIN 13 */
	GPIO_PIN_CAP,
	/* PIN 14 */
	GPIO_PIN_CAP,
	/* PIN 15 */
	GPIO_PIN_CAP,
	/*
	 * HW_PINCTRL_MUXSEL1
	 */
	/* PIN 16 */
	GPIO_PIN_CAP,
	/* PIN 17 */
	0,		/* Reserved for powering OLinuXino MAXI/MINI USB hub. */
	/* PIN 18 */
	GPIO_PIN_CAP,
	/* PIN 19 */
	GPIO_PIN_CAP,
	/* PIN 20 */
	GPIO_PIN_CAP,
	/* PIN 21 */
	GPIO_PIN_CAP,
	/* PIN 22 */
	GPIO_PIN_CAP,
	/* PIN 23 */
	GPIO_PIN_CAP,
	/* PIN 24 */
	GPIO_PIN_CAP,
	/* PIN 25 */
	GPIO_PIN_CAP,
	/* PIN 26 */
	GPIO_PIN_CAP,
	/* PIN 27 */
	GPIO_PIN_CAP,
	/* PIN 28 */
	GPIO_PIN_CAP,
	/* PIN 29 */
	GPIO_PIN_CAP,
	/* PIN 30 */
	GPIO_PIN_CAP,
	/* PIN 31 */
	GPIO_PIN_CAP,
	/*
	 * HW_PINCTRL_MUXSEL2
	 */
	/* PIN 32 */
	GPIO_PIN_CAP,
	/* PIN 33 */
	GPIO_PIN_CAP,
	/* PIN 34 */
	GPIO_PIN_CAP,
	/* PIN 35 */
	GPIO_PIN_CAP,
	/* PIN 36 */
	GPIO_PIN_CAP,
	/* PIN 37 */
	GPIO_PIN_CAP,
	/* PIN 38 */
	GPIO_PIN_CAP,
	/* PIN 39 */
	GPIO_PIN_CAP,
	/* PIN 40 */
	GPIO_PIN_CAP,
	/* PIN 41 */
	GPIO_PIN_CAP,
	/* PIN 42 */
	GPIO_PIN_CAP,
	/* PIN 43 */
	GPIO_PIN_CAP,
	/* PIN 44 */
	GPIO_PIN_CAP,
	/* PIN 45 */
	GPIO_PIN_CAP,
	/* PIN 46 */
	GPIO_PIN_CAP,
	/* PIN 47 */
	GPIO_PIN_CAP,
	/*
	 * HW_PINCTRL_MUXSEL3
	 */
	/* PIN 48 */
	GPIO_PIN_CAP,
	/* PIN 49 */
	GPIO_PIN_CAP,
	/* PIN 50 */
	GPIO_PIN_CAP,
	/* PIN 51 */
	GPIO_PIN_CAP,
	/* PIN 52 */
	GPIO_PIN_CAP,
	/* PIN 53 */
	GPIO_PIN_CAP,
	/* PIN 54 */
	GPIO_PIN_CAP,
	/* PIN 55 */
	GPIO_PIN_CAP,
	/* PIN 56 */
	GPIO_PIN_CAP,
	/* PIN 57 */
	GPIO_PIN_CAP,
	/* PIN 58 */
	GPIO_PIN_CAP,
	/* PIN 59 */
	GPIO_PIN_CAP,
	/* PIN 60 */
	GPIO_PIN_CAP,
	/* PIN 61 */
	GPIO_PIN_CAP,
	/* PIN 62 */
	GPIO_PIN_CAP,
	/* PIN 63 */
	0,		/* Reserved. */
	/*
	 * HW_PINCTRL_MUXSEL4
	 */
	/* PIN 64 */
	GPIO_PIN_CAP,
	/* PIN 65 */
	GPIO_PIN_CAP,
	/* PIN 66 */
	GPIO_PIN_CAP,
	/* PIN 67 */
	GPIO_PIN_CAP,
	/* PIN 68 */
	GPIO_PIN_CAP,
	/* PIN 69 */
	GPIO_PIN_CAP,
	/* PIN 70 */
	GPIO_PIN_CAP,
	/* PIN 71 */
	GPIO_PIN_CAP,
	/* PIN 72 */
	GPIO_PIN_CAP,
	/* PIN 73 */
	0,		/* From this on reserved for EMI (DRAM) pins. */
	/* PIN 74 */
	0,
	/* PIN 75 */
	0,
	/* PIN 76 */
	0,
	/* PIN 77 */
	0,
	/* PIN 78 */
	0,
	/* PIN 79 */
	0,
	/*
	 * HW_PINCTRL_MUXSEL5
	 */
	/* PIN 80 */
	0,
	/* PIN 81 */
	0,
	/* PIN 82 */
	0,
	/* PIN 83 */
	0,
	/* PIN 84 */
	0,
	/* PIN 85 */
	0,
	/* PIN 86 */
	0,
	/* PIN 87 */
	0,
	/* PIN 88 */
	0,
	/* PIN 89 */
	0,
	/* PIN 90 */
	0,
	/* PIN 91 */
	0,
	/* PIN 92 */
	0,
	/* PIN 93 */
	0,
	/* PIN 94 */
	0,
	/* PIN 95 */
	0
};

#define PINCTRL_RD(sc, reg)						\
	bus_space_read_4(sc->sc_iot, sc->sc_hdl, (reg))
#define PINCTRL_WR(sc, reg, val)					\
	bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))

/*
 * Macros to map pin numbers to registers and bit fields.
 */
#define MUXSEL_REG_SIZE	0x10
#define PIN2MUXSEL_REG(pin)						\
	((pin / 16) * MUXSEL_REG_SIZE + HW_PINCTRL_MUXSEL0)
#define PIN2MUXSEL_SET_REG(pin)						\
	((pin / 16) * MUXSEL_REG_SIZE + HW_PINCTRL_MUXSEL0_SET)
#define PIN2MUXSEL_CLR_REG(pin)						\
	((pin / 16) * MUXSEL_REG_SIZE + HW_PINCTRL_MUXSEL0_CLR)
#define PIN2MUXSEL_MASK(pin)	(3<<(pin % 16 * 2))

#define DRIVE_REG_SIZE	0x10
#define PIN2DRIVE_REG(pin)						\
	((pin / 8) * DRIVE_REG_SIZE + HW_PINCTRL_DRIVE0)
#define PIN2DRIVE_SET_REG(pin)						\
	((pin / 8) * DRIVE_REG_SIZE + HW_PINCTRL_DRIVE0_SET)
#define PIN2DRIVE_CLR_REG(pin)						\
	((pin / 8) * DRIVE_REG_SIZE + HW_PINCTRL_DRIVE0_CLR)
#define PIN2DRIVE_MASK(pin)	(3<<(pin % 8 * 4))

#define PULL_REG_SIZE	0x10
#define PIN2PULL_REG(pin)						\
	((pin / 32) * PULL_REG_SIZE + HW_PINCTRL_PULL0)
#define PIN2PULL_SET_REG(pin)						\
	((pin / 32) * PULL_REG_SIZE + HW_PINCTRL_PULL0_SET)
#define PIN2PULL_CLR_REG(pin)						\
	((pin / 32) * PULL_REG_SIZE + HW_PINCTRL_PULL0_CLR)
#define PIN2PULL_MASK(pin)	(1<<(pin % 32))

#define DOUT_REG_SIZE	0x10
#define PIN2DOUT_REG(pin)						\
	((pin / 32) * DOUT_REG_SIZE + HW_PINCTRL_DOUT0)
#define PIN2DOUT_SET_REG(pin)						\
	((pin / 32) * DOUT_REG_SIZE + HW_PINCTRL_DOUT0_SET)
#define PIN2DOUT_CLR_REG(pin)						\
	((pin / 32) * DOUT_REG_SIZE + HW_PINCTRL_DOUT0_CLR)
#define PIN2DOUT_MASK(pin)	(1<<(pin % 32))

#define DIN_REG_SIZE	0x10
#define PIN2DIN_REG(pin)	((pin / 32) * DIN_REG_SIZE + HW_PINCTRL_DIN0)
#define PIN2DIN_MASK(pin)	(1<<(pin % 32))

#define DOE_REG_SIZE	0x10
#define PIN2DOE_REG(pin)						\
	((pin / 32) * DOE_REG_SIZE + HW_PINCTRL_DOE0)
#define PIN2DOE_SET_REG(pin)						\
	((pin / 32) * DOE_REG_SIZE + HW_PINCTRL_DOE0_SET)
#define PIN2DOE_CLR_REG(pin)						\
	((pin / 32) * DOE_REG_SIZE + HW_PINCTRL_DOE0_CLR)
#define PIN2DOE_MASK(pin)	(1<<(pin % 32))

#define DRIVE_STRENGTH_4MA	0
#define DRIVE_STRENGTH_8MA	1
#define DRIVE_STRENGTH_12MA	2

#define MUXEL_GPIO_MODE	3

#define PINCTRL_SOFT_RST_LOOP 455 /* At least 1 us ... */

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx23-pinctrl" },
	DEVICE_COMPAT_EOL
};

static const struct device_compatible_entry child_compat_data[] = {
	{ .compat = "fsl,imx23-gpio" },
	DEVICE_COMPAT_EOL
};

static struct fdtbus_gpio_controller_func imx23_gpio_funcs = {
	.acquire = imx23_gpio_acquire,
	.release = imx23_gpio_release,
	.read = imx23_gpio_read,
	.write = imx23_gpio_write
};

static void *
imx23_gpio_acquire(device_t dev, const void *data, size_t len, int flags) {
	struct imx23_pinctrl_softc * const sc = device_private(dev);
	struct imx23_pinctrl_fdt_pin *pin;
	const uint32_t *gpio = data;

	if (len != 12) return NULL;

	const int bank_phandle =
	    fdtbus_get_phandle_from_native(be32toh(gpio[0]));
	int pin_nr_offset = 0;
	for (int i=0;i<NUM_GPIO_BANKS;i++) {
		if (sc->bank_phandles[i] == bank_phandle) {
			pin_nr_offset = 32*i;
			break;
		}
	}

	const int pin_nr = pin_nr_offset + be32toh(gpio[1]);
	const bool actlo = be32toh(gpio[2]) & 1;

	pin = kmem_zalloc(sizeof(struct imx23_pinctrl_fdt_pin), KM_SLEEP);
	pin->pin_nr = pin_nr;
	pin->pin_actlo = actlo;

	gpiobus_pin_ctl(&sc->gc, pin->pin_nr, flags);

	return pin;
}

static void
imx23_gpio_release(device_t dev, void *priv)
{
	struct imx23_pinctrl_softc * const sc = device_private(dev);
	struct imx23_pinctrl_fdt_pin *pin = priv;

	gpiobus_pin_ctl(&sc->gc, pin->pin_nr, GPIO_PIN_INPUT);

	kmem_free(pin, sizeof(*pin));
}

static int
imx23_gpio_read(device_t dev, void *priv, bool raw)
{
	struct imx23_pinctrl_softc * const sc = device_private(dev);
	struct imx23_pinctrl_fdt_pin *pin = priv;
	int val;

	val = gpiobus_pin_read(&sc->gc, pin->pin_nr);

	if (!raw && pin->pin_actlo)
		val = !val;

	return val;
}

static void
imx23_gpio_write(device_t dev, void *priv, int val, bool raw)
{
	struct imx23_pinctrl_softc * const sc = device_private(dev);
	struct imx23_pinctrl_fdt_pin *pin = priv;

	if (!raw && pin->pin_actlo)
		val = !val;

	gpiobus_pin_write(&sc->gc, pin->pin_nr, val);
}

static int
imx23_pinctrl_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx23_pinctrl_attach(device_t parent, device_t self, void *aux)
{
	struct imx23_pinctrl_softc *const sc = device_private(self);
	struct fdt_attach_args *const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;

	bus_addr_t addr;
	bus_size_t size;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get register address\n");
		return;
	}
	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_hdl)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	imx23_pinctrl_init(sc);

	aprint_normal(": PIN MUX & GPIO\n");

	/* Set pin capabilities. */
	for (int i = 0; i < IMX23_NUM_GPIO_PINS; i++) {
		sc->pins[i].pin_caps = pin_caps[i];
	}

	sc->gc.gp_cookie = sc;
	sc->gc.gp_gc_open = imx23_pinctrl_gp_gc_open;
	sc->gc.gp_gc_close = imx23_pinctrl_gp_gc_close;
	sc->gc.gp_pin_read = imx23_pinctrl_gp_pin_read;
	sc->gc.gp_pin_write = imx23_pinctrl_gp_pin_write;
	sc->gc.gp_pin_ctl = imx23_pinctrl_gp_pin_ctl;

	struct gpiobus_attach_args gpiobus_aa;
	gpiobus_aa.gba_gc = &sc->gc;
	gpiobus_aa.gba_npins = IMX23_NUM_GPIO_PINS;
	gpiobus_aa.gba_pins = sc->pins;

	config_found(sc->sc_dev, &gpiobus_aa, gpiobus_print, CFARGS_NONE);

	/* configure fdt gpio system */
	for (int i=0;i<NUM_GPIO_BANKS;i++) {
		sc->bank_phandles[i] = 0;
	}
	/* add gpio banks  */
	for (int child = OF_child(phandle); child; child = OF_peer(child)) {
		if (!of_compatible_match(child, child_compat_data))
			continue;

		bus_addr_t gpio_instance;
		if (fdtbus_get_reg(child, 0, &gpio_instance, NULL) != 0) {
			aprint_error(": couldn't get register address\n");
			return;
		}

		if (gpio_instance >= NUM_GPIO_BANKS){
			aprint_error(": bank %ld out of range", gpio_instance);
			continue;
		}
		sc->bank_phandles[gpio_instance] = child;

		fdtbus_register_gpio_controller(self, child, &imx23_gpio_funcs);
	}
}

static void
imx23_pinctrl_init(struct imx23_pinctrl_softc *sc)
{
	_sc = sc;
}

static	int
imx23_pinctrl_gp_gc_open(void *cookie, device_t dev)
{
	return 0;
}

static	void
imx23_pinctrl_gp_gc_close(void *cookie, device_t dev)
{
    /* do nothing */
}

static	int
imx23_pinctrl_gp_pin_read(void *cookie, int pin)
{
	int value;
	struct imx23_pinctrl_softc *sc = (struct imx23_pinctrl_softc *) cookie;

	if (PINCTRL_RD(sc, PIN2DIN_REG(pin)) & PIN2DIN_MASK(pin))
		value = 1;
	else
		value = 0;

	return value;
}

static	void
imx23_pinctrl_gp_pin_write(void *cookie, int pin, int value)
{
	struct imx23_pinctrl_softc *sc = (struct imx23_pinctrl_softc *) cookie;

	if (value)
		PINCTRL_WR(sc, PIN2DOUT_SET_REG(pin), PIN2DOUT_MASK(pin));
	else
		PINCTRL_WR(sc, PIN2DOUT_CLR_REG(pin), PIN2DOUT_MASK(pin));
}

/*
 * Configure pin as requested in flags.
 */
static	void
imx23_pinctrl_gp_pin_ctl(void *cookie, int pin, int flags)
{
	struct imx23_pinctrl_softc *sc = (struct imx23_pinctrl_softc *) cookie;
	uint32_t tmpr;

	/* Enable GPIO pin. */
	tmpr = PINCTRL_RD(sc, PIN2MUXSEL_REG(pin));
	tmpr &= ~PIN2MUXSEL_MASK(pin);
	tmpr |= __SHIFTIN(MUXEL_GPIO_MODE, PIN2MUXSEL_MASK(pin));
	PINCTRL_WR(sc, PIN2MUXSEL_REG(pin), tmpr);

	/* Configure pin drive strength. */
	tmpr = PINCTRL_RD(sc, PIN2DRIVE_REG(pin));
	tmpr &= ~PIN2DRIVE_MASK(pin);
	tmpr |= __SHIFTIN(DRIVE_STRENGTH_4MA, PIN2DRIVE_MASK(pin));
	PINCTRL_WR(sc, PIN2DRIVE_REG(pin), tmpr);

	if ((flags & (GPIO_PIN_OUTPUT | GPIO_PIN_INOUT))) {
		/* Configure pullup resistor or gate keeper. */
		if (flags & GPIO_PIN_PULLUP)
			PINCTRL_WR(sc, PIN2PULL_SET_REG(pin),
				PIN2PULL_MASK(pin));
		else
			PINCTRL_WR(sc, PIN2PULL_CLR_REG(pin),
				PIN2PULL_MASK(pin));

		/* Set initial pin value to logic zero. */
		PINCTRL_WR(sc, PIN2DOUT_CLR_REG(pin), PIN2DOUT_MASK(pin));

		/* Enable pin output. */
		PINCTRL_WR(sc, PIN2DOE_SET_REG(pin), PIN2DOE_MASK(pin));
	}

	if (flags & GPIO_PIN_INPUT) {
		/* Disable pin output. */
		PINCTRL_WR(sc, PIN2DOE_CLR_REG(pin), PIN2DOE_MASK(pin));

		/* Configure pullup resistor or gate keeper. */
		if (flags & GPIO_PIN_PULLUP)
			PINCTRL_WR(sc, PIN2PULL_SET_REG(pin),
				PIN2PULL_MASK(pin));
		else
			PINCTRL_WR(sc, PIN2PULL_CLR_REG(pin),
				PIN2PULL_MASK(pin));
	}
}
