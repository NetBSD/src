/*	$Id: at91pdcvar.h,v 1.2.32.1 2013/01/16 05:32:45 yamt Exp $	*/

#ifndef	_AT91PDCVAR_H_
#define	_AT91PDCVAR_H_

#include <arm/at91/at91pdcreg.h>
#include <sys/param.h>

#if	UNTESTED

typedef struct at91pdc_buf {
	void			*buf_arg;	/* argument (mbuf or other data) */
	int			buf_len;	/* length of data sent / recv */
	bus_dmamap_t		buf_dmamap;	/* dma map */
} at91pdc_buf_t;

typedef struct at91pdc_queue {
	at91pdc_buf_t		q_buf[2];	/* two buffers */
	unsigned		q_ndx;		/* buffer being sent (if q_len > 0) */
	unsigned		q_len;		/* number of buffers being used (<= 2) */
} at91pdc_queue_t;

#define	AT91PDC_UNLOAD_BUF(_pq, _dmat, _sync, _free) do {	\
	unsigned	_i = (_pq)->q_ndx % 2;			\
	at91pdc_buf_t	*_buf = &(_pq)->q_buf[i];		\
	void		*_arg = _buf->buf_arg;			\
								\
	if (_sync) {						\
		bus_dmamap_sync((_dmat), buf->buf_dmamap, 0,	\
				buf->buf_len, (_sync));		\
	}							\
	bus_dmamap_unload((_dmat), buf->buf_dmamap);		\
	buf->buf_arg = 0; buf->buf_len = 0;			\
								\
	(_pq)->q_ndx = i ^ 1;					\
	(_pq)->q_len--;						\
								\
	(_free)(_arg);						\
} while (/*CONSTCOND*/0)

#define	AT91PDC_UNLOAD_QUEUE(_pq, _dmat, _sync, _free, _idle)	\
do {								\
	if ((_pq)->q_len > 1)					\
		AT91PDC_UNLOAD_BUF(_pq, _dmat, _sync, _free);	\
	if ((_idle) && (_pq)->q_len > 0)			\
		AT91PDC_UNLOAD_BUF(_pq, _dmat, _sync, _free);	\
} while (/*CONSTCOND*/0)

#endif	/* UNTESTED */


typedef struct at91pdc_fifo {
	bus_dmamap_t	f_dmamap;	/* DMA map			*/
	void		*f_buf;		/* buffer address		*/
	int		f_buf_size;	/* size of the fifo		*/
	int		f_ndx;		/* current read/write index	*/
	int		f_length;	/* number of bytes in fifo	*/

	bus_addr_t	f_buf_addr;	/* buffer bus addr		*/
	int		f_pdc_rd_ndx;	/* PDC read index		*/
	int		f_pdc_wr_ndx;	/* PDC write index		*/
	int		f_pdc_space;	/* number of bytes allocated for pdc */
} at91pdc_fifo_t;

static __inline int AT91PDC_FIFO_EMPTY(at91pdc_fifo_t *fifo)
{
	return fifo->f_length == 0;
}

static __inline int AT91PDC_FIFO_FULL(at91pdc_fifo_t *fifo)
{
	return fifo->f_length >= fifo->f_buf_size;
}

static __inline int AT91PDC_FIFO_SPACE(at91pdc_fifo_t *fifo)
{
	return fifo->f_buf_size - fifo->f_length;
}


static __inline void AT91PDC_RESET_FIFO(bus_space_tag_t iot,
					 bus_space_handle_t ioh,
					 bus_dma_tag_t dmat,
					 uint offset,
					 at91pdc_fifo_t *fifo,
					 int rw)
{
	fifo->f_ndx = fifo->f_length = 0;

	fifo->f_pdc_rd_ndx = fifo->f_pdc_wr_ndx = 0;
	fifo->f_pdc_space = fifo->f_buf_size;

	if (!rw) {
		bus_space_write_4(iot, ioh, offset + PDC_RNCR, 0);
		bus_space_write_4(iot, ioh, offset + PDC_RCR, 0);
		bus_space_write_4(iot, ioh, offset + PDC_RNPR, fifo->f_buf_addr);
		bus_space_write_4(iot, ioh, offset + PDC_RPR, fifo->f_buf_addr);
	} else {
		bus_space_write_4(iot, ioh, offset + PDC_TNCR, 0);
		bus_space_write_4(iot, ioh, offset + PDC_TCR, 0);
		bus_space_write_4(iot, ioh, offset + PDC_TNPR, fifo->f_buf_addr);
		bus_space_write_4(iot, ioh, offset + PDC_TPR, fifo->f_buf_addr);
	}
}

