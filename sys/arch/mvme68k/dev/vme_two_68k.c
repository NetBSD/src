/*	$NetBSD: vme_two_68k.c,v 1.9.34.1 2012/10/30 17:20:03 yamt Exp $	*/

/*-
 * Copyright (c) 1999, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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

/*
 * Front-end for the VMEchip2 found on the MVME-1[67][27] boards.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vme_two_68k.c,v 1.9.34.1 2012/10/30 17:20:03 yamt Exp $");

#include "vmetwo.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/vme/vmereg.h>
#include <dev/vme/vmevar.h>

#include <mvme68k/mvme68k/isr.h>
#include <mvme68k/dev/mainbus.h>

#include <dev/mvme/mvmebus.h>
#include <dev/mvme/vme_tworeg.h>
#include <dev/mvme/vme_twovar.h>

#include "ioconf.h"

static void vmetwoisrlink(void *, int (*)(void *), void *,
	int, int, struct evcnt *);
static void vmetwoisrunlink(void *, int);
static struct evcnt *vmetwoisrevcnt(void *, int);

#if NVMETWO > 0

int vmetwo_match(device_t, cfdata_t, void *);
void vmetwo_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(vmetwo, sizeof(struct vmetwo_softc),
    vmetwo_match, vmetwo_attach, NULL, NULL);


/* ARGSUSED */
int
vmetwo_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma;
	static int matched = 0;

	ma = aux;

	if (strcmp(ma->ma_name, vmetwo_cd.cd_name))
		return 0;

	/* Only one VMEchip2, please. */
	if (matched++)
		return 0;

	/*
	 * Some mvme1[67]2 boards have a `no VMEchip2' build option...
	 */
	return vmetwo_probe(ma->ma_bust, ma->ma_offset) ? 1 : 0;
}

/* ARGSUSED */
void
vmetwo_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *ma;
	struct vmetwo_softc *sc;

	sc = device_private(self);
	sc->sc_mvmebus.sc_dev = self;
	ma = aux;

	/*
	 * Map the local control registers
	 */
	bus_space_map(ma->ma_bust, ma->ma_offset + VME2REG_LCSR_OFFSET,
	    VME2LCSR_SIZE, 0, &sc->sc_lcrh);

#ifdef notyet
	/*
	 * Map the global control registers
	 */
	bus_space_map(ma->ma_bust, ma->ma_offset + VME2REG_GCSR_OFFSET,
	    VME2GCSR_SIZE, 0, &sc->sc_gcrh);
#endif

	/* Initialise stuff for the common vme_two back-end */
	sc->sc_mvmebus.sc_bust = ma->ma_bust;
	sc->sc_mvmebus.sc_dmat = ma->ma_dmat;

	vmetwo_init(sc);
}

#endif	/* NVMETWO > 0 */

void
vmetwo_md_intr_init(struct vmetwo_softc *sc)
{

	sc->sc_isrlink = vmetwoisrlink;
	sc->sc_isrunlink = vmetwoisrunlink;
	sc->sc_isrevcnt = vmetwoisrevcnt;
}

/* ARGSUSED */
static void
vmetwoisrlink(void *cookie, int (*fn)(void *), void *arg, int ipl, int vec,
    struct evcnt *evcnt)
{

	isrlink_vectored(fn, arg, ipl, vec, evcnt);
}

/* ARGSUSED */
static void
vmetwoisrunlink(void *cookie, int vec)
{

	isrunlink_vectored(vec);
}

/* ARGSUSED */
static struct evcnt *
vmetwoisrevcnt(void *cookie, int ipl)
{

	return isrlink_evcnt(ipl);
}
