/*	$NetBSD: espvar.h,v 1.6 1999/01/27 06:37:49 dbj Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

struct esp_softc {
	struct ncr53c9x_softc sc_ncr53c9x;	/* glue to MI code */
	struct nextdma_config sc_scsi_dma;
	bus_space_tag_t	sc_bst;				
	bus_space_handle_t sc_bsh;		/* the device registers */
	bus_dmamap_t  sc_dmamap;			/* i/o dma map */
	int sc_datain;								/* dma direction for map */
	caddr_t sc_slop_bgn_addr;			/* bytes to be fifo'd at beginning */
	caddr_t sc_slop_end_addr;			/* bytes to be fifo'd at end */
	int sc_slop_bgn_size;					/* # bytes to be fifo'd at beginning */
	int sc_slop_end_size;					/* # bytes to be fifo'd at end */
	int sc_dmamap_loaded;					/* 0=not loaded, 1=sc_dmamap, 2=sc_tail_dmamap */
	caddr_t *sc_dmaaddr;
	size_t  *sc_dmalen;
	size_t  sc_dmasize;
#define ESP_DMA_MAXTAIL 128
	caddr_t sc_tail;
	size_t  sc_tail_size;
	char sc_tailbuf[ESP_DMA_MAXTAIL+2*DMA_ENDALIGNMENT];
	bus_dmamap_t  sc_tail_dmamap;
};
