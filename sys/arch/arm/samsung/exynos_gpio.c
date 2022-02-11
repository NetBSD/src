/*	$NetBSD: exynos_gpio.c,v 1.33 2022/02/11 23:48:50 riastradh Exp $ */

/*-
* Copyright (c) 2014, 2020 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Reinoud Zandijk, and by Nick Hudson
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

#include "opt_exynos.h"
#include "opt_arm_debug.h"
#include "gpio.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: exynos_gpio.c,v 1.33 2022/02/11 23:48:50 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_var.h>
#include <arm/samsung/exynos_intr.h>
#include <arm/samsung/exynos_pinctrl.h>

#include <dev/fdt/fdtvar.h>

struct exynos_gpio_bank {
	const char		bank_name[6];
	device_t		bank_dev;
	struct gpio_chipset_tag	bank_gc;
	struct exynos_gpio_softc *bank_sc;
	gpio_pin_t		bank_pins[8];

	const bus_addr_t	bank_core_offset;
	const uint8_t		bank_bits;

	struct exynos_gpio_pin_cfg bank_cfg;
};

struct exynos_gpio_pin {
	struct exynos_gpio_softc *pin_sc;
	int			  pin_no;
	u_int			  pin_flags;
	int			  pin_actlo;
	const struct exynos_gpio_bank   *pin_bank;
};


//#define GPIO_REG(v,s,o) (EXYNOS##v##_GPIO_##s##_OFFSET + (o))
#define GPIO_REG(v,s,o) ((o))
#define GPIO_GRP(v, s, o, n, b)	\
	{ \
		.bank_name = #n, \
		.bank_core_offset = GPIO_REG(v,s,o), \
		.bank_bits = b, \
	}

#define GPIO_GRP_INTR(o, n, b, i)			\
	{ 						\
		.bank_name = #n,			\
		.bank_core_offset = GPIO_REG(v,s,o),	\
		.bank_bits = b,				\
	}

#define GPIO_GRP_NONE(o, n, b)	\
	{ \
		.bank_name = #n, \
		.bank_core_offset = GPIO_REG(v,s,o), \
		.bank_bits = b, \
	}

#define GPIO_GRP_WAKEUP(o, n, b, i)			\
	{ 						\
		.bank_name = #n,			\
		.bank_core_offset = GPIO_REG(v,s,o),	\
		.bank_bits = b,				\
	}



static struct exynos_gpio_bank exynos5420_banks[] = {
	GPIO_GRP(5, MUXA, 0x0000, gpy7, 8),
	GPIO_GRP(5, MUXA, 0x0C00, gpx0, 8),
	GPIO_GRP(5, MUXA, 0x0C20, gpx1, 8),
	GPIO_GRP(5, MUXA, 0x0C40, gpx2, 8),
	GPIO_GRP(5, MUXA, 0x0C60, gpx3, 8),

	GPIO_GRP(5, MUXB, 0x0000, gpc0, 8),
	GPIO_GRP(5, MUXB, 0x0020, gpc1, 8),
	GPIO_GRP(5, MUXB, 0x0040, gpc2, 7),
	GPIO_GRP(5, MUXB, 0x0060, gpc3, 4),
	GPIO_GRP(5, MUXB, 0x0080, gpc4, 2),
	GPIO_GRP(5, MUXB, 0x00A0, gpd1, 8),
	GPIO_GRP(5, MUXB, 0x00C0, gpy0, 6),
	GPIO_GRP(5, MUXB, 0x00E0, gpy1, 4),
	GPIO_GRP(5, MUXB, 0x0100, gpy2, 6),
	GPIO_GRP(5, MUXB, 0x0120, gpy3, 8),
	GPIO_GRP(5, MUXB, 0x0140, gpy4, 8),
	GPIO_GRP(5, MUXB, 0x0160, gpy5, 8),
	GPIO_GRP(5, MUXB, 0x0180, gpy6, 8),

	GPIO_GRP(5, MUXC, 0x0000, gpe0, 8),
	GPIO_GRP(5, MUXC, 0x0020, gpe1, 2),
	GPIO_GRP(5, MUXC, 0x0040, gpf0, 6),
	GPIO_GRP(5, MUXC, 0x0060, gpf1, 8),
	GPIO_GRP(5, MUXC, 0x0080, gpg0, 8),
	GPIO_GRP(5, MUXC, 0x00A0, gpg1, 8),
	GPIO_GRP(5, MUXC, 0x00C0, gpg2, 2),
	GPIO_GRP(5, MUXC, 0x00E0, gpj4, 4),

	GPIO_GRP(5, MUXD, 0x0000, gpa0, 8),
	GPIO_GRP(5, MUXD, 0x0020, gpa1, 6),
	GPIO_GRP(5, MUXD, 0x0040, gpa2, 8),
	GPIO_GRP(5, MUXD, 0x0060, gpb0, 5),
	GPIO_GRP(5, MUXD, 0x0080, gpb1, 5),
	GPIO_GRP(5, MUXD, 0x00A0, gpb2, 4),
	GPIO_GRP(5, MUXD, 0x00C0, gpb3, 8),
	GPIO_GRP(5, MUXD, 0x00E0, gpb4, 2),
	GPIO_GRP(5, MUXD, 0x0100, gph0, 4),

	GPIO_GRP(5, MUXE, 0x0000, gpz, 7),
};

struct exynos_pinctrl_banks exynos5420_pinctrl_banks = {
	.epb_banks = exynos5420_banks,
	.epb_nbanks = __arraycount(exynos5420_banks)
};

static struct exynos_gpio_bank exynos5410_banks[] = {
	/* pin-controller 0 */
	GPIO_GRP_INTR(0x000, gpa0, 8, 0x00),
	GPIO_GRP_INTR(0x020, gpa1, 6, 0x04),
	GPIO_GRP_INTR(0x040, gpa2, 8, 0x08),
	GPIO_GRP_INTR(0x060, gpb0, 5, 0x0c),
	GPIO_GRP_INTR(0x080, gpb1, 5, 0x10),
	GPIO_GRP_INTR(0x0A0, gpb2, 4, 0x14),
	GPIO_GRP_INTR(0x0C0, gpb3, 4, 0x18),
	GPIO_GRP_INTR(0x0E0, gpc0, 7, 0x1c),
	GPIO_GRP_INTR(0x100, gpc3, 4, 0x20),
	GPIO_GRP_INTR(0x120, gpc1, 7, 0x24),
	GPIO_GRP_INTR(0x140, gpc2, 7, 0x28),
	GPIO_GRP_INTR(0x180, gpd1, 8, 0x2c),
	GPIO_GRP_INTR(0x1A0, gpe0, 8, 0x30),
	GPIO_GRP_INTR(0x1C0, gpe1, 2, 0x34),
	GPIO_GRP_INTR(0x1E0, gpf0, 6, 0x38),
	GPIO_GRP_INTR(0x200, gpf1, 8, 0x3c),
	GPIO_GRP_INTR(0x220, gpg0, 8, 0x40),
	GPIO_GRP_INTR(0x240, gpg1, 8, 0x44),
	GPIO_GRP_INTR(0x260, gpg2, 2, 0x48),
	GPIO_GRP_INTR(0x280, gph0, 4, 0x4c),
	GPIO_GRP_INTR(0x2A0, gph1, 8, 0x50),
	GPIO_GRP_NONE(0x160, gpm5, 2),
	GPIO_GRP_NONE(0x2C0, gpm7, 8),
	GPIO_GRP_NONE(0x2E0, gpy0, 6),
	GPIO_GRP_NONE(0x300, gpy1, 4),
	GPIO_GRP_NONE(0x320, gpy2, 6),
	GPIO_GRP_NONE(0x340, gpy3, 8),
	GPIO_GRP_NONE(0x360, gpy4, 8),
	GPIO_GRP_NONE(0x380, gpy5, 8),
	GPIO_GRP_NONE(0x3A0, gpy6, 8),
	GPIO_GRP_NONE(0x3C0, gpy7, 8),
	GPIO_GRP_WAKEUP(0xC00, gpx0, 8, 0x00),
	GPIO_GRP_WAKEUP(0xC20, gpx1, 8, 0x04),
	GPIO_GRP_WAKEUP(0xC40, gpx2, 8, 0x08),
	GPIO_GRP_WAKEUP(0xC60, gpx3, 8, 0x0c),

	/* pin-controller 1 */
	GPIO_GRP_INTR(0x000, gpj0, 5, 0x00),
	GPIO_GRP_INTR(0x020, gpj1, 8, 0x04),
	GPIO_GRP_INTR(0x040, gpj2, 8, 0x08),
	GPIO_GRP_INTR(0x060, gpj3, 8, 0x0c),
	GPIO_GRP_INTR(0x080, gpj4, 2, 0x10),
	GPIO_GRP_INTR(0x0A0, gpk0, 8, 0x14),
	GPIO_GRP_INTR(0x0C0, gpk1, 8, 0x18),
	GPIO_GRP_INTR(0x0E0, gpk2, 8, 0x1c),
	GPIO_GRP_INTR(0x100, gpk3, 7, 0x20),

	/* pin-controller 2 */
	GPIO_GRP_INTR(0x000, gpv0, 8, 0x00),
	GPIO_GRP_INTR(0x020, gpv1, 8, 0x04),
	GPIO_GRP_INTR(0x060, gpv2, 8, 0x08),
	GPIO_GRP_INTR(0x080, gpv3, 8, 0x0c),
	GPIO_GRP_INTR(0x0C0, gpv4, 2, 0x10),

	/* pin-controller 2 */
	GPIO_GRP_INTR(0x000, gpz, 7, 0x00),
};

