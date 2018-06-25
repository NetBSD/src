/* $NetBSD: rk_gpio.c,v 1.2.2.2 2018/06/25 07:25:39 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rk_gpio.c,v 1.2.2.2 2018/06/25 07:25:39 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/gpio.h>
#include <sys/bitops.h>
#include <sys/lwp.h>

#include <dev/fdt/fdtvar.h>
#include <dev/gpio/gpiovar.h>

#define	GPIO_SWPORTA_DR_REG		0x0000
#define	GPIO_SWPORTA_DDR_REG		0x0004
#define	GPIO_INTEN_REG			0x0030
#define	GPIO_INTMASK_REG		0x0034
#define	GPIO_INTTYPE_LEVEL_REG		0x0038
#define	GPIO_INT_POLARITY_REG		0x003c
#define	GPIO_INT_STATUS_REG		0x0040
#define	GPIO_INT_RAWSTATUS_REG		0x0044
#define	GPIO_DEBOUNCE_REG		0x0048
#define	GPIO_PORTA_EOI_REG		0x004c
#define	GPIO_EXT_PORTA_REG		0x0050
#define	GPIO_LS_SYNC_REG		0x0060

static const char * const compatible[] = {
	"rockchip,gpio-bank",
	NULL
};

struct rk_gpio_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	kmutex_t sc_lock;

	struct gpio_chipset_tag sc_gp;
	gpio_pin_t sc_pins[32];
	device_t sc_gpiodev;
};

struct rk_gpio_pin {
	struct rk_gpio_softc *pin_sc;
	u_int pin_nr;
	int pin_flags;
	bool pin_actlo;
};

#define RD4(sc, reg) 		\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val) 	\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	rk_gpio_match(device_t, cfdata_t, void *);
static void	rk_gpio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(rk_gpio, sizeof(struct rk_gpio_softc),
	rk_gpio_match, rk_gpio_attach, NULL, NULL);

static int
rk_gpio_ctl(struct rk_gpio_softc *sc, u_int pin, int flags)
{
	uint32_t ddr;

	KASSERT(mutex_owned(&sc->sc_lock));

	ddr = RD4(sc, GPIO_SWPORTA_DDR_REG);
	if (flags & GPIO_PIN_INPUT)
		ddr &= ~__BIT(pin);
	else if (flags & GPIO_PIN_OUTPUT)
		ddr |= __BIT(pin);
	WR4(sc, GPIO_SWPORTA_DDR_REG, ddr);

	return 0;
}

static void *
rk_gpio_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct rk_gpio_softc * const sc = device_private(dev);
	struct rk_gpio_pin *gpin;
	const u_int *gpio = data;
	int error;

	if (len != 12)
		return NULL;

	const uint8_t pin = be32toh(gpio[1]) & 0xff;
	const bool actlo = be32toh(gpio[2]) & 1;

	if (pin >= __arraycount(sc->sc_pins))
		return NULL;

	mutex_enter(&sc->sc_lock);
	error = rk_gpio_ctl(sc, pin, flags);
	mutex_exit(&sc->sc_lock);

	if (error != 0)
		return NULL;

	gpin = kmem_zalloc(sizeof(*gpin), KM_SLEEP);
	gpin->pin_sc = sc;
	gpin->pin_nr = pin;
	gpin->pin_flags = flags;
	gpin->pin_actlo = actlo;

	return gpin;
}

static void
rk_gpio_release(device_t dev, void *priv)
{
	struct rk_gpio_softc * const sc = device_private(dev);
	struct rk_gpio_pin *pin = priv;

	mutex_enter(&sc->sc_lock);
	rk_gpio_ctl(pin->pin_sc, pin->pin_nr, GPIO_PIN_INPUT);
	mutex_exit(&sc->sc_lock);

	kmem_free(pin, sizeof(*pin));
}

static int
rk_gpio_read(device_t dev, void *priv, bool raw)
{
	struct rk_gpio_softc * const sc = device_private(dev);
	struct rk_gpio_pin *pin = priv;
	uint32_t data;
	int val;

	KASSERT(sc == pin->pin_sc);

	const uint32_t data_mask = __BIT(pin->pin_nr);

	/* No lock required for reads */
	data = RD4(sc, GPIO_EXT_PORTA_REG);
	val = __SHIFTOUT(data, data_mask);
	if (!raw && pin->pin_actlo)
		val = !val;

	return val;
}

