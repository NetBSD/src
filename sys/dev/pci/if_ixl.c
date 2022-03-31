/*	$NetBSD: if_ixl.c,v 1.81 2022/03/31 06:21:41 yamaguchi Exp $	*/

/*
 * Copyright (c) 2013-2015, Intel Corporation
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  3. Neither the name of the Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 2016,2017 David Gwynne <dlg@openbsd.org>
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

/*
 * Copyright (c) 2019 Internet Initiative Japan, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ixl.c,v 1.81 2022/03/31 06:21:41 yamaguchi Exp $");

#ifdef _KERNEL_OPT
#include "opt_net_mpsafe.h"
#include "opt_if_ixl.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <sys/bitops.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/evcnt.h>
#include <sys/interrupt.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/pcq.h>
#include <sys/syslog.h>
#include <sys/workqueue.h>
#include <sys/xcall.h>

#include <sys/bus.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/rss_config.h>

#include <netinet/tcp.h>	/* for struct tcphdr */
#include <netinet/udp.h>	/* for struct udphdr */

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/if_ixlreg.h>
#include <dev/pci/if_ixlvar.h>

#include <prop/proplib.h>

struct ixl_softc; /* defined */

#define I40E_PF_RESET_WAIT_COUNT	200
#define I40E_AQ_LARGE_BUF		512

/* bitfields for Tx queue mapping in QTX_CTL */
#define I40E_QTX_CTL_VF_QUEUE		0x0
#define I40E_QTX_CTL_VM_QUEUE		0x1
#define I40E_QTX_CTL_PF_QUEUE		0x2

#define I40E_QUEUE_TYPE_EOL		0x7ff
#define I40E_INTR_NOTX_QUEUE		0

#define I40E_QUEUE_TYPE_RX		0x0
#define I40E_QUEUE_TYPE_TX		0x1
#define I40E_QUEUE_TYPE_PE_CEQ		0x2
#define I40E_QUEUE_TYPE_UNKNOWN		0x3

#define I40E_ITR_INDEX_RX		0x0
#define I40E_ITR_INDEX_TX		0x1
#define I40E_ITR_INDEX_OTHER		0x2
#define I40E_ITR_INDEX_NONE		0x3
#define IXL_ITR_RX			0x7a /* 4K intrs/sec */
#define IXL_ITR_TX			0x7a /* 4K intrs/sec */

#define I40E_INTR_NOTX_QUEUE		0
#define I40E_INTR_NOTX_INTR		0
#define I40E_INTR_NOTX_RX_QUEUE		0
#define I40E_INTR_NOTX_TX_QUEUE		1
#define I40E_INTR_NOTX_RX_MASK		I40E_PFINT_ICR0_QUEUE_0_MASK
#define I40E_INTR_NOTX_TX_MASK		I40E_PFINT_ICR0_QUEUE_1_MASK

#define I40E_HASH_LUT_SIZE_128		0

#define IXL_ICR0_CRIT_ERR_MASK			\
	(I40E_PFINT_ICR0_PCI_EXCEPTION_MASK |	\
	I40E_PFINT_ICR0_ECC_ERR_MASK |		\
	I40E_PFINT_ICR0_PE_CRITERR_MASK)

#define IXL_QUEUE_MAX_XL710		64
#define IXL_QUEUE_MAX_X722		128

#define IXL_TX_PKT_DESCS		8
#define IXL_TX_PKT_MAXSIZE		(MCLBYTES * IXL_TX_PKT_DESCS)
#define IXL_TX_QUEUE_ALIGN		128
#define IXL_RX_QUEUE_ALIGN		128

#define IXL_MCLBYTES			(MCLBYTES - ETHER_ALIGN)
#define IXL_MTU_ETHERLEN		ETHER_HDR_LEN		\
					+ ETHER_CRC_LEN
#if 0
#define IXL_MAX_MTU			(9728 - IXL_MTU_ETHERLEN)
#else
/* (dbuff * 5) - ETHER_HDR_LEN - ETHER_CRC_LEN */
#define IXL_MAX_MTU			(9600 - IXL_MTU_ETHERLEN)
#endif
#define IXL_MIN_MTU			(ETHER_MIN_LEN - ETHER_CRC_LEN)

#define IXL_PCIREG			PCI_MAPREG_START

#define IXL_ITR0			0x0
#define IXL_ITR1			0x1
#define IXL_ITR2			0x2
#define IXL_NOITR			0x3

#define IXL_AQ_NUM			256
#define IXL_AQ_MASK			(IXL_AQ_NUM - 1)
#define IXL_AQ_ALIGN			64 /* lol */
#define IXL_AQ_BUFLEN			4096

#define IXL_HMC_ROUNDUP			512
#define IXL_HMC_PGSIZE			4096
#define IXL_HMC_DVASZ			sizeof(uint64_t)
#define IXL_HMC_PGS			(IXL_HMC_PGSIZE / IXL_HMC_DVASZ)
#define IXL_HMC_L2SZ			(IXL_HMC_PGSIZE * IXL_HMC_PGS)
#define IXL_HMC_PDVALID			1ULL

#define IXL_ATQ_EXEC_TIMEOUT		(10 * hz)

#define IXL_SRRD_SRCTL_ATTEMPTS		100000

struct ixl_aq_regs {
	bus_size_t		atq_tail;
	bus_size_t		atq_head;
	bus_size_t		atq_len;
	bus_size_t		atq_bal;
	bus_size_t		atq_bah;

	bus_size_t		arq_tail;
	bus_size_t		arq_head;
	bus_size_t		arq_len;
	bus_size_t		arq_bal;
	bus_size_t		arq_bah;

	uint32_t		atq_len_enable;
	uint32_t		atq_tail_mask;
	uint32_t		atq_head_mask;

	uint32_t		arq_len_enable;
	uint32_t		arq_tail_mask;
	uint32_t		arq_head_mask;
};

struct ixl_phy_type {
	uint64_t	phy_type;
	uint64_t	ifm_type;
};

struct ixl_speed_type {
	uint8_t		dev_speed;
	uint64_t	net_speed;
};

struct ixl_hmc_entry {
	uint64_t		 hmc_base;
	uint32_t		 hmc_count;
	uint64_t		 hmc_size;
};

enum  ixl_hmc_types {
	IXL_HMC_LAN_TX = 0,
	IXL_HMC_LAN_RX,
	IXL_HMC_FCOE_CTX,
	IXL_HMC_FCOE_FILTER,
	IXL_HMC_COUNT
};

struct ixl_hmc_pack {
	uint16_t		offset;
	uint16_t		width;
	uint16_t		lsb;
};

/*
 * these hmc objects have weird sizes and alignments, so these are abstract
 * representations of them that are nice for c to populate.
 *
 * the packing code relies on little-endian values being stored in the fields,
 * no high bits in the fields being set, and the fields must be packed in the
 * same order as they are in the ctx structure.
 */

struct ixl_hmc_rxq {
	uint16_t		 head;
	uint8_t			 cpuid;
	uint64_t		 base;
#define IXL_HMC_RXQ_BASE_UNIT		128
	uint16_t		 qlen;
	uint16_t		 dbuff;
#define IXL_HMC_RXQ_DBUFF_UNIT		128
	uint8_t			 hbuff;
#define IXL_HMC_RXQ_HBUFF_UNIT		64
	uint8_t			 dtype;
#define IXL_HMC_RXQ_DTYPE_NOSPLIT	0x0
#define IXL_HMC_RXQ_DTYPE_HSPLIT	0x1
#define IXL_HMC_RXQ_DTYPE_SPLIT_ALWAYS	0x2
	uint8_t			 dsize;
#define IXL_HMC_RXQ_DSIZE_16		0
#define IXL_HMC_RXQ_DSIZE_32		1
	uint8_t			 crcstrip;
	uint8_t			 fc_ena;
	uint8_t			 l2sel;
	uint8_t			 hsplit_0;
	uint8_t			 hsplit_1;
	uint8_t			 showiv;
	uint16_t		 rxmax;
	uint8_t			 tphrdesc_ena;
	uint8_t			 tphwdesc_ena;
	uint8_t			 tphdata_ena;
	uint8_t			 tphhead_ena;
	uint8_t			 lrxqthresh;
	uint8_t			 prefena;
};

static const struct ixl_hmc_pack ixl_hmc_pack_rxq[] = {
	{ offsetof(struct ixl_hmc_rxq, head),		13,	0 },
	{ offsetof(struct ixl_hmc_rxq, cpuid),		8,	13 },
	{ offsetof(struct ixl_hmc_rxq, base),		57,	32 },
	{ offsetof(struct ixl_hmc_rxq, qlen),		13,	89 },
	{ offsetof(struct ixl_hmc_rxq, dbuff),		7,	102 },
	{ offsetof(struct ixl_hmc_rxq, hbuff),		5,	109 },
	{ offsetof(struct ixl_hmc_rxq, dtype),		2,	114 },
	{ offsetof(struct ixl_hmc_rxq, dsize),		1,	116 },
	{ offsetof(struct ixl_hmc_rxq, crcstrip),	1,	117 },
	{ offsetof(struct ixl_hmc_rxq, fc_ena),		1,	118 },
	{ offsetof(struct ixl_hmc_rxq, l2sel),		1,	119 },
	{ offsetof(struct ixl_hmc_rxq, hsplit_0),	4,	120 },
	{ offsetof(struct ixl_hmc_rxq, hsplit_1),	2,	124 },
	{ offsetof(struct ixl_hmc_rxq, showiv),		1,	127 },
	{ offsetof(struct ixl_hmc_rxq, rxmax),		14,	174 },
	{ offsetof(struct ixl_hmc_rxq, tphrdesc_ena),	1,	193 },
	{ offsetof(struct ixl_hmc_rxq, tphwdesc_ena),	1,	194 },
	{ offsetof(struct ixl_hmc_rxq, tphdata_ena),	1,	195 },
	{ offsetof(struct ixl_hmc_rxq, tphhead_ena),	1,	196 },
	{ offsetof(struct ixl_hmc_rxq, lrxqthresh),	3,	198 },
	{ offsetof(struct ixl_hmc_rxq, prefena),	1,	201 },
};

#define IXL_HMC_RXQ_MINSIZE (201 + 1)

struct ixl_hmc_txq {
	uint16_t		head;
	uint8_t			new_context;
	uint64_t		base;
#define IXL_HMC_TXQ_BASE_UNIT		128
	uint8_t			fc_ena;
	uint8_t			timesync_ena;
	uint8_t			fd_ena;
	uint8_t			alt_vlan_ena;
	uint8_t			cpuid;
	uint16_t		thead_wb;
	uint8_t			head_wb_ena;
#define IXL_HMC_TXQ_DESC_WB		0
#define IXL_HMC_TXQ_HEAD_WB		1
	uint16_t		qlen;
	uint8_t			tphrdesc_ena;
	uint8_t			tphrpacket_ena;
	uint8_t			tphwdesc_ena;
	uint64_t		head_wb_addr;
	uint32_t		crc;
	uint16_t		rdylist;
	uint8_t			rdylist_act;
};

static const struct ixl_hmc_pack ixl_hmc_pack_txq[] = {
	{ offsetof(struct ixl_hmc_txq, head),		13,	0 },
	{ offsetof(struct ixl_hmc_txq, new_context),	1,	30 },
	{ offsetof(struct ixl_hmc_txq, base),		57,	32 },
	{ offsetof(struct ixl_hmc_txq, fc_ena),		1,	89 },
	{ offsetof(struct ixl_hmc_txq, timesync_ena),	1,	90 },
	{ offsetof(struct ixl_hmc_txq, fd_ena),		1,	91 },
	{ offsetof(struct ixl_hmc_txq, alt_vlan_ena),	1,	92 },
	{ offsetof(struct ixl_hmc_txq, cpuid),		8,	96 },
/* line 1 */
	{ offsetof(struct ixl_hmc_txq, thead_wb),	13,	0 + 128 },
	{ offsetof(struct ixl_hmc_txq, head_wb_ena),	1,	32 + 128 },
	{ offsetof(struct ixl_hmc_txq, qlen),		13,	33 + 128 },
	{ offsetof(struct ixl_hmc_txq, tphrdesc_ena),	1,	46 + 128 },
	{ offsetof(struct ixl_hmc_txq, tphrpacket_ena),	1,	47 + 128 },
	{ offsetof(struct ixl_hmc_txq, tphwdesc_ena),	1,	48 + 128 },
	{ offsetof(struct ixl_hmc_txq, head_wb_addr),	64,	64 + 128 },
/* line 7 */
	{ offsetof(struct ixl_hmc_txq, crc),		32,	0 + (7*128) },
	{ offsetof(struct ixl_hmc_txq, rdylist),	10,	84 + (7*128) },
	{ offsetof(struct ixl_hmc_txq, rdylist_act),	1,	94 + (7*128) },
};

#define IXL_HMC_TXQ_MINSIZE (94 + (7*128) + 1)

struct ixl_work {
	struct work	 ixw_cookie;
	void		(*ixw_func)(void *);
	void		*ixw_arg;
	unsigned int	 ixw_added;
};
#define IXL_WORKQUEUE_PRI	PRI_SOFTNET

struct ixl_tx_map {
	struct mbuf		*txm_m;
	bus_dmamap_t		 txm_map;
	unsigned int		 txm_eop;
};

struct ixl_tx_ring {
	kmutex_t		 txr_lock;
	struct ixl_softc	*txr_sc;

	unsigned int		 txr_prod;
	unsigned int		 txr_cons;

	struct ixl_tx_map	*txr_maps;
	struct ixl_dmamem	 txr_mem;

	bus_size_t		 txr_tail;
	unsigned int		 txr_qid;
	pcq_t			*txr_intrq;
	void			*txr_si;

	struct evcnt		 txr_defragged;
	struct evcnt		 txr_defrag_failed;
	struct evcnt		 txr_pcqdrop;
	struct evcnt		 txr_transmitdef;
	struct evcnt		 txr_intr;
	struct evcnt		 txr_defer;
};

struct ixl_rx_map {
	struct mbuf		*rxm_m;
	bus_dmamap_t		 rxm_map;
};

struct ixl_rx_ring {
	kmutex_t		 rxr_lock;

	unsigned int		 rxr_prod;
	unsigned int		 rxr_cons;

	struct ixl_rx_map	*rxr_maps;
	struct ixl_dmamem	 rxr_mem;

	struct mbuf		*rxr_m_head;
	struct mbuf		**rxr_m_tail;

	bus_size_t		 rxr_tail;
	unsigned int		 rxr_qid;

	struct evcnt		 rxr_mgethdr_failed;
	struct evcnt		 rxr_mgetcl_failed;
	struct evcnt		 rxr_mbuf_load_failed;
	struct evcnt		 rxr_intr;
	struct evcnt		 rxr_defer;
};

struct ixl_queue_pair {
	struct ixl_softc	*qp_sc;
	struct ixl_tx_ring	*qp_txr;
	struct ixl_rx_ring	*qp_rxr;

	char			 qp_name[16];

	void			*qp_si;
	struct work		 qp_work;
	bool			 qp_workqueue;
};

struct ixl_atq {
	struct ixl_aq_desc	 iatq_desc;
	void			(*iatq_fn)(struct ixl_softc *,
				    const struct ixl_aq_desc *);
};
SIMPLEQ_HEAD(ixl_atq_list, ixl_atq);

struct ixl_product {
	unsigned int	 vendor_id;
	unsigned int	 product_id;
};

struct ixl_stats_counters {
	bool		 isc_has_offset;
	struct evcnt	 isc_crc_errors;
	uint64_t	 isc_crc_errors_offset;
	struct evcnt	 isc_illegal_bytes;
	uint64_t	 isc_illegal_bytes_offset;
	struct evcnt	 isc_rx_bytes;
	uint64_t	 isc_rx_bytes_offset;
	struct evcnt	 isc_rx_discards;
	uint64_t	 isc_rx_discards_offset;
	struct evcnt	 isc_rx_unicast;
	uint64_t	 isc_rx_unicast_offset;
	struct evcnt	 isc_rx_multicast;
	uint64_t	 isc_rx_multicast_offset;
	struct evcnt	 isc_rx_broadcast;
	uint64_t	 isc_rx_broadcast_offset;
	struct evcnt	 isc_rx_size_64;
	uint64_t	 isc_rx_size_64_offset;
	struct evcnt	 isc_rx_size_127;
	uint64_t	 isc_rx_size_127_offset;
	struct evcnt	 isc_rx_size_255;
	uint64_t	 isc_rx_size_255_offset;
	struct evcnt	 isc_rx_size_511;
	uint64_t	 isc_rx_size_511_offset;
	struct evcnt	 isc_rx_size_1023;
	uint64_t	 isc_rx_size_1023_offset;
	struct evcnt	 isc_rx_size_1522;
	uint64_t	 isc_rx_size_1522_offset;
	struct evcnt	 isc_rx_size_big;
	uint64_t	 isc_rx_size_big_offset;
	struct evcnt	 isc_rx_undersize;
	uint64_t	 isc_rx_undersize_offset;
	struct evcnt	 isc_rx_oversize;
	uint64_t	 isc_rx_oversize_offset;
	struct evcnt	 isc_rx_fragments;
	uint64_t	 isc_rx_fragments_offset;
	struct evcnt	 isc_rx_jabber;
	uint64_t	 isc_rx_jabber_offset;
	struct evcnt	 isc_tx_bytes;
	uint64_t	 isc_tx_bytes_offset;
	struct evcnt	 isc_tx_dropped_link_down;
	uint64_t	 isc_tx_dropped_link_down_offset;
	struct evcnt	 isc_tx_unicast;
	uint64_t	 isc_tx_unicast_offset;
	struct evcnt	 isc_tx_multicast;
	uint64_t	 isc_tx_multicast_offset;
	struct evcnt	 isc_tx_broadcast;
	uint64_t	 isc_tx_broadcast_offset;
	struct evcnt	 isc_tx_size_64;
	uint64_t	 isc_tx_size_64_offset;
	struct evcnt	 isc_tx_size_127;
	uint64_t	 isc_tx_size_127_offset;
	struct evcnt	 isc_tx_size_255;
	uint64_t	 isc_tx_size_255_offset;
	struct evcnt	 isc_tx_size_511;
	uint64_t	 isc_tx_size_511_offset;
	struct evcnt	 isc_tx_size_1023;
	uint64_t	 isc_tx_size_1023_offset;
	struct evcnt	 isc_tx_size_1522;
	uint64_t	 isc_tx_size_1522_offset;
	struct evcnt	 isc_tx_size_big;
	uint64_t	 isc_tx_size_big_offset;
	struct evcnt	 isc_mac_local_faults;
	uint64_t	 isc_mac_local_faults_offset;
	struct evcnt	 isc_mac_remote_faults;
	uint64_t	 isc_mac_remote_faults_offset;
	struct evcnt	 isc_link_xon_rx;
	uint64_t	 isc_link_xon_rx_offset;
	struct evcnt	 isc_link_xon_tx;
	uint64_t	 isc_link_xon_tx_offset;
	struct evcnt	 isc_link_xoff_rx;
	uint64_t	 isc_link_xoff_rx_offset;
	struct evcnt	 isc_link_xoff_tx;
	uint64_t	 isc_link_xoff_tx_offset;
	struct evcnt	 isc_vsi_rx_discards;
	uint64_t	 isc_vsi_rx_discards_offset;
	struct evcnt	 isc_vsi_rx_bytes;
	uint64_t	 isc_vsi_rx_bytes_offset;
	struct evcnt	 isc_vsi_rx_unicast;
	uint64_t	 isc_vsi_rx_unicast_offset;
	struct evcnt	 isc_vsi_rx_multicast;
	uint64_t	 isc_vsi_rx_multicast_offset;
	struct evcnt	 isc_vsi_rx_broadcast;
	uint64_t	 isc_vsi_rx_broadcast_offset;
	struct evcnt	 isc_vsi_tx_errors;
	uint64_t	 isc_vsi_tx_errors_offset;
	struct evcnt	 isc_vsi_tx_bytes;
	uint64_t	 isc_vsi_tx_bytes_offset;
	struct evcnt	 isc_vsi_tx_unicast;
	uint64_t	 isc_vsi_tx_unicast_offset;
	struct evcnt	 isc_vsi_tx_multicast;
	uint64_t	 isc_vsi_tx_multicast_offset;
	struct evcnt	 isc_vsi_tx_broadcast;
	uint64_t	 isc_vsi_tx_broadcast_offset;
};

/*
 * Locking notes:
 * + a field in ixl_tx_ring is protected by txr_lock (a spin mutex), and
 *   a field in ixl_rx_ring is protected by rxr_lock (a spin mutex).
 *    - more than one lock of them cannot be held at once.
 * + a field named sc_atq_* in ixl_softc is protected by sc_atq_lock
 *   (a spin mutex).
 *    - the lock cannot held with txr_lock or rxr_lock.
 * + a field named sc_arq_* is not protected by any lock.
 *    - operations for sc_arq_* is done in one context related to
 *      sc_arq_task.
 * + other fields in ixl_softc is protected by sc_cfg_lock
 *   (an adaptive mutex)
 *    - It must be held before another lock is held, and It can be
 *      released after the other lock is released.
 * */

struct ixl_softc {
	device_t		 sc_dev;
	struct ethercom		 sc_ec;
	bool			 sc_attached;
	bool			 sc_dead;
	uint32_t		 sc_port;
	struct sysctllog	*sc_sysctllog;
	struct workqueue	*sc_workq;
	struct workqueue	*sc_workq_txrx;
	int			 sc_stats_intval;
	callout_t		 sc_stats_callout;
	struct ixl_work		 sc_stats_task;
	struct ixl_stats_counters
				 sc_stats_counters;
	uint8_t			 sc_enaddr[ETHER_ADDR_LEN];
	struct ifmedia		 sc_media;
	uint64_t		 sc_media_status;
	uint64_t		 sc_media_active;
	uint64_t		 sc_phy_types;
	uint8_t			 sc_phy_abilities;
	uint8_t			 sc_phy_linkspeed;
	uint8_t			 sc_phy_fec_cfg;
	uint16_t		 sc_eee_cap;
	uint32_t		 sc_eeer_val;
	uint8_t			 sc_d3_lpan;
	kmutex_t		 sc_cfg_lock;
	enum i40e_mac_type	 sc_mac_type;
	uint32_t		 sc_rss_table_size;
	uint32_t		 sc_rss_table_entry_width;
	bool			 sc_txrx_workqueue;
	u_int			 sc_tx_process_limit;
	u_int			 sc_rx_process_limit;
	u_int			 sc_tx_intr_process_limit;
	u_int			 sc_rx_intr_process_limit;

	int			 sc_cur_ec_capenable;

	struct pci_attach_args	 sc_pa;
	pci_intr_handle_t	*sc_ihp;
	void			**sc_ihs;
	unsigned int		 sc_nintrs;

	bus_dma_tag_t		 sc_dmat;
	bus_space_tag_t		 sc_memt;
	bus_space_handle_t	 sc_memh;
	bus_size_t		 sc_mems;

	uint8_t			 sc_pf_id;
	uint16_t		 sc_uplink_seid;	/* le */
	uint16_t		 sc_downlink_seid;	/* le */
	uint16_t		 sc_vsi_number;
	uint16_t		 sc_vsi_stat_counter_idx;
	uint16_t		 sc_seid;
	unsigned int		 sc_base_queue;

	pci_intr_type_t		 sc_intrtype;
	unsigned int		 sc_msix_vector_queue;

	struct ixl_dmamem	 sc_scratch;
	struct ixl_dmamem	 sc_aqbuf;

	const struct ixl_aq_regs *
				 sc_aq_regs;
	uint32_t		 sc_aq_flags;
#define IXL_SC_AQ_FLAG_RXCTL	__BIT(0)
#define IXL_SC_AQ_FLAG_NVMLOCK	__BIT(1)
#define IXL_SC_AQ_FLAG_NVMREAD	__BIT(2)
#define IXL_SC_AQ_FLAG_RSS	__BIT(3)

	kmutex_t		 sc_atq_lock;
	kcondvar_t		 sc_atq_cv;
	struct ixl_dmamem	 sc_atq;
	unsigned int		 sc_atq_prod;
	unsigned int		 sc_atq_cons;

	struct ixl_dmamem	 sc_arq;
	struct ixl_work		 sc_arq_task;
	struct ixl_aq_bufs	 sc_arq_idle;
	struct ixl_aq_buf	*sc_arq_live[IXL_AQ_NUM];
	unsigned int		 sc_arq_prod;
	unsigned int		 sc_arq_cons;

	struct ixl_work		 sc_link_state_task;
	struct ixl_atq		 sc_link_state_atq;

	struct ixl_dmamem	 sc_hmc_sd;
	struct ixl_dmamem	 sc_hmc_pd;
	struct ixl_hmc_entry	 sc_hmc_entries[IXL_HMC_COUNT];

	struct if_percpuq	*sc_ipq;
	unsigned int		 sc_tx_ring_ndescs;
	unsigned int		 sc_rx_ring_ndescs;
	unsigned int		 sc_nqueue_pairs;
	unsigned int		 sc_nqueue_pairs_max;
	unsigned int		 sc_nqueue_pairs_device;
	struct ixl_queue_pair	*sc_qps;
	uint32_t		 sc_itr_rx;
	uint32_t		 sc_itr_tx;

	struct evcnt		 sc_event_atq;
	struct evcnt		 sc_event_link;
	struct evcnt		 sc_event_ecc_err;
	struct evcnt		 sc_event_pci_exception;
	struct evcnt		 sc_event_crit_err;
};

#define IXL_TXRX_PROCESS_UNLIMIT	UINT_MAX
#define IXL_TX_PROCESS_LIMIT		256
#define IXL_RX_PROCESS_LIMIT		256
#define IXL_TX_INTR_PROCESS_LIMIT	256
#define IXL_RX_INTR_PROCESS_LIMIT	0U

#define IXL_IFCAP_RXCSUM	(IFCAP_CSUM_IPv4_Rx |	\
				 IFCAP_CSUM_TCPv4_Rx |	\
				 IFCAP_CSUM_UDPv4_Rx |	\
				 IFCAP_CSUM_TCPv6_Rx |	\
				 IFCAP_CSUM_UDPv6_Rx)
#define IXL_IFCAP_TXCSUM	(IFCAP_CSUM_IPv4_Tx |	\
				 IFCAP_CSUM_TCPv4_Tx |	\
				 IFCAP_CSUM_UDPv4_Tx |	\
				 IFCAP_CSUM_TCPv6_Tx |	\
				 IFCAP_CSUM_UDPv6_Tx)
#define IXL_CSUM_ALL_OFFLOAD	(M_CSUM_IPv4 |			\
				 M_CSUM_TCPv4 | M_CSUM_TCPv6 |	\
				 M_CSUM_UDPv4 | M_CSUM_UDPv6)