struct exynos_pinctrl_banks exynos5410_pinctrl_banks = {
	.epb_banks = exynos5410_banks,
	.epb_nbanks = __arraycount(exynos5410_banks)
};


static int exynos_gpio_pin_read(void *, int);
static void exynos_gpio_pin_write(void *, int, int);
static void exynos_gpio_pin_ctl(void *, int, int);
static void *exynos_gpio_fdt_acquire(device_t, const void *,
				     size_t, int);
static void exynos_gpio_fdt_release(device_t, void *);

static int exynos_gpio_fdt_read(device_t, void *, bool);
static void exynos_gpio_fdt_write(device_t, void *, int, bool);
static int exynos_gpio_cfprint(void *, const char *);

struct fdtbus_gpio_controller_func exynos_gpio_funcs = {
	.acquire = exynos_gpio_fdt_acquire,
	.release = exynos_gpio_fdt_release,
	.read = exynos_gpio_fdt_read,
	.write = exynos_gpio_fdt_write
};
#define GPIO_WRITE(bank, reg, val) \
	bus_space_write_4((bank)->bank_sc->sc_bst, \
	    (bank)->bank_sc->sc_bsh, \
	    (bank)->bank_core_offset + (reg), (val))
#define GPIO_READ(bank, reg) \
	bus_space_read_4((bank)->bank_sc->sc_bst, \
	    (bank)->bank_sc->sc_bsh, \
	    (bank)->bank_core_offset + (reg))

