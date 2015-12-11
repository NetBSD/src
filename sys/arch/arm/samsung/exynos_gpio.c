/*	$NetBSD: exynos_gpio.c,v 1.13 2015/12/11 04:03:44 marty Exp $ */

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
__KERNEL_RCSID(1, "$NetBSD: exynos_gpio.c,v 1.13 2015/12/11 04:03:44 marty Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_io.h>
#include <arm/samsung/exynos_intr.h>

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
	struct exynos_gpio_softc *bank_sc;
	device_t		bank_dev;
	struct gpio_chipset_tag	bank_gc;
	gpio_pin_t		bank_pins[8];

	const bus_addr_t	bank_core_offset;
	const uint8_t		bank_bits;

	uint8_t			bank_pin_mask;
	uint8_t			bank_pin_inuse_mask;
	bus_space_handle_t	bank_bsh;
	struct exynos_gpio_pin_cfg bank_cfg;
};

struct exynos_gpio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct exynos_gpio_bank *sc_banks;
};

struct exynos_gpio_pin {
	struct exynos_gpio_softc *pin_sc;
	const struct exynos_gpio_bank  *pin_bank;
	int			  pin_no;
	u_int			  pin_flags;
};

#define GPIO_REG(v,s,o) (EXYNOS##v##_GPIO_##s##_OFFSET + (o))
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

//	GPIO_GRP(5, MUXE, 0x0000, gpz0,  7),
};

static int exynos_gpio_match(device_t, cfdata_t, void *);
static void exynos_gpio_attach(device_t, device_t, void *);

static int exynos_gpio_pin_read(void *, int);
static void exynos_gpio_pin_write(void *, int, int);
static void exynos_gpio_pin_ctl(void *, int, int);
static int exynos_gpio_cfprint(void *, const char *);
struct exynos_gpio_pin *exynos_gpio_acquire(const char *pinname, u_int flags);

/* force these structures in DATA segment */
static struct exynos_gpio_bank *exynos_gpio_banks = NULL;
static int exynos_n_gpio_banks = 0;

static struct exynos_gpio_softc exynos_gpio_sc = {};

CFATTACH_DECL_NEW(exynos_gpio, sizeof(struct exynos_gpio_softc),
	exynos_gpio_match, exynos_gpio_attach, NULL, NULL);

static int
exynos_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
#ifdef DIAGNOSTIC
	struct exyo_attach_args * const exyoaa = aux;
	struct exyo_locators *loc = &exyoaa->exyo_loc;
#endif

	/* no locators expected */
	KASSERT(loc->loc_offset == 0);
	KASSERT(loc->loc_size   == 0);
	KASSERT(loc->loc_port   == EXYOCF_PORT_DEFAULT);

	/* there can only be one */
	if (exynos_gpio_sc.sc_dev != NULL)
		return 0;
	return 1;
}


#if NGPIO > 0
static void
exynos_gpio_attach_banks(device_t self)
{
	struct exynos_gpio_softc * const sc = &exynos_gpio_sc;
	struct exynos_gpio_bank *bank;
	struct gpiobus_attach_args gba;
	size_t pin_count = 0;
	int i, bit, mask, pincaps, data;

	if (exynos_n_gpio_banks == 0)
		return;

	/* find out how many pins we can offer */
	pin_count = 0;
	for (i = 0; i < exynos_n_gpio_banks; i++) {
		pin_count += exynos_gpio_banks[i].bank_bits;
	}

	/* if no pins available, don't proceed */
	if (pin_count == 0)
		return;

	/* allocate pin data */
	sc->sc_banks = exynos_gpio_banks;
	KASSERT(sc->sc_banks);

	pincaps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
		GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN;

	for (i = 0; i < exynos_n_gpio_banks; i++) {
		bank = &sc->sc_banks[i];
		bank->bank_sc = sc;
		bank->bank_gc.gp_cookie = bank;
		bank->bank_gc.gp_pin_read = exynos_gpio_pin_read;
		bank->bank_gc.gp_pin_write = exynos_gpio_pin_write;
		bank->bank_gc.gp_pin_ctl = exynos_gpio_pin_ctl;
		mask = bank->bank_pin_mask & ~bank->bank_pin_inuse_mask;
		if (mask == 0)
			continue;
		data = bus_space_read_1(sc->sc_bst, bank->bank_bsh,
				EXYNOS_GPIO_DAT);
		for (bit = 0; mask != 0; mask >>= 1, data >>= 1, bit++) {
			if (mask & 1) {
				bank->bank_pins[bit].pin_num = bit + (i << 3);
				bank->bank_pins[bit].pin_caps = pincaps;
				bank->bank_pins[bit].pin_flags = pincaps;
				bank->bank_pins[bit].pin_state = (data & 1) != 0;
			}
		}
		memset(&gba, 0, sizeof(gba));
		gba.gba_gc = &bank->bank_gc;
		gba.gba_pins = bank->bank_pins;
		gba.gba_npins = bit; /* MJF? */
		bank->bank_dev = config_found_ia(self, "gpiobus", &gba,
						 exynos_gpio_cfprint);
	}
}
#endif

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
exynos_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_gpio_softc * const sc = &exynos_gpio_sc;
	struct exyo_attach_args * const exyoaa = aux;

	sc->sc_dev = self;
	sc->sc_bst = exyoaa->exyo_core_bst;
	sc->sc_bsh = exyoaa->exyo_core_bsh;

	exynos_gpio_bootstrap();
	if (exynos_n_gpio_banks == 0) {
		printf(": disabled, no pins defined\n");
		return;
	}

	KASSERT(exynos_gpio_banks);
	KASSERT(exynos_n_gpio_banks);

	aprint_naive("\n");
	aprint_normal("\n");

