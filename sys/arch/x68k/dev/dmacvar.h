/*	$NetBSD: dmacvar.h,v 1.3 2001/04/30 05:47:31 minoura Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Hitachi HD63450 (= Motorola MC68450) DMAC driver for x68k.
 */

#include <dev/ic/mc68450reg.h>
#include <machine/bus.h>

#define DMAC_MAPSIZE 64

typedef int (*dmac_intr_handler_t) __P((void*));

/*
 * Structure that describes a single transfer.
 */
struct dmac_channel_stat;
struct dmac_dma_xfer {
	struct dmac_channel_stat *dx_channel;
	bus_dmamap_t	dx_dmamap;	/* dmamap tag */
	bus_dma_tag_t	dx_tag;		/* dma tag for the transfer */
	int		dx_ocr;		/* direction */
	int		dx_scr;		/* SCR value */
	void		*dx_device;	/* (initial) device address */
	bus_dma_segment_t dx_seg;	/* b_d_s_t for the array chain */
	struct dmac_sg_array *dx_array;	/* DMAC array chain */
	int		dx_arraysize;	/* size of above */
	int		dx_done;
};

/*
 * Struct that holds the channel status.
 * Embedded in the device softc for each channel.
 */
struct dmac_channel_stat {
	int			ch_channel; /* channel number */
	char			ch_name[8]; /* user device name */
	bus_space_handle_t	ch_bht;	/* bus_space handle */
	int			ch_dcr;	/* device description */
	int			ch_ocr;	/* operation size, request mode */
	int			ch_normalv; /* normal interrupt vector */
	int			ch_errorv; /* error interrupt vector */
	dmac_intr_handler_t	ch_normal; /* normal interrupt handler */
	dmac_intr_handler_t	ch_error; /* error interrupt handler */
	void			*ch_normalarg;
	void			*ch_errorarg;
	struct dmac_dma_xfer	ch_xfer;
	struct dmac_sg_array	*ch_map; /* transfer map for arraychain mode */
	bus_dma_segment_t	ch_seg[1];
	struct device		*ch_softc; /* device softc link */
};

/*
 * DMAC softc
 */
struct dmac_softc {
	struct device		sc_dev;

	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bht;

	struct dmac_channel_stat sc_channels[DMAC_NCHAN];
};


#define DMAC_ADDR	0xe84000

#define DMAC_MAXSEGSZ	0xff00
#define DMAC_BOUNDARY	0

struct dmac_channel_stat *dmac_alloc_channel __P((struct device*, int, char*,
						  int,
						  dmac_intr_handler_t, void*,
						  int,
						  dmac_intr_handler_t, void*));
		/* ch, name, normalv, normal, errorv, error */
int dmac_free_channel __P((struct device*, int, void*));
		/* ch, channel */
struct dmac_dma_xfer *dmac_prepare_xfer __P((struct dmac_channel_stat*,
					     bus_dma_tag_t,
					     bus_dmamap_t,
					     int, int, void*));
	/* chan, dmat, map, dir, sequence, dar */
#define dmac_finish_xfer(xfer) free(xfer, M_DEVBUF)
int dmac_start_xfer __P((struct device*, struct dmac_dma_xfer*));
