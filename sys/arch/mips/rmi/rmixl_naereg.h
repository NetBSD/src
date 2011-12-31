/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _MIPS_RMI_RMIXL_NAEREG_H
#define _MIPS_RMI_RMIXL_NAEREG_H

/*
 * RX P2P Descriptor (slightly different betewen XLS/XLR and XLP).
 */
#define	RMIXLS_NEA_RXD_EOP			__BIT(63)
#define	RMIXLS_NEA_RXD_STATUS			__BITS(62,56)
#define	RMIXLS_NEA_RXD_CLASSID			__BITS(55,54)
#define	RMIXLP_NEA_RXD_CONTEXT			__BITS(59,54)
#define	RMIXL_NEA_RXD_LENGTH			__BITS(53,40)
/*
 * L2 cacheline aligned address
 */
#define	RMIXL_NEA_RXD_ADDRESS			__BITS(39,5)
#define	RMIXLP_NEA_RXD_UP			__BIT(5)
#define	RMIXLP_NEA_RXD_ERR			__BIT(4)
#define	RMIXLS_NEA_RXD_UP			__BIT(4)
#define	RMIXLS_NEA_RXD_PORTID			__BIT(3,0)
#define	RMIXLP_NEA_RXD_IC			__BIT(3) // IP CSUM valid
#define	RMIXLP_NEA_RXD_TC			__BIT(2) // TCP CSUM valid
#define	RMIXLP_NEA_RXD_PP			__BIT(1) // Prepad present
#define	RMIXLP_NEA_RXD_P2P			__BIT(0)

/*
 * RXD Status field for XLS/XLR.
 */
#define	RMIXLS_RXD_STATUS_ERROR			__BIT(6)
#define	RMIXLS_RXD_STATUS_OK_BROADCAST		__BIT(5)
#define	RMIXLS_RXD_STATUS_OK_MULTICAST		__BIT(4)
#define	RMIXLS_RXD_STATUS_OK_UNICAST		__BIT(3)
#define	RMIXLS_RXD_STATUS_ERROR_CODE		__BIT(2)
#define	RMIXLS_RXD_STATUS_ERROR_CRC		__BIT(1)
#define	RMIXLS_RXD_STATUS_OK_MACADDR		__BITS(2,1)
#define	RMIXLS_RXD_STATUS_ERROR_LENGTH		__BIT(0)
#define	RMIXLS_RXD_STATUS_OK_VLAN		__BIT(0)

#define	RMIXL_NEA_TXD_TYPE			__BITS(63,62)
#define	RMIXL_NEA_TXD_RDEX			__BIT(61)
#define	RMIXL_NEA_TXD_VFBID			__BITS(60,54)
#define	RMIXL_NEA_TXD_LENGTH			__BITS(53,40)
#define	RMIXL_NEA_TXD_ADDRESS			__BITS(39,0)

#define	RMIXL_NEA_DESC_TYPE_P2D			0	/* Not End Of Packet */
#define	RMIXL_NEA_DESC_TYPE_P2DE		1	/* End Of Packet */
#define	RMIXL_NEA_DESC_TYPE_P2P			2	/* Ptr to Pointers */
#define	RMIXL_NEA_DESC_TYPE_MSC			3	/* MicroStruct */

#define	RMIXL_NEA_MSCD1_TYPE			__BITS(63,62)
#define	RMIXL_NEA_MSCD1_SUBTYPE			__BITS(61,60)
#define	RMIXL_NEA_MSCD1_OPCODE			__BITS(59,56)
#define	RMIXL_NEA_MSCD1__RSVRD			__BITS(55,49)
#define	RMIXL_NEA_MSCD1_IP_FCOE_START		__BITS(48,43)
#define	RMIXL_NEA_MSCD1_TCP_UDP_SCTP_START	__BITS(42,36)
#define	RMIXL_NEA_MSCD1_IP_CSUM_OFFS		__BITS(35,31)
#define	RMIXL_NEA_MSCD1_PSEUDO_HDR_SUM		__BITS(30,15)
#define	RMIXL_NEA_MSCD1_TCP_UDP_CSUM_OFFS	__BITS(14,8)
#define	RMIXL_NEA_MSCD1_PAYLOAD_OFFS		__BITS(7,0)

#define	RMIXL_NEA_MCSD1_SUBTYPE_1_OF_1		0
#define	RMIXL_NEA_MCSD1_SUBTYPE_1_OF_2		1
#define	RMIXL_NEA_MCSD1_SUBTYPE_2_OF_2		2

#define	RMIXL_NEA_MSCD1_OPCODE_IP_CSUM		1
#define	RMIXL_NEA_MSCD1_OPCODE_TCP_CSUM		2
#define	RMIXL_NEA_MSCD1_OPCODE_UDP_CSUM		3
#define	RMIXL_NEA_MSCD1_OPCODE_SCTP_CRC		4
#define	RMIXL_NEA_MSCD1_OPCODE_FCoE_CRC		5
#define	RMIXL_NEA_MSCD1_OPCODE_IP_TCP_CSUM	6
#define	RMIXL_NEA_MSCD1_OPCODE_IP_CSUM_TOS	7
#define	RMIXL_NEA_MSCD1_OPCODE_IP_UDP_CSUM	8
#define	RMIXL_NEA_MSCD1_OPCODE_IP_SCTP_CSUM	9

#define	RMIXL_NEA_MSCD2_TYPE			__BITS(63,62)
#define	RMIXL_NEA_MSCD2_SUBTYPE			__BITS(61,60)
#define	RMIXL_NEA_MSCD2__RSRVD			__BITS(59,50)
#define	RMIXL_NEA_MSCD2_POLYNUM			__BITS(49,48)
#define	RMIXL_NEA_MSCD2_MSS			__BITS(47,32) /* MSS - L1/L2/L3 */
#define	RMIXL_NEA_MSCD2_CRC_STOP_OFFS		__BITS(31,16)
#define	RMIXL_NEA_MSCD2_CRC_INS_OFFS		__BITS(15,0)

#define	RMIXL_NEA_TXFBD_TYPE			__BITS(63,62)
#define	RMIXLP_NEA_TXFBD_RDX			__BIT(61)
#define	RMIXLP_NEA_TXFBD__RSRVD0		__BITS(60,58)
#define	RMIXLS_NEA_TXFBD_COLLISION		__BIT(61)
#define	RMIXLS_NEA_TXFBD_BUS_ERROR		__BIT(60)
#define	RMIXLS_NEA_TXFBD_UNDERRUN		__BIT(59)
#define	RMIXLS_NEA_TXFBD_ABORT			__BIT(58)
#define	RMIXLP_NEA_TXFBD_TS_VALID		__BIT(57)
#define	RMIXLP_NEA_TXFBD_TX_DONE		__BIT(56)
#define	RMIXLP_NEA_TXFBD_MAX_COLL_LATE_ABORT	__BIT(55)
#define	RMIXLP_NEA_TXFBD_UNDERRUN		__BIT(54)
#define	RMIXLS_NEA_TXFBD_PORT_ID		__BITS(57,54)
#define	RMIXLP_NEA_TXFBD__RSRVD1		__BITS(53,50)
#define	RMIXLS_NEA_TXFBD_LENGTH			__BITS(53,40) // always 0
#define	RMIXL_NEA_TXFBD_CONTEXT			__BITS(49,40)
#define	RMIXL_NEA_TXFBD_ADDRESS			__BITS(39,0)

#define	RMIXL_NEA_RXFID__RSRVD0			__BITS(63,40)
#define	RMIXL_NEA_RXFID_ADDRESS			__BITS(39,6)
#define	RMIXL_NEA_RXFID__RSRVD1			__BITS(5,0)

#define	RMIXL_NAE_GMAC0_BASE(n)			(0x0000 + 0x2000*(n))
#define	RMIXL_NAE_GMAC1_BASE(n)			(0x0200 + 0x2000*(n))
#define	RMIXL_NAE_GMAC2_BASE(n)			(0x0400 + 0x2000*(n))
#define	RMIXL_NAE_GMAC3_BASE(n)			(0x0600 + 0x2000*(n))
#define	RMIXL_NAE_XGMAC0_BASE(n)		(0x0800 + 0x2000*(n))
#define	RMIXL_NAE_PHY_BASE(n)			(0x1e00 + 0x2000*(n))
#define	RMIXL_NAE_BASE(n)			0xe000

