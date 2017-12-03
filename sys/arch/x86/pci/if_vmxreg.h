/*	$NetBSD: if_vmxreg.h,v 1.1.6.3 2017/12/03 11:36:50 jdolecek Exp $	*/
/*	$OpenBSD: if_vmxreg.h,v 1.3 2013/08/28 10:19:19 reyk Exp $	*/

/*
 * Copyright (c) 2013 Tsubai Masanari
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

struct UPT1_TxStats {
	uint64_t TSO_packets;
	uint64_t TSO_bytes;
	uint64_t ucast_packets;
	uint64_t ucast_bytes;
	uint64_t mcast_packets;
	uint64_t mcast_bytes;
	uint64_t bcast_packets;
	uint64_t bcast_bytes;
	uint64_t error;
	uint64_t discard;
} __packed;

struct UPT1_RxStats {
	uint64_t LRO_packets;
	uint64_t LRO_bytes;
	uint64_t ucast_packets;
	uint64_t ucast_bytes;
	uint64_t mcast_packets;
	uint64_t mcast_bytes;
	uint64_t bcast_packets;
	uint64_t bcast_bytes;
	uint64_t nobuffer;
	uint64_t error;
} __packed;

#define ETHER_ALIGN 2

/* interrupt moderation levels */
#define UPT1_IMOD_NONE     0		/* no moderation */
#define UPT1_IMOD_HIGHEST  7		/* least interrupts */
#define UPT1_IMOD_ADAPTIVE 8		/* adaptive interrupt moderation */

/* hardware features */
#define UPT1_F_CSUM 0x0001		/* Rx checksum verification */
#define UPT1_F_RSS  0x0002		/* receive side scaling */
#define UPT1_F_VLAN 0x0004		/* VLAN tag stripping */
#define UPT1_F_LRO  0x0008		/* large receive offloading */

#define VMXNET3_BAR0_IMASK(irq)	(0x000 + (irq) * 8)	/* interrupt mask */
#define VMXNET3_BAR0_TXH(q)	(0x600 + (q) * 8)	/* Tx head */
#define VMXNET3_BAR0_RXH1(q)	(0x800 + (q) * 8)	/* ring1 Rx head */
#define VMXNET3_BAR0_RXH2(q)	(0xa00 + (q) * 8)	/* ring2 Rx head */
#define VMXNET3_BAR1_VRRS	0x000	/* VMXNET3 revision report selection */
#define VMXNET3_BAR1_UVRS	0x008	/* UPT version report selection */
#define VMXNET3_BAR1_DSL	0x010	/* driver shared address low */
#define VMXNET3_BAR1_DSH	0x018	/* driver shared address high */
#define VMXNET3_BAR1_CMD	0x020	/* command */
#define VMXNET3_BAR1_MACL	0x028	/* MAC address low */
#define VMXNET3_BAR1_MACH	0x030	/* MAC address high */
#define VMXNET3_BAR1_INTR	0x038	/* interrupt status */
#define VMXNET3_BAR1_EVENT	0x040	/* event status */

#define VMXNET3_CMD_ENABLE	0xcafe0000	/* enable VMXNET3 */
#define VMXNET3_CMD_DISABLE	0xcafe0001	/* disable VMXNET3 */
#define VMXNET3_CMD_RESET	0xcafe0002	/* reset device */
#define VMXNET3_CMD_SET_RXMODE	0xcafe0003	/* set interface flags */
#define VMXNET3_CMD_SET_FILTER	0xcafe0004	/* set address filter */
#define VMXNET3_CMD_VLAN_FILTER	0xcafe0005	/* set VLAN filter */
#define VMXNET3_CMD_GET_STATUS	0xf00d0000	/* get queue errors */
#define VMXNET3_CMD_GET_STATS	0xf00d0001	/* get queue statistics */
#define VMXNET3_CMD_GET_LINK	0xf00d0002	/* get link status */
#define VMXNET3_CMD_GET_MACL	0xf00d0003
#define VMXNET3_CMD_GET_MACH	0xf00d0004
#define VMXNET3_CMD_GET_INTRCFG	0xf00d0008	/* get interrupt config */

#define VMXNET3_DMADESC_ALIGN	128
#define VMXNET3_INIT_GEN	1

/* All descriptors are in little-endian format. */
struct vmxnet3_txdesc {
	uint64_t	addr;

