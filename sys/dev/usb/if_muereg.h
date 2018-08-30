/*	$NetBSD: if_muereg.h,v 1.2 2018/08/30 09:00:08 rin Exp $	*/
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

#ifndef _IF_MUEREG_H_
#define _IF_MUEREG_H_

/* XXX for ETHER_HDR_LEN and ETHER_VLAN_ENCAP_LEN */
#include <net/if_ether.h>

/* XXX for IP_MAXPACKET */
#include <netinet/ip.h>

/* XXX for struct mue_txbuf_hdr */
#include <dev/usb/if_muevar.h>

/* USB vendor requests */
#define MUE_UR_WRITEREG         0xa0
#define MUE_UR_READREG		0xa1

/* registers */
#define MUE_INT_STATUS			0x00c
#define MUE_HW_CFG			0x010
#define MUE_PMT_CTL			0x014
#define MUE_LED_CFG			0x018
#define MUE_DP_SEL			0x024
#define MUE_DP_CMD			0x028
#define MUE_DP_ADDR			0x02c
#define MUE_DP_DATA			0x030
#define MUE_7500_BURST_CAP		0x034
#define MUE_7500_INT_EP_CTL		0x038
#define MUE_7500_BULKIN_DELAY		0x03c
#define MUE_E2P_CMD			0x040
#define MUE_E2P_DATA			0x044
#define MUE_E2P_IND			0x0a5
#define MUE_7500_RFE_CTL		0x060
#define MUE_USB_CFG0			0x080
#define MUE_USB_CFG1			0x084
#define MUE_7500_FCT_RX_CTL		0x090
#define MUE_7800_BURST_CAP		0x090
#define MUE_7500_FCT_TX_CTL		0x094
#define MUE_7800_BULKIN_DELAY		0x094
#define MUE_7500_FCT_RX_FIFO_END	0x098
#define MUE_7800_INT_EP_CTL		0x098
#define MUE_7500_FCT_TX_FIFO_END	0x09c
#define MUE_7500_FCT_FLOW		0x0a0
#define MUE_7800_RFE_CTL		0x0b0
#define MUE_7800_FCT_RX_CTL		0x0c0
#define MUE_7800_FCT_TX_CTL		0x0c4
#define MUE_7800_FCT_RX_FIFO_END	0x0c8
#define MUE_7800_FCT_TX_FIFO_END	0x0cc
#define MUE_7800_FCT_FLOW		0x0d0
#define MUE_LTM_INDEX(idx)		(0x0e0 + (idx) * 4)
#define MUE_NUM_LTM_INDEX		6
#define MUE_MAC_CR			0x100
#define MUE_MAC_RX			0x104
#define MUE_MAC_TX			0x108
#define MUE_FLOW			0x10c
#define MUE_RX_ADDRH			0x118
#define MUE_RX_ADDRL			0x11c
#define MUE_MII_ACCESS			0x120
#define MUE_MII_DATA			0x124
#define MUE_7500_ADDR_FILTX_BASE	0x300
#define MUE_7500_ADDR_FILTX(i)		(MUE_7500_ADDR_FILTX_BASE + 8 * (i))
#define MUE_7800_ADDR_FILTX_BASE	0x400
#define MUE_7800_ADDR_FILTX(i)		(MUE_7800_ADDR_FILTX_BASE + 8 * (i))
#define MUE_NUM_ADDR_FILTX		33

/* hardware configuration register */
#define MUE_HW_CFG_SRST		0x00000001
#define MUE_HW_CFG_LRST		0x00000002
#define MUE_HW_CFG_BCE		0x00000004
#define MUE_HW_CFG_MEF		0x00000010
#define MUE_HW_CFG_BIR		0x00000080
#define MUE_HW_CFG_LED0_EN	0x00100000
#define MUE_HW_CFG_LED1_EN	0x00200000

/* power management control register */
#define MUE_PMT_CTL_PHY_RST	0x00000010
#define MUE_PMT_CTL_READY	0x00000080