/* SGMII (GMAC) Registers */

#define RMIXL_GMAC_CONF1		_RMIXL_OFFSET(0x00) /* MAC Configuration 1 */
#define RMIXL_GMAC_CONF2		_RMIXL_OFFSET(0x01) /* MAC Configuration 2 */
#define RMIXL_GMAC_IPG_IFG		_RMIXL_OFFSET(0x02) /* Interpacket Gap/IFG */
#define RMIXL_GMAC_HLF_DUP		_RMIXL_OFFSET(0x03) /* Half-Duplex Register */
#define RMIXL_GMAC_MAX_FRM		_RMIXL_OFFSET(0x04) /* Maximum Frame Length */
#define RMIXL_GMAC_TEST			_RMIXL_OFFSET(0x07) /* 0x05-x06 Reserved */
					/* 0x10 - 0x1E Reserved */
/* 0x1F-0x4F GMAC statistics */
#define	RMIXL_GMAC_MLR			_RMIXL_OFFSET(0x1F) /* MSB Latching */

					/* Transmit and Receive Counters */
#define	RMIXL_GMAC_TR64			_RMIXL_OFFSET(0x20) /* Transmit & Receive 64 Byte Frame Counter */
#define	RMIXL_GMAC_TR127		_RMIXL_OFFSET(0x21) /* Transmit & Receive 65 to 127 Byte Frame Counter */
#define	RMIXL_GMAC_TR255		_RMIXL_OFFSET(0x22) /* Transmit & Receive 128 to 255 Byte Frame Counter */
#define	RMIXL_GMAC_TR511		_RMIXL_OFFSET(0x23) /* Transmit & Receive 256 to 511 Byte Frame Counter */
#define	RMIXL_GMAC_TR1K			_RMIXL_OFFSET(0x24) /* Transmit & Receive 512 to 1023 Byte Frame Counter */
#define	RMIXL_GMAC_TRMAX		_RMIXL_OFFSET(0x25) /* Transmit & Receive 1024 to 1518 Byte Frame Counter */
#define	RMIXL_GMAC_TRMGV		_RMIXL_OFFSET(0x26) /* Transmit & Receive 1519 to 1522 Byte VLAN Frame Counter */

					/* Receive Counters */
#define	RMIXL_GMAC_RBYT			_RMIXL_OFFSET(0x27) /* Receive Byte Counter */
#define	RMIXL_GMAC_RPKT			_RMIXL_OFFSET(0x28) /* Receive Packet Counter */
#define	RMIXL_GMAC_RFCS			_RMIXL_OFFSET(0x29) /* Receive FCS Error Counter */
#define	RMIXL_GMAC_RMCA			_RMIXL_OFFSET(0x2A) /* Receive Multicast Packet Counter */
#define	RMIXL_GMAC_RBCA			_RMIXL_OFFSET(0x2B) /* Receive Broadcast Packet Counter */
#define	RMIXL_GMAC_RXCF			_RMIXL_OFFSET(0x2C) /* Receive Control Frame Packet Counter */
#define	RMIXL_GMAC_RXPF			_RMIXL_OFFSET(0x2D) /* Receive PAUSE Frame Packet Counter */
#define	RMIXL_GMAC_RXUO			_RMIXL_OFFSET(0x2E) /* Receive Unknown OPCode Packet Counter */
#define	RMIXL_GMAC_RALN			_RMIXL_OFFSET(0x2F) /* Receive Alignment Error Counter */
#define	RMIXL_GMAC_RFLR			_RMIXL_OFFSET(0x30) /* Receive Frame Length Error Counter */
#define	RMIXL_GMAC_RCDE			_RMIXL_OFFSET(0x31) /* Receive Code Error Counter */
#define	RMIXL_GMAC_RCSE			_RMIXL_OFFSET(0x32) /* Receive Carrier Sense Error Counter */
#define	RMIXL_GMAC_RUND			_RMIXL_OFFSET(0x33) /* Receive Undersize Packet Counter */
#define	RMIXL_GMAC_ROVR			_RMIXL_OFFSET(0x34) /* Receive Oversize Packet Counter */
#define	RMIXL_GMAC_RFRG			_RMIXL_OFFSET(0x35) /* Receive Fragments Counter */
#define	RMIXL_GMAC_RJBR			_RMIXL_OFFSET(0x36) /* Receive Jabber Counter */

						/* Transmit Counters */
#define	RMIXL_GMAC_TBYT			_RMIXL_OFFSET(0x38) /* Transmit Byte Counter */
#define	RMIXL_GMAC_TPKT			_RMIXL_OFFSET(0x39) /* Transmit Packet Counter */
#define	RMIXL_GMAC_TMCA			_RMIXL_OFFSET(0x3A) /* Transmit Multicast Packet Counter */
#define	RMIXL_GMAC_TBCA			_RMIXL_OFFSET(0x3B) /* Transmit Broadcast Packet Counter */
#define	RMIXL_GMAC_TXPF			_RMIXL_OFFSET(0x3C) /* Transmit PAUSE Control Frame Counter */
#define	RMIXL_GMAC_TDFR			_RMIXL_OFFSET(0x3D) /* Transmit Deferral Packet Counter */
#define	RMIXL_GMAC_TEDF			_RMIXL_OFFSET(0x3E) /* Transmit Excessive Deferral Packet Counter */
#define	RMIXL_GMAC_TSCL			_RMIXL_OFFSET(0x3F) /* Transmit Single Collision Packet Counter */
#define	RMIXL_GMAC_TMCL			_RMIXL_OFFSET(0x40) /* Transmit Multiple Collision Packet Counter */
#define	RMIXL_GMAC_TLCL			_RMIXL_OFFSET(0x41) /* Transmit Late Collision Packet Counter */
#define	RMIXL_GMAC_TXCL			_RMIXL_OFFSET(0x42) /* Transmit Excessive Collision Packet Counter */
#define	RMIXL_GMAC_TNCL			_RMIXL_OFFSET(0x43) /* Transmit Total Collision Counter */
#define	RMIXL_GMAC_TJBR			_RMIXL_OFFSET(0x46) /* Transmit Jabber Frame Counter */
#define	RMIXL_GMAC_TFCS			_RMIXL_OFFSET(0x47) /* Transmit FCS Error Counter */
#define	RMIXL_GMAC_TXCF			_RMIXL_OFFSET(0x48) /* Transmit Control Frame Counter */
#define	RMIXL_GMAC_TOVR			_RMIXL_OFFSET(0x49) /* Transmit Oversize Frame Counter */
#define	RMIXL_GMAC_TUND			_RMIXL_OFFSET(0x4A) /* Transmit Undersize Frame Counter */
#define	RMIXL_GMAC_TFRG			_RMIXL_OFFSET(0x4B) /* Transmit Fragment Counter */

					/* General Registers */
#define	RMIXL_GMAC_CAR1			_RMIXL_OFFSET(0x4C) /* Carry 1 */
#define	RMIXL_GMAC_CAR2			_RMIXL_OFFSET(0x4D) /* Carry 2 */
#define	RMIXL_GMAC_CAM1			_RMIXL_OFFSET(0x4E) /* Carry Mask 1 */
#define	RMIXL_GMAC_CAM2			_RMIXL_OFFSET(0x4F) /* Carry Mask 2 */

