/* $NetBSD: ibus.c,v 1.2 1999/11/15 09:50:29 nisimura Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: ibus.c,v 1.2 1999/11/15 09:50:29 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <pmax/ibus/ibusvar.h>

extern struct cfdriver ibus_cd;

void
ibusattach(parent, self, aux)
        struct device *parent, *self;
        void *aux;
{
        struct ibus_softc *sc = (struct ibus_softc *)self;
	struct ibus_dev_attach_args *ida = aux;
        int i;

        printf("\n");

        sc->sc_intr_establish = ida->ida_establish;
        sc->sc_intr_disestablish = ida->ida_disestablish;

	for (i = 0; i < ida->ida_ndevs; i++) {
		config_found(self, &ida->ida_devs[i], ibusprint);
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
ibus_intr_establish(dev, cookie, level, handler, arg)
	struct device *dev;
	void *cookie;
	int level;
	int (*handler) __P((void *));
	void *arg;
{
	struct ibus_softc *sc = ibus_cd.cd_devs[0];

	(*sc->sc_intr_establish)(dev, cookie, level, handler, arg);
}


void
ibus_intr_disestablish(dev, arg)
	struct device *dev;
	void *arg;
{
	struct ibus_softc *sc = ibus_cd.cd_devs[0];

	(*sc->sc_intr_disestablish)(dev, arg);
}
