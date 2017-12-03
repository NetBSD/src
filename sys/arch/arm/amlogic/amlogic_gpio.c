/* $NetBSD: amlogic_gpio.c,v 1.2.18.2 2017/12/03 11:35:51 jdolecek Exp $ */

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

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amlogic_gpio.c,v 1.2.18.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>

#include <arm/amlogic/amlogic_reg.h>
#include <arm/amlogic/amlogic_crureg.h>
#include <arm/amlogic/amlogic_var.h>

#define _C(x)	(AMLOGIC_CBUS_OFFSET + (x))
#define _AO(x)	(AMLOGIC_GPIOAO_OFFSET + (x))
const struct amlogic_gpio_pingrp {
	const char *name;
	u_int mask;
	bus_size_t en_reg;
	u_int en_shift;
	bus_size_t out_reg;
	u_int out_shift;
	bus_size_t in_reg;
	u_int in_shift;
	bus_size_t pupd_en_reg;
	u_int pupd_en_shift;
	bus_size_t pupd_reg;
	u_int pupd_shift;
} amlogic_gpio_pingrps[] = {
	{ "GPIOX", 0x3fffff,
	  _C(PREG_PAD_GPIO0_EN_N_REG), 0,
	  _C(PREG_PAD_GPIO0_OUT_REG), 0,
	  _C(PREG_PAD_GPIO0_IN_REG), 0,
	  _C(PAD_PULL_UP_EN_4_REG), 0,
	  _C(PAD_PULL_UP_4_REG), 0 },
	{ "GPIOY", 0x7fff,
	  _C(PREG_PAD_GPIO1_EN_N_REG), 0,
	  _C(PREG_PAD_GPIO1_OUT_REG), 0,
	  _C(PREG_PAD_GPIO1_IN_REG), 0,
	  _C(PAD_PULL_UP_EN_3_REG), 0,
	  _C(PAD_PULL_UP_3_REG), 0 },
	{ "GPIODV", 0x3f000200,
	  _C(PREG_PAD_GPIO2_EN_N_REG), 0,
	  _C(PREG_PAD_GPIO2_OUT_REG), 0,
	  _C(PREG_PAD_GPIO2_IN_REG), 0,
	  _C(PAD_PULL_UP_EN_0_REG), 0,
	  _C(PAD_PULL_UP_0_REG), 0 },
	{ "GPIOH", 0x3f,
	  _C(PREG_PAD_GPIO3_EN_N_REG), 19,
	  _C(PREG_PAD_GPIO3_OUT_REG), 19,
	  _C(PREG_PAD_GPIO3_IN_REG), 19,
	  _C(PAD_PULL_UP_EN_1_REG), 16,
	  _C(PAD_PULL_UP_1_REG), 16 },
	{ "GPIOAO", 0x3fff,
	  _AO(AMLOGIC_GPIOAO_EN_N_REG), 0,
	  _AO(AMLOGIC_GPIOAO_OUT_REG), 16,
	  _AO(AMLOGIC_GPIOAO_IN_REG), 0,
	  _AO(AMLOGIC_GPIOAO_PUPD_EN_REG), 0,
	  _AO(AMLOGIC_GPIOAO_PUPD_REG), 16 },
	{ "BOOT", 0x4ffff,
	  _C(PREG_PAD_GPIO3_EN_N_REG), 0,
	  _C(PREG_PAD_GPIO3_OUT_REG), 0,
	  _C(PREG_PAD_GPIO3_IN_REG), 0,
	  _C(PAD_PULL_UP_EN_2_REG), 0,
	  _C(PAD_PULL_UP_2_REG), 0 },
	{ "CARD", 0x7f,
	  _C(PREG_PAD_GPIO0_EN_N_REG), 22,
	  _C(PREG_PAD_GPIO0_OUT_REG), 22,
	  _C(PREG_PAD_GPIO0_IN_REG), 22,
	  _C(PAD_PULL_UP_EN_2_REG), 20,
	  _C(PAD_PULL_UP_2_REG), 20 }
};
#undef _C
#undef _AO

static int	amlogic_gpio_match(device_t, cfdata_t, void *);
static void	amlogic_gpio_attach(device_t, device_t, void *);

struct amlogic_gpio_softc;

struct amlogic_gpio_group {
	struct amlogic_gpio_softc *grp_sc;
	const struct amlogic_gpio_pingrp *grp_pg;
	device_t		grp_dev;
	struct gpio_chipset_tag	grp_gc;
	gpio_pin_t		grp_pins[32];
};

struct amlogic_gpio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct amlogic_gpio_group *sc_groups;
};

static void	amlogic_gpio_attach_group(struct amlogic_gpio_softc *, u_int);

static int	amlogic_gpio_pin_read(void *, int);
static void	amlogic_gpio_pin_write(void *, int, int);
static void	amlogic_gpio_pin_ctl(void *, int, int);

static int	amlogic_gpio_cfprint(void *, const char *);

CFATTACH_DECL_NEW(amlogic_gpio, sizeof(struct amlogic_gpio_softc),
	amlogic_gpio_match, amlogic_gpio_attach, NULL, NULL);

#define GPIO_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define GPIO_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define GPIO_SET_CLEAR(sc, reg, set, clr) \
	amlogic_reg_set_clear((sc)->sc_bst, (sc)->sc_bsh, \
	    (reg), (set), (clr))