#define RMIXL_GMAC_ADDR0_LO		_RMIXL_OFFSET(0x50) /* Station Address 0 Four-Most-Significant-Bytes */
#define RMIXL_GMAC_ADDR0_H		_RMIXL_OFFSET(0x51) /* Station Address 0 Two-Least-Significant-Bytes
#define RMIXL_GMAC_ADDR1_LO		_RMIXL_OFFSET(0x52) /* Station Address 1 Four-Most-Significant-Bytes
#define RMIXL_GMAC_ADDR1_H		_RMIXL_OFFSET(0x53) /* Station Address 1 Two-Least-Significant-Bytes
#define RMIXL_GMAC_ADDR2_LO		_RMIXL_OFFSET(0x54) /* Station Address 2 Four-Most-Significant-Bytes
#define RMIXL_GMAC_ADDR2_H		_RMIXL_OFFSET(0x55) /* Station Address 2 Two-Least-Significant-Bytes
#define RMIXL_GMAC_ADDR3_LO		_RMIXL_OFFSET(0x56) /* Station Address 3 Four-Most-Significant-Bytes
#define RMIXL_GMAC_ADDR3_H		_RMIXL_OFFSET(0x57) /* Station Address 3 Two-Least-Significant-Bytes
#define	RMIXL_GMAC_ADDR_MASK0_LO	_RMIXL_OFFSET(0x58)
#define	RMIXL_GMAC_ADDR_MASK0_HI	_RMIXL_OFFSET(0x59)
#define	RMIXL_GMAC_ADDR_MASK1_LO	_RMIXL_OFFSET(0x5A)
#define	RMIXL_GMAC_ADDR_MASK1_HI	_RMIXL_OFFSET(0x5B)
#define	RMIXL_GMAC_FILTER_CONFIG	_RMIXL_OFFSET(0x5C)
#define	RMIXL_GMAC_HASH_TABLE_VECTOR	_RMIXL_OFFSET(0x60+(n))

#define RMIXL_GMAC_CONF1_SOFT_RESET		__BIT(31)
#define	RMIXL_GMAC_CONF1_LOOP_BACK		__BIT(8) /* Loop Back control. */
#define RMIXL_GMAC_CONF1_RX_FLOWCTL_ENABLE	__BIT(5)
#define RMIXL_GMAC_CONF1_TX_FLOWCTL_ENABLE	__BIT(4)
#define RMIXL_GMAC_CONF1_SYNC_RX_ENABLE		__BIT(3)
#define RMIXL_GMAC_CONF1_RX_ENABLE		__BIT(2)
#define RMIXL_GMAC_CONF1_SYNC_TX_ENABLE		__BIT(1)
#define RMIXL_GMAC_CONF1_TX_ENABLE		__BIT(0)

#define RMIXL_GMAC_CONF2_PREAMBLE_LEN		__BITS(15,12) /* Length of the preamble field of the packet, in bytes. */
#define RMIXL_GMAC_CONF2_BYTE_MODE		__BIT(9)
#define RMIXL_GMAC_CONF2_NIBBLE_MODE		__BIT(8)
#define RMIXL_GMAC_CONF2_802LENCHK		__BIT(4)
#define RMIXL_GMAC_CONF2_PADCRC_ENABLE		__BIT(2)
#define RMIXL_GMAC_CONF2_CRC_ENABLE		__BIT(1)
#define RMIXL_GMAC_CONF2_FULL_DUPLEX		__BIT(0)

#define RMIXL_GMAC_IFG_IFG_NBBIPG_1		__BITS(30,24)
#define RMIXL_GMAC_IFG_IFG_NBBIPG_2		__BITS(22,16)
#define RMIXL_GMAC_IFG_IFG_MIN_IFG_ENFORCEMTN	__BITS(15,8)
#define RMIXL_GMAC_IFG_IFG_BBIPG		__BITS(6,0)

#define RMIXL_GMAC_HFL_DUP_ABEBT		__BITS(24,20)
#define RMIXL_GMAC_HFL_DUP_ABEBE		__BIT(19)
#define RMIXL_GMAC_HFL_DUP_BP_NB		__BIT(18)
#define RMIXL_GMAC_HFL_DUP_NO_BACKOFF		__BIT(17)
#define RMIXL_GMAC_HFL_DUP_EX_DEF		__BIT(16)
#define RMIXL_GMAC_HFL_DUP_COLLISION_WIN	__BITS(9:0)

#define RMIXL_GMAC_MAX_FRM_LENGTH		__BITS(15:0)
#define RMIXL_GMAC_TEST_MAX_BACKOFF		__BIT(3)
#define RMIXL_GMAC_TEST_REG_TX_FLOW_EN		__BIT(2)
#define RMIXL_GMAC_TEST_TEST_PAUSE		__BIT(1)
#define RMIXL_GMAC_TEST_SHORT_SLOT_TIME		__BIT(0)

#define	RMIXL_GMAC_CAR1_TR64		__BIT(31) /* TR64 Counter Carry bit */
#define	RMIXL_GMAC_CAR1_TR127		__BIT(30) /* TR127 Counter Carry bit */
#define	RMIXL_GMAC_CAR1_TR255		__BIT(29) /* TR255 Counter Carry bit */
#define	RMIXL_GMAC_CAR1_TR511		__BIT(28) /* TR511 Counter Carry bit */
#define	RMIXL_GMAC_CAR1_TR1K		__BIT(27) /* TR1K Counter Carry bit */
#define	RMIXL_GMAC_CAR1_TRMAX		__BIT(26) /* TRMAX Counter Carry bit */
#define	RMIXL_GMAC_CAR1_TRMGV		__BIT(25) /* TRMGV Counter Carry bit */
	
#define	RMIXL_GMAC_CAR1_RBYT		__BIT(16) /* RBYT Counter Carry bit */
#define	RMIXL_GMAC_CAR1_RPKT		__BIT(15) /* RPKT Counter Carry bit */
#define	RMIXL_GMAC_CAR1_RFCS		__BIT(14) /* RFCS Counter Carry bit */
#define	RMIXL_GMAC_CAR1_RMCA		__BIT(13) /* RMCA Counter Carry bit */
#define	RMIXL_GMAC_CAR1_RBCA		__BIT(12) /* RBCA Counter Carry bit */
#define	RMIXL_GMAC_CAR1_RXCF		__BIT(11) /* RXCF Counter Carry bit */
#define	RMIXL_GMAC_CAR1_RXPF		__BIT(10) /* RXPF Counter Carry bit */
#define	RMIXL_GMAC_CAR1_RXU0		__BIT(9) /* RXUO Counter Carry bit */
#define	RMIXL_GMAC_CAR1_RALN		__BIT(8) /* RALN Counter Carry bit */
#define	RMIXL_GMAC_CAR1_RFLR		__BIT(7) /* RFLR Counter Carry bit */
#define	RMIXL_GMAC_CAR1_RCDE		__BIT(6) /* RCDE Counter Carry bit */
#define	RMIXL_GMAC_CAR1_RCSE		__BIT(5) /* RCSE Counter Carry bit */
#define	RMIXL_GMAC_CAR1_RUND		__BIT(4) /* RUND Counter Carry bit */
#define	RMIXL_GMAC_CAR1_ROVR		__BIT(3) /* ROVR Counter Carry bit */
#define	RMIXL_GMAC_CAR1_RFRG		__BIT(2) /* RFRG Counter Carry bit */
#define	RMIXL_GMAC_CAR1_RJBR		__BIT(1) /* RJBR Counter Carry bit */
#define	RMIXL_GMAC_CAR1_RDRP		__BIT(0) /* RDRP Counter Carry bit */

#define	RMIXL_GMAC_CAR2_TJBR		__BIT(19) /* TJBR Counter Carry bit */
#define RMIXL_GMAC_CAR2_TFCS		__BIT(18) /* TFCS Counter Carry bit */
#define RMIXL_GMAC_CAR2_TXCF		__BIT(17) /* TXCF Counter Carry bit */
#define RMIXL_GMAC_CAR2_TOVR		__BIT(16) /* TOVR Counter Carry bit */
#define RMIXL_GMAC_CAR2_TUND		__BIT(15) /* TUND Counter Carry bit */
#define RMIXL_GMAC_CAR2_TFRG		__BIT(14) /* TFRG Counter Carry bit */
#define RMIXL_GMAC_CAR2_TBYT		__BIT(13) /* TBYT Counter Carry bit */
#define RMIXL_GMAC_CAR2_TPKT		__BIT(12) /* TPKT Counter Carry bit */
#define RMIXL_GMAC_CAR2_TMCA		__BIT(11) /* TMCA Counter Carry bit */
#define RMIXL_GMAC_CAR2_TBCA		__BIT(10) /* TBCA Counter Carry bit */
#define RMIXL_GMAC_CAR2_TXPF		__BIT(9) /* TXPF Counter Carry bit */
#define RMIXL_GMAC_CAR2_TDFR		__BIT(8) /* TDFR Counter Carry bit */
#define RMIXL_GMAC_CAR2_TEDF		__BIT(7) /* TEDF Counter Carry bit */
#define RMIXL_GMAC_CAR2_TSCL		__BIT(6) /* TSCL Counter Carry bit */
#define RMIXL_GMAC_CAR2_TMCL		__BIT(5) /* TMCL Counter Carry bit */
#define RMIXL_GMAC_CAR2_TLCL		__BIT(4) /* TLCL Counter Carry bit */
#define RMIXL_GMAC_CAR2_TXCL		__BIT(3) /* TXCL Counter Carry bit */
#define RMIXL_GMAC_CAR2_TNCL		__BIT(2) /* TNCL Counter Carry bit */
#define RMIXL_GMAC_CAR2_TPFH		__BIT(1) /* TPFH Counter Carry bit */
#define RMIXL_GMAC_CAR2_TDRP		__BIT(0) /* TDRP Counter Carry bit */

