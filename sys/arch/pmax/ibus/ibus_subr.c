/*	$NetBSD: ibus_subr.c,v 1.1.2.1 1998/10/15 02:41:16 nisimura Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: ibus_subr.c,v 1.1.2.1 1998/10/15 02:41:16 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <pmax/ibus/ibusvar.h>

static int ibusprint __P((void *, const char *));
extern struct cfdriver ibus_cd;

void
ibus_devattach(dev, aux)
	struct device *dev;
	void *aux;
{
	struct ibus_softc *sc = (struct ibus_softc *)dev;
	struct ibus_dev_attach_args *ibd = aux;
	int i;

	sc->ibd_ndevs = ibd->ibd_ndevs;
	sc->ibd_devs = ibd->ibd_devs;
        sc->ibd_establish = ibd->ibd_establish;
        sc->ibd_disestablish = ibd->ibd_disestablish;
	for (i = 0; i < sc->ibd_ndevs; i++) {
		config_found(dev, &sc->ibd_devs[i], ibusprint);
	}
}

static int
ibusprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	if (pnp)
		return (QUIET);
	return (UNCONF);
}

void
ibus_intr_establish(dev, cookie, level, func, arg)
	struct device *dev;
	void *cookie, *arg;
	int level;
	int (*func) __P((void *));
{
	struct ibus_softc *sc = (void *)ibus_cd.cd_devs[0];

	(*sc->ibd_establish)(dev, cookie, level, func, arg);
}

void
ibus_intr_disestablish(dev, cookie)
	struct device *dev;
	void *cookie;
{
	struct ibus_softc *sc = (void *)ibus_cd.cd_devs[0];

	(*sc->ibd_disestablish)(dev, cookie);
}