#define delaymsec(_x)	DELAY(1000 * (_x))
#ifdef IXL_DEBUG
#define DDPRINTF(sc, fmt, args...)			\
do {							\
	if ((sc) != NULL) {				\
		device_printf(				\
		    ((struct ixl_softc *)(sc))->sc_dev,	\
		    "");				\
	}						\
	printf("%s:\t" fmt, __func__, ##args);		\
} while (0)
#else
#define DDPRINTF(sc, fmt, args...)	__nothing
#endif
#ifndef IXL_STATS_INTERVAL_MSEC
#define IXL_STATS_INTERVAL_MSEC	10000
#endif
#ifndef IXL_QUEUE_NUM
#define IXL_QUEUE_NUM		0
#endif

static bool		 ixl_param_nomsix = false;
static int		 ixl_param_stats_interval = IXL_STATS_INTERVAL_MSEC;
static int		 ixl_param_nqps_limit = IXL_QUEUE_NUM;
static unsigned int	 ixl_param_tx_ndescs = 512;
static unsigned int	 ixl_param_rx_ndescs = 256;

static enum i40e_mac_type
	    ixl_mactype(pci_product_id_t);
static void	ixl_pci_csr_setup(pci_chipset_tag_t, pcitag_t);
static void	ixl_clear_hw(struct ixl_softc *);
static int	ixl_pf_reset(struct ixl_softc *);

static int	ixl_dmamem_alloc(struct ixl_softc *, struct ixl_dmamem *,
		    bus_size_t, bus_size_t);
static void	ixl_dmamem_free(struct ixl_softc *, struct ixl_dmamem *);

static int	ixl_arq_fill(struct ixl_softc *);
static void	ixl_arq_unfill(struct ixl_softc *);

static int	ixl_atq_poll(struct ixl_softc *, struct ixl_aq_desc *,
		    unsigned int);
static void	ixl_atq_set(struct ixl_atq *,
		    void (*)(struct ixl_softc *, const struct ixl_aq_desc *));
static int	ixl_atq_post_locked(struct ixl_softc *, struct ixl_atq *);
static void	ixl_atq_done(struct ixl_softc *);
static int	ixl_atq_exec(struct ixl_softc *, struct ixl_atq *);
static int	ixl_atq_exec_locked(struct ixl_softc *, struct ixl_atq *);
static int	ixl_get_version(struct ixl_softc *);
static int	ixl_get_nvm_version(struct ixl_softc *);
static int	ixl_get_hw_capabilities(struct ixl_softc *);
static int	ixl_pxe_clear(struct ixl_softc *);
static int	ixl_lldp_shut(struct ixl_softc *);
static int	ixl_get_mac(struct ixl_softc *);
static int	ixl_get_switch_config(struct ixl_softc *);
static int	ixl_phy_mask_ints(struct ixl_softc *);
static int	ixl_get_phy_info(struct ixl_softc *);
static int	ixl_set_phy_config(struct ixl_softc *, uint8_t, uint8_t, bool);
static int	ixl_set_phy_autoselect(struct ixl_softc *);
static int	ixl_restart_an(struct ixl_softc *);
static int	ixl_hmc(struct ixl_softc *);
static void	ixl_hmc_free(struct ixl_softc *);
static int	ixl_get_vsi(struct ixl_softc *);
static int	ixl_set_vsi(struct ixl_softc *);
static void	ixl_set_filter_control(struct ixl_softc *);
static void	ixl_get_link_status(void *);
static int	ixl_get_link_status_poll(struct ixl_softc *, int *);
static void	ixl_get_link_status_done(struct ixl_softc *,
		    const struct ixl_aq_desc *);
static int	ixl_set_link_status_locked(struct ixl_softc *,
		    const struct ixl_aq_desc *);
static uint64_t	ixl_search_link_speed(uint8_t);
static uint8_t	ixl_search_baudrate(uint64_t);
static void	ixl_config_rss(struct ixl_softc *);
static int	ixl_add_macvlan(struct ixl_softc *, const uint8_t *,
		    uint16_t, uint16_t);
static int	ixl_remove_macvlan(struct ixl_softc *, const uint8_t *,
		    uint16_t, uint16_t);
static void	ixl_arq(void *);
static void	ixl_hmc_pack(void *, const void *,
		    const struct ixl_hmc_pack *, unsigned int);
static uint32_t	ixl_rd_rx_csr(struct ixl_softc *, uint32_t);
static void	ixl_wr_rx_csr(struct ixl_softc *, uint32_t, uint32_t);
static int	ixl_rd16_nvm(struct ixl_softc *, uint16_t, uint16_t *);

static int	ixl_match(device_t, cfdata_t, void *);
static void	ixl_attach(device_t, device_t, void *);
static int	ixl_detach(device_t, int);

static void	ixl_media_add(struct ixl_softc *);
static int	ixl_media_change(struct ifnet *);
static void	ixl_media_status(struct ifnet *, struct ifmediareq *);
static void	ixl_watchdog(struct ifnet *);
static int	ixl_ioctl(struct ifnet *, u_long, void *);
static void	ixl_start(struct ifnet *);
static int	ixl_transmit(struct ifnet *, struct mbuf *);
static void	ixl_deferred_transmit(void *);
static int	ixl_intr(void *);
static int	ixl_queue_intr(void *);
static int	ixl_other_intr(void *);
static void	ixl_handle_queue(void *);
static void	ixl_handle_queue_wk(struct work *, void *);
static void	ixl_sched_handle_queue(struct ixl_softc *,
		    struct ixl_queue_pair *);
static int	ixl_init(struct ifnet *);
static int	ixl_init_locked(struct ixl_softc *);
static void	ixl_stop(struct ifnet *, int);
static void	ixl_stop_locked(struct ixl_softc *);
static int	ixl_iff(struct ixl_softc *);
static int	ixl_ifflags_cb(struct ethercom *);
static int	ixl_setup_interrupts(struct ixl_softc *);
static int	ixl_establish_intx(struct ixl_softc *);
static int	ixl_establish_msix(struct ixl_softc *);
static void	ixl_enable_queue_intr(struct ixl_softc *,
		    struct ixl_queue_pair *);
static void	ixl_disable_queue_intr(struct ixl_softc *,
		    struct ixl_queue_pair *);
static void	ixl_enable_other_intr(struct ixl_softc *);
static void	ixl_disable_other_intr(struct ixl_softc *);
static void	ixl_config_queue_intr(struct ixl_softc *);
static void	ixl_config_other_intr(struct ixl_softc *);

static struct ixl_tx_ring *
		ixl_txr_alloc(struct ixl_softc *, unsigned int);
static void	ixl_txr_qdis(struct ixl_softc *, struct ixl_tx_ring *, int);
static void	ixl_txr_config(struct ixl_softc *, struct ixl_tx_ring *);
static int	ixl_txr_enabled(struct ixl_softc *, struct ixl_tx_ring *);
static int	ixl_txr_disabled(struct ixl_softc *, struct ixl_tx_ring *);
static void	ixl_txr_unconfig(struct ixl_softc *, struct ixl_tx_ring *);
static void	ixl_txr_clean(struct ixl_softc *, struct ixl_tx_ring *);
static void	ixl_txr_free(struct ixl_softc *, struct ixl_tx_ring *);
static int	ixl_txeof(struct ixl_softc *, struct ixl_tx_ring *, u_int);

static struct ixl_rx_ring *
		ixl_rxr_alloc(struct ixl_softc *, unsigned int);
static void	ixl_rxr_config(struct ixl_softc *, struct ixl_rx_ring *);
static int	ixl_rxr_enabled(struct ixl_softc *, struct ixl_rx_ring *);
static int	ixl_rxr_disabled(struct ixl_softc *, struct ixl_rx_ring *);
static void	ixl_rxr_unconfig(struct ixl_softc *, struct ixl_rx_ring *);
static void	ixl_rxr_clean(struct ixl_softc *, struct ixl_rx_ring *);
static void	ixl_rxr_free(struct ixl_softc *, struct ixl_rx_ring *);
static int	ixl_rxeof(struct ixl_softc *, struct ixl_rx_ring *, u_int);
static int	ixl_rxfill(struct ixl_softc *, struct ixl_rx_ring *);

static struct workqueue *
    ixl_workq_create(const char *, pri_t, int, int);
static void	ixl_workq_destroy(struct workqueue *);
static int	ixl_workqs_teardown(device_t);
static void	ixl_work_set(struct ixl_work *, void (*)(void *), void *);
static void	ixl_work_add(struct workqueue *, struct ixl_work *);
static void	ixl_work_wait(struct workqueue *, struct ixl_work *);
static void	ixl_workq_work(struct work *, void *);
static const struct ixl_product *
		ixl_lookup(const struct pci_attach_args *pa);
static void	ixl_link_state_update(struct ixl_softc *,
		    const struct ixl_aq_desc *);
static int	ixl_vlan_cb(struct ethercom *, uint16_t, bool);
static int	ixl_setup_vlan_hwfilter(struct ixl_softc *);
static void	ixl_teardown_vlan_hwfilter(struct ixl_softc *);
static int	ixl_update_macvlan(struct ixl_softc *);
static int	ixl_setup_interrupts(struct ixl_softc *);
static void	ixl_teardown_interrupts(struct ixl_softc *);
static int	ixl_setup_stats(struct ixl_softc *);
static void	ixl_teardown_stats(struct ixl_softc *);
static void	ixl_stats_callout(void *);
static void	ixl_stats_update(void *);
static int	ixl_setup_sysctls(struct ixl_softc *);
static void	ixl_teardown_sysctls(struct ixl_softc *);
static int	ixl_sysctl_itr_handler(SYSCTLFN_PROTO);
static int	ixl_queue_pairs_alloc(struct ixl_softc *);
static void	ixl_queue_pairs_free(struct ixl_softc *);

static const struct ixl_phy_type ixl_phy_type_map[] = {
	{ 1ULL << IXL_PHY_TYPE_SGMII,		IFM_1000_SGMII },
	{ 1ULL << IXL_PHY_TYPE_1000BASE_KX,	IFM_1000_KX },
	{ 1ULL << IXL_PHY_TYPE_10GBASE_KX4,	IFM_10G_KX4 },
	{ 1ULL << IXL_PHY_TYPE_10GBASE_KR,	IFM_10G_KR },
	{ 1ULL << IXL_PHY_TYPE_40GBASE_KR4,	IFM_40G_KR4 },
	{ 1ULL << IXL_PHY_TYPE_XAUI |
	  1ULL << IXL_PHY_TYPE_XFI,		IFM_10G_CX4 },
	{ 1ULL << IXL_PHY_TYPE_SFI,		IFM_10G_SFI },
	{ 1ULL << IXL_PHY_TYPE_XLAUI |
	  1ULL << IXL_PHY_TYPE_XLPPI,		IFM_40G_XLPPI },
	{ 1ULL << IXL_PHY_TYPE_40GBASE_CR4_CU |
	  1ULL << IXL_PHY_TYPE_40GBASE_CR4,	IFM_40G_CR4 },
	{ 1ULL << IXL_PHY_TYPE_10GBASE_CR1_CU |
	  1ULL << IXL_PHY_TYPE_10GBASE_CR1,	IFM_10G_CR1 },
	{ 1ULL << IXL_PHY_TYPE_10GBASE_AOC,	IFM_10G_AOC },
	{ 1ULL << IXL_PHY_TYPE_40GBASE_AOC,	IFM_40G_AOC },
	{ 1ULL << IXL_PHY_TYPE_100BASE_TX,	IFM_100_TX },
	{ 1ULL << IXL_PHY_TYPE_1000BASE_T_OPTICAL |
	  1ULL << IXL_PHY_TYPE_1000BASE_T,	IFM_1000_T },
	{ 1ULL << IXL_PHY_TYPE_10GBASE_T,	IFM_10G_T },
	{ 1ULL << IXL_PHY_TYPE_10GBASE_SR,	IFM_10G_SR },
	{ 1ULL << IXL_PHY_TYPE_10GBASE_LR,	IFM_10G_LR },
	{ 1ULL << IXL_PHY_TYPE_10GBASE_SFPP_CU,	IFM_10G_TWINAX },
	{ 1ULL << IXL_PHY_TYPE_40GBASE_SR4,	IFM_40G_SR4 },
	{ 1ULL << IXL_PHY_TYPE_40GBASE_LR4,	IFM_40G_LR4 },
	{ 1ULL << IXL_PHY_TYPE_1000BASE_SX,	IFM_1000_SX },
	{ 1ULL << IXL_PHY_TYPE_1000BASE_LX,	IFM_1000_LX },
	{ 1ULL << IXL_PHY_TYPE_20GBASE_KR2,	IFM_20G_KR2 },
	{ 1ULL << IXL_PHY_TYPE_25GBASE_KR,	IFM_25G_KR },
	{ 1ULL << IXL_PHY_TYPE_25GBASE_CR,	IFM_25G_CR },
	{ 1ULL << IXL_PHY_TYPE_25GBASE_SR,	IFM_25G_SR },
	{ 1ULL << IXL_PHY_TYPE_25GBASE_LR,	IFM_25G_LR },
	{ 1ULL << IXL_PHY_TYPE_25GBASE_AOC,	IFM_25G_AOC },
	{ 1ULL << IXL_PHY_TYPE_25GBASE_ACC,	IFM_25G_ACC },
	{ 1ULL << IXL_PHY_TYPE_2500BASE_T_1,	IFM_2500_T },
	{ 1ULL << IXL_PHY_TYPE_5000BASE_T_1,	IFM_5000_T },
	{ 1ULL << IXL_PHY_TYPE_2500BASE_T_2,	IFM_2500_T },
	{ 1ULL << IXL_PHY_TYPE_5000BASE_T_2,	IFM_5000_T },
};

static const struct ixl_speed_type ixl_speed_type_map[] = {
	{ IXL_AQ_LINK_SPEED_40GB,		IF_Gbps(40) },
	{ IXL_AQ_LINK_SPEED_25GB,		IF_Gbps(25) },
	{ IXL_AQ_LINK_SPEED_10GB,		IF_Gbps(10) },
	{ IXL_AQ_LINK_SPEED_5000MB,		IF_Mbps(5000) },
	{ IXL_AQ_LINK_SPEED_2500MB,		IF_Mbps(2500) },
	{ IXL_AQ_LINK_SPEED_1000MB,		IF_Mbps(1000) },
	{ IXL_AQ_LINK_SPEED_100MB,		IF_Mbps(100)},
};

static const struct ixl_aq_regs ixl_pf_aq_regs = {
	.atq_tail	= I40E_PF_ATQT,
	.atq_tail_mask	= I40E_PF_ATQT_ATQT_MASK,
	.atq_head	= I40E_PF_ATQH,
	.atq_head_mask	= I40E_PF_ATQH_ATQH_MASK,
	.atq_len	= I40E_PF_ATQLEN,
	.atq_bal	= I40E_PF_ATQBAL,
	.atq_bah	= I40E_PF_ATQBAH,
	.atq_len_enable	= I40E_PF_ATQLEN_ATQENABLE_MASK,

	.arq_tail	= I40E_PF_ARQT,
	.arq_tail_mask	= I40E_PF_ARQT_ARQT_MASK,
	.arq_head	= I40E_PF_ARQH,
	.arq_head_mask	= I40E_PF_ARQH_ARQH_MASK,
	.arq_len	= I40E_PF_ARQLEN,
	.arq_bal	= I40E_PF_ARQBAL,
	.arq_bah	= I40E_PF_ARQBAH,
	.arq_len_enable	= I40E_PF_ARQLEN_ARQENABLE_MASK,
};

#define ixl_rd(_s, _r)			\
	bus_space_read_4((_s)->sc_memt, (_s)->sc_memh, (_r))
#define ixl_wr(_s, _r, _v)		\
	bus_space_write_4((_s)->sc_memt, (_s)->sc_memh, (_r), (_v))
#define ixl_barrier(_s, _r, _l, _o) \
    bus_space_barrier((_s)->sc_memt, (_s)->sc_memh, (_r), (_l), (_o))
#define ixl_flush(_s)	(void)ixl_rd((_s), I40E_GLGEN_STAT)
#define ixl_nqueues(_sc)	(1 << ((_sc)->sc_nqueue_pairs - 1))

CFATTACH_DECL3_NEW(ixl, sizeof(struct ixl_softc),
    ixl_match, ixl_attach, ixl_detach, NULL, NULL, NULL,
    DVF_DETACH_SHUTDOWN);

static const struct ixl_product ixl_products[] = {
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_XL710_SFP },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_XL710_KX_B },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_XL710_KX_C },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_XL710_QSFP_A },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_XL710_QSFP_B },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_XL710_QSFP_C },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_X710_10G_T_1 },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_X710_10G_T_2 },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_XL710_20G_BP_1 },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_XL710_20G_BP_2 },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_X710_T4_10G },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_XXV710_25G_BP },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_XXV710_25G_SFP28 },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_X722_KX },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_X722_QSFP },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_X722_SFP },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_X722_1G_BASET },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_X722_10G_BASET },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_X722_I_SFP },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_X710_10G_SFP },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_X710_10G_BP },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_V710_5G_T},
	/* required last entry */
	{0, 0}
};

static const struct ixl_product *
ixl_lookup(const struct pci_attach_args *pa)
{
	const struct ixl_product *ixlp;

	for (ixlp = ixl_products; ixlp->vendor_id != 0; ixlp++) {
		if (PCI_VENDOR(pa->pa_id) == ixlp->vendor_id &&
		    PCI_PRODUCT(pa->pa_id) == ixlp->product_id)
			return ixlp;
	}

	return NULL;
}

static void
ixl_intr_barrier(void)
{

	/* wait for finish of all handler */
	xc_barrier(0);
}

static int
ixl_match(device_t parent, cfdata_t match, void *aux)
{
	const struct pci_attach_args *pa = aux;

	return (ixl_lookup(pa) != NULL) ? 1 : 0;
}

static void
ixl_attach(device_t parent, device_t self, void *aux)
{
	struct ixl_softc *sc;
	struct pci_attach_args *pa = aux;
	struct ifnet *ifp;
	pcireg_t memtype;
	uint32_t firstq, port, ari, func;
	char xnamebuf[32];
	int tries, rv, link;

	sc = device_private(self);
	sc->sc_dev = self;
	ifp = &sc->sc_ec.ec_if;

	sc->sc_pa = *pa;
	sc->sc_dmat = (pci_dma64_available(pa)) ?
	    pa->pa_dmat64 : pa->pa_dmat;
	sc->sc_aq_regs = &ixl_pf_aq_regs;

	sc->sc_mac_type = ixl_mactype(PCI_PRODUCT(pa->pa_id));

	ixl_pci_csr_setup(pa->pa_pc, pa->pa_tag);

	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, IXL_PCIREG);
	if (pci_mapreg_map(pa, IXL_PCIREG, memtype, 0,
	    &sc->sc_memt, &sc->sc_memh, NULL, &sc->sc_mems)) {
		aprint_error(": unable to map registers\n");
		return;
	}

	mutex_init(&sc->sc_cfg_lock, MUTEX_DEFAULT, IPL_SOFTNET);

	firstq = ixl_rd(sc, I40E_PFLAN_QALLOC);
	firstq &= I40E_PFLAN_QALLOC_FIRSTQ_MASK;
	firstq >>= I40E_PFLAN_QALLOC_FIRSTQ_SHIFT;
	sc->sc_base_queue = firstq;

	ixl_clear_hw(sc);
	if (ixl_pf_reset(sc) == -1) {
		/* error printed by ixl pf_reset */
		goto unmap;
	}

	port = ixl_rd(sc, I40E_PFGEN_PORTNUM);
	port &= I40E_PFGEN_PORTNUM_PORT_NUM_MASK;
	port >>= I40E_PFGEN_PORTNUM_PORT_NUM_SHIFT;
	sc->sc_port = port;
	aprint_normal(": port %u", sc->sc_port);

	ari = ixl_rd(sc, I40E_GLPCI_CAPSUP);
	ari &= I40E_GLPCI_CAPSUP_ARI_EN_MASK;
	ari >>= I40E_GLPCI_CAPSUP_ARI_EN_SHIFT;

	func = ixl_rd(sc, I40E_PF_FUNC_RID);
	sc->sc_pf_id = func & (ari ? 0xff : 0x7);

	/* initialise the adminq */

	mutex_init(&sc->sc_atq_lock, MUTEX_DEFAULT, IPL_NET);

	if (ixl_dmamem_alloc(sc, &sc->sc_atq,
	    sizeof(struct ixl_aq_desc) * IXL_AQ_NUM, IXL_AQ_ALIGN) != 0) {
		aprint_error("\n" "%s: unable to allocate atq\n",
		    device_xname(self));
		goto unmap;
	}

	SIMPLEQ_INIT(&sc->sc_arq_idle);
	ixl_work_set(&sc->sc_arq_task, ixl_arq, sc);
	sc->sc_arq_cons = 0;
	sc->sc_arq_prod = 0;

	if (ixl_dmamem_alloc(sc, &sc->sc_arq,
	    sizeof(struct ixl_aq_desc) * IXL_AQ_NUM, IXL_AQ_ALIGN) != 0) {
		aprint_error("\n" "%s: unable to allocate arq\n",
		    device_xname(self));
		goto free_atq;
	}

	if (!ixl_arq_fill(sc)) {
		aprint_error("\n" "%s: unable to fill arq descriptors\n",
		    device_xname(self));
		goto free_arq;
	}

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_arq),
	    0, IXL_DMA_LEN(&sc->sc_arq),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	for (tries = 0; tries < 10; tries++) {
		sc->sc_atq_cons = 0;
		sc->sc_atq_prod = 0;

		ixl_wr(sc, sc->sc_aq_regs->atq_head, 0);
		ixl_wr(sc, sc->sc_aq_regs->arq_head, 0);
		ixl_wr(sc, sc->sc_aq_regs->atq_tail, 0);
		ixl_wr(sc, sc->sc_aq_regs->arq_tail, 0);

		ixl_barrier(sc, 0, sc->sc_mems, BUS_SPACE_BARRIER_WRITE);

		ixl_wr(sc, sc->sc_aq_regs->atq_bal,
		    ixl_dmamem_lo(&sc->sc_atq));
		ixl_wr(sc, sc->sc_aq_regs->atq_bah,
		    ixl_dmamem_hi(&sc->sc_atq));
		ixl_wr(sc, sc->sc_aq_regs->atq_len,
		    sc->sc_aq_regs->atq_len_enable | IXL_AQ_NUM);

		ixl_wr(sc, sc->sc_aq_regs->arq_bal,
		    ixl_dmamem_lo(&sc->sc_arq));
		ixl_wr(sc, sc->sc_aq_regs->arq_bah,
		    ixl_dmamem_hi(&sc->sc_arq));
		ixl_wr(sc, sc->sc_aq_regs->arq_len,
		    sc->sc_aq_regs->arq_len_enable | IXL_AQ_NUM);

		rv = ixl_get_version(sc);
		if (rv == 0)
			break;
		if (rv != ETIMEDOUT) {
			aprint_error(", unable to get firmware version\n");
			goto shutdown;
		}

		delaymsec(100);
	}

	ixl_wr(sc, sc->sc_aq_regs->arq_tail, sc->sc_arq_prod);

	if (ixl_dmamem_alloc(sc, &sc->sc_aqbuf, IXL_AQ_BUFLEN, 0) != 0) {
		aprint_error_dev(self, ", unable to allocate nvm buffer\n");
		goto shutdown;
	}

	ixl_get_nvm_version(sc);

	if (sc->sc_mac_type == I40E_MAC_X722)
		sc->sc_nqueue_pairs_device = IXL_QUEUE_MAX_X722;
	else
		sc->sc_nqueue_pairs_device = IXL_QUEUE_MAX_XL710;

	rv = ixl_get_hw_capabilities(sc);
	if (rv != 0) {
		aprint_error(", GET HW CAPABILITIES %s\n",
		    rv == ETIMEDOUT ? "timeout" : "error");
		goto free_aqbuf;
	}

	sc->sc_nqueue_pairs_max = MIN((int)sc->sc_nqueue_pairs_device, ncpu);
	if (ixl_param_nqps_limit > 0) {
		sc->sc_nqueue_pairs_max = MIN((int)sc->sc_nqueue_pairs_max,
		    ixl_param_nqps_limit);
	}

	sc->sc_nqueue_pairs = sc->sc_nqueue_pairs_max;
	sc->sc_tx_ring_ndescs = ixl_param_tx_ndescs;
	sc->sc_rx_ring_ndescs = ixl_param_rx_ndescs;

	KASSERT(IXL_TXRX_PROCESS_UNLIMIT > sc->sc_rx_ring_ndescs);
	KASSERT(IXL_TXRX_PROCESS_UNLIMIT > sc->sc_tx_ring_ndescs);
	KASSERT(sc->sc_rx_ring_ndescs ==
	    (1U << (fls32(sc->sc_rx_ring_ndescs) - 1)));
	KASSERT(sc->sc_tx_ring_ndescs ==
	    (1U << (fls32(sc->sc_tx_ring_ndescs) - 1)));

	if (ixl_get_mac(sc) != 0) {
		/* error printed by ixl_get_mac */
		goto free_aqbuf;
	}

	aprint_normal("\n");
	aprint_naive("\n");

	aprint_normal_dev(self, "Ethernet address %s\n",
	    ether_sprintf(sc->sc_enaddr));

	rv = ixl_pxe_clear(sc);
	if (rv != 0) {
		aprint_debug_dev(self, "CLEAR PXE MODE %s\n",
		    rv == ETIMEDOUT ? "timeout" : "error");
	}

	ixl_set_filter_control(sc);

	if (ixl_hmc(sc) != 0) {
		/* error printed by ixl_hmc */
		goto free_aqbuf;
	}

	if (ixl_lldp_shut(sc) != 0) {
		/* error printed by ixl_lldp_shut */
		goto free_hmc;
	}

	if (ixl_phy_mask_ints(sc) != 0) {
		/* error printed by ixl_phy_mask_ints */
		goto free_hmc;
	}

	if (ixl_restart_an(sc) != 0) {
		/* error printed by ixl_restart_an */
		goto free_hmc;
	}

	if (ixl_get_switch_config(sc) != 0) {
		/* error printed by ixl_get_switch_config */
		goto free_hmc;
	}

	rv = ixl_get_link_status_poll(sc, NULL);
	if (rv != 0) {
		aprint_error_dev(self, "GET LINK STATUS %s\n",
		    rv == ETIMEDOUT ? "timeout" : "error");
		goto free_hmc;
	}

	/*
	 * The FW often returns EIO in "Get PHY Abilities" command
	 * if there is no delay
	 */
	DELAY(500);
	if (ixl_get_phy_info(sc) != 0) {
		/* error printed by ixl_get_phy_info */
		goto free_hmc;
	}

	if (ixl_dmamem_alloc(sc, &sc->sc_scratch,
	    sizeof(struct ixl_aq_vsi_data), 8) != 0) {
		aprint_error_dev(self, "unable to allocate scratch buffer\n");
		goto free_hmc;
	}

	rv = ixl_get_vsi(sc);
	if (rv != 0) {
		aprint_error_dev(self, "GET VSI %s %d\n",
		    rv == ETIMEDOUT ? "timeout" : "error", rv);
		goto free_scratch;
	}

	rv = ixl_set_vsi(sc);
	if (rv != 0) {
		aprint_error_dev(self, "UPDATE VSI error %s %d\n",
		    rv == ETIMEDOUT ? "timeout" : "error", rv);
		goto free_scratch;
	}

	if (ixl_queue_pairs_alloc(sc) != 0) {
		/* error printed by ixl_queue_pairs_alloc */
		goto free_scratch;
	}

	if (ixl_setup_interrupts(sc) != 0) {
		/* error printed by ixl_setup_interrupts */
		goto free_queue_pairs;
	}

	if (ixl_setup_stats(sc) != 0) {
		aprint_error_dev(self, "failed to setup event counters\n");
		goto teardown_intrs;
	}

	if (ixl_setup_sysctls(sc) != 0) {
		/* error printed by ixl_setup_sysctls */
		goto teardown_stats;
	}

	snprintf(xnamebuf, sizeof(xnamebuf), "%s_wq_cfg", device_xname(self));
	sc->sc_workq = ixl_workq_create(xnamebuf, IXL_WORKQUEUE_PRI,
	    IPL_NET, WQ_MPSAFE);
	if (sc->sc_workq == NULL)
		goto teardown_sysctls;

	snprintf(xnamebuf, sizeof(xnamebuf), "%s_wq_txrx", device_xname(self));
	rv = workqueue_create(&sc->sc_workq_txrx, xnamebuf, ixl_handle_queue_wk,
	    sc, IXL_WORKQUEUE_PRI, IPL_NET, WQ_PERCPU | WQ_MPSAFE);
	if (rv != 0) {
		sc->sc_workq_txrx = NULL;
		goto teardown_wqs;
	}

	snprintf(xnamebuf, sizeof(xnamebuf), "%s_atq_cv", device_xname(self));
	cv_init(&sc->sc_atq_cv, xnamebuf);

	strlcpy(ifp->if_xname, device_xname(self), IFNAMSIZ);

	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_extflags = IFEF_MPSAFE;
	ifp->if_ioctl = ixl_ioctl;
	ifp->if_start = ixl_start;
	ifp->if_transmit = ixl_transmit;
	ifp->if_watchdog = ixl_watchdog;
	ifp->if_init = ixl_init;
	ifp->if_stop = ixl_stop;
	IFQ_SET_MAXLEN(&ifp->if_snd, sc->sc_tx_ring_ndescs);
	IFQ_SET_READY(&ifp->if_snd);
	ifp->if_capabilities |= IXL_IFCAP_RXCSUM;
	ifp->if_capabilities |= IXL_IFCAP_TXCSUM;
#if 0
	ifp->if_capabilities |= IFCAP_TSOv4 | IFCAP_TSOv6;
#endif
	ether_set_vlan_cb(&sc->sc_ec, ixl_vlan_cb);
	sc->sc_ec.ec_capabilities |= ETHERCAP_JUMBO_MTU;
	sc->sc_ec.ec_capabilities |= ETHERCAP_VLAN_HWTAGGING;
	sc->sc_ec.ec_capabilities |= ETHERCAP_VLAN_HWFILTER;

	sc->sc_ec.ec_capenable = sc->sc_ec.ec_capabilities;
	/* Disable VLAN_HWFILTER by default */
	CLR(sc->sc_ec.ec_capenable, ETHERCAP_VLAN_HWFILTER);

	sc->sc_cur_ec_capenable = sc->sc_ec.ec_capenable;

	sc->sc_ec.ec_ifmedia = &sc->sc_media;
	ifmedia_init_with_lock(&sc->sc_media, IFM_IMASK, ixl_media_change,
	    ixl_media_status, &sc->sc_cfg_lock);

	ixl_media_add(sc);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_AUTO, 0, NULL);
	if (ISSET(sc->sc_phy_abilities,
	    (IXL_PHY_ABILITY_PAUSE_TX | IXL_PHY_ABILITY_PAUSE_RX))) {
		ifmedia_add(&sc->sc_media,
		    IFM_ETHER | IFM_AUTO | IFM_FLOW, 0, NULL);
	}
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_NONE, 0, NULL);
	ifmedia_set(&sc->sc_media, IFM_ETHER | IFM_AUTO);

	if_initialize(ifp);

	sc->sc_ipq = if_percpuq_create(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, sc->sc_enaddr);
	ether_set_ifflags_cb(&sc->sc_ec, ixl_ifflags_cb);

	rv = ixl_get_link_status_poll(sc, &link);
	if (rv != 0)
		link = LINK_STATE_UNKNOWN;
	if_link_state_change(ifp, link);

	ixl_atq_set(&sc->sc_link_state_atq, ixl_get_link_status_done);
	ixl_work_set(&sc->sc_link_state_task, ixl_get_link_status, sc);

	ixl_config_other_intr(sc);
	ixl_enable_other_intr(sc);

	ixl_set_phy_autoselect(sc);

	/* remove default mac filter and replace it so we can see vlans */
	rv = ixl_remove_macvlan(sc, sc->sc_enaddr, 0, 0);
	if (rv != ENOENT) {
		aprint_debug_dev(self,
		    "unable to remove macvlan %u\n", rv);
	}
	rv = ixl_remove_macvlan(sc, sc->sc_enaddr, 0,
	    IXL_AQ_OP_REMOVE_MACVLAN_IGNORE_VLAN);
	if (rv != ENOENT) {
		aprint_debug_dev(self,
		    "unable to remove macvlan, ignore vlan %u\n", rv);
	}

	if (ixl_update_macvlan(sc) != 0) {
		aprint_debug_dev(self,
		    "couldn't enable vlan hardware filter\n");
		CLR(sc->sc_ec.ec_capenable, ETHERCAP_VLAN_HWFILTER);
		CLR(sc->sc_cur_ec_capenable, ETHERCAP_VLAN_HWFILTER);
	}

	sc->sc_txrx_workqueue = true;
	sc->sc_tx_process_limit = IXL_TX_PROCESS_LIMIT;
	sc->sc_rx_process_limit = IXL_RX_PROCESS_LIMIT;
	sc->sc_tx_intr_process_limit = IXL_TX_INTR_PROCESS_LIMIT;
	sc->sc_rx_intr_process_limit = IXL_RX_INTR_PROCESS_LIMIT;

	ixl_stats_update(sc);
	sc->sc_stats_counters.isc_has_offset = true;

	if (pmf_device_register(self, NULL, NULL) != true)
		aprint_debug_dev(self, "couldn't establish power handler\n");
	sc->sc_itr_rx = IXL_ITR_RX;
	sc->sc_itr_tx = IXL_ITR_TX;
	sc->sc_attached = true;
	if_register(ifp);

	return;

teardown_wqs:
	config_finalize_register(self, ixl_workqs_teardown);
teardown_sysctls:
	ixl_teardown_sysctls(sc);
teardown_stats:
	ixl_teardown_stats(sc);
teardown_intrs:
	ixl_teardown_interrupts(sc);
free_queue_pairs:
	ixl_queue_pairs_free(sc);
free_scratch:
	ixl_dmamem_free(sc, &sc->sc_scratch);
free_hmc:
	ixl_hmc_free(sc);
free_aqbuf:
	ixl_dmamem_free(sc, &sc->sc_aqbuf);
shutdown:
	ixl_wr(sc, sc->sc_aq_regs->atq_head, 0);
	ixl_wr(sc, sc->sc_aq_regs->arq_head, 0);
	ixl_wr(sc, sc->sc_aq_regs->atq_tail, 0);
	ixl_wr(sc, sc->sc_aq_regs->arq_tail, 0);

	ixl_wr(sc, sc->sc_aq_regs->atq_bal, 0);
	ixl_wr(sc, sc->sc_aq_regs->atq_bah, 0);
	ixl_wr(sc, sc->sc_aq_regs->atq_len, 0);

	ixl_wr(sc, sc->sc_aq_regs->arq_bal, 0);
	ixl_wr(sc, sc->sc_aq_regs->arq_bah, 0);
	ixl_wr(sc, sc->sc_aq_regs->arq_len, 0);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_arq),
	    0, IXL_DMA_LEN(&sc->sc_arq),
	    BUS_DMASYNC_POSTREAD);
	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq),
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

	ixl_arq_unfill(sc);
free_arq:
	ixl_dmamem_free(sc, &sc->sc_arq);
free_atq:
	ixl_dmamem_free(sc, &sc->sc_atq);
unmap:
	mutex_destroy(&sc->sc_atq_lock);
	bus_space_unmap(sc->sc_memt, sc->sc_memh, sc->sc_mems);
	mutex_destroy(&sc->sc_cfg_lock);
	sc->sc_mems = 0;

	sc->sc_attached = false;
}

static int
ixl_detach(device_t self, int flags)
{
	struct ixl_softc *sc = device_private(self);
	struct ifnet *ifp = &sc->sc_ec.ec_if;

	if (!sc->sc_attached)
		return 0;

	ixl_stop(ifp, 1);

	ixl_disable_other_intr(sc);
	ixl_intr_barrier();

	callout_halt(&sc->sc_stats_callout, NULL);
	ixl_work_wait(sc->sc_workq, &sc->sc_stats_task);

	ixl_work_wait(sc->sc_workq, &sc->sc_arq_task);
	ixl_work_wait(sc->sc_workq, &sc->sc_link_state_task);

	if (sc->sc_workq != NULL) {
		ixl_workq_destroy(sc->sc_workq);
		sc->sc_workq = NULL;
	}

	if (sc->sc_workq_txrx != NULL) {
		workqueue_destroy(sc->sc_workq_txrx);
		sc->sc_workq_txrx = NULL;
	}

	if_percpuq_destroy(sc->sc_ipq);
	ether_ifdetach(ifp);
	if_detach(ifp);
	ifmedia_fini(&sc->sc_media);

	ixl_teardown_interrupts(sc);
	ixl_teardown_stats(sc);
	ixl_teardown_sysctls(sc);

	ixl_queue_pairs_free(sc);

	ixl_dmamem_free(sc, &sc->sc_scratch);
	ixl_hmc_free(sc);

	/* shutdown */
	ixl_wr(sc, sc->sc_aq_regs->atq_head, 0);
	ixl_wr(sc, sc->sc_aq_regs->arq_head, 0);
	ixl_wr(sc, sc->sc_aq_regs->atq_tail, 0);
	ixl_wr(sc, sc->sc_aq_regs->arq_tail, 0);

	ixl_wr(sc, sc->sc_aq_regs->atq_bal, 0);
	ixl_wr(sc, sc->sc_aq_regs->atq_bah, 0);
	ixl_wr(sc, sc->sc_aq_regs->atq_len, 0);

	ixl_wr(sc, sc->sc_aq_regs->arq_bal, 0);
	ixl_wr(sc, sc->sc_aq_regs->arq_bah, 0);
	ixl_wr(sc, sc->sc_aq_regs->arq_len, 0);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_arq),
	    0, IXL_DMA_LEN(&sc->sc_arq),
	    BUS_DMASYNC_POSTREAD);
	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq),
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

	ixl_arq_unfill(sc);

	ixl_dmamem_free(sc, &sc->sc_arq);
	ixl_dmamem_free(sc, &sc->sc_atq);
	ixl_dmamem_free(sc, &sc->sc_aqbuf);

	cv_destroy(&sc->sc_atq_cv);
	mutex_destroy(&sc->sc_atq_lock);

	if (sc->sc_mems != 0) {
		bus_space_unmap(sc->sc_memt, sc->sc_memh, sc->sc_mems);
		sc->sc_mems = 0;
	}

	mutex_destroy(&sc->sc_cfg_lock);

	return 0;
}

