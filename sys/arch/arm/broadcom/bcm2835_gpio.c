/*	$NetBSD: bcm2835_gpio.c,v 1.6 2017/12/10 21:38:26 skrll Exp $	*/

/*-
 * Copyright (c) 2013, 2014, 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jonathan A. Kollasch, Frank Kardel and Nick Hudson
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_gpio.c,v 1.6 2017/12/10 21:38:26 skrll Exp $");

/*
 * Driver for BCM2835 GPIO
 *
 * see: http://www.raspberrypi.org/wp-content/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <sys/bitops.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_gpioreg.h>

#include <dev/gpio/gpiovar.h>
#include <dev/fdt/fdtvar.h>

/* #define BCM2835_GPIO_DEBUG */
#ifdef BCM2835_GPIO_DEBUG
int bcm2835gpiodebug = 3;
#define DPRINTF(l, x)	do { if (l <= bcm2835gpiodebug) { printf x; } } while (0)
#else
#define DPRINTF(l, x)
#endif

#define	BCMGPIO_MAXPINS	54

struct bcmgpio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct gpio_chipset_tag	sc_gpio_gc;

	kmutex_t		sc_lock;
	gpio_pin_t		sc_gpio_pins[BCMGPIO_MAXPINS];
};

struct bcmgpio_pin {
	int			pin_no;
	u_int			pin_flags;
	bool			pin_actlo;
};


static int	bcmgpio_match(device_t, cfdata_t, void *);
static void	bcmgpio_attach(device_t, device_t, void *);

static int	bcm2835gpio_gpio_pin_read(void *, int);
static void	bcm2835gpio_gpio_pin_write(void *, int, int);
static void	bcm2835gpio_gpio_pin_ctl(void *, int, int);

u_int		bcm283x_pin_getfunc(const struct bcmgpio_softc * const, u_int);
void		bcm283x_pin_setfunc(const struct bcmgpio_softc * const, u_int,
		    u_int);
void		bcm283x_pin_setpull(const struct bcmgpio_softc * const, u_int,
		    u_int);

static int 	bcm283x_pinctrl_set_config(device_t, const void *, size_t);

static void *	bcmgpio_fdt_acquire(device_t, const void *, size_t, int);
static void	bcmgpio_fdt_release(device_t, void *);
static int	bcmgpio_fdt_read(device_t, void *, bool);
static void	bcmgpio_fdt_write(device_t, void *, int, bool);

static struct fdtbus_gpio_controller_func bcmgpio_funcs = {
	.acquire = bcmgpio_fdt_acquire,
	.release = bcmgpio_fdt_release,
	.read = bcmgpio_fdt_read,
	.write = bcmgpio_fdt_write
};

CFATTACH_DECL_NEW(bcmgpio, sizeof(struct bcmgpio_softc),
    bcmgpio_match, bcmgpio_attach, NULL, NULL);


static struct fdtbus_pinctrl_controller_func bcm283x_pinctrl_funcs = {
	.set_config = bcm283x_pinctrl_set_config,
};

static int
bcm283x_pinctrl_set_config(device_t dev, const void *data, size_t len)
{
	struct bcmgpio_softc * const sc = device_private(dev);

	if (len != 4)
		return -1;

	const int phandle = fdtbus_get_phandle_from_native(be32dec(data));

	/*
	 * Required: brcm,pins
	 * Optional: brcm,function, brcm,pull
	 */

	int pins_len;
	const u_int *pins = fdtbus_get_prop(phandle, "brcm,pins", &pins_len);

	if (pins == NULL)
		return -1;

	int pull_len = 0;
	const u_int *pull = fdtbus_get_prop(phandle, "brcm,pull", &pull_len);

	int func_len = 0;
	const u_int *func = fdtbus_get_prop(phandle, "brcm,function", &func_len);

	if (!pull && !func) {
		aprint_error_dev(dev, "one of brcm,pull or brcm,funcion must "
		    "be specified");
		return -1;
	}

	const int npins = pins_len / 4;
	const int npull = pull_len / 4;
	const int nfunc = func_len / 4;

	if (npull > 1 && npull != npins) {
		aprint_error_dev(dev, "brcm,pull must have 1 or %d entries",
		    npins);
		return -1;
	}
	if (nfunc > 1 && nfunc != npins) {
		aprint_error_dev(dev, "brcm,function must have 1 or %d entries",
		    npins);
		return -1;
	}

	mutex_enter(&sc->sc_lock);

	for (int i = 0; i < npins; i++) {
		const u_int pin = be32toh(pins[i]);

		if (pin > BCMGPIO_MAXPINS)
			continue;
		if (pull) {
			const int value = be32toh(pull[npull == 1 ? 0 : i]);
			bcm283x_pin_setpull(sc, pin, value);
		}
		if (func) {
			const int value = be32toh(func[nfunc == 1 ? 0 : i]);
			bcm283x_pin_setfunc(sc, pin, value);
		}
	}

	mutex_exit(&sc->sc_lock);

	return 0;
}

