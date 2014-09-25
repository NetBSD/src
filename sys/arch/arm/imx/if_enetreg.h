/*	$NetBSD: if_enetreg.h,v 1.1 2014/09/25 05:05:28 ryo Exp $	*/

/*-
 * Copyright (c) 2014 Ryo Shimizu <ryo@nerv.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * i.MX6 10/100/1000-Mbps ethernet MAC (ENET)
 */

#ifndef _ARM_IMX_IF_ENETREG_H_
#define _ARM_IMX_IF_ENETREG_H_

#include <sys/cdefs.h>

#define ENET_EIR			0x00000004
# define ENET_EIR_BABR			__BIT(30)
# define ENET_EIR_BABT			__BIT(29)
# define ENET_EIR_GRA			__BIT(28)
# define ENET_EIR_TXF			__BIT(27)
# define ENET_EIR_TXB			__BIT(26)
# define ENET_EIR_RXF			__BIT(25)
# define ENET_EIR_RXB			__BIT(24)
# define ENET_EIR_MII			__BIT(23)
# define ENET_EIR_EBERR			__BIT(22)
# define ENET_EIR_LC			__BIT(21)
# define ENET_EIR_RL			__BIT(20)
# define ENET_EIR_UN			__BIT(19)
# define ENET_EIR_PLR			__BIT(18)
# define ENET_EIR_WAKEUP		__BIT(17)
# define ENET_EIR_TS_AVAIL		__BIT(16)
# define ENET_EIR_TS_TIMER		__BIT(15)
#define ENET_EIMR			0x00000008
#define ENET_RDAR			0x00000010
# define ENET_RDAR_ACTIVE		__BIT(24)
#define ENET_TDAR			0x00000014
# define ENET_TDAR_ACTIVE		__BIT(24)

#define ENET_ECR			0x00000024
# define ENET_ECR_DBSWP			__BIT(8)
# define ENET_ECR_STOPEN		__BIT(7)
# define ENET_ECR_DBGEN			__BIT(6)
# define ENET_ECR_SPEED			__BIT(5)
# define ENET_ECR_EN1588		__BIT(4)
# define ENET_ECR_SLEEP			__BIT(3)
# define ENET_ECR_MAGICEN		__BIT(2)
# define ENET_ECR_ETHEREN		__BIT(1)
# define ENET_ECR_RESET			__BIT(0)
#define ENET_MMFR			0x00000040
# define ENET_MMFR_ST			0x40000000
# define ENET_MMFR_OP_FORCEWRITE	0x00000000
# define ENET_MMFR_OP_WRITE		0x10000000
# define ENET_MMFR_OP_READ		0x20000000
# define ENET_MMFR_OP_FORCEREAD		0x30000000
# define ENET_MMFR_TA			0x00020000
# define ENET_MMFR_PHY_ADDR(phy)	__SHIFTIN(phy, __BITS(27, 23))
# define ENET_MMFR_PHY_REG(reg)		__SHIFTIN(reg, __BITS(22, 18))
# define ENET_MMFR_DATAMASK		0x0000ffff
#define ENET_MSCR			0x00000044
# define ENET_MSCR_HOLDTIME_1CLK	0x00000000
# define ENET_MSCR_HOLDTIME_2CLK	0x00000100
# define ENET_MSCR_HOLDTIME_3CLK	0x00000200
# define ENET_MSCR_HOLDTIME_8CLK	0x00000700
# define ENET_MSCR_DIS_PRE		__BIT(7)
# define ENET_MSCR_MII_SPEED_25MHZ	__SHIFTIN(4, __BITS(6, 1))
# define ENET_MSCR_MII_SPEED_33MHZ	__SHIFTIN(6, __BITS(6, 1))
# define ENET_MSCR_MII_SPEED_40MHZ	__SHIFTIN(7, __BITS(6, 1))
# define ENET_MSCR_MII_SPEED_50MHZ	__SHIFTIN(9, __BITS(6, 1))
# define ENET_MSCR_MII_SPEED_66MHZ	__SHIFTIN(13, __BITS(6, 1))

