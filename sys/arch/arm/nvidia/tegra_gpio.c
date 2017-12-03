/* $NetBSD: tegra_gpio.c,v 1.8.8.2 2017/12/03 11:35:54 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: tegra_gpio.c,v 1.8.8.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_gpioreg.h>
#include <arm/nvidia/tegra_var.h>

#include <dev/fdt/fdtvar.h>

const struct tegra_gpio_pinbank {
	const char *name;
	bus_size_t base;
} tegra_gpio_pinbanks [] = {
	{ "A", 0x000 },
	{ "B", 0x004 },
	{ "C", 0x008 },
	{ "D", 0x00c },
	{ "E", 0x100 },
	{ "F", 0x104 },
	{ "G", 0x108 },
	{ "H", 0x10c },
	{ "I", 0x200 },
	{ "J", 0x204 },
	{ "K", 0x208 },
	{ "L", 0x20c },
	{ "M", 0x300 },
	{ "N", 0x304 },
	{ "O", 0x308 },
	{ "P", 0x30c },
	{ "Q", 0x400 },
	{ "R", 0x404 },
	{ "S", 0x408 },
	{ "T", 0x40c },
	{ "U", 0x500 },
	{ "V", 0x504 },
	{ "W", 0x508 },
	{ "X", 0x50c },
	{ "Y", 0x600 },
	{ "Z", 0x604 },
	{ "AA", 0x608 },
	{ "BB", 0x60c },
	{ "CC", 0x700 },
	{ "DD", 0x704 },
	{ "EE", 0x708 }
};

static int	tegra_gpio_match(device_t, cfdata_t, void *);
static void	tegra_gpio_attach(device_t, device_t, void *);

static void *	tegra_gpio_fdt_acquire(device_t, const void *,
		    size_t, int);
static void	tegra_gpio_fdt_release(device_t, void *);
static int	tegra_gpio_fdt_read(device_t, void *, bool);
static void	tegra_gpio_fdt_write(device_t, void *, int, bool);

struct fdtbus_gpio_controller_func tegra_gpio_funcs = {
	.acquire = tegra_gpio_fdt_acquire,
	.release = tegra_gpio_fdt_release,
	.read = tegra_gpio_fdt_read,
	.write = tegra_gpio_fdt_write
};

struct tegra_gpio_softc;

struct tegra_gpio_bank {
	struct tegra_gpio_softc *bank_sc;
	const struct tegra_gpio_pinbank *bank_pb;
	device_t		bank_dev;
	struct gpio_chipset_tag	bank_gc;
	gpio_pin_t		bank_pins[8];
};

struct tegra_gpio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct tegra_gpio_bank *sc_banks;
};

struct tegra_gpio_pin {
	struct tegra_gpio_softc *pin_sc;
	struct tegra_gpio_bank	pin_bank;
	int			pin_no;
	u_int			pin_flags;
	bool			pin_actlo;
};

static void	tegra_gpio_attach_bank(struct tegra_gpio_softc *, u_int);

static int	tegra_gpio_pin_read(void *, int);
static void	tegra_gpio_pin_write(void *, int, int);
static void	tegra_gpio_pin_ctl(void *, int, int);

static int	tegra_gpio_cfprint(void *, const char *);

CFATTACH_DECL_NEW(tegra_gpio, sizeof(struct tegra_gpio_softc),
	tegra_gpio_match, tegra_gpio_attach, NULL, NULL);

#define GPIO_WRITE(bank, reg, val) \
	bus_space_write_4((bank)->bank_sc->sc_bst, \
	    (bank)->bank_sc->sc_bsh, \
	    (bank)->bank_pb->base + (reg), (val))
#define GPIO_READ(bank, reg) \
	bus_space_read_4((bank)->bank_sc->sc_bst, \
	    (bank)->bank_sc->sc_bsh, \
	    (bank)->bank_pb->base + (reg))

static int
tegra_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"nvidia,tegra210-gpio",
		"nvidia,tegra124-gpio",
		"nvidia,tegra30-gpio",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_gpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;
	u_int n;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": GPIO\n");

	const u_int nbank = __arraycount(tegra_gpio_pinbanks);
	sc->sc_banks = kmem_zalloc(sizeof(*sc->sc_banks) * nbank, KM_SLEEP);
	for (n = 0; n < nbank; n++) {
		tegra_gpio_attach_bank(sc, n);
	}

	fdtbus_register_gpio_controller(self, faa->faa_phandle,
	    &tegra_gpio_funcs);
}

static void
tegra_gpio_attach_bank(struct tegra_gpio_softc *sc, u_int bankno)
{
	struct tegra_gpio_bank *bank = &sc->sc_banks[bankno];
	struct gpiobus_attach_args gba;
	u_int pin;

	bank->bank_sc = sc;
	bank->bank_pb = &tegra_gpio_pinbanks[bankno];
	bank->bank_gc.gp_cookie = bank;
	bank->bank_gc.gp_pin_read = tegra_gpio_pin_read;
	bank->bank_gc.gp_pin_write = tegra_gpio_pin_write;
	bank->bank_gc.gp_pin_ctl = tegra_gpio_pin_ctl;

	const uint32_t cnf = GPIO_READ(bank, GPIO_CNF_REG);

	for (pin = 0; pin < __arraycount(bank->bank_pins); pin++) {
		bank->bank_pins[pin].pin_num = pin;
		/* skip pins in SFIO mode */
		if ((cnf & __BIT(pin)) == 0)
			continue;
		bank->bank_pins[pin].pin_caps =
		    GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
		    GPIO_PIN_TRISTATE;
		bank->bank_pins[pin].pin_state =
		    tegra_gpio_pin_read(bank, pin);
	}

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = &bank->bank_gc;
	gba.gba_pins = bank->bank_pins;
	gba.gba_npins = __arraycount(bank->bank_pins);

	bank->bank_dev = config_found_ia(sc->sc_dev, "gpiobus", &gba,
	    tegra_gpio_cfprint);
}