static int
ixl_workqs_teardown(device_t self)
{
	struct ixl_softc *sc = device_private(self);

	if (sc->sc_workq != NULL) {
		ixl_workq_destroy(sc->sc_workq);
		sc->sc_workq = NULL;
	}

	if (sc->sc_workq_txrx != NULL) {
		workqueue_destroy(sc->sc_workq_txrx);
		sc->sc_workq_txrx = NULL;
	}

	return 0;
}

static int
ixl_vlan_cb(struct ethercom *ec, uint16_t vid, bool set)
{
	struct ifnet *ifp = &ec->ec_if;
	struct ixl_softc *sc = ifp->if_softc;
	int rv;

	if (!ISSET(sc->sc_cur_ec_capenable, ETHERCAP_VLAN_HWFILTER)) {
		return 0;
	}

	if (set) {
		rv = ixl_add_macvlan(sc, sc->sc_enaddr, vid,
		    IXL_AQ_OP_ADD_MACVLAN_PERFECT_MATCH);
		if (rv == 0) {
			rv = ixl_add_macvlan(sc, etherbroadcastaddr,
			    vid, IXL_AQ_OP_ADD_MACVLAN_PERFECT_MATCH);
		}
	} else {
		rv = ixl_remove_macvlan(sc, sc->sc_enaddr, vid,
		    IXL_AQ_OP_REMOVE_MACVLAN_PERFECT_MATCH);
		(void)ixl_remove_macvlan(sc, etherbroadcastaddr, vid,
		    IXL_AQ_OP_REMOVE_MACVLAN_PERFECT_MATCH);
	}

	return rv;
}

static void
ixl_media_add(struct ixl_softc *sc)
{
	struct ifmedia *ifm = &sc->sc_media;
	const struct ixl_phy_type *itype;
	unsigned int i;
	bool flow;

	if (ISSET(sc->sc_phy_abilities,
	    (IXL_PHY_ABILITY_PAUSE_TX | IXL_PHY_ABILITY_PAUSE_RX))) {
		flow = true;
	} else {
		flow = false;
	}

	for (i = 0; i < __arraycount(ixl_phy_type_map); i++) {
		itype = &ixl_phy_type_map[i];

		if (ISSET(sc->sc_phy_types, itype->phy_type)) {
			ifmedia_add(ifm,
			    IFM_ETHER | IFM_FDX | itype->ifm_type, 0, NULL);

			if (flow) {
				ifmedia_add(ifm,
				    IFM_ETHER | IFM_FDX | IFM_FLOW |
				    itype->ifm_type, 0, NULL);
			}

			if (itype->ifm_type != IFM_100_TX)
				continue;

			ifmedia_add(ifm, IFM_ETHER | itype->ifm_type,
			    0, NULL);
			if (flow) {
				ifmedia_add(ifm,
				    IFM_ETHER | IFM_FLOW | itype->ifm_type,
				    0, NULL);
			}
		}
	}
}

static void
ixl_media_status(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct ixl_softc *sc = ifp->if_softc;

	KASSERT(mutex_owned(&sc->sc_cfg_lock));

	ifmr->ifm_status = sc->sc_media_status;
	ifmr->ifm_active = sc->sc_media_active;
}

static int
ixl_media_change(struct ifnet *ifp)
{
	struct ixl_softc *sc = ifp->if_softc;
	struct ifmedia *ifm = &sc->sc_media;
	uint64_t ifm_active = sc->sc_media_active;
	uint8_t link_speed, abilities;

	switch (IFM_SUBTYPE(ifm_active)) {
	case IFM_1000_SGMII:
	case IFM_1000_KX:
	case IFM_10G_KX4:
	case IFM_10G_KR:
	case IFM_40G_KR4:
	case IFM_20G_KR2:
	case IFM_25G_KR:
		/* backplanes */
		return EINVAL;
	}

	abilities = IXL_PHY_ABILITY_AUTONEGO | IXL_PHY_ABILITY_LINKUP;

	switch (IFM_SUBTYPE(ifm->ifm_media)) {
	case IFM_AUTO:
		link_speed = sc->sc_phy_linkspeed;
		break;
	case IFM_NONE:
		link_speed = 0;
		CLR(abilities, IXL_PHY_ABILITY_LINKUP);
		break;
	default:
		link_speed = ixl_search_baudrate(
		    ifmedia_baudrate(ifm->ifm_media));
	}

	if (ISSET(abilities, IXL_PHY_ABILITY_LINKUP)) {
		if (ISSET(link_speed, sc->sc_phy_linkspeed) == 0)
			return EINVAL;
	}

	if (ifm->ifm_media & IFM_FLOW) {
		abilities |= sc->sc_phy_abilities &
		    (IXL_PHY_ABILITY_PAUSE_TX | IXL_PHY_ABILITY_PAUSE_RX);
	}

	return ixl_set_phy_config(sc, link_speed, abilities, false);
}

static void
ixl_watchdog(struct ifnet *ifp)
{

}

static void
ixl_del_all_multiaddr(struct ixl_softc *sc)
{
	struct ethercom *ec = &sc->sc_ec;
	struct ether_multi *enm;
	struct ether_multistep step;

	ETHER_LOCK(ec);
	for (ETHER_FIRST_MULTI(step, ec, enm); enm != NULL;
	    ETHER_NEXT_MULTI(step, enm)) {
		ixl_remove_macvlan(sc, enm->enm_addrlo, 0,
		    IXL_AQ_OP_REMOVE_MACVLAN_IGNORE_VLAN);
	}
	ETHER_UNLOCK(ec);
}

static int
ixl_add_multi(struct ixl_softc *sc, uint8_t *addrlo, uint8_t *addrhi)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	int rv;

	if (ISSET(ifp->if_flags, IFF_ALLMULTI))
		return 0;

	if (memcmp(addrlo, addrhi, ETHER_ADDR_LEN) != 0) {
		ixl_del_all_multiaddr(sc);
		SET(ifp->if_flags, IFF_ALLMULTI);
		return ENETRESET;
	}

	/* multicast address can not use VLAN HWFILTER */
	rv = ixl_add_macvlan(sc, addrlo, 0,
	    IXL_AQ_OP_ADD_MACVLAN_IGNORE_VLAN);

	if (rv == ENOSPC) {
		ixl_del_all_multiaddr(sc);
		SET(ifp->if_flags, IFF_ALLMULTI);
		return ENETRESET;
	}

	return rv;
}

static int
ixl_del_multi(struct ixl_softc *sc, uint8_t *addrlo, uint8_t *addrhi)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct ethercom *ec = &sc->sc_ec;
	struct ether_multi *enm, *enm_last;
	struct ether_multistep step;
	int error, rv = 0;

	if (!ISSET(ifp->if_flags, IFF_ALLMULTI)) {
		ixl_remove_macvlan(sc, addrlo, 0,
		    IXL_AQ_OP_REMOVE_MACVLAN_IGNORE_VLAN);
		return 0;
	}

	ETHER_LOCK(ec);
	for (ETHER_FIRST_MULTI(step, ec, enm); enm != NULL;
	    ETHER_NEXT_MULTI(step, enm)) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
		    ETHER_ADDR_LEN) != 0) {
			goto out;
		}
	}

	for (ETHER_FIRST_MULTI(step, ec, enm); enm != NULL;
	    ETHER_NEXT_MULTI(step, enm)) {
		error = ixl_add_macvlan(sc, enm->enm_addrlo, 0,
		    IXL_AQ_OP_ADD_MACVLAN_IGNORE_VLAN);
		if (error != 0)
			break;
	}

	if (enm != NULL) {
		enm_last = enm;
		for (ETHER_FIRST_MULTI(step, ec, enm); enm != NULL;
		    ETHER_NEXT_MULTI(step, enm)) {
			if (enm == enm_last)
				break;

			ixl_remove_macvlan(sc, enm->enm_addrlo, 0,
			    IXL_AQ_OP_REMOVE_MACVLAN_IGNORE_VLAN);
		}
	} else {
		CLR(ifp->if_flags, IFF_ALLMULTI);
		rv = ENETRESET;
	}

out:
	ETHER_UNLOCK(ec);
	return rv;
}

static int
ixl_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct ifreq *ifr = (struct ifreq *)data;
	struct ixl_softc *sc = (struct ixl_softc *)ifp->if_softc;
	const struct sockaddr *sa;
	uint8_t addrhi[ETHER_ADDR_LEN], addrlo[ETHER_ADDR_LEN];
	int s, error = 0;
	unsigned int nmtu;

	switch (cmd) {
	case SIOCSIFMTU:
		nmtu = ifr->ifr_mtu;

		if (nmtu < IXL_MIN_MTU || nmtu > IXL_MAX_MTU) {
			error = EINVAL;
			break;
		}
		if (ifp->if_mtu != nmtu) {
			s = splnet();
			error = ether_ioctl(ifp, cmd, data);
			splx(s);
			if (error == ENETRESET)
				error = ixl_init(ifp);
		}
		break;
	case SIOCADDMULTI:
		sa = ifreq_getaddr(SIOCADDMULTI, ifr);
		if (ether_addmulti(sa, &sc->sc_ec) == ENETRESET) {
			error = ether_multiaddr(sa, addrlo, addrhi);
			if (error != 0)
				return error;

			error = ixl_add_multi(sc, addrlo, addrhi);
			if (error != 0 && error != ENETRESET) {
				ether_delmulti(sa, &sc->sc_ec);
				error = EIO;
			}
		}
		break;

	case SIOCDELMULTI:
		sa = ifreq_getaddr(SIOCDELMULTI, ifr);
		if (ether_delmulti(sa, &sc->sc_ec) == ENETRESET) {
			error = ether_multiaddr(sa, addrlo, addrhi);
			if (error != 0)
				return error;

			error = ixl_del_multi(sc, addrlo, addrhi);
		}
		break;

	default:
		s = splnet();
		error = ether_ioctl(ifp, cmd, data);
		splx(s);
	}

	if (error == ENETRESET)
		error = ixl_iff(sc);

	return error;
}

static enum i40e_mac_type
ixl_mactype(pci_product_id_t id)
{

	switch (id) {
	case PCI_PRODUCT_INTEL_XL710_SFP:
	case PCI_PRODUCT_INTEL_XL710_KX_B:
	case PCI_PRODUCT_INTEL_XL710_KX_C:
	case PCI_PRODUCT_INTEL_XL710_QSFP_A:
	case PCI_PRODUCT_INTEL_XL710_QSFP_B:
	case PCI_PRODUCT_INTEL_XL710_QSFP_C:
	case PCI_PRODUCT_INTEL_X710_10G_T_1:
	case PCI_PRODUCT_INTEL_X710_10G_T_2:
	case PCI_PRODUCT_INTEL_XL710_20G_BP_1:
	case PCI_PRODUCT_INTEL_XL710_20G_BP_2:
	case PCI_PRODUCT_INTEL_X710_T4_10G:
	case PCI_PRODUCT_INTEL_XXV710_25G_BP:
	case PCI_PRODUCT_INTEL_XXV710_25G_SFP28:
	case PCI_PRODUCT_INTEL_X710_10G_SFP:
	case PCI_PRODUCT_INTEL_X710_10G_BP:
		return I40E_MAC_XL710;

	case PCI_PRODUCT_INTEL_X722_KX:
	case PCI_PRODUCT_INTEL_X722_QSFP:
	case PCI_PRODUCT_INTEL_X722_SFP:
	case PCI_PRODUCT_INTEL_X722_1G_BASET:
	case PCI_PRODUCT_INTEL_X722_10G_BASET:
	case PCI_PRODUCT_INTEL_X722_I_SFP:
		return I40E_MAC_X722;
	}

	return I40E_MAC_GENERIC;
}

static void
ixl_pci_csr_setup(pci_chipset_tag_t pc, pcitag_t tag)
{
	pcireg_t csr;

	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	csr |= (PCI_COMMAND_MASTER_ENABLE |
	    PCI_COMMAND_MEM_ENABLE);
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);
}

static inline void *
ixl_hmc_kva(struct ixl_softc *sc, enum ixl_hmc_types type, unsigned int i)
{
	uint8_t *kva = IXL_DMA_KVA(&sc->sc_hmc_pd);
	struct ixl_hmc_entry *e = &sc->sc_hmc_entries[type];

	if (i >= e->hmc_count)
		return NULL;

	kva += e->hmc_base;
	kva += i * e->hmc_size;

	return kva;
}

static inline size_t
ixl_hmc_len(struct ixl_softc *sc, enum ixl_hmc_types type)
{
	struct ixl_hmc_entry *e = &sc->sc_hmc_entries[type];

	return e->hmc_size;
}

static void
ixl_enable_queue_intr(struct ixl_softc *sc, struct ixl_queue_pair *qp)
{
	struct ixl_rx_ring *rxr = qp->qp_rxr;

	ixl_wr(sc, I40E_PFINT_DYN_CTLN(rxr->rxr_qid),
	    I40E_PFINT_DYN_CTLN_INTENA_MASK |
	    I40E_PFINT_DYN_CTLN_CLEARPBA_MASK |
	    (IXL_NOITR << I40E_PFINT_DYN_CTLN_ITR_INDX_SHIFT));
	ixl_flush(sc);
}

static void
ixl_disable_queue_intr(struct ixl_softc *sc, struct ixl_queue_pair *qp)
{
	struct ixl_rx_ring *rxr = qp->qp_rxr;

	ixl_wr(sc, I40E_PFINT_DYN_CTLN(rxr->rxr_qid),
	    (IXL_NOITR << I40E_PFINT_DYN_CTLN_ITR_INDX_SHIFT));
	ixl_flush(sc);
}

static void
ixl_enable_other_intr(struct ixl_softc *sc)
{

	ixl_wr(sc, I40E_PFINT_DYN_CTL0,
	    I40E_PFINT_DYN_CTL0_INTENA_MASK |
	    I40E_PFINT_DYN_CTL0_CLEARPBA_MASK |
	    (IXL_NOITR << I40E_PFINT_DYN_CTL0_ITR_INDX_SHIFT));
	ixl_flush(sc);
}

static void
ixl_disable_other_intr(struct ixl_softc *sc)
{

	ixl_wr(sc, I40E_PFINT_DYN_CTL0,
	    (IXL_NOITR << I40E_PFINT_DYN_CTL0_ITR_INDX_SHIFT));
	ixl_flush(sc);
}

static int
ixl_reinit(struct ixl_softc *sc)
{
	struct ixl_rx_ring *rxr;
	struct ixl_tx_ring *txr;
	unsigned int i;
	uint32_t reg;

	KASSERT(mutex_owned(&sc->sc_cfg_lock));

	if (ixl_get_vsi(sc) != 0)
		return EIO;

	if (ixl_set_vsi(sc) != 0)
		return EIO;

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		txr = sc->sc_qps[i].qp_txr;
		rxr = sc->sc_qps[i].qp_rxr;

		ixl_txr_config(sc, txr);
		ixl_rxr_config(sc, rxr);
	}

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_hmc_pd),
	    0, IXL_DMA_LEN(&sc->sc_hmc_pd), BUS_DMASYNC_PREWRITE);

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		txr = sc->sc_qps[i].qp_txr;
		rxr = sc->sc_qps[i].qp_rxr;

		ixl_wr(sc, I40E_QTX_CTL(i), I40E_QTX_CTL_PF_QUEUE |
		    (sc->sc_pf_id << I40E_QTX_CTL_PF_INDX_SHIFT));
		ixl_flush(sc);

		ixl_wr(sc, txr->txr_tail, txr->txr_prod);
		ixl_wr(sc, rxr->rxr_tail, rxr->rxr_prod);

		/* ixl_rxfill() needs lock held */
		mutex_enter(&rxr->rxr_lock);
		ixl_rxfill(sc, rxr);
		mutex_exit(&rxr->rxr_lock);

		reg = ixl_rd(sc, I40E_QRX_ENA(i));
		SET(reg, I40E_QRX_ENA_QENA_REQ_MASK);
		ixl_wr(sc, I40E_QRX_ENA(i), reg);
		if (ixl_rxr_enabled(sc, rxr) != 0)
			goto stop;

		ixl_txr_qdis(sc, txr, 1);

		reg = ixl_rd(sc, I40E_QTX_ENA(i));
		SET(reg, I40E_QTX_ENA_QENA_REQ_MASK);
		ixl_wr(sc, I40E_QTX_ENA(i), reg);

		if (ixl_txr_enabled(sc, txr) != 0)
			goto stop;
	}

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_hmc_pd),
	    0, IXL_DMA_LEN(&sc->sc_hmc_pd), BUS_DMASYNC_POSTWRITE);

	return 0;

stop:
	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_hmc_pd),
	    0, IXL_DMA_LEN(&sc->sc_hmc_pd), BUS_DMASYNC_POSTWRITE);

	return ETIMEDOUT;
}

static int
ixl_init_locked(struct ixl_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	unsigned int i;
	int error, eccap_change;

	KASSERT(mutex_owned(&sc->sc_cfg_lock));

	if (ISSET(ifp->if_flags, IFF_RUNNING))
		ixl_stop_locked(sc);

	if (sc->sc_dead) {
		return ENXIO;
	}

	eccap_change = sc->sc_ec.ec_capenable ^ sc->sc_cur_ec_capenable;
	if (ISSET(eccap_change, ETHERCAP_VLAN_HWTAGGING))
		sc->sc_cur_ec_capenable ^= ETHERCAP_VLAN_HWTAGGING;

	if (ISSET(eccap_change, ETHERCAP_VLAN_HWFILTER)) {
		if (ixl_update_macvlan(sc) == 0) {
			sc->sc_cur_ec_capenable ^= ETHERCAP_VLAN_HWFILTER;
		} else {
			CLR(sc->sc_ec.ec_capenable, ETHERCAP_VLAN_HWFILTER);
			CLR(sc->sc_cur_ec_capenable, ETHERCAP_VLAN_HWFILTER);
		}
	}

	if (sc->sc_intrtype != PCI_INTR_TYPE_MSIX)
		sc->sc_nqueue_pairs = 1;
	else
		sc->sc_nqueue_pairs = sc->sc_nqueue_pairs_max;

	error = ixl_reinit(sc);
	if (error) {
		ixl_stop_locked(sc);
		return error;
	}

	SET(ifp->if_flags, IFF_RUNNING);
	CLR(ifp->if_flags, IFF_OACTIVE);

	ixl_config_rss(sc);
	ixl_config_queue_intr(sc);

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		ixl_enable_queue_intr(sc, &sc->sc_qps[i]);
	}

	error = ixl_iff(sc);
	if (error) {
		ixl_stop_locked(sc);
		return error;
	}

	callout_schedule(&sc->sc_stats_callout, mstohz(sc->sc_stats_intval));

	return 0;
}

static int
ixl_init(struct ifnet *ifp)
{
	struct ixl_softc *sc = ifp->if_softc;
	int error;

	mutex_enter(&sc->sc_cfg_lock);
	error = ixl_init_locked(sc);
	mutex_exit(&sc->sc_cfg_lock);

	if (error == 0)
		(void)ixl_get_link_status(sc);

	return error;
}

static int
ixl_iff(struct ixl_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct ixl_atq iatq;
	struct ixl_aq_desc *iaq;
	struct ixl_aq_vsi_promisc_param *param;
	uint16_t flag_add, flag_del;
	int error;

	if (!ISSET(ifp->if_flags, IFF_RUNNING))
		return 0;

	memset(&iatq, 0, sizeof(iatq));

	iaq = &iatq.iatq_desc;
	iaq->iaq_opcode = htole16(IXL_AQ_OP_SET_VSI_PROMISC);

	param = (struct ixl_aq_vsi_promisc_param *)&iaq->iaq_param;
	param->flags = htole16(0);

	if (!ISSET(sc->sc_cur_ec_capenable, ETHERCAP_VLAN_HWFILTER)
	    || ISSET(ifp->if_flags, IFF_PROMISC)) {
		param->flags |= htole16(IXL_AQ_VSI_PROMISC_FLAG_BCAST |
		    IXL_AQ_VSI_PROMISC_FLAG_VLAN);
	}

	if (ISSET(ifp->if_flags, IFF_PROMISC)) {
		param->flags |= htole16(IXL_AQ_VSI_PROMISC_FLAG_UCAST |
		    IXL_AQ_VSI_PROMISC_FLAG_MCAST);
	} else if (ISSET(ifp->if_flags, IFF_ALLMULTI)) {
		param->flags |= htole16(IXL_AQ_VSI_PROMISC_FLAG_MCAST);
	}
	param->valid_flags = htole16(IXL_AQ_VSI_PROMISC_FLAG_UCAST |
	    IXL_AQ_VSI_PROMISC_FLAG_MCAST | IXL_AQ_VSI_PROMISC_FLAG_BCAST |
	    IXL_AQ_VSI_PROMISC_FLAG_VLAN);
	param->seid = sc->sc_seid;

	error = ixl_atq_exec(sc, &iatq);
	if (error)
		return error;

	if (iaq->iaq_retval != htole16(IXL_AQ_RC_OK))
		return EIO;

	if (memcmp(sc->sc_enaddr, CLLADDR(ifp->if_sadl), ETHER_ADDR_LEN) != 0) {
		if (ISSET(sc->sc_cur_ec_capenable, ETHERCAP_VLAN_HWFILTER)) {
			flag_add = IXL_AQ_OP_ADD_MACVLAN_PERFECT_MATCH;
			flag_del = IXL_AQ_OP_REMOVE_MACVLAN_PERFECT_MATCH;
		} else {
			flag_add = IXL_AQ_OP_ADD_MACVLAN_IGNORE_VLAN;
			flag_del = IXL_AQ_OP_REMOVE_MACVLAN_IGNORE_VLAN;
		}

		ixl_remove_macvlan(sc, sc->sc_enaddr, 0, flag_del);

		memcpy(sc->sc_enaddr, CLLADDR(ifp->if_sadl), ETHER_ADDR_LEN);
		ixl_add_macvlan(sc, sc->sc_enaddr, 0, flag_add);
	}
	return 0;
}

static void
ixl_stop_locked(struct ixl_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct ixl_rx_ring *rxr;
	struct ixl_tx_ring *txr;
	unsigned int i;
	uint32_t reg;

	KASSERT(mutex_owned(&sc->sc_cfg_lock));

	CLR(ifp->if_flags, IFF_RUNNING | IFF_OACTIVE);
	callout_stop(&sc->sc_stats_callout);

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		txr = sc->sc_qps[i].qp_txr;
		rxr = sc->sc_qps[i].qp_rxr;

		ixl_disable_queue_intr(sc, &sc->sc_qps[i]);

		mutex_enter(&txr->txr_lock);
		ixl_txr_qdis(sc, txr, 0);
		mutex_exit(&txr->txr_lock);
	}

	/* XXX wait at least 400 usec for all tx queues in one go */
	ixl_flush(sc);
	DELAY(500);

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		txr = sc->sc_qps[i].qp_txr;
		rxr = sc->sc_qps[i].qp_rxr;

		mutex_enter(&txr->txr_lock);
		reg = ixl_rd(sc, I40E_QTX_ENA(i));
		CLR(reg, I40E_QTX_ENA_QENA_REQ_MASK);
		ixl_wr(sc, I40E_QTX_ENA(i), reg);
		mutex_exit(&txr->txr_lock);

		mutex_enter(&rxr->rxr_lock);
		reg = ixl_rd(sc, I40E_QRX_ENA(i));
		CLR(reg, I40E_QRX_ENA_QENA_REQ_MASK);
		ixl_wr(sc, I40E_QRX_ENA(i), reg);
		mutex_exit(&rxr->rxr_lock);
	}

	/* XXX short wait for all queue disables to settle */
	ixl_flush(sc);
	DELAY(50);

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		txr = sc->sc_qps[i].qp_txr;
		rxr = sc->sc_qps[i].qp_rxr;

		mutex_enter(&txr->txr_lock);
		if (ixl_txr_disabled(sc, txr) != 0) {
			mutex_exit(&txr->txr_lock);
			goto die;
		}
		mutex_exit(&txr->txr_lock);

		mutex_enter(&rxr->rxr_lock);
		if (ixl_rxr_disabled(sc, rxr) != 0) {
			mutex_exit(&rxr->rxr_lock);
			goto die;
		}
		mutex_exit(&rxr->rxr_lock);
	}

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		sc->sc_qps[i].qp_workqueue = false;
		workqueue_wait(sc->sc_workq_txrx,
		    &sc->sc_qps[i].qp_work);
	}

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		txr = sc->sc_qps[i].qp_txr;
		rxr = sc->sc_qps[i].qp_rxr;

		mutex_enter(&txr->txr_lock);
		ixl_txr_unconfig(sc, txr);
		mutex_exit(&txr->txr_lock);

		mutex_enter(&rxr->rxr_lock);
		ixl_rxr_unconfig(sc, rxr);
		mutex_exit(&rxr->rxr_lock);

		ixl_txr_clean(sc, txr);
		ixl_rxr_clean(sc, rxr);
	}

	return;
die:
	sc->sc_dead = true;
	log(LOG_CRIT, "%s: failed to shut down rings",
	    device_xname(sc->sc_dev));
	return;
}

static void
ixl_stop(struct ifnet *ifp, int disable)
{
	struct ixl_softc *sc = ifp->if_softc;

	mutex_enter(&sc->sc_cfg_lock);
	ixl_stop_locked(sc);
	mutex_exit(&sc->sc_cfg_lock);
}

static int
ixl_queue_pairs_alloc(struct ixl_softc *sc)
{
	struct ixl_queue_pair *qp;
	unsigned int i;
	size_t sz;

	sz = sizeof(sc->sc_qps[0]) * sc->sc_nqueue_pairs_max;
	sc->sc_qps = kmem_zalloc(sz, KM_SLEEP);

	for (i = 0; i < sc->sc_nqueue_pairs_max; i++) {
		qp = &sc->sc_qps[i];

		qp->qp_si = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
		    ixl_handle_queue, qp);
		if (qp->qp_si == NULL)
			goto free;

		qp->qp_txr = ixl_txr_alloc(sc, i);
		if (qp->qp_txr == NULL)
			goto free;

		qp->qp_rxr = ixl_rxr_alloc(sc, i);
		if (qp->qp_rxr == NULL)
			goto free;

		qp->qp_sc = sc;
		snprintf(qp->qp_name, sizeof(qp->qp_name),
		    "%s-TXRX%d", device_xname(sc->sc_dev), i);
	}

	return 0;
free:
	if (sc->sc_qps != NULL) {
		for (i = 0; i < sc->sc_nqueue_pairs_max; i++) {
			qp = &sc->sc_qps[i];

			if (qp->qp_txr != NULL)
				ixl_txr_free(sc, qp->qp_txr);
			if (qp->qp_rxr != NULL)
				ixl_rxr_free(sc, qp->qp_rxr);
			if (qp->qp_si != NULL)
				softint_disestablish(qp->qp_si);
		}

		sz = sizeof(sc->sc_qps[0]) * sc->sc_nqueue_pairs_max;
		kmem_free(sc->sc_qps, sz);
		sc->sc_qps = NULL;
	}

	return -1;
}

static void
ixl_queue_pairs_free(struct ixl_softc *sc)
{
	struct ixl_queue_pair *qp;
	unsigned int i;
	size_t sz;

	for (i = 0; i < sc->sc_nqueue_pairs_max; i++) {
		qp = &sc->sc_qps[i];
		ixl_txr_free(sc, qp->qp_txr);
		ixl_rxr_free(sc, qp->qp_rxr);
		softint_disestablish(qp->qp_si);
	}

	sz = sizeof(sc->sc_qps[0]) * sc->sc_nqueue_pairs_max;
	kmem_free(sc->sc_qps, sz);
	sc->sc_qps = NULL;
}

static struct ixl_tx_ring *
ixl_txr_alloc(struct ixl_softc *sc, unsigned int qid)
{
	struct ixl_tx_ring *txr = NULL;
	struct ixl_tx_map *maps = NULL, *txm;
	unsigned int i;

	txr = kmem_zalloc(sizeof(*txr), KM_SLEEP);
	maps = kmem_zalloc(sizeof(maps[0]) * sc->sc_tx_ring_ndescs,
	    KM_SLEEP);

	if (ixl_dmamem_alloc(sc, &txr->txr_mem,
	    sizeof(struct ixl_tx_desc) * sc->sc_tx_ring_ndescs,
	    IXL_TX_QUEUE_ALIGN) != 0)
	    goto free;

	for (i = 0; i < sc->sc_tx_ring_ndescs; i++) {
		txm = &maps[i];

		if (bus_dmamap_create(sc->sc_dmat, IXL_TX_PKT_MAXSIZE,
		    IXL_TX_PKT_DESCS, IXL_TX_PKT_MAXSIZE, 0,
		    BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW, &txm->txm_map) != 0)
			goto uncreate;

		txm->txm_eop = -1;
		txm->txm_m = NULL;
	}

	txr->txr_cons = txr->txr_prod = 0;
	txr->txr_maps = maps;

	txr->txr_intrq = pcq_create(sc->sc_tx_ring_ndescs, KM_NOSLEEP);
	if (txr->txr_intrq == NULL)
		goto uncreate;

	txr->txr_si = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
	    ixl_deferred_transmit, txr);
	if (txr->txr_si == NULL)
		goto destroy_pcq;

	txr->txr_tail = I40E_QTX_TAIL(qid);
	txr->txr_qid = qid;
	txr->txr_sc = sc;
	mutex_init(&txr->txr_lock, MUTEX_DEFAULT, IPL_NET);

	return txr;

destroy_pcq:
	pcq_destroy(txr->txr_intrq);
uncreate:
	for (i = 0; i < sc->sc_tx_ring_ndescs; i++) {
		txm = &maps[i];

		if (txm->txm_map == NULL)
			continue;

		bus_dmamap_destroy(sc->sc_dmat, txm->txm_map);
	}

	ixl_dmamem_free(sc, &txr->txr_mem);
free:
	kmem_free(maps, sizeof(maps[0]) * sc->sc_tx_ring_ndescs);
	kmem_free(txr, sizeof(*txr));

	return NULL;
}

static void
ixl_txr_qdis(struct ixl_softc *sc, struct ixl_tx_ring *txr, int enable)
{
	unsigned int qid;
	bus_size_t reg;
	uint32_t r;

	qid = txr->txr_qid + sc->sc_base_queue;
	reg = I40E_GLLAN_TXPRE_QDIS(qid / 128);
	qid %= 128;

	r = ixl_rd(sc, reg);
	CLR(r, I40E_GLLAN_TXPRE_QDIS_QINDX_MASK);
	SET(r, qid << I40E_GLLAN_TXPRE_QDIS_QINDX_SHIFT);
	SET(r, enable ? I40E_GLLAN_TXPRE_QDIS_CLEAR_QDIS_MASK :
	    I40E_GLLAN_TXPRE_QDIS_SET_QDIS_MASK);
	ixl_wr(sc, reg, r);
}

static void
ixl_txr_config(struct ixl_softc *sc, struct ixl_tx_ring *txr)
{
	struct ixl_hmc_txq txq;
	struct ixl_aq_vsi_data *data = IXL_DMA_KVA(&sc->sc_scratch);
	void *hmc;

	memset(&txq, 0, sizeof(txq));
	txq.head = htole16(txr->txr_cons);
	txq.new_context = 1;
	txq.base = htole64(IXL_DMA_DVA(&txr->txr_mem) / IXL_HMC_TXQ_BASE_UNIT);
	txq.head_wb_ena = IXL_HMC_TXQ_DESC_WB;
	txq.qlen = htole16(sc->sc_tx_ring_ndescs);
	txq.tphrdesc_ena = 0;
	txq.tphrpacket_ena = 0;
	txq.tphwdesc_ena = 0;
	txq.rdylist = data->qs_handle[0];

	hmc = ixl_hmc_kva(sc, IXL_HMC_LAN_TX, txr->txr_qid);
	memset(hmc, 0, ixl_hmc_len(sc, IXL_HMC_LAN_TX));
	ixl_hmc_pack(hmc, &txq, ixl_hmc_pack_txq,
	    __arraycount(ixl_hmc_pack_txq));
}

static void
ixl_txr_unconfig(struct ixl_softc *sc, struct ixl_tx_ring *txr)
{
	void *hmc;

	hmc = ixl_hmc_kva(sc, IXL_HMC_LAN_TX, txr->txr_qid);
	memset(hmc, 0, ixl_hmc_len(sc, IXL_HMC_LAN_TX));
	txr->txr_cons = txr->txr_prod = 0;
}

