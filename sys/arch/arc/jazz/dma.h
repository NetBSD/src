/*	$NetBSD: dma.h,v 1.5 2003/05/04 10:07:51 tsutsui Exp $	*/
/*	$OpenBSD: dma.h,v 1.3 1997/04/19 17:19:51 pefo Exp $	*/

/*
 * Copyright (c) 1996 Per Fogelstrom
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
 *      This product includes software developed by Per Fogelstrom.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

/*
 *  Hardware dma registers.
 */

#define R4030_DMA_MODE		0x00
#define  R4030_DMA_MODE_40NS	0x00	/* Device dma timing */
#define  R4030_DMA_MODE_80NS	0x01	/* Device dma timing */
#define  R4030_DMA_MODE_120NS	0x02	/* Device dma timing */
#define  R4030_DMA_MODE_160NS	0x03	/* Device dma timing */
#define  R4030_DMA_MODE_200NS	0x04	/* Device dma timing */
#define  R4030_DMA_MODE_240NS	0x05	/* Device dma timing */
#define  R4030_DMA_MODE_280NS	0x06	/* Device dma timing */
#define  R4030_DMA_MODE_320NS	0x07	/* Device dma timing */
#define  R4030_DMA_MODE_8	0x08	/* Device 8 bit  */
#define  R4030_DMA_MODE_16	0x10	/* Device 16 bit */
#define  R4030_DMA_MODE_32	0x18	/* Device 32 bit */
#define  R4030_DMA_MODE_INT	0x20	/* Interrupt when done */
#define  R4030_DMA_MODE_BURST	0x40	/* Burst mode (Rev 2 only) */
#define  R4030_DMA_MODE_FAST	0x80	/* Fast dma cycle (Rev 2 only) */

#define R4030_DMA_ENAB		0x08
#define  R4030_DMA_ENAB_RUN	0x0001	/* Enable dma */
#define  R4030_DMA_ENAB_READ	0x0000	/* Read from device */
#define  R4030_DMA_ENAB_WRITE	0x0002	/* Write to device */
#define  R4030_DMA_ENAB_TC_IE	0x0100	/* Terminal count int enable */
#define  R4030_DMA_ENAB_ME_IE	0x0200	/* Memory error int enable */
#define  R4030_DMA_ENAB_TL_IE	0x0400	/* Translation limit int enable */

#define R4030_DMA_COUNT		0x10
#define  R4030_DMA_COUNT_MASK	0x000fffff /* Byte count mask */

#define	R4030_DMA_ADDR		0x18

#define R4030_DMA_RANGE		0x20
