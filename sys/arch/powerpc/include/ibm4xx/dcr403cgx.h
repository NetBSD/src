/*	$NetBSD: dcr403cgx.h,v 1.1 2003/03/11 10:40:17 hannken Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

#ifndef _DCR403GCXP_H_
#define	_DCR403GCXP_H_

#ifndef _LOCORE
#define	mtdcr(reg, val)						\
	asm volatile("mtdcr %0,%1" : : "K"(reg), "r"(val))
#define	mfdcr(reg)						\
	( { u_int32_t val;					\
	  asm volatile("mfdcr %0,%1" : "=r"(val) : "K"(reg));	\
	  val; } )
#endif /* _LOCORE */

/* Device Control Register declarations */

#define DCR_EXISR		0x040	/* External Interrupt Status Register */
#define DCR_EXIER		0x042	/* External Interrupt Enable Register */
#define DCR_BRH0		0x070	/* Bank Register High 0 */
#define DCR_BRH1		0x071	/* Bank Register High 1 */
#define DCR_BRH2		0x072	/* Bank Register High 2 */
#define DCR_BRH3		0x073	/* Bank Register High 3 */
#define DCR_BRH4		0x074	/* Bank Register High 4 */
#define DCR_BRH5		0x075	/* Bank Register High 5 */
#define DCR_BRH6		0x076	/* Bank Register High 6 */
#define DCR_BRH7		0x077	/* Bank Register High 7 */
#define DCR_BR0			0x080	/* Bank Register 0 */
#define DCR_BR1			0x081	/* Bank Register 1 */
#define DCR_BR2			0x082	/* Bank Register 2 */
#define DCR_BR3			0x083	/* Bank Register 3 */
#define DCR_BR4			0x084	/* Bank Register 4 */
#define DCR_BR5			0x085	/* Bank Register 5 */
#define DCR_BR6			0x086	/* Bank Register 6 */
#define DCR_BR7			0x087	/* Bank Register 7 */
#define DCR_BEAR		0x090	/* Bus Error Address Register */
#define DCR_BESR		0x091	/* Bus Error Syndrome Register */
#define DCR_IOCR		0x0a0	/* I/O Configuration Register */
#define DCR_DMACR0		0x0c0	/* DMA Channel Control Register 0 */
#define DCR_DMACT0		0x0c1	/* DMA Count Register 0 */
#define DCR_DMADA0		0x0c2	/* DMA Destination Address Reg. 0 */
#define DCR_DMASA0		0x0c3	/* DMA Source Address Register 0 */
#define DCR_DMACC0		0x0c4	/* DMA Chained Count 0 */
#define DCR_DMACR1		0x0c8	/* DMA Channel Control Register 1 */
#define DCR_DMACT1		0x0c9	/* DMA Count Register 1 */
#define DCR_DMADA1		0x0cA	/* DMA Destination Address Reg. 1 */
#define DCR_DMACC1		0x0cC	/* DMA Chained Count 1 */
#define DCR_DMASA1		0x0cb	/* DMA Source Address Register 1 */
#define DCR_DMACR2		0x0d0	/* DMA Channel Control Register 2 */
#define DCR_DMACT2		0x0d1	/* DMA Count Register 2 */
#define DCR_DMADA2		0x0d2	/* DMA Destination Address Reg. 2 */
#define DCR_DMASA2		0x0d3	/* DMA Source Address Register 2 */
#define DCR_DMACC2		0x0d4	/* DMA Chained Count 2 */
#define DCR_DMACR3		0x0d8	/* DMA Channel Control Register 3 */
#define DCR_DMACT3		0x0d9	/* DMA Count Register 3 */
#define DCR_DMADA3		0x0da	/* DMA Destination Address Reg. 3 */
#define DCR_DMASA3		0x0db	/* DMA Source Address Register 3 */
#define DCR_DMACC3		0x0dc	/* DMA Chained Count 3 */
#define DCR_DMASR		0x0e0	/* DMA Status Register */

#endif /* _DCR403GCXP_H_ */
