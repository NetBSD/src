/*	$Id: at91twireg.h,v 1.1.22.1 2008/07/03 18:37:51 simonb Exp $	*/
/*	$NetBSD: at91twireg.h,v 1.1.22.1 2008/07/03 18:37:51 simonb Exp $	*/

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

#ifndef	_AT91TWIREG_H_
#define	_AT91TWIREG_H_

#define	AT91_TWI_SIZE	0x4000U

#define	TWI_CR		0x00U	/* 0x00: Control Register		*/
#define	TWI_MMR		0x04U	/* 0x04: Master Mode Register		*/
#define	TWI_IADR	0x0CU	/* 0x0C: Internal Address Register	*/
#define	TWI_CWGR	0x10U	/* 0x10: Clock Waveform Generator Reg	*/
#define	TWI_SR		0x20U	/* 0x20: Status Register		*/
#define	TWI_IER		0x24U	/* 0x24: Interrupt Enable Register	*/
#define	TWI_IDR		0x28U	/* 0x28: Interrupt Disable Register	*/
#define	TWI_IMR		0x2CU	/* 0x2C: Interrupt Mask Register	*/
#define	TWI_RHR		0x30U	/* 0x30: Receive Holding Register	*/
#define	TWI_THR		0x34U	/* 0x34: Transmit Holding Register	*/

/* Control Registre bits: */
#define	TWI_CR_SWRST	0x80U	/* 1 = do software reset		*/
#define	TWI_CR_MSDIS	0x08U	/* 1 = disable master mode		*/
#define	TWI_CR_MSEN	0x04U	/* 1 = enable master mode		*/
#define	TWI_CR_STOP	0x02U	/* 1 = send a stop condition		*/
#define	TWI_CR_START	0x01U	/* 1 = send a start condition		*/

/* Master Mode Register bits: */
#define	TWI_MMR_DADR 0x7F0000U /* device address			*/
#define	TWI_MMR_DADR_SHIFT 16U
#define	TWI_MMR_MREAD	0x1000U	/* 1 = Master read direction (0= write)	*/
#define	TWI_MMR_IADRSZ 0x300U	/* device address size		*/
#define	TWI_MMR_IADRSZ_SHIFT 8U

/* Clock Waveform Generator Register bits: */
#define	TWI_CWGR_CKDIV 0x70000U	/* Clock Divider		*/
#define	TWI_CWGR_CKDIV_SHIFT 16U
#define	TWI_CWGR_CHDIV 0xFF00U	/* Clock High Divider		*/
#define	TWI_CWGR_CHDIV_SHIFT 8U
#define	TWI_CWGR_CLDIV 0xFFU	/* Clock Low Divider		*/
#define	TWI_CWGR_CLDIV_SHIFT 0U


/* Status Register bits: */
#define	TWI_SR_NACK	0x100U		/* 1 = not acknowledged		*/
#define	TWI_SR_UNRE	0x080U		/* 1 = underrun error		*/
#define	TWI_SR_OVRE	0x040U		/* 1 = overrun error		*/
#define	TWI_SR_TXRDY	0x004U		/* 1 = transmit holding reg rdy	*/
#define	TWI_SR_RXRDY	0x002U		/* 1 = receive hodling reg rdy	*/
#define	TWI_SR_TXCOMP	0x001U		/* 1 = transmission completed	*/


#endif	/* _AT91TWIREG_H_ */