	uint32_t	len:14;
	uint32_t	gen:1;		/* Generation */
	uint32_t	pad1:1;
	uint32_t	dtype:1;	/* Descriptor type */
	uint32_t	pad2:1;
	uint32_t	offload_pos:14;	/* Offloading position */

	uint32_t	hlen:10;	/* Header len */
	uint32_t	offload_mode:2;	/* Offloading mode */
	uint32_t	eop:1;		/* End of packet */
	uint32_t	compreq:1;	/* Completion request */
	uint32_t	pad3:1;
	uint32_t	vtag_mode:1;	/* VLAN tag insertion mode */
	uint32_t	vtag:16;	/* VLAN tag */
} __packed;

/* offloading modes */
#define VMXNET3_OM_NONE 0
#define VMXNET3_OM_CSUM 2
#define VMXNET3_OM_TSO  3

struct vmxnet3_txcompdesc {
	uint32_t	eop_idx:12;	/* EOP index in Tx ring */
	uint32_t	pad1:20;

	uint32_t	pad2:32;
	uint32_t	pad3:32;

	uint32_t	rsvd:24;
	uint32_t	type:7;
	uint32_t	gen:1;
} __packed;

struct vmxnet3_rxdesc {
	uint64_t	addr;

	uint32_t	len:14;
	uint32_t	btype:1;	/* Buffer type */
	uint32_t	dtype:1;	/* Descriptor type */
	uint32_t	rsvd:15;
	uint32_t	gen:1;

	uint32_t	pad1:32;
} __packed;

/* buffer types */
#define VMXNET3_BTYPE_HEAD 0	/* head only */
#define VMXNET3_BTYPE_BODY 1	/* body only */

struct vmxnet3_rxcompdesc {
	uint32_t	rxd_idx:12;	/* Rx descriptor index */
	uint32_t	pad1:2;
	uint32_t	eop:1;		/* End of packet */
	uint32_t	sop:1;		/* Start of packet */
	uint32_t	qid:10;
	uint32_t	rss_type:4;
	uint32_t	no_csum:1;	/* No checksum calculated */
	uint32_t	pad2:1;

	uint32_t	rss_hash:32;	/* RSS hash value */

	uint32_t	len:14;
	uint32_t	error:1;
	uint32_t	vlan:1;		/* 802.1Q VLAN frame */
	uint32_t	vtag:16;	/* VLAN tag */

	uint32_t	csum:16;
	uint32_t	csum_ok:1;	/* TCP/UDP checksum ok */
	uint32_t	udp:1;
	uint32_t	tcp:1;
	uint32_t	ipcsum_ok:1;	/* IP checksum OK */
	uint32_t	ipv6:1;
	uint32_t	ipv4:1;
	uint32_t	fragment:1;	/* IP fragment */
	uint32_t	fcs:1;		/* Frame CRC correct */
	uint32_t	type:7;
	uint32_t	gen:1;
} __packed;

#define VMXNET3_RCD_RSS_TYPE_NONE	0
#define VMXNET3_RCD_RSS_TYPE_IPV4	1
#define VMXNET3_RCD_RSS_TYPE_TCPIPV4	2
#define VMXNET3_RCD_RSS_TYPE_IPV6	3
#define VMXNET3_RCD_RSS_TYPE_TCPIPV6	4

#define VMXNET3_REV1_MAGIC 0xbabefee1

#define VMXNET3_GOS_UNKNOWN 0x00
#define VMXNET3_GOS_LINUX   0x04
#define VMXNET3_GOS_WINDOWS 0x08
#define VMXNET3_GOS_SOLARIS 0x0c
#define VMXNET3_GOS_FREEBSD 0x10
#define VMXNET3_GOS_PXE     0x14

#define VMXNET3_GOS_32BIT   0x01
#define VMXNET3_GOS_64BIT   0x02

#define VMXNET3_MAX_TX_QUEUES 8
#define VMXNET3_MAX_RX_QUEUES 16
#define VMXNET3_MAX_INTRS (VMXNET3_MAX_TX_QUEUES + VMXNET3_MAX_RX_QUEUES + 1)

#define VMXNET3_RX_INTR_INDEX 0
#define VMXNET3_TX_INTR_INDEX 1
#define VMXNET3_EV_INTR_INDEX 2

#define VMXNET3_ICTRL_DISABLE_ALL 0x01

#define VMXNET3_RXMODE_UCAST    0x01
#define VMXNET3_RXMODE_MCAST    0x02
#define VMXNET3_RXMODE_BCAST    0x04
#define VMXNET3_RXMODE_ALLMULTI 0x08
#define VMXNET3_RXMODE_PROMISC  0x10

