/*	$NetBSD: usbnode.c,v 1.3 2026/01/17 05:45:17 skrll Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: usbnode.c,v 1.3 2026/01/17 05:45:17 skrll Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

struct usbnode_softc {
	device_t			sc_dev;
	int				sc_phandle;

	struct fdtbus_gpio_pin *	sc_pin_reset;
	u_int				sc_resetdelay;
};

struct usbnode_data {
	const char *	und_desc;
	u_int		und_resetdelay;
};

/* Genesys Logic GL852G usb hub */
static struct usbnode_data genesys_gl852g_data = {
	.und_desc = "Genesys Logic GL852G usb hub",
	.und_resetdelay = 50,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "usb5e3,610", .data = &genesys_gl852g_data },
	DEVICE_COMPAT_EOL
};

static void
usbnode_enable(struct usbnode_softc *sc,bool enable)
{

	if (enable) {
		fdtbus_gpio_write(sc->sc_pin_reset, 1);
		delay(sc->sc_resetdelay);
	}

	fdtbus_gpio_write(sc->sc_pin_reset, enable ? 0 : 1);
}

static int
usbnode_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static int
usbnode_detach(device_t self, int flags)
{
	struct usbnode_softc * const sc = device_private(self);

	usbnode_enable(sc, false);

	return 0;
}

static void
usbnode_attach(device_t parent, device_t self, void *aux)
{
	struct usbnode_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;

	const struct device_compatible_entry * const dce =
	    of_compatible_lookup(phandle, compat_data);

	const struct usbnode_data *und = dce->data;

	/* reset gpio is ... */
	sc->sc_pin_reset =
	    fdtbus_gpio_acquire(phandle, "reset-gpio", GPIO_PIN_OUTPUT);
	if (sc->sc_pin_reset == NULL) {
		aprint_error(": couldn't acquire reset gpio\n");
	}

	usbnode_enable(sc, true);

	aprint_naive("\n");
	aprint_normal(": USB node '%s'\n", und->und_desc);
}

CFATTACH_DECL_NEW(usbnode, sizeof(struct usbnode_softc),
	usbnode_match, usbnode_attach, usbnode_detach, NULL);
