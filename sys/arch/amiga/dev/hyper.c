/*	$NetBSD: hyper.c,v 1.10.8.1 2002/02/11 20:06:59 jdolecek Exp $ */

/*-
 * Copyright (c) 1997,1998 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hyper.c,v 1.10.8.1 2002/02/11 20:06:59 jdolecek Exp $");

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

int hypermatch(struct device *, struct cfdata *, void *);
void hyperattach(struct device *, struct device *, void *);
int hyperprint(void *auxp, const char *);

struct cfattach hyper_ca = {
	sizeof(struct hyper_softc), hypermatch, hyperattach
};

struct hyper_prods {
	char *name;
	unsigned baseoff;
} hyperproducts [] = {
	{0, 0},			/* 0: not used */
	{0, 0},			/* 1: handled by aster driver */
	{"4", 1},		/* 2 */
	{"Z3", 0},		/* 3 */
	{0, 0},			/* 4: not used */
	{0, 0},			/* 5: not used */
	{"4+", 0x8000},		/* 6 */
	{"3+", 0x8000}		/* 7 */
};

int
hypermatch(struct device *parent, struct cfdata *cfp, void *auxp)
{

	struct zbus_args *zap;

	zap = auxp;

	if (zap->manid != 5001)
		return (0);

	if (zap->prodid < sizeof(hyperproducts)/sizeof(*hyperproducts) &&
	    hyperproducts[zap->prodid].name)

		return (1);

	return (0);
}

#define HYPERPROD3	(1<<3)
#define HYPERPROD4	(1<<2)
#define HYPERPROD4PLUS	(1<<6)
#define HYPERPROD3PLUS	(1<<7)

struct hyper_devs {
	char *name;
	unsigned off;
	int arg;
	u_int32_t productmask;	/* XXX only prodid 0..31 */
} hyperdevices[] = {
	{ "com", 0x00, 115200 * 16 * 4, HYPERPROD3 | HYPERPROD4 },
	{ "com", 0x08, 115200 * 16 * 4, HYPERPROD3 | HYPERPROD4 },
	{ "com", 0x10, 115200 * 16 * 4, HYPERPROD4 },
	{ "com", 0x18, 115200 * 16 * 4, HYPERPROD4 },
	/* not yet { "lpt", 0x40, 0, HYPERPROD3 }, */
	{ "com", 0x0400, 115200 * 16 * 4, HYPERPROD3PLUS | HYPERPROD4PLUS },
	{ "com", 0x0000, 115200 * 16 * 4, HYPERPROD3PLUS | HYPERPROD4PLUS },
	{ "com", 0x0c00, 115200 * 16 * 4, HYPERPROD4PLUS },
	{ "com", 0x1000, 115200 * 16 * 4, HYPERPROD4PLUS },
	{ "lpt", 0x0800, 0, HYPERPROD3PLUS | HYPERPROD4PLUS },
	{ "lpt", 0x1400, 0, HYPERPROD4PLUS },
};

void
hyperattach(struct device *parent, struct device *self, void *auxp)
{
	struct hyper_softc *hprsc;
	struct hyper_devs  *hprsd;
	struct zbus_args *zap;
	struct supio_attach_args supa;
	struct hyper_prods *hprpp;

	hprsc = (struct hyper_softc *)self;
	zap = auxp;
	hprpp = &hyperproducts[zap->prodid];

	if (parent)
		printf(": Hypercom %s\n", hprpp->name);

	hprsc->sc_bst.base = (u_long)zap->va + hprpp->baseoff;
	hprsc->sc_bst.absm = &amiga_bus_stride_4;

	supa.supio_iot = &hprsc->sc_bst;
	supa.supio_ipl = 6;

	hprsd = hyperdevices;

	while (hprsd->name) {
		if (hprsd->productmask & (1 << zap->prodid)) {
			supa.supio_name = hprsd->name;
			supa.supio_iobase = hprsd->off;
			supa.supio_arg = hprsd->arg;
			config_found(self, &supa, hyperprint); /* XXX */
		}
		++hprsd;
	}
}

int
hyperprint(void *auxp, const char *pnp)
{
	struct supio_attach_args *supa;
	supa = auxp;

	if (pnp == NULL)
		return(QUIET);

	printf("%s at %s port 0x%02x",
	    supa->supio_name, pnp, supa->supio_iobase);

	return(UNCONF);
}
