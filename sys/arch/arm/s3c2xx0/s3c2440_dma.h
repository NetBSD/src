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
/* This file implements a simple interface towards the S3C2440 DMA controller.
   At this point queueing of transfers is not supported. In other words, one
   has to be very careful that noone else is using a DMA channel when trying to use it.

   Only device<->memory transfers are supported at this time.

   Currently, the only usage of S3C2440 DMA is in the S3C2440 SD-Interface driver
   (s3c2440_sdi.c).
 */

#ifndef __S3C2440_DMA_H__
#define __S3C2440_DMA_H__

#include <sys/types.h>

void s3c2440_dma_init(void);
int  s3c2440_dma_intr(void *arg);

typedef enum {
	DMAC_BUS_TYPE_SYSTEM = 0,
	DMAC_BUS_TYPE_PERIPHERAL
} dmac_bus_type_t;

typedef enum {
	DMAC_SYNC_BUS_AUTO = 0,
	DMAC_SYNC_BUS_SYSTEM,
	DMAC_SYNC_BUS_PERIPHERAL
} dmac_sync_bus_t;

typedef enum {
	DMAC_XFER_WIDTH_8BIT = 0,
	DMAC_XFER_WIDTH_16BIT = 1,
	DMAC_XFER_WIDTH_32BIT = 2
} dmac_xfer_width_t;

typedef enum {
	DMAC_XFER_MODE_DEMAND = 0,
	DMAC_XFER_MODE_HANDSHAKE = 1
} dmac_xfer_mode_t;

typedef struct dmac_xfer_desc*	dmac_xfer_desc_t;
struct dmac_xfer_desc {
	dmac_bus_type_t		xd_bus_type;
	bool			xd_increment;
	u_int			xd_nsegs;
	bus_dma_segment_t	*xd_dma_segs;
};

typedef u_int	dmac_peripheral_t;
#define	DMAC_PERIPH_NONE	0	/* Software triggered transfer */
#define DMAC_PERIPH_XDREQ0	1
#define DMAC_PERIPH_XDREQ1	2
#define DMAC_PERIPH_UART0	3
#define DMAC_PERIPH_UART1	4
#define DMAC_PERIPH_UART2	5
#define DMAC_PERIPH_I2SSDO	6
#define DMAC_PERIPH_I2SSDI	7
#define DMAC_PERIPH_SDI		8
#define DMAC_PERIPH_SPI0	9
#define DMAC_PERIPH_SPI1	10
#define DMAC_PERIPH_PCMIN	11
#define DMAC_PERIPH_PCMOUT	12
#define DMAC_PERIPH_MICIN	13
#define DMAC_PERIPH_MICOUT	14
#define DMAC_PERIPH_TIMER	15
#define DMAC_PERIPH_USBEP1	16
#define DMAC_PERIPH_USBEP2	17
#define DMAC_PERIPH_USBEP3	18
#define DMAC_PERIPH_USBEP4	19
#define DMAC_N_PERIPH		20

typedef struct dmac_xfer*	dmac_xfer_t;
struct dmac_xfer {
	void			(*dx_done)(dmac_xfer_t, void*);
	void			*dx_cookie;
	dmac_peripheral_t	dx_peripheral; /* Controls trigger mechanism
						  and DMA channel to use */
	struct dmac_xfer_desc	dx_desc[2];
#define DMAC_DESC_SRC	0
#define DMAC_DESC_DST	1

	dmac_sync_bus_t		dx_sync_bus;

	dmac_xfer_width_t	dx_xfer_width;

	dmac_xfer_mode_t	dx_xfer_mode;
};

/* DMA API, inspired by pxa2x0_dmac.h */
dmac_xfer_t s3c2440_dmac_allocate_xfer(int);
void s3c2440_dmac_free_xfer(dmac_xfer_t);
int s3c2440_dmac_start_xfer(dmac_xfer_t);
void s3c2440_dmac_abort_xfer(dmac_xfer_t);
int s3c2440_dmac_wait_xfer(dmac_xfer_t, int);

#endif
