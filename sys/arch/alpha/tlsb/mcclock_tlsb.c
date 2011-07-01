/* $NetBSD: mcclock_tlsb.c,v 1.17 2011/07/01 19:19:51 dyoung Exp $ */

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: mcclock_tlsb.c,v 1.17 2011/07/01 19:19:51 dyoung Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <alpha/tlsb/gbusvar.h>

#include <alpha/tlsb/tlsbreg.h>		/* XXX */

#include <dev/clock_subr.h>

#include <dev/ic/mc146818reg.h>
#include <dev/ic/mc146818var.h>

#include <alpha/alpha/mcclockvar.h>

#include "ioconf.h"

#define	KV(_addr)	((void *)ALPHA_PHYS_TO_K0SEG((_addr)))
/*
 * Registers are 64 bytes apart (and 1 byte wide)
 */
#define	REGSHIFT	6

struct mcclock_tlsb_softc {
	struct mc146818_softc	sc_mc146818;
	unsigned long regbase;
};

int	mcclock_tlsb_match(device_t, cfdata_t, void *);
void	mcclock_tlsb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mcclock_tlsb, sizeof(struct mcclock_tlsb_softc),
    mcclock_tlsb_match, mcclock_tlsb_attach, NULL, NULL);

static void	mcclock_tlsb_write(struct mc146818_softc *, u_int, u_int);
static u_int	mcclock_tlsb_read(struct mc146818_softc *, u_int);


int
mcclock_tlsb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct gbus_attach_args *ga = aux;

	if (strcmp(ga->ga_name, mcclock_cd.cd_name))
		return (0);
	return (1);
}

void
mcclock_tlsb_attach(device_t parent, device_t self, void *aux)
{
	struct mcclock_tlsb_softc *tsc = device_private(self);
	struct gbus_attach_args *ga = aux;
	struct mc146818_softc *sc = &tsc->sc_mc146818;

	/* XXX Should be bus.h'd, so we can accommodate the kn7aa. */
	tsc->regbase = TLSB_GBUS_BASE + ga->ga_offset;

	sc->sc_dev = self;
	sc->sc_mcread  = mcclock_tlsb_read;
	sc->sc_mcwrite = mcclock_tlsb_write;

	mcclock_attach(sc);
}

static void
mcclock_tlsb_write(struct mc146818_softc *sc, u_int reg, u_int val)
{
	struct mcclock_tlsb_softc *tsc = (void *)sc;
	unsigned char *ptr = (unsigned char *)
		KV(tsc->regbase + (reg << REGSHIFT));

	*ptr = val;
}

static u_int
mcclock_tlsb_read(struct mc146818_softc *sc, u_int reg)
{
	struct mcclock_tlsb_softc *tsc = (void *)sc;
	unsigned char *ptr = (unsigned char *)
		KV(tsc->regbase + (reg << REGSHIFT));

	return *ptr;
}
