/*	$NetBSD: igc_evcnt.h,v 1.1.2.2 2023/10/08 13:19:34 martin Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Rin Okuyama <rin@iij.ad.jp>.
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

#ifndef _IGC_EVCNT_H_

#include <sys/param.h>
#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/evcnt.h>

#include <dev/pci/igc/if_igc.h>

#ifndef IGC_EVENT_COUNTERS

#define	IGC_GLOBAL_EVENT(sc, event, delta)	((void)(sc))
#define	IGC_DRIVER_EVENT(q, event, delta)	((void)(q))
#define	IGC_QUEUE_EVENT(q, event, delta)	((void)(q))

#else

struct igc_counter {
	int type;
	const char *name;
};

/*
 * Global counters:
 *
 * Events outside queue context.
 */

enum igc_global_event {
	igcge_watchdog,
	igcge_link,
	igcge_count,
};

static const struct igc_counter igc_global_counters[] = {
	[igcge_watchdog]     = { EVCNT_TYPE_MISC, "Watchdog Timeout" },
	[igcge_link]         = { EVCNT_TYPE_INTR, "Link Event" },
};

#define	IGC_GLOBAL_COUNTERS	__arraycount(igc_global_counters)
CTASSERT(IGC_GLOBAL_COUNTERS == igcge_count);

/*
 * Driver counters:
 *
 * Events in queue context, but summed up over queues.
 */

enum igc_driver_event {
	igcde_txdma_efbig,
	igcde_txdma_defrag,
	igcde_txdma_enomem,
	igcde_txdma_einval,
	igcde_txdma_eagain,
	igcde_txdma_other,
	igcde_rx_ipcs,
	igcde_rx_tcpcs,
	igcde_rx_udpcs,
	igcde_rx_ipcs_bad,
	igcde_rx_l4cs_bad,
	igcde_count,
};

static const struct igc_counter igc_driver_counters[] = {
	[igcde_txdma_efbig]  = { EVCNT_TYPE_MISC, "Tx DMA Soft Fail EFBIG" },
#ifdef notyet
	[igcde_txdma_efbig2] = { EVCNT_TYPE_MISC, "Tx DMA Hard Fail EFBIG" },
#endif
	[igcde_txdma_defrag] = { EVCNT_TYPE_MISC, "Tx DMA Fail Defrag" },
	[igcde_txdma_enomem] = { EVCNT_TYPE_MISC, "Tx DMA Fail ENOMEM" },
	[igcde_txdma_einval] = { EVCNT_TYPE_MISC, "Tx DMA Fail EINVAL" },
	[igcde_txdma_eagain] = { EVCNT_TYPE_MISC, "Tx DMA Fail EAGAIN" },
	[igcde_txdma_other]  = { EVCNT_TYPE_MISC, "Tx DMA Fail Other" },
	[igcde_rx_ipcs]      = { EVCNT_TYPE_MISC, "Rx Csum Offload IPv4" },
	[igcde_rx_tcpcs]     = { EVCNT_TYPE_MISC, "Rx Csum Offload TCP" },
	[igcde_rx_udpcs]     = { EVCNT_TYPE_MISC, "Rx Csum Offload UDP" },
	[igcde_rx_ipcs_bad]  = { EVCNT_TYPE_MISC, "Rx Csum Offload IPv4 Bad" },
	[igcde_rx_l4cs_bad]  = { EVCNT_TYPE_MISC, "Rx Csum Offload L4 Bad" },
};

#define	IGC_DRIVER_COUNTERS	__arraycount(igc_driver_counters)
CTASSERT(IGC_DRIVER_COUNTERS == igcde_count);

/*
 * Queue counters:
 *
 * Per queue events.
 */

enum igc_queue_event {
	igcqe_irqs,
	igcqe_handleq,
	igcqe_req,
	igcqe_tx_bytes,
	igcqe_tx_packets,
	igcqe_tx_pcq_drop,
	igcqe_tx_no_desc,
	igcqe_tx_ctx,
	igcqe_rx_bytes,
	igcqe_rx_packets,
	igcqe_rx_no_mbuf,
	igcqe_rx_discard,
	igcqe_count,
};

