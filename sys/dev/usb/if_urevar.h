/*	$NetBSD: if_urevar.h,v 1.2 2019/02/07 10:36:20 mlelstv Exp $	*/
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

#define URE_ENDPT_RX		0
#define URE_ENDPT_TX		1
#define URE_ENDPT_MAX		2

#ifndef URE_TX_LIST_CNT
#define URE_TX_LIST_CNT		4
#endif
#ifndef URE_RX_LIST_CNT
#define URE_RX_LIST_CNT		4
#endif

struct ure_chain {
	struct ure_softc	*uc_sc;
	struct usbd_xfer	*uc_xfer;
	char			*uc_buf;
};

struct ure_cdata {
	struct ure_chain	tx_chain[URE_TX_LIST_CNT];
	struct ure_chain	rx_chain[URE_RX_LIST_CNT];
	int			tx_prod;
	int			tx_cnt;
};

struct ure_softc {
	device_t		ure_dev;
	struct usbd_device	*ure_udev;

	/* usb */
	struct usbd_interface	*ure_iface;
	struct usb_task		ure_tick_task;
	int			ure_ed[URE_ENDPT_MAX];
	struct usbd_pipe	*ure_ep[URE_ENDPT_MAX];

	/* ethernet */
	struct ethercom		ure_ec;
#define GET_IFP(sc)		(&(sc)->ure_ec.ec_if)
	struct mii_data		ure_mii;
#define GET_MII(sc)		(&(sc)->ure_mii)
	kmutex_t		ure_mii_lock;
	int			ure_refcnt;

	struct ure_cdata	ure_cdata;
	callout_t		ure_stat_ch;

	struct timeval		ure_rx_notice;
	struct timeval		ure_tx_notice;
	u_int			ure_bufsz;

	int			ure_phyno;

	u_int			ure_flags;
#define	URE_FLAG_LINK		0x0001
#define	URE_FLAG_8152		0x1000	/* RTL8152 */

	u_int			ure_chip;
#define	URE_CHIP_VER_4C00	0x01
#define	URE_CHIP_VER_4C10	0x02
#define	URE_CHIP_VER_5C00	0x04
#define	URE_CHIP_VER_5C10	0x08
#define	URE_CHIP_VER_5C20	0x10
#define	URE_CHIP_VER_5C30	0x20

	krndsource_t            ure_rnd_source;
	bool			ure_dying;
};
