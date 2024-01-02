/*	$NetBSD: gftty_mainbus.c,v 1.1 2024/01/02 07:40:59 thorpej Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: gftty_mainbus.c,v 1.1 2024/01/02 07:40:59 thorpej Exp $");

#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <virt68k/dev/mainbusvar.h>

#include <dev/goldfish/gfttyvar.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "google,goldfish-tty" },
	DEVICE_COMPAT_EOL
};


static int
gftty_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const ma = aux;

	return mainbus_compatible_match(ma, compat_data);
}

static void
gftty_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct gftty_softc * const sc = device_private(self);
	struct mainbus_attach_args * const ma = aux;

	sc->sc_dev = self;

	if (! gftty_is_console(sc)) {
		bus_space_handle_t bsh;

		if (bus_space_map(ma->ma_st, ma->ma_addr, ma->ma_size, 0,
				  &bsh) != 0) {
			aprint_error(": couldn't map registers\n");
			return;
		}
		gftty_alloc_config(sc, ma->ma_st, bsh);
	}

	gftty_attach(sc);
}

CFATTACH_DECL_NEW(gftty_mainbus, sizeof(struct gftty_softc),
	gftty_mainbus_match, gftty_mainbus_attach, NULL, NULL);
