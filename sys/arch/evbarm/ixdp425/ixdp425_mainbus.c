/*	$NetBSD: ixdp425_mainbus.c,v 1.2 2003/05/24 01:59:32 ichiro Exp $ */

/*
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixdp425_mainbus.c,v 1.2 2003/05/24 01:59:32 ichiro Exp $");

/*
 * front-end for the ixp425 NetworkProcessor.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>

#include <evbarm/ixdp425/ixdp425var.h>

#include "locators.h"

static int	ixp425_mainbus_match(struct device *, struct cfdata *, void *);
static void	ixp425_mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(ixpio_mainbus, sizeof(struct ixp425_softc),
    ixp425_mainbus_match, ixp425_mainbus_attach, NULL, NULL);

extern struct bus_space ixp425_bs_tag;

int
ixp425_mainbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (1);
}

void
ixp425_mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct ixp425_softc *sc = (void *) self;

	ixp425_bs_init(&ixp425_bs_tag, sc);
	sc->sc_iot = &ixp425_bs_tag;
	sc->sc_ioh = IXP425_IO_VBASE;

	/*
	 * Initialize the interrupt part of our PCI chipset tag
	 */
	ixdp425_pci_init(&sc->ia_pci_chipset, sc);

	ixp425_attach(sc);
}
