/*	$NetBSD: mcclock_jazzio.c,v 1.5 2003/08/07 16:26:51 agc Exp $	*/
/*	$OpenBSD: clock_mc.c,v 1.9 1998/03/16 09:38:26 pefo Exp $	*/
/*	NetBSD: clock_mc.c,v 1.2 1995/06/28 04:30:30 cgd Exp 	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah Hdr: clock.c 1.18 91/01/21
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/10/93
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah Hdr: clock.c 1.18 91/01/21
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mcclock_jazzio.c,v 1.5 2003/08/07 16:26:51 agc Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <dev/ic/mc146818reg.h>

#include <arc/dev/mcclockvar.h>
#include <arc/jazz/jazziovar.h>
#include <arc/jazz/mcclock_jazziovar.h>

int mcclock_jazzio_match __P((struct device *, struct cfdata *, void *));
void mcclock_jazzio_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(mcclock_jazzio, sizeof(struct mcclock_softc),
    mcclock_jazzio_match, mcclock_jazzio_attach, NULL, NULL);

struct mcclock_jazzio_config *mcclock_jazzio_conf = NULL;
int mcclock_jazzio_found = 0;

int
mcclock_jazzio_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct jazzio_attach_args *ja = aux;

	/* make sure that we're looking for this type of device. */
	if (strcmp(ja->ja_name, "dallas_rtc") != 0)
		return (0);

	if (mcclock_jazzio_found)
		return (0);

	return (1);
}

void
mcclock_jazzio_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mcclock_softc *sc = (struct mcclock_softc *)self;
	struct jazzio_attach_args *ja = aux;

	if (mcclock_jazzio_conf == NULL)
		panic("mcclock_jazzio_conf isn't initialized");

	sc->sc_iot = ja->ja_bust;
	if (bus_space_map(sc->sc_iot,
	    ja->ja_addr, mcclock_jazzio_conf->mjc_iosize, 0, &sc->sc_ioh)) {
		printf(": unable to map I/O space\n");
		return;
	}

	mcclock_attach(sc, &mcclock_jazzio_conf->mjc_mcbusfns, 80);

	/* Turn interrupts off, just in case. */
	mc146818_write(sc, MC_REGB, MC_REGB_BINARY | MC_REGB_24HR);

	mcclock_jazzio_found = 1;
}
