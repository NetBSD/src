/* $NetBSD: meson_pinctrl.c,v 1.4 2019/03/02 11:15:55 jmcneill Exp $ */

/*-
 * Copyright (c) 2019 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "opt_soc.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: meson_pinctrl.c,v 1.4 2019/03/02 11:15:55 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>

#include <dev/fdt/fdtvar.h>

#include <arm/amlogic/meson_pinctrl.h>

struct meson_pinctrl_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh_mux;
	bus_space_handle_t	sc_bsh_pull;
	bus_space_handle_t	sc_bsh_pull_enable;
	bus_space_handle_t	sc_bsh_gpio;
	int			sc_phandle;
	int			sc_phandle_gpio;

	kmutex_t		sc_lock;

	const struct meson_pinctrl_config *sc_conf;

	struct gpio_chipset_tag	sc_gp;
	gpio_pin_t		*sc_pins;
};

struct meson_pinctrl_gpio_pin {
	struct meson_pinctrl_softc	*pin_sc;
	const struct meson_pinctrl_gpio	*pin_def;
	int				pin_flags;
	bool				pin_actlo;
};

static const struct of_compat_data compat_data[] = {
#ifdef SOC_MESON8B
	{ "amlogic,meson8b-aobus-pinctrl",	(uintptr_t)&meson8b_aobus_pinctrl_config },
	{ "amlogic,meson8b-cbus-pinctrl",	(uintptr_t)&meson8b_cbus_pinctrl_config },
#endif
#ifdef SOC_MESONGXBB
	{ "amlogic,meson-gxbb-aobus-pinctrl",	(uintptr_t)&mesongxbb_aobus_pinctrl_config },
	{ "amlogic,meson-gxbb-periphs-pinctrl",	(uintptr_t)&mesongxbb_periphs_pinctrl_config },
#endif
	{ NULL, 0 }
};

#define	MUX_READ(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh_mux, (reg))
#define	MUX_WRITE(sc, reg, val)				\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh_mux, (reg), (val))

static const struct meson_pinctrl_group *
meson_pinctrl_find_group(struct meson_pinctrl_softc *sc,
    const char *name)
{
	const struct meson_pinctrl_group *group;
	u_int n;

	for (n = 0; n < sc->sc_conf->ngroups; n++) {
		group = &sc->sc_conf->groups[n];
		if (strcmp(group->name, name) == 0)
			return group;
	}

	return NULL;
}

static bool
meson_pinctrl_group_in_bank(struct meson_pinctrl_softc *sc,
    const struct meson_pinctrl_group *group, u_int bankno)
{
	u_int n;

	for (n = 0; n < group->nbank; n++) {
		if (group->bank[n] == bankno)
			return true;
	}

	return false;
}

static void
meson_pinctrl_set_group(struct meson_pinctrl_softc *sc,
    const struct meson_pinctrl_group *group, bool enable)
{
	uint32_t val;

	val = MUX_READ(sc, group->reg);
	if (enable)
		val |= __BIT(group->bit);
	else
		val &= ~__BIT(group->bit);
	MUX_WRITE(sc, group->reg, val);
}

static void
meson_pinctrl_setfunc(struct meson_pinctrl_softc *sc, const char *name)
{
	const struct meson_pinctrl_group *group, *target_group;
	u_int n, bank;

	target_group = meson_pinctrl_find_group(sc, name);
	if (target_group == NULL) {
		aprint_error_dev(sc->sc_dev, "function '%s' not supported\n", name);
		return;
	}

	/* Disable conflicting groups */
	for (n = 0; n < sc->sc_conf->ngroups; n++) {
		group = &sc->sc_conf->groups[n];
		if (target_group == group)
			continue;
		for (bank = 0; bank < target_group->nbank; bank++) {
			if (meson_pinctrl_group_in_bank(sc, group, target_group->bank[bank]))
				meson_pinctrl_set_group(sc, group, false);
		}
	}

	/* Enable target group */
	meson_pinctrl_set_group(sc, target_group, true);
}

static int
meson_pinctrl_set_config(device_t dev, const void *data, size_t len)
{
	struct meson_pinctrl_softc * const sc = device_private(dev);
	const char *groups;
	int groups_len;

	if (len != 4)
		return -1;

	const int phandle = fdtbus_get_phandle_from_native(be32dec(data));
	const int mux = of_find_firstchild_byname(phandle, "mux");
	if (mux == -1)
		return -1;

	groups = fdtbus_pinctrl_parse_groups(mux, &groups_len);
	if (groups == NULL)
		return -1;

	for (; groups_len > 0;
	    groups_len -= strlen(groups) + 1, groups += strlen(groups) + 1) {
		meson_pinctrl_setfunc(sc, groups);
	}

	return 0;
}

