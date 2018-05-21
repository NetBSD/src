/*	$NetBSD: if_enavar.h,v 1.2.2.2 2018/05/21 04:36:06 pgoyette Exp $	*/

/*-
 * BSD LICENSE
 *
 * Copyright (c) 2015-2017 Amazon.com, Inc. or its affiliates.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/dev/ena/ena.h 333450 2018-05-10 09:06:21Z mw $
 *
 */

#ifndef ENA_H
#define ENA_H

#include <sys/types.h>

#include "external/bsd/ena-com/ena_com.h"
#include "external/bsd/ena-com/ena_eth_com.h"

#define DRV_MODULE_VER_MAJOR	0
#define DRV_MODULE_VER_MINOR	8
#define DRV_MODULE_VER_SUBMINOR 1

#define DRV_MODULE_NAME		"ena"

#ifndef DRV_MODULE_VERSION
#define DRV_MODULE_VERSION				\
	__STRING(DRV_MODULE_VER_MAJOR) "."		\
	__STRING(DRV_MODULE_VER_MINOR) "."		\
	__STRING(DRV_MODULE_VER_SUBMINOR)
#endif
#define DEVICE_NAME	"Elastic Network Adapter (ENA)"
#define DEVICE_DESC	"ENA adapter"

/* Calculate DMA mask - width for ena cannot exceed 48, so it is safe */
#define ENA_DMA_BIT_MASK(x)		((1ULL << (x)) - 1ULL)

/* 1 for AENQ + ADMIN */
#define	ENA_ADMIN_MSIX_VEC		1
#define	ENA_MAX_MSIX_VEC(io_queues)	(ENA_ADMIN_MSIX_VEC + (io_queues))

#define	ENA_REG_BAR			0
#define	ENA_MEM_BAR			2

#define	ENA_BUS_DMA_SEGS		32

#define	ENA_DEFAULT_RING_SIZE		1024

#define	ENA_RX_REFILL_THRESH_DIVIDER	8

#define	ENA_IRQNAME_SIZE		40

#define	ENA_PKT_MAX_BUFS 		19

#define	ENA_RX_RSS_TABLE_LOG_SIZE	7
#define	ENA_RX_RSS_TABLE_SIZE		(1 << ENA_RX_RSS_TABLE_LOG_SIZE)

#define	ENA_HASH_KEY_SIZE		40

#define	ENA_MAX_FRAME_LEN		10000
#define	ENA_MIN_FRAME_LEN 		60

#define ENA_TX_CLEANUP_THRESHOLD	128

#define DB_THRESHOLD	64

#define TX_COMMIT	32
 /*
 * TX budget for cleaning. It should be half of the RX budget to reduce amount
 *  of TCP retransmissions.
 */
#define TX_BUDGET	128
/* RX cleanup budget. -1 stands for infinity. */
#define RX_BUDGET	256
/*
 * How many times we can repeat cleanup in the io irq handling routine if the
 * RX or TX budget was depleted.
 */
#define CLEAN_BUDGET	8

#define RX_IRQ_INTERVAL 20
#define TX_IRQ_INTERVAL 50

#define	ENA_MIN_MTU		128

#define	ENA_TSO_MAXSIZE		65536

#define	ENA_MMIO_DISABLE_REG_READ	BIT(0)

#define	ENA_TX_RING_IDX_NEXT(idx, ring_size) (((idx) + 1) & ((ring_size) - 1))

#define	ENA_RX_RING_IDX_NEXT(idx, ring_size) (((idx) + 1) & ((ring_size) - 1))

#define	ENA_IO_TXQ_IDX(q)		(2 * (q))
#define	ENA_IO_RXQ_IDX(q)		(2 * (q) + 1)

#define	ENA_MGMNT_IRQ_IDX		0
#define	ENA_IO_IRQ_FIRST_IDX		1
#define	ENA_IO_IRQ_IDX(q)		(ENA_IO_IRQ_FIRST_IDX + (q))

/*
 * ENA device should send keep alive msg every 1 sec.
 * We wait for 6 sec just to be on the safe side.
 */
#define DEFAULT_KEEP_ALIVE_TO		(SBT_1S * 6)

