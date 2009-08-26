/*	$NetBSD: sbdio.c,v 1.3.18.1 2009/08/26 03:46:39 matt Exp $	*/

/*-
 * Copyright (c) 2004, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
__KERNEL_RCSID(0, "$NetBSD: sbdio.c,v 1.3.18.1 2009/08/26 03:46:39 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#define _EWS4800MIPS_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/sbdvar.h>
#include <machine/sbdiovar.h>

#include "ioconf.h"

struct sbdio_softc {
	device_t sc_dev;
	struct ews4800mips_bus_space sc_bus_tag;
	struct ews4800mips_bus_dma_tag sc_dma_tag;
};

static int sbdio_match(device_t, cfdata_t, void *);
static void sbdio_attach(device_t, device_t, void *);
static int sbdio_print(void *, const char *);

CFATTACH_DECL_NEW(sbdio, sizeof(struct sbdio_softc),
    sbdio_match, sbdio_attach, NULL, NULL);

static int sbdio_found;

int
sbdio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (sbdio_found != 0)
		return 0;

	if (strcmp(ma->ma_name, sbdio_cd.cd_name) != 0)
		return 0;

	if (platform.sbdiodevs == NULL)
		return 0;

	return 1;
}

void
sbdio_attach(device_t parent, device_t self, void *aux)
{
	struct sbdio_softc *sc = device_private(self);
	struct sbdio_attach_args sa;
	const struct sbdiodevdesc *sd;

	sbdio_found = 1;

	sc->sc_dev = self;
	aprint_normal("\n");

	/* structure assignment */
	sc->sc_dma_tag = ews4800mips_default_bus_dma_tag;

	bus_space_create(&sc->sc_bus_tag, device_xname(sc->sc_dev),
	    MIPS_KSEG1_START, MIPS_KSEG2_START - MIPS_KSEG1_START); /* XXX */

	for (sd = platform.sbdiodevs; sd->sd_name != NULL; sd++) {
		sa.sa_name  = sd->sd_name;
		sa.sa_bust  = &sc->sc_bus_tag;
		sa.sa_dmat  = &sc->sc_dma_tag;
		sa.sa_addr1 = sd->sd_addr1;
		sa.sa_addr2 = sd->sd_addr2;
		sa.sa_irq   = sd->sd_irq;
		sa.sa_flags = sd->sd_flags;
		config_found(self, &sa, sbdio_print);
	}
}

int
sbdio_print(void *aux, const char *pnp)
{
	struct sbdio_attach_args *sa = aux;

	if (sa->sa_addr1 != (paddr_t)-1) {
		aprint_normal(" at %#"PRIxPADDR, sa->sa_addr1);
		if (sa->sa_addr2 != (paddr_t)-1)
			aprint_normal(", %#"PRIxPADDR, sa->sa_addr2);
	}
	if (sa->sa_irq != -1)
		aprint_normal(" irq %d", sa->sa_irq);

	return pnp ? QUIET : UNCONF;
}
