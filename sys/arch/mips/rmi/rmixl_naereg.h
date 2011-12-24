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
 * RX P2P Descriptor
 */
#define	RMIXL_NEA_RXD_CONTEXT			__BITS(63,54)
#define	RMIXL_NEA_RXD_LENGTH			__BITS(53,40)
#define	RMIXL_NEA_RXD_ADDRESS			__BITS(39,6)
#define	RMIXL_NEA_RXD_UP			__BIT(5)
#define	RMIXL_NEA_RXD_ERR			__BIT(4)
#define	RMIXL_NEA_RXD_IC			__BIT(3)
#define	RMIXL_NEA_RXD_TC			__BIT(2)
#define	RMIXL_NEA_RXD_PP			__BIT(1)
#define	RMIXL_NEA_RXD_P2P			__BIT(0)

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
#define	RMIXL_NEA_TXFBD_RDX			__BIT(61)
#define	RMIXL_NEA_TXFBD__RSRVD0			__BITS(60,58)
#define	RMIXL_NEA_TXFBD_TS_VALID		__BIT(57)
#define	RMIXL_NEA_TXFBD_TX_DONE			__BIT(56)
#define	RMIXL_NEA_TXFBD_MAX_COLL_LATE_ABORT	__BIT(55)
#define	RMIXL_NEA_TXFBD_UNDERRUN		__BIT(54)
#define	RMIXL_NEA_TXFBD__RSRVD1			__BITS(53,50)
#define	RMIXL_NEA_TXFBD_CONTEXT			__BITS(49,40)
#define	RMIXL_NEA_TXFBD_ADDRESS			__BITS(39,0)

#define	RMIXL_NEA_RXFID__RSRVD0			__BITS(63,40)
#define	RMIXL_NEA_RXFID_ADDRESS			__BITS(39,6)
#define	RMIXL_NEA_RXFID__RSRVD1			__BITS(5,0)

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

#endif /* _MIPS_RMI_RMIXL_NAEREG_H */
