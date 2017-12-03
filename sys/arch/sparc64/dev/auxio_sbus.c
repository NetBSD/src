/*	$NetBSD: auxio_sbus.c,v 1.1.18.2 2017/12/03 11:36:44 jdolecek Exp $	*/

/*
 * Copyright (c) 2000, 2001, 2015 Matthew R. Green
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

/*
 * AUXIO registers support on the sbus & ebus2, used for the floppy driver
 * and to control the system LED, for the BLINK option.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: auxio_sbus.c,v 1.1.18.2 2017/12/03 11:36:44 jdolecek Exp $");

#include "opt_auxio.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/callout.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>
#include <dev/sbus/sbusvar.h>
#include <sparc64/dev/auxioreg.h>
#include <sparc64/dev/auxiovar.h>

static int	auxio_sbus_match(device_t, cfdata_t, void *);
static void	auxio_sbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(auxio_sbus, sizeof(struct auxio_softc),
    auxio_sbus_match, auxio_sbus_attach, NULL, NULL);

static int
auxio_sbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct sbus_attach_args *sa = aux;

	return strcmp(AUXIO_ROM_NAME, sa->sa_name) == 0;
}

static void
auxio_sbus_attach(device_t parent, device_t self, void *aux)
{
	struct auxio_softc *sc = device_private(self);
	struct sbus_attach_args *sa = aux;

	sc->sc_dev = self;
	sc->sc_tag = sa->sa_bustag;

	if (sa->sa_nreg < 1) {
		printf(": no registers??\n");
		return;
	}

	if (sa->sa_nreg != 1) {
		printf(": not 1 (%d/%d) registers??", sa->sa_nreg,
		    sa->sa_npromvaddrs);
		return;
	}

	/* sbus auxio only has one set of registers */
	sc->sc_flags = AUXIO_LEDONLY;
	if (sa->sa_npromvaddrs > 0) {
		sbus_promaddr_to_handle(sc->sc_tag,
			sa->sa_promvaddr, &sc->sc_led);
	} else {
		sbus_bus_map(sc->sc_tag, sa->sa_slot, sa->sa_offset,
			sa->sa_size, 0, &sc->sc_led);
	}

	auxio_attach_common(sc);
}
