/*	$NetBSD: hyperv_common.c,v 1.6 2022/05/20 13:55:17 nonaka Exp $	*/

/*-
 * Copyright (c) 2009-2012,2016-2017 Microsoft Corp.
 * Copyright (c) 2012 NetApp Inc.
 * Copyright (c) 2012 Citrix Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
__KERNEL_RCSID(0, "$NetBSD: hyperv_common.c,v 1.6 2022/05/20 13:55:17 nonaka Exp $");

#include "hyperv.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/hyperv/hypervreg.h>
#include <dev/hyperv/hypervvar.h>

u_int hyperv_ver_major;
hyperv_tc64_t hyperv_tc64;

int		hyperv_nullop(void);
void		hyperv_voidop(void);
uint64_t	hyperv_hypercall_error(uint64_t, paddr_t, paddr_t);

__weak_alias(hyperv_hypercall, hyperv_hypercall_error);
__weak_alias(hyperv_hypercall_enabled, hyperv_nullop);
__weak_alias(hyperv_synic_supported, hyperv_nullop);
__weak_alias(hyperv_is_gen1, hyperv_nullop);
__weak_alias(hyperv_set_event_proc, hyperv_voidop);
__weak_alias(hyperv_set_message_proc, hyperv_voidop);
__weak_alias(hyperv_send_eom, hyperv_voidop);
__weak_alias(hyperv_intr, hyperv_voidop);
__weak_alias(hyperv_get_vcpuid, hyperv_nullop);
__weak_alias(vmbus_init_interrupts_md, hyperv_voidop);
__weak_alias(vmbus_deinit_interrupts_md, hyperv_voidop);
__weak_alias(vmbus_init_synic_md, hyperv_voidop);
__weak_alias(vmbus_deinit_synic_md, hyperv_voidop);

int
hyperv_nullop(void)
{
	return 0;
}

void
hyperv_voidop(void)
{
}

uint64_t
hyperv_hypercall_error(uint64_t control, paddr_t in_paddr, paddr_t out_paddr)
{
	return ~HYPERCALL_STATUS_SUCCESS;
}

uint64_t
hyperv_hypercall_post_message(paddr_t msg)
{

	return hyperv_hypercall(HYPERCALL_POST_MESSAGE, msg, 0);
}

uint64_t
hyperv_hypercall_signal_event(paddr_t monprm)
{

	return hyperv_hypercall(HYPERCALL_SIGNAL_EVENT, monprm, 0);
}

int
hyperv_guid2str(const struct hyperv_guid *guid, char *buf, size_t sz)
{
	const uint8_t *d = guid->hv_guid;

	return snprintf(buf, sz, "%02x%02x%02x%02x-"
	    "%02x%02x-%02x%02x-%02x%02x-"
	    "%02x%02x%02x%02x%02x%02x",
	    d[3], d[2], d[1], d[0],
	    d[5], d[4], d[7], d[6], d[8], d[9],
	    d[10], d[11], d[12], d[13], d[14], d[15]);
}

/*
 * Hyper-V bus_dma utilities.
 */
void *
hyperv_dma_alloc(bus_dma_tag_t dmat, struct hyperv_dma *dma, bus_size_t size,
    bus_size_t alignment, bus_size_t boundary, int nsegs)
{
	int rseg, error;

	KASSERT(dma != NULL);
	KASSERT(dma->segs == NULL);
	KASSERT(nsegs > 0);

	dma->segs = kmem_zalloc(sizeof(*dma->segs) * nsegs, KM_SLEEP);
	dma->nsegs = nsegs;

	error = bus_dmamem_alloc(dmat, size, alignment, boundary, dma->segs,
	    nsegs, &rseg, BUS_DMA_WAITOK);
	if (error) {
		printf("%s: bus_dmamem_alloc failed: error=%d\n",
		    __func__, error);
		goto fail1;
	}
	error = bus_dmamem_map(dmat, dma->segs, rseg, size, &dma->addr,
	    BUS_DMA_WAITOK);
	if (error) {
		printf("%s: bus_dmamem_map failed: error=%d\n",
		    __func__, error);
		goto fail2;
	}
	error = bus_dmamap_create(dmat, size, rseg, size, boundary,
	    BUS_DMA_WAITOK, &dma->map);
	if (error) {
		printf("%s: bus_dmamap_create failed: error=%d\n",
		    __func__, error);
		goto fail3;
	}
	error = bus_dmamap_load(dmat, dma->map, dma->addr, size, NULL,
	    BUS_DMA_READ | BUS_DMA_WRITE | BUS_DMA_WAITOK);
	if (error) {
		printf("%s: bus_dmamap_load failed: error=%d\n",
		    __func__, error);
		goto fail4;
	}

	memset(dma->addr, 0, dma->map->dm_mapsize);
	bus_dmamap_sync(dmat, dma->map, 0, dma->map->dm_mapsize,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	return dma->addr;

fail4:	bus_dmamap_destroy(dmat, dma->map);
fail3:	bus_dmamem_unmap(dmat, dma->addr, size);
	dma->addr = NULL;
fail2:	bus_dmamem_free(dmat, dma->segs, rseg);
fail1:	kmem_free(dma->segs, sizeof(*dma->segs) * nsegs);
	dma->segs = NULL;
	dma->nsegs = 0;
	return NULL;
}

void
hyperv_dma_free(bus_dma_tag_t dmat, struct hyperv_dma *dma)
{
	bus_size_t size = dma->map->dm_mapsize;
	int rsegs = dma->map->dm_nsegs;

	bus_dmamap_unload(dmat, dma->map);
	bus_dmamap_destroy(dmat, dma->map);
	bus_dmamem_unmap(dmat, dma->addr, size);
	dma->addr = NULL;
	bus_dmamem_free(dmat, dma->segs, rsegs);
	kmem_free(dma->segs, sizeof(*dma->segs) * dma->nsegs);
	dma->segs = NULL;
	dma->nsegs = 0;
}
