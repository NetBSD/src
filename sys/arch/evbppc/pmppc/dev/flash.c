/*	$NetBSD: flash.c,v 1.3.32.1 2011/06/23 14:19:10 cherry Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at Sandburst Corp.
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
__KERNEL_RCSID(0, "$NetBSD: flash.c,v 1.3.32.1 2011/06/23 14:19:10 cherry Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <arch/evbppc/pmppc/dev/mainbus.h>

struct flash_softc {
	device_t sc_dev;
	bus_space_tag_t sc_tag;
	bus_space_handle_t sc_handle;
	u_int32_t sc_addr;
	u_int32_t sc_size;
	u_int32_t sc_width;
};

static int	flash_match(device_t, cfdata_t, void *);
static void	flash_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(flash, sizeof(struct flash_softc),
    flash_match, flash_attach, NULL, NULL);

int
flash_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	return (strcmp(maa->mb_name, "flash") == 0);
}

void
flash_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *maa = aux;
	struct flash_softc *sc = device_private(self);

	sc->sc_tag = maa->mb_bt;
	sc->sc_addr = maa->mb_addr;
	sc->sc_size = maa->u.mb_flash.size / 8; /* bytes */
	sc->sc_width = maa->u.mb_flash.width;

	aprint_normal(": %d Mbyte, %d bits wide\n", sc->sc_size / (1024*1024),
	       sc->sc_width);
#if 0
	/* The extend map doesn't cover this area. */
	if (bus_space_map(sc->sc_tag, sc->sc_addr, sc->sc_size, 0,
			  &sc->sc_handle)) {
		aprint_error_dev(self, "can't map i/o space\n");
		return;
	}
#endif

	aprint_error_dev(self, "driver not implemented\n");
}