static const struct igc_counter igc_queue_counters[] = {
	[igcqe_irqs]        = { EVCNT_TYPE_INTR, "Interrupts" },
	[igcqe_handleq]     = { EVCNT_TYPE_MISC, "Handled in softint" },
	[igcqe_req]         = { EVCNT_TYPE_MISC, "Requeued in softint" },
	[igcqe_tx_bytes]    = { EVCNT_TYPE_MISC, "Tx Bytes" },
	[igcqe_tx_packets]  = { EVCNT_TYPE_MISC, "Tx Packets" },
	[igcqe_tx_pcq_drop] = { EVCNT_TYPE_MISC, "Tx pcq Dropped" },
	[igcqe_tx_no_desc]  = { EVCNT_TYPE_MISC, "Tx No Descriptor Available" },
	[igcqe_tx_ctx]      = { EVCNT_TYPE_MISC, "Tx Advanced CTX Used" },
	[igcqe_rx_bytes]    = { EVCNT_TYPE_MISC, "Rx Bytes" },
	[igcqe_rx_packets]  = { EVCNT_TYPE_MISC, "Rx Packets" },
	[igcqe_rx_no_mbuf]  = { EVCNT_TYPE_MISC, "Rx No mbuf Available" },
	[igcqe_rx_discard]  = { EVCNT_TYPE_MISC, "Rx Discarded" },
#ifdef notyet
	[igcqe_rx_copy]    = { EVCNT_TYPE_MISC, "Rx Copied Frames" },
#endif
};

#define	IGC_QUEUE_COUNTERS	__arraycount(igc_queue_counters)
CTASSERT(IGC_QUEUE_COUNTERS == igcqe_count);

/*
 * MAC counters:
 *
 * Events obtained from MAC Statistics registers.
 */

static const struct igc_mac_counter {
	bus_size_t reg;
	bool is64;
	const char *name;
} igc_mac_counters[] = {
/* Interrupts */
	{ IGC_IAC,	false,	"Interrupt Assertion" },

/* TX errors */
	{ IGC_COLC,	false,	"Collision" },
	{ IGC_SCC,	false,	"Single Collision" },
	{ IGC_MCC,	false,	"Multiple Collision" },
	{ IGC_ECOL,	false,	"Excessive Collision" },
	{ IGC_LATECOL,	false,	"Late Collision" },
	{ IGC_TNCRS,	false,	"Tx-No CRS" },

/* RX errors */
	{ IGC_CRCERRS,	false,	"CRC Error" },
	{ IGC_MPC,	false,	"Missed Packet" },
	{ IGC_RLEC,	false,	"Receive Length Error" },
	{ IGC_LENERRS,	false,	"Length Errors" },
	{ IGC_ALGNERRC,	false,	"Alignment Error" },
	{ IGC_RERC,	false,	"Receive Error" },

/* flow control events */
	{ IGC_XOFFTXC,	false,	"XOFF Tx" },
	{ IGC_XONTXC,	false,	"XON Tx" },
	{ IGC_XOFFRXC,	false,	"XOFF Rx" },
	{ IGC_XONRXC,	false,	"XON Rx" },
	{ IGC_FCRUC,	false,	"Flow Control Rx Unsupported" },

/* TX statistics */
	{ IGC_TOTL,	true,	"Total Octets Tx" },
	{ IGC_GOTCL,	true,	"Good Octets Tx" },
	{ IGC_HGOTCL,	true,	"Host Good Octets Transmit" },
	{ IGC_TPT,	false,	"Total Packets Tx" },
	{ IGC_GPTC,	false,	"Good Packets Tx" },
	{ IGC_MPTC,	false,	"Multicast Packets Tx" },
	{ IGC_BPTC,	false,	"Broadcast Packets Tx" },
	{ IGC_MGTPTC,	false,	"Management Packets Tx" },
	{ IGC_HTDPMC,	false,	"Host Transmit Discarded by MAC" },
	{ IGC_DC,	false,	"Defer" },
	{ IGC_TSCTC,	false,	"TCP Segmentation Context Tx" },
	{ IGC_PTC64,	false,	"Packets Tx (64 bytes)" },
	{ IGC_PTC127,	false,	"Packets Tx (65-127 bytes)" },
	{ IGC_PTC255,	false,	"Packets Tx (128-255 bytes)" },
	{ IGC_PTC511,	false,	"Packets Tx (256-511 bytes)" },
	{ IGC_PTC1023,	false,	"Packets Tx (512-1023 bytes)" },
	{ IGC_PTC1522,	false,	"Packets Tx (1024-1522 bytes)" },

/* RX statistics */
	{ IGC_TORL,	true,	"Total Octets Rx" },
	{ IGC_GORCL,	true,	"Good Octets Rx" },
	{ IGC_HGORCL,	true,	"Host Good Octets Received" },
	{ IGC_TPR,	false,	"Total Packets Rx" },
	{ IGC_GPRC,	false,	"Good Packets Rx" },
	{ IGC_BPRC,	false,	"Broadcast Packets Rx" },
	{ IGC_MPRC,	false,	"Multicast Packets Rx" },
	{ IGC_MGTPRC,	false,	"Management Packets Rx" },
	{ IGC_MGTPDC,	false,	"Management Packets Dropped" },
	{ IGC_RNBC,	false,	"Rx No Buffers" },
	{ IGC_RUC,	false,	"Rx Undersize" },
	{ IGC_RFC,	false,	"Rx Fragment" },
	{ IGC_ROC,	false,	"Rx Oversize" },
	{ IGC_RJC,	false,	"Rx Jabber" },
	{ IGC_RXDMTC,	false,	"Rx Descriptor Minimum Threshold" },
	{ IGC_PRC64,	false,	"Packets Rx (64 bytes)" },
	{ IGC_PRC127,	false,	"Packets Rx (65-127 bytes)" },
	{ IGC_PRC255,	false,	"Packets Rx (128-255 bytes)" },
	{ IGC_PRC511,	false,	"Packets Rx (256-511 bytes)" },
	{ IGC_PRC1023,	false,	"Packets Rx (512-1023 bytes)" },
	{ IGC_PRC1522,	false,	"Packets Rx (1024-1522 bytes)" },

/* EEE */
	{ IGC_TLPIC,	false,	"EEE Tx LPI" },
	{ IGC_RLPIC,	false,	"EEE Rx LPI" },
};