static inline uint32_t
rmixl_gmac_station_addr_lo_make(const uint8_t *p)
{
	reutrn (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3] << 0);
}

static inline uint32_t
rmixl_gmac_station_addr_lo_make(const uint8_t *p)
{
	reutrn (p[4] << 24) | (p[5] << 16);
}

#define RMIXL_GMAC_FILTER_CONFIG_BR_EN		__BIT(10) /* Broadcast Enable
#define RMIXL_GMAC_FILTER_CONFIG_PF_EN		__BIT(9) /* Pause Frame Enable */
#define RMIXL_GMAC_FILTER_CONFIG_AM_EN		__BIT(8) /* All Multicast Enable */
#define RMIXL_GMAC_FILTER_CONFIG_AU_EN		__BIT(7) /* All Unicast Enable */
#define RMIXL_GMAC_FILTER_CONFIG_HM_EN		__BIT(6) /* Hash Multicast Enable */
#define RMIXL_GMAC_FILTER_CONFIG_HU_EN		__BIT(5) /* Hash Unicast Enable */
#define RMIXL_GMAC_FILTER_CONFIG_AMD		__BIT(4) /* Address Match Discard */
#define RMIXL_GMAC_FILTER_CONFIG_MA3V		__BIT(3) /* MAC Address 3 Valid */
#define RMIXL_GMAC_FILTER_CONFIG_MA2V		__BIT(2) /* MAC Address 2 Valid */
#define RMIXL_GMAC_FILTER_CONFIG_MA1V		__BIT(1) /* MAC Address 1 Valid */
#define RMIXL_GMAC_FILTER_CONFIG_MA0V		__BIT(0) /* MAC Address 0 Valid */

/* NEA Ingress Path Registers */

#define RMIXL_NAE_RX_CONFIG			_RMIXL_OFFSET(0x10) // Receive Configuration
#define RMIXL_NAE_RX_IF_BASE_CONFIG0		_RMIXL_OFFSET(0x12) // Receive Interface Base Config 0
#define RMIXL_NAE_RX_IF_BASE_CONFIG1		_RMIXL_OFFSET(0x13) // Receive Interface Base Config 1
#define RMIXL_NAE_RX_IF_BASE_CONFIG2		_RMIXL_OFFSET(0x14) // Receive Interface Base Config 2
#define RMIXL_NAE_RX_IF_BASE_CONFIG3		_RMIXL_OFFSET(0x15) // Receive Interface Base Config 3
#define RMIXL_NAE_RX_IF_BASE_CONFIG4		_RMIXL_OFFSET(0x16) // Receive Interface Base Config 4
#define RMIXL_NAE_RX_IF_BASE_CONFIG5		_RMIXL_OFFSET(0x17) // Receive Interface Base Config 5
#define RMIXL_NAE_RX_IF_BASE_CONFIG6		_RMIXL_OFFSET(0x18) // Receive Interface Base Config 6
#define RMIXL_NAE_RX_IF_BASE_CONFIG7		_RMIXL_OFFSET(0x19) // Receive Interface Base Config 7
#define RMIXL_NAE_RX_IF_BASE_CONFIG8		_RMIXL_OFFSET(0x1A) // Receive Interface Base Config 8
#define RMIXL_NAE_RX_IF_BASE_CONFIG9		_RMIXL_OFFSET(0x1B) // Receive Interface Base Config 9
#define RMIXL_NAE_RX_IF_VEC_VALID		_RMIXL_OFFSET(0x1C) // Receive Interface Vector Valid
#define RMIXL_NAE_RX_IF_SLOT_CAL		_RMIXL_OFFSET(0x1D) // Receive Calendar Valid Slots
#define RMIXL_NAE_PARSER_CONFIG			_RMIXL_OFFSET(0x1E) // Parser Configuration
#define RMIXL_NAE_PARSER_SEQ_FIFO_CFG		_RMIXL_OFFSET(0x1F) // Parser Sequence FIFO Carvings
#define RMIXL_NAE_FREE_IN_LIFO_CFG		_RMIXL_OFFSET(0x20) // Free-In LIFO Carvings
#define RMIXL_NAE_RX_BUFFER_BASE_DEPTH_ADDR	_RMIXL_OFFSET(0x21) // Receive Buffer Base Depth Carvings Index
#define RMIXL_NAE_RX_BUFFER_BASE_DEPTH		_RMIXL_OFFSET(0x22) // Receive Buffer Base Depth Carvings
#define RMIXL_NAE_RX_UCORE_CFG			_RMIXL_OFFSET(0x23) // Receive Micro-core Configuration Register
#define RMIXL_NAE_RX_UCORE_CAM_MASK0_CFG	_RMIXL_OFFSET(0x24) // Receive Micro-core CAM Mask0 Configuration Registers
#define RMIXL_NAE_RX_UCORE_CAM_MASK1_CFG	_RMIXL_OFFSET(0x25) // Receive Micro-core CAM Mask1 Configuration Registers
#define RMIXL_NAE_RX_UCORE_CAM_MASK2_CFG	_RMIXL_OFFSET(0x26) // Receive Micro-core CAM Mask2 Configuration Registers
#define RMIXL_NAE_RX_UCORE_CAM_MASK3_CFG	_RMIXL_OFFSET(0x27) // Receive Micro-core CAM Mask3 Configuration Registers
#define RMIXL_NAE_FREE_IN_LIFO_UNIQ_SZ_CFG	_RMIXL_OFFSET(0x28) // Free-In LIFO Descriptor Size Register
#define RMIXL_NAE_RX_CRC_POLY0_CFG		_RMIXL_OFFSET(0x2A) // Receive CRC Polynomial Register 0
#define RMIXL_NAE_RX_CRC_POLY1_CFG		_RMIXL_OFFSET(0x2B) // Receive CRC Polynomial Register 1
#define RMIXL_NAE_FREE_SPILL0_MEM_CFG		_RMIXL_OFFSET(0x2C) // Spill Memory Regioning Register 0
#define RMIXL_NAE_FREE_SPILL1_MEM_CFG		_RMIXL_OFFSET(0x2D) // Spill Memory Regioning Register 1
#define RMIXL_NAE_FREE_LIFO_THRESHOLD_CFG	_RMIXL_OFFSET(0x2E) // Free-In LIFO Threshold Configuration Register
#define RMIXL_NAE_FLOW_CRC16_POLY_CFG		_RMIXL_OFFSET(0x2F) // FlowID CRC16 Polynomial Configuration
#define RMIXL_NAE_TEST				_RMIXL_OFFSET(0x5F) // Test Register
#define RMIXL_NAE_BIU_TIMEOUT_CFG		_RMIXL_OFFSET(0x60) // Bus Interface Unit Timeout Configuration
#define RMIXL_NAE_BIU_CFG			_RMIXL_OFFSET(0x61) // Bus Interface Unit Configuration
#define RMIXL_NAE_RX_FREE_LIFO_POP		_RMIXL_OFFSET(0x62) // Receive Free-In LIFO Pop Configuration
#define RMIXL_NAE_RX_DSBL_ECC			_RMIXL_OFFSET(0x63) // Receive Disable ECC
#define RMIXL_NAE_FLOW_BASE_MASK_CFG		_RMIXL_OFFSET(0x80) // FlowID Base Mask Register
#define RMIXL_NAE_POE_CLASS_SETUP_CFG		_RMIXL_OFFSET(0x81) // POE Class Setup Configuration Register
#define RMIXL_NAE_UCO_IFACE_MASK_CFG		_RMIXL_OFFSET(0x82) // Micro-core Interface Mask Configuraton Register
#define RMIXL_NAE_RX_BUFFER_XOFFON_THRESH	_RMIXL_OFFSET(0x83) // Receive Buffer XOFF/XON Threshold
#define RMIXL_NAE_FLOW_TABLE1_CFG		_RMIXL_OFFSET(0x84) // Flow Table 1
#define RMIXL_NAE_FLOW_CLASS_BASE_MASK		_RMIXL_OFFSET(0x85) // Flow Class Base Mask
#define RMIXL_NAE_FLOW_TABLE3_CFG		_RMIXL_OFFSET(0x86) // Flow Table 3
#define RMIXL_NAE_RX_FREE_LIFO_THRESH		_RMIXL_OFFSET(0x87) // Receive Free-In LIFO Threshold
#define RMIXL_NAE_RX_PARSER_UNCLA		_RMIXL_OFFSET(0x88) // Receive Parser Unclassify
#define RMIXL_NAE_RX_BUFF_INTR_THRESH		_RMIXL_OFFSET(0x89) // Receive Buffer Interrupt Threshold
#define RMIXL_NAE_IFACE_FIFO_CFG		_RMIXL_OFFSET(0x8A) // Interface FIFO Carvings Register
#define RMIXL_NAE_PARSER_SEQ_FIFOTH_CFG		_RMIXL_OFFSET(0x8B) // Parser Sequence FIFO Threshold Configuration Register
#define RMIXL_NAE_RX_ERRINJ_CTRL0		_RMIXL_OFFSET(0x8C) // Receive Error Injection Control Register 0
#define RMIXL_NAE_RX_ERRINJ_CTRL1		_RMIXL_OFFSET(0x8D) // Receive Error Injection Control Register 1
#define RMIXL_NAE_RX_ERR_LATCH0			_RMIXL_OFFSET(0x8E) // Receive Error Latch Register 0
#define RMIXL_NAE_RX_ERR_LATCH1			_RMIXL_OFFSET(0x8F) // Receive Error Latch Register 1
#define RMIXL_NAE_RX_PERF_CTR_CFG		_RMIXL_OFFSET(0xA0) // Receive Performance Counter Configuration Register
#define RMIXL_NAE_RX_PERF_CTR_VAL		_RMIXL_OFFSET(0xA1) // Receive Performance Counter Value Register

	// NAE Hardware Parser Registers

