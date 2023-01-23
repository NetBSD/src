/*	$NetBSD: sivar.h,v 1.12 2023/01/23 22:16:44 andvar Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, David Jones, and Gordon W. Ross.
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

/*
 * This file defines the interface between si.c and
 * the bus-specific files: si_obio.c, si_vme.c
 */

/*
 * Transfers smaller than this are done using PIO
 * (on assumption they're not worth DMA overhead)
 */
#define	MIN_DMA_LEN 128

/*
 * Transfers larger than 65535 bytes need to be split-up.
 * (Some of the FIFO logic has only 16 bits counters.)
 * Make the size an integer multiple of the page size
 * to avoid buf/cluster remap problems.  (paranoid?)
 */
#define	MAX_DMA_LEN 0xE000

/*
 * This structure is used to keep track of mapped DMA requests.
 */
struct si_dma_handle {
	int 		dh_flags;
#define	SIDH_BUSY	1		/* This DH is in use */
#define	SIDH_OUT	2		/* DMA does data out (write) */
	vaddr_t		dh_dmaaddr;	/* VA of buffer in DVMA space */
	vsize_t		dh_dmalen;	/* Length of KVA mapping. */
};

/*
 * The first structure member has to be the ncr5380_softc
 * so we can just cast to go back and fourth between them.
 */
struct si_softc {
	struct ncr5380_softc	ncr_sc;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	volatile struct si_regs	*sc_regs;
	bus_dma_tag_t		sc_dmat;
	bus_dmamap_t		sc_dmap;
	int		sc_adapter_type;
	int		sc_adapter_iv_am;	/* int. vec + address modifier */
	int 	sc_options;			/* options for this instance */
	int 	sc_reqlen;  		/* requested transfer length */
	struct si_dma_handle *sc_dma;
	/* DMA command block for the OBIO controller. */
	void *sc_dmacmd;
};

/* Options for disconnect/reselect, DMA, and interrupts. */
#define SI_NO_DISCONNECT    0xff
#define SI_NO_PARITY_CHK  0xff00
#define SI_FORCE_POLLING 0x10000
#define SI_DISABLE_DMA   0x20000
/* The options are taken from the config file (PR#1929) */

extern int si_debug;

void si_attach(struct si_softc *);
int  si_intr(void *);

void si_dma_alloc(struct ncr5380_softc *);
void si_dma_free(struct ncr5380_softc *);
void si_dma_poll(struct ncr5380_softc *);
