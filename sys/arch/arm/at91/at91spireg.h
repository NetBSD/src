/*	$Id: at91spireg.h,v 1.2 2008/07/03 01:15:38 matt Exp $	*/
/*	$NetBSD: at91spireg.h,v 1.2 2008/07/03 01:15:38 matt Exp $	*/

/*-
 * Copyright (c) 2007 Embedtronics Oy.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_AT91SPIREG_H_
#define	_AT91SPIREG_H_

#define	SPI_CS_COUNT	4

#define	AT91_SPI_SIZE	0x4000U

#define	SPI_CR		0x00U		/* 0x00: Control Register	*/
#define	SPI_MR		0x04U		/* 0x04: Mode Register		*/
#define	SPI_RDR		0x08U		/* 0x08: Receive Data Register	*/
#define	SPI_TDR		0x0CU		/* 0x0C: Transmit Data Register	*/
#define	SPI_SR		0x10U		/* 0x10: Status Register	*/
#define	SPI_IER		0x14U		/* 0x14: Interrupt Enable Reg	*/
#define	SPI_IDR		0x18U		/* 0x18: Interrupt Disable Reg	*/
#define	SPI_IMR		0x1CU		/* 0x1C: Interrupt Mask Reg	*/
#define	SPI_CSR(slv)	(0x30U + 4 * (slv)) /* 0x30: Chip Select Regs	*/
#define	SPI_PDC_BASE	0x100U		/* 0x100: PDC			*/

/* Control Register bits: */
#define	SPI_CR_SWRST	0x80		/* 1 = Reset the SPI		*/
#define	SPI_CR_SPIDIS	0x2		/* 1 = disables the SPI		*/
#define	SPI_CR_SPIEN	0x1		/* 1 = enables the SPI		*/

/* Mode Register bits: */
#define	SPI_MR_DLYBCS 0xFF000000	/* delay between chip selects	*/
#define	SPI_MR_DLYBCS_SHIFT 24
#define	SPI_MR_PCS	0x000F0000	/* peripheral chip select	*/
#define	SPI_MR_PCS_SHIFT	16
#define	SPI_MR_LLB	0x80		/* 1 = local loopback enabled	*/
#define	SPI_MR_MODFDIS	0x10		/* 1 = mode fault detection dis	*/
#define	SPI_MR_DIV32	0x08		/* 1 = SPI operates at MCK/32	*/
#define	SPI_MR_PCSDEC	0x04		/* 1 = use 4- to 16-bit decoder	*/
#define	SPI_MR_PS	0x02		/* 1 = variable peripheral sel.	*/
#define	SPI_MR_MSTR	0x01		/* 1 = SPI is in Master mode	*/

/* Status Register bits: */
#define	SPI_SR_SPIENS	0x10000		/* 1 = SPI is enabled		*/
#define	SPI_SR_TXBUFE	0x80		/* 1 = TX Buffer empty		*/
#define	SPI_SR_RXBUFF	0x40		/* 1 = RX buffer full		*/
#define	SPI_SR_ENDTX	0x20		/* 1 = End of TX buffer		*/
#define	SPI_SR_ENDRX	0x10		/* 1 = End of RX buffer		*/
#define	SPI_SR_OVRES	0x08		/* 1 = Overrun occurred		*/
#define	SPI_SR_MODF	0x04		/* 1 = Mode fault occurred	*/
#define	SPI_SR_TDRE	0x02		/* 1 = Transmit Data Reg empty	*/
#define	SPI_SR_RDRF	0x01		/* 1 = Receive Data Reg full	*/

/* Chip Select Register: */
#define	SPI_CSR_DLYBCT		0xFF000000
#define	SPI_CSR_DLYBCT_SHIFT	24
#define	SPI_CSR_DLYBS		0x00FF0000
#define	SPI_CSR_DLYBS_SHIFT	16
#define	SPI_CSR_SCBR		0x0000FF00
#define	SPI_CSR_SCBR_SHIFT	8
#define	SPI_CSR_BITS		0x000000F0
#define	SPI_CSR_BITS_8		(0U<<4)
#define	SPI_CSR_BITS_16		(8U<<4)
#define	SPI_CSR_BITS_SHIFT	4
#define	SPI_CSR_NCPHA		0x00000002
#define	SPI_CSR_CPOL		0x00000001
#define	SPI_CSR_RESERVED	0x0000000C

#endif	/* _AT91SPIREG_H_ */
