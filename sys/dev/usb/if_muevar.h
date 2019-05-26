/*	$NetBSD: if_muevar.h,v 1.8 2019/05/26 04:52:07 mlelstv Exp $	*/
/*	$OpenBSD: if_muereg.h,v 1.1 2018/08/03 01:50:15 kevlo Exp $	*/

/*
 * Copyright (c) 2018 Kevin Lo <kevlo@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _IF_MUEVAR_H_
#define _IF_MUEVAR_H_

#include <sys/rndsource.h>

struct mue_chain {
	struct mue_softc	*mue_sc;
	struct usbd_xfer	*mue_xfer;
	char			*mue_buf;
};

struct mue_cdata {
#ifndef MUE_TX_LIST_CNT
#define MUE_TX_LIST_CNT	4
#endif
	struct mue_chain	mue_tx_chain[MUE_TX_LIST_CNT];
#ifndef MUE_RX_LIST_CNT
#define MUE_RX_LIST_CNT	4
#endif
	struct mue_chain	mue_rx_chain[MUE_RX_LIST_CNT];
	int			mue_tx_prod;
	int			mue_tx_cnt;
};

struct mue_rxbuf_hdr {
	uint32_t		rx_cmd_a;
#define MUE_RX_CMD_A_LEN_MASK	0x00003fff
#define MUE_RX_CMD_A_ICSM	0x00004000
#define MUE_RX_CMD_A_ERRORS	__BITS(16, 21)	/* non-checksum errors */
#define MUE_RX_CMD_A_RED	0x00400000
#define MUE_RX_CMD_A_PID	__BITS(28, 27)
#define MUE_RX_CMD_A_PID_TCP	__SHIFTIN(1, MUE_RX_CMD_A_PID)
#define MUE_RX_CMD_A_PID_UDP	__SHIFTIN(2, MUE_RX_CMD_A_PID)
#define MUE_RX_CMD_A_PID_IP	__SHIFTIN(3, MUE_RX_CMD_A_PID)
#define MUE_RX_CMD_A_IPV	__BIT(29)
#define MUE_RX_CMD_A_TCE	__BIT(30)
#define MUE_RX_CMD_A_ICE	__BIT(31)

	uint32_t		rx_cmd_b;
	uint16_t		rx_cmd_c;
} __packed;

struct mue_txbuf_hdr {
	uint32_t		tx_cmd_a;
#define MUE_TX_CMD_A_LEN_MASK	0x000fffff
#define MUE_TX_CMD_A_FCS	0x00400000
#define MUE_TX_CMD_A_TPE	0x02000000
#define MUE_TX_CMD_A_IPE	0x04000000
#define MUE_TX_CMD_A_LSO	0x08000000

	uint32_t		tx_cmd_b;
#define MUE_TX_MSS_MIN		8
#define MUE_TX_CMD_B_MSS_SHIFT	16
#define MUE_TX_CMD_B_MSS_MASK	0x3fff0000

} __packed;

struct mue_softc {
	device_t		mue_dev;
	bool			mue_dying;

	uint8_t			mue_enaddr[ETHER_ADDR_LEN];
	struct ethercom		mue_ec;
	struct mii_data		mue_mii;
#define GET_MII(sc)	(&(sc)->mue_mii)
#define GET_IFP(sc)	(&(sc)->mue_ec.ec_if)

/* The interrupt endpoint is currently unused by the Moschip part. */
#define MUE_ENDPT_RX	0
#define MUE_ENDPT_TX	1
#define MUE_ENDPT_INTR	2
#define MUE_ENDPT_MAX	3
	int			mue_ed[MUE_ENDPT_MAX];
	struct usbd_pipe	*mue_ep[MUE_ENDPT_MAX];

	struct mue_cdata	mue_cdata;
	callout_t		mue_stat_ch;

	struct usbd_device	*mue_udev;
	struct usbd_interface	*mue_iface;

	struct usb_task		mue_tick_task;

	kmutex_t		mue_mii_lock;

	struct timeval		mue_rx_notice;
	struct timeval		mue_tx_notice;

	uint16_t		mue_product;
	uint16_t		mue_flags;
	uint32_t		mue_id_rev;

	int			mue_if_flags;
	int			mue_refcnt;

	krndsource_t		mue_rnd_source;

	int			mue_phyno;
	uint32_t		mue_rxbufsz;
	uint32_t		mue_txbufsz;
	int			mue_link;

	unsigned		mue_rx_list_cnt;
	unsigned		mue_tx_list_cnt;

	kmutex_t		mue_usb_lock;
};

#endif /* _IF_MUEVAR_H_ */
