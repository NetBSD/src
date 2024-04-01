/* $NetBSD: ti_gpio.c,v 1.15 2024/04/01 15:52:08 jakllsch Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: ti_gpio.c,v 1.15 2024/04/01 15:52:08 jakllsch Exp $");

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

#include <arm/ti/ti_prcm.h>

#define	TI_GPIO_NPINS			32

enum ti_gpio_type {
	TI_GPIO_OMAP3,
	TI_GPIO_OMAP4,
	TI_NGPIO
};

enum {
	GPIO_IRQSTATUS1,
	GPIO_IRQENABLE1,	/* OMAP3 */
	GPIO_IRQENABLE1_SET,	/* OMAP4 */
	GPIO_IRQENABLE1_CLR,	/* OMAP4 */
	GPIO_OE,
	GPIO_DATAIN,
	GPIO_DATAOUT,
	GPIO_LEVELDETECT0,
	GPIO_LEVELDETECT1,
	GPIO_RISINGDETECT,
	GPIO_FALLINGDETECT,
	GPIO_CLEARDATAOUT,
	GPIO_SETDATAOUT,
	GPIO_NREG
};

static const u_int ti_gpio_regmap[TI_NGPIO][GPIO_NREG] = {
	[TI_GPIO_OMAP3] = {
		[GPIO_IRQSTATUS1]	= 0x18,
		[GPIO_IRQENABLE1]	= 0x1c,
		[GPIO_OE]		= 0x34,
		[GPIO_DATAIN]		= 0x38,
		[GPIO_DATAOUT]		= 0x3c,
		[GPIO_LEVELDETECT0]	= 0x40,
		[GPIO_LEVELDETECT1]	= 0x44,
		[GPIO_RISINGDETECT]	= 0x48,
		[GPIO_FALLINGDETECT]	= 0x4c,
		[GPIO_CLEARDATAOUT]	= 0x90,
		[GPIO_SETDATAOUT]	= 0x94,
	},
	[TI_GPIO_OMAP4] = {
		[GPIO_IRQSTATUS1]	= 0x2c,
		[GPIO_IRQENABLE1_SET]	= 0x34,
		[GPIO_IRQENABLE1_CLR]	= 0x38,
		[GPIO_OE]		= 0x134,
		[GPIO_DATAIN]		= 0x138,
		[GPIO_DATAOUT]		= 0x13c,
		[GPIO_LEVELDETECT0]	= 0x140,
		[GPIO_LEVELDETECT1]	= 0x144,
		[GPIO_RISINGDETECT]	= 0x148,
		[GPIO_FALLINGDETECT]	= 0x14c,
		[GPIO_CLEARDATAOUT]	= 0x190,
		[GPIO_SETDATAOUT]	= 0x194,
	},
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,omap3-gpio",	.value = TI_GPIO_OMAP3 },
	{ .compat = "ti,omap4-gpio",	.value = TI_GPIO_OMAP4 },
	DEVICE_COMPAT_EOL
};

struct ti_gpio_intr {
	u_int intr_pin;
	int (*intr_func)(void *);
	void *intr_arg;
	bool intr_mpsafe;
};

struct ti_gpio_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	kmutex_t sc_lock;
	enum ti_gpio_type sc_type;
	const char *sc_modname;
	void *sc_ih;

	struct gpio_chipset_tag sc_gp;
	gpio_pin_t sc_pins[TI_GPIO_NPINS];
	bool sc_pinout[TI_GPIO_NPINS];
	struct ti_gpio_intr sc_intr[TI_GPIO_NPINS];
	device_t sc_gpiodev;
};

struct ti_gpio_pin {
	struct ti_gpio_softc *pin_sc;
	u_int pin_nr;
	int pin_flags;
	bool pin_actlo;
};

#define RD4(sc, reg) 		\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, ti_gpio_regmap[(sc)->sc_type][(reg)])
#define WR4(sc, reg, val) 	\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, ti_gpio_regmap[(sc)->sc_type][(reg)], (val))

