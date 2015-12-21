/*	$NetBSD: exynos_gpio.c,v 1.15 2015/12/21 04:58:50 marty Exp $ */

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
__KERNEL_RCSID(1, "$NetBSD: exynos_gpio.c,v 1.15 2015/12/21 04:58:50 marty Exp $");

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

struct exynos_gpio_pin_cfg {
	uint32_t cfg;
	uint32_t pud;
	uint32_t drv;
	uint32_t conpwd;
	uint32_t pudpwd;
};

struct exynos_gpio_softc;

struct exynos_gpio_bank {
	const char		bank_name[6];
	device_t		bank_dev;
	struct gpio_chipset_tag	bank_gc;
	struct exynos_gpio_softc *bank_softc;
	gpio_pin_t		bank_pins[8];

	const bus_addr_t	bank_core_offset;
	const uint8_t		bank_bits;

	uint8_t			bank_pin_mask;
	uint8_t			bank_pin_inuse_mask;
	bus_space_handle_t	bank_bsh;
	struct exynos_gpio_pin_cfg bank_cfg;
	struct exynos_gpio_bank * bank_next;
};

struct exynos_gpio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};

struct exynos_gpio_pin {
	struct exynos_gpio_softc *pin_sc;
	int			  pin_no;
	u_int			  pin_flags;
	int			  pin_actlo;
	const struct exynos_gpio_bank   *pin_bank;
};

struct exynos_gpio_bank *exynos_gpio_banks;

static void exynos_gpio_pin_ctl(void *cookie, int pin, int flags);
static void *exynos_gpio_fdt_acquire(device_t, const void *,
				     size_t, int);
static void exynos_gpio_fdt_release(device_t, void *);

static int exynos_gpio_fdt_read(device_t, void *);
static void exynos_gpio_fdt_write(device_t, void *, int);
//static int exynos_gpio_cfprint(void *, const char *);

struct fdtbus_gpio_controller_func exynos_gpio_funcs = {
	.acquire = exynos_gpio_fdt_acquire,
	.release = exynos_gpio_fdt_release,
	.read = exynos_gpio_fdt_read,
	.write = exynos_gpio_fdt_write
};

#if 0
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
#endif

