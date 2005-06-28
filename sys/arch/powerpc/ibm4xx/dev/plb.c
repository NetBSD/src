/* $NetBSD: plb.c,v 1.13 2005/06/28 18:30:00 drochner Exp $ */

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
__KERNEL_RCSID(0, "$NetBSD: plb.c,v 1.13 2005/06/28 18:30:00 drochner Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#define _POWERPC_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <powerpc/ibm4xx/dev/plbvar.h>
#include <powerpc/ibm4xx/ibm405gp.h>

/*
 * The devices that attach to the processor local bus on the 405GP CPU.
 */
const struct plb_dev plb_devs [] = {
	{ "cpu", },
	{ "ecc", },
	{ "opb", },
	{ "pchb", },
	{ NULL }
};

static int	plb_match(struct device *, struct cfdata *, void *);
static void	plb_attach(struct device *, struct device *, void *);
static int	plb_print(void *, const char *);

CFATTACH_DECL(plb, sizeof(struct device),
    plb_match, plb_attach, NULL, NULL);

/*
 * "generic" DMA struct, nothing special.
 */
struct powerpc_bus_dma_tag ibm4xx_default_bus_dma_tag = {
	0,			/* _bounce_thresh */
	_bus_dmamap_create, 
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
	_bus_dma_phys_to_bus_mem_generic,
	_bus_dma_bus_mem_to_phys_generic,
};

/*
 * Probe for the plb; always succeeds.
 */
static int
plb_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (1);
}

/*
 * Attach the processor local bus.
 */
static void
plb_attach(struct device *parent, struct device *self, void *aux)
{
	struct plb_attach_args paa;
	struct plb_dev *local_plb_devs = aux;
	int i;

	printf("\n");

	for (i = 0; plb_devs[i].plb_name != NULL; i++) {
		paa.plb_name = plb_devs[i].plb_name;
		paa.plb_dmat = &ibm4xx_default_bus_dma_tag;
		paa.plb_irq = PLBCF_IRQ_DEFAULT;

		(void) config_found_ia(self, "plb", &paa, plb_print);
	}

	while (local_plb_devs && local_plb_devs->plb_name != NULL) {
		paa.plb_name = local_plb_devs->plb_name;
		paa.plb_dmat = &ibm4xx_default_bus_dma_tag;
		paa.plb_irq = PLBCF_IRQ_DEFAULT;

		(void) config_found_ia(self, "plb", &paa, plb_print);
		local_plb_devs++;
	}
}

static int
plb_print(void *aux, const char *pnp)
{
	struct plb_attach_args *paa = aux;

	if (pnp)
		aprint_normal("%s at %s", paa->plb_name, pnp);
	if (paa->plb_irq != PLBCF_IRQ_DEFAULT)
		aprint_normal(" irq %d", paa->plb_irq);

	return (UNCONF);
}