/* LED configuration register */
#define MUE_LED_CFG_LEDGPIO_EN		0x0000f000
#define MUE_LED_CFG_LED10_FUN_SEL	0x40000000
#define MUE_LED_CFG_LED2_FUN_SEL	0x80000000

/* data port select register */
#define MUE_DP_SEL_RSEL_MASK	0x0000000f
#define MUE_DP_SEL_VHF		0x00000001
#define MUE_DP_SEL_DPRDY	0x80000000
#define MUE_DP_SEL_VHF_HASH_LEN	16
#define MUE_DP_SEL_VHF_VLAN_LEN	128

/* data port command register */
#define MUE_DP_CMD_WRITE	0x00000001

/* burst cap register and etc */
#define MUE_SS_USB_PKT_SIZE		1024
#define MUE_HS_USB_PKT_SIZE		512
#define MUE_FS_USB_PKT_SIZE		64
#define MUE_7500_HS_RX_BUFSIZE		\
	(16 * 1024 + 5 * MUE_HS_USB_PKT_SIZE)
#define MUE_7500_FS_RX_BUFSIZE		\
	(6 * 1024 + 33 * MUE_FS_USB_PKT_SIZE)
#define MUE_7500_MAX_RX_FIFO_SIZE	(20 * 1024)
#define MUE_7500_MAX_TX_FIFO_SIZE	(12 * 1024)
#define MUE_7800_RX_BUFSIZE		(12 * 1024)
#define MUE_7800_MAX_RX_FIFO_SIZE	MUE_7800_RX_BUFSIZE
#define MUE_7800_MAX_TX_FIFO_SIZE	MUE_7800_RX_BUFSIZE
#define MUE_TX_BUFSIZE			(sizeof(struct mue_txbuf_hdr) + \
	ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN + IP_MAXPACKET)

/* interrupt endpoint control register */
#define MUE_INT_EP_CTL_PHY_INT		0x20000

/* bulk-in delay register */
#define MUE_7500_DEFAULT_BULKIN_DELAY	0x00002000
#define MUE_7800_DEFAULT_BULKIN_DELAY	0x00000800

/* EEPROM command register */
#define MUE_E2P_CMD_ADDR_MASK	0x000001ff
#define MUE_E2P_CMD_READ	0x00000000
#define MUE_E2P_CMD_LOADED	0x00000200
#define MUE_E2P_CMD_TIMEOUT	0x00000400
#define MUE_E2P_CMD_BUSY	0x80000000
#define MUE_E2P_IND_OFFSET	0x000
#define	MUE_E2P_MAC_OFFSET	0x001
#define	MUE_E2P_LTM_OFFSET	0x03f

/* Receive Filtering Engine control register */
#define MUE_RFE_CTL_PERFECT		0x00000002
#define MUE_RFE_CTL_MULTICAST_HASH	0x00000008
#define MUE_RFE_CTL_VLAN_FILTER		0x00000020
#define MUE_RFE_CTL_UNICAST		0x00000100
#define MUE_RFE_CTL_MULTICAST		0x00000200
#define MUE_RFE_CTL_BROADCAST		0x00000400
#define MUE_RFE_CTL_IP_COE		0x00000800
#define MUE_RFE_CTL_TCPUDP_COE		0x00001000
#define MUE_RFE_CTL_ICMP_COE		0x00002000
#define MUE_RFE_CTL_IGMP_COE		0x00004000

/* USB configuration register 0 */
#define MUE_USB_CFG0_BCE	0x00000020
#define MUE_USB_CFG0_BIR	0x00000040

/* USB configuration register 1 */
#define MUE_USB_CFG1_LTM_ENABLE		0x00000100
#define MUE_USB_CFG1_DEV_U1_INIT_EN	0x00000400
#define MUE_USB_CFG1_DEV_U2_INIT_EN	0x00001000

/* RX FIFO control register */
#define MUE_FCT_RX_CTL_EN	0x80000000

/* TX FIFO control register */
#define MUE_FCT_TX_CTL_EN	0x80000000