static int
exynos_gpio_cfprint(void *priv, const char *pnp)
{
	struct gpiobus_attach_args *gba = priv;
	struct exynos_gpio_bank *bank = gba->gba_gc->gp_cookie;
	const char *bankname = bank->bank_name;

	if (pnp)
		aprint_normal("gpiobus at %s", pnp);

	aprint_normal(" (%s)", bankname);

	return UNCONF;
}

static int
exynos_gpio_pin_read(void *cookie, int pin)
{
	struct exynos_gpio_bank * const bank = cookie;
	uint8_t val;

	KASSERT(pin < bank->bank_bits);
	val = bus_space_read_1(bank->bank_sc->sc_bst, bank->bank_sc->sc_bsh,
	    EXYNOS_GPIO_DAT);

	return __SHIFTOUT(val, __BIT(pin));
}

static void
exynos_gpio_pin_write(void *cookie, int pin, int value)
{
	struct exynos_gpio_bank * const bank = cookie;
	int val;

	KASSERT(pin < bank->bank_bits);
	val = bus_space_read_1(bank->bank_sc->sc_bst, bank->bank_sc->sc_bsh,
	    EXYNOS_GPIO_DAT);
	val &= ~__BIT(pin);
	if (value)
		val |= __BIT(pin);
	bus_space_write_1(bank->bank_sc->sc_bst, bank->bank_sc->sc_bsh,
	    EXYNOS_GPIO_DAT, val);
}