/* Time in jiffies before concluding the transmitter is hung. */
#define DEFAULT_TX_CMP_TO		(SBT_1S * 5)

/* Number of queues to check for missing queues per timer tick */
#define DEFAULT_TX_MONITORED_QUEUES	(4)

/* Max number of timeouted packets before device reset */
#define DEFAULT_TX_CMP_THRESHOLD	(128)

/*
 * Supported PCI vendor and devices IDs
 */
#define	PCI_VENDOR_ID_AMAZON	0x1d0f

#define	PCI_DEV_ID_ENA_PF	0x0ec2
#define	PCI_DEV_ID_ENA_LLQ_PF	0x1ec2
#define	PCI_DEV_ID_ENA_VF	0xec20
#define	PCI_DEV_ID_ENA_LLQ_VF	0xec21

struct msix_entry {
	int entry;
	int vector;
};

typedef struct _ena_vendor_info_t {
	unsigned int vendor_id;
	unsigned int device_id;
	unsigned int index;
} ena_vendor_info_t;

struct ena_irq {
	/* Interrupt resources */
	struct resource *res;
	void *handler;
	void *data;
	void *cookie;
	unsigned int vector;
	bool requested;
	int cpu;
	char name[ENA_IRQNAME_SIZE];
};

struct ena_que {
	struct ena_adapter *adapter;
	struct ena_ring *tx_ring;
	struct ena_ring *rx_ring;
	uint32_t id;
	int cpu;
};

struct ena_tx_buffer {
	struct mbuf *mbuf;
	/* # of ena desc for this specific mbuf
	 * (includes data desc and metadata desc) */
	unsigned int tx_descs;
	/* # of buffers used by this mbuf */
	unsigned int num_of_bufs;
	bus_dmamap_t map;

	/* Used to detect missing tx packets */
	struct bintime timestamp;
	bool print_once;

	struct ena_com_buf bufs[ENA_PKT_MAX_BUFS];
} __aligned(CACHE_LINE_SIZE);

struct ena_rx_buffer {
	struct mbuf *mbuf;
	bus_dmamap_t map;
	struct ena_com_buf ena_buf;
} __aligned(CACHE_LINE_SIZE);

struct ena_stats_tx {
	struct evcnt cnt;
	struct evcnt bytes;
	struct evcnt prepare_ctx_err;
	struct evcnt dma_mapping_err;
	struct evcnt doorbells;
	struct evcnt missing_tx_comp;
	struct evcnt bad_req_id;
	struct evcnt collapse;
	struct evcnt collapse_err;
};

struct ena_stats_rx {
	struct evcnt cnt;
	struct evcnt bytes;
	struct evcnt refil_partial;
	struct evcnt bad_csum;
	struct evcnt mjum_alloc_fail;
	struct evcnt mbuf_alloc_fail;
	struct evcnt dma_mapping_err;
	struct evcnt bad_desc_num;
	struct evcnt bad_req_id;
	struct evcnt empty_rx_ring;
};

struct ena_ring {
	/* Holds the empty requests for TX/RX out of order completions */
	union {
		uint16_t *free_tx_ids;
		uint16_t *free_rx_ids;
	};
	struct ena_com_dev *ena_dev;
	struct ena_adapter *adapter;
	struct ena_com_io_cq *ena_com_io_cq;
	struct ena_com_io_sq *ena_com_io_sq;

	uint16_t qid;

	/* Determines if device will use LLQ or normal mode for TX */
	enum ena_admin_placement_policy_type tx_mem_queue_type;
	/* The maximum length the driver can push to the device (For LLQ) */
	uint8_t tx_max_header_size;

	struct ena_com_rx_buf_info ena_bufs[ENA_PKT_MAX_BUFS];

	/*
	 * Fields used for Adaptive Interrupt Modulation - to be implemented in
	 * the future releases
	 */
	uint32_t  smoothed_interval;
	enum ena_intr_moder_level moder_tbl_idx;

	struct ena_que *que;
#ifdef LRO
	struct lro_ctrl lro;
#endif

	uint16_t next_to_use;
	uint16_t next_to_clean;

