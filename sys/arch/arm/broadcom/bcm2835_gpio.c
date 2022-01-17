/*	$NetBSD: bcm2835_gpio.c,v 1.24 2022/01/17 19:38:14 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_gpio.c,v 1.24 2022/01/17 19:38:14 thorpej Exp $");

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
#include <sys/proc.h>
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

#define BCM2835_GPIO_MAXPINS 54
#define BCM2838_GPIO_MAXPINS 58
#define	BCMGPIO_MAXPINS	BCM2838_GPIO_MAXPINS

struct bcmgpio_eint {
	 int			(*eint_func)(void *);
	 void			*eint_arg;
	 int			eint_flags;
	 int			eint_bank;
	 int			eint_num;
};

#define	BCMGPIO_INTR_POS_EDGE	0x01
#define	BCMGPIO_INTR_NEG_EDGE	0x02
#define	BCMGPIO_INTR_HIGH_LEVEL	0x04
#define	BCMGPIO_INTR_LOW_LEVEL	0x08
#define	BCMGPIO_INTR_MPSAFE	0x10

struct bcmgpio_softc;
struct bcmgpio_bank {
	struct bcmgpio_softc	*sc_bcm;
	void			*sc_ih;
	struct bcmgpio_eint	sc_eint[32];
	int			sc_bankno;
};
#define	BCMGPIO_NBANKS	2

struct bcmgpio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct gpio_chipset_tag	sc_gpio_gc;

	kmutex_t		sc_lock;
	gpio_pin_t		sc_gpio_pins[BCMGPIO_MAXPINS];

	/* For interrupt support. */
	struct bcmgpio_bank	sc_banks[BCMGPIO_NBANKS];

	bool			sc_is2835;	/* for pullup on 2711 */
	u_int			sc_maxpins;
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

static void *	bcmgpio_gpio_intr_establish(void *, int, int, int,
					    int (*)(void *), void *);
static void	bcmgpio_gpio_intr_disestablish(void *, void *);
static bool	bcmgpio_gpio_intrstr(void *, int, int, char *, size_t);

static int	bcmgpio_intr(void *);

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

static void *	bcmgpio_fdt_intr_establish(device_t, u_int *, int, int,
		    int (*func)(void *), void *, const char *);
static void	bcmgpio_fdt_intr_disestablish(device_t, void *);
static bool	bcmgpio_fdt_intrstr(device_t, u_int *, char *, size_t);

static struct fdtbus_interrupt_controller_func bcmgpio_fdt_intrfuncs = {
	.establish = bcmgpio_fdt_intr_establish,
	.disestablish = bcmgpio_fdt_intr_disestablish,
	.intrstr = bcmgpio_fdt_intrstr,
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
		aprint_error_dev(dev, "one of brcm,pull or brcm,function must "
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

		if (pin >= sc->sc_maxpins)
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

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "brcm,bcm2835-gpio" },
	{ .compat = "brcm,bcm2838-gpio" },
	{ .compat = "brcm,bcm2711-gpio" },
	DEVICE_COMPAT_EOL
};

