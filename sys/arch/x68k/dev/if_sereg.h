/*
 * National Semiconductor DS8390 NIC register definitions 
 *
 * $NetBSD: if_sereg.h,v 1.2 1997/10/13 14:22:52 lukem Exp $
 *
 */

#define	CMD_CLEAR_TALLIES	0x02
#define	CMD_SEND_ETHERPACKET	0x05
#define	CMD_RECIEVE_ETHERPACKET	0x0e
#define	CMD_SET_ETHERADDR	0x06
#define	CMD_SET_MULTICASTADDR	0x09
#define	CMD_SET_PACKETRECEPTMODE	0x0c
#define	CMD_ETHERNET_RECIEVE	0x0d

/*
 * Page 0 register offsets
 */
#define SE_P0_CR	0x00	/* Command Register */

#define SE_P0_CLDA0	0x01	/* Current Local DMA Addr low (read) */
#define SE_P0_PSTART	0x01	/* Page Start register (write) */

#define SE_P0_CLDA1	0x02	/* Current Local DMA Addr high (read) */
#define SE_P0_PSTOP	0x02	/* Page Stop register (write) */

#define SE_P0_BNRY	0x03	/* Boundary Pointer */

#define SE_P0_TSR	0x04	/* Transmit Status Register (read) */
#define SE_P0_TPSR	0x04	/* Transmit Page Start (write) */

#define SE_P0_NCR	0x05	/* Number of Collisions Reg (read) */
#define SE_P0_TBCR0	0x05	/* Transmit Byte count, low (write) */

#define SE_P0_FIFO	0x06	/* FIFO register (read) */
#define SE_P0_TBCR1	0x06	/* Transmit Byte count, high (write) */

#define SE_P0_ISR	0x07	/* Interrupt Status Register */

#define SE_P0_CRDA0	0x08	/* Current Remote DMA Addr low (read) */
#define SE_P0_RSAR0	0x08	/* Remote Start Address low (write) */

#define SE_P0_CRDA1	0x09	/* Current Remote DMA Addr high (read) */
#define SE_P0_RSAR1	0x09	/* Remote Start Address high (write) */

#define SE_P0_RBCR0	0x0a	/* Remote Byte Count low (write) */

#define SE_P0_RBCR1	0x0b	/* Remote Byte Count high (write) */

#define SE_P0_RSR	0x0c	/* Receive Status (read) */
#define SE_P0_RCR	0x0c	/* Receive Configuration Reg (write) */

#define SE_P0_CNTR0	0x0d	/* frame alignment error counter (read) */
#define SE_P0_TCR	0x0d	/* Transmit Configuration Reg (write) */

#define SE_P0_CNTR1	0x0e	/* CRC error counter (read) */
#define SE_P0_DCR	0x0e	/* Data Configuration Reg (write) */

#define SE_P0_CNTR2	0x0f	/* missed packet counter (read) */
#define SE_P0_IMR	0x0f	/* Interrupt Mask Register (write) */

/*
 * Page 1 register offsets
 */
#define SE_P1_CR	0x00	/* Command Register */
#define SE_P1_PAR0	0x01	/* Physical Address Register 0 */
#define SE_P1_PAR1	0x02	/* Physical Address Register 1 */
#define SE_P1_PAR2	0x03	/* Physical Address Register 2 */
#define SE_P1_PAR3	0x04	/* Physical Address Register 3 */
#define SE_P1_PAR4	0x05	/* Physical Address Register 4 */
#define SE_P1_PAR5	0x06	/* Physical Address Register 5 */
#define SE_P1_CURR	0x07	/* Current RX ring-buffer page */
#define SE_P1_MAR0	0x08	/* Multicast Address Register 0 */
#define SE_P1_MAR1	0x09	/* Multicast Address Register 1 */
#define SE_P1_MAR2	0x0a	/* Multicast Address Register 2 */
#define SE_P1_MAR3	0x0b	/* Multicast Address Register 3 */
#define SE_P1_MAR4	0x0c	/* Multicast Address Register 4 */
#define SE_P1_MAR5	0x0d	/* Multicast Address Register 5 */
#define SE_P1_MAR6	0x0e	/* Multicast Address Register 6 */
#define SE_P1_MAR7	0x0f	/* Multicast Address Register 7 */