#define	IGC_MAC_COUNTERS	__arraycount(igc_mac_counters)

#define	IGC_ATOMIC_ADD(p, delta)					\
    atomic_store_relaxed(p, atomic_load_relaxed(p) + (delta))
#define	IGC_ATOMIC_STORE(p, val)					\
    atomic_store_relaxed(p, val)
#define	IGC_ATOMIC_LOAD(p)						\
    atomic_load_relaxed(p)

#define	IGC_GLOBAL_COUNTER(sc, cnt)					\
    ((sc)->sc_global_evcnts[cnt].ev_count)
#define	IGC_GLOBAL_COUNTER_ADD(sc, cnt, delta)				\
    IGC_ATOMIC_ADD(&IGC_GLOBAL_COUNTER(sc, cnt), delta)
#define	IGC_GLOBAL_COUNTER_STORE(sc, cnt, val)				\
    IGC_ATOMIC_STORE(&IGC_GLOBAL_COUNTER(sc, cnt), val)

#define	IGC_DRIVER_COUNTER(sc, cnt)					\
    ((sc)->sc_driver_evcnts[cnt].ev_count)
#define	IGC_DRIVER_COUNTER_ADD(sc, cnt, delta)				\
    IGC_ATOMIC_ADD(&IGC_DRIVER_COUNTER(sc, cnt), delta)
#define	IGC_DRIVER_COUNTER_STORE(sc, cnt, val)				\
    IGC_ATOMIC_STORE(&IGC_DRIVER_COUNTER(sc, cnt), val)

#define	IGC_QUEUE_DRIVER_COUNTER(q, cnt)				\
    ((q)->igcq_driver_counters[cnt])
#define	IGC_QUEUE_DRIVER_COUNTER_VAL(q, cnt)				\
    IGC_ATOMIC_LOAD(&IGC_QUEUE_DRIVER_COUNTER(q, cnt))
#define	IGC_QUEUE_DRIVER_COUNTER_ADD(q, cnt, delta)			\
    IGC_ATOMIC_ADD(&IGC_QUEUE_DRIVER_COUNTER(q, cnt), delta)
#define	IGC_QUEUE_DRIVER_COUNTER_STORE(q, cnt, val)			\
    IGC_ATOMIC_STORE(&IGC_QUEUE_DRIVER_COUNTER(q, cnt), val)

#define	IGC_QUEUE_COUNTER(q, cnt)					\
    ((q)->igcq_queue_evcnts[cnt].ev_count)
#define	IGC_QUEUE_COUNTER_ADD(q, cnt, delta)				\
    IGC_ATOMIC_ADD(&IGC_QUEUE_COUNTER(q, cnt), delta)
#define	IGC_QUEUE_COUNTER_STORE(q, cnt, val)				\
    IGC_ATOMIC_STORE(&IGC_QUEUE_COUNTER(q, cnt), val)

#define	IGC_MAC_COUNTER(sc, cnt)					\
    ((sc)->sc_mac_evcnts[cnt].ev_count)
#define	IGC_MAC_COUNTER_ADD(sc, cnt, delta)				\
    IGC_ATOMIC_ADD(&IGC_MAC_COUNTER(sc, cnt), delta)
#define	IGC_MAC_COUNTER_STORE(sc, cnt, val)				\
    IGC_ATOMIC_STORE(&IGC_MAC_COUNTER(sc, cnt), val)

#define	IGC_GLOBAL_EVENT(sc, event, delta)				\
    IGC_GLOBAL_COUNTER_ADD(sc, igcge_ ## event, delta)
#define	IGC_DRIVER_EVENT(q, event, delta)				\
    IGC_QUEUE_DRIVER_COUNTER_ADD(q, igcde_ ## event, delta)
#define	IGC_QUEUE_EVENT(q, event, delta)				\
    IGC_QUEUE_COUNTER_ADD(q, igcqe_ ## event, delta)

#endif /* IGC_EVENT_COUNTERS */

#endif /* _IGC_EVCNT_ */
