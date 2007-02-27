/*	$NetBSD: pxa2x0_dmac.h,v 1.2.26.1 2007/02/27 16:49:40 yamt Exp $	*/

/*
 * Copyright (c) 2003, 2005 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
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

#ifndef __PXA2X0_DMAC_H
#define __PXA2X0_DMAC_H

typedef u_int dmac_peripheral_t;
#define DMAC_PERIPH_NONE    	(~0)
#define DMAC_PERIPH_DREQ0    	0
#define DMAC_PERIPH_DREQ1     	1
#define DMAC_PERIPH_I2SRX     	2
#define DMAC_PERIPH_I2STX     	3
#define DMAC_PERIPH_BTUARTRX	4
#define DMAC_PERIPH_BTUARTTX	5
#define DMAC_PERIPH_FFUARTRX	6
#define DMAC_PERIPH_FFUARTTX	7
#define DMAC_PERIPH_AC97MIC	8
#define DMAC_PERIPH_AC97MODEMRX	9
#define DMAC_PERIPH_AC97MODEMTX	10
#define DMAC_PERIPH_AC97AUDIORX 11
#define DMAC_PERIPH_AC97AUDIOTX 12
#define DMAC_PERIPH_SSPRX	13
#define DMAC_PERIPH_SSPTX	14
#define DMAC_PERIPH_FICPRX	17
#define DMAC_PERIPH_FICPTX	18
#define DMAC_PERIPH_STUARTRX	19
#define DMAC_PERIPH_STUARTTX	20
#define DMAC_PERIPH_MMCRX	21
#define DMAC_PERIPH_MMCTX	22
#define DMAC_PERIPH_USBEP(n)	(24+(n))   /* for endpoint 1..4,6..9,11..14 */
#define	DMAC_N_PERIPH		40

typedef enum {
#define	DMAC_PRIORITY_NORMAL	DMAC_PRIORITY_LOW
	DMAC_PRIORITY_LOW = 0,
	DMAC_PRIORITY_MED,
	DMAC_PRIORITY_HIGH
} dmac_priority_t;

typedef enum {
	DMAC_FLOW_CTRL_NONE,
	DMAC_FLOW_CTRL_SRC,
	DMAC_FLOW_CTRL_DEST
} dmac_flow_ctrl_t;

typedef enum {
	DMAC_DEV_WIDTH_DEFAULT = 0,
	DMAC_DEV_WIDTH_1,
	DMAC_DEV_WIDTH_2,
	DMAC_DEV_WIDTH_4
} dmac_dev_width_t;

typedef enum {
	DMAC_BURST_SIZE_8 = 1,
	DMAC_BURST_SIZE_16,
	DMAC_BURST_SIZE_32
} dmac_burst_size_t;

struct dmac_xfer_desc {
	/*
	 * Hold the source/destination address.
	 * Note that if this is TRUE, then xd_nsegs must be at least '1',
	 * and the first dma segment in xd_dma_segs must have a non-zero
	 * ds_len field.
	 */
	bool xd_addr_hold;

	u_int xd_nsegs;
	bus_dma_segment_t *xd_dma_segs;
};

struct dmac_xfer {
	/*
	 * The following fields must be initialised by clients of the
	 * DMAC driver.
	 * The DMAC driver treats all these fields as Read-Only.
	 */

	/* Client-specific cookie for this request */
	void *dx_cookie;

	/* Client function to invoke when the transfer completes */
	void (*dx_done)(struct dmac_xfer *, int);

	/* Priority to assign to the transfer */
	dmac_priority_t dx_priority;

	/* Peripheral device involved in the transfer */
	dmac_peripheral_t dx_peripheral;

	/* Flow Control */
	dmac_flow_ctrl_t dx_flow;

	/* Device width */
	dmac_dev_width_t dx_dev_width;

	/* Burst size */
	dmac_burst_size_t dx_burst_size;

	/* Loop notification period */
	size_t dx_loop_notify;
/*
 * Initialise dx_loop_notify to the following value if you do not
 * need to use the looping facility of the DMA engine. (Looping is
 * primarily for use by the AC97 driver, but may be of interest to
 * other drivers...)
 */
#define	DMAC_DONT_LOOP	0

	/* Source/Destination descriptors */
	struct dmac_xfer_desc dx_desc[2];
#define DMAC_DESC_SRC 0
#define DMAC_DESC_DST 1
};

extern struct dmac_xfer *pxa2x0_dmac_allocate_xfer(int);
extern void pxa2x0_dmac_free_xfer(struct dmac_xfer *);
extern int pxa2x0_dmac_start_xfer(struct dmac_xfer *);
extern void pxa2x0_dmac_abort_xfer(struct dmac_xfer *);

#endif /* __PXA2X0_DMAC_H */