static __inline int AT91PDC_FIFO_PREREAD(bus_space_tag_t iot,
					  bus_space_handle_t ioh,
					  bus_dma_tag_t dmat,
					  uint offset,
					  at91pdc_fifo_t *fifo,
					  uint chunk_size)
{
	int al;
	int ret = 1;

	/* then check if we can queue new block */
	if (bus_space_read_4(iot, ioh, offset + PDC_RNCR))
		goto get_out;
	if (fifo->f_pdc_space < chunk_size) {
		ret = 0;
		goto get_out;
	}
	/* fifo has enough space left for next chunk! */
	bus_dmamap_sync(dmat,
			fifo->f_dmamap,
			fifo->f_pdc_wr_ndx,
			chunk_size,
			BUS_DMASYNC_PREREAD);
	bus_space_write_4(iot, ioh, offset + PDC_RNPR, fifo->f_buf_addr + fifo->f_pdc_wr_ndx);
	bus_space_write_4(iot, ioh, offset + PDC_RNCR, chunk_size);
	if ((fifo->f_pdc_wr_ndx += chunk_size) >= fifo->f_buf_size)
		fifo->f_pdc_wr_ndx = 0;
	fifo->f_pdc_space -= chunk_size;
get_out:
	/* now check if we need to re-synchronize last read chunk too */
	al = fifo->f_pdc_rd_ndx % chunk_size;
	if (al) {
		bus_dmamap_sync(dmat,
				fifo->f_dmamap,
				fifo->f_pdc_rd_ndx,
				chunk_size - al,
				BUS_DMASYNC_PREREAD);
	}
	return ret;
}

static __inline void AT91PDC_FIFO_POSTREAD(bus_space_tag_t iot,
					    bus_space_handle_t ioh,
					    bus_dma_tag_t dmat,
					    uint offset,
					    at91pdc_fifo_t *fifo)
{
	uint32_t	pdc_ptr = bus_space_read_4(iot, ioh, offset + PDC_RPR);
	int32_t		cc = pdc_ptr - fifo->f_buf_addr - fifo->f_pdc_rd_ndx;

	/* handle fifo wrapping: */
	if (cc < 0) {
		cc = fifo->f_buf_size - fifo->f_pdc_rd_ndx;
		if (cc > 0) {
			bus_dmamap_sync(dmat, fifo->f_dmamap, fifo->f_pdc_rd_ndx, cc,
					BUS_DMASYNC_POSTREAD);
			fifo->f_length += cc;
			fifo->f_pdc_rd_ndx += cc;
		}
		fifo->f_pdc_rd_ndx = 0;
		cc = pdc_ptr - fifo->f_buf_addr;
	}

	if (cc > 0) {
		/* data has been received! */
		bus_dmamap_sync(dmat, fifo->f_dmamap, fifo->f_pdc_rd_ndx, cc,
				BUS_DMASYNC_POSTREAD);
		fifo->f_length += cc;
		fifo->f_pdc_rd_ndx += cc;
	}
}

static __inline void *AT91PDC_FIFO_RDPTR(at91pdc_fifo_t *fifo, int *num_bytes)
{
	if (fifo->f_length <= 0) {
		return NULL;
	}
	int contig_bytes = fifo->f_buf_size - fifo->f_ndx;
	if (contig_bytes > fifo->f_length)
		contig_bytes = fifo->f_length;
	*num_bytes = contig_bytes;
	return (void*)((uintptr_t)fifo->f_buf + fifo->f_ndx);
}

static __inline void AT91PDC_FIFO_READ(at91pdc_fifo_t *fifo, int bytes_read)
{
	if (bytes_read > fifo->f_length)
		bytes_read = fifo->f_length;
	int contig_bytes = fifo->f_buf_size - fifo->f_ndx;
	fifo->f_length -= bytes_read;
	fifo->f_pdc_space += bytes_read;
	if (bytes_read < contig_bytes)
		fifo->f_ndx += bytes_read;
	else
		fifo->f_ndx = bytes_read - contig_bytes;
}