static int	ti_gpio_match(device_t, cfdata_t, void *);
static void	ti_gpio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ti_gpio, sizeof(struct ti_gpio_softc),
	ti_gpio_match, ti_gpio_attach, NULL, NULL);

static int
ti_gpio_ctl(struct ti_gpio_softc *sc, u_int pin, int flags)
{
	uint32_t oe;

	KASSERT(mutex_owned(&sc->sc_lock));

	oe = RD4(sc, GPIO_OE);
	if (flags & GPIO_PIN_INPUT)
		oe |= __BIT(pin);
	else if (flags & GPIO_PIN_OUTPUT)
		oe &= ~__BIT(pin);
	WR4(sc, GPIO_OE, oe);

	sc->sc_pinout[pin] = (flags & GPIO_PIN_OUTPUT) != 0;

	return 0;
}

static void *
ti_gpio_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct ti_gpio_softc * const sc = device_private(dev);
	struct ti_gpio_pin *gpin;
	const u_int *gpio = data;
	int error;

	if (len != 12)
		return NULL;

	const uint8_t pin = be32toh(gpio[1]) & 0xff;
	const bool actlo = be32toh(gpio[2]) & 1;

	if (pin >= __arraycount(sc->sc_pins))
		return NULL;

	mutex_enter(&sc->sc_lock);
	error = ti_gpio_ctl(sc, pin, flags);
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
ti_gpio_release(device_t dev, void *priv)
{
	struct ti_gpio_softc * const sc = device_private(dev);
	struct ti_gpio_pin *pin = priv;

	mutex_enter(&sc->sc_lock);
	ti_gpio_ctl(pin->pin_sc, pin->pin_nr, GPIO_PIN_INPUT);
	mutex_exit(&sc->sc_lock);

	kmem_free(pin, sizeof(*pin));
}

static int
ti_gpio_read(device_t dev, void *priv, bool raw)
{
	struct ti_gpio_softc * const sc = device_private(dev);
	struct ti_gpio_pin *pin = priv;
	uint32_t data;
	int val;

	KASSERT(sc == pin->pin_sc);

	const uint32_t data_mask = __BIT(pin->pin_nr);

	/* No lock required for reads */
	if (sc->sc_pinout[pin->pin_nr])
		data = RD4(sc, GPIO_DATAOUT);
	else
		data = RD4(sc, GPIO_DATAIN);
	val = __SHIFTOUT(data, data_mask);
	if (!raw && pin->pin_actlo)
		val = !val;

	return val;
}

static void
ti_gpio_write(device_t dev, void *priv, int val, bool raw)
{
	struct ti_gpio_softc * const sc = device_private(dev);
	struct ti_gpio_pin *pin = priv;

	KASSERT(sc == pin->pin_sc);

	const uint32_t data_mask = __BIT(pin->pin_nr);

	if (!raw && pin->pin_actlo)
		val = !val;

	const u_int data_reg = val ? GPIO_SETDATAOUT : GPIO_CLEARDATAOUT;

	WR4(sc, data_reg, data_mask);
}

static struct fdtbus_gpio_controller_func ti_gpio_funcs = {
	.acquire = ti_gpio_acquire,
	.release = ti_gpio_release,
	.read = ti_gpio_read,
	.write = ti_gpio_write,
};

static void
ti_gpio_intr_disable(struct ti_gpio_softc * const sc, struct ti_gpio_intr * const intr)
{
	const u_int pin = intr->intr_pin;
	const uint32_t pin_mask = __BIT(pin);
	uint32_t val;

	/* Disable interrupts */
	if (sc->sc_type == TI_GPIO_OMAP3) {
		val = RD4(sc, GPIO_IRQENABLE1);
		WR4(sc, GPIO_IRQENABLE1, val & ~pin_mask);
	} else {
		WR4(sc, GPIO_IRQENABLE1_CLR, pin_mask);
	}

	intr->intr_func = NULL;
	intr->intr_arg = NULL;
	intr->intr_mpsafe = false;
}