#define VMXNET3_EVENT_RQERROR 0x01
#define VMXNET3_EVENT_TQERROR 0x02
#define VMXNET3_EVENT_LINK    0x04
#define VMXNET3_EVENT_DIC     0x08
#define VMXNET3_EVENT_DEBUG   0x10

#define VMXNET3_MAX_MTU 9000
#define VMXNET3_MIN_MTU 60

#define VMXNET3_IMM_AUTO	0x00
#define VMXNET3_IMM_ACTIVE	0x01
#define VMXNET3_IMM_LAZY	0x02

#define VMXNET3_IT_AUTO   0x00
#define VMXNET3_IT_LEGACY 0x01
#define VMXNET3_IT_MSI    0x02
#define VMXNET3_IT_MSIX   0x03

struct vmxnet3_driver_shared {
	uint32_t magic;
	uint32_t pad1;

	uint32_t version;		/* driver version */
	uint32_t guest;		/* guest OS */
	uint32_t vmxnet3_revision;	/* supported VMXNET3 revision */
	uint32_t upt_version;		/* supported UPT version */
	uint64_t upt_features;
	uint64_t driver_data;
	uint64_t queue_shared;
	uint32_t driver_data_len;
	uint32_t queue_shared_len;
	uint32_t mtu;
	uint16_t nrxsg_max;
	uint8_t ntxqueue;
	uint8_t nrxqueue;
	uint32_t reserved1[4];

	/* interrupt control */
	uint8_t automask;
	uint8_t nintr;
	uint8_t evintr;
	uint8_t modlevel[VMXNET3_MAX_INTRS];
	uint32_t ictrl;
	uint32_t reserved2[2];

	/* receive filter parameters */
	uint32_t rxmode;
	uint16_t mcast_tablelen;
	uint16_t pad2;
	uint64_t mcast_table;
	uint32_t vlan_filter[4096 / 32];

	struct {
		uint32_t version;
		uint32_t len;
		uint64_t paddr;
	} rss, pm, plugin;

	uint32_t event;
	uint32_t reserved3[5];
} __packed;

struct vmxnet3_txq_shared {
	uint32_t npending;
	uint32_t intr_threshold;
	uint64_t reserved1;

	uint64_t cmd_ring;
	uint64_t data_ring;
	uint64_t comp_ring;
	uint64_t driver_data;
	uint64_t reserved2;
	uint32_t cmd_ring_len;
	uint32_t data_ring_len;
	uint32_t comp_ring_len;
	uint32_t driver_data_len;
	uint8_t intr_idx;
	uint8_t pad1[7];

	uint8_t stopped;
	uint8_t pad2[3];
	uint32_t error;

	struct UPT1_TxStats stats;

	uint8_t pad3[88];
} __packed;

struct vmxnet3_rxq_shared {
	uint8_t update_rxhead;
	uint8_t pad1[7];
	uint64_t reserved1;

	uint64_t cmd_ring[2];
	uint64_t comp_ring;
	uint64_t driver_data;
	uint64_t reserved2;
	uint32_t cmd_ring_len[2];
	uint32_t comp_ring_len;
	uint32_t driver_data_len;
	uint8_t intr_idx;
	uint8_t pad2[7];

	uint8_t stopped;
	uint8_t pad3[3];
	uint32_t error;

	struct UPT1_RxStats stats;

	uint8_t pad4[88];
} __packed;

#define UPT1_RSS_HASH_TYPE_NONE		0x00
#define UPT1_RSS_HASH_TYPE_IPV4		0x01
#define UPT1_RSS_HASH_TYPE_TCP_IPV4	0x02
#define UPT1_RSS_HASH_TYPE_IPV6		0x04
#define UPT1_RSS_HASH_TYPE_TCP_IPV6	0x08

#define UPT1_RSS_HASH_FUNC_NONE		0x00
#define UPT1_RSS_HASH_FUNC_TOEPLITZ	0x01

#define UPT1_RSS_MAX_KEY_SIZE		40
#define UPT1_RSS_MAX_IND_TABLE_SIZE	128

struct vmxnet3_rss_shared {
	uint16_t		hash_type;
	uint16_t		hash_func;
	uint16_t		hash_key_size;
	uint16_t		ind_table_size;
	uint8_t			hash_key[UPT1_RSS_MAX_KEY_SIZE];
	uint8_t			ind_table[UPT1_RSS_MAX_IND_TABLE_SIZE];
} __packed;

