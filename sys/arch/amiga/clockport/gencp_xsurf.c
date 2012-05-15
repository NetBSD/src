/*      $NetBSD: gencp_xsurf.c,v 1.1 2012/05/15 17:35:43 rkujawa Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/* Clockports on top of X-Surf. */

#include <sys/cdefs.h>

#include <sys/systm.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/kmem.h>

#include <machine/cpu.h>

#include <amiga/amiga/device.h>

#include <amiga/dev/zbusvar.h>
#include <amiga/dev/xsurfvar.h>

#include <amiga/clockport/clockportvar.h>

static int	gencp_xsurf_match(struct device *pdp, struct cfdata *cfp, void *aux);
static void	gencp_xsurf_attach(device_t parent, device_t self, void *aux);

CFATTACH_DECL_NEW(gencp_xsurf, sizeof(struct gencp_softc),
    gencp_xsurf_match, gencp_xsurf_attach, NULL, NULL);

static int
gencp_xsurf_match(struct device *pdp, struct cfdata *cfp, void *aux)
{
	struct xsurfbus_attach_args *xsb_aa;
	static int attach_count = 0;

	xsb_aa = (struct xsurfbus_attach_args *) aux;

	if (strcmp(xsb_aa->xaa_name, "gencp_xsurf") != 0) 
		return 0;

	/* No X-Surf with more than 2 clockports exists. */
	if (attach_count >= 2)
		return 0;	

	attach_count++;

	return 1;
}

static void
gencp_xsurf_attach(device_t parent, device_t self, void *aux)
{
	struct gencp_softc *sc;
	struct bus_space_tag cpb_bst;
	struct clockportbus_attach_args cpb_aa;
	struct xsurfbus_attach_args *xsb_aa;

	xsb_aa = (struct xsurfbus_attach_args *) aux;
	sc = device_private(self);
	sc->sc_dev = self;
	sc->cpb_aa = &cpb_aa;

	/* Set the address and bus access methods. */
	cpb_bst.base = xsb_aa->xaa_base;
	cpb_bst.absm = &amiga_bus_stride_4;

	sc->cpb_aa->cp_iot = &cpb_bst;

	gencp_attach(sc);
}