static int
bcmgpio_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "brcm,bcm2835-gpio", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
bcmgpio_attach(device_t parent, device_t self, void *aux)
{
	struct bcmgpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct gpiobus_attach_args gba;
	bus_addr_t addr;
	bus_size_t size;
	u_int func;
	int error;
	int pin;

	const int phandle = faa->faa_phandle;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal(": GPIO controller\n");

	sc->sc_iot = faa->faa_bst;
	error = bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh);
	if (error) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	for (pin = 0; pin < BCMGPIO_MAXPINS; pin++) {
		sc->sc_gpio_pins[pin].pin_num = pin;
		/*
		 * find out pins still available for GPIO
		 */
		func = bcm283x_pin_getfunc(sc, pin);

		if (func == BCM2835_GPIO_IN ||
		    func == BCM2835_GPIO_OUT) {
			sc->sc_gpio_pins[pin].pin_caps = GPIO_PIN_INPUT |
				GPIO_PIN_OUTPUT |
				GPIO_PIN_PUSHPULL | GPIO_PIN_TRISTATE |
				GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN;
			/* read initial state */
			sc->sc_gpio_pins[pin].pin_state =
				bcm2835gpio_gpio_pin_read(sc, pin);
			DPRINTF(1, ("%s: attach pin %d\n", device_xname(sc->sc_dev), pin));
		} else {
			sc->sc_gpio_pins[pin].pin_caps = 0;
			sc->sc_gpio_pins[pin].pin_state = 0;
  			DPRINTF(1, ("%s: skip pin %d - func = 0x%x\n", device_xname(sc->sc_dev), pin, func));
		}
	}

	/* create controller tag */
	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = bcm2835gpio_gpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = bcm2835gpio_gpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = bcm2835gpio_gpio_pin_ctl;

	gba.gba_gc = &sc->sc_gpio_gc;
	for (pin = 0; pin < BCMGPIO_MAXPINS;) {
		const int npins = MIN(BCMGPIO_MAXPINS - pin, 32);
		gba.gba_pins = &sc->sc_gpio_pins[pin];
		gba.gba_npins = npins;
		config_found_ia(self, "gpiobus", &gba, gpiobus_print);
		pin += npins;
	}

	fdtbus_register_gpio_controller(self, faa->faa_phandle, &bcmgpio_funcs);

	for (int child = OF_child(phandle); child; child = OF_peer(child)) {
		if (!of_hasprop(child, "brcm,pins"))
			continue;
		fdtbus_register_pinctrl_config(self, child,
		    &bcm283x_pinctrl_funcs);
	}

	fdtbus_pinctrl_configure();
}

/* GPIO support functions */
static int
bcm2835gpio_gpio_pin_read(void *arg, int pin)
{
	struct bcmgpio_softc *sc = arg;
	uint32_t val;
	int res;

	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		BCM2835_GPIO_GPLEV(pin / BCM2835_GPIO_GPLEV_PINS_PER_REGISTER));

	res = val & (1 << (pin % BCM2835_GPIO_GPLEV_PINS_PER_REGISTER)) ?
		GPIO_PIN_HIGH : GPIO_PIN_LOW;

	DPRINTF(2, ("%s: gpio_read pin %d->%d\n", device_xname(sc->sc_dev),
	    pin, (res == GPIO_PIN_HIGH)));

	return res;
}

static void
bcm2835gpio_gpio_pin_write(void *arg, int pin, int value)
{
	struct bcmgpio_softc *sc = arg;
	bus_size_t reg;

	if (value == GPIO_PIN_HIGH) {
		reg = BCM2835_GPIO_GPSET(pin / BCM2835_GPIO_GPSET_PINS_PER_REGISTER);
	} else {
		reg = BCM2835_GPIO_GPCLR(pin / BCM2835_GPIO_GPCLR_PINS_PER_REGISTER);
	}

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, reg,
	    1 << (pin % BCM2835_GPIO_GPSET_PINS_PER_REGISTER));

	DPRINTF(2, ("%s: gpio_write pin %d<-%d\n", device_xname(sc->sc_dev),
	    pin, (value == GPIO_PIN_HIGH)));
}


void
bcm283x_pin_setfunc(const struct bcmgpio_softc * const sc, u_int pin,
    u_int func)
{
	const u_int mask = (1 << BCM2835_GPIO_GPFSEL_BITS_PER_PIN) - 1;
	const u_int regid = (pin / BCM2835_GPIO_GPFSEL_PINS_PER_REGISTER);
	const u_int shift = (pin % BCM2835_GPIO_GPFSEL_PINS_PER_REGISTER) *
	    BCM2835_GPIO_GPFSEL_BITS_PER_PIN;
	uint32_t v;

	KASSERT(mutex_owned(&sc->sc_lock));
	KASSERT(func <= mask);

	v = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BCM2835_GPIO_GPFSEL(regid));

	if (((v >> shift) & mask) == func) {
		return;
	}

	DPRINTF(2, ("%s: gpio_write pin %d<-%d\n", device_xname(sc->sc_dev),
	    pin, func));

	v &= ~(mask << shift);
	v |=  (func << shift);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BCM2835_GPIO_GPFSEL(regid), v);
}

