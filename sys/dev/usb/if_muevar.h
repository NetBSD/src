/*	$NetBSD: if_muevar.h,v 1.9 2019/08/15 08:02:32 mrg Exp $	*/
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

#ifndef MUE_TX_LIST_CNT
#define MUE_TX_LIST_CNT	4
#endif
#ifndef MUE_RX_LIST_CNT
#define MUE_RX_LIST_CNT	4
#endif

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

#endif /* _IF_MUEVAR_H_ */