#if NGPIO > 0
	exynos_gpio_attach_banks(self);
#endif
}

static void
exynos_gpio_set_pin_func(struct exynos_gpio_pin_cfg *cfg,
	int pin, int func)
{
	const u_int shift = (pin & 7) << 2;

	cfg->cfg &= ~(0x0f << shift);
	cfg->cfg |= func << shift;
}


static void
exynos_gpio_set_pin_pull(struct exynos_gpio_pin_cfg *cfg, int pin, int pull)
{
	const u_int shift = (pin & 7) << 1;

	cfg->pud &= ~(0x3 << shift);
	cfg->pud |= pull << shift;
}

static int
exynos_gpio_pin_read(void *cookie, int pin)
{
	struct exynos_gpio_bank * const bank = cookie;

	KASSERT(pin < bank->bank_bits);
	return (bus_space_read_1(exynos_gpio_sc.sc_bst, bank->bank_bsh,
		EXYNOS_GPIO_DAT) >> pin) & 1;
}

static void
exynos_gpio_pin_write(void *cookie, int pin, int value)
{
	struct exynos_gpio_bank * const bank = cookie;
	int val;

	KASSERT(pin < bank->bank_bits);
	val = bus_space_read_1(exynos_gpio_sc.sc_bst, bank->bank_bsh,
		EXYNOS_GPIO_DAT);
	val &= ~__BIT(pin);
	if (value)
		val |= __BIT(pin);
	bus_space_write_1(exynos_gpio_sc.sc_bst, bank->bank_bsh,
		EXYNOS_GPIO_DAT, val);
}

static void
exynos_gpio_update_cfg_regs(struct exynos_gpio_bank *bank,
	const struct exynos_gpio_pin_cfg *ncfg)
{
	bus_space_tag_t bst = &exynos_bs_tag;

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
	int pull;

	/* honour pullup requests */
	pull = EXYNOS_GPIO_PIN_FLOAT;
	if (flags & GPIO_PIN_PULLUP)
		pull = EXYNOS_GPIO_PIN_PULL_UP;
	if (flags & GPIO_PIN_PULLDOWN)
		pull = EXYNOS_GPIO_PIN_PULL_DOWN;
	exynos_gpio_set_pin_pull(&ncfg, pin, pull);

	/* honour i/o */
	if (flags & GPIO_PIN_INPUT)
		exynos_gpio_set_pin_func(&ncfg, pin, EXYNOS_GPIO_FUNC_INPUT);
	if (flags & GPIO_PIN_OUTPUT)
		exynos_gpio_set_pin_func(&ncfg, pin, EXYNOS_GPIO_FUNC_OUTPUT);

	/* update any config registers that changed */
	exynos_gpio_update_cfg_regs(bank, &ncfg);
}

/* bootstrapping */
void
exynos_gpio_bootstrap(void)
{
	bus_space_tag_t bst = &exynos_bs_tag;
	struct exynos_gpio_bank *bank;
	struct gpio_chipset_tag *gc_tag;
	int i;

	/* determine what we're running on */
	if (IS_EXYNOS5_P()) {
		exynos_gpio_banks = exynos5_banks;
		exynos_n_gpio_banks = __arraycount(exynos5_banks);
	}

	if (exynos_n_gpio_banks == 0)
		return;

	/* init banks */
	for (i = 0; i < exynos_n_gpio_banks; i++) {
		bank = &exynos_gpio_banks[i];
		gc_tag = &bank->bank_gc;

		bus_space_subregion(&exynos_bs_tag, exynos_core_bsh,
			bank->bank_core_offset, EXYNOS_GPIO_GRP_SIZE,
			&bank->bank_bsh);
		KASSERT(&bank->bank_bsh);

		bank->bank_pin_mask = __BIT(bank->bank_bits) - 1;
		bank->bank_pin_inuse_mask = 0;

		gc_tag->gp_cookie = bank;
		gc_tag->gp_pin_read  = exynos_gpio_pin_read;
		gc_tag->gp_pin_write = exynos_gpio_pin_write;
		gc_tag->gp_pin_ctl   = exynos_gpio_pin_ctl;

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
	}

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
	u_int n;
	int pin;

	KASSERT(strlen(pinname) == 2 || strlen(pinname) == 3);

	memset(bankname, 0, sizeof(bankname));
	bankname[0] = pinname[0]; /* 'g' */
	bankname[1] = pinname[1]; /* 'p' */
	bankname[2] = pinname[2]; /*  L  */
	bankname[3] = pinname[3]; /*  D  */
	pin = pinname[5] - '0';	  /* skip the '-' */

	for (n = 0; n < __arraycount(exynos_gpio_banks); n++) {
		const struct exynos_gpio_bank *pb =
		    &exynos_gpio_banks[n];
		if (strcmp(pb->bank_name, bankname) == 0) {
			*ppin = pin;
			return pb;
		}
	}

	return NULL;
}

struct exynos_gpio_pin *
exynos_gpio_acquire(const char *pinname, u_int flags)
{
	const struct exynos_gpio_bank *bank;
	struct exynos_gpio_pin *gpin;
	int pin;

	bank = exynos_gpio_pin_lookup(pinname, &pin);
	if (bank == NULL)
		return NULL;

	gpin = kmem_alloc(sizeof(*gpin), KM_SLEEP);
	gpin->pin_sc = bank->bank_sc;
	gpin->pin_bank = bank;
	gpin->pin_no = pin;
	gpin->pin_flags = flags;

	exynos_gpio_pin_ctl(&gpin->pin_bank, gpin->pin_no, gpin->pin_flags);

	return gpin;
}
