/* $NetBSD: ti_omapwugen.c,v 1.1 2025/12/16 12:20:22 skrll Exp $ */

/*-
 * Copyright (c) 2025 Rui-Xiang Guo
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
__KERNEL_RCSID(0, "$NetBSD: ti_omapwugen.c,v 1.1 2025/12/16 12:20:22 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/fdt/fdtvar.h>

static int omapwugen_match(device_t, cfdata_t, void *);
static void omapwugen_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,omap4-wugen-mpu" },
	DEVICE_COMPAT_EOL
};

struct omapwugen_softc {
	device_t sc_dev;
	int sc_phandle;
};

static void *
omapwugen_fdt_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct omapwugen_softc * const sc = device_private(dev);

	const int ihandle = fdtbus_intr_parent(sc->sc_phandle);
	if (ihandle == -1) {
		return NULL;
	}

	return fdtbus_intr_establish_raw(ihandle, specifier, ipl,
	    flags, func, arg, xname);
}

static void
omapwugen_fdt_disestablish(device_t dev, void *ih)
{
	intr_disestablish(ih);
}

static bool
omapwugen_fdt_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	struct omapwugen_softc * const sc = device_private(dev);

	const int ihandle = fdtbus_intr_parent(sc->sc_phandle);
	if (ihandle == -1) {
		return false;
	}

	return fdtbus_intr_str_raw(ihandle, specifier, buf, buflen);
}

static const struct fdtbus_interrupt_controller_func omapwugen_fdt_funcs = {
	.establish = omapwugen_fdt_establish,
	.disestablish = omapwugen_fdt_disestablish,
	.intrstr = omapwugen_fdt_intrstr,
};

static int
omapwugen_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
omapwugen_attach(device_t parent, device_t self, void *aux)
{
	struct omapwugen_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	int error;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;

	aprint_naive("\n");
	aprint_normal("\n");

	error = fdtbus_register_interrupt_controller(self, phandle,
	    &omapwugen_fdt_funcs);
	if (error) {
		aprint_error_dev(self, "couldn't register with fdtbus: %d\n",
		    error);
		return;
	}
}

CFATTACH_DECL_NEW(omapwugen, sizeof(struct omapwugen_softc),
	omapwugen_match, omapwugen_attach, NULL, NULL);