#define RMIXL_NAE_L2_TYPE_PORTn(n)		_RMIXL_OFFSET(0x210+(n)) // L2 Type 0-19
#define RMIXL_NAE_L3_CTABLE_MASK0		_RMIXL_OFFSET(0x22C) // L3 CAM Table Masks
#define RMIXL_NAE_L3_CTABLE_MASK1		_RMIXL_OFFSET(0x22D) // L3 CAM Table Masks
#define RMIXL_NAE_L3_CTABLE_MASK2		_RMIXL_OFFSET(0x22E) // L3 CAM Table Masks
#define RMIXL_NAE_L3_CTABLE_MASK3		_RMIXL_OFFSET(0x22F) // L3 CAM Table Masks
#define RMIXL_NAE_L3_CTABLE_EVENn(n)		_RMIXL_OFFSET(0x230+2*(n)) // L3 CAM Table Even 0-15
#define RMIXL_NAE_L3_CTABLE_ODDn(n)		_RMIXL_OFFSET(0x231+2*(n)) // L3 CAM Table Odd 0-15
#define RMIXL_NAE_L4_CTABLEn(n)			_RMIXL_OFFSET(0x250+(n)) // L4 CAM Table 0-15
#define RMIXL_NAE_IPV6_EXT_HEAD0		_RMIXL_OFFSET(0x260) // IPv6 Extension Headers
#define RMIXL_NAE_IPV6_EXT_HEAD1		_RMIXL_OFFSET(0x261) // IPv6 Extension Headers
#define RMIXL_NAE_VLAN_TYPES01			_RMIXL_OFFSET(0x262) // VLAN Headers
#define RMIXL_NAE_VLAN_TYPES23			_RMIXL_OFFSET(0x263) // VLAN Headers

	// Egress Path Registers