static void
ixl_txr_clean(struct ixl_softc *sc, struct ixl_tx_ring *txr)
{
	struct ixl_tx_map *maps, *txm;
	bus_dmamap_t map;
	unsigned int i;

	maps = txr->txr_maps;
	for (i = 0; i < sc->sc_tx_ring_ndescs; i++) {
		txm = &maps[i];

		if (txm->txm_m == NULL)
			continue;

		map = txm->txm_map;
		bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, map);

		m_freem(txm->txm_m);
		txm->txm_m = NULL;
	}
}

static int
ixl_txr_enabled(struct ixl_softc *sc, struct ixl_tx_ring *txr)
{
	bus_size_t ena = I40E_QTX_ENA(txr->txr_qid);
	uint32_t reg;
	int i;

	for (i = 0; i < 10; i++) {
		reg = ixl_rd(sc, ena);
		if (ISSET(reg, I40E_QTX_ENA_QENA_STAT_MASK))
			return 0;

		delaymsec(10);
	}

	return ETIMEDOUT;
}

static int
ixl_txr_disabled(struct ixl_softc *sc, struct ixl_tx_ring *txr)
{
	bus_size_t ena = I40E_QTX_ENA(txr->txr_qid);
	uint32_t reg;
	int i;

	KASSERT(mutex_owned(&txr->txr_lock));

	for (i = 0; i < 10; i++) {
		reg = ixl_rd(sc, ena);
		if (ISSET(reg, I40E_QTX_ENA_QENA_STAT_MASK) == 0)
			return 0;

		delaymsec(10);
	}

	return ETIMEDOUT;
}

static void
ixl_txr_free(struct ixl_softc *sc, struct ixl_tx_ring *txr)
{
	struct ixl_tx_map *maps, *txm;
	struct mbuf *m;
	unsigned int i;

	softint_disestablish(txr->txr_si);
	while ((m = pcq_get(txr->txr_intrq)) != NULL)
		m_freem(m);
	pcq_destroy(txr->txr_intrq);

	maps = txr->txr_maps;
	for (i = 0; i < sc->sc_tx_ring_ndescs; i++) {
		txm = &maps[i];

		bus_dmamap_destroy(sc->sc_dmat, txm->txm_map);
	}

	ixl_dmamem_free(sc, &txr->txr_mem);
	mutex_destroy(&txr->txr_lock);
	kmem_free(maps, sizeof(maps[0]) * sc->sc_tx_ring_ndescs);
	kmem_free(txr, sizeof(*txr));
}

static inline int
ixl_load_mbuf(bus_dma_tag_t dmat, bus_dmamap_t map, struct mbuf **m0,
    struct ixl_tx_ring *txr)
{
	struct mbuf *m;
	int error;

	KASSERT(mutex_owned(&txr->txr_lock));

	m = *m0;

	error = bus_dmamap_load_mbuf(dmat, map, m,
	    BUS_DMA_STREAMING | BUS_DMA_WRITE | BUS_DMA_NOWAIT);
	if (error != EFBIG)
		return error;

	m = m_defrag(m, M_DONTWAIT);
	if (m != NULL) {
		*m0 = m;
		txr->txr_defragged.ev_count++;

		error = bus_dmamap_load_mbuf(dmat, map, m,
		    BUS_DMA_STREAMING | BUS_DMA_WRITE | BUS_DMA_NOWAIT);
	} else {
		txr->txr_defrag_failed.ev_count++;
		error = ENOBUFS;
	}

	return error;
}

static inline int
ixl_tx_setup_offloads(struct mbuf *m, uint64_t *cmd_txd)
{
	struct ether_header *eh;
	size_t len;
	uint64_t cmd;

	cmd = 0;

	eh = mtod(m, struct ether_header *);
	switch (htons(eh->ether_type)) {
	case ETHERTYPE_IP:
	case ETHERTYPE_IPV6:
		len = ETHER_HDR_LEN;
		break;
	case ETHERTYPE_VLAN:
		len = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
		break;
	default:
		len = 0;
	}
	cmd |= ((len >> 1) << IXL_TX_DESC_MACLEN_SHIFT);

	if (m->m_pkthdr.csum_flags &
	    (M_CSUM_TSOv4 | M_CSUM_TCPv4 | M_CSUM_UDPv4)) {
		cmd |= IXL_TX_DESC_CMD_IIPT_IPV4;
	}
	if (m->m_pkthdr.csum_flags & M_CSUM_IPv4) {
		cmd |= IXL_TX_DESC_CMD_IIPT_IPV4_CSUM;
	}

	if (m->m_pkthdr.csum_flags &
	    (M_CSUM_TSOv6 | M_CSUM_TCPv6 | M_CSUM_UDPv6)) {
		cmd |= IXL_TX_DESC_CMD_IIPT_IPV6;
	}

	switch (cmd & IXL_TX_DESC_CMD_IIPT_MASK) {
	case IXL_TX_DESC_CMD_IIPT_IPV4:
	case IXL_TX_DESC_CMD_IIPT_IPV4_CSUM:
		len = M_CSUM_DATA_IPv4_IPHL(m->m_pkthdr.csum_data);
		break;
	case IXL_TX_DESC_CMD_IIPT_IPV6:
		len = M_CSUM_DATA_IPv6_IPHL(m->m_pkthdr.csum_data);
		break;
	default:
		len = 0;
	}
	cmd |= ((len >> 2) << IXL_TX_DESC_IPLEN_SHIFT);

	if (m->m_pkthdr.csum_flags &
	    (M_CSUM_TSOv4 | M_CSUM_TSOv6 | M_CSUM_TCPv4 | M_CSUM_TCPv6)) {
		len = sizeof(struct tcphdr);
		cmd |= IXL_TX_DESC_CMD_L4T_EOFT_TCP;
	} else if (m->m_pkthdr.csum_flags & (M_CSUM_UDPv4 | M_CSUM_UDPv6)) {
		len = sizeof(struct udphdr);
		cmd |= IXL_TX_DESC_CMD_L4T_EOFT_UDP;
	} else {
		len = 0;
	}
	cmd |= ((len >> 2) << IXL_TX_DESC_L4LEN_SHIFT);

	*cmd_txd |= cmd;
	return 0;
}

static void
ixl_tx_common_locked(struct ifnet *ifp, struct ixl_tx_ring *txr,
    bool is_transmit)
{
	struct ixl_softc *sc = ifp->if_softc;
	struct ixl_tx_desc *ring, *txd;
	struct ixl_tx_map *txm;
	bus_dmamap_t map;
	struct mbuf *m;
	uint64_t cmd, cmd_txd;
	unsigned int prod, free, last, i;
	unsigned int mask;
	int post = 0;

	KASSERT(mutex_owned(&txr->txr_lock));

	if (!ISSET(ifp->if_flags, IFF_RUNNING)
	    || (!is_transmit && ISSET(ifp->if_flags, IFF_OACTIVE))) {
		if (!is_transmit)
			IFQ_PURGE(&ifp->if_snd);
		return;
	}

	prod = txr->txr_prod;
	free = txr->txr_cons;
	if (free <= prod)
		free += sc->sc_tx_ring_ndescs;
	free -= prod;

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&txr->txr_mem),
	    0, IXL_DMA_LEN(&txr->txr_mem), BUS_DMASYNC_POSTWRITE);

	ring = IXL_DMA_KVA(&txr->txr_mem);
	mask = sc->sc_tx_ring_ndescs - 1;
	last = prod;
	cmd = 0;
	txd = NULL;

	for (;;) {
		if (free <= IXL_TX_PKT_DESCS) {
			if (!is_transmit)
				SET(ifp->if_flags, IFF_OACTIVE);
			break;
		}

		if (is_transmit)
			m = pcq_get(txr->txr_intrq);
		else
			IFQ_DEQUEUE(&ifp->if_snd, m);

		if (m == NULL)
			break;

		txm = &txr->txr_maps[prod];
		map = txm->txm_map;

		if (ixl_load_mbuf(sc->sc_dmat, map, &m, txr) != 0) {
			if_statinc(ifp, if_oerrors);
			m_freem(m);
			continue;
		}

		cmd_txd = 0;
		if (m->m_pkthdr.csum_flags & IXL_CSUM_ALL_OFFLOAD) {
			ixl_tx_setup_offloads(m, &cmd_txd);
		}

		if (vlan_has_tag(m)) {
			cmd_txd |= (uint64_t)vlan_get_tag(m) <<
			    IXL_TX_DESC_L2TAG1_SHIFT;
			cmd_txd |= IXL_TX_DESC_CMD_IL2TAG1;
		}

		bus_dmamap_sync(sc->sc_dmat, map, 0,
		    map->dm_mapsize, BUS_DMASYNC_PREWRITE);

		for (i = 0; i < (unsigned int)map->dm_nsegs; i++) {
			txd = &ring[prod];

			cmd = (uint64_t)map->dm_segs[i].ds_len <<
			    IXL_TX_DESC_BSIZE_SHIFT;
			cmd |= IXL_TX_DESC_DTYPE_DATA | IXL_TX_DESC_CMD_ICRC;
			cmd |= cmd_txd;

			txd->addr = htole64(map->dm_segs[i].ds_addr);
			txd->cmd = htole64(cmd);

			last = prod;

			prod++;
			prod &= mask;
		}
		cmd |= IXL_TX_DESC_CMD_EOP | IXL_TX_DESC_CMD_RS;
		txd->cmd = htole64(cmd);

		txm->txm_m = m;
		txm->txm_eop = last;

		bpf_mtap(ifp, m, BPF_D_OUT);

		free -= i;
		post = 1;
	}

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&txr->txr_mem),
	    0, IXL_DMA_LEN(&txr->txr_mem), BUS_DMASYNC_PREWRITE);

	if (post) {
		txr->txr_prod = prod;
		ixl_wr(sc, txr->txr_tail, prod);
	}
}

static int
ixl_txeof(struct ixl_softc *sc, struct ixl_tx_ring *txr, u_int txlimit)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct ixl_tx_desc *ring, *txd;
	struct ixl_tx_map *txm;
	struct mbuf *m;
	bus_dmamap_t map;
	unsigned int cons, prod, last;
	unsigned int mask;
	uint64_t dtype;
	int done = 0, more = 0;

	KASSERT(mutex_owned(&txr->txr_lock));

	prod = txr->txr_prod;
	cons = txr->txr_cons;

	if (cons == prod)
		return 0;

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&txr->txr_mem),
	    0, IXL_DMA_LEN(&txr->txr_mem), BUS_DMASYNC_POSTREAD);

	ring = IXL_DMA_KVA(&txr->txr_mem);
	mask = sc->sc_tx_ring_ndescs - 1;

	net_stat_ref_t nsr = IF_STAT_GETREF(ifp);

	do {
		if (txlimit-- <= 0) {
			more = 1;
			break;
		}

		txm = &txr->txr_maps[cons];
		last = txm->txm_eop;
		txd = &ring[last];

		dtype = txd->cmd & htole64(IXL_TX_DESC_DTYPE_MASK);
		if (dtype != htole64(IXL_TX_DESC_DTYPE_DONE))
			break;

		map = txm->txm_map;

		bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, map);

		m = txm->txm_m;
		if (m != NULL) {
			if_statinc_ref(nsr, if_opackets);
			if_statadd_ref(nsr, if_obytes, m->m_pkthdr.len);
			if (ISSET(m->m_flags, M_MCAST))
				if_statinc_ref(nsr, if_omcasts);
			m_freem(m);
		}

		txm->txm_m = NULL;
		txm->txm_eop = -1;

		cons = last + 1;
		cons &= mask;
		done = 1;
	} while (cons != prod);

	IF_STAT_PUTREF(ifp);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&txr->txr_mem),
	    0, IXL_DMA_LEN(&txr->txr_mem), BUS_DMASYNC_PREREAD);

	txr->txr_cons = cons;

	if (done) {
		softint_schedule(txr->txr_si);
		if (txr->txr_qid == 0) {
			CLR(ifp->if_flags, IFF_OACTIVE);
			if_schedule_deferred_start(ifp);
		}
	}

	return more;
}

static void
ixl_start(struct ifnet *ifp)
{
	struct ixl_softc	*sc;
	struct ixl_tx_ring	*txr;

	sc = ifp->if_softc;
	txr = sc->sc_qps[0].qp_txr;

	mutex_enter(&txr->txr_lock);
	ixl_tx_common_locked(ifp, txr, false);
	mutex_exit(&txr->txr_lock);
}

static inline unsigned int
ixl_select_txqueue(struct ixl_softc *sc, struct mbuf *m)
{
	u_int cpuid;

	cpuid = cpu_index(curcpu());

	return (unsigned int)(cpuid % sc->sc_nqueue_pairs);
}

static int
ixl_transmit(struct ifnet *ifp, struct mbuf *m)
{
	struct ixl_softc *sc;
	struct ixl_tx_ring *txr;
	unsigned int qid;

	sc = ifp->if_softc;
	qid = ixl_select_txqueue(sc, m);

	txr = sc->sc_qps[qid].qp_txr;

	if (__predict_false(!pcq_put(txr->txr_intrq, m))) {
		mutex_enter(&txr->txr_lock);
		txr->txr_pcqdrop.ev_count++;
		mutex_exit(&txr->txr_lock);

		m_freem(m);
		return ENOBUFS;
	}

	if (mutex_tryenter(&txr->txr_lock)) {
		ixl_tx_common_locked(ifp, txr, true);
		mutex_exit(&txr->txr_lock);
	} else {
		kpreempt_disable();
		softint_schedule(txr->txr_si);
		kpreempt_enable();
	}

	return 0;
}

static void
ixl_deferred_transmit(void *xtxr)
{
	struct ixl_tx_ring *txr = xtxr;
	struct ixl_softc *sc = txr->txr_sc;
	struct ifnet *ifp = &sc->sc_ec.ec_if;

	mutex_enter(&txr->txr_lock);
	txr->txr_transmitdef.ev_count++;
	if (pcq_peek(txr->txr_intrq) != NULL)
		ixl_tx_common_locked(ifp, txr, true);
	mutex_exit(&txr->txr_lock);
}

static struct ixl_rx_ring *
ixl_rxr_alloc(struct ixl_softc *sc, unsigned int qid)
{
	struct ixl_rx_ring *rxr = NULL;
	struct ixl_rx_map *maps = NULL, *rxm;
	unsigned int i;

	rxr = kmem_zalloc(sizeof(*rxr), KM_SLEEP);
	maps = kmem_zalloc(sizeof(maps[0]) * sc->sc_rx_ring_ndescs,
	    KM_SLEEP);

	if (ixl_dmamem_alloc(sc, &rxr->rxr_mem,
	    sizeof(struct ixl_rx_rd_desc_32) * sc->sc_rx_ring_ndescs,
	    IXL_RX_QUEUE_ALIGN) != 0)
		goto free;

	for (i = 0; i < sc->sc_rx_ring_ndescs; i++) {
		rxm = &maps[i];

		if (bus_dmamap_create(sc->sc_dmat,
		    IXL_MCLBYTES, 1, IXL_MCLBYTES, 0,
		    BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW, &rxm->rxm_map) != 0)
			goto uncreate;

		rxm->rxm_m = NULL;
	}

	rxr->rxr_cons = rxr->rxr_prod = 0;
	rxr->rxr_m_head = NULL;
	rxr->rxr_m_tail = &rxr->rxr_m_head;
	rxr->rxr_maps = maps;

	rxr->rxr_tail = I40E_QRX_TAIL(qid);
	rxr->rxr_qid = qid;
	mutex_init(&rxr->rxr_lock, MUTEX_DEFAULT, IPL_NET);

	return rxr;

uncreate:
	for (i = 0; i < sc->sc_rx_ring_ndescs; i++) {
		rxm = &maps[i];

		if (rxm->rxm_map == NULL)
			continue;

		bus_dmamap_destroy(sc->sc_dmat, rxm->rxm_map);
	}

	ixl_dmamem_free(sc, &rxr->rxr_mem);
free:
	kmem_free(maps, sizeof(maps[0]) * sc->sc_rx_ring_ndescs);
	kmem_free(rxr, sizeof(*rxr));

	return NULL;
}

static void
ixl_rxr_clean(struct ixl_softc *sc, struct ixl_rx_ring *rxr)
{
	struct ixl_rx_map *maps, *rxm;
	bus_dmamap_t map;
	unsigned int i;

	maps = rxr->rxr_maps;
	for (i = 0; i < sc->sc_rx_ring_ndescs; i++) {
		rxm = &maps[i];

		if (rxm->rxm_m == NULL)
			continue;

		map = rxm->rxm_map;
		bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, map);

		m_freem(rxm->rxm_m);
		rxm->rxm_m = NULL;
	}

	m_freem(rxr->rxr_m_head);
	rxr->rxr_m_head = NULL;
	rxr->rxr_m_tail = &rxr->rxr_m_head;

	rxr->rxr_prod = rxr->rxr_cons = 0;
}

static int
ixl_rxr_enabled(struct ixl_softc *sc, struct ixl_rx_ring *rxr)
{
	bus_size_t ena = I40E_QRX_ENA(rxr->rxr_qid);
	uint32_t reg;
	int i;

	for (i = 0; i < 10; i++) {
		reg = ixl_rd(sc, ena);
		if (ISSET(reg, I40E_QRX_ENA_QENA_STAT_MASK))
			return 0;

		delaymsec(10);
	}

	return ETIMEDOUT;
}

static int
ixl_rxr_disabled(struct ixl_softc *sc, struct ixl_rx_ring *rxr)
{
	bus_size_t ena = I40E_QRX_ENA(rxr->rxr_qid);
	uint32_t reg;
	int i;

	KASSERT(mutex_owned(&rxr->rxr_lock));

	for (i = 0; i < 10; i++) {
		reg = ixl_rd(sc, ena);
		if (ISSET(reg, I40E_QRX_ENA_QENA_STAT_MASK) == 0)
			return 0;

		delaymsec(10);
	}

	return ETIMEDOUT;
}

static void
ixl_rxr_config(struct ixl_softc *sc, struct ixl_rx_ring *rxr)
{
	struct ixl_hmc_rxq rxq;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	uint16_t rxmax;
	void *hmc;

	memset(&rxq, 0, sizeof(rxq));
	rxmax = ifp->if_mtu + IXL_MTU_ETHERLEN;

	rxq.head = htole16(rxr->rxr_cons);
	rxq.base = htole64(IXL_DMA_DVA(&rxr->rxr_mem) / IXL_HMC_RXQ_BASE_UNIT);
	rxq.qlen = htole16(sc->sc_rx_ring_ndescs);
	rxq.dbuff = htole16(IXL_MCLBYTES / IXL_HMC_RXQ_DBUFF_UNIT);
	rxq.hbuff = 0;
	rxq.dtype = IXL_HMC_RXQ_DTYPE_NOSPLIT;
	rxq.dsize = IXL_HMC_RXQ_DSIZE_32;
	rxq.crcstrip = 1;
	rxq.l2sel = 1;
	rxq.showiv = 1;
	rxq.rxmax = htole16(rxmax);
	rxq.tphrdesc_ena = 0;
	rxq.tphwdesc_ena = 0;
	rxq.tphdata_ena = 0;
	rxq.tphhead_ena = 0;
	rxq.lrxqthresh = 0;
	rxq.prefena = 1;

	hmc = ixl_hmc_kva(sc, IXL_HMC_LAN_RX, rxr->rxr_qid);
	memset(hmc, 0, ixl_hmc_len(sc, IXL_HMC_LAN_RX));
	ixl_hmc_pack(hmc, &rxq, ixl_hmc_pack_rxq,
	    __arraycount(ixl_hmc_pack_rxq));
}

static void
ixl_rxr_unconfig(struct ixl_softc *sc, struct ixl_rx_ring *rxr)
{
	void *hmc;

	hmc = ixl_hmc_kva(sc, IXL_HMC_LAN_RX, rxr->rxr_qid);
	memset(hmc, 0, ixl_hmc_len(sc, IXL_HMC_LAN_RX));
	rxr->rxr_cons = rxr->rxr_prod = 0;
}

static void
ixl_rxr_free(struct ixl_softc *sc, struct ixl_rx_ring *rxr)
{
	struct ixl_rx_map *maps, *rxm;
	unsigned int i;

	maps = rxr->rxr_maps;
	for (i = 0; i < sc->sc_rx_ring_ndescs; i++) {
		rxm = &maps[i];

		bus_dmamap_destroy(sc->sc_dmat, rxm->rxm_map);
	}

	ixl_dmamem_free(sc, &rxr->rxr_mem);
	mutex_destroy(&rxr->rxr_lock);
	kmem_free(maps, sizeof(maps[0]) * sc->sc_rx_ring_ndescs);
	kmem_free(rxr, sizeof(*rxr));
}

static inline void
ixl_rx_csum(struct mbuf *m, uint64_t qword)
{
	int flags_mask;

	if (!ISSET(qword, IXL_RX_DESC_L3L4P)) {
		/* No L3 or L4 checksum was calculated */
		return;
	}

	switch (__SHIFTOUT(qword, IXL_RX_DESC_PTYPE_MASK)) {
	case IXL_RX_DESC_PTYPE_IPV4FRAG:
	case IXL_RX_DESC_PTYPE_IPV4:
	case IXL_RX_DESC_PTYPE_SCTPV4:
	case IXL_RX_DESC_PTYPE_ICMPV4:
		flags_mask = M_CSUM_IPv4 | M_CSUM_IPv4_BAD;
		break;
	case IXL_RX_DESC_PTYPE_TCPV4:
		flags_mask = M_CSUM_IPv4 | M_CSUM_IPv4_BAD;
		flags_mask |= M_CSUM_TCPv4 | M_CSUM_TCP_UDP_BAD;
		break;
	case IXL_RX_DESC_PTYPE_UDPV4:
		flags_mask = M_CSUM_IPv4 | M_CSUM_IPv4_BAD;
		flags_mask |= M_CSUM_UDPv4 | M_CSUM_TCP_UDP_BAD;
		break;
	case IXL_RX_DESC_PTYPE_TCPV6:
		flags_mask = M_CSUM_TCPv6 | M_CSUM_TCP_UDP_BAD;
		break;
	case IXL_RX_DESC_PTYPE_UDPV6:
		flags_mask = M_CSUM_UDPv6 | M_CSUM_TCP_UDP_BAD;
		break;
	default:
		flags_mask = 0;
	}

	m->m_pkthdr.csum_flags |= (flags_mask & (M_CSUM_IPv4 |
	    M_CSUM_TCPv4 | M_CSUM_TCPv6 | M_CSUM_UDPv4 | M_CSUM_UDPv6));

	if (ISSET(qword, IXL_RX_DESC_IPE)) {
		m->m_pkthdr.csum_flags |= (flags_mask & M_CSUM_IPv4_BAD);
	}

	if (ISSET(qword, IXL_RX_DESC_L4E)) {
		m->m_pkthdr.csum_flags |= (flags_mask & M_CSUM_TCP_UDP_BAD);
	}
}

static int
ixl_rxeof(struct ixl_softc *sc, struct ixl_rx_ring *rxr, u_int rxlimit)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct ixl_rx_wb_desc_32 *ring, *rxd;
	struct ixl_rx_map *rxm;
	bus_dmamap_t map;
	unsigned int cons, prod;
	struct mbuf *m;
	uint64_t word, word0;
	unsigned int len;
	unsigned int mask;
	int done = 0, more = 0;

	KASSERT(mutex_owned(&rxr->rxr_lock));

	if (!ISSET(ifp->if_flags, IFF_RUNNING))
		return 0;

	prod = rxr->rxr_prod;
	cons = rxr->rxr_cons;

	if (cons == prod)
		return 0;

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&rxr->rxr_mem),
	    0, IXL_DMA_LEN(&rxr->rxr_mem),
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

	ring = IXL_DMA_KVA(&rxr->rxr_mem);
	mask = sc->sc_rx_ring_ndescs - 1;

	net_stat_ref_t nsr = IF_STAT_GETREF(ifp);

	do {
		if (rxlimit-- <= 0) {
			more = 1;
			break;
		}

		rxd = &ring[cons];

		word = le64toh(rxd->qword1);

		if (!ISSET(word, IXL_RX_DESC_DD))
			break;

		rxm = &rxr->rxr_maps[cons];

		map = rxm->rxm_map;
		bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
		    BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmat, map);

		m = rxm->rxm_m;
		rxm->rxm_m = NULL;

		KASSERT(m != NULL);

		len = (word & IXL_RX_DESC_PLEN_MASK) >> IXL_RX_DESC_PLEN_SHIFT;
		m->m_len = len;
		m->m_pkthdr.len = 0;

		m->m_next = NULL;
		*rxr->rxr_m_tail = m;
		rxr->rxr_m_tail = &m->m_next;

		m = rxr->rxr_m_head;
		m->m_pkthdr.len += len;

		if (ISSET(word, IXL_RX_DESC_EOP)) {
			word0 = le64toh(rxd->qword0);

			if (ISSET(word, IXL_RX_DESC_L2TAG1P)) {
				vlan_set_tag(m,
				    __SHIFTOUT(word0, IXL_RX_DESC_L2TAG1_MASK));
			}

			if ((ifp->if_capenable & IXL_IFCAP_RXCSUM) != 0)
				ixl_rx_csum(m, word);

			if (!ISSET(word,
			    IXL_RX_DESC_RXE | IXL_RX_DESC_OVERSIZE)) {
				m_set_rcvif(m, ifp);
				if_statinc_ref(nsr, if_ipackets);
				if_statadd_ref(nsr, if_ibytes,
				    m->m_pkthdr.len);
				if_percpuq_enqueue(sc->sc_ipq, m);
			} else {
				if_statinc_ref(nsr, if_ierrors);
				m_freem(m);
			}

			rxr->rxr_m_head = NULL;
			rxr->rxr_m_tail = &rxr->rxr_m_head;
		}

		cons++;
		cons &= mask;

		done = 1;
	} while (cons != prod);

	if (done) {
		rxr->rxr_cons = cons;
		if (ixl_rxfill(sc, rxr) == -1)
			if_statinc_ref(nsr, if_iqdrops);
	}

	IF_STAT_PUTREF(ifp);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&rxr->rxr_mem),
	    0, IXL_DMA_LEN(&rxr->rxr_mem),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	return more;
}

static int
ixl_rxfill(struct ixl_softc *sc, struct ixl_rx_ring *rxr)
{
	struct ixl_rx_rd_desc_32 *ring, *rxd;
	struct ixl_rx_map *rxm;
	bus_dmamap_t map;
	struct mbuf *m;
	unsigned int prod;
	unsigned int slots;
	unsigned int mask;
	int post = 0, error = 0;

	KASSERT(mutex_owned(&rxr->rxr_lock));

	prod = rxr->rxr_prod;
	slots = ixl_rxr_unrefreshed(rxr->rxr_prod, rxr->rxr_cons,
	    sc->sc_rx_ring_ndescs);

	ring = IXL_DMA_KVA(&rxr->rxr_mem);
	mask = sc->sc_rx_ring_ndescs - 1;

	if (__predict_false(slots <= 0))
		return -1;

	do {
		rxm = &rxr->rxr_maps[prod];

		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
			rxr->rxr_mgethdr_failed.ev_count++;
			error = -1;
			break;
		}

		MCLGET(m, M_DONTWAIT);
		if (!ISSET(m->m_flags, M_EXT)) {
			rxr->rxr_mgetcl_failed.ev_count++;
			error = -1;
			m_freem(m);
			break;
		}

		m->m_len = m->m_pkthdr.len = MCLBYTES;
		m_adj(m, ETHER_ALIGN);

		map = rxm->rxm_map;

		if (bus_dmamap_load_mbuf(sc->sc_dmat, map, m,
		    BUS_DMA_READ | BUS_DMA_NOWAIT) != 0) {
			rxr->rxr_mbuf_load_failed.ev_count++;
			error = -1;
			m_freem(m);
			break;
		}

		rxm->rxm_m = m;

		bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
		    BUS_DMASYNC_PREREAD);

		rxd = &ring[prod];

		rxd->paddr = htole64(map->dm_segs[0].ds_addr);
		rxd->haddr = htole64(0);

		prod++;
		prod &= mask;

		post = 1;

	} while (--slots);

	if (post) {
		rxr->rxr_prod = prod;
		ixl_wr(sc, rxr->rxr_tail, prod);
	}

	return error;
}

static inline int
ixl_handle_queue_common(struct ixl_softc *sc, struct ixl_queue_pair *qp,
    u_int txlimit, struct evcnt *txevcnt,
    u_int rxlimit, struct evcnt *rxevcnt)
{
	struct ixl_tx_ring *txr = qp->qp_txr;
	struct ixl_rx_ring *rxr = qp->qp_rxr;
	int txmore, rxmore;
	int rv;

	mutex_enter(&txr->txr_lock);
	txevcnt->ev_count++;
	txmore = ixl_txeof(sc, txr, txlimit);
	mutex_exit(&txr->txr_lock);

	mutex_enter(&rxr->rxr_lock);
	rxevcnt->ev_count++;
	rxmore = ixl_rxeof(sc, rxr, rxlimit);
	mutex_exit(&rxr->rxr_lock);

	rv = txmore | (rxmore << 1);

	return rv;
}

static void
ixl_sched_handle_queue(struct ixl_softc *sc, struct ixl_queue_pair *qp)
{

	if (qp->qp_workqueue)
		workqueue_enqueue(sc->sc_workq_txrx, &qp->qp_work, NULL);
	else
		softint_schedule(qp->qp_si);
}

static int
ixl_intr(void *xsc)
{
	struct ixl_softc *sc = xsc;
	struct ixl_tx_ring *txr;
	struct ixl_rx_ring *rxr;
	uint32_t icr, rxintr, txintr;
	int rv = 0;
	unsigned int i;

	KASSERT(sc != NULL);

	ixl_enable_other_intr(sc);
	icr = ixl_rd(sc, I40E_PFINT_ICR0);

	if (ISSET(icr, I40E_PFINT_ICR0_ADMINQ_MASK)) {
		atomic_inc_64(&sc->sc_event_atq.ev_count);
		ixl_atq_done(sc);
		ixl_work_add(sc->sc_workq, &sc->sc_arq_task);
		rv = 1;
	}

	if (ISSET(icr, I40E_PFINT_ICR0_LINK_STAT_CHANGE_MASK)) {
		atomic_inc_64(&sc->sc_event_link.ev_count);
		ixl_work_add(sc->sc_workq, &sc->sc_link_state_task);
		rv = 1;
	}

	rxintr = icr & I40E_INTR_NOTX_RX_MASK;
	txintr = icr & I40E_INTR_NOTX_TX_MASK;

	if (txintr || rxintr) {
		for (i = 0; i < sc->sc_nqueue_pairs; i++) {
			txr = sc->sc_qps[i].qp_txr;
			rxr = sc->sc_qps[i].qp_rxr;

			ixl_handle_queue_common(sc, &sc->sc_qps[i],
			    IXL_TXRX_PROCESS_UNLIMIT, &txr->txr_intr,
			    IXL_TXRX_PROCESS_UNLIMIT, &rxr->rxr_intr);
		}
		rv = 1;
	}

	return rv;
}

static int
ixl_queue_intr(void *xqp)
{
	struct ixl_queue_pair *qp = xqp;
	struct ixl_tx_ring *txr = qp->qp_txr;
	struct ixl_rx_ring *rxr = qp->qp_rxr;
	struct ixl_softc *sc = qp->qp_sc;
	u_int txlimit, rxlimit;
	int more;

	txlimit = sc->sc_tx_intr_process_limit;
	rxlimit = sc->sc_rx_intr_process_limit;
	qp->qp_workqueue = sc->sc_txrx_workqueue;

	more = ixl_handle_queue_common(sc, qp,
	    txlimit, &txr->txr_intr, rxlimit, &rxr->rxr_intr);

	if (more != 0) {
		ixl_sched_handle_queue(sc, qp);
	} else {
		/* for ALTQ */
		if (txr->txr_qid == 0)
			if_schedule_deferred_start(&sc->sc_ec.ec_if);
		softint_schedule(txr->txr_si);

		ixl_enable_queue_intr(sc, qp);
	}

	return 1;
}

static void
ixl_handle_queue_wk(struct work *wk, void *xsc)
{
	struct ixl_queue_pair *qp;

	qp = container_of(wk, struct ixl_queue_pair, qp_work);
	ixl_handle_queue(qp);
}

static void
ixl_handle_queue(void *xqp)
{
	struct ixl_queue_pair *qp = xqp;
	struct ixl_softc *sc = qp->qp_sc;
	struct ixl_tx_ring *txr = qp->qp_txr;
	struct ixl_rx_ring *rxr = qp->qp_rxr;
	u_int txlimit, rxlimit;
	int more;

	txlimit = sc->sc_tx_process_limit;
	rxlimit = sc->sc_rx_process_limit;

	more = ixl_handle_queue_common(sc, qp,
	    txlimit, &txr->txr_defer, rxlimit, &rxr->rxr_defer);

	if (more != 0)
		ixl_sched_handle_queue(sc, qp);
	else
		ixl_enable_queue_intr(sc, qp);
}

