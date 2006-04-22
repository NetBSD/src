/*	$NetBSD: ixp425_sip.c,v 1.9.6.1 2006/04/22 11:37:18 simonb Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ixp425_sip.c,v 1.9.6.1 2006/04/22 11:37:18 simonb Exp $");

/*
 * Slow peripheral bus of IXP425 Processor
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <arm/xscale/ixp425var.h>
#include <arm/xscale/ixp425_sipvar.h>

#include "locators.h"

static int	ixpsip_match(struct device *, struct cfdata *, void *);
static void	ixpsip_attach(struct device *, struct device *, void *);
static int	ixpsip_search(struct device *, struct cfdata *,
			      const int *, void *);
static int	ixpsip_print(void *, const char *);

CFATTACH_DECL(ixpsip, sizeof(struct ixpsip_softc),
		ixpsip_match, ixpsip_attach, NULL, NULL);

struct ixpsip_softc *ixpsip_softc;

int
ixpsip_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (1);
}

void
ixpsip_attach(struct device *parent, struct device *self, void *aux)
{
	struct ixpsip_softc *sc = (void *) self;
	sc->sc_iot = &ixp425_bs_tag;

	ixpsip_softc = sc;

	printf("\n");

	if (bus_space_map(sc->sc_iot, IXP425_EXP_HWBASE, IXP425_EXP_SIZE,
	    0, &sc->sc_ioh)) {
		printf("%s: Can't map expansion bus control registers!\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 *  Attach each devices
	 */
	config_search_ia(ixpsip_search, self, "ixpsip", NULL);
}

int
ixpsip_search(struct device *parent, struct cfdata *cf,
	      const int *ldesc, void *aux)
{
	struct ixpsip_softc *sc = (struct ixpsip_softc *)parent;
	struct ixpsip_attach_args sa;

	sa.sa_iot = sc->sc_iot;
	sa.sa_addr = cf->cf_loc[IXPSIPCF_ADDR];
	sa.sa_size = cf->cf_loc[IXPSIPCF_SIZE];
	sa.sa_index = cf->cf_loc[IXPSIPCF_INDEX];
	sa.sa_intr = cf->cf_loc[IXPSIPCF_INTR];

	if (config_match(parent, cf, &sa) > 0)
		config_attach(parent, cf, &sa, ixpsip_print);

	return (0);
}

static int
ixpsip_print(void *aux, const char *name)
{
	struct ixpsip_attach_args *sa = (struct ixpsip_attach_args*)aux;

	if (sa->sa_size)
		aprint_normal(" addr 0x%lx", sa->sa_addr);
	if (sa->sa_size > 1)
		aprint_normal("-0x%lx", sa->sa_addr + sa->sa_size - 1);
	if (sa->sa_index > 1)
		aprint_normal(" index %d", sa->sa_index);
	if (sa->sa_intr > 1)
		aprint_normal(" intr %d", sa->sa_intr);

	return (UNCONF);
}