static void
exynos_gpio_pin_ctl(void *cookie, int pin, int flags)
{
	struct exynos_gpio_bank * const bank = cookie;
	struct exynos_gpio_pin_cfg ncfg = { 0 };

	/* honour pullup requests */
	if (flags & GPIO_PIN_PULLUP) {
		ncfg.pud = EXYNOS_GPIO_PIN_PULL_UP;
		ncfg.pud_valid = true;
	}
	if (flags & GPIO_PIN_PULLDOWN) {
		ncfg.pud = EXYNOS_GPIO_PIN_PULL_DOWN;
		ncfg.pud_valid = true;
	}

	/* honour i/o */
	if (flags & GPIO_PIN_INPUT) {
		ncfg.cfg = EXYNOS_GPIO_FUNC_INPUT;
		ncfg.cfg_valid = true;
	} else if (flags & GPIO_PIN_OUTPUT) {
		ncfg.cfg = EXYNOS_GPIO_FUNC_OUTPUT;
		ncfg.cfg_valid = true;
	}

	/* update any config registers that changed */
	exynos_gpio_pin_ctl_write(bank, &ncfg, pin);
}

void exynos_gpio_pin_ctl_write(const struct exynos_gpio_bank *bank,
			       const struct exynos_gpio_pin_cfg *cfg,
			       int pin)
{
	uint32_t val;

	if (cfg->cfg_valid) {
		val = GPIO_READ(bank, EXYNOS_GPIO_CON);
		val &= ~(0xf << (pin * 4));
		val |= (cfg->cfg << (pin * 4));
		GPIO_WRITE(bank, EXYNOS_GPIO_CON, val);
	}

	if (cfg->pud_valid) {
		val = GPIO_READ(bank, EXYNOS_GPIO_PUD);
		val &= ~(0x3 << (pin * 2));
		val |= (cfg->pud << (pin * 2));
		GPIO_WRITE(bank, EXYNOS_GPIO_PUD, val);
	}

	if (cfg->drv_valid) {
		val = GPIO_READ(bank, EXYNOS_GPIO_DRV);
		val &= ~(0x3 << (pin * 2));
		val |= (cfg->drv << (pin * 2));
		GPIO_WRITE(bank, EXYNOS_GPIO_DRV, val);
	}

	if (cfg->conpwd_valid) {
		val = GPIO_READ(bank, EXYNOS_GPIO_CONPWD);
		val &= ~(0x3 << (pin * 2));
		val |= (cfg->conpwd << (pin * 2));
		GPIO_WRITE(bank, EXYNOS_GPIO_CONPWD, val);
	}

	if (cfg->pudpwd_valid) {
		val = GPIO_READ(bank, EXYNOS_GPIO_PUDPWD);
		val &= ~(0x3 << (pin * 2));
		val |= (cfg->pudpwd << (pin * 2));
		GPIO_WRITE(bank, EXYNOS_GPIO_PUDPWD, val);
	}
}

struct exynos_gpio_softc *
exynos_gpio_bank_config(struct exynos_pinctrl_softc * parent,
			const struct fdt_attach_args *faa, int node)
{
	struct exynos_gpio_softc *sc = kmem_zalloc(sizeof(*sc), KM_SLEEP);
	struct gpiobus_attach_args gba;
	struct gpio_chipset_tag *gc_tag;
	char result[64];

	OF_getprop(node, "name", result, sizeof(result));
	struct exynos_gpio_bank *bank =
	    exynos_gpio_bank_lookup(parent->sc_epb, result);
	if (bank == NULL) {
		aprint_error_dev(parent->sc_dev, "no bank found for %s\n",
		    result);
		return NULL;
	}

	sc->sc_dev = parent->sc_dev;
	sc->sc_bst = &armv7_generic_bs_tag;
	sc->sc_bsh = parent->sc_bsh;
	sc->sc_bank = bank;

	gc_tag = &bank->bank_gc;
	gc_tag->gp_cookie = bank;
	gc_tag->gp_pin_read  = exynos_gpio_pin_read;
	gc_tag->gp_pin_write = exynos_gpio_pin_write;
	gc_tag->gp_pin_ctl   = exynos_gpio_pin_ctl;
	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = &bank->bank_gc;
	gba.gba_pins = bank->bank_pins;
	gba.gba_npins = bank->bank_bits;

	bank->bank_sc = sc;
	bank->bank_dev =
	    config_found(parent->sc_dev, &gba, exynos_gpio_cfprint, CFARGS_NONE);

	/* read in our initial settings */
	bank->bank_cfg.cfg = GPIO_READ(bank, EXYNOS_GPIO_CON);
	bank->bank_cfg.pud = GPIO_READ(bank, EXYNOS_GPIO_PUD);
	bank->bank_cfg.drv = GPIO_READ(bank, EXYNOS_GPIO_DRV);
	bank->bank_cfg.conpwd = GPIO_READ(bank, EXYNOS_GPIO_CONPWD);
	bank->bank_cfg.pudpwd = GPIO_READ(bank, EXYNOS_GPIO_PUDPWD);

	fdtbus_register_gpio_controller(bank->bank_dev, node,
					&exynos_gpio_funcs);
	return sc;
}

