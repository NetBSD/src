/*	$NetBSD: hyper.c,v 1.4 1998/12/14 20:33:45 is Exp $ */

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
 * zbus HyperCom driver
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
#include <amiga/dev/zbusvar.h>


struct hyper_softc {
	struct device sc_dev;
	struct bus_space_tag sc_bst;
};

int hypermatch __P((struct device *, struct cfdata *, void *));
void hyperattach __P((struct device *, struct device *, void *));
int hyperprint __P((void *auxp, const char *));

struct cfattach hyper_ca = {
	sizeof(struct hyper_softc), hypermatch, hyperattach
};

int
hypermatch(parent, cfp, auxp)
	struct device *parent;
	struct cfdata *cfp;
	void *auxp;
{

	struct zbus_args *zap;

	zap = auxp;

	if (zap->manid != 5001)
		return (0);

	if (zap->prodid != 2 && zap->prodid != 3  &&
	    zap->prodid != 6 && zap->prodid != 7)
		return (0);

	return (1);
}

struct hyper_devs {
	char *name;
	unsigned off;
	int arg;
};

struct hyper_devs hyper3devs[] = {
	{ "com", 0x00, 115200 * 16 * 4 },
	{ "com", 0x08, 115200 * 16 * 4 },
	/* not yet { "lpt", 0x40, 0 }, */
	{ 0 }
};

struct hyper_devs hyper4devs[] = {
	{ "com", 0x00, 115200 * 16 * 4 },
	{ "com", 0x08, 115200 * 16 * 4 },
	{ "com", 0x10, 115200 * 16 * 4 },
	{ "com", 0x18, 115200 * 16 * 4 },
	{ 0 }
};

struct hyper_devs hyper3Pdevs[] = {
	{ "com", 0x0400, 115200 * 16 * 4 },
	{ "com", 0x0000, 115200 * 16 * 4 },
#ifdef notyet
	{ "lpt", 0x0800, 0 },
#endif
	{ 0 }
};

struct hyper_devs hyper4Pdevs[] = {
	{ "com", 0x0400, 115200 * 16 * 4 },
	{ "com", 0x0000, 115200 * 16 * 4 },
	{ "com", 0x0c00, 115200 * 16 * 4 },
	{ "com", 0x1000, 115200 * 16 * 4 },
#ifdef notyet
	{ "lpt", 0x0800, 0 },
	{ "lpt", 0x1400, 0 },
#endif
	{ 0 }
};

void
hyperattach(parent, self, auxp)
	struct device *parent, *self;
	void *auxp;
{
	struct hyper_softc *hprsc;
	struct hyper_devs  *hprsd;
	struct zbus_args *zap;
	struct supio_attach_args supa;

	hprsc = (struct hyper_softc *)self;
	zap = auxp;

	if (zap->prodid == 2) {
		if (parent)
			printf(": HyperCom 4\n");
		hprsc->sc_bst.base = (u_long)zap->va + 1;
		hprsc->sc_bst.stride = 2;
		hprsd = hyper4devs;
	} else if (zap->prodid == 3) {
		if (parent)
			printf(": HyperCom 3Z\n");
		hprsc->sc_bst.base = (u_long)zap->va + 0;
		hprsc->sc_bst.stride = 2;
		hprsd = hyper3devs;
	} else if (zap->prodid == 6) {
		if (parent)
			printf(": HyperCom 4+\n");
		hprsc->sc_bst.base = (u_long)zap->va + 0x8000;
		hprsc->sc_bst.stride = 2;
		hprsd = hyper4Pdevs;
	} else /* if (zap->prodid == 7) */ {
		if (parent)
			printf(": HyperCom 3+\n");
		hprsc->sc_bst.base = (u_long)zap->va + 0x8000;
		hprsc->sc_bst.stride = 2;
		hprsd = hyper3Pdevs;
	}

	supa.supio_iot = &hprsc->sc_bst;
	supa.supio_ipl = 6;

	while (hprsd->name) {
		supa.supio_name = hprsd->name;
		supa.supio_iobase = hprsd->off;
		supa.supio_arg = hprsd->arg;
		config_found(self, &supa, hyperprint); /* XXX */
		++hprsd;
	}
}

int
hyperprint(auxp, pnp)
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
