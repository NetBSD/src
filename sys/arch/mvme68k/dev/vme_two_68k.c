/*	$NetBSD: vme_two_68k.c,v 1.1.8.3 2002/06/23 17:38:18 jdolecek Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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


static void vmetwoisrlink(void *, int (*)(void *), void *,
	int, int, struct evcnt *);
static void vmetwoisrunlink(void *, int);
static struct evcnt *vmetwoisrevcnt(void *, int);

#if NVMETWO > 0

int vmetwo_match __P((struct device *, struct cfdata *, void *));
void vmetwo_attach __P((struct device *, struct device *, void *));

struct cfattach vmetwo_ca = {
	sizeof(struct vmetwo_softc), vmetwo_match, vmetwo_attach
};
extern struct cfdriver vmetwo_cd;


/* ARGSUSED */
int
vmetwo_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma;
	static int matched = 0;

	ma = aux;

	if (strcmp(ma->ma_name, vmetwo_cd.cd_name))
		return (0);

	/* Only one VMEchip2, please. */
	if (matched++)
		return (0);

	/*
	 * Some mvme1[67]2 boards have a `no VMEchip2' build option...
	 */
	return (vmetwo_probe(ma->ma_bust, ma->ma_offset) ? 1 : 0);
}

/* ARGSUSED */
void
vmetwo_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mainbus_attach_args *ma;
	struct vmetwo_softc *sc;

	sc = (struct vmetwo_softc *) self;
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
vmetwo_md_intr_init(sc)
	struct vmetwo_softc *sc;
{

	sc->sc_isrlink = vmetwoisrlink;
	sc->sc_isrunlink = vmetwoisrunlink;
	sc->sc_isrevcnt = vmetwoisrevcnt;
}

/* ARGSUSED */
static void
vmetwoisrlink(cookie, fn, arg, ipl, vec, evcnt)
	void *cookie;
	int (*fn)(void *);
	void *arg;
	int ipl, vec;
	struct evcnt *evcnt;
{

	isrlink_vectored(fn, arg, ipl, vec, evcnt);
}

/* ARGSUSED */
static void
vmetwoisrunlink(cookie, vec)
	void *cookie;
	int vec;
{

	isrunlink_vectored(vec);
}

/* ARGSUSED */
static struct evcnt *
vmetwoisrevcnt(cookie, ipl)
	void *cookie;
	int ipl;
{

	return (isrlink_evcnt(ipl));
}