/*
 * Page 2 register offsets
 */
#define SE_P2_CR	0x00	/* Command Register */
#define SE_P2_PSTART	0x01	/* Page Start (read) */
#define SE_P2_CLDA0	0x01	/* Current Local DMA Addr 0 (write) */
#define SE_P2_PSTOP	0x02	/* Page Stop (read) */
#define SE_P2_CLDA1	0x02	/* Current Local DMA Addr 1 (write) */
#define SE_P2_RNPP	0x03	/* Remote Next Packet Pointer */
#define SE_P2_TPSR	0x04	/* Transmit Page Start (read) */
#define SE_P2_LNPP	0x05	/* Local Next Packet Pointer */
#define SE_P2_ACU	0x06	/* Address Counter Upper */
#define SE_P2_ACL	0x07	/* Address Counter Lower */
#define SE_P2_RCR	0x0c	/* Receive Configuration Register (read) */
#define SE_P2_TCR	0x0d	/* Transmit Configuration Register (read) */
#define SE_P2_DCR	0x0e	/* Data Configuration Register (read) */
#define SE_P2_IMR	0x0f	/* Interrupt Mask Register (read) */

/*
 *		Command Register (CR) definitions
 */

/*
 * STP: SToP. Software reset command. Takes the controller offline. No
 *	packets will be received or transmitted. Any reception or
 *	transmission in progress will continue to completion before
 *	entering reset state. To exit this state, the STP bit must
 *	reset and the STA bit must be set. The software reset has
 *	executed only when indicated by the RST bit in the ISR being
 *	set.
 */
#define SE_CR_STP	0x01

/*
 * STA: STArt. This bit is used to activate the NIC after either power-up,
 *	or when the NIC has been put in reset mode by software command
 *	or error.
 */
#define SE_CR_STA	0x02

/*
 * TXP: Transmit Packet. This bit must be set to indicate transmission of
 *	a packet. TXP is internally reset either after the transmission is
 *	completed or aborted. This bit should be set only after the Transmit
 *	Byte Count and Transmit Page Start register have been programmed.
 */
#define SE_CR_TXP	0x04

/*
 * RD0, RD1, RD2: Remote DMA Command. These three bits control the operation
 *	of the remote DMA channel. RD2 can be set to abort any remote DMA
 *	command in progress. The Remote Byte Count registers should be cleared
 *	when a remote DMA has been aborted. The Remote Start Addresses are not
 *	restored to the starting address if the remote DMA is aborted.
 *
 *	RD2 RD1 RD0	function
 *	 0   0   0	not allowed
 *	 0   0   1	remote read
 *	 0   1   0	remote write
 *	 0   1   1	send packet
 *	 1   X   X	abort
 */
#define SE_CR_RD0	0x08
#define SE_CR_RD1	0x10
#define SE_CR_RD2	0x20

/*
 * PS0, PS1: Page Select. The two bits select which register set or 'page' to
 *	access.
 *
 *	PS1 PS0		page
 *	 0   0		0
 *	 0   1		1
 *	 1   0		2
 *	 1   1		reserved
 */
#define SE_CR_PS0	0x40
#define SE_CR_PS1	0x80
/* bit encoded aliases */
#define SE_CR_PAGE_0	0x00 /* (for consistency) */
#define SE_CR_PAGE_1	0x40
#define SE_CR_PAGE_2	0x80

/*
 *		Interrupt Status Register (ISR) definitions
 */

/*
 * PRX: Packet Received. Indicates packet received with no errors.
 */
#define SE_ISR_PRX	0x01

/*
 * PTX: Packet Transmitted. Indicates packet transmitted with no errors.
 */
#define SE_ISR_PTX	0x02

/*
 * RXE: Receive Error. Indicates that a packet was received with one or more
 *	the following errors: CRC error, frame alignment error, FIFO overrun,
 *	missed packet.
 */
#define SE_ISR_RXE	0x04

/*
 * TXE: Transmission Error. Indicates that an attempt to transmit a packet
 *	resulted in one or more of the following errors: excessive
 *	collisions, FIFO underrun.
 */
#define SE_ISR_TXE	0x08

