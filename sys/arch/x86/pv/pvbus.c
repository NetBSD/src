/*	$NetBSD: pvbus.c,v 1.2 2025/01/14 08:03:40 riastradh Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emile 'iMil' Heitor.
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

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <machine/bus_private.h>

#include <arch/x86/pv/pvvar.h>

static int _pv_dma_may_bounce(bus_dma_tag_t, bus_dmamap_t, int, int *);
static int pv_match(device_t, cfdata_t, void *);
static void pv_attach(device_t, device_t, void *);
static int pv_submatch(device_t, cfdata_t, const int *, void *);

struct x86_bus_dma_tag pvbus_bus_dma_tag = {
	._tag_needs_free	= 0,
	._bounce_thresh		= 0,
	._bounce_alloc_lo	= 0,
	._bounce_alloc_hi	= 0,
	._may_bounce		= _pv_dma_may_bounce,
};

CFATTACH_DECL_NEW(pv, sizeof(struct pv_softc),
    pv_match, pv_attach, NULL, NULL);

static int
_pv_dma_may_bounce(bus_dma_tag_t t, bus_dmamap_t map, int flags,
    int *cookieflagsp)
{

	if ((map->_dm_size / PAGE_SIZE) > map->_dm_segcnt)
		*cookieflagsp |= X86_DMA_MIGHT_NEED_BOUNCE;

	return 0;
}

static int
pv_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

static void
pv_attach(device_t parent, device_t self, void *aux)
{
	struct pv_attach_args pvaa;

	pvaa.pvaa_memt = x86_bus_space_mem;
	pvaa.pvaa_dmat = &pvbus_bus_dma_tag;

	aprint_naive("\n");
	aprint_normal("\n");

	config_found(self, &pvaa, NULL, CFARGS(.search = pv_submatch));
}

static int
pv_submatch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct pv_attach_args *pvaa = aux;

	if (config_probe(parent, cf, pvaa)) {
		config_attach(parent, cf, pvaa, NULL, CFARGS_NONE);
		return 0;
	}
	return 0;
}
