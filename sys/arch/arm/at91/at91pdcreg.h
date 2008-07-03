/*	$NetBSD: at91pdcreg.h,v 1.2 2008/07/03 01:15:38 matt Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _AT91PDCREG_H_
#define _AT91PDCREG_H_

#define	PDC_RPR		0x00UL		/* Receive Pointer Register	*/
#define	PDC_RCR		0x04UL		/* Receive Counter Register	*/
#define	PDC_TPR		0x08UL		/* Transmit Pointer Register	*/
#define	PDC_TCR		0x0CUL		/* Transmit Counter Register	*/
#define	PDC_RNPR	0x10UL		/* Receive Next Pointer Reg	*/
#define	PDC_RNCR	0x14UL		/* Receive Next Counter Reg	*/
#define	PDC_TNPR	0x18UL		/* Transmit Next Ptt Register	*/
#define	PDC_TNCR	0x1CUL		/* Transmit Next Counter Reg	*/
#define	PDC_PTCR	0x20UL		/* PDC Transfer Ctl Reg	PDC_	*/
#define	PDC_PTSR	0x24UL		/* PDC Transfer Status Reg	*/

/* Transfer Control Register bits: */
#define	PDC_PTCR_TXTDIS	0x200		/* disable transmitter		*/
#define	PDC_PTCR_TXTEN	0x100		/* enable transmitter		*/
#define	PDC_PTCR_RXTDIS	0x002		/* disable receiver		*/
#define	PDC_PTCR_RXTEN	0x001		/* enable receiver		*/

/* Transfer Status Register bits: */
#define	PDC_PTSR_TXTEN	PDC_PTCR_TXTEN
#define	PDC_PTSR_RXTEN	PDC_PTCR_RXTEN

#endif	// _AT91PDCREG_H_