/*
 * OVW: OverWrite. Indicates a receive ring-buffer overrun. Incoming network
 *	would exceed (has exceeded?) the boundry pointer, resulting in data
 *	that was previously received and not yet read from the buffer to be
 *	overwritten.
 */
#define SE_ISR_OVW	0x10

/*
 * CNT: Counter Overflow. Set when the MSB of one or more of the Network Talley
 *	Counters has been set.
 */
#define SE_ISR_CNT	0x20

/*
 * RDC: Remote Data Complete. Indicates that a Remote DMA operation has completed.
 */
#define SE_ISR_RDC	0x40

/*
 * RST: Reset status. Set when the NIC enters the reset state and cleared when a
 *	Start Command is issued to the CR. This bit is also set when a receive
 *	ring-buffer overrun (OverWrite) occurs and is cleared when one or more
 *	packets have been removed from the ring. This is a read-only bit.
 */
#define SE_ISR_RST	0x80

/*
 *		Interrupt Mask Register (IMR) definitions
 */

/*
 * PRXE: Packet Received interrupt Enable. If set, a received packet will cause
 *	an interrupt.
 */
#define SE_IMR_PRXE	0x01

/*
 * PTXE: Packet Transmit interrupt Enable. If set, an interrupt is generated when
 *	a packet transmission completes.
 */
#define SE_IMR_PTXE	0x02

/*
 * RXEE: Receive Error interrupt Enable. If set, an interrupt will occur whenever a
 *	packet is received with an error.
 */
#define SE_IMR_RXEE 	0x04

/*
 * TXEE: Transmit Error interrupt Enable. If set, an interrupt will occur whenever
 *	a transmission results in an error.
 */
#define SE_IMR_TXEE	0x08

/*
 * OVWE: OverWrite error interrupt Enable. If set, an interrupt is generated whenever
 *	the receive ring-buffer is overrun. i.e. when the boundry pointer is exceeded.
 */
#define SE_IMR_OVWE	0x10

/*
 * CNTE: Counter overflow interrupt Enable. If set, an interrupt is generated whenever
 *	the MSB of one or more of the Network Statistics counters has been set.
 */
#define SE_IMR_CNTE	0x20

/*
 * RDCE: Remote DMA Complete interrupt Enable. If set, an interrupt is generated
 *	when a remote DMA transfer has completed.
 */
#define SE_IMR_RDCE	0x40

/*
 * bit 7 is unused/reserved
 */

/*
 *		Data Configuration Register (DCR) definitions
 */

/*
 * WTS: Word Transfer Select. WTS establishes byte or word transfers for
 *	both remote and local DMA transfers
 */
#define SE_DCR_WTS	0x01

/*
 * BOS: Byte Order Select. BOS sets the byte order for the host.
 *	Should be 0 for 80x86, and 1 for 68000 series processors
 */
#define SE_DCR_BOS	0x02

/*
 * LAS: Long Address Select. When LAS is 1, the contents of the remote
 *	DMA registers RSAR0 and RSAR1 are used to provide A16-A31
 */
#define SE_DCR_LAS	0x04

#ifdef huh
/*
 * LS: Loopback Select. When 0, loopback mode is selected. Bits D1 and D2
 *	of the TCR must also be programmed for loopback operation.
 *	When 1, normal operation is selected.
 */
#define SE_DCR_LS	0x08
#endif
/*
 * BMS: Burst Mode Select
 */
#define SE_DCR_BMS	0x08

/*
 * AR: Auto-initialize Remote. When 0, data must be removed from ring-buffer
 *	under program control. When 1, remote DMA is automatically initiated
 *	and the boundry pointer is automatically updated
 */
#define SE_DCR_AR	0x10

/*
 * FT0, FT1: Fifo Threshold select.
 *		FT1	FT0	Word-width	Byte-width
 *		 0	 0	1 word		2 bytes
 *		 0	 1	2 words		4 bytes
 *		 1	 0	4 words		8 bytes
 *		 1	 1	8 words		12 bytes
 *
 *	During transmission, the FIFO threshold indicates the number of bytes
 *	or words that the FIFO has filled from the local DMA before BREQ is
 *	asserted. The transmission threshold is 16 bytes minus the receiver
 *	threshold.
 */
#define SE_DCR_FT0	0x20
#define SE_DCR_FT1	0x40

