/*	$NetBSD: sbdio.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sbdio.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $");

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
	struct device sc_dev;
	struct ews4800mips_bus_space sc_bus_tag;
	struct ews4800mips_bus_dma_tag sc_dma_tag;
};

int sbdio_match(struct device *, struct cfdata *, void *);
void sbdio_attach(struct device *, struct device *, void *);
int sbdio_print(void *, const char *);

CFATTACH_DECL(sbdio, sizeof(struct sbdio_softc),
    sbdio_match, sbdio_attach, NULL, NULL);

static int sbdio_found;

int
sbdio_match(struct device *parent, struct cfdata *match, void *aux)
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
sbdio_attach(struct device *parent, struct device *self, void *aux)
{
	struct sbdio_softc *sc = (void *)self;
	struct sbdio_attach_args sa;
	const struct sbdiodevdesc *sd;

	sbdio_found = 1;

	printf("\n");

	/* structure assignment */
	sc->sc_dma_tag = ews4800mips_default_bus_dma_tag;

	bus_space_create(&sc->sc_bus_tag, sc->sc_dev.dv_xname,
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

	return pnp ? QUIET : UNCONF;
}