#define RMIXL_NAE_TX_CONFIG			_RMIXL_OFFSET(0x11) // Transmit Configuration
#define RMIXL_NAE_DMA_TX_CREDIT_TH		_RMIXL_OFFSET(0x29) // DMA Transmit Credit Threshold
#define RMIXL_NAE_STG1_STG2CRDT_CMD		_RMIXL_OFFSET(0x30) // Stage1 to Stage2 Credit Command
#define RMIXL_NAE_STG2_EHCRDT_CMD		_RMIXL_OFFSET(0x32) // Stage2 to Exit Hold FIFO Credit Command
#define RMIXL_NAE_STG2_FREECRDT_CMD		_RMIXL_OFFSET(0x34) // Stage2 Free FIFO Credit Command
#define RMIXL_NAE_STG2_STRCRDT_CMD		_RMIXL_OFFSET(0x36) // Stage2 to Micro-Struct FIFO Credit Command
#define RMIXL_NAE_VFBID_DESTMAP_CMD		_RMIXL_OFFSET(0x3A) // VFBID to Destination Map Command
#define RMIXL_NAE_STG1_PMEM_PROG		_RMIXL_OFFSET(0x3C) // Stage1 Context Memory Pointer Setup
#define RMIXL_NAE_STG2_PMEM_PROG		_RMIXL_OFFSET(0x3E) // Stage2 Context Memory Pointer Setup
#define RMIXL_NAE_EH_PMEM_PROG			_RMIXL_OFFSET(0x40) // Exit Hold Context Memory Pointer Setup
#define RMIXL_NAE_FREE_PMEM_PROG		_RMIXL_OFFSET(0x42) // Free Context Memory Pointer Setup
#define RMIXL_NAE_TX_DDR_ACTVLIST_CMD		_RMIXL_OFFSET(0x44) // Transmit DRR Active List Setup
#define RMIXL_NAE_TX_IF_BURSTMAX_CMD		_RMIXL_OFFSET(0x46) // Transmit Interface Burst Max Setup
#define RMIXL_NAE_TX_IF_ENABLE_CMD		_RMIXL_OFFSET(0x48) // Transmit Interface Enable Command
#define RMIXL_NAE_TX_PKTLEN_PMEM_CMD		_RMIXL_OFFSET(0x4A) // Transmit Pktlen Pointer Memory Setup
#define RMIXL_NAE_TX_SCHED_MAP_CMD0		_RMIXL_OFFSET(0x4C) // Transmit Context Scheduling and Mapping Setup
#define RMIXL_NAE_TX_SCHED_MAP_CMD1		_RMIXL_OFFSET(0x4D) // Transmit Context Scheduling and Mapping Setup
#define RMIXL_NAE_EGR_NIOR_CAL_LEN		_RMIXL_OFFSET(0x4E) // Egress NetIOR Credit Calendar Length
#define RMIXL_NAE_TX_PKT_PMEM_CMD0		_RMIXL_OFFSET(0x50) // Transmit Packet Pointer Memory Setup 0
#define RMIXL_NAE_TX_PKT_PMEM_CMD1		_RMIXL_OFFSET(0x51) // Transmit Packet Pointer Memory Setup 1
#define RMIXL_NAE_EGR_NIOR_CRDT_CAL_CMD		_RMIXL_OFFSET(0x52) // Egress NetIOR Credit Calendar Setup
#define RMIXL_NAE_TX_SCHED_CTRL			_RMIXL_OFFSET(0x53) // Transmit Scheduler Control
#define RMIXL_NAE_TX_CRC_POLY0			_RMIXL_OFFSET(0x54) // Transmit CRC Polynomial 0
#define RMIXL_NAE_TX_CRC_POLY1			_RMIXL_OFFSET(0x55) // Transmit CRC Polynomial 1
#define RMIXL_NAE_TX_CRC_POLY2			_RMIXL_OFFSET(0x56) // Transmit CRC Polynomial 2
#define RMIXL_NAE_TX_CRC_POLY3			_RMIXL_OFFSET(0x57) // Transmit CRC Polynomial 3
#define RMIXL_NAE_STR_PMEM_CMD			_RMIXL_OFFSET(0x58) // MicroStruct Descriptor Pointer
#define RMIXL_NAE_TX_IORCRDT_INIT		_RMIXL_OFFSET(0x59) // Transmit Network Interface Credit Initialization
#define RMIXL_NAE_TX_DSBL_ECC			_RMIXL_OFFSET(0x5A) // Transmit Disable ECC
#define RMIXL_NAE_TX_IORCRDT_IGNORE		_RMIXL_OFFSET(0x5B) // Transmit Network Interface Credit Ignore
#define RMIXL_NAE_TX_BW_VALUE0			_RMIXL_OFFSET(0x64) // Transmit Bandwidth Value 0
#define RMIXL_NAE_TX_BW_VALUE1			_RMIXL_OFFSET(0x65) // Transmit Bandwidth Value 1
#define RMIXL_NAE_TX_BW_VALUE2			_RMIXL_OFFSET(0x66) // Transmit Bandwidth Value 2
#define RMIXL_NAE_TX_BW_VALUE3			_RMIXL_OFFSET(0x67) // Transmit Bandwidth Value 3
#define RMIXL_NAE_TX_BW_CHOICE_CMD		_RMIXL_OFFSET(0x68) // Transmit Bandwidth Select
#define RMIXL_NAE_NAE_FREQ_CMD			_RMIXL_OFFSET(0x69) // NAE Frequency Select
#define RMIXL_NAE_NAE_TX_XOFF_RD_CMD		_RMIXL_OFFSET(0x6A) // Transmit XOFF Read Command
#define RMIXL_NAE_IFn_1588_TMSTMP_HI(n)		_RMIXL_OFFSET(0x300+2*(n)) // Interface 0-18 1588 Timestamp High
#define RMIXL_NAE_IFn_1588_TMSTMP_LO(n)		_RMIXL_OFFSET(0x301+2*(n)) // Interface 0-18 1588 Timestamp Low
#define RMIXL_NAE_TX_EL0			_RMIXL_OFFSET(0x328) // Transmit RAM Error Log 0
#define RMIXL_NAE_TX_EL1			_RMIXL_OFFSET(0x329) // Transmit RAM Error Log 1
#define RMIXL_NAE_EIC0				_RMIXL_OFFSET(0x32A) // Error Injection Control 0
#define RMIXL_NAE_EIC1				_RMIXL_OFFSET(0x32B) // Error Injection Control 1
#define RMIXL_NAE_STG1_STG2CRDT_STATUS		_RMIXL_OFFSET(0x32C) // Stage1 to Stage2 Credit Status
#define RMIXL_NAE_STG2_EHCRDT_STATUS		_RMIXL_OFFSET(0x32D) // Stage2 to Exit Hold FIFO Credit Status
#define RMIXL_NAE_STG2_FREECRDT_STATUS		_RMIXL_OFFSET(0x32E) // Stage2 to FreeFIFO Credit Status
#define RMIXL_NAE_STG2_STRCRDT_STATUS		_RMIXL_OFFSET(0x32F) // Stage2 to Micro-Struct FIFO Credit Status
#define RMIXL_NAE_TX_PERF_CNTR_INTR_STATUS	_RMIXL_OFFSET(0x330) // Transmit Performance Counter Interrupt Status
#define RMIXL_NAE_TX_PERF_CNTR_ROLL_STATUS	_RMIXL_OFFSET(0x331) // Transmit Performance Counter Roll Status
#define RMIXL_NAE_TX_PERF_CNTR0			_RMIXL_OFFSET(0x332) // Transmit Performance Counter 0
#define RMIXL_NAE_TX_PERF_CTRL0			_RMIXL_OFFSET(0x333) // Transmit Performance Counter 0 Control
#define RMIXL_NAE_TX_PERF_CNTR1			_RMIXL_OFFSET(0x334) // Transmit Performance Counter 1
#define RMIXL_NAE_TX_PERF_CTRL1			_RMIXL_OFFSET(0x335) // Transmit Performance Counter 1 Control
#define RMIXL_NAE_TX_PERF_CNTR2			_RMIXL_OFFSET(0x336) // Transmit Performance Counter 2
#define RMIXL_NAE_TX_PERF_CTRL2			_RMIXL_OFFSET(0x337) // Transmit Performance Counter 2 Control
#define RMIXL_NAE_TX_PERF_CNTR3			_RMIXL_OFFSET(0x338) // Transmit Performance Counter 3
#define RMIXL_NAE_TX_PERF_CTRL3			_RMIXL_OFFSET(0x339) // Transmit Performance Counter 3 Control
#define RMIXL_NAE_TX_PERF_CNTR4			_RMIXL_OFFSET(0x33A) // Transmit Performance Counter 4
#define RMIXL_NAE_TX_PERF_CTRL4			_RMIXL_OFFSET(0x33B) // Transmit Performance Counter 4 Control
#define RMIXL_NAE_TX_BW_CHOICE_STATUS		_RMIXL_OFFSET(0x33C) // Transmit Bandwidth Select Status
#define RMIXL_NAE_NAE_TXOFF_RD_STATUS		_RMIXL_OFFSET(0x33D) // NAE Transmit XOFF Read Status
#define RMIXL_NAE_VFBID_DESTMAP_STATUS		_RMIXL_OFFSET(0x380) // VFBID to Destination Map Status
#define RMIXL_NAE_STG2_PMEM_STATUS		_RMIXL_OFFSET(0x381) // Stage2 Context Memory Pointer Status
#define RMIXL_NAE_EH_PMEM_STATUS		_RMIXL_OFFSET(0x382) // Exit Hold Context Memory Pointer Status
#define RMIXL_NAE_FREE_PMEM_STATUS		_RMIXL_OFFSET(0x383) // Free Context Memory Pointer Status
#define RMIXL_NAE_TX_DDR_ACTVLIST_STATU	S	_RMIXL_OFFSET(0x384) // Transmit DRR Active List Status
#define RMIXL_NAE_TX_IF_BURSTMAX_STATUS		_RMIXL_OFFSET(0x385) // Transmit Interface Burst Max Status
#define RMIXL_NAE_TX_PKTLEN_PMEM_STATUS		_RMIXL_OFFSET(0x386) // Transmit Pktlen Pointer Memory Status
#define RMIXL_NAE_TX_SCHED_MAP_STATUS0		_RMIXL_OFFSET(0x387) // Transmit Context Scheduling and Mapping Status0
#define RMIXL_NAE_TX_SCHED_MAP_STATUS1		_RMIXL_OFFSET(0x388) // Transmit Context Scheduling and Mapping Status1
#define RMIXL_NAE_TX_PKT_PMEM_STATUS		_RMIXL_OFFSET(0x389) // Transmit Packet Pointer Memory Status
#define RMIXL_NAE_STR_PMEM_STATUS		_RMIXL_OFFSET(0x38A) // MicroStruct Pointer Memory Status
#define RMIXL_NAE_EGR_NIOR_CAL_STATUS		_RMIXL_OFFSET(0x38B) // Egress NetIOR Credit Calendar Status

	// Interrupt Registers
#define RMIXL_NAE_NET_IF_INTR_STATn(n)		_RMIXL_OFFSET(0x280+2*(n)) // Interface 0-18 Interrupt Status Registers
#define RMIXL_NAE_NET_IF_INTR_MASKn(n)		_RMIXL_OFFSET(0x281+2*(n)) // Interface 0-18 Interrupt Mask Registers
#define RMIXL_NAE_NET_COMMON0_INTR_STAT		_RMIXL_OFFSET(0x2A8) // Common Interrupt Status Register 0
#define RMIXL_NAE_NET_COMMON0_INTR_MASK		_RMIXL_OFFSET(0x2A9) // Common Interrupt Mask Register 0
#define RMIXL_NAE_NET_COMMON1_INTR_STAT		_RMIXL_OFFSET(0x2AA) // Common Interrupt Status Register 1
#define RMIXL_NAE_NET_COMMON1_INTR_MASK		_RMIXL_OFFSET(0x2AB) // Common Interrupt Mask Register 1

	// Note: In the following registers, registers IDs are in Interface 0xE.
	// For Block/Interface/Register decoding, see Table 11-14.
