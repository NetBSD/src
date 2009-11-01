/*	$NetBSD: npx_hv.c,v 1.7.8.1 2009/11/01 13:58:44 jym Exp $	*/

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npx_hv.c,v 1.7.8.1 2009/11/01 13:58:44 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/stdarg.h>

#include <xen/xen.h>
#include <xen/hypervisor.h>

#include <i386/isa/npxvar.h>

int npx_hv_probe(device_t, cfdata_t, void *);
void npx_hv_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(npx_hv, sizeof(struct npx_softc),
    npx_hv_probe, npx_hv_attach, NULL, NULL);

int
npx_hv_probe(device_t parent, cfdata_t match, void *aux)
{
	struct xen_npx_attach_args *xa = (struct xen_npx_attach_args *)aux;

	if (strcmp(xa->xa_device, "npx") == 0)
		return 1;
	return 0;
}

void
npx_hv_attach(device_t parent, device_t self, void *aux)
{
	struct npx_softc *sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_type = NPX_EXCEPTION;

	aprint_normal(": using exception 16\n");

	npxattach(sc);
}
