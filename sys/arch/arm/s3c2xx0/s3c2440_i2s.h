/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Fleischer <paul@xpg.dk>
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
#ifndef _S3C2440_I2S_H_
#define	_S3C2440_I2S_H_

struct s3c2440_i2s_attach_args {
	void		*i2sa_handle;	/* Handle of the S3C2440 I2S Controller */
};

/* It should be possible to allocate most buffer sizes within
   2 segments.
   However, worst case actually is PHYSMEM / PAGE_SIZE, which seems to
   be 65535/4096 = 16 as default for most architectures.
   This does seem like a too pesimistic number.
 */
#define S3C2440_I2S_BUF_MAX_SEGS 2

struct s3c2440_i2s_buf {
	void			*i2b_parent; /* I2S handle */

	/* Size of the buffer */
	int			i2b_size;

	/* Kernelspace virtual address of the buffer */
	void			*i2b_addr;

	/* Physical segments holding the buffer */
	bus_dma_segment_t	i2b_segs[S3C2440_I2S_BUF_MAX_SEGS];

	/* Number of segments in use */
	int			i2b_nsegs;

	/* Map used for transfers DMA transfers */
	bus_dmamap_t		i2b_dmamap;

	/* DMA transfer structure */
	dmac_xfer_t		i2b_xfer;

	/* Callback */
	void			(*i2b_cb)(void*);
	void			*i2b_cb_cookie;
};

typedef struct s3c2440_i2s_buf* s3c2440_i2s_buf_t;


void s3c2440_i2s_set_intr_lock(void *handle, kmutex_t *sc_intr_lock);

#define S3C2440_I2S_NONE	(0)
#define S3C2440_I2S_TRANSMIT	(1<<0)
#define S3C2440_I2S_RECEIVE	(1<<1)
#define S3C2440_I2S_BOTH	(S3C2440_I2S_TRANSMIT | S3C2440_I2S_RECEIVE)
/* Set transfer direction of the I2S bus */
void s3c2440_i2s_set_direction(void *handle, int direction);

/* Set the I2S clocks (master and serial) from the given sample rate */
void s3c2440_i2s_set_sample_rate(void *handle, int sample_rate);
void s3c2440_i2s_set_sample_width(void *handle, int width);

#define S3C2440_I2S_BUS_MSB 1
#define S3C2440_I2S_BUS_I2S 2
void s3c2440_i2s_set_bus_format(void *handle, int format);

/* Enable the I2S bus */
int s3c2440_i2s_commit(void *handle);

/* Disable the I2S bus */
int s3c2440_i2s_disable(void *handle);

int s3c2440_i2s_get_master_clock(void *handle);
int s3c2440_i2s_get_serial_clock(void *handle);

/* Allocate a I2S buffer. */
int s3c2440_i2s_alloc(void *, int, size_t, int, s3c2440_i2s_buf_t *);

/* Free an I2S buffer. */
void s3c2440_i2s_free(s3c2440_i2s_buf_t);

/* Write data (from the I2S buffer) onto the I2S bus */
int s3c2440_i2s_output(s3c2440_i2s_buf_t, void *, int, void (*)(void*), void*);
int s3c2440_i2s_input(s3c2440_i2s_buf_t, void *, int, void (*)(void*), void*);

int s3c2440_i2s_halt_output(s3c2440_i2s_buf_t);

#endif

