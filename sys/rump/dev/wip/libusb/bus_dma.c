/*	$NetBSD: bus_dma.c,v 1.1 2009/10/01 21:46:31 pooka Exp $	*/

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <machine/bus.h>

/*
 * bus dma "implementation" for rump.  needs "a little" more work
 */

int
bus_dmamap_create(bus_dma_tag_t tag, bus_size_t sz, int flag, bus_size_t bsz,
	bus_size_t bsz2, int i, bus_dmamap_t *ptr)
{

	return 0;
}

void
bus_dmamap_destroy(bus_dma_tag_t tag, bus_dmamap_t map)
{

	panic("unimplemented %s", __func__);
}

int
bus_dmamap_load(bus_dma_tag_t t, bus_dmamap_t a, void *b, bus_size_t c,
	struct proc *d, int e)
{

	return 0;
}

void
bus_dmamap_unload(bus_dma_tag_t a, bus_dmamap_t b)
{

	panic("unimplemented %s", __func__);
}

void
bus_dmamap_sync(bus_dma_tag_t a, bus_dmamap_t b, bus_addr_t c,
	bus_size_t d, int e)
{

	panic("unimplemented %s", __func__);
}

int
bus_dmamem_alloc(bus_dma_tag_t tag, bus_size_t size, bus_size_t align,
	bus_size_t boundary, bus_dma_segment_t *segs, int nsegs,
	int *rsegs, int flags)
{

	*rsegs = nsegs;
	return 0;
}

void
bus_dmamem_free(bus_dma_tag_t a, bus_dma_segment_t *b, int c)
{

	panic("unimplemented %s", __func__);
}

int
bus_dmamem_map(bus_dma_tag_t tag, bus_dma_segment_t *segs, int nsegs,
                       size_t size, void **kvap, int flags)
{

	KASSERT(nsegs == 1);
	*kvap = kmem_alloc(size, KM_SLEEP);
	return 0;
}

void
bus_dmamem_unmap(bus_dma_tag_t a, void *kva, size_t b)
{

	panic("unimplemented %s", __func__);
}
