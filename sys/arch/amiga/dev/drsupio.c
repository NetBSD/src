/*	$NetBSD: drsupio.c,v 1.1.2.3 1997/09/22 06:30:33 thorpej Exp $ */

/*
 * Copyright (c) 1997 Ignatios Souvatzis
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Ignatios Souvatzis
 *      for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

struct cfattach drsupio_ca = {
	sizeof(struct drsupio_softc), drsupiomatch, drsupioattach
};

struct cfdriver drsupio_cd = {
	NULL, "drsupio", DV_DULL
};

int
drsupiomatch(parent, cfp, auxp)
	struct device *parent;
	struct cfdata *cfp;
	void *auxp;
{

	/* Exactly one of us lives on the DraCo */

	if (is_draco() && matchname(auxp, "drsupio") && (cfp->cf_unit == 0))
		return 1;

	return 0;
}

struct drsupio_devs {
	char *name;
	int off;
	int arg;
} drsupiodevs[] = {
	{ "com", 0x3f8, 115200 * 16 },
	{ "com", 0x2f8, 115200 * 16 },
	{ "lpt", 0x378, 0 },
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
	struct supio_attach_args supa;

	drsc = (struct drsupio_softc *)self;
	drsd = drsupiodevs;

	if (parent)
		printf("\n");

	drsc->sc_bst.base = DRCCADDR + NBPG * DRSUPIOPG + 1;
	drsc->sc_bst.stride = 2;
	
	supa.supio_iot = &drsc->sc_bst;
	supa.supio_ipl = 5;

	while (drsd->name) {
		supa.supio_name = drsd->name;
		supa.supio_iobase = drsd->off;
		supa.supio_arg = drsd->arg;
		config_found(self, &supa, drsupprint); /* XXX */
		++drsd;
	}
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