/*
 * bit 7 (0x80) is unused/reserved
 */

/*
 *		Transmit Configuration Register (TCR) definitions
 */

/*
 * CRC: Inhibit CRC. If 0, CRC will be appended by the transmitter, if 0, CRC
 *	is not appended by the transmitter.
 */
#define SE_TCR_CRC	0x01

/*
 * LB0, LB1: Loopback control. These two bits set the type of loopback that is
 *	to be performed.
 *
 *	LB1 LB0		mode
 *	 0   0		0 - normal operation (DCR_LS = 0)
 *	 0   1		1 - internal loopback (DCR_LS = 0)
 *	 1   0		2 - external loopback (DCR_LS = 1)
 *	 1   1		3 - external loopback (DCR_LS = 0)
 */
#define SE_TCR_LB0	0x02
#define SE_TCR_LB1	0x04

/*
 * ATD: Auto Transmit Disable. Clear for normal operation. When set, allows
 *	another station to disable the NIC's transmitter by transmitting to
 *	a multicast address hashing to bit 62. Reception of a multicast address
 *	hashing to bit 63 enables the transmitter.
 */
#define SE_TCR_ATD	0x08

/*
 * OFST: Collision Offset enable. This bit when set modifies the backoff
 *	algorithm to allow prioritization of nodes.
 */
#define SE_TCR_OFST	0x10
 
/*
 * bits 5, 6, and 7 are unused/reserved
 */

/*
 *		Transmit Status Register (TSR) definitions
 */

/*
 * PTX: Packet Transmitted. Indicates successful transmission of packet.
 */
#define SE_TSR_PTX	0x01

/*
 * bit 1 (0x02) is unused/reserved
 */

/*
 * COL: Transmit Collided. Indicates that the transmission collided at least
 *	once with another station on the network.
 */
#define SE_TSR_COL	0x04

/*
 * ABT: Transmit aborted. Indicates that the transmission was aborted due to
 *	excessive collisions.
 */
#define SE_TSR_ABT	0x08

/*
 * CRS: Carrier Sense Lost. Indicates that carrier was lost during the
 *	transmission of the packet. (Transmission is not aborted because
 *	of a loss of carrier)
 */
#define SE_TSR_CRS	0x10

/*
 * FU: FIFO Underrun. Indicates that the NIC wasn't able to access bus/
 *	transmission memory before the FIFO emptied. Transmission of the
 *	packet was aborted.
 */
#define SE_TSR_FU	0x20

/*
 * CDH: CD Heartbeat. Indicates that the collision detection circuitry
 *	isn't working correctly during a collision heartbeat test.
 */
#define SE_TSR_CDH	0x40

/*
 * OWC: Out of Window Collision: Indicates that a collision occurred after
 *	a slot time (51.2us). The transmission is rescheduled just as in
 *	normal collisions.
 */
#define SE_TSR_OWC	0x80

/*
 *		Receiver Configuration Register (RCR) definitions
 */

/*
 * SEP: Save Errored Packets. If 0, error packets are discarded. If set to 1,
 *	packets with CRC and frame errors are not discarded.
 */
#define SE_RCR_SEP	0x01

/*
 * AR: Accept Runt packet. If 0, packet with less than 64 byte are discarded.
 *	If set to 1, packets with less than 64 byte are not discarded.
 */
#define SE_RCR_AR	0x02

/*
 * AB: Accept Broadcast. If set, packets sent to the broadcast address will be
 *	accepted.
 */
#define SE_RCR_AB	0x04

/*
 * AM: Accept Multicast. If set, packets sent to a multicast address are checked
 *	for a match in the hashing array. If clear, multicast packets are ignored.
 */
#define SE_RCR_AM	0x08

/*
 * PRO: Promiscuous Physical. If set, all packets with a physical addresses are
 *	accepted. If clear, a physical destination address must match this
 *	station's address. Note: for full promiscuous mode, RCR_AB and RCR_AM
 *	must also be set. In addition, the multicast hashing array must be set
 *	to all 1's so that all multicast addresses are accepted.
 */
#define SE_RCR_PRO	0x10

/*
 * MON: Monitor Mode. If set, packets will be checked for good CRC and framing,
 *	but are not stored in the ring-buffer. If clear, packets are stored (normal
 *	operation).
 */
