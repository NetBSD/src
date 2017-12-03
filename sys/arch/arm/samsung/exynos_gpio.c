/*	$NetBSD: exynos_gpio.c,v 1.7.2.3 2017/12/03 11:35:56 jdolecek Exp $ */

/*-
* Copyright (c) 2014 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Reinoud Zandijk
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
__KERNEL_RCSID(1, "$NetBSD: exynos_gpio.c,v 1.7.2.3 2017/12/03 11:35:56 jdolecek Exp $");

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

	uint8_t			bank_pin_mask;
	uint8_t			bank_pin_inuse_mask;
	bus_space_handle_t	bank_bsh;
	struct exynos_gpio_pin_cfg bank_cfg;
	struct exynos_gpio_bank * bank_next;
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

static struct exynos_gpio_bank exynos5_banks[] = {
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

struct exynos_gpio_bank *exynos_gpio_banks = exynos5_banks;

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

static void
exynos_gpio_update_cfg_regs(struct exynos_gpio_bank *bank,
	const struct exynos_gpio_pin_cfg *ncfg)
{
	if (bank->bank_cfg.cfg != ncfg->cfg) {
		GPIO_WRITE(bank, EXYNOS_GPIO_CON, ncfg->cfg);
		bank->bank_cfg.cfg = ncfg->cfg;
	}
	if (bank->bank_cfg.pud != ncfg->pud) {
		GPIO_WRITE(bank, EXYNOS_GPIO_PUD, ncfg->pud);
		bank->bank_cfg.pud = ncfg->pud;
	}

	if (bank->bank_cfg.drv != ncfg->drv) {
		GPIO_WRITE(bank, EXYNOS_GPIO_DRV, ncfg->drv);
		bank->bank_cfg.drv = ncfg->drv;
	}
	if (bank->bank_cfg.conpwd != ncfg->conpwd) {
		GPIO_WRITE(bank, EXYNOS_GPIO_CONPWD, ncfg->conpwd);
		bank->bank_cfg.conpwd = ncfg->conpwd;
	}
	if (bank->bank_cfg.pudpwd != ncfg->pudpwd) {
		GPIO_WRITE(bank, EXYNOS_GPIO_PUDPWD, ncfg->pudpwd);
		bank->bank_cfg.pudpwd = ncfg->pudpwd;
	}
}

static int
exynos_gpio_pin_read(void *cookie, int pin)
{
	struct exynos_gpio_bank * const bank = cookie;

	KASSERT(pin < bank->bank_bits);
	return (bus_space_read_1(bank->bank_sc->sc_bst,
				 bank->bank_sc->sc_bsh,
		EXYNOS_GPIO_DAT) >> pin) & 1;
}

static void
exynos_gpio_pin_write(void *cookie, int pin, int value)
{
	struct exynos_gpio_bank * const bank = cookie;
	int val;

	KASSERT(pin < bank->bank_bits);
	val = bus_space_read_1(bank->bank_sc->sc_bst,
			       bank->bank_sc->sc_bsh,
			       EXYNOS_GPIO_DAT);
	val &= ~__BIT(pin);
	if (value)
		val |= __BIT(pin);
	bus_space_write_1(bank->bank_sc->sc_bst,
			  bank->bank_sc->sc_bsh,
		EXYNOS_GPIO_DAT, val);
}

static void
exynos_gpio_pin_ctl(void *cookie, int pin, int flags)
{
	struct exynos_gpio_bank * const bank = cookie;
	struct exynos_gpio_pin_cfg ncfg = bank->bank_cfg;
	u_int shift;
	int pull;

	/* honour pullup requests */
	pull = EXYNOS_GPIO_PIN_FLOAT;
	if (flags & GPIO_PIN_PULLUP)
		pull = EXYNOS_GPIO_PIN_PULL_UP;
	if (flags & GPIO_PIN_PULLDOWN)
		pull = EXYNOS_GPIO_PIN_PULL_DOWN;
	shift = (pin & 7) << 1;
	ncfg.pud &= ~(0x3 << shift);
	ncfg.pud |= pull << shift;

	/* honour i/o */
	if (flags & GPIO_PIN_INPUT) {
		ncfg.cfg &= ~(0x0f << shift);
		ncfg.cfg |= EXYNOS_GPIO_FUNC_INPUT << shift;
	} else if (flags & GPIO_PIN_OUTPUT) {
		ncfg.cfg &= ~(0x0f << shift);
		ncfg.cfg |= EXYNOS_GPIO_FUNC_OUTPUT << shift;
	}

	/* update any config registers that changed */
	exynos_gpio_update_cfg_regs(bank, &ncfg);
}

void exynos_gpio_pin_ctl_read(const struct exynos_gpio_bank *bank,
			      struct exynos_gpio_pin_cfg *cfg)
{
	cfg->cfg = GPIO_READ(bank, EXYNOS_GPIO_CON);
	cfg->pud = GPIO_READ(bank, EXYNOS_GPIO_PUD);
	cfg->drv = GPIO_READ(bank, EXYNOS_GPIO_DRV);
	cfg->conpwd = GPIO_READ(bank, EXYNOS_GPIO_CONPWD);
	cfg->pudpwd = GPIO_READ(bank, EXYNOS_GPIO_PUDPWD);
}

void exynos_gpio_pin_ctl_write(const struct exynos_gpio_bank *bank,
			       const struct exynos_gpio_pin_cfg *cfg)
{
		GPIO_WRITE(bank, EXYNOS_GPIO_CON, cfg->cfg);
		GPIO_WRITE(bank, EXYNOS_GPIO_PUD, cfg->pud);
		GPIO_WRITE(bank, EXYNOS_GPIO_DRV, cfg->drv);
		GPIO_WRITE(bank, EXYNOS_GPIO_CONPWD, cfg->conpwd);
		GPIO_WRITE(bank, EXYNOS_GPIO_PUDPWD, cfg->pudpwd);
}

struct exynos_gpio_softc *
exynos_gpio_bank_config(struct exynos_pinctrl_softc * parent,
			const struct fdt_attach_args *faa, int node)
{
	struct exynos_gpio_bank *bank = kmem_zalloc(sizeof(*bank), KM_SLEEP);
	struct exynos_gpio_softc *sc = kmem_zalloc(sizeof(*sc), KM_SLEEP);
	struct gpiobus_attach_args gba;
	struct gpio_chipset_tag *gc_tag;
	char result[64];

	OF_getprop(node, "name", result, sizeof(result));
	bank = exynos_gpio_bank_lookup(result);
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
	bank->bank_dev = config_found_ia(parent->sc_dev, "gpiobus", &gba,
					 exynos_gpio_cfprint);

	bank->bank_pin_mask = __BIT(bank->bank_bits) - 1;
	bank->bank_pin_inuse_mask = 0;


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
exynos_gpio_bank_lookup(const char *name)
{
	struct exynos_gpio_bank *bank;

	for (u_int n = 0; n < __arraycount(exynos5_banks); n++) {
		bank = &exynos_gpio_banks[n];
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
	const u_int *cells = data;
	struct exynos_gpio_bank *bank = NULL;
	struct exynos_gpio_pin *gpin;
	int n;

	if (len != 12)
		return NULL;

	const int pin = be32toh(cells[1]) & 0x0f;
	const int actlo = be32toh(cells[2]) & 0x01;

	for (n = 0; n < __arraycount(exynos5_banks); n++) {
		if (exynos_gpio_banks[n].bank_dev == dev) {
			bank = &exynos_gpio_banks[n];
			break;
		}
	}
	if (bank == NULL)
		return NULL;

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