#define ENET_MIBC			0x00000064
# define ENET_MIBC_MIB_DIS		__BIT(31)
# define ENET_MIBC_MIB_IDLE		__BIT(30)
# define ENET_MIBC_MIB_CLEAR		__BIT(29)

#define ENET_RCR			0x00000084
# define ENET_RCR_GRS			__BIT(31)
# define ENET_RCR_NLC			__BIT(30)
# define ENET_RCR_MAX_FL(n)		__SHIFTIN(n, __BITS(29, 16))
# define ENET_RCR_CFEN			__BIT(15)
# define ENET_RCR_CRCFWD		__BIT(14)
# define ENET_RCR_PAUFWD		__BIT(13)
# define ENET_RCR_PADEN			__BIT(12)
# define ENET_RCR_RMII_10T		__BIT(9)
# define ENET_RCR_RGMII_EN		__BIT(6)
# define ENET_RCR_FCE			__BIT(5)
# define ENET_RCR_PROM			__BIT(3)
# define ENET_RCR_DRT			__BIT(1)

#define ENET_TCR			0x000000c4
# define ENET_TCR_FDEN			__BIT(2)

#define ENET_PALR			0x000000e4
#define ENET_PAUR			0x000000e8
#define ENET_OPD			0x000000ec
#define ENET_IAUR			0x00000118
#define ENET_IALR			0x0000011c
#define ENET_GAUR			0x00000120
#define ENET_GALR			0x00000124
#define ENET_TFWR			0x00000144
# define ENET_TFWR_STRFWD		__BIT(8)
# define ENET_TFWR_FIFO(n)		__SHIFTIN(((n) / 64), __BITS(5, 0))
#define ENET_RDSR			0x00000180
#define ENET_TDSR			0x00000184
#define ENET_MRBR			0x00000188
#define ENET_RSFL			0x00000190
#define ENET_RSEM			0x00000194
#define ENET_RAEM			0x00000198
#define ENET_RAFL			0x0000019c
#define ENET_TSEM			0x000001a0
#define ENET_TAEM			0x000001a4
#define ENET_TAFL			0x000001a8
#define ENET_TIPG			0x000001ac
#define ENET_FTRL			0x000001b0
#define ENET_TACC			0x000001c0
# define ENET_TACC_PROCHK		__BIT(4)
# define ENET_TACC_IPCHK		__BIT(3)
# define ENET_TACC_SHIFT16		__BIT(0)
#define ENET_RACC			0x000001c4
# define ENET_RACC_SHIFT16		__BIT(7)
# define ENET_RACC_LINEDIS		__BIT(6)
# define ENET_RACC_PRODIS		__BIT(2)
# define ENET_RACC_IPDIS		__BIT(1)
# define ENET_RACC_PADREM		__BIT(0)