u_int
bcm283x_pin_getfunc(const struct bcmgpio_softc * const sc, u_int pin)
{
	const u_int mask = (1 << BCM2835_GPIO_GPFSEL_BITS_PER_PIN) - 1;
	const u_int regid = (pin / BCM2835_GPIO_GPFSEL_PINS_PER_REGISTER);
	const u_int shift = (pin % BCM2835_GPIO_GPFSEL_PINS_PER_REGISTER) *
	    BCM2835_GPIO_GPFSEL_BITS_PER_PIN;
	uint32_t v;

	v = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BCM2835_GPIO_GPFSEL(regid));

	return ((v >> shift) & mask);
}

void
bcm283x_pin_setpull(const struct bcmgpio_softc * const sc, u_int pin, u_int pud)
{

	KASSERT(mutex_owned(&sc->sc_lock));

	const u_int mask = 1 << (pin % BCM2835_GPIO_GPPUD_PINS_PER_REGISTER);
	const u_int regid = (pin / BCM2835_GPIO_GPPUD_PINS_PER_REGISTER);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BCM2835_GPIO_GPPUD, pud);
	delay(1);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BCM2835_GPIO_GPPUDCLK(regid), mask);
	delay(1);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BCM2835_GPIO_GPPUD, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BCM2835_GPIO_GPPUDCLK(regid), 0);
}


static void
bcm2835gpio_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct bcmgpio_softc *sc = arg;
	uint32_t cmd;

	DPRINTF(2, ("%s: gpio_ctl pin %d flags 0x%x\n", device_xname(sc->sc_dev), pin, flags));

	mutex_enter(&sc->sc_lock);
	if (flags & (GPIO_PIN_OUTPUT|GPIO_PIN_INPUT)) {
		if ((flags & GPIO_PIN_INPUT) || !(flags & GPIO_PIN_OUTPUT)) {
			/* for safety INPUT will overide output */
			bcm283x_pin_setfunc(sc, pin, BCM2835_GPIO_IN);
		} else {
			bcm283x_pin_setfunc(sc, pin, BCM2835_GPIO_OUT);
		}
	}

	if (flags & (GPIO_PIN_PULLUP|GPIO_PIN_PULLDOWN)) {
		cmd = (flags & GPIO_PIN_PULLUP) ?
			BCM2835_GPIO_GPPUD_PULLUP : BCM2835_GPIO_GPPUD_PULLDOWN;
	} else {
		cmd = BCM2835_GPIO_GPPUD_PULLOFF;
	}

	/* set up control signal */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BCM2835_GPIO_GPPUD, cmd);
	delay(1); /* wait 150 cycles */
	/* set clock signal */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
	    BCM2835_GPIO_GPPUDCLK(pin / BCM2835_GPIO_GPLEV_PINS_PER_REGISTER),
	    1 << (pin % BCM2835_GPIO_GPPUD_PINS_PER_REGISTER));
	delay(1); /* wait 150 cycles */
	/* reset control signal and clock */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
	    BCM2835_GPIO_GPPUD, BCM2835_GPIO_GPPUD_PULLOFF);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
	    BCM2835_GPIO_GPPUDCLK(pin / BCM2835_GPIO_GPLEV_PINS_PER_REGISTER),
	    0);
	mutex_exit(&sc->sc_lock);
}

static void *
bcmgpio_fdt_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct bcmgpio_softc *sc = device_private(dev);
	struct bcmgpio_pin *gpin;
	const u_int *gpio = data;

	if (len != 12)
		return NULL;

	const u_int pin = be32toh(gpio[1]);
	const bool actlo = be32toh(gpio[2]) & 1;

	if (pin >= BCMGPIO_MAXPINS)
		return NULL;

	gpin = kmem_alloc(sizeof(*gpin), KM_SLEEP);
	gpin->pin_no = pin;
	gpin->pin_flags = flags;
	gpin->pin_actlo = actlo;

	bcm2835gpio_gpio_pin_ctl(sc, gpin->pin_no, gpin->pin_flags);

	return gpin;
}

static void
bcmgpio_fdt_release(device_t dev, void *priv)
{
	struct bcmgpio_softc *sc = device_private(dev);
	struct bcmgpio_pin *gpin = priv;

	bcm2835gpio_gpio_pin_ctl(sc, gpin->pin_no, GPIO_PIN_INPUT);
	kmem_free(gpin, sizeof(*gpin));
}

static int
bcmgpio_fdt_read(device_t dev, void *priv, bool raw)
{
	struct bcmgpio_softc *sc = device_private(dev);
	struct bcmgpio_pin *gpin = priv;
	int val;

	val = bcm2835gpio_gpio_pin_read(sc, gpin->pin_no);

	if (!raw && gpin->pin_actlo)
		val = !val;

	return val;
}

static void
bcmgpio_fdt_write(device_t dev, void *priv, int val, bool raw)
{
	struct bcmgpio_softc *sc = device_private(dev);
	struct bcmgpio_pin *gpin = priv;

	if (!raw && gpin->pin_actlo)
		val = !val;

	bcm2835gpio_gpio_pin_write(sc, gpin->pin_no, val);
}