static struct fdtbus_pinctrl_controller_func meson_pinctrl_funcs = {
	.set_config = meson_pinctrl_set_config,
};

static bus_space_handle_t
meson_pinctrl_gpio_handle(struct meson_pinctrl_softc *sc,
    const struct meson_pinctrl_gpioreg *gpioreg)
{
	switch (gpioreg->type) {
	case MESON_PINCTRL_REGTYPE_PULL:
		return sc->sc_bsh_pull;
	case MESON_PINCTRL_REGTYPE_PULL_ENABLE:
		return sc->sc_bsh_pull_enable;
	case MESON_PINCTRL_REGTYPE_GPIO:
		return sc->sc_bsh_gpio;
	default:
		panic("unsupported GPIO regtype %d", gpioreg->type);
	}
}

static int
meson_pinctrl_pin_read(void *priv, int pin)
{
	struct meson_pinctrl_softc * const sc = priv;
	const struct meson_pinctrl_gpio *pin_def = &sc->sc_conf->gpios[pin];
	const struct meson_pinctrl_gpioreg *gpio_reg = &pin_def->in;
	bus_space_handle_t bsh;
	uint32_t data;
	int val;

	KASSERT(pin < sc->sc_conf->ngpios);

	bsh = meson_pinctrl_gpio_handle(sc, gpio_reg);
	data = bus_space_read_4(sc->sc_bst, bsh, gpio_reg->reg);
	val = __SHIFTOUT(data, gpio_reg->mask);

	return val;
}

static void
meson_pinctrl_pin_write(void *priv, int pin, int val)
{
	struct meson_pinctrl_softc * const sc = priv;
	const struct meson_pinctrl_gpio *pin_def = &sc->sc_conf->gpios[pin];
	const struct meson_pinctrl_gpioreg *gpio_reg = &pin_def->out;
	bus_space_handle_t bsh;
	uint32_t data;

	KASSERT(pin < sc->sc_conf->ngpios);

	bsh = meson_pinctrl_gpio_handle(sc, gpio_reg);

	mutex_enter(&sc->sc_lock);
	data = bus_space_read_4(sc->sc_bst, bsh, gpio_reg->reg);
	if (val)
		data |= gpio_reg->mask;
	else
		data &= ~gpio_reg->mask;
	bus_space_write_4(sc->sc_bst, bsh, gpio_reg->reg, data);
	mutex_exit(&sc->sc_lock);
}

static void
meson_pinctrl_pin_dir(struct meson_pinctrl_softc *sc,
    const struct meson_pinctrl_gpio *pin_def, int flags)
{
	bus_space_handle_t bsh;
	uint32_t data;

	KASSERT(mutex_owned(&sc->sc_lock));

	bsh = meson_pinctrl_gpio_handle(sc, &pin_def->oen);
	data = bus_space_read_4(sc->sc_bst, bsh, pin_def->oen.reg);
	if ((flags & GPIO_PIN_INPUT) != 0)
		data |= pin_def->oen.mask;
	else
		data &= ~pin_def->oen.mask;
	bus_space_write_4(sc->sc_bst, bsh, pin_def->oen.reg, data);
}

static void
meson_pinctrl_pin_ctl(void *priv, int pin, int flags)
{
	struct meson_pinctrl_softc * const sc = priv;
	const struct meson_pinctrl_gpio *pin_def = &sc->sc_conf->gpios[pin];
	bus_space_handle_t bsh;
	uint32_t data;

	KASSERT(pin < sc->sc_conf->ngpios);

	mutex_enter(&sc->sc_lock);

	if ((flags & (GPIO_PIN_INPUT|GPIO_PIN_OUTPUT)) != 0)
		meson_pinctrl_pin_dir(sc, pin_def, flags);

	if ((flags & (GPIO_PIN_PULLUP|GPIO_PIN_PULLDOWN)) != 0) {
		bsh = meson_pinctrl_gpio_handle(sc, &pin_def->pupd);
		data = bus_space_read_4(sc->sc_bst, bsh, pin_def->pupd.reg);
		if ((flags & GPIO_PIN_PULLUP) != 0)
			data |= pin_def->pupd.mask;
		else
			data &= ~pin_def->pupd.mask;
		bus_space_write_4(sc->sc_bst, bsh, pin_def->pupd.reg, data);

		bsh = meson_pinctrl_gpio_handle(sc, &pin_def->pupden);
		data = bus_space_read_4(sc->sc_bst, bsh, pin_def->pupden.reg);
		data |= pin_def->pupden.mask;
		bus_space_write_4(sc->sc_bst, bsh, pin_def->pupden.reg, data);
	} else {
		bsh = meson_pinctrl_gpio_handle(sc, &pin_def->pupden);
		data = bus_space_read_4(sc->sc_bst, bsh, pin_def->pupden.reg);
		data &= ~pin_def->pupden.mask;
		bus_space_write_4(sc->sc_bst, bsh, pin_def->pupden.reg, data);
	}

	mutex_exit(&sc->sc_lock);
}