static inline void
ixl_print_hmc_error(struct ixl_softc *sc, uint32_t reg)
{
	uint32_t hmc_idx, hmc_isvf;
	uint32_t hmc_errtype, hmc_objtype, hmc_data;

	hmc_idx = reg & I40E_PFHMC_ERRORINFO_PMF_INDEX_MASK;
	hmc_idx = hmc_idx >> I40E_PFHMC_ERRORINFO_PMF_INDEX_SHIFT;
	hmc_isvf = reg & I40E_PFHMC_ERRORINFO_PMF_ISVF_MASK;
	hmc_isvf = hmc_isvf >> I40E_PFHMC_ERRORINFO_PMF_ISVF_SHIFT;
	hmc_errtype = reg & I40E_PFHMC_ERRORINFO_HMC_ERROR_TYPE_MASK;
	hmc_errtype = hmc_errtype >> I40E_PFHMC_ERRORINFO_HMC_ERROR_TYPE_SHIFT;
	hmc_objtype = reg & I40E_PFHMC_ERRORINFO_HMC_OBJECT_TYPE_MASK;
	hmc_objtype = hmc_objtype >> I40E_PFHMC_ERRORINFO_HMC_OBJECT_TYPE_SHIFT;
	hmc_data = ixl_rd(sc, I40E_PFHMC_ERRORDATA);

	device_printf(sc->sc_dev,
	    "HMC Error (idx=0x%x, isvf=0x%x, err=0x%x, obj=0x%x, data=0x%x)\n",
	    hmc_idx, hmc_isvf, hmc_errtype, hmc_objtype, hmc_data);
}

static int
ixl_other_intr(void *xsc)
{
	struct ixl_softc *sc = xsc;
	uint32_t icr, mask, reg;
	int rv;

	icr = ixl_rd(sc, I40E_PFINT_ICR0);
	mask = ixl_rd(sc, I40E_PFINT_ICR0_ENA);

	if (ISSET(icr, I40E_PFINT_ICR0_ADMINQ_MASK)) {
		atomic_inc_64(&sc->sc_event_atq.ev_count);
		ixl_atq_done(sc);
		ixl_work_add(sc->sc_workq, &sc->sc_arq_task);
		rv = 1;
	}

	if (ISSET(icr, I40E_PFINT_ICR0_LINK_STAT_CHANGE_MASK)) {
		if (ISSET(sc->sc_ec.ec_if.if_flags, IFF_DEBUG))
			device_printf(sc->sc_dev, "link stat changed\n");

		atomic_inc_64(&sc->sc_event_link.ev_count);
		ixl_work_add(sc->sc_workq, &sc->sc_link_state_task);
		rv = 1;
	}

	if (ISSET(icr, I40E_PFINT_ICR0_GRST_MASK)) {
		CLR(mask, I40E_PFINT_ICR0_ENA_GRST_MASK);
		reg = ixl_rd(sc, I40E_GLGEN_RSTAT);
		reg = reg & I40E_GLGEN_RSTAT_RESET_TYPE_MASK;
		reg = reg >> I40E_GLGEN_RSTAT_RESET_TYPE_SHIFT;

		device_printf(sc->sc_dev, "GRST: %s\n",
		    reg == I40E_RESET_CORER ? "CORER" :
		    reg == I40E_RESET_GLOBR ? "GLOBR" :
		    reg == I40E_RESET_EMPR ? "EMPR" :
		    "POR");
	}

	if (ISSET(icr, I40E_PFINT_ICR0_ECC_ERR_MASK))
		atomic_inc_64(&sc->sc_event_ecc_err.ev_count);
	if (ISSET(icr, I40E_PFINT_ICR0_PCI_EXCEPTION_MASK))
		atomic_inc_64(&sc->sc_event_pci_exception.ev_count);
	if (ISSET(icr, I40E_PFINT_ICR0_PE_CRITERR_MASK))
		atomic_inc_64(&sc->sc_event_crit_err.ev_count);

	if (ISSET(icr, IXL_ICR0_CRIT_ERR_MASK)) {
		CLR(mask, IXL_ICR0_CRIT_ERR_MASK);
		device_printf(sc->sc_dev, "critical error\n");
	}

	if (ISSET(icr, I40E_PFINT_ICR0_HMC_ERR_MASK)) {
		reg = ixl_rd(sc, I40E_PFHMC_ERRORINFO);
		if (ISSET(reg, I40E_PFHMC_ERRORINFO_ERROR_DETECTED_MASK))
			ixl_print_hmc_error(sc, reg);
		ixl_wr(sc, I40E_PFHMC_ERRORINFO, 0);
	}

	ixl_wr(sc, I40E_PFINT_ICR0_ENA, mask);
	ixl_flush(sc);
	ixl_enable_other_intr(sc);
	return rv;
}

static void
ixl_get_link_status_done(struct ixl_softc *sc,
    const struct ixl_aq_desc *iaq)
{
	struct ixl_aq_desc iaq_buf;

	memcpy(&iaq_buf, iaq, sizeof(iaq_buf));

	/*
	 * The lock can be released here
	 * because there is no post processing about ATQ
	 */
	mutex_exit(&sc->sc_atq_lock);
	ixl_link_state_update(sc, &iaq_buf);
	mutex_enter(&sc->sc_atq_lock);
}

static void
ixl_get_link_status(void *xsc)
{
	struct ixl_softc *sc = xsc;
	struct ixl_aq_desc *iaq;
	struct ixl_aq_link_param *param;
	int error;

	mutex_enter(&sc->sc_atq_lock);

	iaq = &sc->sc_link_state_atq.iatq_desc;
	memset(iaq, 0, sizeof(*iaq));
	iaq->iaq_opcode = htole16(IXL_AQ_OP_PHY_LINK_STATUS);
	param = (struct ixl_aq_link_param *)iaq->iaq_param;
	param->notify = IXL_AQ_LINK_NOTIFY;

	error = ixl_atq_exec_locked(sc, &sc->sc_link_state_atq);
	ixl_atq_set(&sc->sc_link_state_atq, ixl_get_link_status_done);

	if (error == 0) {
		ixl_get_link_status_done(sc, iaq);
	}

	mutex_exit(&sc->sc_atq_lock);
}

static void
ixl_link_state_update(struct ixl_softc *sc, const struct ixl_aq_desc *iaq)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	int link_state;

	mutex_enter(&sc->sc_cfg_lock);
	link_state = ixl_set_link_status_locked(sc, iaq);
	mutex_exit(&sc->sc_cfg_lock);

	if (ifp->if_link_state != link_state)
		if_link_state_change(ifp, link_state);

	if (link_state != LINK_STATE_DOWN) {
		kpreempt_disable();
		if_schedule_deferred_start(ifp);
		kpreempt_enable();
	}
}

static void
ixl_aq_dump(const struct ixl_softc *sc, const struct ixl_aq_desc *iaq,
    const char *msg)
{
	char	 buf[512];
	size_t	 len;

	len = sizeof(buf);
	buf[--len] = '\0';

	device_printf(sc->sc_dev, "%s\n", msg);
	snprintb(buf, len, IXL_AQ_FLAGS_FMT, le16toh(iaq->iaq_flags));
	device_printf(sc->sc_dev, "flags %s opcode %04x\n",
	    buf, le16toh(iaq->iaq_opcode));
	device_printf(sc->sc_dev, "datalen %u retval %u\n",
	    le16toh(iaq->iaq_datalen), le16toh(iaq->iaq_retval));
	device_printf(sc->sc_dev, "cookie %016" PRIx64 "\n", iaq->iaq_cookie);
	device_printf(sc->sc_dev, "%08x %08x %08x %08x\n",
	    le32toh(iaq->iaq_param[0]), le32toh(iaq->iaq_param[1]),
	    le32toh(iaq->iaq_param[2]), le32toh(iaq->iaq_param[3]));
}

static void
ixl_arq(void *xsc)
{
	struct ixl_softc *sc = xsc;
	struct ixl_aq_desc *arq, *iaq;
	struct ixl_aq_buf *aqb;
	unsigned int cons = sc->sc_arq_cons;
	unsigned int prod;
	int done = 0;

	prod = ixl_rd(sc, sc->sc_aq_regs->arq_head) &
	    sc->sc_aq_regs->arq_head_mask;

	if (cons == prod)
		goto done;

	arq = IXL_DMA_KVA(&sc->sc_arq);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_arq),
	    0, IXL_DMA_LEN(&sc->sc_arq),
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

	do {
		iaq = &arq[cons];
		aqb = sc->sc_arq_live[cons];

		KASSERT(aqb != NULL);

		bus_dmamap_sync(sc->sc_dmat, aqb->aqb_map, 0, IXL_AQ_BUFLEN,
		    BUS_DMASYNC_POSTREAD);

		if (ISSET(sc->sc_ec.ec_if.if_flags, IFF_DEBUG))
			ixl_aq_dump(sc, iaq, "arq event");

		switch (iaq->iaq_opcode) {
		case htole16(IXL_AQ_OP_PHY_LINK_STATUS):
			ixl_link_state_update(sc, iaq);
			break;
		}

		memset(iaq, 0, sizeof(*iaq));
		sc->sc_arq_live[cons] = NULL;
		SIMPLEQ_INSERT_TAIL(&sc->sc_arq_idle, aqb, aqb_entry);

		cons++;
		cons &= IXL_AQ_MASK;

		done = 1;
	} while (cons != prod);

	if (done) {
		sc->sc_arq_cons = cons;
		ixl_arq_fill(sc);
		bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_arq),
		    0, IXL_DMA_LEN(&sc->sc_arq),
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	}

done:
	ixl_enable_other_intr(sc);
}

static void
ixl_atq_set(struct ixl_atq *iatq,
    void (*fn)(struct ixl_softc *, const struct ixl_aq_desc *))
{

	iatq->iatq_fn = fn;
}

static int
ixl_atq_post_locked(struct ixl_softc *sc, struct ixl_atq *iatq)
{
	struct ixl_aq_desc *atq, *slot;
	unsigned int prod, cons, prod_next;

	/* assert locked */
	KASSERT(mutex_owned(&sc->sc_atq_lock));

	atq = IXL_DMA_KVA(&sc->sc_atq);
	prod = sc->sc_atq_prod;
	cons = sc->sc_atq_cons;
	prod_next = (prod +1) & IXL_AQ_MASK;

	if (cons == prod_next)
		return ENOMEM;

	slot = &atq[prod];

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq), BUS_DMASYNC_POSTWRITE);

	KASSERT(iatq->iatq_fn != NULL);
	*slot = iatq->iatq_desc;
	slot->iaq_cookie = (uint64_t)((intptr_t)iatq);

	if (ISSET(sc->sc_ec.ec_if.if_flags, IFF_DEBUG))
		ixl_aq_dump(sc, slot, "atq command");

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq), BUS_DMASYNC_PREWRITE);

	sc->sc_atq_prod = prod_next;
	ixl_wr(sc, sc->sc_aq_regs->atq_tail, sc->sc_atq_prod);

	return 0;
}

static void
ixl_atq_done_locked(struct ixl_softc *sc)
{
	struct ixl_aq_desc *atq, *slot;
	struct ixl_atq *iatq;
	unsigned int cons;
	unsigned int prod;

	KASSERT(mutex_owned(&sc->sc_atq_lock));

	prod = sc->sc_atq_prod;
	cons = sc->sc_atq_cons;

	if (prod == cons)
		return;

	atq = IXL_DMA_KVA(&sc->sc_atq);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq),
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

	do {
		slot = &atq[cons];
		if (!ISSET(slot->iaq_flags, htole16(IXL_AQ_DD)))
			break;

		iatq = (struct ixl_atq *)((intptr_t)slot->iaq_cookie);
		iatq->iatq_desc = *slot;

		memset(slot, 0, sizeof(*slot));

		if (ISSET(sc->sc_ec.ec_if.if_flags, IFF_DEBUG))
			ixl_aq_dump(sc, &iatq->iatq_desc, "atq response");

		(*iatq->iatq_fn)(sc, &iatq->iatq_desc);

		cons++;
		cons &= IXL_AQ_MASK;
	} while (cons != prod);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	sc->sc_atq_cons = cons;
}

static void
ixl_atq_done(struct ixl_softc *sc)
{

	mutex_enter(&sc->sc_atq_lock);
	ixl_atq_done_locked(sc);
	mutex_exit(&sc->sc_atq_lock);
}

static void
ixl_wakeup(struct ixl_softc *sc, const struct ixl_aq_desc *iaq)
{

	KASSERT(mutex_owned(&sc->sc_atq_lock));

	cv_signal(&sc->sc_atq_cv);
}

static int
ixl_atq_exec(struct ixl_softc *sc, struct ixl_atq *iatq)
{
	int error;

	mutex_enter(&sc->sc_atq_lock);
	error = ixl_atq_exec_locked(sc, iatq);
	mutex_exit(&sc->sc_atq_lock);

	return error;
}

static int
ixl_atq_exec_locked(struct ixl_softc *sc, struct ixl_atq *iatq)
{
	int error;

	KASSERT(mutex_owned(&sc->sc_atq_lock));
	KASSERT(iatq->iatq_desc.iaq_cookie == 0);

	ixl_atq_set(iatq, ixl_wakeup);

	error = ixl_atq_post_locked(sc, iatq);
	if (error)
		return error;

	error = cv_timedwait(&sc->sc_atq_cv, &sc->sc_atq_lock,
	    IXL_ATQ_EXEC_TIMEOUT);

	return error;
}

static int
ixl_atq_poll(struct ixl_softc *sc, struct ixl_aq_desc *iaq, unsigned int tm)
{
	struct ixl_aq_desc *atq, *slot;
	unsigned int prod;
	unsigned int t = 0;

	mutex_enter(&sc->sc_atq_lock);

	atq = IXL_DMA_KVA(&sc->sc_atq);
	prod = sc->sc_atq_prod;
	slot = atq + prod;

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq), BUS_DMASYNC_POSTWRITE);

	*slot = *iaq;
	slot->iaq_flags |= htole16(IXL_AQ_SI);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq), BUS_DMASYNC_PREWRITE);

	prod++;
	prod &= IXL_AQ_MASK;
	sc->sc_atq_prod = prod;
	ixl_wr(sc, sc->sc_aq_regs->atq_tail, prod);

	while (ixl_rd(sc, sc->sc_aq_regs->atq_head) != prod) {
		delaymsec(1);

		if (t++ > tm) {
			mutex_exit(&sc->sc_atq_lock);
			return ETIMEDOUT;
		}
	}

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq), BUS_DMASYNC_POSTREAD);
	*iaq = *slot;
	memset(slot, 0, sizeof(*slot));
	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq), BUS_DMASYNC_PREREAD);

	sc->sc_atq_cons = prod;

	mutex_exit(&sc->sc_atq_lock);

	return 0;
}

static int
ixl_get_version(struct ixl_softc *sc)
{
	struct ixl_aq_desc iaq;
	uint32_t fwbuild, fwver, apiver;
	uint16_t api_maj_ver, api_min_ver;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_opcode = htole16(IXL_AQ_OP_GET_VERSION);

	iaq.iaq_retval = le16toh(23);

	if (ixl_atq_poll(sc, &iaq, 2000) != 0)
		return ETIMEDOUT;
	if (iaq.iaq_retval != htole16(IXL_AQ_RC_OK))
		return EIO;

	fwbuild = le32toh(iaq.iaq_param[1]);
	fwver = le32toh(iaq.iaq_param[2]);
	apiver = le32toh(iaq.iaq_param[3]);

	api_maj_ver = (uint16_t)apiver;
	api_min_ver = (uint16_t)(apiver >> 16);

	aprint_normal(", FW %hu.%hu.%05u API %hu.%hu", (uint16_t)fwver,
	    (uint16_t)(fwver >> 16), fwbuild, api_maj_ver, api_min_ver);

	if (sc->sc_mac_type == I40E_MAC_X722) {
		SET(sc->sc_aq_flags, IXL_SC_AQ_FLAG_NVMLOCK |
		    IXL_SC_AQ_FLAG_NVMREAD);
		SET(sc->sc_aq_flags, IXL_SC_AQ_FLAG_RXCTL);
		SET(sc->sc_aq_flags, IXL_SC_AQ_FLAG_RSS);
	}

#define IXL_API_VER(maj, min)	(((uint32_t)(maj) << 16) | (min))
	if (IXL_API_VER(api_maj_ver, api_min_ver) >= IXL_API_VER(1, 5)) {
		SET(sc->sc_aq_flags, IXL_SC_AQ_FLAG_RXCTL);
		SET(sc->sc_aq_flags, IXL_SC_AQ_FLAG_NVMLOCK);
	}
#undef IXL_API_VER

	return 0;
}

static int
ixl_get_nvm_version(struct ixl_softc *sc)
{
	uint16_t nvmver, cfg_ptr, eetrack_hi, eetrack_lo, oem_hi, oem_lo;
	uint32_t eetrack, oem;
	uint16_t nvm_maj_ver, nvm_min_ver, oem_build;
	uint8_t oem_ver, oem_patch;

	nvmver = cfg_ptr = eetrack_hi = eetrack_lo = oem_hi = oem_lo = 0;
	ixl_rd16_nvm(sc, I40E_SR_NVM_DEV_STARTER_VERSION, &nvmver);
	ixl_rd16_nvm(sc, I40E_SR_NVM_EETRACK_HI, &eetrack_hi);
	ixl_rd16_nvm(sc, I40E_SR_NVM_EETRACK_LO, &eetrack_lo);
	ixl_rd16_nvm(sc, I40E_SR_BOOT_CONFIG_PTR, &cfg_ptr);
	ixl_rd16_nvm(sc, cfg_ptr + I40E_NVM_OEM_VER_OFF, &oem_hi);
	ixl_rd16_nvm(sc, cfg_ptr + I40E_NVM_OEM_VER_OFF + 1, &oem_lo);

	nvm_maj_ver = (uint16_t)__SHIFTOUT(nvmver, IXL_NVM_VERSION_HI_MASK);
	nvm_min_ver = (uint16_t)__SHIFTOUT(nvmver, IXL_NVM_VERSION_LO_MASK);
	eetrack = ((uint32_t)eetrack_hi << 16) | eetrack_lo;
	oem = ((uint32_t)oem_hi << 16) | oem_lo;
	oem_ver = __SHIFTOUT(oem, IXL_NVM_OEMVERSION_MASK);
	oem_build = __SHIFTOUT(oem, IXL_NVM_OEMBUILD_MASK);
	oem_patch = __SHIFTOUT(oem, IXL_NVM_OEMPATCH_MASK);

	aprint_normal(" nvm %x.%02x etid %08x oem %d.%d.%d",
	    nvm_maj_ver, nvm_min_ver, eetrack,
	    oem_ver, oem_build, oem_patch);

	return 0;
}

static int
ixl_pxe_clear(struct ixl_softc *sc)
{
	struct ixl_aq_desc iaq;
	int rv;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_opcode = htole16(IXL_AQ_OP_CLEAR_PXE_MODE);
	iaq.iaq_param[0] = htole32(0x2);

	rv = ixl_atq_poll(sc, &iaq, 250);

	ixl_wr(sc, I40E_GLLAN_RCTL_0, 0x1);

	if (rv != 0)
		return ETIMEDOUT;

	switch (iaq.iaq_retval) {
	case htole16(IXL_AQ_RC_OK):
	case htole16(IXL_AQ_RC_EEXIST):
		break;
	default:
		return EIO;
	}

	return 0;
}

static int
ixl_lldp_shut(struct ixl_softc *sc)
{
	struct ixl_aq_desc iaq;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_opcode = htole16(IXL_AQ_OP_LLDP_STOP_AGENT);
	iaq.iaq_param[0] = htole32(IXL_LLDP_SHUTDOWN);

	if (ixl_atq_poll(sc, &iaq, 250) != 0) {
		aprint_error_dev(sc->sc_dev, "STOP LLDP AGENT timeout\n");
		return -1;
	}

	switch (iaq.iaq_retval) {
	case htole16(IXL_AQ_RC_EMODE):
	case htole16(IXL_AQ_RC_EPERM):
		/* ignore silently */
	default:
		break;
	}

	return 0;
}

static void
ixl_parse_hw_capability(struct ixl_softc *sc, struct ixl_aq_capability *cap)
{
	uint16_t id;
	uint32_t number, logical_id;

	id = le16toh(cap->cap_id);
	number = le32toh(cap->number);
	logical_id = le32toh(cap->logical_id);

	switch (id) {
	case IXL_AQ_CAP_RSS:
		sc->sc_rss_table_size = number;
		sc->sc_rss_table_entry_width = logical_id;
		break;
	case IXL_AQ_CAP_RXQ:
	case IXL_AQ_CAP_TXQ:
		sc->sc_nqueue_pairs_device = MIN(number,
		    sc->sc_nqueue_pairs_device);
		break;
	}
}

static int
ixl_get_hw_capabilities(struct ixl_softc *sc)
{
	struct ixl_dmamem idm;
	struct ixl_aq_desc iaq;
	struct ixl_aq_capability *caps;
	size_t i, ncaps;
	bus_size_t caps_size;
	uint16_t status;
	int rv;

	caps_size = sizeof(caps[0]) * 40;
	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_opcode = htole16(IXL_AQ_OP_LIST_FUNC_CAP);

	do {
		if (ixl_dmamem_alloc(sc, &idm, caps_size, 0) != 0) {
			return -1;
		}

		iaq.iaq_flags = htole16(IXL_AQ_BUF |
		    (caps_size > I40E_AQ_LARGE_BUF ? IXL_AQ_LB : 0));
		iaq.iaq_datalen = htole16(caps_size);
		ixl_aq_dva(&iaq, IXL_DMA_DVA(&idm));

		bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&idm), 0,
		    IXL_DMA_LEN(&idm), BUS_DMASYNC_PREREAD);

		rv = ixl_atq_poll(sc, &iaq, 250);

		bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&idm), 0,
		    IXL_DMA_LEN(&idm), BUS_DMASYNC_POSTREAD);

		if (rv != 0) {
			aprint_error(", HW capabilities timeout\n");
			goto done;
		}

		status = le16toh(iaq.iaq_retval);

		if (status == IXL_AQ_RC_ENOMEM) {
			caps_size = le16toh(iaq.iaq_datalen);
			ixl_dmamem_free(sc, &idm);
		}
	} while (status == IXL_AQ_RC_ENOMEM);

	if (status != IXL_AQ_RC_OK) {
		aprint_error(", HW capabilities error\n");
		goto done;
	}

	caps = IXL_DMA_KVA(&idm);
	ncaps = le16toh(iaq.iaq_param[1]);

	for (i = 0; i < ncaps; i++) {
		ixl_parse_hw_capability(sc, &caps[i]);
	}

done:
	ixl_dmamem_free(sc, &idm);
	return rv;
}

static int
ixl_get_mac(struct ixl_softc *sc)
{
	struct ixl_dmamem idm;
	struct ixl_aq_desc iaq;
	struct ixl_aq_mac_addresses *addrs;
	int rv;

	if (ixl_dmamem_alloc(sc, &idm, sizeof(*addrs), 0) != 0) {
		aprint_error(", unable to allocate mac addresses\n");
		return -1;
	}

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF);
	iaq.iaq_opcode = htole16(IXL_AQ_OP_MAC_ADDRESS_READ);
	iaq.iaq_datalen = htole16(sizeof(*addrs));
	ixl_aq_dva(&iaq, IXL_DMA_DVA(&idm));

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&idm), 0, IXL_DMA_LEN(&idm),
	    BUS_DMASYNC_PREREAD);

	rv = ixl_atq_poll(sc, &iaq, 250);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&idm), 0, IXL_DMA_LEN(&idm),
	    BUS_DMASYNC_POSTREAD);

	if (rv != 0) {
		aprint_error(", MAC ADDRESS READ timeout\n");
		rv = -1;
		goto done;
	}
	if (iaq.iaq_retval != htole16(IXL_AQ_RC_OK)) {
		aprint_error(", MAC ADDRESS READ error\n");
		rv = -1;
		goto done;
	}

	addrs = IXL_DMA_KVA(&idm);
	if (!ISSET(iaq.iaq_param[0], htole32(IXL_AQ_MAC_PORT_VALID))) {
		printf(", port address is not valid\n");
		goto done;
	}

	memcpy(sc->sc_enaddr, addrs->port, ETHER_ADDR_LEN);
	rv = 0;

done:
	ixl_dmamem_free(sc, &idm);
	return rv;
}

static int
ixl_get_switch_config(struct ixl_softc *sc)
{
	struct ixl_dmamem idm;
	struct ixl_aq_desc iaq;
	struct ixl_aq_switch_config *hdr;
	struct ixl_aq_switch_config_element *elms, *elm;
	unsigned int nelm, i;
	int rv;

	if (ixl_dmamem_alloc(sc, &idm, IXL_AQ_BUFLEN, 0) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to allocate switch config buffer\n");
		return -1;
	}

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF |
	    (IXL_AQ_BUFLEN > I40E_AQ_LARGE_BUF ? IXL_AQ_LB : 0));
	iaq.iaq_opcode = htole16(IXL_AQ_OP_SWITCH_GET_CONFIG);
	iaq.iaq_datalen = htole16(IXL_AQ_BUFLEN);
	ixl_aq_dva(&iaq, IXL_DMA_DVA(&idm));

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&idm), 0, IXL_DMA_LEN(&idm),
	    BUS_DMASYNC_PREREAD);

	rv = ixl_atq_poll(sc, &iaq, 250);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&idm), 0, IXL_DMA_LEN(&idm),
	    BUS_DMASYNC_POSTREAD);

	if (rv != 0) {
		aprint_error_dev(sc->sc_dev, "GET SWITCH CONFIG timeout\n");
		rv = -1;
		goto done;
	}
	if (iaq.iaq_retval != htole16(IXL_AQ_RC_OK)) {
		aprint_error_dev(sc->sc_dev, "GET SWITCH CONFIG error\n");
		rv = -1;
		goto done;
	}

	hdr = IXL_DMA_KVA(&idm);
	elms = (struct ixl_aq_switch_config_element *)(hdr + 1);

	nelm = le16toh(hdr->num_reported);
	if (nelm < 1) {
		aprint_error_dev(sc->sc_dev, "no switch config available\n");
		rv = -1;
		goto done;
	}

	for (i = 0; i < nelm; i++) {
		elm = &elms[i];

		aprint_debug_dev(sc->sc_dev,
		    "type %x revision %u seid %04x\n",
		    elm->type, elm->revision, le16toh(elm->seid));
		aprint_debug_dev(sc->sc_dev,
		    "uplink %04x downlink %04x\n",
		    le16toh(elm->uplink_seid),
		    le16toh(elm->downlink_seid));
		aprint_debug_dev(sc->sc_dev,
		    "conntype %x scheduler %04x extra %04x\n",
		    elm->connection_type,
		    le16toh(elm->scheduler_id),
		    le16toh(elm->element_info));
	}

	elm = &elms[0];

	sc->sc_uplink_seid = elm->uplink_seid;
	sc->sc_downlink_seid = elm->downlink_seid;
	sc->sc_seid = elm->seid;

	if ((sc->sc_uplink_seid == htole16(0)) !=
	    (sc->sc_downlink_seid == htole16(0))) {
		aprint_error_dev(sc->sc_dev, "SEIDs are misconfigured\n");
		rv = -1;
		goto done;
	}

done:
	ixl_dmamem_free(sc, &idm);
	return rv;
}

static int
ixl_phy_mask_ints(struct ixl_softc *sc)
{
	struct ixl_aq_desc iaq;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_opcode = htole16(IXL_AQ_OP_PHY_SET_EVENT_MASK);
	iaq.iaq_param[2] = htole32(IXL_AQ_PHY_EV_MASK &
	    ~(IXL_AQ_PHY_EV_LINK_UPDOWN | IXL_AQ_PHY_EV_MODULE_QUAL_FAIL |
	      IXL_AQ_PHY_EV_MEDIA_NA));

	if (ixl_atq_poll(sc, &iaq, 250) != 0) {
		aprint_error_dev(sc->sc_dev, "SET PHY EVENT MASK timeout\n");
		return -1;
	}
	if (iaq.iaq_retval != htole16(IXL_AQ_RC_OK)) {
		aprint_error_dev(sc->sc_dev, "SET PHY EVENT MASK error\n");
		return -1;
	}

	return 0;
}

static int
ixl_get_phy_abilities(struct ixl_softc *sc, struct ixl_dmamem *idm)
{
	struct ixl_aq_desc iaq;
	int rv;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF |
	    (IXL_DMA_LEN(idm) > I40E_AQ_LARGE_BUF ? IXL_AQ_LB : 0));
	iaq.iaq_opcode = htole16(IXL_AQ_OP_PHY_GET_ABILITIES);
	iaq.iaq_datalen = htole16(IXL_DMA_LEN(idm));
	iaq.iaq_param[0] = htole32(IXL_AQ_PHY_REPORT_INIT);
	ixl_aq_dva(&iaq, IXL_DMA_DVA(idm));

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(idm), 0, IXL_DMA_LEN(idm),
	    BUS_DMASYNC_PREREAD);

	rv = ixl_atq_poll(sc, &iaq, 250);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(idm), 0, IXL_DMA_LEN(idm),
	    BUS_DMASYNC_POSTREAD);

	if (rv != 0)
		return -1;

	return le16toh(iaq.iaq_retval);
}

static int
ixl_get_phy_info(struct ixl_softc *sc)
{
	struct ixl_dmamem idm;
	struct ixl_aq_phy_abilities *phy;
	int rv;

	if (ixl_dmamem_alloc(sc, &idm, IXL_AQ_BUFLEN, 0) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to allocate phy abilities buffer\n");
		return -1;
	}

	rv = ixl_get_phy_abilities(sc, &idm);
	switch (rv) {
	case -1:
		aprint_error_dev(sc->sc_dev, "GET PHY ABILITIES timeout\n");
		goto done;
	case IXL_AQ_RC_OK:
		break;
	case IXL_AQ_RC_EIO:
		aprint_error_dev(sc->sc_dev,"unable to query phy types\n");
		goto done;
	default:
		aprint_error_dev(sc->sc_dev,
		    "GET PHY ABILITIIES error %u\n", rv);
		goto done;
	}

	phy = IXL_DMA_KVA(&idm);

	sc->sc_phy_types = le32toh(phy->phy_type);
	sc->sc_phy_types |= (uint64_t)le32toh(phy->phy_type_ext) << 32;

	sc->sc_phy_abilities = phy->abilities;
	sc->sc_phy_linkspeed = phy->link_speed;
	sc->sc_phy_fec_cfg = phy->fec_cfg_curr_mod_ext_info &
	    (IXL_AQ_ENABLE_FEC_KR | IXL_AQ_ENABLE_FEC_RS |
	    IXL_AQ_REQUEST_FEC_KR | IXL_AQ_REQUEST_FEC_RS);
	sc->sc_eee_cap = phy->eee_capability;
	sc->sc_eeer_val = phy->eeer_val;
	sc->sc_d3_lpan = phy->d3_lpan;

	rv = 0;

done:
	ixl_dmamem_free(sc, &idm);
	return rv;
}

static int
ixl_set_phy_config(struct ixl_softc *sc,
    uint8_t link_speed, uint8_t abilities, bool polling)
{
	struct ixl_aq_phy_param *param;
	struct ixl_atq iatq;
	struct ixl_aq_desc *iaq;
	int error;

	memset(&iatq, 0, sizeof(iatq));

	iaq = &iatq.iatq_desc;
	iaq->iaq_opcode = htole16(IXL_AQ_OP_PHY_SET_CONFIG);
	param = (struct ixl_aq_phy_param *)&iaq->iaq_param;
	param->phy_types = htole32((uint32_t)sc->sc_phy_types);
	param->phy_type_ext = (uint8_t)(sc->sc_phy_types >> 32);
	param->link_speed = link_speed;
	param->abilities = abilities | IXL_AQ_PHY_ABILITY_AUTO_LINK;
	param->fec_cfg = sc->sc_phy_fec_cfg;
	param->eee_capability = sc->sc_eee_cap;
	param->eeer_val = sc->sc_eeer_val;
	param->d3_lpan = sc->sc_d3_lpan;

	if (polling)
		error = ixl_atq_poll(sc, iaq, 250);
	else
		error = ixl_atq_exec(sc, &iatq);

	if (error != 0)
		return error;

	switch (le16toh(iaq->iaq_retval)) {
	case IXL_AQ_RC_OK:
		break;
	case IXL_AQ_RC_EPERM:
		return EPERM;
	default:
		return EIO;
	}

	return 0;
}