static void *
ti_gpio_intr_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct ti_gpio_softc * const sc = device_private(dev);
	uint32_t val;

	/* 1st cell is the pin */
	/* 2nd cell is flags */
	const u_int pin = be32toh(specifier[0]);
	const u_int type = be32toh(specifier[2]) & 0xf;

	if (ipl != IPL_VM || pin >= __arraycount(sc->sc_pins))
		return NULL;

	/*
	 * Enabling both high and low level triggers will cause the GPIO
	 * controller to always assert the interrupt.
	 */
	if ((type & (FDT_INTR_TYPE_LOW_LEVEL|FDT_INTR_TYPE_HIGH_LEVEL)) ==
	    (FDT_INTR_TYPE_LOW_LEVEL|FDT_INTR_TYPE_HIGH_LEVEL))
		return NULL;

	if (sc->sc_intr[pin].intr_func != NULL)
		return NULL;

	/* Set pin as input */
	if (ti_gpio_ctl(sc, pin, GPIO_PIN_INPUT) != 0)
		return NULL;

	sc->sc_intr[pin].intr_pin = pin;
	sc->sc_intr[pin].intr_func = func;
	sc->sc_intr[pin].intr_arg = arg;
	sc->sc_intr[pin].intr_mpsafe = (flags & FDT_INTR_MPSAFE) != 0;

	const uint32_t pin_mask = __BIT(pin);

	/* Configure triggers */
	val = RD4(sc, GPIO_LEVELDETECT0);
	if ((type & FDT_INTR_TYPE_LOW_LEVEL) != 0)
		val |= pin_mask;
	else
		val &= ~pin_mask;
	WR4(sc, GPIO_LEVELDETECT0, val);

	val = RD4(sc, GPIO_LEVELDETECT1);
	if ((type & FDT_INTR_TYPE_HIGH_LEVEL) != 0)
		val |= pin_mask;
	else
		val &= ~pin_mask;
	WR4(sc, GPIO_LEVELDETECT1, val);

	val = RD4(sc, GPIO_RISINGDETECT);
	if ((type & FDT_INTR_TYPE_POS_EDGE) != 0)
		val |= pin_mask;
	else
		val &= ~pin_mask;
	WR4(sc, GPIO_RISINGDETECT, val);

	val = RD4(sc, GPIO_FALLINGDETECT);
	if ((type & FDT_INTR_TYPE_NEG_EDGE) != 0)
		val |= pin_mask;
	else
		val &= ~pin_mask;
	WR4(sc, GPIO_FALLINGDETECT, val);

	/* Enable interrupts */
	if (sc->sc_type == TI_GPIO_OMAP3) {
		val = RD4(sc, GPIO_IRQENABLE1);
		WR4(sc, GPIO_IRQENABLE1, val | pin_mask);
	} else {
		WR4(sc, GPIO_IRQENABLE1_SET, pin_mask);
	}

	return &sc->sc_intr[pin];
}

static void
ti_gpio_intr_disestablish(device_t dev, void *ih)
{
	struct ti_gpio_softc * const sc = device_private(dev);
	struct ti_gpio_intr * const intr = ih;
	
	ti_gpio_intr_disable(sc, intr);
}

static bool
ti_gpio_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	struct ti_gpio_softc * const sc = device_private(dev);

	/* 1st cell is the pin */
	/* 2nd cell is flags */
	const u_int pin = be32toh(specifier[0]);

	if (pin >= __arraycount(sc->sc_pins))
		return false;

	snprintf(buf, buflen, "%s pin %d", sc->sc_modname, pin);
	return true;
}

static struct fdtbus_interrupt_controller_func ti_gpio_intrfuncs = {
	.establish = ti_gpio_intr_establish,
	.disestablish = ti_gpio_intr_disestablish,
	.intrstr = ti_gpio_intrstr,
};

static int
ti_gpio_pin_read(void *priv, int pin)
{
	struct ti_gpio_softc * const sc = priv;
	uint32_t data;
	int val;

	KASSERT(pin < __arraycount(sc->sc_pins));

	const uint32_t data_mask = __BIT(pin);

	data = RD4(sc, GPIO_DATAIN);
	val = __SHIFTOUT(data, data_mask);

	return val;
}

