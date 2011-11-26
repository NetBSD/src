/*	$NetBSD: joy_pnpbios.c,v 1.15 2011/11/26 14:06:44 tron Exp $	*/

/*-
 * Copyright (c) 2001, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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
__KERNEL_RCSID(0, "$NetBSD: joy_pnpbios.c,v 1.15 2011/11/26 14:06:44 tron Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <i386/pnpbios/pnpbiosvar.h>

#include <dev/ic/joyvar.h>

struct joy_pnpbios_softc {
	struct joy_softc sc_joy;
	kmutex_t sc_lock;
};

static int	joy_pnpbios_match(device_t, cfdata_t, void *);
static void	joy_pnpbios_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(joy_pnpbios, sizeof(struct joy_pnpbios_softc),
    joy_pnpbios_match, joy_pnpbios_attach, NULL, NULL);

static int
joy_pnpbios_match(device_t parent, cfdata_t match, void *aux)
{
	struct pnpbiosdev_attach_args *aa = aux;

	if (strcmp(aa->idstr, "PNPB02F"))
		return 0;

	return 1;
}

static void
joy_pnpbios_attach(device_t parent, device_t self, void *aux)
{
	struct joy_softc *sc = device_private(self);
	struct joy_pnpbios_softc *psc = device_private(self);
	struct pnpbiosdev_attach_args *aa = aux;

	if (pnpbios_io_map(aa->pbt, aa->resc, 0, &sc->sc_iot, &sc->sc_ioh)) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	aprint_normal("\n");
	sc->sc_dev = self;
	pnpbios_print_devres(self, aa);

	mutex_init(&psc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_lock = &psc->sc_lock;

	joyattach(sc);
}
