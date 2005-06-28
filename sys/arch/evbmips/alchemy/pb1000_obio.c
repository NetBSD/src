/* $NetBSD: pb1000_obio.c,v 1.7 2005/06/28 18:29:59 drochner Exp $ */

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pb1000_obio.c,v 1.7 2005/06/28 18:29:59 drochner Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <mips/cache.h>
#include <mips/cpuregs.h>

#include <evbmips/alchemy/pb1000reg.h>
#include <evbmips/alchemy/pb1000_obiovar.h>

#include "locators.h"

static int	obio_match(struct device *, struct cfdata *, void *);
static void	obio_attach(struct device *, struct device *, void *);
static int	obio_submatch(struct device *, struct cfdata *,
			      const locdesc_t *, void *);
static int	obio_print(void *, const char *);

CFATTACH_DECL(obio, sizeof(struct device),
    obio_match, obio_attach, NULL, NULL);

/* There can be only one. */
int	obio_found;

struct obiodev {
	const char *od_name;
	bus_addr_t od_addr;
	int od_irq;
};

struct obiodev obiodevs[] = {
	{ "pcmcia",		-1,	-1 },
	{ "lcd",		-1,	-1 },
	{ "flash",		-1,	-1 },
	{ NULL,			0,	0 },
};

static int
obio_match(struct device *parent, struct cfdata *match, void *aux)
{

	if (obio_found)
		return (0);

	return (1);
}

static void
obio_attach(struct device *parent, struct device *self, void *aux)
{
	struct obio_attach_args oa;
	struct obiodev *od;

	obio_found = 1;
	printf("\n");

	for (od = obiodevs; od->od_name != NULL; od++) {
		oa.oba_name = od->od_name;
		oa.oba_addr = od->od_addr;
		oa.oba_irq = od->od_irq;
		(void) config_found_sm_loc(self, "obio", NULL, &oa, obio_print,
		    obio_submatch);
	}
}

static int
obio_submatch(struct device *parent, struct cfdata *cf,
	      const locdesc_t *ldesc, void *aux)
{
	struct obio_attach_args *oa = aux;

	if (cf->cf_loc[OBIOCF_ADDR] != OBIOCF_ADDR_DEFAULT &&
	    cf->cf_loc[OBIOCF_ADDR] != oa->oba_addr)
		return (0);

	return (config_match(parent, cf, aux));
}

static int
obio_print(void *aux, const char *pnp)
{
	struct obio_attach_args *oa = aux;

	if (pnp)
		aprint_normal("%s at %s", oa->oba_name, pnp);
	if (oa->oba_addr != OBIOCF_ADDR_DEFAULT)
		aprint_normal(" addr 0x%lx", oa->oba_addr);

	return (UNCONF);
}