static int
bcmgpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
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
	int bank;
	uint32_t reg;

	const int phandle = faa->faa_phandle;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;

	sc->sc_iot = faa->faa_bst;
	error = bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh);
	if (error) {
		aprint_error_dev(self, ": couldn't map registers\n");
		return;
	}

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	/* BCM2835, BCM2836, BCM2837 return 'gpio' in this unused register */
	reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BCM2838_GPIO_GPPUPPDN(3));
	sc->sc_is2835 = reg == 0x6770696f;
	sc->sc_maxpins = sc->sc_is2835 ? BCM2835_GPIO_MAXPINS
	                               : BCM2838_GPIO_MAXPINS;

	aprint_naive("\n");
	aprint_normal(": GPIO controller %s\n", sc->sc_is2835 ? "2835" : "2838");

	for (pin = 0; pin < sc->sc_maxpins; pin++) {
		sc->sc_gpio_pins[pin].pin_num = pin;
		/*
		 * find out pins still available for GPIO
		 */
		func = bcm283x_pin_getfunc(sc, pin);

		if (func == BCM2835_GPIO_IN ||
		    func == BCM2835_GPIO_OUT) {
			/* XXX TRISTATE?  Really? */
			sc->sc_gpio_pins[pin].pin_caps = GPIO_PIN_INPUT |
				GPIO_PIN_OUTPUT |
				GPIO_PIN_PUSHPULL | GPIO_PIN_TRISTATE |
				GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN |
				GPIO_PIN_ALT0 | GPIO_PIN_ALT1 |
				GPIO_PIN_ALT2 | GPIO_PIN_ALT3 |
				GPIO_PIN_ALT4 | GPIO_PIN_ALT5;
			sc->sc_gpio_pins[pin].pin_intrcaps =
				GPIO_INTR_POS_EDGE |
				GPIO_INTR_NEG_EDGE |
				GPIO_INTR_DOUBLE_EDGE |
				GPIO_INTR_HIGH_LEVEL |
				GPIO_INTR_LOW_LEVEL |
				GPIO_INTR_MPSAFE;
			/* read initial state */
			sc->sc_gpio_pins[pin].pin_state =
				bcm2835gpio_gpio_pin_read(sc, pin);
			aprint_debug_dev(sc->sc_dev, "attach pin %d\n", pin);
		} else {
			sc->sc_gpio_pins[pin].pin_caps = 0;
			sc->sc_gpio_pins[pin].pin_state = 0;
			aprint_debug_dev(sc->sc_dev, "skip pin %d - func = %x\n", pin, func);
		}
	}

	/* Initialize interrupts. */
	for (bank = 0; bank < BCMGPIO_NBANKS; bank++) {
		char intrstr[128];

		if (!fdtbus_intr_str(phandle, bank, intrstr, sizeof(intrstr))) {
			aprint_error_dev(self, "failed to decode interrupt\n");
			continue;
		}

		char xname[16];
		snprintf(xname, sizeof(xname), "%s #%u", device_xname(self),
		    bank);
		sc->sc_banks[bank].sc_bankno = bank;
		sc->sc_banks[bank].sc_bcm = sc;
		sc->sc_banks[bank].sc_ih = fdtbus_intr_establish_xname(phandle,
		    bank, IPL_VM, FDT_INTR_MPSAFE, bcmgpio_intr,
		    &sc->sc_banks[bank], xname);
		if (sc->sc_banks[bank].sc_ih) {
			aprint_normal_dev(self,
			    "pins %d..%d interrupting on %s\n",
			    bank * 32,
			    MIN((bank * 32) + 31, sc->sc_maxpins),
			    intrstr);
		} else {
			aprint_error_dev(self,
			    "failed to establish interrupt for pins %d..%d\n",
			    bank * 32,
			    MIN((bank * 32) + 31, sc->sc_maxpins));
		}
	}

	fdtbus_register_gpio_controller(self, faa->faa_phandle, &bcmgpio_funcs);

	for (int child = OF_child(phandle); child; child = OF_peer(child)) {
		if (!of_hasprop(child, "brcm,pins"))
			continue;
		fdtbus_register_pinctrl_config(self, child,
		    &bcm283x_pinctrl_funcs);
	}

	fdtbus_register_interrupt_controller(self, phandle,
	    &bcmgpio_fdt_intrfuncs);

	/* create controller tag */
	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = bcm2835gpio_gpio_pin_read;
	sc->sc_gpio_gc.gp_pin_write = bcm2835gpio_gpio_pin_write;
	sc->sc_gpio_gc.gp_pin_ctl = bcm2835gpio_gpio_pin_ctl;
	sc->sc_gpio_gc.gp_intr_establish = bcmgpio_gpio_intr_establish;
	sc->sc_gpio_gc.gp_intr_disestablish = bcmgpio_gpio_intr_disestablish;
	sc->sc_gpio_gc.gp_intr_str = bcmgpio_gpio_intrstr;

	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = &sc->sc_gpio_pins[0];
	gba.gba_npins = sc->sc_maxpins;
	config_found(self, &gba, gpiobus_print,
	    CFARGS(.devhandle = device_handle(self)));
}

