/*	$NetBSD: flscvar.h,v 1.5 1999/09/25 21:47:11 is Exp $	*/

/*
 * Copyright (c) 1997 Michael L. Hitch.
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
 *	This product includes software developed for the NetBSD Project
 *	by Michael L. Hitch.
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

struct flsc_softc {
	struct ncr53c9x_softc	sc_ncr53c9x;	/* glue to MI code */

	struct	isr		 sc_isr;	/* Interrupt chain struct */

	volatile u_char *sc_reg;		/* the registers */
	volatile u_char *sc_dmabase;

	int		sc_active;		/* Pseudo-DMA state vars */
	int		sc_piomode;
	int		sc_datain;
	int		sc_tc;
	size_t		sc_dmasize;
	size_t		sc_dmatrans;
	char		**sc_dmaaddr;
	size_t		*sc_pdmalen;
	paddr_t		sc_pa;

	char		*sc_alignbuf;
	u_char		sc_pad1[2];		/* XXX */
	u_char		sc_unalignbuf[256];
	u_char		sc_pad2[16];
	u_char		sc_hardbits;
	u_char		sc_portbits;
	u_char		sc_csr;
	u_char		sc_xfr_align;

};

#define FLSC_HB_DISABLED	0x01
#define FLSC_HB_BUSID6		0x02
#define FLSC_HB_SEAGATE		0x04
#define FLSC_HB_SLOW		0x08
#define FLSC_HB_SYNCHRON	0x10
#define FLSC_HB_CREQ		0x20
#define FLSC_HB_IACT		0x40
#define FLSC_HB_MINT		0x80

#define FLSC_PB_ESI		0x01
#define FLSC_PB_EDI		0x02
#define FLSC_PB_ENABLE_DMA	0x04
#define FLSC_PB_DISABLE_DMA	0x00	/* Symmetric reasons */
#define FLSC_PB_DMA_WRITE	0x08
#define FLSC_PB_DMA_READ	0x00	/* Symmetric reasons */
#define FLSC_PB_LED		0x10

#define FLSC_PB_INT_BITS (FLSC_PB_ESI | FLSC_PB_EDI)
#define FLSC_PB_DMA_BITS (FLSC_PB_ENABLE_DMA | FLSC_PB_DMA_WRITE)