static int
amlogic_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
amlogic_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct amlogic_gpio_softc * const sc = device_private(self);
	struct amlogicio_attach_args * const aio = aux;
	u_int n;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	sc->sc_bsh = aio->aio_bsh;

	aprint_naive("\n");
	aprint_normal(": GPIO controller\n");

	const u_int ngrp = __arraycount(amlogic_gpio_pingrps);
	sc->sc_groups = kmem_zalloc(sizeof(*sc->sc_groups) * ngrp, KM_SLEEP);
	for (n = 0; n < ngrp; n++) {
		amlogic_gpio_attach_group(sc, n);
	}
}

static void
amlogic_gpio_attach_group(struct amlogic_gpio_softc *sc, u_int grpno)
{
	struct amlogic_gpio_group *grp = &sc->sc_groups[grpno];
	struct gpiobus_attach_args gba;
	u_int pin;

	grp->grp_sc = sc;
	grp->grp_pg = &amlogic_gpio_pingrps[grpno];
	grp->grp_gc.gp_cookie = grp;
	grp->grp_gc.gp_pin_read = amlogic_gpio_pin_read;
	grp->grp_gc.gp_pin_write = amlogic_gpio_pin_write;
	grp->grp_gc.gp_pin_ctl = amlogic_gpio_pin_ctl;
	for (pin = 0; pin < __arraycount(grp->grp_pins); pin++) {
		grp->grp_pins[pin].pin_num = pin;
		if ((grp->grp_pg->mask & (1 << pin)) == 0)
			continue;
		grp->grp_pins[pin].pin_caps =
		    GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
		    GPIO_PIN_PUSHPULL | GPIO_PIN_TRISTATE |
		    GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN;
		grp->grp_pins[pin].pin_state =
		    amlogic_gpio_pin_read(grp, pin);
	}

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = &grp->grp_gc;
	gba.gba_pins = grp->grp_pins;
	gba.gba_npins = 32 - __builtin_clz(grp->grp_pg->mask);

	grp->grp_dev = config_found_ia(sc->sc_dev, "gpiobus", &gba,
	    amlogic_gpio_cfprint);
}

static int
amlogic_gpio_cfprint(void *priv, const char *pnp)
{
	struct gpiobus_attach_args *gba = priv;
	struct amlogic_gpio_group *grp = gba->gba_gc->gp_cookie;
	const char *grpname = grp->grp_pg->name;

	if (pnp)
		aprint_normal("gpiobus at %s", pnp);

	aprint_normal(" (%s)", grpname);

	return UNCONF;
}

static int
amlogic_gpio_pin_read(void *priv, int pin)
{
	struct amlogic_gpio_group *grp = priv;
	struct amlogic_gpio_softc *sc = grp->grp_sc;
	const bus_size_t reg = grp->grp_pg->in_reg;
	const u_int shift = grp->grp_pg->in_shift;
	const uint32_t mask = grp->grp_pg->mask;

	const uint32_t v = (GPIO_READ(sc, reg) >> shift) & mask;

	return (v >> pin) & 1;
}

static void
amlogic_gpio_pin_write(void *priv, int pin, int val)
{
	struct amlogic_gpio_group *grp = priv;
	struct amlogic_gpio_softc *sc = grp->grp_sc;
	const bus_size_t reg = grp->grp_pg->out_reg;
	const u_int shift = grp->grp_pg->out_shift;
	const uint32_t mask __diagused = grp->grp_pg->mask;
	uint32_t v;

	KASSERT(mask & (1 << pin));

	v = GPIO_READ(sc, reg);
	if (val) {
		v |= ((1 << pin) << shift);
	} else {
		v &= ~((1 << pin) << shift);
	}
	GPIO_WRITE(sc, reg, v);
}

static void
amlogic_gpio_pin_ctl(void *priv, int pin, int flags)
{
	struct amlogic_gpio_group *grp = priv;
	struct amlogic_gpio_softc *sc = grp->grp_sc;
	const bus_size_t en_reg = grp->grp_pg->en_reg;
	const bus_size_t pupd_en_reg = grp->grp_pg->pupd_en_reg;
	const bus_size_t pupd_reg = grp->grp_pg->pupd_reg;
	const u_int en_shift = grp->grp_pg->en_shift;
	const u_int pupd_en_shift = grp->grp_pg->pupd_en_shift;
	const u_int pupd_shift = grp->grp_pg->pupd_shift;
	const uint32_t mask __diagused = grp->grp_pg->mask;
	uint32_t v, pupd_en, pupd;

	KASSERT(mask & (1 << pin));

	if (flags & GPIO_PIN_INPUT) {
		v = GPIO_READ(sc, en_reg);
		v |= ((1 << pin) << en_shift);
		GPIO_WRITE(sc, en_reg, v);
	} else if (flags & GPIO_PIN_OUTPUT) {
		v = GPIO_READ(sc, en_reg);
		v &= ~((1 << pin) << en_shift);
		GPIO_WRITE(sc, en_reg, v);
	}

	pupd_en = GPIO_READ(sc, pupd_en_reg);
	if (flags & (GPIO_PIN_PULLUP|GPIO_PIN_PULLDOWN)) {
		pupd = GPIO_READ(sc, pupd_reg);
		if (flags & GPIO_PIN_PULLUP) {
			pupd |= ((1 << pin) << pupd_shift);
		} else {
			pupd &= ~((1 << pin) << pupd_shift);
		}
		GPIO_WRITE(sc, pupd_reg, pupd);
		pupd_en |= ((1 << pin) << pupd_en_shift);
	} else {
		pupd_en &= ~((1 << pin) << pupd_en_shift);
	}
	GPIO_WRITE(sc, pupd_en_reg, pupd_en);
}
