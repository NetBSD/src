/*	$NetBSD: if_xevar.h,v 1.4.128.1 2010/04/30 14:39:40 uebayasi Exp $	*/
/*
 * Copyright (c) 1998 Darrin B. Jewell
 * All rights reserved.
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

struct xe_softc {
	struct mb8795_softc	sc_mb8795;	/* glue to MI code */

	bus_space_tag_t		sc_bst;		/* bus space tag */
	bus_space_handle_t	sc_bsh;		/* bus space handle */

	struct nextdma_softc	*sc_rxdma;
	struct nextdma_softc	*sc_txdma;

	bus_dmamap_t		sc_tx_dmamap;	/* should we have multiple of these? */
	struct mbuf		*sc_tx_mb_head;	/* pointer to data for this command */
	int sc_tx_loaded;
	u_char			*sc_txbuf;	/* to solve alignment problems, we
						 * copy the mbuf into this buffer before
						 * trying to DMA it */

	bus_dmamap_t		sc_rx_dmamap[MB8795_NRXBUFS];
	struct mbuf		*sc_rx_mb_head[MB8795_NRXBUFS];
	int			sc_rx_loaded_idx;
	int			sc_rx_completed_idx;
	int			sc_rx_handled_idx;
};
