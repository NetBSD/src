/*	$NetBSD: espvar.h,v 1.10 2022/08/15 12:16:25 rin Exp $	*/

/*
 * Copyright (c) 1997 Allen Briggs.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Peter Galbavy.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

struct esp_softc {
	struct ncr53c9x_softc	sc_ncr53c9x;	/* glue to MI code */

	volatile uint8_t	*sc_reg;	/* the registers */
	volatile uint32_t	*sc_dreqreg;	/* DREQ register for DAFB */

	int		sc_active;		/* Pseudo-DMA state vars */
	int		sc_datain;
	size_t		sc_dmasize;
	uint8_t		**sc_dmaaddr;
	size_t		*sc_dmalen;
	int		sc_tc;			/* used in PIO */
	int		sc_pad;			/* only used in quick */

	/* for avdma */
	int		sc_rset;
	int		sc_ibuf_used;
	uint8_t		*sc_obuf;
	uint8_t		*sc_ibuf;
	bus_dma_tag_t	sc_dmat;
	bus_dmamap_t	sc_dmap;
};