static int
ixl_set_phy_autoselect(struct ixl_softc *sc)
{
	uint8_t link_speed, abilities;

	link_speed = sc->sc_phy_linkspeed;
	abilities = IXL_PHY_ABILITY_LINKUP | IXL_PHY_ABILITY_AUTONEGO;

	return ixl_set_phy_config(sc, link_speed, abilities, true);
}

static int
ixl_get_link_status_poll(struct ixl_softc *sc, int *l)
{
	struct ixl_aq_desc iaq;
	struct ixl_aq_link_param *param;
	int link;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_opcode = htole16(IXL_AQ_OP_PHY_LINK_STATUS);
	param = (struct ixl_aq_link_param *)iaq.iaq_param;
	param->notify = IXL_AQ_LINK_NOTIFY;

	if (ixl_atq_poll(sc, &iaq, 250) != 0) {
		return ETIMEDOUT;
	}
	if (iaq.iaq_retval != htole16(IXL_AQ_RC_OK)) {
		return EIO;
	}

	/* It is unneccessary to hold lock */
	link = ixl_set_link_status_locked(sc, &iaq);

	if (l != NULL)
		*l = link;

	return 0;
}

static int
ixl_get_vsi(struct ixl_softc *sc)
{
	struct ixl_dmamem *vsi = &sc->sc_scratch;
	struct ixl_aq_desc iaq;
	struct ixl_aq_vsi_param *param;
	struct ixl_aq_vsi_reply *reply;
	struct ixl_aq_vsi_data *data;
	int rv;

	/* grumble, vsi info isn't "known" at compile time */

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF |
	    (IXL_DMA_LEN(vsi) > I40E_AQ_LARGE_BUF ? IXL_AQ_LB : 0));
	iaq.iaq_opcode = htole16(IXL_AQ_OP_GET_VSI_PARAMS);
	iaq.iaq_datalen = htole16(IXL_DMA_LEN(vsi));
	ixl_aq_dva(&iaq, IXL_DMA_DVA(vsi));

	param = (struct ixl_aq_vsi_param *)iaq.iaq_param;
	param->uplink_seid = sc->sc_seid;

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(vsi), 0, IXL_DMA_LEN(vsi),
	    BUS_DMASYNC_PREREAD);

	rv = ixl_atq_poll(sc, &iaq, 250);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(vsi), 0, IXL_DMA_LEN(vsi),
	    BUS_DMASYNC_POSTREAD);

	if (rv != 0) {
		return ETIMEDOUT;
	}

	switch (le16toh(iaq.iaq_retval)) {
	case IXL_AQ_RC_OK:
		break;
	case IXL_AQ_RC_ENOENT:
		return ENOENT;
	case IXL_AQ_RC_EACCES:
		return EACCES;
	default:
		return EIO;
	}

	reply = (struct ixl_aq_vsi_reply *)iaq.iaq_param;
	sc->sc_vsi_number = le16toh(reply->vsi_number);
	data = IXL_DMA_KVA(vsi);
	sc->sc_vsi_stat_counter_idx = le16toh(data->stat_counter_idx);

	return 0;
}

static int
ixl_set_vsi(struct ixl_softc *sc)
{
	struct ixl_dmamem *vsi = &sc->sc_scratch;
	struct ixl_aq_desc iaq;
	struct ixl_aq_vsi_param *param;
	struct ixl_aq_vsi_data *data = IXL_DMA_KVA(vsi);
	unsigned int qnum;
	uint16_t val;
	int rv;

	qnum = sc->sc_nqueue_pairs - 1;

	data->valid_sections = htole16(IXL_AQ_VSI_VALID_QUEUE_MAP |
	    IXL_AQ_VSI_VALID_VLAN);

	CLR(data->mapping_flags, htole16(IXL_AQ_VSI_QUE_MAP_MASK));
	SET(data->mapping_flags, htole16(IXL_AQ_VSI_QUE_MAP_CONTIG));
	data->queue_mapping[0] = htole16(0);
	data->tc_mapping[0] = htole16((0 << IXL_AQ_VSI_TC_Q_OFFSET_SHIFT) |
	    (qnum << IXL_AQ_VSI_TC_Q_NUMBER_SHIFT));

	val = le16toh(data->port_vlan_flags);
	CLR(val, IXL_AQ_VSI_PVLAN_MODE_MASK | IXL_AQ_VSI_PVLAN_EMOD_MASK);
	SET(val, IXL_AQ_VSI_PVLAN_MODE_ALL);

	if (ISSET(sc->sc_cur_ec_capenable, ETHERCAP_VLAN_HWTAGGING)) {
		SET(val, IXL_AQ_VSI_PVLAN_EMOD_STR_BOTH);
	} else {
		SET(val, IXL_AQ_VSI_PVLAN_EMOD_NOTHING);
	}

	data->port_vlan_flags = htole16(val);

	/* grumble, vsi info isn't "known" at compile time */

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD |
	    (IXL_DMA_LEN(vsi) > I40E_AQ_LARGE_BUF ? IXL_AQ_LB : 0));
	iaq.iaq_opcode = htole16(IXL_AQ_OP_UPD_VSI_PARAMS);
	iaq.iaq_datalen = htole16(IXL_DMA_LEN(vsi));
	ixl_aq_dva(&iaq, IXL_DMA_DVA(vsi));

	param = (struct ixl_aq_vsi_param *)iaq.iaq_param;
	param->uplink_seid = sc->sc_seid;

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(vsi), 0, IXL_DMA_LEN(vsi),
	    BUS_DMASYNC_PREWRITE);

	rv = ixl_atq_poll(sc, &iaq, 250);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(vsi), 0, IXL_DMA_LEN(vsi),
	    BUS_DMASYNC_POSTWRITE);

	if (rv != 0) {
		return ETIMEDOUT;
	}

	switch (le16toh(iaq.iaq_retval)) {
	case IXL_AQ_RC_OK:
		break;
	case IXL_AQ_RC_ENOENT:
		return ENOENT;
	case IXL_AQ_RC_EACCES:
		return EACCES;
	default:
		return EIO;
	}

	return 0;
}

static void
ixl_set_filter_control(struct ixl_softc *sc)
{
	uint32_t reg;

	reg = ixl_rd_rx_csr(sc, I40E_PFQF_CTL_0);

	CLR(reg, I40E_PFQF_CTL_0_HASHLUTSIZE_MASK);
	SET(reg, I40E_HASH_LUT_SIZE_128 << I40E_PFQF_CTL_0_HASHLUTSIZE_SHIFT);

	SET(reg, I40E_PFQF_CTL_0_FD_ENA_MASK);
	SET(reg, I40E_PFQF_CTL_0_ETYPE_ENA_MASK);
	SET(reg, I40E_PFQF_CTL_0_MACVLAN_ENA_MASK);

	ixl_wr_rx_csr(sc, I40E_PFQF_CTL_0, reg);
}

static inline void
ixl_get_default_rss_key(uint32_t *buf, size_t len)
{
	size_t cplen;
	uint8_t rss_seed[RSS_KEYSIZE];

	rss_getkey(rss_seed);
	memset(buf, 0, len);

	cplen = MIN(len, sizeof(rss_seed));
	memcpy(buf, rss_seed, cplen);
}

static int
ixl_set_rss_key(struct ixl_softc *sc, uint8_t *key, size_t keylen)
{
	struct ixl_dmamem *idm;
	struct ixl_atq iatq;
	struct ixl_aq_desc *iaq;
	struct ixl_aq_rss_key_param *param;
	struct ixl_aq_rss_key_data *data;
	size_t len, datalen, stdlen, extlen;
	uint16_t vsi_id;
	int rv;

	memset(&iatq, 0, sizeof(iatq));
	iaq = &iatq.iatq_desc;
	idm = &sc->sc_aqbuf;

	datalen = sizeof(*data);

	/*XXX The buf size has to be less than the size of the register */
	datalen = MIN(IXL_RSS_KEY_SIZE_REG * sizeof(uint32_t), datalen);

	iaq->iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD |
	    (datalen > I40E_AQ_LARGE_BUF ? IXL_AQ_LB : 0));
	iaq->iaq_opcode = htole16(IXL_AQ_OP_RSS_SET_KEY);
	iaq->iaq_datalen = htole16(datalen);

	param = (struct ixl_aq_rss_key_param *)iaq->iaq_param;
	vsi_id = (sc->sc_vsi_number << IXL_AQ_RSSKEY_VSI_ID_SHIFT) |
	    IXL_AQ_RSSKEY_VSI_VALID;
	param->vsi_id = htole16(vsi_id);

	memset(IXL_DMA_KVA(idm), 0, IXL_DMA_LEN(idm));
	data = IXL_DMA_KVA(idm);

	len = MIN(keylen, datalen);
	stdlen = MIN(sizeof(data->standard_rss_key), len);
	memcpy(data->standard_rss_key, key, stdlen);
	len = (len > stdlen) ? (len - stdlen) : 0;

	extlen = MIN(sizeof(data->extended_hash_key), len);
	extlen = (stdlen < keylen) ? 0 : keylen - stdlen;
	memcpy(data->extended_hash_key, key + stdlen, extlen);

	ixl_aq_dva(iaq, IXL_DMA_DVA(idm));

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(idm), 0,
	    IXL_DMA_LEN(idm), BUS_DMASYNC_PREWRITE);

	rv = ixl_atq_exec(sc, &iatq);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(idm), 0,
	    IXL_DMA_LEN(idm), BUS_DMASYNC_POSTWRITE);

	if (rv != 0) {
		return ETIMEDOUT;
	}

	if (iaq->iaq_retval != htole16(IXL_AQ_RC_OK)) {
		return EIO;
	}

	return 0;
}

static int
ixl_set_rss_lut(struct ixl_softc *sc, uint8_t *lut, size_t lutlen)
{
	struct ixl_dmamem *idm;
	struct ixl_atq iatq;
	struct ixl_aq_desc *iaq;
	struct ixl_aq_rss_lut_param *param;
	uint16_t vsi_id;
	uint8_t *data;
	size_t dmalen;
	int rv;

	memset(&iatq, 0, sizeof(iatq));
	iaq = &iatq.iatq_desc;
	idm = &sc->sc_aqbuf;

	dmalen = MIN(lutlen, IXL_DMA_LEN(idm));

	iaq->iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD |
	    (dmalen > I40E_AQ_LARGE_BUF ? IXL_AQ_LB : 0));
	iaq->iaq_opcode = htole16(IXL_AQ_OP_RSS_SET_LUT);
	iaq->iaq_datalen = htole16(dmalen);

	memset(IXL_DMA_KVA(idm), 0, IXL_DMA_LEN(idm));
	data = IXL_DMA_KVA(idm);
	memcpy(data, lut, dmalen);
	ixl_aq_dva(iaq, IXL_DMA_DVA(idm));

	param = (struct ixl_aq_rss_lut_param *)iaq->iaq_param;
	vsi_id = (sc->sc_vsi_number << IXL_AQ_RSSLUT_VSI_ID_SHIFT) |
	    IXL_AQ_RSSLUT_VSI_VALID;
	param->vsi_id = htole16(vsi_id);
	param->flags = htole16(IXL_AQ_RSSLUT_TABLE_TYPE_PF <<
	    IXL_AQ_RSSLUT_TABLE_TYPE_SHIFT);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(idm), 0,
	    IXL_DMA_LEN(idm), BUS_DMASYNC_PREWRITE);

	rv = ixl_atq_exec(sc, &iatq);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(idm), 0,
	    IXL_DMA_LEN(idm), BUS_DMASYNC_POSTWRITE);

	if (rv != 0) {
		return ETIMEDOUT;
	}

	if (iaq->iaq_retval != htole16(IXL_AQ_RC_OK)) {
		return EIO;
	}

	return 0;
}

static int
ixl_register_rss_key(struct ixl_softc *sc)
{
	uint32_t rss_seed[IXL_RSS_KEY_SIZE_REG];
	int rv;
	size_t i;

	ixl_get_default_rss_key(rss_seed, sizeof(rss_seed));

	if (ISSET(sc->sc_aq_flags, IXL_SC_AQ_FLAG_RSS)) {
		rv = ixl_set_rss_key(sc, (uint8_t*)rss_seed,
		    sizeof(rss_seed));
	} else {
		rv = 0;
		for (i = 0; i < IXL_RSS_KEY_SIZE_REG; i++) {
			ixl_wr_rx_csr(sc, I40E_PFQF_HKEY(i), rss_seed[i]);
		}
	}

	return rv;
}

static void
ixl_register_rss_pctype(struct ixl_softc *sc)
{
	uint64_t set_hena = 0;
	uint32_t hena0, hena1;

	/*
	 * We use TCP/UDP with IPv4/IPv6 by default.
	 * Note: the device can not use just IP header in each
	 * TCP/UDP packets for the RSS hash calculation.
	 */
	if (sc->sc_mac_type == I40E_MAC_X722)
		set_hena = IXL_RSS_HENA_DEFAULT_X722;
	else
		set_hena = IXL_RSS_HENA_DEFAULT_XL710;

	hena0 = ixl_rd_rx_csr(sc, I40E_PFQF_HENA(0));
	hena1 = ixl_rd_rx_csr(sc, I40E_PFQF_HENA(1));

	SET(hena0, set_hena);
	SET(hena1, set_hena >> 32);

	ixl_wr_rx_csr(sc, I40E_PFQF_HENA(0), hena0);
	ixl_wr_rx_csr(sc, I40E_PFQF_HENA(1), hena1);
}

static int
ixl_register_rss_hlut(struct ixl_softc *sc)
{
	unsigned int qid;
	uint8_t hlut_buf[512], lut_mask;
	uint32_t *hluts;
	size_t i, hluts_num;
	int rv;

	lut_mask = (0x01 << sc->sc_rss_table_entry_width) - 1;

	for (i = 0; i < sc->sc_rss_table_size; i++) {
		qid = i % sc->sc_nqueue_pairs;
		hlut_buf[i] = qid & lut_mask;
	}

	if (ISSET(sc->sc_aq_flags, IXL_SC_AQ_FLAG_RSS)) {
		rv = ixl_set_rss_lut(sc, hlut_buf, sizeof(hlut_buf));
	} else {
		rv = 0;
		hluts = (uint32_t *)hlut_buf;
		hluts_num = sc->sc_rss_table_size >> 2;
		for (i = 0; i < hluts_num; i++) {
			ixl_wr(sc, I40E_PFQF_HLUT(i), hluts[i]);
		}
		ixl_flush(sc);
	}

	return rv;
}

static void
ixl_config_rss(struct ixl_softc *sc)
{

	KASSERT(mutex_owned(&sc->sc_cfg_lock));

	ixl_register_rss_key(sc);
	ixl_register_rss_pctype(sc);
	ixl_register_rss_hlut(sc);
}

static const struct ixl_phy_type *
ixl_search_phy_type(uint8_t phy_type)
{
	const struct ixl_phy_type *itype;
	uint64_t mask;
	unsigned int i;

	if (phy_type >= 64)
		return NULL;

	mask = 1ULL << phy_type;

	for (i = 0; i < __arraycount(ixl_phy_type_map); i++) {
		itype = &ixl_phy_type_map[i];

		if (ISSET(itype->phy_type, mask))
			return itype;
	}

	return NULL;
}

static uint64_t
ixl_search_link_speed(uint8_t link_speed)
{
	const struct ixl_speed_type *type;
	unsigned int i;

	for (i = 0; i < __arraycount(ixl_speed_type_map); i++) {
		type = &ixl_speed_type_map[i];

		if (ISSET(type->dev_speed, link_speed))
			return type->net_speed;
	}

	return 0;
}

static uint8_t
ixl_search_baudrate(uint64_t baudrate)
{
	const struct ixl_speed_type *type;
	unsigned int i;

	for (i = 0; i < __arraycount(ixl_speed_type_map); i++) {
		type = &ixl_speed_type_map[i];

		if (type->net_speed == baudrate) {
			return type->dev_speed;
		}
	}

	return 0;
}

static int
ixl_restart_an(struct ixl_softc *sc)
{
	struct ixl_aq_desc iaq;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_opcode = htole16(IXL_AQ_OP_PHY_RESTART_AN);
	iaq.iaq_param[0] =
	    htole32(IXL_AQ_PHY_RESTART_AN | IXL_AQ_PHY_LINK_ENABLE);

	if (ixl_atq_poll(sc, &iaq, 250) != 0) {
		aprint_error_dev(sc->sc_dev, "RESTART AN timeout\n");
		return -1;
	}
	if (iaq.iaq_retval != htole16(IXL_AQ_RC_OK)) {
		aprint_error_dev(sc->sc_dev, "RESTART AN error\n");
		return -1;
	}

	return 0;
}

static int
ixl_add_macvlan(struct ixl_softc *sc, const uint8_t *macaddr,
    uint16_t vlan, uint16_t flags)
{
	struct ixl_aq_desc iaq;
	struct ixl_aq_add_macvlan *param;
	struct ixl_aq_add_macvlan_elem *elem;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IXL_AQ_OP_ADD_MACVLAN);
	iaq.iaq_datalen = htole16(sizeof(*elem));
	ixl_aq_dva(&iaq, IXL_DMA_DVA(&sc->sc_scratch));

	param = (struct ixl_aq_add_macvlan *)&iaq.iaq_param;
	param->num_addrs = htole16(1);
	param->seid0 = htole16(0x8000) | sc->sc_seid;
	param->seid1 = 0;
	param->seid2 = 0;

	elem = IXL_DMA_KVA(&sc->sc_scratch);
	memset(elem, 0, sizeof(*elem));
	memcpy(elem->macaddr, macaddr, ETHER_ADDR_LEN);
	elem->flags = htole16(IXL_AQ_OP_ADD_MACVLAN_PERFECT_MATCH | flags);
	elem->vlan = htole16(vlan);

	if (ixl_atq_poll(sc, &iaq, 250) != 0) {
		return IXL_AQ_RC_EINVAL;
	}

	switch (le16toh(iaq.iaq_retval)) {
	case IXL_AQ_RC_OK:
		break;
	case IXL_AQ_RC_ENOSPC:
		return ENOSPC;
	case IXL_AQ_RC_ENOENT:
		return ENOENT;
	case IXL_AQ_RC_EACCES:
		return EACCES;
	case IXL_AQ_RC_EEXIST:
		return EEXIST;
	case IXL_AQ_RC_EINVAL:
		return EINVAL;
	default:
		return EIO;
	}

	return 0;
}

static int
ixl_remove_macvlan(struct ixl_softc *sc, const uint8_t *macaddr,
    uint16_t vlan, uint16_t flags)
{
	struct ixl_aq_desc iaq;
	struct ixl_aq_remove_macvlan *param;
	struct ixl_aq_remove_macvlan_elem *elem;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IXL_AQ_OP_REMOVE_MACVLAN);
	iaq.iaq_datalen = htole16(sizeof(*elem));
	ixl_aq_dva(&iaq, IXL_DMA_DVA(&sc->sc_scratch));

	param = (struct ixl_aq_remove_macvlan *)&iaq.iaq_param;
	param->num_addrs = htole16(1);
	param->seid0 = htole16(0x8000) | sc->sc_seid;
	param->seid1 = 0;
	param->seid2 = 0;

	elem = IXL_DMA_KVA(&sc->sc_scratch);
	memset(elem, 0, sizeof(*elem));
	memcpy(elem->macaddr, macaddr, ETHER_ADDR_LEN);
	elem->flags = htole16(IXL_AQ_OP_REMOVE_MACVLAN_PERFECT_MATCH | flags);
	elem->vlan = htole16(vlan);

	if (ixl_atq_poll(sc, &iaq, 250) != 0) {
		return EINVAL;
	}

	switch (le16toh(iaq.iaq_retval)) {
	case IXL_AQ_RC_OK:
		break;
	case IXL_AQ_RC_ENOENT:
		return ENOENT;
	case IXL_AQ_RC_EACCES:
		return EACCES;
	case IXL_AQ_RC_EINVAL:
		return EINVAL;
	default:
		return EIO;
	}

	return 0;
}

static int
ixl_hmc(struct ixl_softc *sc)
{
	struct {
		uint32_t   count;
		uint32_t   minsize;
		bus_size_t objsiz;
		bus_size_t setoff;
		bus_size_t setcnt;
	} regs[] = {
		{
			0,
			IXL_HMC_TXQ_MINSIZE,
			I40E_GLHMC_LANTXOBJSZ,
			I40E_GLHMC_LANTXBASE(sc->sc_pf_id),
			I40E_GLHMC_LANTXCNT(sc->sc_pf_id),
		},
		{
			0,
			IXL_HMC_RXQ_MINSIZE,
			I40E_GLHMC_LANRXOBJSZ,
			I40E_GLHMC_LANRXBASE(sc->sc_pf_id),
			I40E_GLHMC_LANRXCNT(sc->sc_pf_id),
		},
		{
			0,
			0,
			I40E_GLHMC_FCOEDDPOBJSZ,
			I40E_GLHMC_FCOEDDPBASE(sc->sc_pf_id),
			I40E_GLHMC_FCOEDDPCNT(sc->sc_pf_id),
		},
		{
			0,
			0,
			I40E_GLHMC_FCOEFOBJSZ,
			I40E_GLHMC_FCOEFBASE(sc->sc_pf_id),
			I40E_GLHMC_FCOEFCNT(sc->sc_pf_id),
		},
	};
	struct ixl_hmc_entry *e;
	uint64_t size, dva;
	uint8_t *kva;
	uint64_t *sdpage;
	unsigned int i;
	int npages, tables;
	uint32_t reg;

	CTASSERT(__arraycount(regs) <= __arraycount(sc->sc_hmc_entries));

	regs[IXL_HMC_LAN_TX].count = regs[IXL_HMC_LAN_RX].count =
	    ixl_rd(sc, I40E_GLHMC_LANQMAX);

	size = 0;
	for (i = 0; i < __arraycount(regs); i++) {
		e = &sc->sc_hmc_entries[i];

		e->hmc_count = regs[i].count;
		reg = ixl_rd(sc, regs[i].objsiz);
		e->hmc_size = IXL_BIT_ULL(0x3F & reg);
		e->hmc_base = size;

		if ((e->hmc_size * 8) < regs[i].minsize) {
			aprint_error_dev(sc->sc_dev,
			    "kernel hmc entry is too big\n");
			return -1;
		}

		size += roundup(e->hmc_size * e->hmc_count, IXL_HMC_ROUNDUP);
	}
	size = roundup(size, IXL_HMC_PGSIZE);
	npages = size / IXL_HMC_PGSIZE;

	tables = roundup(size, IXL_HMC_L2SZ) / IXL_HMC_L2SZ;

	if (ixl_dmamem_alloc(sc, &sc->sc_hmc_pd, size, IXL_HMC_PGSIZE) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to allocate hmc pd memory\n");
		return -1;
	}

	if (ixl_dmamem_alloc(sc, &sc->sc_hmc_sd, tables * IXL_HMC_PGSIZE,
	    IXL_HMC_PGSIZE) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to allocate hmc sd memory\n");
		ixl_dmamem_free(sc, &sc->sc_hmc_pd);
		return -1;
	}

	kva = IXL_DMA_KVA(&sc->sc_hmc_pd);
	memset(kva, 0, IXL_DMA_LEN(&sc->sc_hmc_pd));

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_hmc_pd),
	    0, IXL_DMA_LEN(&sc->sc_hmc_pd),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	dva = IXL_DMA_DVA(&sc->sc_hmc_pd);
	sdpage = IXL_DMA_KVA(&sc->sc_hmc_sd);
	memset(sdpage, 0, IXL_DMA_LEN(&sc->sc_hmc_sd));

	for (i = 0; (int)i < npages; i++) {
		*sdpage = htole64(dva | IXL_HMC_PDVALID);
		sdpage++;

		dva += IXL_HMC_PGSIZE;
	}

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_hmc_sd),
	    0, IXL_DMA_LEN(&sc->sc_hmc_sd),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	dva = IXL_DMA_DVA(&sc->sc_hmc_sd);
	for (i = 0; (int)i < tables; i++) {
		uint32_t count;

		KASSERT(npages >= 0);

		count = ((unsigned int)npages > IXL_HMC_PGS) ?
		    IXL_HMC_PGS : (unsigned int)npages;

		ixl_wr(sc, I40E_PFHMC_SDDATAHIGH, dva >> 32);
		ixl_wr(sc, I40E_PFHMC_SDDATALOW, dva |
		    (count << I40E_PFHMC_SDDATALOW_PMSDBPCOUNT_SHIFT) |
		    (1U << I40E_PFHMC_SDDATALOW_PMSDVALID_SHIFT));
		ixl_barrier(sc, 0, sc->sc_mems, BUS_SPACE_BARRIER_WRITE);
		ixl_wr(sc, I40E_PFHMC_SDCMD,
		    (1U << I40E_PFHMC_SDCMD_PMSDWR_SHIFT) | i);

		npages -= IXL_HMC_PGS;
		dva += IXL_HMC_PGSIZE;
	}

	for (i = 0; i < __arraycount(regs); i++) {
		e = &sc->sc_hmc_entries[i];

		ixl_wr(sc, regs[i].setoff, e->hmc_base / IXL_HMC_ROUNDUP);
		ixl_wr(sc, regs[i].setcnt, e->hmc_count);
	}

	return 0;
}

static void
ixl_hmc_free(struct ixl_softc *sc)
{
	ixl_dmamem_free(sc, &sc->sc_hmc_sd);
	ixl_dmamem_free(sc, &sc->sc_hmc_pd);
}

static void
ixl_hmc_pack(void *d, const void *s, const struct ixl_hmc_pack *packing,
    unsigned int npacking)
{
	uint8_t *dst = d;
	const uint8_t *src = s;
	unsigned int i;

	for (i = 0; i < npacking; i++) {
		const struct ixl_hmc_pack *pack = &packing[i];
		unsigned int offset = pack->lsb / 8;
		unsigned int align = pack->lsb % 8;
		const uint8_t *in = src + pack->offset;
		uint8_t *out = dst + offset;
		int width = pack->width;
		unsigned int inbits = 0;

		if (align) {
			inbits = (*in++) << align;
			*out++ |= (inbits & 0xff);
			inbits >>= 8;

			width -= 8 - align;
		}

		while (width >= 8) {
			inbits |= (*in++) << align;
			*out++ = (inbits & 0xff);
			inbits >>= 8;

			width -= 8;
		}

		if (width > 0) {
			inbits |= (*in) << align;
			*out |= (inbits & ((1 << width) - 1));
		}
	}
}

static struct ixl_aq_buf *
ixl_aqb_alloc(struct ixl_softc *sc)
{
	struct ixl_aq_buf *aqb;

	aqb = kmem_alloc(sizeof(*aqb), KM_SLEEP);

	aqb->aqb_size = IXL_AQ_BUFLEN;

	if (bus_dmamap_create(sc->sc_dmat, aqb->aqb_size, 1,
	    aqb->aqb_size, 0,
	    BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW, &aqb->aqb_map) != 0)
		goto free;
	if (bus_dmamem_alloc(sc->sc_dmat, aqb->aqb_size,
	    IXL_AQ_ALIGN, 0, &aqb->aqb_seg, 1, &aqb->aqb_nsegs,
	    BUS_DMA_WAITOK) != 0)
		goto destroy;
	if (bus_dmamem_map(sc->sc_dmat, &aqb->aqb_seg, aqb->aqb_nsegs,
	    aqb->aqb_size, &aqb->aqb_data, BUS_DMA_WAITOK) != 0)
		goto dma_free;
	if (bus_dmamap_load(sc->sc_dmat, aqb->aqb_map, aqb->aqb_data,
	    aqb->aqb_size, NULL, BUS_DMA_WAITOK) != 0)
		goto unmap;

	return aqb;
unmap:
	bus_dmamem_unmap(sc->sc_dmat, aqb->aqb_data, aqb->aqb_size);
dma_free:
	bus_dmamem_free(sc->sc_dmat, &aqb->aqb_seg, 1);
destroy:
	bus_dmamap_destroy(sc->sc_dmat, aqb->aqb_map);
free:
	kmem_free(aqb, sizeof(*aqb));

	return NULL;
}

static void
ixl_aqb_free(struct ixl_softc *sc, struct ixl_aq_buf *aqb)
{

	bus_dmamap_unload(sc->sc_dmat, aqb->aqb_map);
	bus_dmamem_unmap(sc->sc_dmat, aqb->aqb_data, aqb->aqb_size);
	bus_dmamem_free(sc->sc_dmat, &aqb->aqb_seg, 1);
	bus_dmamap_destroy(sc->sc_dmat, aqb->aqb_map);
	kmem_free(aqb, sizeof(*aqb));
}

static int
ixl_arq_fill(struct ixl_softc *sc)
{
	struct ixl_aq_buf *aqb;
	struct ixl_aq_desc *arq, *iaq;
	unsigned int prod = sc->sc_arq_prod;
	unsigned int n;
	int post = 0;

	n = ixl_rxr_unrefreshed(sc->sc_arq_prod, sc->sc_arq_cons,
	    IXL_AQ_NUM);
	arq = IXL_DMA_KVA(&sc->sc_arq);

	if (__predict_false(n <= 0))
		return 0;

	do {
		aqb = sc->sc_arq_live[prod];
		iaq = &arq[prod];

		if (aqb == NULL) {
			aqb = SIMPLEQ_FIRST(&sc->sc_arq_idle);
			if (aqb != NULL) {
				SIMPLEQ_REMOVE(&sc->sc_arq_idle, aqb,
				    ixl_aq_buf, aqb_entry);
			} else if ((aqb = ixl_aqb_alloc(sc)) == NULL) {
				break;
			}

			sc->sc_arq_live[prod] = aqb;
			memset(aqb->aqb_data, 0, aqb->aqb_size);

			bus_dmamap_sync(sc->sc_dmat, aqb->aqb_map, 0,
			    aqb->aqb_size, BUS_DMASYNC_PREREAD);

			iaq->iaq_flags = htole16(IXL_AQ_BUF |
			    (IXL_AQ_BUFLEN > I40E_AQ_LARGE_BUF ?
			    IXL_AQ_LB : 0));
			iaq->iaq_opcode = 0;
			iaq->iaq_datalen = htole16(aqb->aqb_size);
			iaq->iaq_retval = 0;
			iaq->iaq_cookie = 0;
			iaq->iaq_param[0] = 0;
			iaq->iaq_param[1] = 0;
			ixl_aq_dva(iaq, aqb->aqb_map->dm_segs[0].ds_addr);
		}

		prod++;
		prod &= IXL_AQ_MASK;

		post = 1;

	} while (--n);

	if (post) {
		sc->sc_arq_prod = prod;
		ixl_wr(sc, sc->sc_aq_regs->arq_tail, sc->sc_arq_prod);
	}

	return post;
}

static void
ixl_arq_unfill(struct ixl_softc *sc)
{
	struct ixl_aq_buf *aqb;
	unsigned int i;

	for (i = 0; i < __arraycount(sc->sc_arq_live); i++) {
		aqb = sc->sc_arq_live[i];
		if (aqb == NULL)
			continue;

		sc->sc_arq_live[i] = NULL;
		bus_dmamap_sync(sc->sc_dmat, aqb->aqb_map, 0, aqb->aqb_size,
		    BUS_DMASYNC_POSTREAD);
		ixl_aqb_free(sc, aqb);
	}

	while ((aqb = SIMPLEQ_FIRST(&sc->sc_arq_idle)) != NULL) {
		SIMPLEQ_REMOVE(&sc->sc_arq_idle, aqb,
		    ixl_aq_buf, aqb_entry);
		ixl_aqb_free(sc, aqb);
	}
}

