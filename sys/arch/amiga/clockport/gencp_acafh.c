/*      $NetBSD: gencp_acafh.c,v 1.1.10.2 2014/08/20 00:02:43 tls Exp $ */

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

/* Clockport on ACA500. */

#include <sys/cdefs.h>

#include <sys/systm.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/kmem.h>

#include <machine/cpu.h>

#include <amiga/amiga/device.h>

#include <amiga/dev/acafhvar.h>
#include <amiga/dev/zbusvar.h>

#include <amiga/clockport/clockportvar.h>

static int	gencp_acafh_match(device_t, cfdata_t, void *);
static void	gencp_acafh_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(gencp_acafh, sizeof(struct gencp_softc),
    gencp_acafh_match, gencp_acafh_attach, NULL, NULL);

static int
gencp_acafh_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acafhbus_attach_args *aaa_aa;
	static int attach_count = 0;

	aaa_aa = aux;

	if (strcmp(aaa_aa->aaa_name, "gencp_acafh") != 0) 
		return 0;

	if (attach_count >= 1)
		return 0;	

	attach_count++;

	return 1;
}

static void
gencp_acafh_attach(device_t parent, device_t self, void *aux)
{
	struct gencp_softc *sc;
	struct bus_space_tag cpb_bst;
	struct clockportbus_attach_args cpb_aa;
	struct acafhbus_attach_args *aaa_aa;

	aaa_aa = aux;
	sc = device_private(self);
	sc->sc_dev = self;
	sc->cpb_aa = &cpb_aa;

	/* Set the address and bus access methods. */
	cpb_bst.base = (bus_addr_t) __UNVOLATILE(ztwomap(aaa_aa->aaa_pbase));
	cpb_bst.absm = &amiga_bus_stride_4;

	sc->cpb_aa->cp_iot = &cpb_bst;

	gencp_attach(sc);
}