/* GPIO interrupt support functions */

static int
bcmgpio_intr(void *arg)
{
	struct bcmgpio_bank * const b = arg;
	struct bcmgpio_softc * const sc = b->sc_bcm;
	struct bcmgpio_eint *eint;
	uint32_t status, pending, bit;
	uint32_t clear_level;
	int (*func)(void *);
	int rv = 0;

	for (;;) {
		status = pending = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    BCM2835_GPIO_GPEDS(b->sc_bankno));
		if (status == 0)
			break;

		/*
		 * This will clear the indicator for any pending
		 * edge-triggered pins, but level-triggered pins
		 * will still be indicated until the pin is
		 * de-asserted.  We'll have to clear level-triggered
		 * indicators below.
		 */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    BCM2835_GPIO_GPEDS(b->sc_bankno), status);
		clear_level = 0;

		while ((bit = ffs32(pending)) != 0) {
			pending &= ~__BIT(bit - 1);
			eint = &b->sc_eint[bit - 1];
			if ((func = eint->eint_func) == NULL)
				continue;
			if (eint->eint_flags & (BCMGPIO_INTR_HIGH_LEVEL |
						BCMGPIO_INTR_LOW_LEVEL))
				clear_level |= __BIT(bit - 1);
			const bool mpsafe =
			    (eint->eint_flags & BCMGPIO_INTR_MPSAFE) != 0;
			if (!mpsafe)
				KERNEL_LOCK(1, curlwp);
			rv |= (*func)(eint->eint_arg);
			if (!mpsafe)
				KERNEL_UNLOCK_ONE(curlwp);
		}

		/*
		 * Now that all of the handlers have been called,
		 * we can clear the indicators for any level-triggered
		 * pins.
		 */
		if (clear_level)
			bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			    BCM2835_GPIO_GPEDS(b->sc_bankno), clear_level);
	}

	return (rv);
}

static void *
bmcgpio_intr_enable(struct bcmgpio_softc *sc, int (*func)(void *), void *arg,
		    int bank, int pin, int flags)
{
	struct bcmgpio_eint *eint;
	uint32_t mask, enabled_ren, enabled_fen, enabled_hen, enabled_len;
	int has_edge = flags & (BCMGPIO_INTR_POS_EDGE|BCMGPIO_INTR_NEG_EDGE);
	int has_level = flags &
	    (BCMGPIO_INTR_HIGH_LEVEL|BCMGPIO_INTR_LOW_LEVEL);

	if (bank < 0 || bank >= BCMGPIO_NBANKS)
		return NULL;
	if (pin < 0 || pin >= 32)
		return (NULL);

	/* Must specify a mode. */
	if (!has_edge && !has_level)
		return (NULL);

	/* Can't have HIGH and LOW together. */
	if (has_level == (BCMGPIO_INTR_HIGH_LEVEL|BCMGPIO_INTR_LOW_LEVEL))
		return (NULL);

	/* Can't have EDGE and LEVEL together. */
	if (has_edge && has_level)
		return (NULL);

	eint = &sc->sc_banks[bank].sc_eint[pin];

	mask = __BIT(pin);

	mutex_enter(&sc->sc_lock);

	if (eint->eint_func != NULL) {
		mutex_exit(&sc->sc_lock);
		return (NULL);	/* in use */
	}

	eint->eint_func = func;
	eint->eint_arg = arg;
	eint->eint_flags = flags;
	eint->eint_bank = bank;
	eint->eint_num = pin;

	enabled_ren = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
				       BCM2835_GPIO_GPREN(bank));
	enabled_fen = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
				       BCM2835_GPIO_GPFEN(bank));
	enabled_hen = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
				       BCM2835_GPIO_GPHEN(bank));
	enabled_len = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
				       BCM2835_GPIO_GPLEN(bank));

	enabled_ren &= ~mask;
	enabled_fen &= ~mask;
	enabled_hen &= ~mask;
	enabled_len &= ~mask;

	if (flags & BCMGPIO_INTR_POS_EDGE)
		enabled_ren |= mask;
	if (flags & BCMGPIO_INTR_NEG_EDGE)
		enabled_fen |= mask;
	if (flags & BCMGPIO_INTR_HIGH_LEVEL)
		enabled_hen |= mask;
	if (flags & BCMGPIO_INTR_LOW_LEVEL)
		enabled_len |= mask;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  BCM2835_GPIO_GPREN(bank), enabled_ren);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  BCM2835_GPIO_GPFEN(bank), enabled_fen);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  BCM2835_GPIO_GPHEN(bank), enabled_hen);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  BCM2835_GPIO_GPLEN(bank), enabled_len);

	mutex_exit(&sc->sc_lock);
	return (eint);
}