static void
ixl_clear_hw(struct ixl_softc *sc)
{
	uint32_t num_queues, base_queue;
	uint32_t num_pf_int;
	uint32_t num_vf_int;
	uint32_t num_vfs;
	uint32_t i, j;
	uint32_t val;
	uint32_t eol = 0x7ff;

	/* get number of interrupts, queues, and vfs */
	val = ixl_rd(sc, I40E_GLPCI_CNF2);
	num_pf_int = (val & I40E_GLPCI_CNF2_MSI_X_PF_N_MASK) >>
	    I40E_GLPCI_CNF2_MSI_X_PF_N_SHIFT;
	num_vf_int = (val & I40E_GLPCI_CNF2_MSI_X_VF_N_MASK) >>
	    I40E_GLPCI_CNF2_MSI_X_VF_N_SHIFT;

	val = ixl_rd(sc, I40E_PFLAN_QALLOC);
	base_queue = (val & I40E_PFLAN_QALLOC_FIRSTQ_MASK) >>
	    I40E_PFLAN_QALLOC_FIRSTQ_SHIFT;
	j = (val & I40E_PFLAN_QALLOC_LASTQ_MASK) >>
	    I40E_PFLAN_QALLOC_LASTQ_SHIFT;
	if (val & I40E_PFLAN_QALLOC_VALID_MASK)
		num_queues = (j - base_queue) + 1;
	else
		num_queues = 0;

	val = ixl_rd(sc, I40E_PF_VT_PFALLOC);
	i = (val & I40E_PF_VT_PFALLOC_FIRSTVF_MASK) >>
	    I40E_PF_VT_PFALLOC_FIRSTVF_SHIFT;
	j = (val & I40E_PF_VT_PFALLOC_LASTVF_MASK) >>
	    I40E_PF_VT_PFALLOC_LASTVF_SHIFT;
	if (val & I40E_PF_VT_PFALLOC_VALID_MASK)
		num_vfs = (j - i) + 1;
	else
		num_vfs = 0;

	/* stop all the interrupts */
	ixl_wr(sc, I40E_PFINT_ICR0_ENA, 0);
	ixl_flush(sc);
	val = 0x3 << I40E_PFINT_DYN_CTLN_ITR_INDX_SHIFT;
	for (i = 0; i < num_pf_int - 2; i++)
		ixl_wr(sc, I40E_PFINT_DYN_CTLN(i), val);
	ixl_flush(sc);

	/* Set the FIRSTQ_INDX field to 0x7FF in PFINT_LNKLSTx */
	val = eol << I40E_PFINT_LNKLST0_FIRSTQ_INDX_SHIFT;
	ixl_wr(sc, I40E_PFINT_LNKLST0, val);
	for (i = 0; i < num_pf_int - 2; i++)
		ixl_wr(sc, I40E_PFINT_LNKLSTN(i), val);
	val = eol << I40E_VPINT_LNKLST0_FIRSTQ_INDX_SHIFT;
	for (i = 0; i < num_vfs; i++)
		ixl_wr(sc, I40E_VPINT_LNKLST0(i), val);
	for (i = 0; i < num_vf_int - 2; i++)
		ixl_wr(sc, I40E_VPINT_LNKLSTN(i), val);

	/* warn the HW of the coming Tx disables */
	for (i = 0; i < num_queues; i++) {
		uint32_t abs_queue_idx = base_queue + i;
		uint32_t reg_block = 0;

		if (abs_queue_idx >= 128) {
			reg_block = abs_queue_idx / 128;
			abs_queue_idx %= 128;
		}

		val = ixl_rd(sc, I40E_GLLAN_TXPRE_QDIS(reg_block));
		val &= ~I40E_GLLAN_TXPRE_QDIS_QINDX_MASK;
		val |= (abs_queue_idx << I40E_GLLAN_TXPRE_QDIS_QINDX_SHIFT);
		val |= I40E_GLLAN_TXPRE_QDIS_SET_QDIS_MASK;

		ixl_wr(sc, I40E_GLLAN_TXPRE_QDIS(reg_block), val);
	}
	delaymsec(400);

	/* stop all the queues */
	for (i = 0; i < num_queues; i++) {
		ixl_wr(sc, I40E_QINT_TQCTL(i), 0);
		ixl_wr(sc, I40E_QTX_ENA(i), 0);
		ixl_wr(sc, I40E_QINT_RQCTL(i), 0);
		ixl_wr(sc, I40E_QRX_ENA(i), 0);
	}

	/* short wait for all queue disables to settle */
	delaymsec(50);
}

static int
ixl_pf_reset(struct ixl_softc *sc)
{
	uint32_t cnt = 0;
	uint32_t cnt1 = 0;
	uint32_t reg = 0, reg0 = 0;
	uint32_t grst_del;

	/*
	 * Poll for Global Reset steady state in case of recent GRST.
	 * The grst delay value is in 100ms units, and we'll wait a
	 * couple counts longer to be sure we don't just miss the end.
	 */
	grst_del = ixl_rd(sc, I40E_GLGEN_RSTCTL);
	grst_del &= I40E_GLGEN_RSTCTL_GRSTDEL_MASK;
	grst_del >>= I40E_GLGEN_RSTCTL_GRSTDEL_SHIFT;

	grst_del = grst_del * 20;

	for (cnt = 0; cnt < grst_del; cnt++) {
		reg = ixl_rd(sc, I40E_GLGEN_RSTAT);
		if (!(reg & I40E_GLGEN_RSTAT_DEVSTATE_MASK))
			break;
		delaymsec(100);
	}
	if (reg & I40E_GLGEN_RSTAT_DEVSTATE_MASK) {
		aprint_error(", Global reset polling failed to complete\n");
		return -1;
	}

	/* Now Wait for the FW to be ready */
	for (cnt1 = 0; cnt1 < I40E_PF_RESET_WAIT_COUNT; cnt1++) {
		reg = ixl_rd(sc, I40E_GLNVM_ULD);
		reg &= (I40E_GLNVM_ULD_CONF_CORE_DONE_MASK |
		    I40E_GLNVM_ULD_CONF_GLOBAL_DONE_MASK);
		if (reg == (I40E_GLNVM_ULD_CONF_CORE_DONE_MASK |
		    I40E_GLNVM_ULD_CONF_GLOBAL_DONE_MASK))
			break;

		delaymsec(10);
	}
	if (!(reg & (I40E_GLNVM_ULD_CONF_CORE_DONE_MASK |
	    I40E_GLNVM_ULD_CONF_GLOBAL_DONE_MASK))) {
		aprint_error(", wait for FW Reset complete timed out "
		    "(I40E_GLNVM_ULD = 0x%x)\n", reg);
		return -1;
	}

	/*
	 * If there was a Global Reset in progress when we got here,
	 * we don't need to do the PF Reset
	 */
	if (cnt == 0) {
		reg = ixl_rd(sc, I40E_PFGEN_CTRL);
		ixl_wr(sc, I40E_PFGEN_CTRL, reg | I40E_PFGEN_CTRL_PFSWR_MASK);
		for (cnt = 0; cnt < I40E_PF_RESET_WAIT_COUNT; cnt++) {
			reg = ixl_rd(sc, I40E_PFGEN_CTRL);
			if (!(reg & I40E_PFGEN_CTRL_PFSWR_MASK))
				break;
			delaymsec(1);

			reg0 = ixl_rd(sc, I40E_GLGEN_RSTAT);
			if (reg0 & I40E_GLGEN_RSTAT_DEVSTATE_MASK) {
				aprint_error(", Core reset upcoming."
				    " Skipping PF reset reset request\n");
				return -1;
			}
		}
		if (reg & I40E_PFGEN_CTRL_PFSWR_MASK) {
			aprint_error(", PF reset polling failed to complete"
			    "(I40E_PFGEN_CTRL= 0x%x)\n", reg);
			return -1;
		}
	}

	return 0;
}

static int
ixl_dmamem_alloc(struct ixl_softc *sc, struct ixl_dmamem *ixm,
    bus_size_t size, bus_size_t align)
{
	ixm->ixm_size = size;

	if (bus_dmamap_create(sc->sc_dmat, ixm->ixm_size, 1,
	    ixm->ixm_size, 0,
	    BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW,
	    &ixm->ixm_map) != 0)
		return 1;
	if (bus_dmamem_alloc(sc->sc_dmat, ixm->ixm_size,
	    align, 0, &ixm->ixm_seg, 1, &ixm->ixm_nsegs,
	    BUS_DMA_WAITOK) != 0)
		goto destroy;
	if (bus_dmamem_map(sc->sc_dmat, &ixm->ixm_seg, ixm->ixm_nsegs,
	    ixm->ixm_size, &ixm->ixm_kva, BUS_DMA_WAITOK) != 0)
		goto free;
	if (bus_dmamap_load(sc->sc_dmat, ixm->ixm_map, ixm->ixm_kva,
	    ixm->ixm_size, NULL, BUS_DMA_WAITOK) != 0)
		goto unmap;

	memset(ixm->ixm_kva, 0, ixm->ixm_size);

	return 0;
unmap:
	bus_dmamem_unmap(sc->sc_dmat, ixm->ixm_kva, ixm->ixm_size);
free:
	bus_dmamem_free(sc->sc_dmat, &ixm->ixm_seg, 1);
destroy:
	bus_dmamap_destroy(sc->sc_dmat, ixm->ixm_map);
	return 1;
}

static void
ixl_dmamem_free(struct ixl_softc *sc, struct ixl_dmamem *ixm)
{
	bus_dmamap_unload(sc->sc_dmat, ixm->ixm_map);
	bus_dmamem_unmap(sc->sc_dmat, ixm->ixm_kva, ixm->ixm_size);
	bus_dmamem_free(sc->sc_dmat, &ixm->ixm_seg, 1);
	bus_dmamap_destroy(sc->sc_dmat, ixm->ixm_map);
}

static int
ixl_setup_vlan_hwfilter(struct ixl_softc *sc)
{
	struct ethercom *ec = &sc->sc_ec;
	struct vlanid_list *vlanidp;
	int rv;

	ixl_remove_macvlan(sc, sc->sc_enaddr, 0,
	    IXL_AQ_OP_REMOVE_MACVLAN_IGNORE_VLAN);
	ixl_remove_macvlan(sc, etherbroadcastaddr, 0,
	    IXL_AQ_OP_REMOVE_MACVLAN_IGNORE_VLAN);

	rv = ixl_add_macvlan(sc, sc->sc_enaddr, 0,
	    IXL_AQ_OP_ADD_MACVLAN_PERFECT_MATCH);
	if (rv != 0)
		return rv;
	rv = ixl_add_macvlan(sc, etherbroadcastaddr, 0,
	    IXL_AQ_OP_ADD_MACVLAN_PERFECT_MATCH);
	if (rv != 0)
		return rv;

	ETHER_LOCK(ec);
	SIMPLEQ_FOREACH(vlanidp, &ec->ec_vids, vid_list) {
		rv = ixl_add_macvlan(sc, sc->sc_enaddr,
		    vlanidp->vid, IXL_AQ_OP_ADD_MACVLAN_PERFECT_MATCH);
		if (rv != 0)
			break;
		rv = ixl_add_macvlan(sc, etherbroadcastaddr,
		    vlanidp->vid, IXL_AQ_OP_ADD_MACVLAN_PERFECT_MATCH);
		if (rv != 0)
			break;
	}
	ETHER_UNLOCK(ec);

	return rv;
}

static void
ixl_teardown_vlan_hwfilter(struct ixl_softc *sc)
{
	struct vlanid_list *vlanidp;
	struct ethercom *ec = &sc->sc_ec;

	ixl_remove_macvlan(sc, sc->sc_enaddr, 0,
	    IXL_AQ_OP_REMOVE_MACVLAN_PERFECT_MATCH);
	ixl_remove_macvlan(sc, etherbroadcastaddr, 0,
	    IXL_AQ_OP_REMOVE_MACVLAN_PERFECT_MATCH);

	ETHER_LOCK(ec);
	SIMPLEQ_FOREACH(vlanidp, &ec->ec_vids, vid_list) {
		ixl_remove_macvlan(sc, sc->sc_enaddr,
		    vlanidp->vid, IXL_AQ_OP_REMOVE_MACVLAN_PERFECT_MATCH);
		ixl_remove_macvlan(sc, etherbroadcastaddr,
		    vlanidp->vid, IXL_AQ_OP_REMOVE_MACVLAN_PERFECT_MATCH);
	}
	ETHER_UNLOCK(ec);

	ixl_add_macvlan(sc, sc->sc_enaddr, 0,
	    IXL_AQ_OP_ADD_MACVLAN_IGNORE_VLAN);
	ixl_add_macvlan(sc, etherbroadcastaddr, 0,
	    IXL_AQ_OP_ADD_MACVLAN_IGNORE_VLAN);
}

static int
ixl_update_macvlan(struct ixl_softc *sc)
{
	int rv = 0;
	int next_ec_capenable = sc->sc_ec.ec_capenable;

	if (ISSET(next_ec_capenable, ETHERCAP_VLAN_HWFILTER)) {
		rv = ixl_setup_vlan_hwfilter(sc);
		if (rv != 0)
			ixl_teardown_vlan_hwfilter(sc);
	} else {
		ixl_teardown_vlan_hwfilter(sc);
	}

	return rv;
}

static int
ixl_ifflags_cb(struct ethercom *ec)
{
	struct ifnet *ifp = &ec->ec_if;
	struct ixl_softc *sc = ifp->if_softc;
	int rv, change;

	mutex_enter(&sc->sc_cfg_lock);

	change = ec->ec_capenable ^ sc->sc_cur_ec_capenable;

	if (ISSET(change, ETHERCAP_VLAN_HWTAGGING)) {
		sc->sc_cur_ec_capenable ^= ETHERCAP_VLAN_HWTAGGING;
		rv = ENETRESET;
		goto out;
	}

	if (ISSET(change, ETHERCAP_VLAN_HWFILTER)) {
		rv = ixl_update_macvlan(sc);
		if (rv == 0) {
			sc->sc_cur_ec_capenable ^= ETHERCAP_VLAN_HWFILTER;
		} else {
			CLR(ec->ec_capenable, ETHERCAP_VLAN_HWFILTER);
			CLR(sc->sc_cur_ec_capenable, ETHERCAP_VLAN_HWFILTER);
		}
	}

	rv = ixl_iff(sc);
out:
	mutex_exit(&sc->sc_cfg_lock);

	return rv;
}

static int
ixl_set_link_status_locked(struct ixl_softc *sc, const struct ixl_aq_desc *iaq)
{
	const struct ixl_aq_link_status *status;
	const struct ixl_phy_type *itype;

	uint64_t ifm_active = IFM_ETHER;
	uint64_t ifm_status = IFM_AVALID;
	int link_state = LINK_STATE_DOWN;
	uint64_t baudrate = 0;

	status = (const struct ixl_aq_link_status *)iaq->iaq_param;
	if (!ISSET(status->link_info, IXL_AQ_LINK_UP_FUNCTION)) {
		ifm_active |= IFM_NONE;
		goto done;
	}

	ifm_active |= IFM_FDX;
	ifm_status |= IFM_ACTIVE;
	link_state = LINK_STATE_UP;

	itype = ixl_search_phy_type(status->phy_type);
	if (itype != NULL)
		ifm_active |= itype->ifm_type;

	if (ISSET(status->an_info, IXL_AQ_LINK_PAUSE_TX))
		ifm_active |= IFM_ETH_TXPAUSE;
	if (ISSET(status->an_info, IXL_AQ_LINK_PAUSE_RX))
		ifm_active |= IFM_ETH_RXPAUSE;

	baudrate = ixl_search_link_speed(status->link_speed);

done:
	/* sc->sc_cfg_lock held expect during attach */
	sc->sc_media_active = ifm_active;
	sc->sc_media_status = ifm_status;

	sc->sc_ec.ec_if.if_baudrate = baudrate;

	return link_state;
}

static int
ixl_establish_intx(struct ixl_softc *sc)
{
	pci_chipset_tag_t pc = sc->sc_pa.pa_pc;
	pci_intr_handle_t *intr;
	char xnamebuf[32];
	char intrbuf[PCI_INTRSTR_LEN];
	char const *intrstr;

	KASSERT(sc->sc_nintrs == 1);

	intr = &sc->sc_ihp[0];

	intrstr = pci_intr_string(pc, *intr, intrbuf, sizeof(intrbuf));
	snprintf(xnamebuf, sizeof(xnamebuf), "%s:legacy",
	    device_xname(sc->sc_dev));

	sc->sc_ihs[0] = pci_intr_establish_xname(pc, *intr, IPL_NET, ixl_intr,
	    sc, xnamebuf);

	if (sc->sc_ihs[0] == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "unable to establish interrupt at %s\n", intrstr);
		return -1;
	}

	aprint_normal_dev(sc->sc_dev, "interrupting at %s\n", intrstr);
	return 0;
}

static int
ixl_establish_msix(struct ixl_softc *sc)
{
	pci_chipset_tag_t pc = sc->sc_pa.pa_pc;
	kcpuset_t *affinity;
	unsigned int vector = 0;
	unsigned int i;
	int affinity_to, r;
	char xnamebuf[32];
	char intrbuf[PCI_INTRSTR_LEN];
	char const *intrstr;

	kcpuset_create(&affinity, false);

	/* the "other" intr is mapped to vector 0 */
	vector = 0;
	intrstr = pci_intr_string(pc, sc->sc_ihp[vector],
	    intrbuf, sizeof(intrbuf));
	snprintf(xnamebuf, sizeof(xnamebuf), "%s others",
	    device_xname(sc->sc_dev));
	sc->sc_ihs[vector] = pci_intr_establish_xname(pc,
	    sc->sc_ihp[vector], IPL_NET, ixl_other_intr,
	    sc, xnamebuf);
	if (sc->sc_ihs[vector] == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "unable to establish interrupt at %s\n", intrstr);
		goto fail;
	}

	aprint_normal_dev(sc->sc_dev, "other interrupt at %s", intrstr);

	affinity_to = ncpu > (int)sc->sc_nqueue_pairs_max ? 1 : 0;
	affinity_to = (affinity_to + sc->sc_nqueue_pairs_max) % ncpu;

	kcpuset_zero(affinity);
	kcpuset_set(affinity, affinity_to);
	r = interrupt_distribute(sc->sc_ihs[vector], affinity, NULL);
	if (r == 0) {
		aprint_normal(", affinity to %u", affinity_to);
	}
	aprint_normal("\n");
	vector++;

	sc->sc_msix_vector_queue = vector;
	affinity_to = ncpu > (int)sc->sc_nqueue_pairs_max ? 1 : 0;

	for (i = 0; i < sc->sc_nqueue_pairs_max; i++) {
		intrstr = pci_intr_string(pc, sc->sc_ihp[vector],
		    intrbuf, sizeof(intrbuf));
		snprintf(xnamebuf, sizeof(xnamebuf), "%s TXRX%d",
		    device_xname(sc->sc_dev), i);

		sc->sc_ihs[vector] = pci_intr_establish_xname(pc,
		    sc->sc_ihp[vector], IPL_NET, ixl_queue_intr,
		    (void *)&sc->sc_qps[i], xnamebuf);

		if (sc->sc_ihs[vector] == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "unable to establish interrupt at %s\n", intrstr);
			goto fail;
		}

		aprint_normal_dev(sc->sc_dev,
		    "for TXRX%d interrupt at %s", i, intrstr);

		kcpuset_zero(affinity);
		kcpuset_set(affinity, affinity_to);
		r = interrupt_distribute(sc->sc_ihs[vector], affinity, NULL);
		if (r == 0) {
			aprint_normal(", affinity to %u", affinity_to);
			affinity_to = (affinity_to + 1) % ncpu;
		}
		aprint_normal("\n");
		vector++;
	}

	kcpuset_destroy(affinity);

	return 0;
fail:
	for (i = 0; i < vector; i++) {
		pci_intr_disestablish(pc, sc->sc_ihs[i]);
	}

	sc->sc_msix_vector_queue = 0;
	sc->sc_msix_vector_queue = 0;
	kcpuset_destroy(affinity);

	return -1;
}

static void
ixl_config_queue_intr(struct ixl_softc *sc)
{
	unsigned int i, vector;

	if (sc->sc_intrtype == PCI_INTR_TYPE_MSIX) {
		vector = sc->sc_msix_vector_queue;
	} else {
		vector = I40E_INTR_NOTX_INTR;

		ixl_wr(sc, I40E_PFINT_LNKLST0,
		    (I40E_INTR_NOTX_QUEUE <<
		     I40E_PFINT_LNKLST0_FIRSTQ_INDX_SHIFT) |
		    (I40E_QUEUE_TYPE_RX <<
		     I40E_PFINT_LNKLSTN_FIRSTQ_TYPE_SHIFT));
	}

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		ixl_wr(sc, I40E_PFINT_DYN_CTLN(i), 0);
		ixl_flush(sc);

		ixl_wr(sc, I40E_PFINT_LNKLSTN(i),
		    ((i) << I40E_PFINT_LNKLSTN_FIRSTQ_INDX_SHIFT) |
		    (I40E_QUEUE_TYPE_RX <<
		     I40E_PFINT_LNKLSTN_FIRSTQ_TYPE_SHIFT));

		ixl_wr(sc, I40E_QINT_RQCTL(i),
		    (vector << I40E_QINT_RQCTL_MSIX_INDX_SHIFT) |
		    (I40E_ITR_INDEX_RX <<
		     I40E_QINT_RQCTL_ITR_INDX_SHIFT) |
		    (I40E_INTR_NOTX_RX_QUEUE <<
		     I40E_QINT_RQCTL_MSIX0_INDX_SHIFT) |
		    (i << I40E_QINT_RQCTL_NEXTQ_INDX_SHIFT) |
		    (I40E_QUEUE_TYPE_TX <<
		     I40E_QINT_RQCTL_NEXTQ_TYPE_SHIFT) |
		    I40E_QINT_RQCTL_CAUSE_ENA_MASK);

		ixl_wr(sc, I40E_QINT_TQCTL(i),
		    (vector << I40E_QINT_TQCTL_MSIX_INDX_SHIFT) |
		    (I40E_ITR_INDEX_TX <<
		     I40E_QINT_TQCTL_ITR_INDX_SHIFT) |
		    (I40E_INTR_NOTX_TX_QUEUE <<
		     I40E_QINT_TQCTL_MSIX0_INDX_SHIFT) |
		    (I40E_QUEUE_TYPE_EOL <<
		     I40E_QINT_TQCTL_NEXTQ_INDX_SHIFT) |
		    (I40E_QUEUE_TYPE_RX <<
		     I40E_QINT_TQCTL_NEXTQ_TYPE_SHIFT) |
		     I40E_QINT_TQCTL_CAUSE_ENA_MASK);

		if (sc->sc_intrtype == PCI_INTR_TYPE_MSIX) {
			ixl_wr(sc, I40E_PFINT_ITRN(I40E_ITR_INDEX_RX, i),
			    sc->sc_itr_rx);
			ixl_wr(sc, I40E_PFINT_ITRN(I40E_ITR_INDEX_TX, i),
			    sc->sc_itr_tx);
			vector++;
		}
	}
	ixl_flush(sc);

	ixl_wr(sc, I40E_PFINT_ITR0(I40E_ITR_INDEX_RX), sc->sc_itr_rx);
	ixl_wr(sc, I40E_PFINT_ITR0(I40E_ITR_INDEX_TX), sc->sc_itr_tx);
	ixl_flush(sc);
}

static void
ixl_config_other_intr(struct ixl_softc *sc)
{
	ixl_wr(sc, I40E_PFINT_ICR0_ENA, 0);
	(void)ixl_rd(sc, I40E_PFINT_ICR0);

	ixl_wr(sc, I40E_PFINT_ICR0_ENA,
	    I40E_PFINT_ICR0_ENA_ECC_ERR_MASK |
	    I40E_PFINT_ICR0_ENA_GRST_MASK |
	    I40E_PFINT_ICR0_ENA_ADMINQ_MASK |
	    I40E_PFINT_ICR0_ENA_MAL_DETECT_MASK |
	    I40E_PFINT_ICR0_ENA_HMC_ERR_MASK |
	    I40E_PFINT_ICR0_ENA_VFLR_MASK |
	    I40E_PFINT_ICR0_ENA_PE_CRITERR_MASK |
	    I40E_PFINT_ICR0_ENA_PCI_EXCEPTION_MASK |
	    I40E_PFINT_ICR0_ENA_LINK_STAT_CHANGE_MASK);

	ixl_wr(sc, I40E_PFINT_LNKLST0, 0x7FF);
	ixl_wr(sc, I40E_PFINT_ITR0(I40E_ITR_INDEX_OTHER), 0);
	ixl_wr(sc, I40E_PFINT_STAT_CTL0,
	    (I40E_ITR_INDEX_OTHER <<
	     I40E_PFINT_STAT_CTL0_OTHER_ITR_INDX_SHIFT));
	ixl_flush(sc);
}

static int
ixl_setup_interrupts(struct ixl_softc *sc)
{
	struct pci_attach_args *pa = &sc->sc_pa;
	pci_intr_type_t max_type, intr_type;
	int counts[PCI_INTR_TYPE_SIZE];
	int error;
	unsigned int i;
	bool retry;

	memset(counts, 0, sizeof(counts));
	max_type = PCI_INTR_TYPE_MSIX;
	/* QPs + other interrupt */
	counts[PCI_INTR_TYPE_MSIX] = sc->sc_nqueue_pairs_max + 1;
	counts[PCI_INTR_TYPE_INTX] = 1;

	if (ixl_param_nomsix)
		counts[PCI_INTR_TYPE_MSIX] = 0;

	do {
		retry = false;
		error = pci_intr_alloc(pa, &sc->sc_ihp, counts, max_type);
		if (error != 0) {
			aprint_error_dev(sc->sc_dev,
			    "couldn't map interrupt\n");
			break;
		}

		intr_type = pci_intr_type(pa->pa_pc, sc->sc_ihp[0]);
		sc->sc_nintrs = counts[intr_type];
		KASSERT(sc->sc_nintrs > 0);

		for (i = 0; i < sc->sc_nintrs; i++) {
			pci_intr_setattr(pa->pa_pc, &sc->sc_ihp[i],
			    PCI_INTR_MPSAFE, true);
		}

		sc->sc_ihs = kmem_zalloc(sizeof(sc->sc_ihs[0]) * sc->sc_nintrs,
		    KM_SLEEP);

		if (intr_type == PCI_INTR_TYPE_MSIX) {
			error = ixl_establish_msix(sc);
			if (error) {
				counts[PCI_INTR_TYPE_MSIX] = 0;
				retry = true;
			}
		} else if (intr_type == PCI_INTR_TYPE_INTX) {
			error = ixl_establish_intx(sc);
		} else {
			error = -1;
		}

		if (error) {
			kmem_free(sc->sc_ihs,
			    sizeof(sc->sc_ihs[0]) * sc->sc_nintrs);
			pci_intr_release(pa->pa_pc, sc->sc_ihp, sc->sc_nintrs);
		} else {
			sc->sc_intrtype = intr_type;
		}
	} while (retry);

	return error;
}

static void
ixl_teardown_interrupts(struct ixl_softc *sc)
{
	struct pci_attach_args *pa = &sc->sc_pa;
	unsigned int i;

	for (i = 0; i < sc->sc_nintrs; i++) {
		pci_intr_disestablish(pa->pa_pc, sc->sc_ihs[i]);
	}

	pci_intr_release(pa->pa_pc, sc->sc_ihp, sc->sc_nintrs);

	kmem_free(sc->sc_ihs, sizeof(sc->sc_ihs[0]) * sc->sc_nintrs);
	sc->sc_ihs = NULL;
	sc->sc_nintrs = 0;
}