	union {
		struct ena_tx_buffer *tx_buffer_info; /* contex of tx packet */
		struct ena_rx_buffer *rx_buffer_info; /* contex of rx packet */
	};
	int ring_size; /* number of tx/rx_buffer_info's entries */

	struct buf_ring *br; /* only for TX */

	kmutex_t ring_mtx;
	char mtx_name[16];

	union {
		struct {
			struct work enqueue_task;
			struct workqueue *enqueue_tq;
		};
		struct {
			struct work cmpl_task;
			struct workqueue *cmpl_tq;
		};
	};

	union {
		struct ena_stats_tx tx_stats;
		struct ena_stats_rx rx_stats;
	};

	int empty_rx_queue;
} __aligned(CACHE_LINE_SIZE);

struct ena_stats_dev {
	struct evcnt wd_expired;
	struct evcnt interface_up;
	struct evcnt interface_down;
	struct evcnt admin_q_pause;
};

struct ena_hw_stats {
	struct evcnt rx_packets;
	struct evcnt tx_packets;

	struct evcnt rx_bytes;
	struct evcnt tx_bytes;

	struct evcnt rx_drops;
};

/* Board specific private data structure */
struct ena_adapter {
	struct ena_com_dev *ena_dev;

	/* OS defined structs */
	device_t pdev;
        struct ethercom sc_ec;
	struct ifnet *ifp;		/* set to point to sc_ec */
	struct ifmedia	media;

	/* OS resources */
	struct resource *memory;
	struct resource *registers;

	kmutex_t global_mtx;
	krwlock_t ioctl_sx;

	/* MSI-X */
	uint32_t msix_enabled;
	struct msix_entry *msix_entries;
	int msix_vecs;

	/* DMA tag used throughout the driver adapter for Tx and Rx */
	bus_dma_tag_t sc_dmat;
	int dma_width;

	uint32_t max_mtu;

	uint16_t max_tx_sgl_size;
	uint16_t max_rx_sgl_size;

	uint32_t tx_offload_cap;

	/* Tx fast path data */
	int num_queues;

	unsigned int tx_ring_size;
	unsigned int rx_ring_size;

	/* RSS*/
	uint8_t	rss_ind_tbl[ENA_RX_RSS_TABLE_SIZE];
	bool rss_support;

	uint8_t mac_addr[ETHER_ADDR_LEN];
	/* mdio and phy*/

	bool link_status;
	bool trigger_reset;
	bool up;
	bool running;

	/* Queue will represent one TX and one RX ring */
	struct ena_que que[ENA_MAX_NUM_IO_QUEUES]
	    __aligned(CACHE_LINE_SIZE);

	/* TX */
	struct ena_ring tx_ring[ENA_MAX_NUM_IO_QUEUES]
	    __aligned(CACHE_LINE_SIZE);

	/* RX */
	struct ena_ring rx_ring[ENA_MAX_NUM_IO_QUEUES]
	    __aligned(CACHE_LINE_SIZE);

	struct ena_irq irq_tbl[ENA_MAX_MSIX_VEC(ENA_MAX_NUM_IO_QUEUES)];

	/* Timer service */
	struct callout timer_service;
	struct bintime keep_alive_timestamp;
	uint32_t next_monitored_tx_qid;
	struct work reset_task;
	struct workqueue *reset_tq;
	int wd_active;
	struct bintime keep_alive_timeout;
	struct bintime missing_tx_timeout;
	uint32_t missing_tx_max_queues;
	uint32_t missing_tx_threshold;

	/* Statistics */
	struct ena_stats_dev dev_stats;
	struct ena_hw_stats hw_stats;

	enum ena_regs_reset_reason_types reset_reason;
};

#define	ENA_RING_MTX_LOCK(_ring)	mutex_enter(&(_ring)->ring_mtx)
#define	ENA_RING_MTX_TRYLOCK(_ring)	mutex_tryenter(&(_ring)->ring_mtx)
#define	ENA_RING_MTX_UNLOCK(_ring)	mutex_exit(&(_ring)->ring_mtx)

static inline int ena_mbuf_count(struct mbuf *mbuf)
{
	int count = 1;

	while ((mbuf = mbuf->m_next) != NULL)
		++count;

	return count;
}

#endif /* !(ENA_H) */
