/* $NetBSD: auspireg.h,v 1.1 2006/10/02 08:00:07 gdamore Exp $ */

/*-
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * Portions of this code were written by Garrett D'Amore for the
 * Champaign-Urbana Community Wireless Network Project.
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

#ifndef	_MIPS_ALCHEMY_AUSPIREG_H_
#define	_MIPS_ALCHEMY_AUSPIREG_H_

#define	SPICFG_RT_8		(0x3 << 30)
#define	SPICFG_RT_4		(0x2 << 30)
#define	SPICFG_RT_2		(0x1 << 30)
#define	SPICFG_RT_1		(0x0 << 30)
#define	SPICFG_TT_8		(0x3 << 28)
#define	SPICFG_TT_4		(0x2 << 28)
#define	SPICFG_TT_2		(0x1 << 28)
#define	SPICFG_TT_1		(0x0 << 28)
#define	SPICFG_DD		(1 << 27)	/* disable DMA */
#define	SPICFG_DE		(1 << 26)	/* device enable */
#define	SPICFG_BRG_MASK		(0x3f << 15)	/* baud rate generator */
#define	SPICFG_BRG_SHIFT	15
#define	SPICFG_DIV_MASK		(0x3 << 13)	/* psc clock divider */
#define	SPICFG_DIV_SHIFT	13
#define	SPICFG_BI		(1 << 12)	/* bit clock invert */
#define	SPICFG_PSE		(1 << 11)	/* port swap enable */
#define	SPICFG_CGE		(1 << 10)	/* clock gate enable */
#define	SPICFG_CDE		(1 << 9)	/* clock phase delay enable */
#define	SPICFG_LEN_MASK		(0x1f << 4)	/* data length */
#define	SPICFG_LEN_SHIFT	4
#define	SPICFG_LB		(1 << 3)	/* loopback mode */
#define	SPICFG_MLF		(1 << 2)	/* msb/lsb data first */
#define	SPICFG_MO		(1 << 0)	/* master only mode */

/* and also SPIEVNT */
#define	SPIMSK_MM		(1 << 16)	/* multiple master error */
#define	SPIMSK_RR		(1 << 13)	/* rx fifo request */
#define	SPIMSK_RO		(1 << 12)	/* rx fifo overflow */
#define	SPIMSK_RU		(1 << 11)	/* rx fifo underflow */
#define	SPIMSK_TR		(1 << 10)	/* tx fifo request */
#define	SPIMSK_TO		(1 << 9)	/* tx fifo overflow */
#define	SPIMSK_TU		(1 << 8)	/* tx fifo underflow */
#define	SPIMSK_SD		(1 << 5)	/* slave done */
#define	SPIMSK_MD		(1 << 4)	/* master done */
#define	SPIMSK_ALL		(SPIMSK_MM | SPIMSK_RR | SPIMSK_RO | \
				 SPIMSK_RU | SPIMSK_TR | SPIMSK_TO | \
				 SPIMSK_TU | SPIMSK_SD | SPIMSK_MD)
#define	SPIMSK_NORM		(SPIMSK_RU | SPIMSK_TO | SPIMSK_TR | SPIMSK_SD)

#define	SPIPCR_RC		(1 << 6)	/* rx data clear */
#define	SPIPCR_SP		(1 << 5)	/* slave stop */
#define	SPIPCR_SS		(1 << 4)	/* slave start */
#define	SPIPCR_TC		(1 << 2)	/* tx data clear */
#define	SPIPCR_MS		(1 << 0)	/* master start*/

#define	SPISTAT_RF		(1 << 13)	/* rx fifo full */
#define	SPISTAT_RE		(1 << 12)	/* rx fifo empty */
#define	SPISTAT_RR		(1 << 11)	/* rx request */
#define	SPISTAT_TF		(1 << 10)	/* tx fifo full */
#define	SPISTAT_TE		(1 << 9)	/* tx fifo empty */
#define	SPISTAT_TR		(1 << 8)	/* tx request */
#define	SPISTAT_SB		(1 << 5)	/* slave busy */
#define	SPISTAT_MB		(1 << 4)	/* master busy */
#define	SPISTAT_DI		(1 << 2)	/* device interrupt */
#define	SPISTAT_DR		(1 << 1)	/* device ready */
#define	SPISTAT_SR		(1 << 0)	/* psc ready */

#define	SPITXRX_LC		(1 << 29)	/* last character */
#define	SPITXRX_ST		(1 << 28)	/* select togle */
#define	SPITXRX_DATA_MASK	(0xffffff)
#define	SPITXRX_DATA_SHIFT	0

#endif	/* _MIPS_ALCHEMY_AUSPIREG_H_ */