static const struct meson_pinctrl_gpio *
meson_pinctrl_gpio_lookup(struct meson_pinctrl_softc *sc, u_int id)
{
	if (id >= sc->sc_conf->ngpios)
		return NULL;

	if (sc->sc_conf->gpios[id].name == NULL)
		return NULL;

	return &sc->sc_conf->gpios[id];
}

static void *
meson_pinctrl_gpio_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct meson_pinctrl_softc * const sc = device_private(dev);
	const struct meson_pinctrl_gpio *pin_def;
	const struct meson_pinctrl_group *group;
	struct meson_pinctrl_gpio_pin *gpin;
	const u_int *gpio = data;
	u_int n, bank;

	if (len != 12)
		return NULL;

	const u_int id = be32toh(gpio[1]);
	const bool actlo = be32toh(gpio[2]) & 1;

	pin_def = meson_pinctrl_gpio_lookup(sc, id);
	if (pin_def == NULL)
		return NULL;

	/* Disable conflicting groups */
	for (n = 0; n < sc->sc_conf->ngroups; n++) {
		group = &sc->sc_conf->groups[n];
		for (bank = 0; bank < group->nbank; bank++) {
			if (group->bank[bank] == pin_def->id)
				meson_pinctrl_set_group(sc, group, false);
		}
	}

	mutex_enter(&sc->sc_lock);
	meson_pinctrl_pin_dir(sc, pin_def, flags);
	mutex_exit(&sc->sc_lock);

	gpin = kmem_zalloc(sizeof(*gpin), KM_SLEEP);
	gpin->pin_sc = sc;
	gpin->pin_def = pin_def;
	gpin->pin_flags = flags;
	gpin->pin_actlo = actlo;

	return gpin;
}

static void
meson_pinctrl_gpio_release(device_t dev, void *priv)
{
	struct meson_pinctrl_softc * const sc = device_private(dev);
	struct meson_pinctrl_gpio_pin *gpin = priv;
	const struct meson_pinctrl_gpio *pin_def = gpin->pin_def;

	KASSERT(sc == gpin->pin_sc);

	mutex_enter(&sc->sc_lock);
	meson_pinctrl_pin_dir(sc, pin_def, GPIO_PIN_INPUT);
	mutex_exit(&sc->sc_lock);

	kmem_free(gpin, sizeof(*gpin));
}

static int
meson_pinctrl_gpio_read(device_t dev, void *priv, bool raw)
{
	struct meson_pinctrl_softc * const sc = device_private(dev);
	struct meson_pinctrl_gpio_pin *gpin = priv;
	const struct meson_pinctrl_gpio *pin_def = gpin->pin_def;
	int val;

	val = meson_pinctrl_pin_read(sc, pin_def->id);
	if (!raw && gpin->pin_actlo)
		val = !val;

	return val;
}

static void
meson_pinctrl_gpio_write(device_t dev, void *priv, int val, bool raw)
{
	struct meson_pinctrl_softc * const sc = device_private(dev);
	struct meson_pinctrl_gpio_pin *gpin = priv;
	const struct meson_pinctrl_gpio *pin_def = gpin->pin_def;

	if (!raw && gpin->pin_actlo)
		val = !val;

	meson_pinctrl_pin_write(sc, pin_def->id, val);
}

static struct fdtbus_gpio_controller_func meson_pinctrl_gpio_funcs = {
	.acquire = meson_pinctrl_gpio_acquire,
	.release = meson_pinctrl_gpio_release,
	.read = meson_pinctrl_gpio_read,
	.write = meson_pinctrl_gpio_write,
};

static int
meson_pinctrl_initres(struct meson_pinctrl_softc *sc)
{
	bool gpio_found = false;
	bus_addr_t addr;
	bus_size_t size;
	int child;

	for (child = OF_child(sc->sc_phandle); child; child = OF_peer(child)) {
		if (of_hasprop(child, "gpio-controller")) {
			if (gpio_found)
				continue;
			gpio_found = true;

			if (fdtbus_get_reg_byname(child, "mux", &addr, &size) != 0 ||
			    bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh_mux) != 0) {
				aprint_error(": couldn't map mux registers\n");
				return ENXIO;
			}
			if (fdtbus_get_reg_byname(child, "pull", &addr, &size) != 0 ||
			    bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh_pull) != 0) {
				aprint_error(": couldn't map pull registers\n");
				return ENXIO;
			}
			if (fdtbus_get_reg_byname(child, "gpio", &addr, &size) != 0 ||
			    bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh_gpio) != 0) {
				aprint_error(": couldn't map gpio registers\n");
				return ENXIO;
			}

			/* pull-enable register is optional */
			if (fdtbus_get_reg_byname(child, "pull-enable", &addr, &size) == 0) {
				if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh_pull_enable) != 0) {
					aprint_error(": couldn't map pull-enable registers\n");
					return ENXIO;
				}
			}

			sc->sc_phandle_gpio = child;
		} else if (of_find_firstchild_byname(child, "mux") != -1) {
			fdtbus_register_pinctrl_config(sc->sc_dev, child, &meson_pinctrl_funcs);
		}
	}

	if (!gpio_found) {
		aprint_error(": couldn't find gpio controller\n");
		return ENOENT;
	}

	return 0;
}

