/*	$NetBSD: pciide_geode_reg.h,v 1.1 2004/07/09 18:38:37 bouyer Exp $	*/

/*
 * Copyright (c) 2004 Manuel Bouyer.
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
 *	This product includes software developed by Manuel Bouyer.
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
 *
 */

#define PIO_REG(chan, drv)	(0x20 + (chan) * 0x10 + (drv) * 0x8)
#define DMA_REG(chan, drv)	(0x24 + (chan) * 0x10 + (drv) * 0x8)
#define 	DMA_REG_PIO_FORMAT	0x80000000 /* select PIO format 1 */
#define		DMA_REG_UDMA		0x00100000 /* enable Ultra-DMA */

/*
 * Recommeded values from the cs5530 data sheet.
 * Note that the udma values include DMA_REG_UDMA.
 * However, from the datasheet bits 30-21 should be reserved, yet 
 * geode_udma sets bit 23 to 1. I don't know if it's the definition of
 * DMA_REG_UDMA which is wrong (bit 23 instead of 20) or these values.
 */
static const int32_t geode_pio[] __attribute__((__unused__)) =
    {0x9172d132, 0x21717121, 0x00803020, 0x20102010, 0x00100010};
static const int32_t geode_dma[] __attribute__((__unused__)) =
    {0x00077771, 0x00012121, 0x00002020};
static const int32_t geode_udma[] __attribute__((__unused__)) =
    {0x00921250, 0x00911140, 0x00911030};
