/*	$NetBSD: mainbus.c,v 1.20.28.1 2011/06/06 09:05:59 jruoho Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.20.28.1 2011/06/06 09:05:59 jruoho Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#define _M68K_BUS_DMA_PRIVATE
#include <machine/autoconf.h>

static int	mainbus_match(device_t, cfdata_t, void *);
static void	mainbus_attach(device_t, device_t, void *);
static int	mainbus_search(device_t, cfdata_t,
			       const int *ldesc, void *);

CFATTACH_DECL_NEW(mainbus, 0,
    mainbus_match, mainbus_attach, NULL, NULL);

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
mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	static int mainbus_matched = 0;

	/* Allow only one instance. */
	if (mainbus_matched)
		return (0);

	mainbus_matched = 1;
	return 1;
}

static void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args	mba;

	printf("\n");

	mba.mba_bst = MAC68K_BUS_SPACE_MEM;
	mba.mba_dmat = &mac68k_bus_dma_tag;

	/* Search for and attach children. */
	config_search_ia(mainbus_search, self, "mainbus", &mba);
}

static int
mainbus_search(device_t parent, cfdata_t cf,
	       const int *ldesc, void *aux)
{
	if (config_match(parent, cf, aux) > 0)
		config_attach(parent, cf, aux, NULL);
	return 0;
}