/*
 * This function is a bit funky.  Given a string that may look like
 * 'gpAN' or 'gpAN-P' it is meant to find a match to the part before
 * the '-', or the four character string if the dash is not present.
 */
struct exynos_gpio_bank *
exynos_gpio_bank_lookup(const struct exynos_pinctrl_banks *epb,
    const char *name)
{
	struct exynos_gpio_bank *bank;

	for (u_int n = 0; n < epb->epb_nbanks; n++) {
		bank = &epb->epb_banks[n];
		if (!strncmp(bank->bank_name, name,
			     strlen(bank->bank_name))) {
			return bank;
		}
	}

	return NULL;
}

#if notyet
static int
exynos_gpio_pin_lookup(const char *name)
{
	char *p;

	p = strchr(name, '-');
	if (p == NULL || p[1] < '0' || p[1] > '9')
		return -1;

	return p[1] - '0';
}
#endif

static void *
exynos_gpio_fdt_acquire(device_t dev, const void *data, size_t len, int flags)
{
	device_t parent = device_parent(dev);
	struct exynos_pinctrl_softc *sc = device_private(parent);
	const struct exynos_pinctrl_banks *epb = sc->sc_epb;
	struct exynos_gpio_bank *bank = NULL;
	struct exynos_gpio_pin *gpin;
	u_int n;

	KASSERT(device_is_a(parent, "exyopctl"));

	if (len != 12)
		return NULL;

	const u_int *cells = data;
	const int pin = be32toh(cells[1]) & 0x0f;
	const int actlo = be32toh(cells[2]) & 0x01;

	for (n = 0; n < epb->epb_nbanks; n++) {
		if (epb->epb_banks[n].bank_dev == dev) {
			bank = &epb->epb_banks[n];
			break;
		}
	}
	KASSERTMSG(bank != NULL, "no such gpio bank child of %s @ %p: %s @ %p",
	    device_xname(parent), parent, device_xname(dev), dev);

	gpin = kmem_alloc(sizeof(*gpin), KM_SLEEP);
	gpin->pin_sc = bank->bank_sc;
	gpin->pin_bank = bank;
	gpin->pin_no = pin;
	gpin->pin_flags = flags;
	gpin->pin_actlo = actlo;

	exynos_gpio_pin_ctl(bank, gpin->pin_no, gpin->pin_flags);

	return gpin;
}

static void
exynos_gpio_fdt_release(device_t dev, void *priv)
{
	struct exynos_gpio_pin *gpin = priv;

	kmem_free(gpin, sizeof(*gpin));
}

static int
exynos_gpio_fdt_read(device_t dev, void *priv, bool raw)
{
	struct exynos_gpio_pin *gpin = priv;
	int val;

	val = (bus_space_read_1(gpin->pin_sc->sc_bst,
				 gpin->pin_sc->sc_bsh,
				 EXYNOS_GPIO_DAT) >> gpin->pin_no) & 1;

	if (!raw && gpin->pin_actlo)
		val = !val;

	return val;
}

static void
exynos_gpio_fdt_write(device_t dev, void *priv, int val, bool raw)
{
	struct exynos_gpio_pin *gpin = priv;

	if (!raw && gpin->pin_actlo)
		val = !val;

	val = bus_space_read_1(gpin->pin_sc->sc_bst,
			       gpin->pin_sc->sc_bsh,
			       EXYNOS_GPIO_DAT);
	val &= ~__BIT(gpin->pin_no);
	if (val)
		val |= __BIT(gpin->pin_no);
	bus_space_write_1(gpin->pin_sc->sc_bst,
			  gpin->pin_sc->sc_bsh,
			  EXYNOS_GPIO_DAT, val);

}