static void
rk_gpio_write(device_t dev, void *priv, int val, bool raw)
{
	struct rk_gpio_softc * const sc = device_private(dev);
	struct rk_gpio_pin *pin = priv;
	uint32_t data;

	KASSERT(sc == pin->pin_sc);

	const uint32_t data_mask = __BIT(pin->pin_nr);

	if (!raw && pin->pin_actlo)
		val = !val;

	mutex_enter(&sc->sc_lock);
	data = RD4(sc, GPIO_SWPORTA_DR_REG);
	if (val)
		data |= data_mask;
	else
		data &= ~data_mask;
	WR4(sc, GPIO_SWPORTA_DR_REG, data);
	mutex_exit(&sc->sc_lock);
}

static struct fdtbus_gpio_controller_func rk_gpio_funcs = {
	.acquire = rk_gpio_acquire,
	.release = rk_gpio_release,
	.read = rk_gpio_read,
	.write = rk_gpio_write,
};

static int
rk_gpio_pin_read(void *priv, int pin)
{
	struct rk_gpio_softc * const sc = priv;
	uint32_t data;
	int val;

	KASSERT(pin < __arraycount(sc->sc_pins));

	const uint32_t data_mask = __BIT(pin);

	/* No lock required for reads */
	data = RD4(sc, GPIO_EXT_PORTA_REG);
	val = __SHIFTOUT(data, data_mask);

	return val;
}

static void
rk_gpio_pin_write(void *priv, int pin, int val)
{
	struct rk_gpio_softc * const sc = priv;
	uint32_t data;

	KASSERT(pin < __arraycount(sc->sc_pins));

	const uint32_t data_mask = __BIT(pin);

	mutex_enter(&sc->sc_lock);
	data = RD4(sc, GPIO_SWPORTA_DR_REG);
	if (val)
		data |= data_mask;
	else
		data &= ~data_mask;
	WR4(sc, GPIO_SWPORTA_DR_REG, data);
	mutex_exit(&sc->sc_lock);
}

static void
rk_gpio_pin_ctl(void *priv, int pin, int flags)
{
	struct rk_gpio_softc * const sc = priv;

	KASSERT(pin < __arraycount(sc->sc_pins));

	mutex_enter(&sc->sc_lock);
	rk_gpio_ctl(sc, pin, flags);
	mutex_exit(&sc->sc_lock);
}

static void
rk_gpio_attach_ports(struct rk_gpio_softc *sc)
{
	struct gpio_chipset_tag *gp = &sc->sc_gp;
	struct gpiobus_attach_args gba;
	u_int pin;

	gp->gp_cookie = sc;
	gp->gp_pin_read = rk_gpio_pin_read;
	gp->gp_pin_write = rk_gpio_pin_write;
	gp->gp_pin_ctl = rk_gpio_pin_ctl;

	for (pin = 0; pin < __arraycount(sc->sc_pins); pin++) {
		sc->sc_pins[pin].pin_num = pin;
		sc->sc_pins[pin].pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT;
		sc->sc_pins[pin].pin_state = rk_gpio_pin_read(sc, pin);
	}

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = gp;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = __arraycount(sc->sc_pins);
	sc->sc_gpiodev = config_found_ia(sc->sc_dev, "gpiobus", &gba, NULL);
}

static int
rk_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
rk_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct rk_gpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if ((clk = fdtbus_clock_get_index(phandle, 0)) == NULL || clk_enable(clk) != 0) {
		aprint_error(": couldn't enable clock\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	aprint_naive("\n");
	aprint_normal(": GPIO (%s)\n", fdtbus_get_string(phandle, "name"));

	fdtbus_register_gpio_controller(self, phandle, &rk_gpio_funcs);

	rk_gpio_attach_ports(sc);
}
