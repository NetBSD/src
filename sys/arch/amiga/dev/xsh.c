/*	$NetBSD: xsh.c,v 1.2.10.2 2014/08/20 00:02:43 tls Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xsh.c,v 1.2.10.2 2014/08/20 00:02:43 tls Exp $");

/*
 * X-Surf 100 driver. 
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/bus.h>

#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>

#include <amiga/dev/zbusvar.h>
#include <amiga/dev/xshvar.h>

int	xsh_match(device_t, cfdata_t , void *);
void	xsh_attach(device_t, device_t, void *);
static int xsh_print(void *, const char *);

struct xsh_softc {
	device_t sc_dev;	
};

CFATTACH_DECL_NEW(xsh, sizeof(struct xsh_softc),
    xsh_match, xsh_attach, NULL, NULL);

#define XSURF100_NE_OFFSET		0x0800

int
xsh_match(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap = aux;

	/* X-surf 100 ethernet card */
	if (zap->manid == 4626 && zap->prodid == 100) 
		return (1);

	return (0);
}

void
xsh_attach(device_t parent, device_t self, void *aux)
{
	struct xsh_softc *sc;
	struct xshbus_attach_args xaa_ne;

	struct zbus_args *zap = aux;

	sc = device_private(self);
	sc->sc_dev = self;

	aprint_normal(": Individual Computers X-Surf 100\n");

	/* Add ne(4). */
	xaa_ne.xaa_base = (bus_addr_t)zap->va + XSURF100_NE_OFFSET;
	strcpy(xaa_ne.xaa_name, "ne_xsh");
	config_found_ia(sc->sc_dev, "xshbus", &xaa_ne, xsh_print);

	/* TODO: add USB module... */
}

static int
xsh_print(void *aux, const char *w)
{
	if (w == NULL)
		return 0;

	return 0;
}

