/* $NetBSD: mcclock_tlsb.c,v 1.5.4.2 1997/07/22 18:51:04 jonathan Exp $ */

/*
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <machine/options.h>		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: mcclock_tlsb.c,v 1.5.4.2 1997/07/22 18:51:04 jonathan Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/dec/clockvar.h>
#include <dev/dec/mcclockvar.h>
#include <alpha/tlsb/tlsbreg.h>
#include <dev/ic/mc146818reg.h>

#define	KV(_addr)	((caddr_t)ALPHA_PHYS_TO_K0SEG((_addr)))
/*
 * Registers are 64 bytes apart (and 1 byte wide)
 */
#define	REGSHIFT	6

struct mcclock_tlsb_softc {
	struct mcclock_softc	sc_mcclock;
	unsigned long regbase;
};

int	mcclock_tlsb_match __P((struct device *, struct cfdata *, void *));
void	mcclock_tlsb_attach __P((struct device *, struct device *, void *));

struct cfattach mcclock_tlsb_ca = {
	sizeof (struct mcclock_tlsb_softc),
	mcclock_tlsb_match,
	mcclock_tlsb_attach, 
};

static void	mcclock_tlsb_write __P((struct mcclock_softc *, u_int, u_int));
static u_int	mcclock_tlsb_read __P((struct mcclock_softc *, u_int));

const struct mcclock_busfns mcclock_tlsb_busfns = {
	mcclock_tlsb_write, mcclock_tlsb_read,
};
extern struct cfdriver mcclock_cd;

int
mcclock_tlsb_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct confargs *ca = aux;
	if (strcmp(ca->ca_name, mcclock_cd.cd_name))
		return (0);
	return (1);
}

void
mcclock_tlsb_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct mcclock_tlsb_softc *sc = (struct mcclock_tlsb_softc *)self;
	struct confargs *ca = aux;
	sc->regbase = TLSB_GBUS_BASE + ca->ca_offset;
	mcclock_attach(&sc->sc_mcclock, &mcclock_tlsb_busfns);
}

static void
mcclock_tlsb_write(mcsc, reg, val)
	struct mcclock_softc *mcsc;
	u_int reg, val;
{
	struct mcclock_tlsb_softc *sc = (struct mcclock_tlsb_softc *)mcsc;
	unsigned char *ptr = (unsigned char *)
		KV(sc->regbase + (reg << REGSHIFT));
	*ptr = val;
}

static u_int
mcclock_tlsb_read(mcsc, reg)
	struct mcclock_softc *mcsc;
	u_int reg;
{
	struct mcclock_tlsb_softc *sc = (struct mcclock_tlsb_softc *)mcsc;
	unsigned char *ptr = (unsigned char *)
		KV(sc->regbase + (reg << REGSHIFT));
	return *ptr;
}