static int
ixl_setup_stats(struct ixl_softc *sc)
{
	struct ixl_queue_pair *qp;
	struct ixl_tx_ring *txr;
	struct ixl_rx_ring *rxr;
	struct ixl_stats_counters *isc;
	unsigned int i;

	for (i = 0; i < sc->sc_nqueue_pairs_max; i++) {
		qp = &sc->sc_qps[i];
		txr = qp->qp_txr;
		rxr = qp->qp_rxr;

		evcnt_attach_dynamic(&txr->txr_defragged, EVCNT_TYPE_MISC,
		    NULL, qp->qp_name, "m_defrag successed");
		evcnt_attach_dynamic(&txr->txr_defrag_failed, EVCNT_TYPE_MISC,
		    NULL, qp->qp_name, "m_defrag_failed");
		evcnt_attach_dynamic(&txr->txr_pcqdrop, EVCNT_TYPE_MISC,
		    NULL, qp->qp_name, "Dropped in pcq");
		evcnt_attach_dynamic(&txr->txr_transmitdef, EVCNT_TYPE_MISC,
		    NULL, qp->qp_name, "Deferred transmit");
		evcnt_attach_dynamic(&txr->txr_intr, EVCNT_TYPE_INTR,
		    NULL, qp->qp_name, "Interrupt on queue");
		evcnt_attach_dynamic(&txr->txr_defer, EVCNT_TYPE_MISC,
		    NULL, qp->qp_name, "Handled queue in softint/workqueue");

		evcnt_attach_dynamic(&rxr->rxr_mgethdr_failed, EVCNT_TYPE_MISC,
		    NULL, qp->qp_name, "MGETHDR failed");
		evcnt_attach_dynamic(&rxr->rxr_mgetcl_failed, EVCNT_TYPE_MISC,
		    NULL, qp->qp_name, "MCLGET failed");
		evcnt_attach_dynamic(&rxr->rxr_mbuf_load_failed,
		    EVCNT_TYPE_MISC, NULL, qp->qp_name,
		    "bus_dmamap_load_mbuf failed");
		evcnt_attach_dynamic(&rxr->rxr_intr, EVCNT_TYPE_INTR,
		    NULL, qp->qp_name, "Interrupt on queue");
		evcnt_attach_dynamic(&rxr->rxr_defer, EVCNT_TYPE_MISC,
		    NULL, qp->qp_name, "Handled queue in softint/workqueue");
	}

	evcnt_attach_dynamic(&sc->sc_event_atq, EVCNT_TYPE_INTR,
	    NULL, device_xname(sc->sc_dev), "Interrupt for other events");
	evcnt_attach_dynamic(&sc->sc_event_link, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Link status event");
	evcnt_attach_dynamic(&sc->sc_event_ecc_err, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "ECC error");
	evcnt_attach_dynamic(&sc->sc_event_pci_exception, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "PCI exception");
	evcnt_attach_dynamic(&sc->sc_event_crit_err, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Critical error");

	isc = &sc->sc_stats_counters;
	evcnt_attach_dynamic(&isc->isc_crc_errors, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "CRC errors");
	evcnt_attach_dynamic(&isc->isc_illegal_bytes, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Illegal bytes");
	evcnt_attach_dynamic(&isc->isc_mac_local_faults, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Mac local faults");
	evcnt_attach_dynamic(&isc->isc_mac_remote_faults, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Mac remote faults");
	evcnt_attach_dynamic(&isc->isc_link_xon_rx, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx xon");
	evcnt_attach_dynamic(&isc->isc_link_xon_tx, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx xon");
	evcnt_attach_dynamic(&isc->isc_link_xoff_rx, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx xoff");
	evcnt_attach_dynamic(&isc->isc_link_xoff_tx, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx xoff");
	evcnt_attach_dynamic(&isc->isc_rx_fragments, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx fragments");
	evcnt_attach_dynamic(&isc->isc_rx_jabber, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx jabber");

	evcnt_attach_dynamic(&isc->isc_rx_size_64, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx size 64");
	evcnt_attach_dynamic(&isc->isc_rx_size_127, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx size 127");
	evcnt_attach_dynamic(&isc->isc_rx_size_255, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx size 255");
	evcnt_attach_dynamic(&isc->isc_rx_size_511, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx size 511");
	evcnt_attach_dynamic(&isc->isc_rx_size_1023, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx size 1023");
	evcnt_attach_dynamic(&isc->isc_rx_size_1522, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx size 1522");
	evcnt_attach_dynamic(&isc->isc_rx_size_big, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx jumbo packets");
	evcnt_attach_dynamic(&isc->isc_rx_undersize, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx under size");
	evcnt_attach_dynamic(&isc->isc_rx_oversize, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx over size");

	evcnt_attach_dynamic(&isc->isc_rx_bytes, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx bytes / port");
	evcnt_attach_dynamic(&isc->isc_rx_discards, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx discards / port");
	evcnt_attach_dynamic(&isc->isc_rx_unicast, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx unicast / port");
	evcnt_attach_dynamic(&isc->isc_rx_multicast, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx multicast / port");
	evcnt_attach_dynamic(&isc->isc_rx_broadcast, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx broadcast / port");

	evcnt_attach_dynamic(&isc->isc_vsi_rx_bytes, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx bytes / vsi");
	evcnt_attach_dynamic(&isc->isc_vsi_rx_discards, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx discard / vsi");
	evcnt_attach_dynamic(&isc->isc_vsi_rx_unicast, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx unicast / vsi");
	evcnt_attach_dynamic(&isc->isc_vsi_rx_multicast, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx multicast / vsi");
	evcnt_attach_dynamic(&isc->isc_vsi_rx_broadcast, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Rx broadcast / vsi");

	evcnt_attach_dynamic(&isc->isc_tx_size_64, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx size 64");
	evcnt_attach_dynamic(&isc->isc_tx_size_127, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx size 127");
	evcnt_attach_dynamic(&isc->isc_tx_size_255, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx size 255");
	evcnt_attach_dynamic(&isc->isc_tx_size_511, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx size 511");
	evcnt_attach_dynamic(&isc->isc_tx_size_1023, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx size 1023");
	evcnt_attach_dynamic(&isc->isc_tx_size_1522, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx size 1522");
	evcnt_attach_dynamic(&isc->isc_tx_size_big, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx jumbo packets");

	evcnt_attach_dynamic(&isc->isc_tx_bytes, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx bytes / port");
	evcnt_attach_dynamic(&isc->isc_tx_dropped_link_down, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev),
	    "Tx dropped due to link down / port");
	evcnt_attach_dynamic(&isc->isc_tx_unicast, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx unicast / port");
	evcnt_attach_dynamic(&isc->isc_tx_multicast, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx multicast / port");
	evcnt_attach_dynamic(&isc->isc_tx_broadcast, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx broadcast / port");

	evcnt_attach_dynamic(&isc->isc_vsi_tx_bytes, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx bytes / vsi");
	evcnt_attach_dynamic(&isc->isc_vsi_tx_errors, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx errors / vsi");
	evcnt_attach_dynamic(&isc->isc_vsi_tx_unicast, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx unicast / vsi");
	evcnt_attach_dynamic(&isc->isc_vsi_tx_multicast, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx multicast / vsi");
	evcnt_attach_dynamic(&isc->isc_vsi_tx_broadcast, EVCNT_TYPE_MISC,
	    NULL, device_xname(sc->sc_dev), "Tx broadcast / vsi");

	sc->sc_stats_intval = ixl_param_stats_interval;
	callout_init(&sc->sc_stats_callout, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_stats_callout, ixl_stats_callout, sc);
	ixl_work_set(&sc->sc_stats_task, ixl_stats_update, sc);

	return 0;
}

static void
ixl_teardown_stats(struct ixl_softc *sc)
{
	struct ixl_tx_ring *txr;
	struct ixl_rx_ring *rxr;
	struct ixl_stats_counters *isc;
	unsigned int i;

	for (i = 0; i < sc->sc_nqueue_pairs_max; i++) {
		txr = sc->sc_qps[i].qp_txr;
		rxr = sc->sc_qps[i].qp_rxr;

		evcnt_detach(&txr->txr_defragged);
		evcnt_detach(&txr->txr_defrag_failed);
		evcnt_detach(&txr->txr_pcqdrop);
		evcnt_detach(&txr->txr_transmitdef);
		evcnt_detach(&txr->txr_intr);
		evcnt_detach(&txr->txr_defer);

		evcnt_detach(&rxr->rxr_mgethdr_failed);
		evcnt_detach(&rxr->rxr_mgetcl_failed);
		evcnt_detach(&rxr->rxr_mbuf_load_failed);
		evcnt_detach(&rxr->rxr_intr);
		evcnt_detach(&rxr->rxr_defer);
	}

	isc = &sc->sc_stats_counters;
	evcnt_detach(&isc->isc_crc_errors);
	evcnt_detach(&isc->isc_illegal_bytes);
	evcnt_detach(&isc->isc_mac_local_faults);
	evcnt_detach(&isc->isc_mac_remote_faults);
	evcnt_detach(&isc->isc_link_xon_rx);
	evcnt_detach(&isc->isc_link_xon_tx);
	evcnt_detach(&isc->isc_link_xoff_rx);
	evcnt_detach(&isc->isc_link_xoff_tx);
	evcnt_detach(&isc->isc_rx_fragments);
	evcnt_detach(&isc->isc_rx_jabber);
	evcnt_detach(&isc->isc_rx_bytes);
	evcnt_detach(&isc->isc_rx_discards);
	evcnt_detach(&isc->isc_rx_unicast);
	evcnt_detach(&isc->isc_rx_multicast);
	evcnt_detach(&isc->isc_rx_broadcast);
	evcnt_detach(&isc->isc_rx_size_64);
	evcnt_detach(&isc->isc_rx_size_127);
	evcnt_detach(&isc->isc_rx_size_255);
	evcnt_detach(&isc->isc_rx_size_511);
	evcnt_detach(&isc->isc_rx_size_1023);
	evcnt_detach(&isc->isc_rx_size_1522);
	evcnt_detach(&isc->isc_rx_size_big);
	evcnt_detach(&isc->isc_rx_undersize);
	evcnt_detach(&isc->isc_rx_oversize);
	evcnt_detach(&isc->isc_tx_bytes);
	evcnt_detach(&isc->isc_tx_dropped_link_down);
	evcnt_detach(&isc->isc_tx_unicast);
	evcnt_detach(&isc->isc_tx_multicast);
	evcnt_detach(&isc->isc_tx_broadcast);
	evcnt_detach(&isc->isc_tx_size_64);
	evcnt_detach(&isc->isc_tx_size_127);
	evcnt_detach(&isc->isc_tx_size_255);
	evcnt_detach(&isc->isc_tx_size_511);
	evcnt_detach(&isc->isc_tx_size_1023);
	evcnt_detach(&isc->isc_tx_size_1522);
	evcnt_detach(&isc->isc_tx_size_big);
	evcnt_detach(&isc->isc_vsi_rx_discards);
	evcnt_detach(&isc->isc_vsi_rx_bytes);
	evcnt_detach(&isc->isc_vsi_rx_unicast);
	evcnt_detach(&isc->isc_vsi_rx_multicast);
	evcnt_detach(&isc->isc_vsi_rx_broadcast);
	evcnt_detach(&isc->isc_vsi_tx_errors);
	evcnt_detach(&isc->isc_vsi_tx_bytes);
	evcnt_detach(&isc->isc_vsi_tx_unicast);
	evcnt_detach(&isc->isc_vsi_tx_multicast);
	evcnt_detach(&isc->isc_vsi_tx_broadcast);

	evcnt_detach(&sc->sc_event_atq);
	evcnt_detach(&sc->sc_event_link);
	evcnt_detach(&sc->sc_event_ecc_err);
	evcnt_detach(&sc->sc_event_pci_exception);
	evcnt_detach(&sc->sc_event_crit_err);

	callout_destroy(&sc->sc_stats_callout);
}

static void
ixl_stats_callout(void *xsc)
{
	struct ixl_softc *sc = xsc;

	ixl_work_add(sc->sc_workq, &sc->sc_stats_task);
	callout_schedule(&sc->sc_stats_callout, mstohz(sc->sc_stats_intval));
}

static uint64_t
ixl_stat_delta(struct ixl_softc *sc, uint32_t reg_hi, uint32_t reg_lo,
    uint64_t *offset, bool has_offset)
{
	uint64_t value, delta;
	int bitwidth;

	bitwidth = reg_hi == 0 ? 32 : 48;

	value = ixl_rd(sc, reg_lo);

	if (bitwidth > 32) {
		value |= ((uint64_t)ixl_rd(sc, reg_hi) << 32);
	}

	if (__predict_true(has_offset)) {
		delta = value;
		if (value < *offset)
			delta += ((uint64_t)1 << bitwidth);
		delta -= *offset;
	} else {
		delta = 0;
	}
	atomic_swap_64(offset, value);

	return delta;
}

static void
ixl_stats_update(void *xsc)
{
	struct ixl_softc *sc = xsc;
	struct ixl_stats_counters *isc;
	uint64_t delta;

	isc = &sc->sc_stats_counters;

	/* errors */
	delta = ixl_stat_delta(sc,
	    0, I40E_GLPRT_CRCERRS(sc->sc_port),
	    &isc->isc_crc_errors_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_crc_errors.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    0, I40E_GLPRT_ILLERRC(sc->sc_port),
	    &isc->isc_illegal_bytes_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_illegal_bytes.ev_count, delta);

	/* rx */
	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_GORCH(sc->sc_port), I40E_GLPRT_GORCL(sc->sc_port),
	    &isc->isc_rx_bytes_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_rx_bytes.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    0, I40E_GLPRT_RDPC(sc->sc_port),
	    &isc->isc_rx_discards_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_rx_discards.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_UPRCH(sc->sc_port), I40E_GLPRT_UPRCL(sc->sc_port),
	    &isc->isc_rx_unicast_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_rx_unicast.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_MPRCH(sc->sc_port), I40E_GLPRT_MPRCL(sc->sc_port),
	    &isc->isc_rx_multicast_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_rx_multicast.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_BPRCH(sc->sc_port), I40E_GLPRT_BPRCL(sc->sc_port),
	    &isc->isc_rx_broadcast_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_rx_broadcast.ev_count, delta);

	/* Packet size stats rx */
	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_PRC64H(sc->sc_port), I40E_GLPRT_PRC64L(sc->sc_port),
	    &isc->isc_rx_size_64_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_rx_size_64.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_PRC127H(sc->sc_port), I40E_GLPRT_PRC127L(sc->sc_port),
	    &isc->isc_rx_size_127_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_rx_size_127.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_PRC255H(sc->sc_port), I40E_GLPRT_PRC255L(sc->sc_port),
	    &isc->isc_rx_size_255_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_rx_size_255.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_PRC511H(sc->sc_port), I40E_GLPRT_PRC511L(sc->sc_port),
	    &isc->isc_rx_size_511_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_rx_size_511.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_PRC1023H(sc->sc_port), I40E_GLPRT_PRC1023L(sc->sc_port),
	    &isc->isc_rx_size_1023_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_rx_size_1023.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_PRC1522H(sc->sc_port), I40E_GLPRT_PRC1522L(sc->sc_port),
	    &isc->isc_rx_size_1522_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_rx_size_1522.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_PRC9522H(sc->sc_port), I40E_GLPRT_PRC9522L(sc->sc_port),
	    &isc->isc_rx_size_big_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_rx_size_big.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    0, I40E_GLPRT_RUC(sc->sc_port),
	    &isc->isc_rx_undersize_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_rx_undersize.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    0, I40E_GLPRT_ROC(sc->sc_port),
	    &isc->isc_rx_oversize_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_rx_oversize.ev_count, delta);

	/* tx */
	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_GOTCH(sc->sc_port), I40E_GLPRT_GOTCL(sc->sc_port),
	    &isc->isc_tx_bytes_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_tx_bytes.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    0, I40E_GLPRT_TDOLD(sc->sc_port),
	    &isc->isc_tx_dropped_link_down_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_tx_dropped_link_down.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_UPTCH(sc->sc_port), I40E_GLPRT_UPTCL(sc->sc_port),
	    &isc->isc_tx_unicast_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_tx_unicast.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_MPTCH(sc->sc_port), I40E_GLPRT_MPTCL(sc->sc_port),
	    &isc->isc_tx_multicast_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_tx_multicast.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_BPTCH(sc->sc_port), I40E_GLPRT_BPTCL(sc->sc_port),
	    &isc->isc_tx_broadcast_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_tx_broadcast.ev_count, delta);

	/* Packet size stats tx */
	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_PTC64L(sc->sc_port), I40E_GLPRT_PTC64L(sc->sc_port),
	    &isc->isc_tx_size_64_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_tx_size_64.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_PTC127H(sc->sc_port), I40E_GLPRT_PTC127L(sc->sc_port),
	    &isc->isc_tx_size_127_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_tx_size_127.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_PTC255H(sc->sc_port), I40E_GLPRT_PTC255L(sc->sc_port),
	    &isc->isc_tx_size_255_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_tx_size_255.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_PTC511H(sc->sc_port), I40E_GLPRT_PTC511L(sc->sc_port),
	    &isc->isc_tx_size_511_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_tx_size_511.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_PTC1023H(sc->sc_port), I40E_GLPRT_PTC1023L(sc->sc_port),
	    &isc->isc_tx_size_1023_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_tx_size_1023.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_PTC1522H(sc->sc_port), I40E_GLPRT_PTC1522L(sc->sc_port),
	    &isc->isc_tx_size_1522_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_tx_size_1522.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLPRT_PTC9522H(sc->sc_port), I40E_GLPRT_PTC9522L(sc->sc_port),
	    &isc->isc_tx_size_big_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_tx_size_big.ev_count, delta);

	/* mac faults */
	delta = ixl_stat_delta(sc,
	    0, I40E_GLPRT_MLFC(sc->sc_port),
	    &isc->isc_mac_local_faults_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_mac_local_faults.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    0, I40E_GLPRT_MRFC(sc->sc_port),
	    &isc->isc_mac_remote_faults_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_mac_remote_faults.ev_count, delta);

	/* Flow control (LFC) stats */
	delta = ixl_stat_delta(sc,
	    0, I40E_GLPRT_LXONRXC(sc->sc_port),
	    &isc->isc_link_xon_rx_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_link_xon_rx.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    0, I40E_GLPRT_LXONTXC(sc->sc_port),
	    &isc->isc_link_xon_tx_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_link_xon_tx.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    0, I40E_GLPRT_LXOFFRXC(sc->sc_port),
	    &isc->isc_link_xoff_rx_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_link_xoff_rx.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    0, I40E_GLPRT_LXOFFTXC(sc->sc_port),
	    &isc->isc_link_xoff_tx_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_link_xoff_tx.ev_count, delta);

	/* fragments */
	delta = ixl_stat_delta(sc,
	    0, I40E_GLPRT_RFC(sc->sc_port),
	    &isc->isc_rx_fragments_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_rx_fragments.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    0, I40E_GLPRT_RJC(sc->sc_port),
	    &isc->isc_rx_jabber_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_rx_jabber.ev_count, delta);

	/* VSI rx counters */
	delta = ixl_stat_delta(sc,
	    0, I40E_GLV_RDPC(sc->sc_vsi_stat_counter_idx),
	    &isc->isc_vsi_rx_discards_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_vsi_rx_discards.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLV_GORCH(sc->sc_vsi_stat_counter_idx),
	    I40E_GLV_GORCL(sc->sc_vsi_stat_counter_idx),
	    &isc->isc_vsi_rx_bytes_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_vsi_rx_bytes.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLV_UPRCH(sc->sc_vsi_stat_counter_idx),
	    I40E_GLV_UPRCL(sc->sc_vsi_stat_counter_idx),
	    &isc->isc_vsi_rx_unicast_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_vsi_rx_unicast.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLV_MPRCH(sc->sc_vsi_stat_counter_idx),
	    I40E_GLV_MPRCL(sc->sc_vsi_stat_counter_idx),
	    &isc->isc_vsi_rx_multicast_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_vsi_rx_multicast.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLV_BPRCH(sc->sc_vsi_stat_counter_idx),
	    I40E_GLV_BPRCL(sc->sc_vsi_stat_counter_idx),
	    &isc->isc_vsi_rx_broadcast_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_vsi_rx_broadcast.ev_count, delta);

	/* VSI tx counters */
	delta = ixl_stat_delta(sc,
	    0, I40E_GLV_TEPC(sc->sc_vsi_stat_counter_idx),
	    &isc->isc_vsi_tx_errors_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_vsi_tx_errors.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLV_GOTCH(sc->sc_vsi_stat_counter_idx),
	    I40E_GLV_GOTCL(sc->sc_vsi_stat_counter_idx),
	    &isc->isc_vsi_tx_bytes_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_vsi_tx_bytes.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLV_UPTCH(sc->sc_vsi_stat_counter_idx),
	    I40E_GLV_UPTCL(sc->sc_vsi_stat_counter_idx),
	    &isc->isc_vsi_tx_unicast_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_vsi_tx_unicast.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLV_MPTCH(sc->sc_vsi_stat_counter_idx),
	    I40E_GLV_MPTCL(sc->sc_vsi_stat_counter_idx),
	    &isc->isc_vsi_tx_multicast_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_vsi_tx_multicast.ev_count, delta);

	delta = ixl_stat_delta(sc,
	    I40E_GLV_BPTCH(sc->sc_vsi_stat_counter_idx),
	    I40E_GLV_BPTCL(sc->sc_vsi_stat_counter_idx),
	    &isc->isc_vsi_tx_broadcast_offset, isc->isc_has_offset);
	atomic_add_64(&isc->isc_vsi_tx_broadcast.ev_count, delta);
}

static int
ixl_setup_sysctls(struct ixl_softc *sc)
{
	const char *devname;
	struct sysctllog **log;
	const struct sysctlnode *rnode, *rxnode, *txnode;
	int error;

	log = &sc->sc_sysctllog;
	devname = device_xname(sc->sc_dev);

	error = sysctl_createv(log, 0, NULL, &rnode,
	    0, CTLTYPE_NODE, devname,
	    SYSCTL_DESCR("ixl information and settings"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "txrx_workqueue",
	    SYSCTL_DESCR("Use workqueue for packet processing"),
	    NULL, 0, &sc->sc_txrx_workqueue, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rnode, NULL,
	    CTLFLAG_READONLY, CTLTYPE_INT, "stats_interval",
	    SYSCTL_DESCR("Statistics collection interval in milliseconds"),
	    NULL, 0, &sc->sc_stats_intval, 0, CTL_CREATE, CTL_EOL);

	error = sysctl_createv(log, 0, &rnode, &rxnode,
	    0, CTLTYPE_NODE, "rx",
	    SYSCTL_DESCR("ixl information and settings for Rx"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rxnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "itr",
	    SYSCTL_DESCR("Interrupt Throttling"),
	    ixl_sysctl_itr_handler, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rxnode, NULL,
	    CTLFLAG_READONLY, CTLTYPE_INT, "descriptor_num",
	    SYSCTL_DESCR("the number of rx descriptors"),
	    NULL, 0, &sc->sc_rx_ring_ndescs, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rxnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "intr_process_limit",
	    SYSCTL_DESCR("max number of Rx packets"
	    " to process for interrupt processing"),
	    NULL, 0, &sc->sc_rx_intr_process_limit, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rxnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "process_limit",
	    SYSCTL_DESCR("max number of Rx packets"
	    " to process for deferred processing"),
	    NULL, 0, &sc->sc_rx_process_limit, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rnode, &txnode,
	    0, CTLTYPE_NODE, "tx",
	    SYSCTL_DESCR("ixl information and settings for Tx"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &txnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "itr",
	    SYSCTL_DESCR("Interrupt Throttling"),
	    ixl_sysctl_itr_handler, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &txnode, NULL,
	    CTLFLAG_READONLY, CTLTYPE_INT, "descriptor_num",
	    SYSCTL_DESCR("the number of tx descriptors"),
	    NULL, 0, &sc->sc_tx_ring_ndescs, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &txnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "intr_process_limit",
	    SYSCTL_DESCR("max number of Tx packets"
	    " to process for interrupt processing"),
	    NULL, 0, &sc->sc_tx_intr_process_limit, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &txnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "process_limit",
	    SYSCTL_DESCR("max number of Tx packets"
	    " to process for deferred processing"),
	    NULL, 0, &sc->sc_tx_process_limit, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

out:
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "unable to create sysctl node\n");
		sysctl_teardown(log);
	}

	return error;
}

static void
ixl_teardown_sysctls(struct ixl_softc *sc)
{

	sysctl_teardown(&sc->sc_sysctllog);
}

static bool
ixl_sysctlnode_is_rx(struct sysctlnode *node)
{

	if (strstr(node->sysctl_parent->sysctl_name, "rx") != NULL)
		return true;

	return false;
}

static int
ixl_sysctl_itr_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct ixl_softc *sc = (struct ixl_softc *)node.sysctl_data;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	uint32_t newitr, *itrptr;
	int error;

	if (ixl_sysctlnode_is_rx(&node)) {
		itrptr = &sc->sc_itr_rx;
	} else {
		itrptr = &sc->sc_itr_tx;
	}

	newitr = *itrptr;
	node.sysctl_data = &newitr;
	node.sysctl_size = sizeof(newitr);

	error = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (error || newp == NULL)
		return error;

	/* ITRs are applied in ixl_init() for simple implementaion */
	if (ISSET(ifp->if_flags, IFF_RUNNING))
		return EBUSY;

	if (newitr > 0x07ff)
		return EINVAL;

	*itrptr = newitr;

	return 0;
}

static struct workqueue *
ixl_workq_create(const char *name, pri_t prio, int ipl, int flags)
{
	struct workqueue *wq;
	int error;

	error = workqueue_create(&wq, name, ixl_workq_work, NULL,
	    prio, ipl, flags);

	if (error)
		return NULL;

	return wq;
}

static void
ixl_workq_destroy(struct workqueue *wq)
{

	workqueue_destroy(wq);
}

static void
ixl_work_set(struct ixl_work *work, void (*func)(void *), void *arg)
{

	memset(work, 0, sizeof(*work));
	work->ixw_func = func;
	work->ixw_arg = arg;
}

static void
ixl_work_add(struct workqueue *wq, struct ixl_work *work)
{
	if (atomic_cas_uint(&work->ixw_added, 0, 1) != 0)
		return;

	kpreempt_disable();
	workqueue_enqueue(wq, &work->ixw_cookie, NULL);
	kpreempt_enable();
}

static void
ixl_work_wait(struct workqueue *wq, struct ixl_work *work)
{

	workqueue_wait(wq, &work->ixw_cookie);
}

static void
ixl_workq_work(struct work *wk, void *context)
{
	struct ixl_work *work;

	work = container_of(wk, struct ixl_work, ixw_cookie);

	atomic_swap_uint(&work->ixw_added, 0);
	work->ixw_func(work->ixw_arg);
}

static int
ixl_rx_ctl_read(struct ixl_softc *sc, uint32_t reg, uint32_t *rv)
{
	struct ixl_aq_desc iaq;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_opcode = htole16(IXL_AQ_OP_RX_CTL_REG_READ);
	iaq.iaq_param[1] = htole32(reg);

	if (ixl_atq_poll(sc, &iaq, 250) != 0)
		return ETIMEDOUT;

	switch (htole16(iaq.iaq_retval)) {
	case IXL_AQ_RC_OK:
		/* success */
		break;
	case IXL_AQ_RC_EACCES:
		return EPERM;
	case IXL_AQ_RC_EAGAIN:
		return EAGAIN;
	default:
		return EIO;
	}

	*rv = htole32(iaq.iaq_param[3]);
	return 0;
}

static uint32_t
ixl_rd_rx_csr(struct ixl_softc *sc, uint32_t reg)
{
	uint32_t val;
	int rv, retry, retry_limit;

	if (ISSET(sc->sc_aq_flags, IXL_SC_AQ_FLAG_RXCTL)) {
		retry_limit = 5;
	} else {
		retry_limit = 0;
	}

	for (retry = 0; retry < retry_limit; retry++) {
		rv = ixl_rx_ctl_read(sc, reg, &val);
		if (rv == 0)
			return val;
		else if (rv == EAGAIN)
			delaymsec(1);
		else
			break;
	}

	val = ixl_rd(sc, reg);

	return val;
}

static int
ixl_rx_ctl_write(struct ixl_softc *sc, uint32_t reg, uint32_t value)
{
	struct ixl_aq_desc iaq;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_opcode = htole16(IXL_AQ_OP_RX_CTL_REG_WRITE);
	iaq.iaq_param[1] = htole32(reg);
	iaq.iaq_param[3] = htole32(value);

	if (ixl_atq_poll(sc, &iaq, 250) != 0)
		return ETIMEDOUT;

	switch (htole16(iaq.iaq_retval)) {
	case IXL_AQ_RC_OK:
		/* success */
		break;
	case IXL_AQ_RC_EACCES:
		return EPERM;
	case IXL_AQ_RC_EAGAIN:
		return EAGAIN;
	default:
		return EIO;
	}

	return 0;
}

static void
ixl_wr_rx_csr(struct ixl_softc *sc, uint32_t reg, uint32_t value)
{
	int rv, retry, retry_limit;

	if (ISSET(sc->sc_aq_flags, IXL_SC_AQ_FLAG_RXCTL)) {
		retry_limit = 5;
	} else {
		retry_limit = 0;
	}

	for (retry = 0; retry < retry_limit; retry++) {
		rv = ixl_rx_ctl_write(sc, reg, value);
		if (rv == 0)
			return;
		else if (rv == EAGAIN)
			delaymsec(1);
		else
			break;
	}

	ixl_wr(sc, reg, value);
}

static int
ixl_nvm_lock(struct ixl_softc *sc, char rw)
{
	struct ixl_aq_desc iaq;
	struct ixl_aq_req_resource_param *param;
	int rv;

	if (!ISSET(sc->sc_aq_flags, IXL_SC_AQ_FLAG_NVMLOCK))
		return 0;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_opcode = htole16(IXL_AQ_OP_REQUEST_RESOURCE);

	param = (struct ixl_aq_req_resource_param *)&iaq.iaq_param;
	param->resource_id = htole16(IXL_AQ_RESOURCE_ID_NVM);
	if (rw == 'R') {
		param->access_type = htole16(IXL_AQ_RESOURCE_ACCES_READ);
	} else {
		param->access_type = htole16(IXL_AQ_RESOURCE_ACCES_WRITE);
	}

	rv = ixl_atq_poll(sc, &iaq, 250);

	if (rv != 0)
		return ETIMEDOUT;

	switch (le16toh(iaq.iaq_retval)) {
	case IXL_AQ_RC_OK:
		break;
	case IXL_AQ_RC_EACCES:
		return EACCES;
	case IXL_AQ_RC_EBUSY:
		return EBUSY;
	case IXL_AQ_RC_EPERM:
		return EPERM;
	}

	return 0;
}

static int
ixl_nvm_unlock(struct ixl_softc *sc)
{
	struct ixl_aq_desc iaq;
	struct ixl_aq_rel_resource_param *param;
	int rv;

	if (!ISSET(sc->sc_aq_flags, IXL_SC_AQ_FLAG_NVMLOCK))
		return 0;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_opcode = htole16(IXL_AQ_OP_RELEASE_RESOURCE);

	param = (struct ixl_aq_rel_resource_param *)&iaq.iaq_param;
	param->resource_id = htole16(IXL_AQ_RESOURCE_ID_NVM);

	rv = ixl_atq_poll(sc, &iaq, 250);

	if (rv != 0)
		return ETIMEDOUT;

	switch (le16toh(iaq.iaq_retval)) {
	case IXL_AQ_RC_OK:
		break;
	default:
		return EIO;
	}
	return 0;
}

static int
ixl_srdone_poll(struct ixl_softc *sc)
{
	int wait_count;
	uint32_t reg;

	for (wait_count = 0; wait_count < IXL_SRRD_SRCTL_ATTEMPTS;
	    wait_count++) {
		reg = ixl_rd(sc, I40E_GLNVM_SRCTL);
		if (ISSET(reg, I40E_GLNVM_SRCTL_DONE_MASK))
			break;

		delaymsec(5);
	}

	if (wait_count == IXL_SRRD_SRCTL_ATTEMPTS)
		return -1;

	return 0;
}

static int
ixl_nvm_read_srctl(struct ixl_softc *sc, uint16_t offset, uint16_t *data)
{
	uint32_t reg;

	if (ixl_srdone_poll(sc) != 0)
		return ETIMEDOUT;

	reg = ((uint32_t)offset << I40E_GLNVM_SRCTL_ADDR_SHIFT) |
	    __BIT(I40E_GLNVM_SRCTL_START_SHIFT);
	ixl_wr(sc, I40E_GLNVM_SRCTL, reg);

	if (ixl_srdone_poll(sc) != 0) {
		aprint_debug("NVM read error: couldn't access "
		    "Shadow RAM address: 0x%x\n", offset);
		return ETIMEDOUT;
	}

	reg = ixl_rd(sc, I40E_GLNVM_SRDATA);
	*data = (uint16_t)__SHIFTOUT(reg, I40E_GLNVM_SRDATA_RDDATA_MASK);

	return 0;
}

static int
ixl_nvm_read_aq(struct ixl_softc *sc, uint16_t offset_word,
    void *data, size_t len)
{
	struct ixl_dmamem *idm;
	struct ixl_aq_desc iaq;
	struct ixl_aq_nvm_param *param;
	uint32_t offset_bytes;
	int rv;

	idm = &sc->sc_aqbuf;
	if (len > IXL_DMA_LEN(idm))
		return ENOMEM;

	memset(IXL_DMA_KVA(idm), 0, IXL_DMA_LEN(idm));
	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_opcode = htole16(IXL_AQ_OP_NVM_READ);
	iaq.iaq_flags = htole16(IXL_AQ_BUF |
	    ((len > I40E_AQ_LARGE_BUF) ? IXL_AQ_LB : 0));
	iaq.iaq_datalen = htole16(len);
	ixl_aq_dva(&iaq, IXL_DMA_DVA(idm));

	param = (struct ixl_aq_nvm_param *)iaq.iaq_param;
	param->command_flags = IXL_AQ_NVM_LAST_CMD;
	param->module_pointer = 0;
	param->length = htole16(len);
	offset_bytes = (uint32_t)offset_word * 2;
	offset_bytes &= 0x00FFFFFF;
	param->offset = htole32(offset_bytes);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(idm), 0, IXL_DMA_LEN(idm),
	    BUS_DMASYNC_PREREAD);

	rv = ixl_atq_poll(sc, &iaq, 250);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(idm), 0, IXL_DMA_LEN(idm),
	    BUS_DMASYNC_POSTREAD);

	if (rv != 0) {
		return ETIMEDOUT;
	}

	switch (le16toh(iaq.iaq_retval)) {
	case IXL_AQ_RC_OK:
		break;
	case IXL_AQ_RC_EPERM:
		return EPERM;
	case IXL_AQ_RC_EINVAL:
		return EINVAL;
	case IXL_AQ_RC_EBUSY:
		return EBUSY;
	case IXL_AQ_RC_EIO:
	default:
		return EIO;
	}

	memcpy(data, IXL_DMA_KVA(idm), len);

	return 0;
}

static int
ixl_rd16_nvm(struct ixl_softc *sc, uint16_t offset, uint16_t *data)
{
	int error;
	uint16_t buf;

	error = ixl_nvm_lock(sc, 'R');
	if (error)
		return error;

	if (ISSET(sc->sc_aq_flags, IXL_SC_AQ_FLAG_NVMREAD)) {
		error = ixl_nvm_read_aq(sc, offset,
		    &buf, sizeof(buf));
		if (error == 0)
			*data = le16toh(buf);
	} else {
		error = ixl_nvm_read_srctl(sc, offset, &buf);
		if (error == 0)
			*data = buf;
	}

	ixl_nvm_unlock(sc);

	return error;
}

MODULE(MODULE_CLASS_DRIVER, if_ixl, "pci");

#ifdef _MODULE
#include "ioconf.c"
#endif

#ifdef _MODULE
static void
ixl_parse_modprop(prop_dictionary_t dict)
{
	prop_object_t obj;
	int64_t val;
	uint64_t uval;

	if (dict == NULL)
		return;

	obj = prop_dictionary_get(dict, "nomsix");
	if (obj != NULL && prop_object_type(obj) == PROP_TYPE_BOOL) {
		ixl_param_nomsix = prop_bool_true((prop_bool_t)obj);
	}

	obj = prop_dictionary_get(dict, "stats_interval");
	if (obj != NULL && prop_object_type(obj) == PROP_TYPE_NUMBER) {
		val = prop_number_signed_value((prop_number_t)obj);

		/* the range has no reason */
		if (100 < val && val < 180000) {
			ixl_param_stats_interval = val;
		}
	}

	obj = prop_dictionary_get(dict, "nqps_limit");
	if (obj != NULL && prop_object_type(obj) == PROP_TYPE_NUMBER) {
		val = prop_number_signed_value((prop_number_t)obj);

		if (val <= INT32_MAX)
			ixl_param_nqps_limit = val;
	}

	obj = prop_dictionary_get(dict, "rx_ndescs");
	if (obj != NULL && prop_object_type(obj) == PROP_TYPE_NUMBER) {
		uval = prop_number_unsigned_integer_value((prop_number_t)obj);

		if (uval > 8)
			ixl_param_rx_ndescs = uval;
	}

	obj = prop_dictionary_get(dict, "tx_ndescs");
	if (obj != NULL && prop_object_type(obj) == PROP_TYPE_NUMBER) {
		uval = prop_number_unsigned_integer_value((prop_number_t)obj);

		if (uval > IXL_TX_PKT_DESCS)
			ixl_param_tx_ndescs = uval;
	}

}
#endif

static int
if_ixl_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

#ifdef _MODULE
	switch (cmd) {
	case MODULE_CMD_INIT:
		ixl_parse_modprop((prop_dictionary_t)opaque);
		error = config_init_component(cfdriver_ioconf_if_ixl,
		    cfattach_ioconf_if_ixl, cfdata_ioconf_if_ixl);
		break;
	case MODULE_CMD_FINI:
		error = config_fini_component(cfdriver_ioconf_if_ixl,
		    cfattach_ioconf_if_ixl, cfdata_ioconf_if_ixl);
		break;
	default:
		error = ENOTTY;
		break;
	}
#endif

	return error;
}