static void
bcmgpio_intr_disable(struct bcmgpio_softc *sc, struct bcmgpio_eint *eint)
{
	uint32_t mask, enabled_ren, enabled_fen, enabled_hen, enabled_len;
	int bank = eint->eint_bank;

	mask = __BIT(eint->eint_num);

	KASSERT(eint->eint_func != NULL);

	mutex_enter(&sc->sc_lock);

	enabled_ren = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
				       BCM2835_GPIO_GPREN(bank));
	enabled_fen = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
				       BCM2835_GPIO_GPFEN(bank));
	enabled_hen = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
				       BCM2835_GPIO_GPHEN(bank));
	enabled_len = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
				       BCM2835_GPIO_GPLEN(bank));

	enabled_ren &= ~mask;
	enabled_fen &= ~mask;
	enabled_hen &= ~mask;
	enabled_len &= ~mask;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  BCM2835_GPIO_GPREN(bank), enabled_ren);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  BCM2835_GPIO_GPFEN(bank), enabled_fen);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  BCM2835_GPIO_GPHEN(bank), enabled_hen);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  BCM2835_GPIO_GPLEN(bank), enabled_len);

	eint->eint_func = NULL;
	eint->eint_arg = NULL;
	eint->eint_flags = 0;

	mutex_exit(&sc->sc_lock);
}

static void *
bcmgpio_fdt_intr_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct bcmgpio_softc * const sc = device_private(dev);
	int eint_flags = (flags & FDT_INTR_MPSAFE) ? BCMGPIO_INTR_MPSAFE : 0;

	if (ipl != IPL_VM) {
		aprint_error_dev(dev, "%s: wrong IPL %d (expected %d)\n",
		    __func__, ipl, IPL_VM);
		return (NULL);
	}

	/* 1st cell is the GPIO number */
	/* 2nd cell is flags */
	const u_int bank = be32toh(specifier[0]) / 32;
	const u_int pin = be32toh(specifier[0]) % 32;
	const u_int type = be32toh(specifier[1]) & 0xf;

	switch (type) {
	case FDT_INTR_TYPE_POS_EDGE:
		eint_flags |= BCMGPIO_INTR_POS_EDGE;
		break;
	case FDT_INTR_TYPE_NEG_EDGE:
		eint_flags |= BCMGPIO_INTR_NEG_EDGE;
		break;
	case FDT_INTR_TYPE_DOUBLE_EDGE:
		eint_flags |= BCMGPIO_INTR_POS_EDGE | BCMGPIO_INTR_NEG_EDGE;
		break;
	case FDT_INTR_TYPE_HIGH_LEVEL:
		eint_flags |= BCMGPIO_INTR_HIGH_LEVEL;
		break;
	case FDT_INTR_TYPE_LOW_LEVEL:
		eint_flags |= BCMGPIO_INTR_LOW_LEVEL;
		break;
	default:
		aprint_error_dev(dev, "%s: unsupported irq type 0x%x\n",
		    __func__, type);
		return (NULL);
	}

	return (bmcgpio_intr_enable(sc, func, arg, bank, pin, eint_flags));
}