static void
ti_gpio_pin_write(void *priv, int pin, int val)
{
	struct ti_gpio_softc * const sc = priv;

	KASSERT(pin < __arraycount(sc->sc_pins));

	const u_int data_reg = val ? GPIO_SETDATAOUT : GPIO_CLEARDATAOUT;
	const uint32_t data_mask = __BIT(pin);

	WR4(sc, data_reg, data_mask);
}

static void
ti_gpio_pin_ctl(void *priv, int pin, int flags)
{
	struct ti_gpio_softc * const sc = priv;

	KASSERT(pin < __arraycount(sc->sc_pins));

	mutex_enter(&sc->sc_lock);
	ti_gpio_ctl(sc, pin, flags);
	mutex_exit(&sc->sc_lock);
}

static void *
ti_gpio_gp_intr_establish(void *vsc, int pin, int ipl, int irqmode,
    int (*func)(void *), void *arg)
{
	struct ti_gpio_softc * const sc = vsc;
	uint32_t val;

	if (ipl != IPL_VM || pin < 0 || pin >= __arraycount(sc->sc_pins))
		return NULL;

	if (sc->sc_intr[pin].intr_func != NULL)
		return NULL;

	/*
	 * Enabling both high and low level triggers will cause the GPIO
	 * controller to always assert the interrupt.
	 */
	if ((irqmode & (GPIO_INTR_LOW_LEVEL|GPIO_INTR_HIGH_LEVEL)) ==
	    (GPIO_INTR_LOW_LEVEL|GPIO_INTR_HIGH_LEVEL))
		return NULL;

	/* Set pin as input */
	mutex_enter(&sc->sc_lock);
	if (ti_gpio_ctl(sc, pin, GPIO_PIN_INPUT) != 0) {
		mutex_exit(&sc->sc_lock);
		return NULL;
	}

	sc->sc_intr[pin].intr_pin = pin;
	sc->sc_intr[pin].intr_func = func;
	sc->sc_intr[pin].intr_arg = arg;
	sc->sc_intr[pin].intr_mpsafe = (irqmode & GPIO_INTR_MPSAFE) != 0;

	const uint32_t pin_mask = __BIT(pin);

	/* Configure triggers */
	val = RD4(sc, GPIO_LEVELDETECT0);
	if ((irqmode & GPIO_INTR_LOW_LEVEL) != 0)
		val |= pin_mask;
	else
		val &= ~pin_mask;
	WR4(sc, GPIO_LEVELDETECT0, val);

	val = RD4(sc, GPIO_LEVELDETECT1);
	if ((irqmode & GPIO_INTR_HIGH_LEVEL) != 0)
		val |= pin_mask;
	else
		val &= ~pin_mask;
	WR4(sc, GPIO_LEVELDETECT1, val);

	val = RD4(sc, GPIO_RISINGDETECT);
	if ((irqmode & GPIO_INTR_POS_EDGE) != 0 ||
	    (irqmode & GPIO_INTR_DOUBLE_EDGE) != 0)
		val |= pin_mask;
	else
		val &= ~pin_mask;
	WR4(sc, GPIO_RISINGDETECT, val);

	val = RD4(sc, GPIO_FALLINGDETECT);
	if ((irqmode & GPIO_INTR_NEG_EDGE) != 0 ||
	    (irqmode & GPIO_INTR_DOUBLE_EDGE) != 0)
		val |= pin_mask;
	else
		val &= ~pin_mask;
	WR4(sc, GPIO_FALLINGDETECT, val);

	/* Enable interrupts */
	if (sc->sc_type == TI_GPIO_OMAP3) {
		val = RD4(sc, GPIO_IRQENABLE1);
		WR4(sc, GPIO_IRQENABLE1, val | pin_mask);
	} else {
		WR4(sc, GPIO_IRQENABLE1_SET, pin_mask);
	}

	mutex_exit(&sc->sc_lock);
	
	return &sc->sc_intr[pin];
}

