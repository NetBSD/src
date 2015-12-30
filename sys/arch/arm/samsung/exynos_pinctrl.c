/*	$NetBSD: exynos_pinctrl.c,v 1.8 2015/12/30 04:30:27 marty Exp $ */

/*-
* Copyright (c) 2015 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Marty Fouts
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
__KERNEL_RCSID(1, "$NetBSD: exynos_pinctrl.c,v 1.8 2015/12/30 04:30:27 marty Exp $");

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

static int exynos_pinctrl_match(device_t, cfdata_t, void *);
static void exynos_pinctrl_attach(device_t, device_t, void *);

static void *exynos_pinctrl_acquire(device_t, const char *);
static void  exynos_pinctrl_release(device_t, void *);
static void  exynos_pinctrl_get_cfg(struct fdtbus_pinctrl_pin *, void *);
static void  exynos_pinctrl_set_cfg(struct fdtbus_pinctrl_pin *, void *);

static struct fdtbus_pinctrl_controller_func exynos_pinctrl_controller_func = {
	.acquire = exynos_pinctrl_acquire,
	.release = exynos_pinctrl_release,
	.get     = exynos_pinctrl_get_cfg,
	.set     = exynos_pinctrl_set_cfg,
};

CFATTACH_DECL_NEW(exynos_pinctrl, sizeof(struct exynos_pinctrl_softc),
	exynos_pinctrl_match, exynos_pinctrl_attach, NULL, NULL);

static int
exynos_pinctrl_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "samsung,exynos5420-pinctrl",
					    NULL };
	struct fdt_attach_args * const faa = aux;
	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
exynos_pinctrl_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_pinctrl_softc * const sc
		= kmem_zalloc(sizeof(*sc), KM_SLEEP);
	struct fdt_attach_args * const faa = aux;
	struct exynos_gpio_softc *child_sc;
	bus_addr_t addr;
	bus_size_t size;
	int error;
	int child;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	aprint_normal(" pinctl @ 0x%08x ", (uint)addr);
	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d",
			     (uint64_t)addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal("\n");

	for (child = OF_child(faa->faa_phandle); child;
	     child = OF_peer(child)) {
		if (of_getprop_bool(child, "gpio-controller") == false)
			continue;
		child_sc = exynos_gpio_bank_config(sc, faa, child);
		fdtbus_register_pinctrl_controller(child_sc->sc_dev, child,
					    &exynos_pinctrl_controller_func);
	}
}


static void *exynos_pinctrl_acquire(device_t self, const char *name)
{
	return exynos_gpio_bank_lookup(name);
}

static void exynos_pinctrl_release(device_t self, void *cookie)
{
}

static void exynos_pinctrl_get_cfg(struct fdtbus_pinctrl_pin *pin,
				   void *cookie)
{
	struct exynos_gpio_bank *bank = pin->pp_priv;
	struct exynos_gpio_pin_cfg *cfg  = cookie;
	struct exynos_gpio_pin_cfg **cfgp = &cfg;
	struct exynos_gpio_pin_cfg *newcfg = kmem_zalloc(sizeof(*newcfg),
							 KM_SLEEP);
	if (newcfg == NULL)
		return;
	exynos_gpio_pin_ctl_read(bank, newcfg);
	*cfgp = newcfg;
	return;
}

static void exynos_pinctrl_set_cfg(struct fdtbus_pinctrl_pin *pin,
				   void *cookie)
{
	struct exynos_gpio_bank *bank = pin->pp_priv;
	struct exynos_gpio_pin_cfg *cfg = cookie;
	exynos_gpio_pin_ctl_write(bank, cfg);
}
