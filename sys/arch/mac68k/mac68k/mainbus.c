/*	$NetBSD: mainbus.c,v 1.11 2002/04/13 17:49:41 briggs Exp $	*/

/*
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#define _M68K_BUS_DMA_PRIVATE
#include <machine/autoconf.h>

static int	mainbus_match __P((struct device *, struct cfdata *, void *));
static void	mainbus_attach __P((struct device *, struct device *, void *));
static int	mainbus_search __P((struct device *, struct cfdata *, void *));

struct cfattach mainbus_ca = {
	sizeof(struct device), mainbus_match, mainbus_attach
};

struct m68k_bus_dma_tag mac68k_bus_dma_tag = {
	NULL,					/* _cookie */

	0,					/* _boundary */

	_bus_dmamap_create,			/* _dmamap_create */
	_bus_dmamap_destroy,			/* _dmamap_destroy */
	_bus_dmamap_load_direct,		/* _dmamap_load */
	_bus_dmamap_load_mbuf_direct,		/* _dmamap_load_mbuf */
	_bus_dmamap_load_uio_direct,		/* _dmamap_load_uio */
	_bus_dmamap_load_raw_direct,		/* _dmamap_load_raw */
	_bus_dmamap_unload,			/* _dmamap_unload */
	_bus_dmamap_sync,			/* _dmamap_sync */
  
	_bus_dmamem_alloc,			/* _dmamem_alloc */
	_bus_dmamem_free,			/* _dmamem_free */
	_bus_dmamem_map,			/* _dmamem_map */
	_bus_dmamem_unmap,			/* _dmamem_unmap */
	_bus_dmamem_mmap			/* _dmamem_mmap */
};

static int
mainbus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	static int mainbus_matched = 0;

	/* Allow only one instance. */
	if (mainbus_matched)
		return (0);

	mainbus_matched = 1;
	return 1;
}

static void
mainbus_attach(parent, self, aux)
	struct device	*parent, *self;
	void		*aux;
{
	struct mainbus_attach_args	mba;

	printf("\n");

	mba.mba_bst = MAC68K_BUS_SPACE_MEM;
	mba.mba_dmat = &mac68k_bus_dma_tag;

	/* Search for and attach children. */
	config_search(mainbus_search, self, &mba);
}

static int
mainbus_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	if ((*cf->cf_attach->ca_match)(parent, cf, aux) > 0)
		config_attach(parent, cf, aux, NULL);
	return 0;
}