static void
exynos_gpio_update_cfg_regs(struct exynos_gpio_bank *bank,
	const struct exynos_gpio_pin_cfg *ncfg)
{
	bus_space_tag_t bst = &armv7_generic_bs_tag;

	if (bank->bank_cfg.cfg != ncfg->cfg) {
		bus_space_write_4(bst, bank->bank_bsh,
			EXYNOS_GPIO_CON, ncfg->cfg);
		bank->bank_cfg.cfg = ncfg->cfg;
	}
	if (bank->bank_cfg.pud != ncfg->pud) {
		bus_space_write_4(bst, bank->bank_bsh,
			EXYNOS_GPIO_PUD, ncfg->pud);
		bank->bank_cfg.pud = ncfg->pud;
	}

	/* the following attributes are not yet setable */
#if 0
	if (bank->bank_cfg.drv != ncfg->drv) {
		bus_space_write_4(bst, bank->bank_bsh,
			EXYNOS_GPIO_DRV, ncfg->drv);
		bank->bank_cfg.drv = ncfg->drv;
	}
	if (bank->bank_cfg.conpwd != ncfg->conpwd) {
		bus_space_write_4(bst, bank->bank_bsh,
			EXYNOS_GPIO_CONPWD, ncfg->conpwd);
		bank->bank_cfg.conpwd = ncfg->conpwd;
	}
	if (bank->bank_cfg.pudpwd != ncfg->pudpwd) {
		bus_space_write_4(bst, bank->bank_bsh,
			EXYNOS_GPIO_PUDPWD, ncfg->pudpwd);
		bank->bank_cfg.pudpwd = ncfg->pudpwd;
	}
#endif
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

void
exynos_gpio_bank_config(struct exynos_pinctrl_softc * parent, int node)
{
//	bus_space_tag_t bst = &exynos_bs_tag; /* MJF: This is wrong */
	struct exynos_gpio_bank *bank = kmem_zalloc(sizeof(*bank), KM_SLEEP);
//	struct exynos_gpio_softc *sc = kmem_zalloc(sizeof(*sc), KM_SLEEP);
//	struct gpiobus_attach_args gba;
	char result[64];
//	int data;
//	int error;

	/*error =*/ OF_getprop(node, "name", result, sizeof(result));
	printf(" GPIO %s\n", result);
	if (exynos_gpio_banks)
		bank->bank_next = exynos_gpio_banks;
	exynos_gpio_banks = bank;
	/* MJF: TODO: process all of the node properties */
#if 0
//	pincaps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
//		GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN;

//	data = bus_space_read_1(sc->sc_bst, bank->bank_bsh,
//				EXYNOS_GPIO_DAT);

	sc->sc_dev = parent->sc_dev;
	sc->sc_bst = parent->sc_bst;
	
	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = &bank->bank_gc;
	gba.gba_pins = bank->bank_pins;
//	gba.gba_npins = bit; /* MJF? */
	bank->bank_dev = config_found_ia(parent->sc_dev, "gpiobus", &gba,
					 exynos_gpio_cfprint);

	bank->bank_pin_mask = __BIT(bank->bank_bits) - 1;
	bank->bank_pin_inuse_mask = 0;


	/* read in our initial settings */
	bank->bank_cfg.cfg = bus_space_read_4(bst, bank->bank_bsh,
					      EXYNOS_GPIO_CON);
	bank->bank_cfg.pud = bus_space_read_4(bst, bank->bank_bsh,
					      EXYNOS_GPIO_PUD);
	bank->bank_cfg.drv = bus_space_read_4(bst, bank->bank_bsh,
					      EXYNOS_GPIO_DRV);
	bank->bank_cfg.conpwd = bus_space_read_4(bst, bank->bank_bsh,
						 EXYNOS_GPIO_CONPWD);
	bank->bank_cfg.pudpwd = bus_space_read_4(bst, bank->bank_bsh,
						 EXYNOS_GPIO_PUDPWD);
	/* MJF: TODO: Configure from the node data, if present */
#endif
}

/*
 * pinmame = gpLD-N
 *     L = 'a' - 'z' -+
 *     D = '0' - '8' -+ ===== bank name
 *     N = '0' - '8'    ===== pin number
 */

static const struct exynos_gpio_bank *
exynos_gpio_pin_lookup(const char *pinname, int *ppin)
{
	char bankname[5];
	int pin;
	struct exynos_gpio_bank *bank;

	KASSERT(strlen(pinname) == 2 || strlen(pinname) == 3);

	memset(bankname, 0, sizeof(bankname));
	bankname[0] = pinname[0]; /* 'g' */
	bankname[1] = pinname[1]; /* 'p' */
	bankname[2] = pinname[2]; /*  L  */
	bankname[3] = pinname[3]; /*  D  */
	pin = pinname[5] - '0';	  /* skip the '-' */

	for (bank = exynos_gpio_banks; bank; bank = bank->bank_next)
		if (strcmp(bank->bank_name, bankname) == 0) {
			*ppin = pin;
			return bank;
		}

	return NULL;
}

static void *
exynos_gpio_fdt_acquire(device_t dev, const void *data, size_t len, int flags)
{
	/* MJF:  This is wrong.  data is a u_int but I need a name */
//	const u_int *gpio = data;
	const char *pinname = data;
	const struct exynos_gpio_bank *bank;
	struct exynos_gpio_pin *gpin;
	int pin;

	bank = exynos_gpio_pin_lookup(pinname, &pin);
	if (bank == NULL)
		return NULL;

	gpin = kmem_alloc(sizeof(*gpin), KM_SLEEP);
	gpin->pin_sc = bank->bank_softc;
	gpin->pin_bank = bank;
	gpin->pin_no = pin;
	gpin->pin_flags = flags;
	gpin->pin_actlo = 0;

	exynos_gpio_pin_ctl(&gpin->pin_bank, gpin->pin_no, gpin->pin_flags);

	return gpin;
}

static void
exynos_gpio_fdt_release(device_t dev, void *priv)
{
	struct exynos_gpio_pin *gpin = priv;

	kmem_free(gpin, sizeof(*gpin));
}

static int
exynos_gpio_fdt_read(device_t dev, void *priv)
{
	struct exynos_gpio_pin *gpin = priv;
	int val;

	val = (bus_space_read_1(gpin->pin_sc->sc_bst,
				 gpin->pin_sc->sc_bsh,
				 EXYNOS_GPIO_DAT) >> gpin->pin_no) & 1;

	if (gpin->pin_actlo)
		val = !val;

	return val;
}

static void
exynos_gpio_fdt_write(device_t dev, void *priv, int val)
{
	struct exynos_gpio_pin *gpin = priv;

	if (gpin->pin_actlo)
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
