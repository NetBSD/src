/*	$NetBSD: obio.c,v 1.2 2003/05/23 05:42:05 briggs Exp $	*/

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
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

/*
 * On-board device autoconfiguration support for ADI BRH
 * evaluation boards.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <arm/xscale/beccreg.h>
#include <arm/xscale/beccvar.h>

#include <evbarm/adi_brh/brhreg.h>
#include <evbarm/adi_brh/obiovar.h>

#include "locators.h"

int	obio_match(struct device *, struct cfdata *, void *);
void	obio_attach(struct device *, struct device *, void *);

CFATTACH_DECL(obio, sizeof(struct device),
    obio_match, obio_attach, NULL, NULL);

int	obio_print(void *, const char *);
int	obio_search(struct device *, struct cfdata *, void *);

/* there can be only one */
int	obio_found;

int
obio_match(struct device *parent, struct cfdata *cf, void *aux)
{
#if 0
	struct mainbus_attach_args *ma = aux;
#endif

	if (obio_found)
		return (0);

#if 1
	/* XXX Shoot arch/arm/mainbus in the head. */
	return (1);
#else
	if (strcmp(cf->cf_name, ma->ma_name) == 0)
		return (1);

	return (0);
#endif
}

void
obio_attach(struct device *parent, struct device *self, void *aux)
{

	obio_found = 1;

	aprint_normal("\n");

	/*
	 * Attach all on-board devices as described in the kernel
	 * configuration file.
	 */
	config_search(obio_search, self, NULL);
}

int
obio_print(void *aux, const char *pnp)
{
	struct obio_attach_args *oba = aux;

	aprint_normal(" addr 0x%08lx", oba->oba_addr);
	if (oba->oba_irq != -1)
		aprint_normal(" irq %d", oba->oba_irq);

	return (UNCONF);
}

int
obio_search(struct device *parent, struct cfdata *cf, void *aux)
{
	struct obio_attach_args oba;

	oba.oba_st = &obio_bs_tag;

	oba.oba_addr = cf->cf_loc[OBIOCF_ADDR];
	oba.oba_irq = cf->cf_loc[OBIOCF_IRQ];

	if (config_match(parent, cf, &oba) > 0)
		config_attach(parent, cf, &oba, obio_print);

	return (0);
}