#define SE_RCR_MON	0x20

/*
 * bits 6 and 7 are unused/reserved.
 */

/*
 *		Receiver Status Register (RSR) definitions
 */

/*
 * PRX: Packet Received without error.
 */
#define SE_RSR_PRX	0x01

/*
 * CRC: CRC error. Indicates that a packet has a CRC error. Also set for frame
 *	alignment errors.
 */
#define SE_RSR_CRC	0x02

/*
 * FAE: Frame Alignment Error. Indicates that the incoming packet did not end on
 *	a byte boundry and the CRC did not match at the last byte boundry.
 */
#define SE_RSR_FAE	0x04

/*
 * FO: FIFO Overrun. Indicates that the FIFO was not serviced (during local DMA)
 *	causing it to overrun. Reception of the packet is aborted.
 */
#define SE_RSR_FO	0x08

/*
 * MPA: Missed Packet. Indicates that the received packet couldn't be stored in
 *	the ring-buffer because of insufficient buffer space (exceeding the
 *	boundry pointer), or because the transfer to the ring-buffer was inhibited
 *	by RCR_MON - monitor mode.
 */
#define SE_RSR_MPA	0x10

/*
 * PHY: Physical address. If 0, the packet received was sent to a physical address.
 *	If 1, the packet was accepted because of a multicast/broadcast address
 *	match.
 */
#define SE_RSR_PHY	0x20

/*
 * DIS: Receiver Disabled. Set to indicate that the receiver has enetered monitor
 *	mode. Cleared when the receiver exits monitor mode.
 */
#define SE_RSR_DIS	0x40

/*
 * DFR: Deferring. Set to indicate a 'jabber' condition. The CRS and COL inputs
 *	are active, and the transceiver has set the CD line as a result of the
 *	jabber.
 */
#define SE_RSR_DFR	0x80

/*
 * receive ring discriptor
 *
 * The National Semiconductor DS8390 Network interface controller uses
 * the following receive ring headers.  The way this works is that the
 * memory on the interface card is chopped up into 256 bytes blocks.
 * A contiguous portion of those blocks are marked for receive packets
 * by setting start and end block #'s in the NIC.  For each packet that
 * is put into the receive ring, one of these headers (4 bytes each) is
 * tacked onto the front.
 */
struct SE_ring	{
	u_char	rcv_status;		/* received packet status	*/
	u_char	next_packet;		/* pointer to next packet	*/
	u_char	count[2];		/* bytes in packet (length + 4)	*/
};

/*
 * 				Common constants
 */
#define SE_PAGE_SIZE		256		/* Size of RAM pages in bytes */
#define SE_TXBUF_SIZE		6		/* Size of TX buffer in pages */

/*
 * Vendor types
 */
#define SE_VENDOR_UNKNOWN	0xFF		/* Unknown network card */
#define SE_VENDOR_APPLE		0x00		/* Apple Ethernet card */
#define SE_VENDOR_INTERLAN	0x01		/* Interlan A310 card (GatorCard) */
#define SE_VENDOR_DAYNA		0x02		/* DaynaPORT E/30s (and others?) */

/*
 * Compile-time config flags
 */
/*
 * this sets the default for enabling/disablng the tranceiver
 */
#define SE_FLAGS_DISABLE_TRANCEIVER	0x01

/*
 * This disables the use of double transmit buffers.
 */
#define SE_FLAGS_NO_DOUBLE_BUFFERING	0x08

/* */
#define	GC_RESET_OFFSET		0x000c0000 /* writes here reset NIC */
#define	GC_ROM_OFFSET		0x000c0000 /* address prom */
#define GC_DATA_OFFSET		0x000d0000 /* Offset to NIC memory */
#define GC_NIC_OFFSET		0x000e0000 /* Offset to NIC registers */

#define DP_ROM_OFFSET		0x000f0000
#define DP_DATA_OFFSET		0x000d0000 /* Offset to SONIC memory */
#define DP_NIC_OFFSET		0x000e0000 /* Offset to SONIC registers */

#define SE_ROM_OFFSET		0x000f0000
#define SE_DATA_OFFSET		0x000d0000 /* Offset to NIC memory */
#define SE_NIC_OFFSET		0x000e0000 /* Offset to NIC registers */