/* MAC control register */
#define MUE_MAC_CR_RST		0x00000001
#define MUE_MAC_CR_FULL_DUPLEX	0x00000008
#define MUE_MAC_CR_AUTO_SPEED	0x00000800
#define MUE_MAC_CR_AUTO_DUPLEX	0x00001000
#define MUE_MAC_CR_GMII_EN	0x00080000

/* MAC receive register */
#define MUE_MAC_RX_RXEN			0x00000001
#define MUE_MAC_RX_MAX_SIZE_MASK	0x3fff0000
#define MUE_MAC_RX_MAX_SIZE_SHIFT	16
#define MUE_MAC_RX_MAX_LEN(x)	\
	(((x) << MUE_MAC_RX_MAX_SIZE_SHIFT) & MUE_MAC_RX_MAX_SIZE_MASK)

/* MAC transmit register */
#define MUE_MAC_TX_TXEN		0x00000001

/* flow control register */
#define MUE_FLOW_PAUSE_TIME	0x0000ffff
#define MUE_FLOW_RX_FCEN	0x20000000
#define MUE_FLOW_TX_FCEN	0x40000000

/* MII access register */
#define MUE_MII_ACCESS_READ		0x00000000
#define MUE_MII_ACCESS_BUSY		0x00000001
#define MUE_MII_ACCESS_WRITE		0x00000002
#define MUE_MII_ACCESS_REGADDR_MASK	0x000007c0
#define MUE_MII_ACCESS_REGADDR_SHIFT	6
#define MUE_MII_ACCESS_PHYADDR_MASK	0x0000f800
#define MUE_MII_ACCESS_PHYADDR_SHIFT	11
#define MUE_MII_ACCESS_REGADDR(x)	\
	(((x) << MUE_MII_ACCESS_REGADDR_SHIFT) & MUE_MII_ACCESS_REGADDR_MASK)
#define MUE_MII_ACCESS_PHYADDR(x)	\
	(((x) << MUE_MII_ACCESS_PHYADDR_SHIFT) & MUE_MII_ACCESS_PHYADDR_MASK)

/* MAC address perfect filter register */
#define MUE_ADDR_FILTX_VALID	0x80000000

/* undocumented OTP registers from Linux via FreeBSD */
#define MUE_OTP_BASE_ADDR		0x01000
#define MUE_OTP_ADDR(off)		(MUE_OTP_BASE_ADDR + 4 * (off))
#define MUE_OTP_PWR_DN			MUE_OTP_ADDR(0x00)
#define MUE_OTP_PWR_DN_PWRDN_N		0x01
#define MUE_OTP_ADDR1			MUE_OTP_ADDR(0x01)
#define MUE_OTP_ADDR1_MASK		0x1f
#define MUE_OTP_ADDR2			MUE_OTP_ADDR(0x02)
#define MUE_OTP_ADDR2_MASK		0xff
#define MUE_OTP_ADDR3			MUE_OTP_ADDR(0x03)
#define MUE_OTP_ADDR3_MASK		0x03
#define MUE_OTP_RD_DATA			MUE_OTP_ADDR(0x06)
#define MUE_OTP_FUNC_CMD		MUE_OTP_ADDR(0x08)
#define MUE_OTP_FUNC_CMD_RESET		0x04
#define MUE_OTP_FUNC_CMD_PROGRAM	0x02
#define MUE_OTP_FUNC_CMD_READ		0x01
#define MUE_OTP_MAC_OFFSET		0x01
#define MUE_OTP_IND_OFFSET		0x00
#define MUE_OTP_IND_1			0xf3
#define MUE_OTP_IND_2			0xf7
#define MUE_OTP_CMD_GO			MUE_OTP_ADDR(0x0a)
#define MUE_OTP_CMD_GO_GO		0x01
#define MUE_OTP_STATUS			MUE_OTP_ADDR(0x0a)
#define MUE_OTP_STATUS_OTP_LOCK		0x10
#define MUE_OTP_STATUS_BUSY		0x01

#endif /* _IF_MUEREG_H_ */