static void
ti_gpio_gp_intr_disestablish(void *vsc, void *ih)
{
	struct ti_gpio_softc * const sc = vsc;
	struct ti_gpio_intr * const intr = ih;

	ti_gpio_intr_disable(sc, intr);
}

static bool
ti_gpio_gp_intrstr(void *vsc, int pin, int irqmode, char *buf, size_t buflen)
{
	struct ti_gpio_softc * const sc = vsc;

	if (pin < 0 || pin >= TI_GPIO_NPINS)
		return false;

	snprintf(buf, buflen, "%s pin %d", sc->sc_modname, pin);
	return true;
}

static void
ti_gpio_attach_ports(struct ti_gpio_softc *sc)
{
	struct gpio_chipset_tag *gp = &sc->sc_gp;
	struct gpiobus_attach_args gba;
	u_int pin;

	gp->gp_cookie = sc;
	gp->gp_pin_read = ti_gpio_pin_read;
	gp->gp_pin_write = ti_gpio_pin_write;
	gp->gp_pin_ctl = ti_gpio_pin_ctl;
	gp->gp_intr_establish = ti_gpio_gp_intr_establish;
	gp->gp_intr_disestablish = ti_gpio_gp_intr_disestablish;
	gp->gp_intr_str = ti_gpio_gp_intrstr;

	for (pin = 0; pin < __arraycount(sc->sc_pins); pin++) {
		sc->sc_pins[pin].pin_num = pin;
		sc->sc_pins[pin].pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT;
		sc->sc_pins[pin].pin_intrcaps =
		    GPIO_INTR_POS_EDGE | GPIO_INTR_NEG_EDGE |
		    GPIO_INTR_DOUBLE_EDGE | GPIO_INTR_HIGH_LEVEL |
		    GPIO_INTR_LOW_LEVEL | GPIO_INTR_MPSAFE;
		sc->sc_pins[pin].pin_state = ti_gpio_pin_read(sc, pin);
	}

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = gp;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = __arraycount(sc->sc_pins);
	sc->sc_gpiodev = config_found(sc->sc_dev, &gba, NULL, CFARGS_NONE);
}

static int
ti_gpio_intr(void *priv)
{
	struct ti_gpio_softc * const sc = priv;
	uint32_t status;
	u_int bit;
	int rv = 0;

	status = RD4(sc, GPIO_IRQSTATUS1);
	WR4(sc, GPIO_IRQSTATUS1, status);

	while ((bit = ffs32(status)) != 0) {
		const u_int pin = bit - 1;
		const uint32_t pin_mask = __BIT(pin);
		struct ti_gpio_intr *intr = &sc->sc_intr[pin];
		status &= ~pin_mask;
		if (intr->intr_func == NULL)
			continue;
		if (!intr->intr_mpsafe)
			KERNEL_LOCK(1, curlwp);
		rv |= intr->intr_func(intr->intr_arg);
		if (!intr->intr_mpsafe)
			KERNEL_UNLOCK_ONE(curlwp);
	}

	return rv;
}

static int
ti_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
ti_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct ti_gpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": couldn't decode interrupt\n");
		return;
	}
	if (ti_prcm_enable_hwmod(phandle, 0) != 0) {
		aprint_error(": couldn't enable module\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_type = of_compatible_lookup(phandle, compat_data)->value;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	sc->sc_modname = fdtbus_get_string(phandle, "ti,hwmods");
	if (sc->sc_modname == NULL)
		sc->sc_modname = fdtbus_get_string(OF_parent(phandle), "ti,hwmods");
	if (sc->sc_modname == NULL)
		sc->sc_modname = kmem_asprintf("gpio@%" PRIxBUSADDR, addr);

	aprint_naive("\n");
	aprint_normal(": GPIO (%s)\n", sc->sc_modname);

	fdtbus_register_gpio_controller(self, phandle, &ti_gpio_funcs);

	ti_gpio_attach_ports(sc);

	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_VM,
	    FDT_INTR_MPSAFE, ti_gpio_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
	fdtbus_register_interrupt_controller(self, phandle, &ti_gpio_intrfuncs);
}
