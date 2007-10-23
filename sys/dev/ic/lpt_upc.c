/* $NetBSD: lpt_upc.c,v 1.7.30.1 2007/10/23 20:07:34 ad Exp $ */
/*-
 * Copyright (c) 2000 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lpt_upc.c,v 1.7.30.1 2007/10/23 20:07:34 ad Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/ic/lptvar.h>
#include <dev/ic/upcvar.h>

static int lpt_upc_match(struct device *, struct cfdata *, void *);
static void lpt_upc_attach(struct device *, struct device *, void *);

CFATTACH_DECL(lpt_upc, sizeof(struct lpt_softc),
    lpt_upc_match, lpt_upc_attach, NULL, NULL);

static int
lpt_upc_match(struct device *parent, struct cfdata *cf, void *aux)
{

	/* upc_submatch does it all anyway */
	return 1;
}

static void
lpt_upc_attach(struct device *parent, struct device *self, void *aux)
{
	struct lpt_softc *sc = (struct lpt_softc *)self;
	struct upc_attach_args *ua = aux;

	sc->sc_iot = ua->ua_iot;
	sc->sc_ioh = ua->ua_ioh;

	lpt_attach_subr(sc);
	upc_intr_establish(ua->ua_irqhandle, IPL_TTY, lptintr, sc);
	printf("\n");
}