/* Statistics counters */
#define ENET_RMON_T_DROP		0x00000200
#define ENET_RMON_T_PACKETS		0x00000204
#define ENET_RMON_T_BC_PKT		0x00000208
#define ENET_RMON_T_MC_PKT		0x0000020c
#define ENET_RMON_T_CRC_ALIGN		0x00000210
#define ENET_RMON_T_UNDERSIZE		0x00000214
#define ENET_RMON_T_OVERSIZE		0x00000218
#define ENET_RMON_T_FRAG		0x0000021c
#define ENET_RMON_T_JAB			0x00000220
#define ENET_RMON_T_COL			0x00000224
#define ENET_RMON_T_P64			0x00000228
#define ENET_RMON_T_P65TO127N		0x0000022c
#define ENET_RMON_T_P128TO255N		0x00000230
#define ENET_RMON_T_P256TO511		0x00000234
#define ENET_RMON_T_P512TO1023		0x00000238
#define ENET_RMON_T_P1024TO2047		0x0000023c
#define ENET_RMON_T_P_GTE2048		0x00000240
#define ENET_RMON_T_OCTETS		0x00000244
#define ENET_IEEE_T_DROP		0x00000248
#define ENET_IEEE_T_FRAME_OK		0x0000024c
#define ENET_IEEE_T_1COL		0x00000250
#define ENET_IEEE_T_MCOL		0x00000254
#define ENET_IEEE_T_DEF			0x00000258
#define ENET_IEEE_T_LCOL		0x0000025c
#define ENET_IEEE_T_EXCOL		0x00000260
#define ENET_IEEE_T_MACERR		0x00000264
#define ENET_IEEE_T_CSERR		0x00000268
#define ENET_IEEE_T_SQE			0x0000026c
#define ENET_IEEE_T_FDXFC		0x00000270
#define ENET_IEEE_T_OCTETS_OK		0x00000274
#define ENET_RMON_R_PACKETS		0x00000284
#define ENET_RMON_R_BC_PKT		0x00000288
#define ENET_RMON_R_MC_PKT		0x0000028c
#define ENET_RMON_R_CRC_ALIGN		0x00000290
#define ENET_RMON_R_UNDERSIZE		0x00000294
#define ENET_RMON_R_OVERSIZE		0x00000298
#define ENET_RMON_R_FRAG		0x0000029c
#define ENET_RMON_R_JAB			0x000002a0
#define ENET_RMON_R_RESVD_0		0x000002a4
#define ENET_RMON_R_P64			0x000002a8
#define ENET_RMON_R_P65TO127		0x000002ac
#define ENET_RMON_R_P128TO255		0x000002b0
#define ENET_RMON_R_P256TO511		0x000002b4
#define ENET_RMON_R_P512TO1023		0x000002b8
#define ENET_RMON_R_P1024TO2047		0x000002bc
#define ENET_RMON_R_P_GTE2048		0x000002c0
#define ENET_RMON_R_OCTETS		0x000002c4
#define ENET_IEEE_R_DROP		0x000002c8
#define ENET_IEEE_R_FRAME_OK		0x000002cc
#define ENET_IEEE_R_CRC			0x000002d0
#define ENET_IEEE_R_ALIGN		0x000002d4
#define ENET_IEEE_R_MACERR		0x000002d8
#define ENET_IEEE_R_FDXFC		0x000002dc
#define ENET_IEEE_R_OCTETS_OK		0x000002e0

/* IEEE1588 control */
#define ENET_ATCR			0x00000400
#define ENET_ATVR			0x00000404
#define ENET_ATOFF			0x00000408
#define ENET_ATPER			0x0000040c
#define ENET_ATCOR			0x00000410
#define ENET_ATINC			0x00000414
#define ENET_ATSTMP			0x00000418

/* Capture/compare block */
#define ENET_TGSR			0x00000604
#define ENET_TCSR0			0x00000608
#define ENET_TCCR0			0x0000060c
#define ENET_TCSR1			0x00000610
#define ENET_TCCR1			0x00000614
#define ENET_TCSR2			0x00000618
#define ENET_TCCR2			0x0000061c
#define ENET_TCSR3			0x00000620
#define ENET_TCCR3			0x00000624