static int
tegra_gpio_cfprint(void *priv, const char *pnp)
{
	struct gpiobus_attach_args *gba = priv;
	struct tegra_gpio_bank *bank = gba->gba_gc->gp_cookie;
	const char *bankname = bank->bank_pb->name;

	if (pnp)
		aprint_normal("gpiobus at %s", pnp);

	aprint_normal(" (%s)", bankname);

	return UNCONF;
}

static int
tegra_gpio_pin_read(void *priv, int pin)
{
	struct tegra_gpio_bank *bank = priv;

	const uint32_t v = GPIO_READ(bank, GPIO_IN_REG);

	return (v >> pin) & 1;
}

static void
tegra_gpio_pin_write(void *priv, int pin, int val)
{
	struct tegra_gpio_bank *bank = priv;
	uint32_t v;

	v = (1 << (pin + 8));
	v |= (val << pin);
	GPIO_WRITE(bank, GPIO_MSK_OUT_REG, v);
}

static void
tegra_gpio_pin_ctl(void *priv, int pin, int flags)
{
	struct tegra_gpio_bank *bank = priv;
	uint32_t v;

	if (flags & GPIO_PIN_INPUT) {
		v = (1 << (pin + 8));
		GPIO_WRITE(bank, GPIO_MSK_OE_REG, v);
	} else if (flags & GPIO_PIN_OUTPUT) {
		v = (1 << (pin + 8));
		v |= (1 << pin);
		GPIO_WRITE(bank, GPIO_MSK_OE_REG, v);
	}
}

static void *
tegra_gpio_fdt_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct tegra_gpio_bank gbank;
	struct tegra_gpio_pin *gpin;
	const u_int *gpio = data;

	if (len != 12)
		return NULL;

	const u_int bank = be32toh(gpio[1]) >> 3;
	const u_int pin = be32toh(gpio[1]) & 7;
	const bool actlo = be32toh(gpio[2]) & 1;

	if (bank >= __arraycount(tegra_gpio_pinbanks) || pin > 8)
		return NULL;

	gbank.bank_sc = device_private(dev);
	gbank.bank_pb = &tegra_gpio_pinbanks[bank];

	const uint32_t cnf = GPIO_READ(&gbank, GPIO_CNF_REG);
	if ((cnf & __BIT(pin)) == 0)
		GPIO_WRITE(&gbank, GPIO_CNF_REG, cnf | __BIT(pin));

	gpin = kmem_zalloc(sizeof(*gpin), KM_SLEEP);
	gpin->pin_bank = gbank;
	gpin->pin_no = pin;
	gpin->pin_flags = flags;
	gpin->pin_actlo = actlo;

	tegra_gpio_pin_ctl(&gpin->pin_bank, gpin->pin_no, gpin->pin_flags);

	return gpin;
}

