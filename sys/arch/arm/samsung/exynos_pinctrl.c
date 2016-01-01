/*	$NetBSD: exynos_pinctrl.c,v 1.9 2016/01/01 22:37:07 marty Exp $ */

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
__KERNEL_RCSID(1, "$NetBSD: exynos_pinctrl.c,v 1.9 2016/01/01 22:37:07 marty Exp $");

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

struct exynos_pinctrl_config {
	int pc_phandle;
	struct exynos_gpio_pin_cfg *pc_pincfg;
	struct exynos_pinctrl_softc *pc_sc;
};

static int exynos_pinctrl_match(device_t, cfdata_t, void *);
static void exynos_pinctrl_attach(device_t, device_t, void *);

static int  exynos_pinctrl_set_cfg(void *);
static struct exynos_gpio_pin_cfg *
exynos_parse_config(struct exynos_pinctrl_config *pc);

static struct fdtbus_pinctrl_controller_func exynos_pinctrl_controller_func = {
	.set_config = exynos_pinctrl_set_cfg
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

static bool
is_pinctrl(int phandle)
{
	int len = OF_getproplen(phandle, "samsung,pins");
	return len > 0;
}

static void
exynos_pinctrl_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_pinctrl_softc * const sc
		= kmem_zalloc(sizeof(*sc), KM_SLEEP);
	struct fdt_attach_args * const faa = aux;
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

		if (of_getprop_bool(child, "gpio-controller")) {
			exynos_gpio_bank_config(sc, faa, child);
		}

		if (is_pinctrl(child)) {
			struct exynos_pinctrl_config *pc;
			pc = kmem_alloc(sizeof(*pc), KM_SLEEP);
			pc->pc_phandle = child;
			pc->pc_sc = sc;
			pc->pc_pincfg = exynos_parse_config(pc);
			fdtbus_register_pinctrl_config(pc, child,
						       &exynos_pinctrl_controller_func);
		}
	}
}

static struct exynos_gpio_pin_cfg *
exynos_parse_config(struct exynos_pinctrl_config *pc)
{
	struct exynos_gpio_pin_cfg *gc = kmem_zalloc(sizeof(*gc), KM_SLEEP);
	int len;
	int value;

	len = OF_getprop(pc->pc_phandle, "samsung,pin-function",
			 &value, sizeof(value));
	if (len > 0) {
		gc->cfg = be32toh(value);
	}

	len = OF_getprop(pc->pc_phandle, "samsung,pin-pud", &value,
			 sizeof(&value));
	if (len > 0) {
		gc->pud = be32toh(value);
	}

	len = OF_getprop(pc->pc_phandle, "samsung,pin-drv", &value,
			 sizeof(&value));
	if (len > 0) {
		gc->drv = be32toh(value);
	}

	len = OF_getprop(pc->pc_phandle, "samsung,pin-conpwd", &value,
			 sizeof(&value));
	if (len > 0) {
		gc->conpwd = be32toh(value);
	}

	len = OF_getprop(pc->pc_phandle, "samsung,pin-pudpwd", &value,
			 sizeof(&value));
	if (len > 0) {
		gc->pudpwd = be32toh(value);
	}
	return gc;
}

static int
exynos_do_config(struct exynos_pinctrl_config *pc)
{
	struct exynos_gpio_pin_cfg *gc = pc->pc_pincfg;
	struct exynos_gpio_bank *bank;
	int len;
	char result[20];

	if (gc == NULL) {
		printf("%s: No configuration available\n", __func__);
		return -1;
	}

	len = OF_getprop(pc->pc_phandle, "samsung,pins", result,
			 sizeof(result));
	if (len <= 0) {
		printf("%s: couldn't get pins. (%d)\n", __func__,
			pc->pc_phandle);
		return -1;
	}

	bank = exynos_gpio_bank_lookup(&result[0]);
	if (!bank) {
		printf("%s: Couldn't get bank \"%s\".\n", __func__, result);
		return -1;
	}

	exynos_gpio_pin_ctl_write(bank, gc);
	return 0;
}
	
static int
exynos_pinctrl_set_cfg(void *cookie)
{
	struct exynos_pinctrl_config *pc = cookie;
	return exynos_do_config(pc);
}
