/*	$Id: at91pdc.c,v 1.1.16.1 2008/09/28 10:39:48 mjf Exp $	*/

#include <sys/types.h>
#include <machine/bus.h>
#include <arm/at91/at91pdcvar.h>

int at91pdc_alloc_fifo(bus_dma_tag_t dmat, at91pdc_fifo_t *fifo, int size,
		       int flags)
{
	bus_dma_segment_t segs;
	int rsegs;
	int err;
	int f;

	memset(fifo, 0, sizeof(*fifo));

	/* allocate map: */
	f = flags & (BUS_DMA_BUS1 | BUS_DMA_BUS2 | BUS_DMA_BUS3 | BUS_DMA_BUS4);
	err = bus_dmamap_create(dmat, size, 1, size, 0,
				BUS_DMA_WAITOK | f,
				&fifo->f_dmamap);
	if (err)
		goto fail_0;

	/* allocate DMA safe memory: */
	f |= flags & BUS_DMA_STREAMING;
	err = bus_dmamem_alloc(dmat, size, 0, size, &segs, 1, &rsegs,
			       BUS_DMA_WAITOK | f);
	if (err)
		goto fail_1;

	/* allocate virtual memory: */
	f |= flags & (BUS_DMA_COHERENT | BUS_DMA_NOCACHE);
	err = bus_dmamem_map(dmat, &segs, 1, size, &fifo->f_buf,
			     BUS_DMA_WAITOK | f);
	if (err)
		goto fail_2;

	/* connect physical to virtual memory: */
	f |= flags & (BUS_DMA_READ | BUS_DMA_WRITE);
	f &= ~(BUS_DMA_COHERENT | BUS_DMA_NOCACHE);
	err = bus_dmamap_load(dmat, fifo->f_dmamap, fifo->f_buf, size, NULL,
			      BUS_DMA_NOWAIT | f);
	if (err)
		goto fail_3;

	/* initialize rest of the structure: */
	fifo->f_buf_size = size;
	fifo->f_ndx = fifo->f_length = 0;

	fifo->f_buf_addr = fifo->f_dmamap->dm_segs[0].ds_addr;
	fifo->f_pdc_rd_ndx = fifo->f_pdc_wr_ndx = 0;
	fifo->f_pdc_space = fifo->f_buf_size;

	return 0;
fail_3:
	bus_dmamem_unmap(dmat, fifo->f_buf, size);  
fail_2:
	bus_dmamem_free(dmat, &segs, rsegs);
fail_1:
	bus_dmamap_destroy(dmat, fifo->f_dmamap);  
fail_0:
	return err;
}
