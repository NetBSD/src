/*	$NetBSD: if_ecreg.h,v 1.1 2001/06/27 17:24:35 fredette Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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
 * 3Com Ethernet controller registers.
 */

#define	EC_BUF_SZ	2048

#if __for_reference_only__
struct ec_regs {
	u_int16_t	ec_csr;		/* control/status register */
	u_int16_t	ec_backoff;	/* backoff seed */
	u_int8_t	ec_pad1[0x400 - (2 * sizeof(u_int16_t))];
	struct		ether_addr ec_arom;	/* address ROM */
	u_int8_t	ec_pad2[0x200 - sizeof(struct ether_addr)];
	struct		ether_addr ec_aram;	/* address RAM */
	u_int8_t	ec_pad3[0x200 - sizeof(struct ether_addr)];
	u_int8_t	ec_tbuf[EC_BUF_SZ];	/* transmit buffer */
	u_int8_t	ec_abuf[EC_BUF_SZ];	/* receive buffer A */
	u_int8_t	ec_bbuf[EC_BUF_SZ];	/* receive buffer B */
};
#endif

/* Register offsets. */
#define	ECREG_CSR	(0)
#define	ECREG_BACKOFF	(2)
#define	ECREG_AROM	(1024)
#define	ECREG_ARAM	(1536)
#define	ECREG_TBUF	(2048)
#define ECREG_ABUF	(ECREG_TBUF + EC_BUF_SZ)
#define ECREG_BBUF	(ECREG_ABUF + EC_BUF_SZ)
#define	ECREG_BANK_SZ	(ECREG_BBUF + EC_BUF_SZ)

/*
 * Control/status register bits
 */
#define	EC_CSR_BBSW	0x8000		/* B buffer empty (belongs to card) */
#define	EC_CSR_ABSW	0x4000		/* A buffer empty (belongs to card) */
#define	EC_CSR_TBSW	0x2000		/* T buffer full (belongs to card) */
#define	EC_CSR_JAM	0x1000		/* Ethernet jammed (collision) */
#define	EC_CSR_AMSW	0x0800		/* address RAM belongs to ether */
#define	EC_CSR_RBBA	0x0400		/* B buffer received before A */
#define	EC_CSR_RESET	0x0100		/* reset the card */
#define	EC_CSR_BINT	0x0080		/* B buffer interrupt enable */
#define	EC_CSR_AINT	0x0040		/* A buffer interrupt enable */
#define	EC_CSR_TINT	0x0020		/* T buffer interrupt enable */
#define	EC_CSR_JINT	0x0010		/* jam interrupt enable */
#define	EC_CSR_INTPA	0x00ff		/* mask for interrupt and PA fields */
#define	EC_CSR_PAMASK	0x000f		/* PA field */

#define	EC_CSR_PA	0x0007		/* receive mine+broadcast-errors */
#define EC_CSR_PROMISC	0x0001		/* receive all-errors */

/*
 * Turns an EC_CSR_xINT value into an ECREG_xBUF value.
 * NB: does not work with EC_CSR_TINT.
 */
#define EC_CSR_INT_BUF(x) (((x) << 5) + 2048)

/*
 * Turns an EC_CSR_xINT value into an ECREG_xBSW value.
 */
#define EC_CSR_INT_BSW(x) ((x) << 8)

/*
 * Receive status bits.  The first 16 bits of a receive
 * buffer are a status word.
 */
#define	EC_PKT_FCSERR		0x8000		/* FCS error */
#define	EC_PKT_BROADCAST	0x4000		/* packet was broadcast */
#define	EC_PKT_RGERR		0x2000		/* range error */
#define	EC_PKT_ADDRMATCH	0x1000		/* address match */
#define	EC_PKT_FRERR		0x0800		/* framing error */
#define	EC_PKT_DOFF		0x07ff		/* first free byte */

#define	EC_PKT_MAXTDOFF	(EC_BUF_SZ-ETHERMIN)	/* max xmit doff (min size) */
#define	EC_PKT_RDOFF	2			/* packet offset in buffer */
#define	EC_PKT_MINRDOFF	(EC_PKT_RDOFF+ETHERMIN)	/* min packet doff (min size) */
#define	EC_PKT_MAXRDOFF	(EC_PKT_RDOFF+ETHERMTU)	/* max packet doff (max size) */
