/*	$NetBSD: drsupio.c,v 1.8 2000/03/16 16:37:20 kleink Exp $ */

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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
 * DraCo multi-io chip bus space stuff
 */

#include <sys/types.h>

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/param.h>

#include <machine/bus.h>
#include <machine/conf.h>

#include <amiga/include/cpu.h>

#include <amiga/amiga/device.h>
#include <amiga/amiga/drcustom.h>

#include <amiga/dev/supio.h>

struct drsupio_softc {
	struct device sc_dev;
	struct bus_space_tag sc_bst;
};

int drsupiomatch __P((struct device *, struct cfdata *, void *));
void drsupioattach __P((struct device *, struct device *, void *));
int drsupprint __P((void *auxp, const char *));
void drlptintack __P((void *));

struct cfattach drsupio_ca = {
	sizeof(struct drsupio_softc), drsupiomatch, drsupioattach
};

int
drsupiomatch(parent, cfp, auxp)
	struct device *parent;
	struct cfdata *cfp;
	void *auxp;
{
	static int drsupio_matched = 0;

	/* Exactly one of us lives on the DraCo */
	if (!is_draco() || !matchname(auxp, "drsupio") || drsupio_matched)
		return 0;

	drsupio_matched = 1;
	return 1;
}

struct drsupio_devs {
	char *name;
	int off;
	int arg;
} drsupiodevs[] = {
	{ "com", 0x3f8, 115200 * 16 },
	{ "com", 0x2f8, 115200 * 16 },
	{ "lpt", 0x278, (int)drlptintack },
	{ "fdc", 0x3f0, 0 },
	/* WD port? */
	{ 0 }
};

void
drsupioattach(parent, self, auxp)
	struct device *parent, *self;
	void *auxp;
{
	struct drsupio_softc *drsc;
	struct drsupio_devs  *drsd;
	struct drioct *ioct;
	struct supio_attach_args supa;

	drsc = (struct drsupio_softc *)self;
	drsd = drsupiodevs;

	if (parent)
		printf("\n");

	drsc->sc_bst.base = DRCCADDR + NBPG * DRSUPIOPG + 1;
	drsc->sc_bst.absm = &amiga_bus_stride_4;
	
	supa.supio_iot = &drsc->sc_bst;
	supa.supio_ipl = 5;

	while (drsd->name) {
		supa.supio_name = drsd->name;
		supa.supio_iobase = drsd->off;
		supa.supio_arg = drsd->arg;
		config_found(self, &supa, drsupprint); /* XXX */
		++drsd;
	}

	drlptintack(0);
	ioct = (struct drioct *)(DRCCADDR + NBPG * DRIOCTLPG);
	ioct->io_status2 |= DRSTAT2_PARIRQENA;
}

void
drlptintack(p)
	void *p;
{
	struct drioct *ioct;

	(void)p;
	ioct = (struct drioct *)(DRCCADDR + NBPG * DRIOCTLPG);

	ioct->io_parrst = 0;	/* any value works */
}

int
drsupprint(auxp, pnp)
	void *auxp;
	const char *pnp;
{
	struct supio_attach_args *supa;
	supa = auxp;

	if (pnp == NULL)
		return(QUIET);

	printf("%s at %s port 0x%02x",
	    supa->supio_name, pnp, supa->supio_iobase);

	return(UNCONF);
}