#define RMIXL_NAE_PHY_LANE<0-3>_STATUS		_RMIXL_OFFSET(0-3) // PHY Lane <0-3> Status
#define RMIXL_NAE_PHY_LANE<0-3>_CTRL		_RMIXL_OFFSET(4-7) // PHY Lane <0-3> Control
#define RMIXL_NAE_SYNCE_SEL_REFCLK		_RMIXL_OFFSET(8) // Quad Sync-E Reference Clock Select Register

	// Network Interface Top-Block Registers
#define RMIXL_NAE_LANE_CFG_CPLX_0_1		_RMIXL_OFFSET(0x780) // Lane Configuration Complex 0 and 1
#define RMIXL_NAE_LANE_CFG_CPLX_2_3		_RMIXL_OFFSET(0x781) // Lane Configuration Complex 2 and 3
#define RMIXL_NAE_LANE_CFG_CPLX_4		_RMIXL_OFFSET(0x782) // Lane Configuration Complex 4

#define RMIXL_NAE_SOFT_RESET			_RMIXL_OFFSET(0x783) // Network Interface Soft Reset Register
#define RMIXL_NAE_1588_PTP_OFFSET_HI		_RMIXL_OFFSET(0x784) // IEEE 1588 Timer Offset High
#define RMIXL_NAE_1588_PTP_OFFSET_LO		_RMIXL_OFFSET(0x785) // IEEE 1588 Timer Offset Low
#define RMIXL_NAE_1588_PTP_INC_DEN		_RMIXL_OFFSET(0x786) // IEEE 1588 Incremental Denominator
#define RMIXL_NAE_1588_PTP_INC_NUM		_RMIXL_OFFSET(0x787) // IEEE 1588 Incremental Numerator
#define RMIXL_NAE_1588_PTP_INC_INTG		_RMIXL_OFFSET(0x788) // IEEE 1588 Incremental Integer
#define RMIXL_NAE_1588_PTP_CONTROL		_RMIXL_OFFSET(0x789) // IEEE 1588 Control
#define RMIXL_NAE_1588_PTP_STATUS		_RMIXL_OFFSET(0x78A) // IEEE 1588 Status
#define RMIXL_NAE_1588_PTP_USER_VALUE_HI	_RMIXL_OFFSET(0x78B) // IEEE 1588 User Value High
#define RMIXL_NAE_1588_PTP_USER_VALUE_LO	_RMIXL_OFFSET(0x78C) // IEEE 1588 User Value Low
#define RMIXL_NAE_1588_PTP_TMR1_HI		_RMIXL_OFFSET(0x78D) // IEEE 1588 Timer 1 High
#define RMIXL_NAE_1588_PTP_TMR1_LOW		_RMIXL_OFFSET(0x78E) // IEEE 1588 Timer 1 Low
#define RMIXL_NAE_1588_PTP_TMR2_HI		_RMIXL_OFFSET(0x78F) // IEEE 1588 Timer 2 High
#define RMIXL_NAE_1588_PTP_TMR2_LOW		_RMIXL_OFFSET(0x790) // IEEE 1588 Timer 2 Low
#define RMIXL_NAE_1588_PTP_TMR3_HI		_RMIXL_OFFSET(0x791) // IEEE 1588 Timer 3 High
#define RMIXL_NAE_1588_PTP_TMR3_LOW		_RMIXL_OFFSET(0x792) // IEEE 1588 Timer 3 Low
#define RMIXL_NAE_TX_FC_CAL_IDX_TBL_CTRL	_RMIXL_OFFSET(0x793) // Tx Flow Control Calendar Index Table Control
#define RMIXL_NAE_TX_FC_CAL_TBL_CTRL		_RMIXL_OFFSET(0x794) // Tx Flow Control Calendar Table Control
#define RMIXL_NAE_TX_FC_CAL_TBL_DATA0		_RMIXL_OFFSET(0x795) // Tx Flow Control Table Data 0
#define RMIXL_NAE_TX_FC_CAL_TBL_DATA1		_RMIXL_OFFSET(0x796) // Tx Flow Control Table Data 1
#define RMIXL_NAE_TX_FC_CAL_TBL_DATA2		_RMIXL_OFFSET(0x797) // Tx Flow Control Table Data 2
#define RMIXL_NAE_TX_FC_CAL_TBL_DATA3		_RMIXL_OFFSET(0x798) // Tx Flow Control Table Data 3
#define RMIXL_NAE_INT_MDIO_CTRL			_RMIXL_OFFSET(0x799) // Internal MDIO Master Control
#define RMIXL_NAE_INT_MDIO_CTRL_DATA		_RMIXL_OFFSET(0x79A) // Internal MDIO Control Data
#define RMIXL_NAE_INT_MDIO_RD_STAT		_RMIXL_OFFSET(0x79B) // Internal MDIO Read Status
#define RMIXL_NAE_INT_MDIO_LINK_STAT		_RMIXL_OFFSET(0x79C) // Internal MDIO MII Link Fail Status
#define RMIXL_NAE_EXT_G0_MDIO_CTRL		_RMIXL_OFFSET(0x79D) // External Gig 0 MDIO Master Control
#define RMIXL_NAE_EXT_G0_MDIO_CTRL_DATA		_RMIXL_OFFSET(0x79E) // External Gig 0 MDIO Master Control Data
#define RMIXL_NAE_EXT_G0_MDIO_RD_STAT		_RMIXL_OFFSET(0x79F) // External Gig 0 MDIO Master Read Status Data
#define RMIXL_NAE_EXT_G0_MDIO_LINK_STAT		_RMIXL_OFFSET(0x7A0) // External Gig 0 MDIO MII Link Fail Status
#define RMIXL_NAE_EXT_G1_MDIO_CTRL		_RMIXL_OFFSET(0x7A1) // External Gig 1 MDIO Master Control
#define RMIXL_NAE_EXT_G1_MDIO_CTRL_DATA		_RMIXL_OFFSET(0x7A2) // External Gig 1 MDIO Master Control Data
#define RMIXL_NAE_EXT_G1_MDIO_RD_STAT		_RMIXL_OFFSET(0x7A3) // External Gig 1 MDIO Master Read Status Data
#define RMIXL_NAE_EXT_G1_MDIO_LINK_STAT		_RMIXL_OFFSET(0x7A4) // External Gig 1 MDIO MII Link Fail Status
#define RMIXL_NAE_EXT_XG0_MDIO_CTRL		_RMIXL_OFFSET(0x7A5) // External XGig 0 MDIO Control
#define RMIXL_NAE_EXT_XG0_MDIO_CTRL_DATA	_RMIXL_OFFSET(0x7A6) // External XGig 0 MDIO Control Data
#define RMIXL_NAE_EXT_XG0_MDIO_RD_STAT		_RMIXL_OFFSET(0x7A7) // External XGig 0 MDIO Read Status
#define RMIXL_NAE_EXT_XG0_MDIO_LINK_STAT	_RMIXL_OFFSET(0x7A8) // External XGig 0 MDIO MII Link Fail Status
#define RMIXL_NAE_EXT_XG1_MDIO_CTRL		_RMIXL_OFFSET(0x7A9) // External XGig 1 MDIO Control
#define RMIXL_NAE_EXT_XG1_MDIO_CTRL_DATA	_RMIXL_OFFSET(0x7AA) // External XGig 1 MDIO Control Data
#define RMIXL_NAE_EXT_XG1_MDIO_RD_STAT		_RMIXL_OFFSET(0x7AB) // External XGig 1 MDIO Read Status
#define RMIXL_NAE_EXT_XG1_MDIO_LINK_STAT	_RMIXL_OFFSET(0x7AC) // External XGig 1 MDIO MII Link Fail Status
#define RMIXL_NAE_GMAC_FC_SL0T0			_RMIXL_OFFSET(0x7AD) // GMAC Flow Control Slot 0
#define RMIXL_NAE_GMAC_FC_SL0T1			_RMIXL_OFFSET(0x7AE) // GMAC Flow Control Slot 1
#define RMIXL_NAE_GMAC_FC_SL0T2			_RMIXL_OFFSET(0x7AF) // GMAC Flow Control Slot 2
#define RMIXL_NAE_GMAC_FC_SL0T3			_RMIXL_OFFSET(0x7B0) // GMAC Flow Control Slot 3
#define RMIXL_NAE_NETIOR_NTB_SLOT		_RMIXL_OFFSET(0x7B1) // Network Interface to NAE Ingress Bus Slot
#define RMIXL_NAE_NETIOR_MISC_CTRL0		_RMIXL_OFFSET(0x7B2) // Miscellaneous Control 0
#define RMIXL_NAE_NETIOR_INT0			_RMIXL_OFFSET(0x7B3) // Interrupt 0 - Lane Fault
#define RMIXL_NAE_NETIOR_INT0_MASK		_RMIXL_OFFSET(0x7B4) // Interrupt 0 Mask
#define RMIXL_NAE_NETIOR_INT1			_RMIXL_OFFSET(0x7B5) // Interrupt 1 - Exception
#define RMIXL_NAE_NETIOR_INT1_MASK		_RMIXL_OFFSET(0x7B6) // Interrupt 1 Mask
#define RMIXL_NAE_GMAC_PFC_REPEAT		_RMIXL_OFFSET(0x7B7) // Priority Flow Control Repeat
#define RMIXL_NAE_XGMAC_PFC_REPEAT		_RMIXL_OFFSET(0x7B8) // XGMAC Priority Flow Control Repeat
#define RMIXL_NAE_NETIOR_MISC_CTRL1		_RMIXL_OFFSET(0x7B9) // Miscellaneous Control 1
#define RMIXL_NAE_NETIOR_MISC_CTRL2		_RMIXL_OFFSET(0x7BA) // Miscellaneous Control 2
#define RMIXL_NAE_NETIOR_INT2			_RMIXL_OFFSET(0x7BB) // Interrupt 2 - Transition Not Detected
#define RMIXL_NAE_NETIOR_INT2_MASK		_RMIXL_OFFSET(0x7BC) // Interrupt 2 Mask
#define RMIXL_NAE_NETIOR_MISC_CTRL3		_RMIXL_OFFSET(0x7BD) // Miscellaneous Control 3

	// Receive Configuration (RX_CONFIG)

	// Free-in descriptor Size Unique.
	//    Free In Descriptor Size is unique per Free-In LIFO pool.
	//    If this is set then FrInDescCLSize is not used.
	//    To program individual Free-In LIFO Sizes use the Free-In LIFO
	//    Descriptor Size Register (FREE_IN_LIFO_UNIQ_SZ_CFG).