static void
meson_pinctrl_initgpio(struct meson_pinctrl_softc *sc)
{
	const struct meson_pinctrl_gpio *pin_def;
	struct gpio_chipset_tag *gp;
	struct gpiobus_attach_args gba;
	int child, len, val;
	u_int pin;

	fdtbus_register_gpio_controller(sc->sc_dev, sc->sc_phandle_gpio, &meson_pinctrl_gpio_funcs);

	for (child = OF_child(sc->sc_phandle_gpio); child; child = OF_peer(child)) {
		if (!of_hasprop(child, "gpio-hog"))
			continue;

		const char *line_name = fdtbus_get_string(child, "line-name");
		if (line_name == NULL)
			line_name = fdtbus_get_string(child, "name");

		const bool input = of_hasprop(child, "input");
		const bool output_low = of_hasprop(child, "output-low");
		const bool output_high = of_hasprop(child, "output-high");

		if (!input && !output_low && !output_high) {
			aprint_error_dev(sc->sc_dev, "no configuration for line %s\n", line_name);
			continue;
		}

		const u_int *gpio = fdtbus_get_prop(child, "gpios", &len);
		while (len >= 8) {
			const u_int id = be32toh(gpio[0]);
			const bool actlo = be32toh(gpio[1]) & 1;

			pin_def = meson_pinctrl_gpio_lookup(sc, id);
			if (pin_def != NULL) {
				if (input) {
					device_printf(sc->sc_dev, "%s %s set to input\n",
					    line_name, pin_def->name);
					meson_pinctrl_pin_ctl(sc, pin_def->id, GPIO_PIN_INPUT);
				} else {
					val = output_high;
					if (actlo)
						val = !val;
					device_printf(sc->sc_dev, "%s %s set to output (%s)\n",
					    line_name, pin_def->name, val ? "high" : "low");
					meson_pinctrl_pin_write(sc, pin_def->id, val);
					meson_pinctrl_pin_ctl(sc, pin_def->id, GPIO_PIN_OUTPUT);
				}
			} else {
				aprint_error_dev(sc->sc_dev, "%s: unsupported pin %d\n", line_name, id);
			}

			len -= 8;
			gpio += 8;
		}
	}

	const u_int npins = sc->sc_conf->ngpios;
	sc->sc_pins = kmem_zalloc(sizeof(*sc->sc_pins) * npins, KM_SLEEP);
	for (pin = 0; pin < npins; pin++) {
		pin_def = &sc->sc_conf->gpios[pin];
		sc->sc_pins[pin].pin_num = pin;
		if (pin_def->name == NULL)
			continue;
		sc->sc_pins[pin].pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
		    GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN;
		sc->sc_pins[pin].pin_state = meson_pinctrl_pin_read(sc, pin);
		strlcpy(sc->sc_pins[pin].pin_defname, pin_def->name,
		    sizeof(sc->sc_pins[pin].pin_defname));
	}

	gp = &sc->sc_gp;
	gp->gp_cookie = sc;
	gp->gp_pin_read = meson_pinctrl_pin_read;
	gp->gp_pin_write = meson_pinctrl_pin_write;
	gp->gp_pin_ctl = meson_pinctrl_pin_ctl;

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = gp;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = npins;
	config_found_ia(sc->sc_dev, "gpiobus", &gba, NULL);
}

static int
meson_pinctrl_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
meson_pinctrl_attach(device_t parent, device_t self, void *aux)
{
	struct meson_pinctrl_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_bst = faa->faa_bst;
	sc->sc_conf = (void *)of_search_compatible(sc->sc_phandle, compat_data)->data;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	if (meson_pinctrl_initres(sc) != 0)
		return;

	aprint_naive("\n");
	aprint_normal(": %s\n", sc->sc_conf->name);

	fdtbus_pinctrl_configure();

	meson_pinctrl_initgpio(sc);
}

CFATTACH_DECL_NEW(meson_pinctrl, sizeof(struct meson_pinctrl_softc),
	meson_pinctrl_match, meson_pinctrl_attach, NULL, NULL);
