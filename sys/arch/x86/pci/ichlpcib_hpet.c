/* $NetBSD: ichlpcib_hpet.c,v 1.2.2.2 2011/06/23 14:19:48 cherry Exp $ */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto and Matthew R. Green.
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
__KERNEL_RCSID(0, "$NetBSD: ichlpcib_hpet.c,v 1.2.2.2 2011/06/23 14:19:48 cherry Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/time.h>
#include <sys/timetc.h>

#include <machine/bus.h>

#include <dev/ic/hpetreg.h>
#include <dev/ic/hpetvar.h>
#include <dev/ic/i82801lpcreg.h>
#include <dev/ic/i82801lpcvar.h>

static int	lpcib_hpet_match(device_t , cfdata_t , void *);
static void	lpcib_hpet_attach(device_t, device_t, void *);
static int	lpcib_hpet_detach(device_t, int);

CFATTACH_DECL_NEW(ichlpcib_hpet, sizeof(struct hpet_softc),
    lpcib_hpet_match, lpcib_hpet_attach, lpcib_hpet_detach, NULL);

static int
lpcib_hpet_match(device_t parent, cfdata_t match, void *aux)
{
	struct lpcib_hpet_attach_args *arg = aux;
	bus_space_handle_t bh;
	bus_space_tag_t bt;

	bt = arg->hpet_mem_t;

	if (bus_space_map(bt, arg->hpet_reg, HPET_WINDOW_SIZE, 0, &bh) != 0)
		return 0;

	bus_space_unmap(bt, bh, HPET_WINDOW_SIZE);

	return 1;
}


static void
lpcib_hpet_attach(device_t parent, device_t self, void *aux)
{
	struct hpet_softc *sc = device_private(self);
	struct lpcib_hpet_attach_args *arg = aux;

	sc->sc_mapped = false;
	sc->sc_memt = arg->hpet_mem_t;
	sc->sc_mems = HPET_WINDOW_SIZE;

	if (bus_space_map(sc->sc_memt, arg->hpet_reg,
		sc->sc_mems, 0, &sc->sc_memh) != 0) {
		aprint_error(": failed to map mem space\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": high precision event timer (mem 0x%08x-0x%08x)\n",
	    arg->hpet_reg, arg->hpet_reg + HPET_WINDOW_SIZE);

	sc->sc_mapped = true;
	hpet_attach_subr(self);
}

static int
lpcib_hpet_detach(device_t self, int flags)
{
	struct hpet_softc *sc = device_private(self);
	int rv;

	if (sc->sc_mapped != true)
		return 0;

	rv = hpet_detach(self, flags);

	if (rv != 0)
		return rv;

	bus_space_unmap(sc->sc_memt, sc->sc_memh, sc->sc_mems);

	return 0;
}