#define	RMIXL_NAE_RX_CONFIG_FSU			__BIT(31)
	// Rx Status Mask.
	//    The 7 error status events mask. Set each bit for the error types
	//    shown below so that the Err bit in register
	//    Rx Packet Descriptor Format is set.
#define	RMIXL_NAE_RX_CONFIG_RSM			__BITS(30,24)
	//    Bit 6: Truncated - If a packet overflows the NetIor ingress
	//    FIFOs, it will be truncated. This is the only place a packet
	//    can get truncated.
#define	RMIXL_NAE_RX_CONFIG_RSM_TRUNC		__BIT(30)
	//    Bit 5: Pause - Only for SGMII/XAUI. Pause Frames can be directly
	//    understood and used by the hardware. If software does not want to
	//    decode these and drops them, then this bit needs to be set.
#define	RMIXL_NAE_RX_CONFIG_RSM_PAUSE		__BIT(29)
	//    Bit 4: Rx OK - Good Packet
#define	RMIXL_NAE_RX_CONFIG_RSM_RX_OK		__BIT(28)
	//    Bit 3: LengthOutOfRange - (legacy mode, should be 0)
#define	RMIXL_NAE_RX_CONFIG_RSM_LENGTH_RANGE	__BIT(27)
	//    Bit 2: LengthCheckError - (legacy mode, should be 0)
#define	RMIXL_NAE_RX_CONFIG_RSM_LENGTH_CHECK	__BIT(26)
	//    Bit 1: Link CRC error (bit 1 in INTR_STAT register)
	//           Could also be a "truncated packet"
#define	RMIXL_NAE_RX_CONFIG_RSM_LINK_CRC	__BIT(25)
	//    Bit 0: Code Error (legacy mode; reserved)
#define	RMIXL_NAE_RX_CONFIG_RSM_CODE_ERROR	__BIT(24)

#define	RMIXL_NAE_RX_CONFIG_PREPAD_SIZE		__BITS(23,22) // Pre-pad Size in 16-byte units.
#define	RMIXL_NAE_RX_CONFIG_PREPAD_WORD3	__BITS(21,20) // Pre-pad Word 3
#define	RMIXL_NAE_RX_CONFIG_PREPAD_WORD2	__BITS(19,18) // Pre-pad Word 2
#define	RMIXL_NAE_RX_CONFIG_PREPAD_WORD1	__BITS(17,16) // Pre-pad Word 1
#define	RMIXL_NAE_RX_CONFIG_PREPAD_WORD0	__BITS(15,14) // Pre-pad Word 0
#define	RMIXL_NAE_RX_CONFIG_PREPAD_EN		__BIT(13) // Pre-pad Enable

#define	RMIXL_NAE_RX_CONFIG_HPE			__BIT(12) // H/W Parser Enable

	// Free-In Descriptor Size (in 64-byte Cache Lines).
	//	If this field is set to 0, the descriptor size is 16 KB.
#define	RMIXL_NAE_RX_CONFIG_FREEIN_DESC_SIZE	__BITS(11,4)

#define	RMIXL_NAE_RX_CONFIG_DED			__BIT(3) // Device ECC Disable

	// Receive Maximum Message Size minus one
	//	The POE adds its own descriptor to the number of descriptors
#define	RMIXL_NAE_RX_CONFIG_MMS			__BITS(2,1)
#define	RMIXL_NAE_RX_CONFIG_MMS_ONE		0	// 1 descriptor
#define	RMIXL_NAE_RX_CONFIG_MMS_TWO		1	// 2 descriptors
#define	RMIXL_NAE_RX_CONFIG_MMS_THREE		2	// 3 descriptors
#define	RMIXL_NAE_RX_CONFIG_MMS_INVALID		3	// Invalid

	// Rx Enable.
#define	RMIXL_NAE_RX_CONFIG_RE			__BIT(0)

	// Receive Interface Base Config <0-9> (RX_IF_BASE_CONFIG<0-9>)
	// Block 7, Register 0x12..0x1b (8xx/4xx)
	// Block 7, Register 0x12..0x16 (3xx)
	// RX_IF_BASE_CONFIG0: Complex 0 phy0/phy1 (EVEN/ODD) Channel Offset,
	// RX_IF_BASE_CONFIG1: Complex 0 phy2/phy3 (EVEN/ODD) Channel Offset,
	// RX_IF_BASE_CONFIG2: Complex 1 phy0/phy1 (EVEN/ODD) Channel Offset,
	// RX_IF_BASE_CONFIG3: Complex 1 phy2/phy3 (EVEN/ODD) Channel Offset,
	// RX_IF_BASE_CONFIG4: Complex 2 phy0/phy1 (EVEN/ODD) Channel Offset,
	// RX_IF_BASE_CONFIG5: Complex 2 phy2/phy3 (EVEN/ODD) Channel Offset,
	// These are 8xx/4xx only:
	// RX_IF_BASE_CONFIG6: Complex 3 phy0/phy1 (EVEN/ODD) Channel Offset,
	// RX_IF_BASE_CONFIG7: Complex 3 phy2/phy3 (EVEN/ODD) Channel Offset,
	// RX_IF_BASE_CONFIG8: Complex 4 phy0/phy1 (EVEN/ODD) Channel Offset,
	// RX_IF_BASE_CONFIG9: Complex 4 phy2/phy3 (EVEN/ODD) Channel Offset,
	// On XLP 800 and 400 processors, the valid range is 0-523.
	// On XLP 300 and 300-Lite processors, the valid range is 0-127.
#define	RMIXL_NAE_RX_IF_BASE_CONFIG_ODD_LANE_BASE	__BITS(25,16)
#define	RMIXL_NAE_RX_IF_BASE_CONFIG_EVEN_LANE_BASE	__BITS(9,0)

#endif /* _MIPS_RMI_RMIXL_NAEREG_H */
