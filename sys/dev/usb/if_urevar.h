/*	$NetBSD: if_urevar.h,v 1.5 2019/08/09 06:46:35 mrg Exp $	*/

/*	$OpenBSD: if_urereg.h,v 1.5 2018/11/02 21:32:30 jcs Exp $	*/
/*-
 * Copyright (c) 2015-2016 Kevin Lo <kevlo@FreeBSD.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

struct ure_intrpkt {
	uint8_t	ure_tsr;
	uint8_t	ure_rsr;
	uint8_t	ure_gep_msr;
	uint8_t	ure_waksr;
	uint8_t	ure_txok_cnt;
	uint8_t	ure_rxlost_cnt;
	uint8_t	ure_crcerr_cnt;
	uint8_t	ure_col_cnt;
} __packed;

struct ure_rxpkt {
	uint32_t ure_pktlen;
#define	URE_RXPKT_LEN_MASK	0x7fff
	uint32_t ure_csum;
#define URE_RXPKT_IPV4_CS	__BIT(19)
#define URE_RXPKT_IPV6_CS	__BIT(20)
#define URE_RXPKT_TCP_CS	__BIT(22)
#define URE_RXPKT_UDP_CS	__BIT(23)
	uint32_t ure_misc;
#define URE_RXPKT_RX_VLAN_TAG	__BIT(16)
#define URE_RXPKT_TCP_F		__BIT(21)
#define URE_RXPKT_UDP_F		__BIT(22)
#define URE_RXPKT_IP_F		__BIT(23)
	uint32_t ure_rsvd2;
	uint32_t ure_rsvd3;
	uint32_t ure_rsvd4;
} __packed;

struct ure_txpkt {
	uint32_t ure_pktlen;
#define	URE_TXPKT_TX_FS		__BIT(31)
#define	URE_TXPKT_TX_LS		__BIT(30)
#define	URE_TXPKT_LEN_MASK	0xffff
	uint32_t ure_csum;
#define URE_L4_OFFSET_MAX	0x7ff
#define URE_L4_OFFSET_SHIFT	17
#define URE_TXPKT_IPV6_CS	__BIT(28)
#define URE_TXPKT_IPV4_CS	__BIT(29)
#define URE_TXPKT_TCP_CS	__BIT(30)
#define URE_TXPKT_UDP_CS	__BIT(31)
} __packed;

#ifndef URE_TX_LIST_CNT
#define URE_TX_LIST_CNT		4
#endif
#ifndef URE_RX_LIST_CNT
#define URE_RX_LIST_CNT		4
#endif

/* usbnet::un_flags values */
#define	URE_FLAG_VER_4C00	0x0001
#define	URE_FLAG_VER_4C10	0x0002
#define	URE_FLAG_VER_5C00	0x0004
#define	URE_FLAG_VER_5C10	0x0008
#define	URE_FLAG_VER_5C20	0x0010
#define	URE_FLAG_VER_5C30	0x0020
#define	URE_FLAG_8152		0x1000	/* RTL8152 */
