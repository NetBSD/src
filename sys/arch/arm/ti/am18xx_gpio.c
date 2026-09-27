/* $NetBSD: am18xx_gpio.c,v 1.1 2026/09/27 19:56:21 yurix Exp $ */

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yuri Honegger.
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
 * GPIOs for the TI AM18XX SoC
 */

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/kmem.h>
#include <sys/mutex.h>

#include <dev/fdt/fdtvar.h>
#include <dev/gpio/gpiovar.h>

struct am18xx_gpio_fdt_pin {
	int pin_nr;
	bool pin_actlo;
};

struct am18xx_gpio_softc {
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	kmutex_t sc_lock;
	struct gpio_chipset_tag sc_gc;
	int sc_num_gpios;
};

static int 	am18xx_gpio_match(device_t, cfdata_t, void *);
static void 	am18xx_gpio_attach(device_t, device_t, void *);
static void *	am18xx_gpio_fdt_acquire(device_t, const void *, size_t, int);
static void 	am18xx_gpio_fdt_release(device_t, void *);
static int 	am18xx_gpio_fdt_read(device_t, void *, bool);
static void 	am18xx_gpio_fdt_write(device_t, void *, int, bool);
static int	am18xx_gpio_gp_pin_read(void *, int);
static void	am18xx_gpio_gp_pin_write(void *, int, int);
static void	am18xx_gpio_gp_pin_ctl(void *, int, int);

CFATTACH_DECL_NEW(am18xxgpio, sizeof(struct am18xx_gpio_softc),
    am18xx_gpio_match, am18xx_gpio_attach, NULL, NULL);

#define AM18XX_GPIO_BANK_BASE	0x10
#define AM18XX_GPIO_BANK_SIZE	0x28
#define AM18XX_GPIO_BANK_DIR	0x0
#define AM18XX_GPIO_BANK_SET	0x8
#define AM18XX_GPIO_BANK_CLEAR	0xC
#define AM18XX_GPIO_BANK_INPUT	0x10

#define	GPIO_READ(sc, reg)					\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, reg)
#define	GPIO_WRITE(sc, reg, val)				\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, reg, val)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,dm6441-gpio" },
	DEVICE_COMPAT_EOL
};

static const struct fdtbus_gpio_controller_func am18xx_gpio_fdt_funcs = {
	.acquire = am18xx_gpio_fdt_acquire,
	.release = am18xx_gpio_fdt_release,
	.read = am18xx_gpio_fdt_read,
	.write = am18xx_gpio_fdt_write,
};

static void *
am18xx_gpio_fdt_acquire(device_t dev, const void *rdata, size_t len, int flags)
{
	struct am18xx_gpio_softc * const sc = device_private(dev);
	const uint32_t *data = rdata;
	struct am18xx_gpio_fdt_pin *pin;

	if (len != 12)
		return NULL;

	int pin_nr = be32toh(data[1]);
	int actlo = be32toh(data[2]) & 1;

	if (pin_nr < 0 || pin_nr >= sc->sc_num_gpios)
		return NULL;

	pin = kmem_zalloc(sizeof(struct am18xx_gpio_fdt_pin), KM_SLEEP);
	if (pin == NULL) {
		return NULL;
	}

	pin->pin_nr = pin_nr;
	pin->pin_actlo = actlo;

	gpiobus_pin_ctl(&sc->sc_gc, pin->pin_nr, flags);

	return pin;
}

static void
am18xx_gpio_fdt_release(device_t dev, void *priv)
{
	struct am18xx_gpio_softc * const sc = device_private(dev);
	struct am18xx_gpio_fdt_pin * const pin = priv;

	am18xx_gpio_gp_pin_ctl(sc, pin->pin_nr, GPIO_PIN_INPUT);

	kmem_free(pin, sizeof(*pin));
}

static int
am18xx_gpio_fdt_read(device_t dev, void *priv, bool raw)
{
	struct am18xx_gpio_softc * const sc = device_private(dev);
	struct am18xx_gpio_fdt_pin * const pin = priv;
	int val;

	val = am18xx_gpio_gp_pin_read(sc, pin->pin_nr);

	if (!raw && pin->pin_actlo)
		val = !val;

	return val;
}

static void
am18xx_gpio_fdt_write(device_t dev, void *priv, int val, bool raw)
{
	struct am18xx_gpio_softc * const sc = device_private(dev);
	struct am18xx_gpio_fdt_pin * const pin = priv;

	if (!raw && pin->pin_actlo)
		val = !val;

	am18xx_gpio_gp_pin_write(sc, pin->pin_nr, val);
}