/* enhanced transmit buffer descriptor */
struct enet_txdesc {
	uint32_t tx_flags1_len;
#define TXFLAGS1_R			__BIT(31)	/* Ready */
#define TXFLAGS1_T1			__BIT(30)	/* TX software owner1 */
#define TXFLAGS1_W			__BIT(29)	/* Wrap */
#define TXFLAGS1_T2			__BIT(28)	/* TX software owner2 */
#define TXFLAGS1_L			__BIT(27)	/* Last in frame */
#define TXFLAGS1_TC			__BIT(26)	/* Transmit CRC */
#define TXFLAGS1_ABC			__BIT(25)	/* Append bad CRC */
#define TXFLAGS1_LEN(n)			((n) & 0xffff)
	uint32_t tx_databuf;
	uint32_t tx_flags2;
#define TXFLAGS2_INT			__BIT(30)	/* Interrupt */
#define TXFLAGS2_TS			__BIT(29)	/* Timestamp */
#define TXFLAGS2_PINS			__BIT(28)	/* Insert Proto csum */
#define TXFLAGS2_IINS			__BIT(27)	/* Insert IP csum */
#define TXFLAGS2_TXE			__BIT(15)	/* Transmit error */
#define TXFLAGS2_UE			__BIT(13)	/* Underflow error */
#define TXFLAGS2_EE			__BIT(12)	/* Excess colls Err */
#define TXFLAGS2_FE			__BIT(11)	/* Frame Error */
#define TXFLAGS2_LCE			__BIT(10)	/* Late collision Err */
#define TXFLAGS2_OE			__BIT(9)	/* Overfow Error */
#define TXFLAGS2_TSE			__BIT(8)	/* Timestamp Error */
	uint32_t tx__reserved1;
	uint32_t tx_flags3;
#define TXFLAGS3_BDU			__BIT(31)
	uint32_t tx_1588timestamp;
	uint32_t tx__reserved2;
	uint32_t tx__reserved3;
} __packed;

/* enhanced receive buffer descriptor */
struct enet_rxdesc {
	uint32_t rx_flags1_len;
#define RXFLAGS1_E			__BIT(31)	/* Empty */
#define RXFLAGS1_R1			__BIT(30)	/* RX software owner1 */
#define RXFLAGS1_W			__BIT(29)	/* Wrap */
#define RXFLAGS1_R2			__BIT(28)	/* RX software owner2 */
#define RXFLAGS1_L			__BIT(27)	/* Last in frame */
#define RXFLAGS1_M			__BIT(24)	/* Miss */
#define RXFLAGS1_BC			__BIT(23)	/* Broadcast */
#define RXFLAGS1_MC			__BIT(22)	/* Multicast */
#define RXFLAGS1_LG			__BIT(21)	/* Length Violation */
#define RXFLAGS1_NO			__BIT(20)	/* Non-Octet aligned  */
#define RXFLAGS1_CR			__BIT(18)	/* CRC or frame error */
#define RXFLAGS1_OV			__BIT(17)	/* Overrun */
#define RXFLAGS1_TR			__BIT(16)	/* Truncated */
#define RXFLAGS1_LEN(n)			((n) & 0xffff)
	uint32_t rx_databuf;
	uint32_t rx_flags2;
#define RXFLAGS2_ME			__BIT(31)	/* MAC error */
#define RXFLAGS2_PE			__BIT(26)	/* PHY error */
#define RXFLAGS2_CE			__BIT(25)	/* Collision */
#define RXFLAGS2_UC			__BIT(24)	/* Unicast */
#define RXFLAGS2_INT			__BIT(23)	/* RXB/RXF interrupt */
#define RXFLAGS2_ICE			__BIT(5)	/* IP csum error */
#define RXFLAGS2_PCR			__BIT(4)	/* Proto csum error */
#define RXFLAGS2_VLAN			__BIT(2)	/* VLAN */
#define RXFLAGS2_IPV6			__BIT(1)	/* IPv6 frame */
#define RXFLAGS2_FRAG			__BIT(0)	/* IPv4 fragment */
#if _BYTE_ORDER == _LITTLE_ENDIAN
	uint16_t rx_cksum;
	uint8_t rx_proto;
	uint8_t rx_hl;
#else
	uint8_t rx_hl;
	uint8_t rx_proto;
	uint16_t rx_cksum;
#endif
	uint32_t rx_flags3;
#define RXFLAGS3_BDU			__BIT(31)
	uint32_t rx_1588timestamp;
	uint32_t rx__reserved2;
	uint32_t rx__reserved3;
} __packed;

#endif /* _ARM_IMX_IF_ENETREG_H_ */
