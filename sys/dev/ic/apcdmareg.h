/*	$NetBSD: apcdmareg.h,v 1.1 1999/06/05 14:29:10 mrg Exp $	*/

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 * APC DMA hardware; from SunOS header
 * Thanks to Derrick J. Brashear for additional info on the
 * meaning of some of these bits.
 */
struct apc_dma {
	volatile u_int32_t dmacsr;	/* APC CSR */
	volatile u_int32_t lpad[3];	/* */
	volatile u_int32_t dmacva;	/* Capture Virtual Address */
	volatile u_int32_t dmacc;	/* Capture Count */
	volatile u_int32_t dmacnva;	/* Capture Next Virtual Address */
	volatile u_int32_t dmacnc;	/* Capture next count */
	volatile u_int32_t dmapva;	/* Playback Virtual Address */
	volatile u_int32_t dmapc;	/* Playback Count */
	volatile u_int32_t dmapnva;	/* Playback Next VAddress */
	volatile u_int32_t dmapnc;	/* Playback Next Count */
};

/*
 * APC CSR Register bit definitions
 */
#define	APC_IP		0x00800000	/* Interrupt Pending */
#define	APC_PI		0x00400000	/* Playback interrupt */
#define	APC_CI		0x00200000	/* Capture interrupt */
#define	APC_EI		0x00100000	/* General interrupt */
#define	APC_IE		0x00080000	/* General ext int. enable */
#define	APC_PIE		0x00040000	/* Playback ext intr */
#define	APC_CIE		0x00020000	/* Capture ext intr */
#define	APC_EIE		0x00010000	/* Error ext intr */
#define	APC_PMI		0x00008000	/* Pipe empty interrupt */
#define	APC_PM		0x00004000	/* Play pipe empty */
#define	APC_PD		0x00002000	/* Playback NVA dirty */
#define	APC_PMIE	0x00001000	/* play pipe empty Int enable */
#define	APC_CM		0x00000800	/* Cap data dropped on floor */
#define	APC_CD		0x00000400	/* Capture NVA dirty */
#define	APC_CMI		0x00000200	/* Capture pipe empty interrupt */
#define	APC_CMIE	0x00000100	/* Cap. pipe empty int enable */
#define	APC_PPAUSE	0x00000080	/* Pause the play DMA */
#define	APC_CPAUSE	0x00000040	/* Pause the capture DMA */
#define	APC_CODEC_PDN   0x00000020	/* CODEC RESET */
#define	PDMA_GO		0x00000008
#define	CDMA_GO		0x00000004	/* bit 2 of the csr */
#define	APC_RESET	0x00000001	/* Reset the chip */

#define APC_BITS					\
	"\20\30IP\27PI\26CI\25EI\24IE"			\
	"\23PIE\22CIE\21EIE\20PMI\17PM\16PD\15PMIE"	\
	"\14CM\13CD\12CMI\11CMIE\10PPAUSE\7CPAUSE\6PDN\4PGO\3CGO"

/*
 * To start DMA, you write to dma[cp]nva and dma[cp]nc and set [CP]DMA_GO
 * in dmacsr. dma[cp]va and dma[cp]c, when read, appear to be the live
 * counter as the DMA operation progresses.
 * Supposedly, you get an interrupt with the "dirty" bits (APC_PD,APC_CD)
 * set, when the next DMA buffer can be programmed, while the current one
 * is still in progress. We don't currently use this feature, since I
 * haven't been able to make it work.. instead the next buffer goes in
 * as soon as we see a "pipe empty" (APC_PM) interrupt.
 */

/* It's not clear if there's a maximum DMA size.. */
#define APC_MAX		(sc->sc_blksz)/*(16*1024)*/

/*
 * List of device memory allocations (see cs4231_malloc/cs4231_free).
 */
struct cs_dma {
	struct	cs_dma *next;
	caddr_t	addr;
	bus_dma_segment_t segs[1];
	int	nsegs;
	size_t	size;
};