static __inline int AT91PDC_FIFO_PREWRITE(bus_space_tag_t iot,
					   bus_space_handle_t ioh,
					   bus_dma_tag_t dmat,
					   uint offset,
					   at91pdc_fifo_t *fifo,
					   uint max_chunk_size)
{
	if (bus_space_read_4(iot, ioh, offset + PDC_TNCR) != 0)
		return 1;
	int len = fifo->f_buf_size - fifo->f_pdc_rd_ndx;
	int max_len = fifo->f_length - (fifo->f_buf_size - fifo->f_pdc_space);
	if (len > max_len)
		len = max_len;
	if (len > max_chunk_size)
		len = max_chunk_size;
	if (len > fifo->f_pdc_space)
		panic("%s: len %d > pdc_space (f_length=%d space=%d size=%d)",
		      __FUNCTION__, len, fifo->f_length, fifo->f_pdc_space, fifo->f_buf_size);
	if (len == 0)
		return 0;
	if (len < 0)
		panic("%s: len < 0 (f_length=%d space=%d size=%d)",
		      __FUNCTION__, fifo->f_length, fifo->f_pdc_space, fifo->f_buf_size);

	/* there's something to write */
	bus_dmamap_sync(dmat,
			fifo->f_dmamap,
			fifo->f_pdc_rd_ndx,
			len,
			BUS_DMASYNC_PREWRITE);
	bus_space_write_4(iot, ioh, offset + PDC_TNPR, fifo->f_buf_addr + fifo->f_pdc_rd_ndx);
	bus_space_write_4(iot, ioh, offset + PDC_TNCR, len);
	if ((fifo->f_pdc_rd_ndx += len) >= fifo->f_buf_size)
		fifo->f_pdc_rd_ndx = 0;
	fifo->f_pdc_space -= len;

	return 1;
}

static __inline void AT91PDC_FIFO_POSTWRITE(bus_space_tag_t iot,
					     bus_space_handle_t ioh,
					     bus_dma_tag_t dmat,
					     uint offset,
					     at91pdc_fifo_t *fifo)
{
	uint32_t	pdc_ptr = bus_space_read_4(iot, ioh, offset + PDC_TPR);
	int32_t		cc = pdc_ptr - fifo->f_buf_addr - fifo->f_pdc_wr_ndx;

	/* handle fifo wrapping: */
	if (cc < 0) {
		cc = fifo->f_buf_size - fifo->f_pdc_wr_ndx;
		if (cc > 0) {
			bus_dmamap_sync(dmat, fifo->f_dmamap, fifo->f_pdc_wr_ndx, cc,
					BUS_DMASYNC_POSTWRITE);
			fifo->f_length -= cc;
			fifo->f_pdc_space += cc;
		}
		fifo->f_pdc_wr_ndx = 0;
		cc = pdc_ptr - fifo->f_buf_addr;
	}

	if (cc > 0) {
		/* data has been sent! */
		bus_dmamap_sync(dmat, fifo->f_dmamap, fifo->f_pdc_wr_ndx, cc,
				BUS_DMASYNC_POSTWRITE);
		fifo->f_length -= cc;
		fifo->f_pdc_space += cc;
		fifo->f_pdc_wr_ndx += cc;
	}
}

static __inline void *AT91PDC_FIFO_WRPTR(at91pdc_fifo_t *fifo, int *max_bytes)
{
	int space = fifo->f_buf_size - fifo->f_length;
	if (space <= 0)
		return NULL;
	int contig_bytes = fifo->f_buf_size - fifo->f_ndx;
	if (contig_bytes > space)
		contig_bytes = space;
	*max_bytes = contig_bytes;
	return (void*)((uintptr_t)fifo->f_buf + fifo->f_ndx);
}

static __inline void AT91PDC_FIFO_WRITTEN(at91pdc_fifo_t *fifo, int bytes_written)
{
	if (bytes_written > (fifo->f_buf_size - fifo->f_length))
		bytes_written = (fifo->f_buf_size - fifo->f_length);
	int contig_bytes = fifo->f_buf_size - fifo->f_ndx;
	fifo->f_length += bytes_written;
	if (bytes_written < contig_bytes)
		fifo->f_ndx += bytes_written;
	else
		fifo->f_ndx = bytes_written - contig_bytes;
}

int at91pdc_alloc_fifo(bus_dma_tag_t dmat, at91pdc_fifo_t *fifo, int size, int flags);

#endif	// !_AT91PDCVAR_H_
