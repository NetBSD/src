/*	$NetBSD: ibus.c,v 1.1 1998/04/19 02:52:45 jonathan Exp $	*/

/*
 * Copyright (c) 1998 Jonathan Stone.  All rights reserved.
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
 *	This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: ibus.c,v 1.1 1998/04/19 02:52:45 jonathan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <pmax/ibus/ibusvar.h>
#include <pmax/pmax/pmaxtype.h>

static int  ibusmatch __P((struct device *, struct cfdata *, void *));
static void ibusattach __P((struct device *, struct device *, void *));
int  ibusprint __P((void *, const char *));

struct ibus_softc {
	struct device	ibd_dev;
	int		ibd_ndevs;
	struct ibus_attach_args *ibd_devs;
	ibus_intr_establish_t (*ibd_establish);

	ibus_intr_disestablish_t (*ibd_disestablish);
};

struct cfattach ibus_ca = {
        sizeof(struct ibus_softc), ibusmatch, ibusattach
};

extern struct cfdriver ibus_cd;


static int
ibusmatch(parent, cfdata, aux)
        struct device *parent;
        struct cfdata *cfdata;
        void *aux;
{
	struct ibus_dev_attach_args *ibd =  (struct ibus_dev_attach_args *)aux;

	if (strcmp(ibd->ibd_busname, "ibus") != 0)  {
		return 0;
	}

	if (systype != DS_PMAX && systype != DS_MIPSMATE && systype != DS_3MAX)
		return (0);
	return(1);
}


static void
ibusattach(parent, self, aux)
        struct device *parent;
        struct device *self;
        void *aux;
{
        struct ibus_softc *sc = (struct ibus_softc *)self;
	struct ibus_dev_attach_args* ibd_args = aux;
        struct ibus_attach_args *child;
        int i;

        printf("\n");

	sc->ibd_ndevs = ibd_args->ibd_ndevs;
	sc->ibd_devs = ibd_args->ibd_devs;
        sc->ibd_establish = ibd_args->ibd_establish;
        sc->ibd_disestablish = ibd_args->ibd_disestablish;

	for (i = 0; i <  sc->ibd_ndevs; i++) {
		child = &sc->ibd_devs[i];
		config_found(self, child, ibusprint);
	}
}

int
ibusprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	if (pnp)
		return (QUIET);
	return (UNCONF);
}


void
ibus_intr_establish(cookie, level, handler, arg)
	void * cookie;
	int level;
	int (*handler) __P((intr_arg_t));
	intr_arg_t arg;
{
	((struct ibus_softc*)ibus_cd.cd_devs[0])->
	      ibd_establish(cookie, level, handler, arg);
}


void
ibus_intr_disestablish(ia)
	struct ibus_attach_args *ia;
{
	((struct ibus_softc*)ibus_cd.cd_devs[0])->ibd_disestablish(ia);
}