static void
tegra_gpio_fdt_release(device_t dev, void *priv)
{
	struct tegra_gpio_pin *gpin = priv;

	tegra_gpio_release(gpin);
}

static int
tegra_gpio_fdt_read(device_t dev, void *priv, bool raw)
{
	struct tegra_gpio_pin *gpin = priv;
	int val;

	val = tegra_gpio_read(gpin);

	if (!raw && gpin->pin_actlo)
		val = !val;

	return val;
}

static void
tegra_gpio_fdt_write(device_t dev, void *priv, int val, bool raw)
{
	struct tegra_gpio_pin *gpin = priv;

	if (!raw && gpin->pin_actlo)
		val = !val;

	tegra_gpio_write(gpin, val);
}

static const struct tegra_gpio_pinbank *
tegra_gpio_pin_lookup(const char *pinname, int *ppin)
{
	char bankname[3];
	u_int n;
	int pin;

	KASSERT(strlen(pinname) == 2 || strlen(pinname) == 3);

	memset(bankname, 0, sizeof(bankname));
	bankname[0] = pinname[0];
	if (strlen(pinname) == 2) {
		pin = pinname[1] - '0';
	} else {
		bankname[1] = pinname[1];
		pin = pinname[2] - '0';
	}

	for (n = 0; n < __arraycount(tegra_gpio_pinbanks); n++) {
		const struct tegra_gpio_pinbank *pb =
		    &tegra_gpio_pinbanks[n];
		if (strcmp(pb->name, bankname) == 0) {
			*ppin = pin;
			return pb;
		}
	}

	return NULL;
}

struct tegra_gpio_pin *
tegra_gpio_acquire(const char *pinname, u_int flags)
{
	struct tegra_gpio_bank bank;
	struct tegra_gpio_pin *gpin;
	int pin;
	device_t dev;

	dev = device_find_by_driver_unit("tegragpio", 0);
	if (dev == NULL)
		return NULL;

	bank.bank_sc = device_private(dev);
	bank.bank_pb = tegra_gpio_pin_lookup(pinname, &pin);
	if (bank.bank_pb == NULL)
		return NULL;

	const uint32_t cnf = GPIO_READ(&bank, GPIO_CNF_REG);
	if ((cnf & __BIT(pin)) == 0)
		GPIO_WRITE(&bank, GPIO_CNF_REG, cnf | __BIT(pin));

	gpin = kmem_alloc(sizeof(*gpin), KM_SLEEP);
	gpin->pin_bank = bank;
	gpin->pin_no = pin;
	gpin->pin_flags = flags;

	tegra_gpio_pin_ctl(&gpin->pin_bank, gpin->pin_no, gpin->pin_flags);

	return gpin;
}

void
tegra_gpio_release(struct tegra_gpio_pin *gpin)
{
	tegra_gpio_pin_ctl(&gpin->pin_bank, gpin->pin_no, GPIO_PIN_INPUT);
	kmem_free(gpin, sizeof(*gpin));
}

int
tegra_gpio_read(struct tegra_gpio_pin *gpin)
{
	int ret;

	if (gpin->pin_flags & GPIO_PIN_INPUT) {
		ret = tegra_gpio_pin_read(&gpin->pin_bank, gpin->pin_no);
	} else {
		const uint32_t v = GPIO_READ(&gpin->pin_bank, GPIO_OUT_REG);
		ret = (v >> gpin->pin_no) & 1;
	}

	return ret;
}

void
tegra_gpio_write(struct tegra_gpio_pin *gpin, int val)
{
	KASSERT((gpin->pin_flags & GPIO_PIN_OUTPUT) != 0);

	tegra_gpio_pin_write(&gpin->pin_bank, gpin->pin_no, val);
}