static int
am18xx_gpio_gp_pin_read(void *priv, int pin)
{
	struct am18xx_gpio_softc * const sc = priv;

	KASSERT(pin >= 0 && pin < sc->sc_num_gpios);

	int bank_group = pin / 32;
	uint32_t group_bit = __BIT(pin % 32);
	int bank_base = AM18XX_GPIO_BANK_BASE +
			bank_group * AM18XX_GPIO_BANK_SIZE;

	/* GPIO input value reads are built to be atomic */
	uint32_t val = GPIO_READ(sc, bank_base + AM18XX_GPIO_BANK_INPUT);

	return (val & group_bit) != 0;
}

static void
am18xx_gpio_gp_pin_write(void *priv, int pin, int val)
{
	struct am18xx_gpio_softc * const sc = priv;

	KASSERT(pin >= 0 && pin < sc->sc_num_gpios);

	int bank_group = pin / 32;
	uint32_t group_bit = __BIT(pin % 32);
	int bank_base = AM18XX_GPIO_BANK_BASE +
			bank_group * AM18XX_GPIO_BANK_SIZE;

	/*
	 * We do *not* need the lock because BANK_SET/BANK_CLEAR is built to
	 * be set by multiple threads at once.
	 */
	if (val) {
		GPIO_WRITE(sc, bank_base + AM18XX_GPIO_BANK_SET, group_bit);
	} else {
		GPIO_WRITE(sc, bank_base + AM18XX_GPIO_BANK_CLEAR, group_bit);
	}
}

static void
am18xx_gpio_gp_pin_ctl(void *priv, int pin, int flags)
{
	struct am18xx_gpio_softc * const sc = priv;

	KASSERT(pin >= 0 && pin < sc->sc_num_gpios);

	int bank_group = pin / 32;
	uint32_t group_bit = __BIT(pin % 32);
	int bank_base = AM18XX_GPIO_BANK_BASE +
			bank_group * AM18XX_GPIO_BANK_SIZE;

	mutex_enter(&sc->sc_lock);
	if (flags & GPIO_PIN_OUTPUT) {
		uint32_t val = GPIO_READ(sc, bank_base + AM18XX_GPIO_BANK_DIR);
		val &= ~group_bit;
		GPIO_WRITE(sc, bank_base + AM18XX_GPIO_BANK_DIR, val);
	} else if (flags & GPIO_PIN_INPUT) {
		uint32_t val = GPIO_READ(sc, bank_base + AM18XX_GPIO_BANK_DIR);
		val |= group_bit;
		GPIO_WRITE(sc, bank_base + AM18XX_GPIO_BANK_DIR, val);
	}
	mutex_exit(&sc->sc_lock);
}

int
am18xx_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

void
am18xx_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct am18xx_gpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_bst = faa->faa_bst;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	if (fdtbus_clock_enable(phandle, "gpio", true) != 0) {
		aprint_error(": couldn't enable clock\n");
		return;
	}

	if (of_getprop_uint32(phandle, "ti,ngpio", &sc->sc_num_gpios) != 0) {
		aprint_error(": couldn't get gpio count\n");
		return;
	}

	gpio_pin_t *gpio_pin_desc = kmem_alloc(
	    sc->sc_num_gpios * sizeof(gpio_pin_t), KM_SLEEP);
	if (gpio_pin_desc == NULL) {
		aprint_error(": couldn't allocate memory\n");
		return;
	}
	memset(gpio_pin_desc, 0, sc->sc_num_gpios * sizeof(gpio_pin_t));
	for (int i = 0; i < sc->sc_num_gpios; i++) {
		gpio_pin_desc[i].pin_num = i;
		gpio_pin_desc[i].pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
					    GPIO_PIN_INOUT | GPIO_PIN_PUSHPULL;
		gpio_pin_desc[i].pin_gc = &sc->sc_gc;
	}

	sc->sc_gc.gp_cookie = sc;
	sc->sc_gc.gp_pin_read = am18xx_gpio_gp_pin_read;
	sc->sc_gc.gp_pin_write = am18xx_gpio_gp_pin_write;
	sc->sc_gc.gp_pin_ctl = am18xx_gpio_gp_pin_ctl;

	struct gpiobus_attach_args gpiobus_aa;
	gpiobus_aa.gba_gc = &sc->sc_gc;
	gpiobus_aa.gba_npins = sc->sc_num_gpios;
	gpiobus_aa.gba_pins = gpio_pin_desc;

	aprint_naive("\n");
	aprint_normal("\n");

	config_found(self, &gpiobus_aa, gpiobus_print, CFARGS_NONE);

	fdtbus_register_gpio_controller(self, phandle, &am18xx_gpio_fdt_funcs);
}