static void
bcmgpio_fdt_intr_disestablish(device_t dev, void *ih)
{
	struct bcmgpio_softc * const sc = device_private(dev);
	struct bcmgpio_eint * const eint = ih;

	bcmgpio_intr_disable(sc, eint);
}

static void *
bcmgpio_gpio_intr_establish(void *vsc, int pin, int ipl, int irqmode,
			    int (*func)(void *), void *arg)
{
	struct bcmgpio_softc * const sc = vsc;
	int eint_flags = (irqmode & GPIO_INTR_MPSAFE) ? BCMGPIO_INTR_MPSAFE : 0;
	int bank = pin / 32;
	int type = irqmode & GPIO_INTR_MODE_MASK;

	pin %= 32;

	if (ipl != IPL_VM) {
		aprint_error_dev(sc->sc_dev, "%s: wrong IPL %d (expected %d)\n",
		    __func__, ipl, IPL_VM);
		return (NULL);
	}

	switch (type) {
	case GPIO_INTR_POS_EDGE:
		eint_flags |= BCMGPIO_INTR_POS_EDGE;
		break;
	case GPIO_INTR_NEG_EDGE:
		eint_flags |= BCMGPIO_INTR_NEG_EDGE;
		break;
	case GPIO_INTR_DOUBLE_EDGE:
		eint_flags |= BCMGPIO_INTR_POS_EDGE | BCMGPIO_INTR_NEG_EDGE;
		break;
	case GPIO_INTR_HIGH_LEVEL:
		eint_flags |= BCMGPIO_INTR_HIGH_LEVEL;
		break;
	case GPIO_INTR_LOW_LEVEL:
		eint_flags |= BCMGPIO_INTR_LOW_LEVEL;
		break;
	default:
		aprint_error_dev(sc->sc_dev, "%s: unsupported irq type 0x%x\n",
		    __func__, type);
		return (NULL);
	}

	return (bmcgpio_intr_enable(sc, func, arg, bank, pin, eint_flags));
}

static void
bcmgpio_gpio_intr_disestablish(void *vsc, void *ih)
{
	struct bcmgpio_softc * const sc = vsc;
	struct bcmgpio_eint * const eint = ih;

	bcmgpio_intr_disable(sc, eint);
}

static bool
bcmgpio_gpio_intrstr(void *vsc, int pin, int irqmode, char *buf, size_t buflen)
{
	struct bcmgpio_softc * const sc = vsc;

	if (pin < 0 || pin >= sc->sc_maxpins)
		return (false);

	snprintf(buf, buflen, "GPIO %d", pin);

	return (true);
}

