/* $NetBSD: sio.c,v 1.3.56.1 2007/03/12 05:48:42 rmind Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: sio.c,v 1.3.56.1 2007/03/12 05:48:42 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>

#include <luna68k/luna68k/isr.h>
#include <luna68k/dev/siovar.h>

static int  sio_match __P((struct device *, struct cfdata *, void *));
static void sio_attach __P((struct device *, struct device *, void *));
static int  sio_print __P((void *, const char *));

CFATTACH_DECL(sio, sizeof(struct sio_softc),
    sio_match, sio_attach, NULL, NULL);
extern struct cfdriver sio_cd;

static void nullintr __P((int));
static int xsiointr __P((void *));

static int
sio_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, sio_cd.cd_name))
		return 0;
	if (badaddr((void *)ma->ma_addr, 4))
		return 0;
	return 1;
}

static void
sio_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sio_softc *sc = (void *)self;
	struct mainbus_attach_args *ma = aux;
	struct sio_attach_args sio_args;
	int channel;
	extern int sysconsole; /* console: 0 for ttya, 1 for desktop */

	printf(": 7201a\n");

	sc->scp_ctl = (void *)ma->ma_addr;
	sc->scp_intr[0] = sc->scp_intr[1] = nullintr;
	for (channel = 0; channel < 2; channel++) {
		sio_args.channel = channel;
		sio_args.hwflags = (channel == sysconsole);
		config_found(self, (void *)&sio_args, sio_print);
	}

	isrlink_autovec(xsiointr, sc, ma->ma_ilvl, ISRPRI_TTYNOBUF);
}

static int
sio_print(aux, name)
	void *aux;
	const char *name;
{
	struct sio_attach_args *args = aux;

	if (name != NULL)
		aprint_normal("%s: ", name);

	if (args->channel != -1)
		aprint_normal(" channel %d", args->channel);

	return UNCONF;
}

static int
xsiointr(arg)
	void *arg;
{
	struct sio_softc *sc = arg;

	(*sc->scp_intr[0])(0); 	/* 0: ttya system serial port */
	(*sc->scp_intr[1])(1);	/* 1: keyboard and mouse */
	return 1;
}

static void nullintr(v) int v; { }
