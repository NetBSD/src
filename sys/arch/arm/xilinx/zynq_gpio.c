/* $NetBSD: zynq_gpio.c,v 1.4 2022/10/31 23:04:50 jmcneill Exp $ */

/*-
 * Copyright (c) 2022 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: zynq_gpio.c,v 1.4 2022/10/31 23:04:50 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bitops.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/mutex.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>
#include <dev/gpio/gpiovar.h>

#define	ZYNQ_GPIO_NPINS		(4 * 32)

#define	MASK_DATA_REG(pin)	(0x000 + 0x4 * ((pin) / 16))
#define	DATA_RO_REG(pin)	(0x060 + 0x4 * ((pin) / 32))
#define	DATA_RO_BIT(pin)	__BIT((pin) % 32)
#define	DIRM_REG(pin)		(0x204 + 0x40 * ((pin) / 32))
#define	DIRM_BIT(pin)		__BIT((pin) % 32)
#define	OEN_REG(pin)		(0x208 + 0x40 * ((pin) / 32))
#define	OEN_BIT(pin)		__BIT((pin) % 32)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "xlnx,zynq-gpio-1.0" },
	DEVICE_COMPAT_EOL
};

struct zynq_gpio_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	kmutex_t sc_lock;
	struct gpio_chipset_tag sc_gp;
	gpio_pin_t sc_pins[ZYNQ_GPIO_NPINS];
	device_t sc_gpiodev;
};

struct zynq_gpio_pin {
	struct zynq_gpio_softc *pin_sc;
	u_int pin_nr;
	int pin_flags;
	bool pin_actlo;
};

#define RD4(sc, reg) 		\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val) 	\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	zynq_gpio_match(device_t, cfdata_t, void *);
static void	zynq_gpio_attach(device_t, device_t, void *);

static int	zynq_gpio_pin_read(void *, int);
static void	zynq_gpio_pin_write(void *, int, int);

CFATTACH_DECL_NEW(zynqgpio, sizeof(struct zynq_gpio_softc),
	zynq_gpio_match, zynq_gpio_attach, NULL, NULL);

static int
zynq_gpio_ctl(struct zynq_gpio_softc *sc, u_int pin, int flags)
{
	uint32_t dirm, oen;

	KASSERT(mutex_owned(&sc->sc_lock));

	dirm = RD4(sc, DIRM_REG(pin));
	oen = RD4(sc, OEN_REG(pin));
	if ((flags & GPIO_PIN_INPUT) != 0) {
		dirm &= ~DIRM_BIT(pin);
		oen &= ~OEN_BIT(pin);
	} else if ((flags & GPIO_PIN_OUTPUT) != 0) {
		dirm |= DIRM_BIT(pin);
		oen |= OEN_BIT(pin);
	}
	WR4(sc, OEN_REG(pin), oen);
	WR4(sc, DIRM_REG(pin), dirm);

	return 0;
}

static void *
zynq_gpio_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct zynq_gpio_softc * const sc = device_private(dev);
	struct zynq_gpio_pin *gpin;
	const u_int *gpio = data;
	int error;

	if (len != 12)
		return NULL;

	const uint8_t pin = be32toh(gpio[1]) & 0xff;
	const bool actlo = be32toh(gpio[2]) & 1;

	if (pin >= __arraycount(sc->sc_pins))
		return NULL;

	mutex_enter(&sc->sc_lock);
	error = zynq_gpio_ctl(sc, pin, flags);
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
zynq_gpio_release(device_t dev, void *priv)
{
	struct zynq_gpio_softc * const sc = device_private(dev);
	struct zynq_gpio_pin *pin = priv;

	mutex_enter(&sc->sc_lock);
	zynq_gpio_ctl(pin->pin_sc, pin->pin_nr, GPIO_PIN_INPUT);
	mutex_exit(&sc->sc_lock);

	kmem_free(pin, sizeof(*pin));
}

static int
zynq_gpio_read(device_t dev, void *priv, bool raw)
{
	struct zynq_gpio_softc * const sc = device_private(dev);
	struct zynq_gpio_pin *pin = priv;
	int val;

	KASSERT(sc == pin->pin_sc);

	val = zynq_gpio_pin_read(sc, pin->pin_nr);
	if (!raw && pin->pin_actlo)
		val = !val;

	return val;
}

static void
zynq_gpio_write(device_t dev, void *priv, int val, bool raw)
{
	struct zynq_gpio_softc * const sc = device_private(dev);
	struct zynq_gpio_pin *pin = priv;

	KASSERT(sc == pin->pin_sc);

	if (!raw && pin->pin_actlo)
		val = !val;

	zynq_gpio_pin_write(sc, pin->pin_nr, val);
}

static struct fdtbus_gpio_controller_func zynq_gpio_funcs = {
	.acquire = zynq_gpio_acquire,
	.release = zynq_gpio_release,
	.read = zynq_gpio_read,
	.write = zynq_gpio_write,
};

static int
zynq_gpio_pin_read(void *priv, int pin)
{
	struct zynq_gpio_softc * const sc = priv;
	uint32_t data;
	int val;

	KASSERT(pin < __arraycount(sc->sc_pins));

	data = RD4(sc, DATA_RO_REG(pin));
	val = __SHIFTOUT(data, DATA_RO_BIT(pin));

	return val;
}

static void
zynq_gpio_pin_write(void *priv, int pin, int val)
{
	struct zynq_gpio_softc * const sc = priv;
	uint32_t mask_data;

	KASSERT(pin < __arraycount(sc->sc_pins));

	mask_data = (0xffff & ~__BIT(pin % 16)) << 16;
	if (val) {
		mask_data |= __BIT(pin % 16);
	}
	WR4(sc, MASK_DATA_REG(pin), mask_data);
}

static void
zynq_gpio_pin_ctl(void *priv, int pin, int flags)
{
	struct zynq_gpio_softc * const sc = priv;

	KASSERT(pin < __arraycount(sc->sc_pins));

	mutex_enter(&sc->sc_lock);
	zynq_gpio_ctl(sc, pin, flags);
	mutex_exit(&sc->sc_lock);
}

static void
zynq_gpio_attach_ports(struct zynq_gpio_softc *sc)
{
	struct gpio_chipset_tag *gp = &sc->sc_gp;
	struct gpiobus_attach_args gba;
	u_int pin;

	gp->gp_cookie = sc;
	gp->gp_pin_read = zynq_gpio_pin_read;
	gp->gp_pin_write = zynq_gpio_pin_write;
	gp->gp_pin_ctl = zynq_gpio_pin_ctl;

	for (pin = 0; pin < __arraycount(sc->sc_pins); pin++) {
		sc->sc_pins[pin].pin_num = pin;
		sc->sc_pins[pin].pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT;
		sc->sc_pins[pin].pin_state = zynq_gpio_pin_read(sc, pin);
	}

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = gp;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = __arraycount(sc->sc_pins);
	sc->sc_gpiodev = config_found(sc->sc_dev, &gba, NULL, CFARGS_NONE);
}

static int
zynq_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
zynq_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct zynq_gpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
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
	aprint_normal(": XGPIOPS\n");

	fdtbus_register_gpio_controller(self, phandle, &zynq_gpio_funcs);

	zynq_gpio_attach_ports(sc);
}