static bool
bcmgpio_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{

	/* 1st cell is the GPIO number */
	/* 2nd cell is flags */
	if (!specifier)
		return (false);
	const u_int bank = be32toh(specifier[0]) / 32;
	const u_int pin = be32toh(specifier[0]) % 32;
	const u_int type = be32toh(specifier[1]) & 0xf;
	char const* typestr;

	if (bank >= BCMGPIO_NBANKS)
		return (false);
	switch (type) {
	case FDT_INTR_TYPE_DOUBLE_EDGE:
		typestr = "double edge";
		break;
	case FDT_INTR_TYPE_POS_EDGE:
		typestr = "positive edge";
		break;
	case FDT_INTR_TYPE_NEG_EDGE:
		typestr = "negative edge";
		break;
	case FDT_INTR_TYPE_HIGH_LEVEL:
		typestr = "high level";
		break;
	case FDT_INTR_TYPE_LOW_LEVEL:
		typestr = "low level";
		break;
	default:
		aprint_error_dev(dev, "%s: unsupported irq type 0x%x\n",
		    __func__, type);

		return (false);
	}

	snprintf(buf, buflen, "GPIO %u (%s)", (bank * 32) + pin, typestr);

	return (true);
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

	u_int mask, regid;
	uint32_t reg;

	if (sc->sc_is2835) {
		mask = 1 << (pin % BCM2835_GPIO_GPPUD_PINS_PER_REGISTER);
		regid = (pin / BCM2835_GPIO_GPPUD_PINS_PER_REGISTER);

		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    BCM2835_GPIO_GPPUD, pud);
		delay(1);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    BCM2835_GPIO_GPPUDCLK(regid), mask);
		delay(1);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    BCM2835_GPIO_GPPUD, 0);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    BCM2835_GPIO_GPPUDCLK(regid), 0);
	} else {
		mask = BCM2838_GPIO_GPPUD_MASK(pin);
		regid = BCM2838_GPIO_GPPUD_REGID(pin);

		switch (pud) {
		case BCM2835_GPIO_GPPUD_PULLUP:
			pud = BCM2838_GPIO_GPPUD_PULLUP;
			break;
		case BCM2835_GPIO_GPPUD_PULLDOWN:
			pud = BCM2838_GPIO_GPPUD_PULLDOWN;
			break;
		default:
			pud = BCM2838_GPIO_GPPUD_PULLOFF;
			break;
		}

		reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    BCM2838_GPIO_GPPUPPDN(regid));
		reg &= ~mask;
		reg |= __SHIFTIN(pud, mask);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    BCM2838_GPIO_GPPUPPDN(regid), reg);
	}
}


static void
bcm2835gpio_gpio_pin_ctl(void *arg, int pin, int flags)
{
	struct bcmgpio_softc *sc = arg;
	uint32_t cmd;
	uint32_t altmask = GPIO_PIN_ALT0 | GPIO_PIN_ALT1 |
	                   GPIO_PIN_ALT2 | GPIO_PIN_ALT3 |
	                   GPIO_PIN_ALT4 | GPIO_PIN_ALT5;

	DPRINTF(2, ("%s: gpio_ctl pin %d flags 0x%x\n", device_xname(sc->sc_dev), pin, flags));

	mutex_enter(&sc->sc_lock);
	if (flags & (GPIO_PIN_OUTPUT|GPIO_PIN_INPUT)) {
		if ((flags & GPIO_PIN_INPUT) != 0) {
			/* for safety INPUT will override output */
			bcm283x_pin_setfunc(sc, pin, BCM2835_GPIO_IN);
		} else {
			bcm283x_pin_setfunc(sc, pin, BCM2835_GPIO_OUT);
		}
	} else if ((flags & altmask) != 0) {
		u_int func;

		switch (flags & altmask) {
		case GPIO_PIN_ALT0:
			func = BCM2835_GPIO_ALT0;
			break;
		case GPIO_PIN_ALT1:
			func = BCM2835_GPIO_ALT1;
			break;
		case GPIO_PIN_ALT2:
			func = BCM2835_GPIO_ALT2;
			break;
		case GPIO_PIN_ALT3:
			func = BCM2835_GPIO_ALT3;
			break;
		case GPIO_PIN_ALT4:
			func = BCM2835_GPIO_ALT4;
			break;
		case GPIO_PIN_ALT5:
			func = BCM2835_GPIO_ALT5;
			break;
		default:
			/* ignored below */
			func = BCM2835_GPIO_IN;
			break;
		}
		if (func != BCM2835_GPIO_IN)
			bcm283x_pin_setfunc(sc, pin, func);
	}

	if (flags & (GPIO_PIN_PULLUP|GPIO_PIN_PULLDOWN)) {
		cmd = (flags & GPIO_PIN_PULLUP) ?
			BCM2835_GPIO_GPPUD_PULLUP : BCM2835_GPIO_GPPUD_PULLDOWN;
	} else {
		cmd = BCM2835_GPIO_GPPUD_PULLOFF;
	}

	bcm283x_pin_setpull(sc, pin, cmd);
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

	if (pin >= sc->sc_maxpins)
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
