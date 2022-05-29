/*	$NetBSD: if_hvn.c,v 1.23 2022/05/29 10:43:45 rin Exp $	*/
/*	$OpenBSD: if_hvn.c,v 1.39 2018/03/11 14:31:34 mikeb Exp $	*/

/*-
 * Copyright (c) 2009-2012,2016 Microsoft Corp.
 * Copyright (c) 2010-2012 Citrix Inc.
 * Copyright (c) 2012 NetApp Inc.
 * Copyright (c) 2016 Mike Belopuhov <mike@esdenera.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * The OpenBSD port was done under funding by Esdenera Networks GmbH.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_hvn.c,v 1.23 2022/05/29 10:43:45 rin Exp $");

#ifdef _KERNEL_OPT
#include "opt_if_hvn.h"
#include "opt_inet.h"
#include "opt_inet6.h"
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/bitops.h>
#include <sys/bus.h>
#include <sys/condvar.h>
#include <sys/cpu.h>
#include <sys/evcnt.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/pcq.h>
#include <sys/sysctl.h>
#include <sys/workqueue.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/if_vlanvar.h>
#include <net/rss_config.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>

#include <net/bpf.h>

#include <dev/ic/ndisreg.h>
#include <dev/ic/rndisreg.h>

#include <dev/hyperv/vmbusvar.h>
#include <dev/hyperv/if_hvnreg.h>

#ifndef EVL_PRIO_BITS
#define EVL_PRIO_BITS	13
#endif
#ifndef EVL_CFI_BITS
#define EVL_CFI_BITS	12
#endif

#define HVN_CHIM_SIZE			(15 * 1024 * 1024)

#define HVN_NVS_MSGSIZE			32
#define HVN_NVS_BUFSIZE			PAGE_SIZE

#define HVN_RING_BUFSIZE		(128 * PAGE_SIZE)
#define HVN_RING_IDX2CPU(sc, idx)	((idx) % ncpu)

#ifndef HVN_CHANNEL_MAX_COUNT_DEFAULT
#define HVN_CHANNEL_MAX_COUNT_DEFAULT	8
#endif

#ifndef HVN_LINK_STATE_CHANGE_DELAY
#define HVN_LINK_STATE_CHANGE_DELAY	5000
#endif

#define HVN_WORKQUEUE_PRI		PRI_SOFTNET

/*
 * RNDIS control interface
 */
#define HVN_RNDIS_CTLREQS		4
#define HVN_RNDIS_BUFSIZE		512

struct rndis_cmd {
	uint32_t			rc_id;
	struct hvn_nvs_rndis		rc_msg;
	void				*rc_req;
	bus_dmamap_t			rc_dmap;
	bus_dma_segment_t		rc_segs;
	int				rc_nsegs;
	uint64_t			rc_gpa;
	struct rndis_packet_msg		rc_cmp;
	uint32_t			rc_cmplen;
	uint8_t				rc_cmpbuf[HVN_RNDIS_BUFSIZE];
	int				rc_done;
	TAILQ_ENTRY(rndis_cmd)		rc_entry;
	kmutex_t			rc_lock;
	kcondvar_t			rc_cv;
};
TAILQ_HEAD(rndis_queue, rndis_cmd);

#define HVN_MTU_MIN			68
#define HVN_MTU_MAX			(65535 - ETHER_ADDR_LEN)

#define HVN_RNDIS_XFER_SIZE		2048

#define HVN_NDIS_TXCSUM_CAP_IP4 \
	(NDIS_TXCSUM_CAP_IP4 | NDIS_TXCSUM_CAP_IP4OPT)
#define HVN_NDIS_TXCSUM_CAP_TCP4 \
	(NDIS_TXCSUM_CAP_TCP4 | NDIS_TXCSUM_CAP_TCP4OPT)
#define HVN_NDIS_TXCSUM_CAP_TCP6 \
	(NDIS_TXCSUM_CAP_TCP6 | NDIS_TXCSUM_CAP_TCP6OPT | \
	    NDIS_TXCSUM_CAP_IP6EXT)
#define HVN_NDIS_TXCSUM_CAP_UDP6 \
	(NDIS_TXCSUM_CAP_UDP6 | NDIS_TXCSUM_CAP_IP6EXT)
#define HVN_NDIS_LSOV2_CAP_IP6 \
	(NDIS_LSOV2_CAP_IP6EXT | NDIS_LSOV2_CAP_TCP6OPT)

#define HVN_RNDIS_CMD_NORESP	__BIT(0)

#define HVN_NVS_CMD_NORESP	__BIT(0)

/*
 * Tx ring
 */
#define HVN_TX_DESC			512
#define HVN_TX_FRAGS			15		/* 31 is the max */
#define HVN_TX_FRAG_SIZE		PAGE_SIZE
#define HVN_TX_PKT_SIZE			16384

#define HVN_RNDIS_PKT_LEN					\
	(sizeof(struct rndis_packet_msg) +			\
	 sizeof(struct rndis_pktinfo) + NDIS_VLAN_INFO_SIZE +	\
	 sizeof(struct rndis_pktinfo) + NDIS_TXCSUM_INFO_SIZE)

#define HVN_PKTSIZE_MIN(align)						\
	roundup2(ETHER_MIN_LEN + ETHER_VLAN_ENCAP_LEN - ETHER_CRC_LEN +	\
	HVN_RNDIS_PKT_LEN, (align))
#define HVN_PKTSIZE(m, align)						\
	roundup2((m)->m_pkthdr.len + HVN_RNDIS_PKT_LEN, (align))

struct hvn_tx_desc {
	uint32_t			txd_id;
	struct vmbus_gpa		txd_sgl[HVN_TX_FRAGS + 1];
	int				txd_nsge;
	struct mbuf			*txd_buf;
	bus_dmamap_t			txd_dmap;
	struct vmbus_gpa		txd_gpa;
	struct rndis_packet_msg		*txd_req;
	TAILQ_ENTRY(hvn_tx_desc)	txd_entry;
	u_int				txd_refs;
	uint32_t			txd_flags;
#define HVN_TXD_FLAG_ONAGG		__BIT(0)
#define HVN_TXD_FLAG_DMAMAP		__BIT(1)
	uint32_t			txd_chim_index;
	int				txd_chim_size;
	STAILQ_ENTRY(hvn_tx_desc)	txd_agg_entry;
	STAILQ_HEAD(, hvn_tx_desc)	txd_agg_list;
};

struct hvn_softc;
struct hvn_rx_ring;

struct hvn_tx_ring {
	struct hvn_softc		*txr_softc;
	struct vmbus_channel		*txr_chan;
	struct hvn_rx_ring		*txr_rxr;
	void				*txr_si;
	char				txr_name[16];

	int				txr_id;
	int				txr_oactive;
	int				txr_suspended;
	int				txr_csum_assist;
	uint64_t			txr_caps_assist;
	uint32_t			txr_flags;
#define HVN_TXR_FLAG_UDP_HASH		__BIT(0)

	struct evcnt			txr_evpkts;
	struct evcnt			txr_evsends;
	struct evcnt			txr_evnodesc;
	struct evcnt			txr_evdmafailed;
	struct evcnt			txr_evdefrag;
	struct evcnt			txr_evpcqdrop;
	struct evcnt			txr_evtransmitdefer;
	struct evcnt			txr_evflushfailed;
	struct evcnt			txr_evchimneytried;
	struct evcnt			txr_evchimney;
	struct evcnt			txr_evvlanfixup;
	struct evcnt			txr_evvlanhwtagging;
	struct evcnt			txr_evvlantap;

	kmutex_t			txr_lock;
	pcq_t				*txr_interq;

	uint32_t			txr_avail;
	TAILQ_HEAD(, hvn_tx_desc)	txr_list;
	struct hvn_tx_desc		txr_desc[HVN_TX_DESC];
	uint8_t				*txr_msgs;
	struct hyperv_dma		txr_dma;

	int				txr_chim_size;

	/* Applied packet transmission aggregation limits. */
	int				txr_agg_szmax;
	short				txr_agg_pktmax;
	short				txr_agg_align;

	/* Packet transmission aggregation states. */
	struct hvn_tx_desc		*txr_agg_txd;
	int				txr_agg_szleft;
	short				txr_agg_pktleft;
	struct rndis_packet_msg		*txr_agg_prevpkt;

	/* Temporary stats for each sends. */
	int				txr_stat_pkts;
	int				txr_stat_size;
	int				txr_stat_mcasts;

	int				(*txr_sendpkt)(struct hvn_tx_ring *,
					    struct hvn_tx_desc *);
} __aligned(CACHE_LINE_SIZE);

struct hvn_rx_ring {
	struct hvn_softc		*rxr_softc;
	struct vmbus_channel		*rxr_chan;
	struct hvn_tx_ring		*rxr_txr;
	void				*rxr_si;
	bool				rxr_workqueue;
	char				rxr_name[16];

	struct work			rxr_wk;
	volatile bool			rxr_onlist;
	volatile bool			rxr_onproc;
	kmutex_t			rxr_onwork_lock;
	kcondvar_t			rxr_onwork_cv;

	uint32_t			rxr_flags;
#define HVN_RXR_FLAG_UDP_HASH		__BIT(0)

	kmutex_t			rxr_lock;

	struct evcnt			rxr_evpkts;
	struct evcnt			rxr_evcsum_ip;
	struct evcnt			rxr_evcsum_tcp;
	struct evcnt			rxr_evcsum_udp;
	struct evcnt			rxr_evvlanhwtagging;
	struct evcnt			rxr_evintr;
	struct evcnt			rxr_evdefer;
	struct evcnt			rxr_evdeferreq;
	struct evcnt			rxr_evredeferreq;

	/* NVS */
	uint8_t				*rxr_nvsbuf;
} __aligned(CACHE_LINE_SIZE);

struct hvn_softc {
	device_t			sc_dev;

	struct vmbus_softc		*sc_vmbus;
	struct vmbus_channel		*sc_prichan;
	bus_dma_tag_t			sc_dmat;

	struct ethercom			sc_ec;
	struct ifmedia			sc_media;
	struct if_percpuq		*sc_ipq;
	struct workqueue		*sc_wq;
	bool				sc_txrx_workqueue;
	kmutex_t			sc_core_lock;

	kmutex_t			sc_link_lock;
	kcondvar_t			sc_link_cv;
	callout_t			sc_link_tmout;
	lwp_t				*sc_link_lwp;
	uint32_t			sc_link_ev;
#define HVN_LINK_EV_STATE_CHANGE	__BIT(0)
#define HVN_LINK_EV_NETWORK_CHANGE_TMOUT __BIT(1)
#define HVN_LINK_EV_NETWORK_CHANGE	__BIT(2)
#define HVN_LINK_EV_RESUME_NETWORK	__BIT(3)
#define HVN_LINK_EV_EXIT_THREAD		__BIT(4)
	int				sc_link_state;
	bool				sc_link_onproc;
	bool				sc_link_pending;
	bool				sc_link_suspend;

	int				sc_tx_process_limit;
	int				sc_rx_process_limit;
	int				sc_tx_intr_process_limit;
	int				sc_rx_intr_process_limit;

	struct sysctllog		*sc_sysctllog;

	uint32_t			sc_caps;
#define HVN_CAPS_VLAN			__BIT(0)
#define HVN_CAPS_MTU			__BIT(1)
#define HVN_CAPS_IPCS			__BIT(2)
#define HVN_CAPS_TCP4CS			__BIT(3)
#define HVN_CAPS_TCP6CS			__BIT(4)
#define HVN_CAPS_UDP4CS			__BIT(5)
#define HVN_CAPS_UDP6CS			__BIT(6)
#define HVN_CAPS_TSO4			__BIT(7)
#define HVN_CAPS_TSO6			__BIT(8)
#define HVN_CAPS_HASHVAL		__BIT(9)
#define HVN_CAPS_UDPHASH		__BIT(10)

	uint32_t			sc_flags;
#define HVN_SCF_ATTACHED		__BIT(0)
#define HVN_SCF_RXBUF_CONNECTED		__BIT(1)
#define HVN_SCF_CHIM_CONNECTED		__BIT(2)
#define HVN_SCF_REVOKED			__BIT(3)
#define HVN_SCF_HAS_RSSKEY		__BIT(4)
#define HVN_SCF_HAS_RSSIND		__BIT(5)

	/* NVS protocol */
	int				sc_proto;
	uint32_t			sc_nvstid;
	uint8_t				sc_nvsrsp[HVN_NVS_MSGSIZE];
	int				sc_nvsdone;
	kmutex_t			sc_nvsrsp_lock;
	kcondvar_t			sc_nvsrsp_cv;

	/* RNDIS protocol */
	int				sc_ndisver;
	uint32_t			sc_rndisrid;
	int				sc_tso_szmax;
	int				sc_tso_sgmin;
	uint32_t			sc_rndis_agg_size;
	uint32_t			sc_rndis_agg_pkts;
	uint32_t			sc_rndis_agg_align;
	struct rndis_queue		sc_cntl_sq; /* submission queue */
	kmutex_t			sc_cntl_sqlck;
	struct rndis_queue		sc_cntl_cq; /* completion queue */
	kmutex_t			sc_cntl_cqlck;
	struct rndis_queue		sc_cntl_fq; /* free queue */
	kmutex_t			sc_cntl_fqlck;
	kcondvar_t			sc_cntl_fqcv;
	struct rndis_cmd		sc_cntl_msgs[HVN_RNDIS_CTLREQS];
	struct hvn_nvs_rndis		sc_data_msg;

	int				sc_rss_ind_size;
	uint32_t			sc_rss_hash; /* setting, NDIS_HASH_ */
	uint32_t			sc_rss_hcap; /* caps, NDIS_HASH_ */
	struct ndis_rssprm_toeplitz	sc_rss;

	/* Rx ring */
	uint8_t				*sc_rx_ring;
	int				sc_rx_size;
	uint32_t			sc_rx_hndl;
	struct hyperv_dma		sc_rx_dma;
	struct hvn_rx_ring		*sc_rxr;
	int				sc_nrxr;
	int				sc_nrxr_inuse;

	/* Tx ring */
	struct hvn_tx_ring		*sc_txr;
	int				sc_ntxr;
	int				sc_ntxr_inuse;

	/* chimney sending buffers */
	uint8_t				*sc_chim;
	uint32_t			sc_chim_hndl;
	struct hyperv_dma		sc_chim_dma;
	kmutex_t			sc_chim_bmap_lock;
	u_long				*sc_chim_bmap;
	int				sc_chim_bmap_cnt;
	int				sc_chim_cnt;
	int				sc_chim_szmax;

	/* Packet transmission aggregation user settings. */
	int				sc_agg_size;
	int				sc_agg_pkts;
};

#define SC2IFP(_sc_)	(&(_sc_)->sc_ec.ec_if)
#define IFP2SC(_ifp_)	((_ifp_)->if_softc)

#ifndef HVN_TX_PROCESS_LIMIT_DEFAULT
#define HVN_TX_PROCESS_LIMIT_DEFAULT		128
#endif
#ifndef HVN_RX_PROCESS_LIMIT_DEFAULT
#define HVN_RX_PROCESS_LIMIT_DEFAULT		128
#endif
#ifndef HVN_TX_INTR_PROCESS_LIMIT_DEFAULT
#define HVN_TX_INTR_PROCESS_LIMIT_DEFAULT	256
#endif
#ifndef HVN_RX_INTR_PROCESS_LIMIT_DEFAULT
#define HVN_RX_INTR_PROCESS_LIMIT_DEFAULT	256
#endif

/*
 * See hvn_set_hlen().
 *
 * This value is for Azure.  For Hyper-V, set this above
 * 65536 to disable UDP datagram checksum fixup.
 */
#ifndef HVN_UDP_CKSUM_FIXUP_MTU_DEFAULT
#define HVN_UDP_CKSUM_FIXUP_MTU_DEFAULT	1420
#endif
static int hvn_udpcs_fixup_mtu = HVN_UDP_CKSUM_FIXUP_MTU_DEFAULT;

/* Limit chimney send size */
static int hvn_tx_chimney_size = 0;

/* # of channels to use; each channel has one RX ring and one TX ring */
#ifndef HVN_CHANNEL_COUNT_DEFAULT
#define HVN_CHANNEL_COUNT_DEFAULT	0
#endif
static int hvn_channel_cnt = HVN_CHANNEL_COUNT_DEFAULT;

/* # of transmit rings to use */
#ifndef HVN_TX_RING_COUNT_DEFAULT
#define HVN_TX_RING_COUNT_DEFAULT	0
#endif
static int hvn_tx_ring_cnt = HVN_TX_RING_COUNT_DEFAULT;

/* Packet transmission aggregation size limit */
static int hvn_tx_agg_size = -1;

/* Packet transmission aggregation count limit */
static int hvn_tx_agg_pkts = -1;

static int	hvn_match(device_t, cfdata_t, void *);
static void	hvn_attach(device_t, device_t, void *);
static int	hvn_detach(device_t, int);

CFATTACH_DECL_NEW(hvn, sizeof(struct hvn_softc),
    hvn_match, hvn_attach, hvn_detach, NULL);

static int	hvn_ioctl(struct ifnet *, u_long, void *);
static int	hvn_media_change(struct ifnet *);
static void	hvn_media_status(struct ifnet *, struct ifmediareq *);
static void	hvn_link_task(void *);
static void	hvn_link_event(struct hvn_softc *, uint32_t);
static void	hvn_link_netchg_tmout_cb(void *);
static int	hvn_init(struct ifnet *);
static int	hvn_init_locked(struct ifnet *);
static void	hvn_stop(struct ifnet *, int);
static void	hvn_stop_locked(struct ifnet *);
static void	hvn_start(struct ifnet *);
static int	hvn_transmit(struct ifnet *, struct mbuf *);
static void	hvn_deferred_transmit(void *);
static int	hvn_flush_txagg(struct hvn_tx_ring *);
static int	hvn_encap(struct hvn_tx_ring *, struct hvn_tx_desc *,
		    struct mbuf *, int);
static int	hvn_txpkt(struct hvn_tx_ring *, struct hvn_tx_desc *);
static void	hvn_txeof(struct hvn_tx_ring *, uint64_t);
static int	hvn_rx_ring_create(struct hvn_softc *, int);
static int	hvn_rx_ring_destroy(struct hvn_softc *);
static void	hvn_fixup_rx_data(struct hvn_softc *);
static int	hvn_tx_ring_create(struct hvn_softc *, int);
static void	hvn_tx_ring_destroy(struct hvn_softc *);
static void	hvn_set_chim_size(struct hvn_softc *, int);
static uint32_t	hvn_chim_alloc(struct hvn_softc *);
static void	hvn_chim_free(struct hvn_softc *, uint32_t);
static void	hvn_fixup_tx_data(struct hvn_softc *);
static struct mbuf *
		hvn_set_hlen(struct mbuf *, int *);
static int	hvn_txd_peek(struct hvn_tx_ring *);
static struct hvn_tx_desc *
		hvn_txd_get(struct hvn_tx_ring *);
static void	hvn_txd_put(struct hvn_tx_ring *, struct hvn_tx_desc *);
static void	hvn_txd_gc(struct hvn_tx_ring *, struct hvn_tx_desc *);
static void	hvn_txd_hold(struct hvn_tx_desc *);
static void	hvn_txd_agg(struct hvn_tx_desc *, struct hvn_tx_desc *);
static int	hvn_tx_ring_pending(struct hvn_tx_ring *);
static void	hvn_tx_ring_qflush(struct hvn_softc *, struct hvn_tx_ring *);
static int	hvn_get_rsscaps(struct hvn_softc *, int *);
static int	hvn_set_rss(struct hvn_softc *, uint16_t);
static void	hvn_fixup_rss_ind(struct hvn_softc *);
static int	hvn_get_hwcaps(struct hvn_softc *, struct ndis_offload *);
static int	hvn_set_capabilities(struct hvn_softc *, int);
static int	hvn_get_lladdr(struct hvn_softc *, uint8_t *);
static void	hvn_update_link_status(struct hvn_softc *);
static int	hvn_get_mtu(struct hvn_softc *, uint32_t *);
static int	hvn_channel_attach(struct hvn_softc *, struct vmbus_channel *);
static void	hvn_channel_detach(struct hvn_softc *, struct vmbus_channel *);
static void	hvn_channel_detach_all(struct hvn_softc *);
static int	hvn_subchannel_attach(struct hvn_softc *);
static int	hvn_synth_alloc_subchannels(struct hvn_softc *, int *);
static int	hvn_synth_attachable(const struct hvn_softc *);
static int	hvn_synth_attach(struct hvn_softc *, int);
static void	hvn_synth_detach(struct hvn_softc *);
static void	hvn_set_ring_inuse(struct hvn_softc *, int);
static void	hvn_disable_rx(struct hvn_softc *);
static void	hvn_drain_rxtx(struct hvn_softc *, int );
static void	hvn_suspend_data(struct hvn_softc *);
static void	hvn_suspend_mgmt(struct hvn_softc *);
static void	hvn_suspend(struct hvn_softc *) __unused;
static void	hvn_resume_tx(struct hvn_softc *, int);
static void	hvn_resume_data(struct hvn_softc *);
static void	hvn_resume_mgmt(struct hvn_softc *);
static void	hvn_resume(struct hvn_softc *) __unused;
static void	hvn_init_sysctls(struct hvn_softc *);

/* NSVP */
static int	hvn_nvs_init(struct hvn_softc *);
static void	hvn_nvs_destroy(struct hvn_softc *);
static int	hvn_nvs_attach(struct hvn_softc *, int);
static int	hvn_nvs_connect_rxbuf(struct hvn_softc *);
static int	hvn_nvs_disconnect_rxbuf(struct hvn_softc *);
static int	hvn_nvs_connect_chim(struct hvn_softc *);
static int	hvn_nvs_disconnect_chim(struct hvn_softc *);
static void	hvn_handle_ring_work(struct work *, void *);
static void	hvn_nvs_softintr(void *);
static void	hvn_nvs_intr(void *);
static void	hvn_nvs_intr1(struct hvn_rx_ring *, int, int);
static int	hvn_nvs_cmd(struct hvn_softc *, void *, size_t, uint64_t,
		    u_int);
static int	hvn_nvs_ack(struct hvn_rx_ring *, uint64_t);
static void	hvn_nvs_detach(struct hvn_softc *);
static int	hvn_nvs_alloc_subchannels(struct hvn_softc *, int *);

/* RNDIS */
static int	hvn_rndis_init(struct hvn_softc *);
static void	hvn_rndis_destroy(struct hvn_softc *);
static int	hvn_rndis_attach(struct hvn_softc *, int);
static int	hvn_rndis_cmd(struct hvn_softc *, struct rndis_cmd *, u_int);
static int	hvn_rndis_input(struct hvn_rx_ring *, uint64_t, void *);
static int	hvn_rxeof(struct hvn_rx_ring *, uint8_t *, uint32_t);
static void	hvn_rndis_complete(struct hvn_softc *, uint8_t *, uint32_t);
static int	hvn_rndis_output_sgl(struct hvn_tx_ring *,
		    struct hvn_tx_desc *);
static int	hvn_rndis_output_chim(struct hvn_tx_ring *,
		    struct hvn_tx_desc *);
static void	hvn_rndis_status(struct hvn_softc *, uint8_t *, uint32_t);
static int	hvn_rndis_query(struct hvn_softc *, uint32_t, void *, size_t *);
static int	hvn_rndis_query2(struct hvn_softc *, uint32_t, const void *,
		    size_t, void *, size_t *, size_t);
static int	hvn_rndis_set(struct hvn_softc *, uint32_t, void *, size_t);
static int	hvn_rndis_open(struct hvn_softc *);
static int	hvn_rndis_close(struct hvn_softc *);
static void	hvn_rndis_detach(struct hvn_softc *);

static int
hvn_match(device_t parent, cfdata_t match, void *aux)
{
	struct vmbus_attach_args *aa = aux;

	if (memcmp(aa->aa_type, &hyperv_guid_network, sizeof(*aa->aa_type)))
		return 0;
	return 1;
}

static void
hvn_attach(device_t parent, device_t self, void *aux)
{
	struct hvn_softc *sc = device_private(self);
	struct vmbus_attach_args *aa = aux;
	struct ifnet *ifp = SC2IFP(sc);
	char xnamebuf[32];
	uint8_t enaddr[ETHER_ADDR_LEN];
	uint32_t mtu;
	int tx_ring_cnt, ring_cnt;
	int error;

	sc->sc_dev = self;
	sc->sc_vmbus = (struct vmbus_softc *)device_private(parent);
	sc->sc_prichan = aa->aa_chan;
	sc->sc_dmat = sc->sc_vmbus->sc_dmat;

	aprint_naive("\n");
	aprint_normal(": Hyper-V NetVSC\n");

	sc->sc_txrx_workqueue = true;
	sc->sc_tx_process_limit = HVN_TX_PROCESS_LIMIT_DEFAULT;
	sc->sc_rx_process_limit = HVN_RX_PROCESS_LIMIT_DEFAULT;
	sc->sc_tx_intr_process_limit = HVN_TX_INTR_PROCESS_LIMIT_DEFAULT;
	sc->sc_rx_intr_process_limit = HVN_RX_INTR_PROCESS_LIMIT_DEFAULT;
	sc->sc_agg_size = hvn_tx_agg_size;
	sc->sc_agg_pkts = hvn_tx_agg_pkts;

	mutex_init(&sc->sc_core_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	mutex_init(&sc->sc_link_lock, MUTEX_DEFAULT, IPL_NET);
	cv_init(&sc->sc_link_cv, "hvnknkcv");
	callout_init(&sc->sc_link_tmout, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_link_tmout, hvn_link_netchg_tmout_cb, sc);
	if (kthread_create(PRI_NONE, KTHREAD_MUSTJOIN | KTHREAD_MPSAFE, NULL,
	    hvn_link_task, sc, &sc->sc_link_lwp, "%slink",
	    device_xname(self))) {
		aprint_error_dev(self, "failed to create link thread\n");
		return;
	}

	snprintf(xnamebuf, sizeof(xnamebuf), "%srxtx", device_xname(self));
	if (workqueue_create(&sc->sc_wq, xnamebuf, hvn_handle_ring_work,
	    sc, HVN_WORKQUEUE_PRI, IPL_NET, WQ_PERCPU | WQ_MPSAFE)) {
		aprint_error_dev(self, "failed to create workqueue\n");
		sc->sc_wq = NULL;
		goto destroy_link_thread;
	}

	ring_cnt = hvn_channel_cnt;
	if (ring_cnt <= 0) {
		ring_cnt = ncpu;
		if (ring_cnt > HVN_CHANNEL_MAX_COUNT_DEFAULT)
			ring_cnt = HVN_CHANNEL_MAX_COUNT_DEFAULT;
	} else if (ring_cnt > ncpu)
		ring_cnt = ncpu;

	tx_ring_cnt = hvn_tx_ring_cnt;
	if (tx_ring_cnt <= 0 || tx_ring_cnt > ring_cnt)
		tx_ring_cnt = ring_cnt;

	if (hvn_tx_ring_create(sc, tx_ring_cnt)) {
		aprint_error_dev(self, "failed to create Tx ring\n");
		goto destroy_wq;
	}

	if (hvn_rx_ring_create(sc, ring_cnt)) {
		aprint_error_dev(self, "failed to create Rx ring\n");
		goto destroy_tx_ring;
	}

	strlcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_extflags = IFEF_MPSAFE;
	ifp->if_ioctl = hvn_ioctl;
	ifp->if_start = hvn_start;
	ifp->if_transmit = hvn_transmit;
	ifp->if_init = hvn_init;
	ifp->if_stop = hvn_stop;
	ifp->if_baudrate = IF_Gbps(10);

	IFQ_SET_MAXLEN(&ifp->if_snd, uimax(HVN_TX_DESC - 1, IFQ_MAXLEN));
	IFQ_SET_READY(&ifp->if_snd);

	/* Initialize ifmedia structures. */
	sc->sc_ec.ec_ifmedia = &sc->sc_media;
	ifmedia_init_with_lock(&sc->sc_media, IFM_IMASK,
	    hvn_media_change, hvn_media_status, &sc->sc_core_lock);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_AUTO, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_10G_T | IFM_FDX, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_10G_T, 0, NULL);
	ifmedia_set(&sc->sc_media, IFM_ETHER | IFM_AUTO);

	if_initialize(ifp);
	sc->sc_ipq = if_percpuq_create(ifp);
	if_deferred_start_init(ifp, NULL);

	hvn_nvs_init(sc);
	hvn_rndis_init(sc);
	if (hvn_synth_attach(sc, ETHERMTU)) {
		aprint_error_dev(self, "failed to attach synth\n");
		goto destroy_if_percpuq;
	}

	aprint_normal_dev(self, "NVS %d.%d NDIS %d.%d\n",
	    sc->sc_proto >> 16, sc->sc_proto & 0xffff,
	    sc->sc_ndisver >> 16 , sc->sc_ndisver & 0xffff);

	if (hvn_get_lladdr(sc, enaddr)) {
		aprint_error_dev(self,
		    "failed to obtain an ethernet address\n");
		goto detach_synth;
	}
	aprint_normal_dev(self, "Ethernet address %s\n", ether_sprintf(enaddr));

	/*
	 * Fixup TX/RX stuffs after synthetic parts are attached.
	 */
	hvn_fixup_tx_data(sc);
	hvn_fixup_rx_data(sc);

	ifp->if_capabilities |= sc->sc_txr[0].txr_caps_assist &
		(IFCAP_CSUM_IPv4_Tx | IFCAP_CSUM_IPv4_Rx |
		 IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_TCPv4_Rx |
		 IFCAP_CSUM_TCPv6_Tx | IFCAP_CSUM_TCPv6_Rx |
		 IFCAP_CSUM_UDPv4_Tx | IFCAP_CSUM_UDPv4_Rx |
		 IFCAP_CSUM_UDPv6_Tx | IFCAP_CSUM_UDPv6_Rx);
	/* XXX TSOv4, TSOv6 */
	if (sc->sc_caps & HVN_CAPS_VLAN) {
		/* XXX not sure about VLAN_MTU. */
		sc->sc_ec.ec_capabilities |= ETHERCAP_VLAN_HWTAGGING;
		sc->sc_ec.ec_capabilities |= ETHERCAP_VLAN_MTU;
	}
	sc->sc_ec.ec_capabilities |= ETHERCAP_JUMBO_MTU;

	ether_ifattach(ifp, enaddr);

	error = hvn_get_mtu(sc, &mtu);
	if (error)
		mtu = ETHERMTU;
	if (mtu < ETHERMTU) {
		DPRINTF("%s: fixup mtu %u -> %u\n", device_xname(sc->sc_dev),
		    ETHERMTU, mtu);
		ifp->if_mtu = mtu;
	}

	if_register(ifp);

	/*
	 * Kick off link status check.
	 */
	hvn_link_event(sc, HVN_LINK_EV_STATE_CHANGE);

	hvn_init_sysctls(sc);

	if (pmf_device_register(self, NULL, NULL))
		pmf_class_network_register(self, ifp);
	else
		aprint_error_dev(self, "couldn't establish power handler\n");

	SET(sc->sc_flags, HVN_SCF_ATTACHED);
	return;

detach_synth:
	hvn_synth_detach(sc);
	hvn_rndis_destroy(sc);
	hvn_nvs_destroy(sc);
destroy_if_percpuq:
	if_percpuq_destroy(sc->sc_ipq);
	hvn_rx_ring_destroy(sc);
destroy_tx_ring:
	hvn_tx_ring_destroy(sc);
destroy_wq:
	workqueue_destroy(sc->sc_wq);
	sc->sc_wq = NULL;
destroy_link_thread:
	hvn_link_event(sc, HVN_LINK_EV_EXIT_THREAD);
	kthread_join(sc->sc_link_lwp);
	callout_destroy(&sc->sc_link_tmout);
	cv_destroy(&sc->sc_link_cv);
	mutex_destroy(&sc->sc_link_lock);
	mutex_destroy(&sc->sc_core_lock);
}

static int
hvn_detach(device_t self, int flags)
{
	struct hvn_softc *sc = device_private(self);
	struct ifnet *ifp = SC2IFP(sc);

	if (!ISSET(sc->sc_flags, HVN_SCF_ATTACHED))
		return 0;

	if (vmbus_channel_is_revoked(sc->sc_prichan))
		SET(sc->sc_flags, HVN_SCF_REVOKED);

	pmf_device_deregister(self);

	mutex_enter(&sc->sc_core_lock);

	if (ifp->if_flags & IFF_RUNNING)
		hvn_stop_locked(ifp);
	/*
	 * NOTE:
	 * hvn_stop() only suspends data, so managment
	 * stuffs have to be suspended manually here.
	 */
	hvn_suspend_mgmt(sc);

	ether_ifdetach(ifp);
	if_detach(ifp);
	if_percpuq_destroy(sc->sc_ipq);

	hvn_link_event(sc, HVN_LINK_EV_EXIT_THREAD);
	kthread_join(sc->sc_link_lwp);
	callout_halt(&sc->sc_link_tmout, NULL);

	hvn_synth_detach(sc);
	hvn_rndis_destroy(sc);
	hvn_nvs_destroy(sc);

	mutex_exit(&sc->sc_core_lock);

	hvn_rx_ring_destroy(sc);
	hvn_tx_ring_destroy(sc);

	workqueue_destroy(sc->sc_wq);
	callout_destroy(&sc->sc_link_tmout);
	cv_destroy(&sc->sc_link_cv);
	mutex_destroy(&sc->sc_link_lock);
	mutex_destroy(&sc->sc_core_lock);

	sysctl_teardown(&sc->sc_sysctllog);

	return 0;
}

static int
hvn_ioctl(struct ifnet *ifp, u_long command, void * data)
{
	struct hvn_softc *sc = IFP2SC(ifp);
	struct ifreq *ifr = (struct ifreq *)data;
	uint32_t mtu;
	int s, error = 0;

	switch (command) {
	case SIOCSIFMTU:
		if (ifr->ifr_mtu < HVN_MTU_MIN || ifr->ifr_mtu > HVN_MTU_MAX) {
			error = EINVAL;
			break;
		}

		mutex_enter(&sc->sc_core_lock);

		if (!(sc->sc_caps & HVN_CAPS_MTU)) {
			/* Can't change MTU */
			mutex_exit(&sc->sc_core_lock);
			error = EOPNOTSUPP;
			break;
		}

		if (ifp->if_mtu == ifr->ifr_mtu) {
			mutex_exit(&sc->sc_core_lock);
			break;
		}

		/*
		 * Suspend this interface before the synthetic parts
		 * are ripped.
		 */
		hvn_suspend(sc);

		/*
		 * Detach the synthetics parts, i.e. NVS and RNDIS.
		 */
		hvn_synth_detach(sc);

		/*
		 * Reattach the synthetic parts, i.e. NVS and RNDIS,
		 * with the new MTU setting.
		 */
		error = hvn_synth_attach(sc, ifr->ifr_mtu);
		if (error) {
			mutex_exit(&sc->sc_core_lock);
			break;
		}

		error = hvn_get_mtu(sc, &mtu);
		if (error)
			mtu = ifr->ifr_mtu;
		DPRINTF("%s: RNDIS mtu=%d\n", device_xname(sc->sc_dev), mtu);

		/*
		 * Commit the requested MTU, after the synthetic parts
		 * have been successfully attached.
		 */
		if (mtu >= ifr->ifr_mtu) {
			mtu = ifr->ifr_mtu;
		} else {
			DPRINTF("%s: fixup mtu %d -> %u\n",
			    device_xname(sc->sc_dev), ifr->ifr_mtu, mtu);
		}
		ifp->if_mtu = mtu;

		/*
		 * Synthetic parts' reattach may change the chimney
		 * sending size; update it.
		 */
		if (sc->sc_txr[0].txr_chim_size > sc->sc_chim_szmax)
			hvn_set_chim_size(sc, sc->sc_chim_szmax);

		/*
		 * All done!  Resume the interface now.
		 */
		hvn_resume(sc);

		mutex_exit(&sc->sc_core_lock);
		break;
	default:
		s = splnet();
		if (command == SIOCGIFMEDIA || command == SIOCSIFMEDIA)
			error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, command);
		else
			error = ether_ioctl(ifp, command, data);
		splx(s);
		if (error == ENETRESET) {
			mutex_enter(&sc->sc_core_lock);
			if (ifp->if_flags & IFF_RUNNING)
				hvn_init_locked(ifp);
			mutex_exit(&sc->sc_core_lock);
			error = 0;
		}
		break;
	}

	return error;
}

static int
hvn_media_change(struct ifnet *ifp)
{
	struct hvn_softc *sc = IFP2SC(ifp);
	struct ifmedia *ifm = &sc->sc_media;

	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return EINVAL;

	switch (IFM_SUBTYPE(ifm->ifm_media)) {
	case IFM_AUTO:
		break;
	default:
		device_printf(sc->sc_dev, "Only auto media type\n");
		return EINVAL;
	}
	return 0;
}

static void
hvn_media_status(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct hvn_softc *sc = IFP2SC(ifp);

	ifmr->ifm_status = IFM_AVALID;
	ifmr->ifm_active = IFM_ETHER;

	if (sc->sc_link_state != LINK_STATE_UP) {
		ifmr->ifm_active |= IFM_NONE;
		return;
	}

	ifmr->ifm_status |= IFM_ACTIVE;
	ifmr->ifm_active |= IFM_10G_T | IFM_FDX;
}

static void
hvn_link_task(void *arg)
{
	struct hvn_softc *sc = arg;
	struct ifnet *ifp = SC2IFP(sc);
	uint32_t event;
	int old_link_state;

	mutex_enter(&sc->sc_link_lock);
	sc->sc_link_onproc = false;
	for (;;) {
		if (sc->sc_link_ev == 0) {
			cv_wait(&sc->sc_link_cv, &sc->sc_link_lock);
			continue;
		}

		sc->sc_link_onproc = true;
		event = sc->sc_link_ev;
		sc->sc_link_ev = 0;
		mutex_exit(&sc->sc_link_lock);

		if (event & HVN_LINK_EV_EXIT_THREAD)
			break;

		if (sc->sc_link_suspend)
			goto next;

		if (event & HVN_LINK_EV_RESUME_NETWORK) {
			if (sc->sc_link_pending)
				event |= HVN_LINK_EV_NETWORK_CHANGE;
			else
				event |= HVN_LINK_EV_STATE_CHANGE;
		}

		if (event & HVN_LINK_EV_NETWORK_CHANGE) {
			/* Prevent any link status checks from running. */
			sc->sc_link_pending = true;

			/*
			 * Fake up a [link down --> link up] state change;
			 * 5 seconds delay is used, which closely simulates
			 * miibus reaction upon link down event.
			 */
			old_link_state = sc->sc_link_state;
			sc->sc_link_state = LINK_STATE_DOWN;
			if (old_link_state != sc->sc_link_state) {
				if_link_state_change(ifp, LINK_STATE_DOWN);
			}
#if defined(HVN_LINK_STATE_CHANGE_DELAY) && HVN_LINK_STATE_CHANGE_DELAY > 0
			callout_schedule(&sc->sc_link_tmout,
			    mstohz(HVN_LINK_STATE_CHANGE_DELAY));
#else
			hvn_link_event(sc, HVN_LINK_EV_NETWORK_CHANGE_TMOUT);
#endif
		} else if (event & HVN_LINK_EV_NETWORK_CHANGE_TMOUT) {
			/* Re-allow link status checks. */
			sc->sc_link_pending = false;
			hvn_update_link_status(sc);
		} else if (event & HVN_LINK_EV_STATE_CHANGE) {
			if (!sc->sc_link_pending)
				hvn_update_link_status(sc);
		}
 next:
		mutex_enter(&sc->sc_link_lock);
		sc->sc_link_onproc = false;
	}

	mutex_enter(&sc->sc_link_lock);
	sc->sc_link_onproc = false;
	mutex_exit(&sc->sc_link_lock);

	kthread_exit(0);
}

static void
hvn_link_event(struct hvn_softc *sc, uint32_t ev)
{

	mutex_enter(&sc->sc_link_lock);
	SET(sc->sc_link_ev, ev);
	cv_signal(&sc->sc_link_cv);
	mutex_exit(&sc->sc_link_lock);
}

static void
hvn_link_netchg_tmout_cb(void *arg)
{
	struct hvn_softc *sc = arg;

	hvn_link_event(sc, HVN_LINK_EV_NETWORK_CHANGE_TMOUT);
}

static int
hvn_init(struct ifnet *ifp)
{
	struct hvn_softc *sc = IFP2SC(ifp);
	int error;

	mutex_enter(&sc->sc_core_lock);
	error = hvn_init_locked(ifp);
	mutex_exit(&sc->sc_core_lock);

	return error;
}

static int
hvn_init_locked(struct ifnet *ifp)
{
	struct hvn_softc *sc = IFP2SC(ifp);
	int error;

	KASSERT(mutex_owned(&sc->sc_core_lock));

	hvn_stop_locked(ifp);

	error = hvn_rndis_open(sc);
	if (error)
		return error;

	/* Clear OACTIVE bit. */
	ifp->if_flags &= ~IFF_OACTIVE;

	/* Clear TX 'suspended' bit. */
	hvn_resume_tx(sc, sc->sc_ntxr_inuse);

	/* Everything is ready; unleash! */
	ifp->if_flags |= IFF_RUNNING;

	return 0;
}

static void
hvn_stop(struct ifnet *ifp, int disable)
{
	struct hvn_softc *sc = IFP2SC(ifp);

	mutex_enter(&sc->sc_core_lock);
	hvn_stop_locked(ifp);
	mutex_exit(&sc->sc_core_lock);
}

static void
hvn_stop_locked(struct ifnet *ifp)
{
	struct hvn_softc *sc = IFP2SC(ifp);
	int i;

	KASSERT(mutex_owned(&sc->sc_core_lock));

	/* Clear RUNNING bit ASAP. */
	ifp->if_flags &= ~IFF_RUNNING;

	/* Suspend data transfers. */
	hvn_suspend_data(sc);

	/* Clear OACTIVE bit. */
	ifp->if_flags &= ~IFF_OACTIVE;
	for (i = 0; i < sc->sc_ntxr_inuse; i++)
		sc->sc_txr[i].txr_oactive = 0;
}

static void
hvn_transmit_common(struct ifnet *ifp, struct hvn_tx_ring *txr,
    bool is_transmit)
{
	struct hvn_tx_desc *txd;
	struct mbuf *m;
	int l2hlen = ETHER_HDR_LEN;

	KASSERT(mutex_owned(&txr->txr_lock));

	if (!(ifp->if_flags & IFF_RUNNING))
		return;
	if (!is_transmit && (ifp->if_flags & IFF_OACTIVE))
		return;
	if (txr->txr_oactive)
		return;
	if (txr->txr_suspended)
		return;

	for (;;) {
		if (!hvn_txd_peek(txr)) {
			/* transient */
			if (!is_transmit)
				ifp->if_flags |= IFF_OACTIVE;
			txr->txr_oactive = 1;
			txr->txr_evnodesc.ev_count++;
			break;
		}

		if (is_transmit)
			m = pcq_get(txr->txr_interq);
		else
			IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

#if defined(INET) || defined(INET6)
		if (m->m_pkthdr.csum_flags &
		    (M_CSUM_TCPv4|M_CSUM_UDPv4|M_CSUM_TCPv6|M_CSUM_UDPv6)) {
			m = hvn_set_hlen(m, &l2hlen);
			if (__predict_false(m == NULL)) {
				if_statinc(ifp, if_oerrors);
				continue;
			}
		}
#endif

		txd = hvn_txd_get(txr);
		if (hvn_encap(txr, txd, m, l2hlen)) {
			/* the chain is too large */
			if_statinc(ifp, if_oerrors);
			hvn_txd_put(txr, txd);
			m_freem(m);
			continue;
		}

		if (txr->txr_agg_pktleft == 0) {
			if (txr->txr_agg_txd != NULL) {
				hvn_flush_txagg(txr);
			} else {
				if (hvn_txpkt(txr, txd)) {
					/* txd is freed, but m is not. */
					m_freem(m);
					if_statinc(ifp, if_oerrors);
				}
			}
		}
	}

	/* Flush pending aggerated transmission. */
	if (txr->txr_agg_txd != NULL)
		hvn_flush_txagg(txr);
}

static void
hvn_start(struct ifnet *ifp)
{
	struct hvn_softc *sc = IFP2SC(ifp);
	struct hvn_tx_ring *txr = &sc->sc_txr[0];

	mutex_enter(&txr->txr_lock);
	hvn_transmit_common(ifp, txr, false);
	mutex_exit(&txr->txr_lock);
}

static int
hvn_select_txqueue(struct ifnet *ifp, struct mbuf *m __unused)
{
	struct hvn_softc *sc = IFP2SC(ifp);
	u_int cpu;

	cpu = cpu_index(curcpu());

	return cpu % sc->sc_ntxr_inuse;
}

static int
hvn_transmit(struct ifnet *ifp, struct mbuf *m)
{
	struct hvn_softc *sc = IFP2SC(ifp);
	struct hvn_tx_ring *txr;
	int qid;

	qid = hvn_select_txqueue(ifp, m);
	txr = &sc->sc_txr[qid];

	if (__predict_false(!pcq_put(txr->txr_interq, m))) {
		mutex_enter(&txr->txr_lock);
		txr->txr_evpcqdrop.ev_count++;
		mutex_exit(&txr->txr_lock);
		m_freem(m);
		return ENOBUFS;
	}

	kpreempt_disable();
	softint_schedule(txr->txr_si);
	kpreempt_enable();
	return 0;
}

static void
hvn_deferred_transmit(void *arg)
{
	struct hvn_tx_ring *txr = arg;
	struct hvn_softc *sc = txr->txr_softc;
	struct ifnet *ifp = SC2IFP(sc);

	mutex_enter(&txr->txr_lock);
	txr->txr_evtransmitdefer.ev_count++;
	hvn_transmit_common(ifp, txr, true);
	mutex_exit(&txr->txr_lock);
}

static inline char *
hvn_rndis_pktinfo_append(struct rndis_packet_msg *pkt, size_t pktsize,
    size_t datalen, uint32_t type)
{
	struct rndis_pktinfo *pi;
	size_t pi_size = sizeof(*pi) + datalen;
	char *cp;

	KASSERT(pkt->rm_pktinfooffset + pkt->rm_pktinfolen + pi_size <=
	    pktsize);

	cp = (char *)pkt + pkt->rm_pktinfooffset + pkt->rm_pktinfolen;
	pi = (struct rndis_pktinfo *)cp;
	pi->rm_size = pi_size;
	pi->rm_type = type;
	pi->rm_pktinfooffset = sizeof(*pi);
	pkt->rm_pktinfolen += pi_size;
	pkt->rm_dataoffset += pi_size;
	pkt->rm_len += pi_size;

	return (char *)pi->rm_data;
}

static struct mbuf *
hvn_pullup_hdr(struct mbuf *m, int len)
{
	struct mbuf *mn;

	if (__predict_false(m->m_len < len)) {
		mn = m_pullup(m, len);
		if (mn == NULL)
			return NULL;
		m = mn;
	}
	return m;
}

/*
 * NOTE: If this function failed, the m would be freed.
 */
static struct mbuf *
hvn_set_hlen(struct mbuf *m, int *l2hlenp)
{
	const struct ether_header *eh;
	int l2hlen, off;

	m = hvn_pullup_hdr(m, sizeof(*eh));
	if (m == NULL)
		return NULL;

	eh = mtod(m, const struct ether_header *);
	if (eh->ether_type == ntohs(ETHERTYPE_VLAN))
		l2hlen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
	else
		l2hlen = ETHER_HDR_LEN;

#if defined(INET)
	if (m->m_pkthdr.csum_flags & (M_CSUM_TCPv4 | M_CSUM_UDPv4)) {
		const struct ip *ip;

		off = l2hlen + sizeof(*ip);
		m = hvn_pullup_hdr(m, off);
		if (m == NULL)
			return NULL;

		ip = (struct ip *)((mtod(m, uint8_t *)) + off);

		/*
		 * UDP checksum offload does not work in Azure, if the
		 * following conditions meet:
		 * - sizeof(IP hdr + UDP hdr + payload) > 1420.
		 * - IP_DF is not set in the IP hdr.
		 *
		 * Fallback to software checksum for these UDP datagrams.
		 */
		if ((m->m_pkthdr.csum_flags & M_CSUM_UDPv4) &&
		    m->m_pkthdr.len > hvn_udpcs_fixup_mtu + l2hlen &&
		    !(ntohs(ip->ip_off) & IP_DF)) {
			uint16_t *csump;

			off = l2hlen +
			    M_CSUM_DATA_IPv4_IPHL(m->m_pkthdr.csum_data);
			m = hvn_pullup_hdr(m, off + sizeof(struct udphdr));
			if (m == NULL)
				return NULL;

			csump = (uint16_t *)(mtod(m, uint8_t *) + off +
			    M_CSUM_DATA_IPv4_OFFSET(m->m_pkthdr.csum_data));
			*csump = cpu_in_cksum(m, m->m_pkthdr.len - off, off, 0);
			m->m_pkthdr.csum_flags &= ~M_CSUM_UDPv4;
		}
	}
#endif	/* INET */
#if defined(INET) && defined(INET6)
	else
#endif	/* INET && INET6 */
#if defined(INET6)
	{
		const struct ip6_hdr *ip6;

		off = l2hlen + sizeof(*ip6);
		m = hvn_pullup_hdr(m, off);
		if (m == NULL)
			return NULL;

		ip6 = (struct ip6_hdr *)((mtod(m, uint8_t *)) + l2hlen);
		if (ip6->ip6_nxt != IPPROTO_TCP &&
		    ip6->ip6_nxt != IPPROTO_UDP) {
			m_freem(m);
			return NULL;
		}
	}
#endif	/* INET6 */

	*l2hlenp = l2hlen;

	return m;
}

static int
hvn_flush_txagg(struct hvn_tx_ring *txr)
{
	struct hvn_softc *sc = txr->txr_softc;
	struct ifnet *ifp = SC2IFP(sc);
	struct hvn_tx_desc *txd;
	struct mbuf *m;
	int error, pkts;

	txd = txr->txr_agg_txd;
	KASSERTMSG(txd != NULL, "no aggregate txdesc");

	/*
	 * Since hvn_txpkt() will reset this temporary stat, save
	 * it now, so that oerrors can be updated properly, if
	 * hvn_txpkt() ever fails.
	 */
	pkts = txr->txr_stat_pkts;

	/*
	 * Since txd's mbuf will _not_ be freed upon hvn_txpkt()
	 * failure, save it for later freeing, if hvn_txpkt() ever
	 * fails.
	 */
	m = txd->txd_buf;
	error = hvn_txpkt(txr, txd);
	if (__predict_false(error)) {
		/* txd is freed, but m is not. */
		m_freem(m);
		txr->txr_evflushfailed.ev_count++;
		if_statadd(ifp, if_oerrors, pkts);
	}

	/* Reset all aggregation states. */
	txr->txr_agg_txd = NULL;
	txr->txr_agg_szleft = 0;
	txr->txr_agg_pktleft = 0;
	txr->txr_agg_prevpkt = NULL;

	return error;
}

static void *
hvn_try_txagg(struct hvn_tx_ring *txr, struct hvn_tx_desc *txd, int pktsz)
{
	struct hvn_softc *sc = txr->txr_softc;
	struct hvn_tx_desc *agg_txd;
	struct rndis_packet_msg *pkt;
	void *chim;
	int olen;

	if (txr->txr_agg_txd != NULL) {
		if (txr->txr_agg_pktleft > 0 && txr->txr_agg_szleft > pktsz) {
			agg_txd = txr->txr_agg_txd;
			pkt = txr->txr_agg_prevpkt;

			/*
			 * Update the previous RNDIS packet's total length,
			 * it can be increased due to the mandatory alignment
			 * padding for this RNDIS packet.  And update the
			 * aggregating txdesc's chimney sending buffer size
			 * accordingly.
			 *
			 * XXX
			 * Zero-out the padding, as required by the RNDIS spec.
			 */
			olen = pkt->rm_len;
			pkt->rm_len = roundup2(olen, txr->txr_agg_align);
			agg_txd->txd_chim_size += pkt->rm_len - olen;

			/* Link this txdesc to the parent. */
			hvn_txd_agg(agg_txd, txd);

			chim = (uint8_t *)pkt + pkt->rm_len;
			/* Save the current packet for later fixup. */
			txr->txr_agg_prevpkt = chim;

			txr->txr_agg_pktleft--;
			txr->txr_agg_szleft -= pktsz;
			if (txr->txr_agg_szleft <=
			    HVN_PKTSIZE_MIN(txr->txr_agg_align)) {
				/*
				 * Probably can't aggregate more packets,
				 * flush this aggregating txdesc proactively.
				 */
				txr->txr_agg_pktleft = 0;
			}

			/* Done! */
			return chim;
		}
		hvn_flush_txagg(txr);
	}

	txr->txr_evchimneytried.ev_count++;
	txd->txd_chim_index = hvn_chim_alloc(sc);
	if (txd->txd_chim_index == HVN_NVS_CHIM_IDX_INVALID)
		return NULL;
	txr->txr_evchimney.ev_count++;

	chim = sc->sc_chim + (txd->txd_chim_index * sc->sc_chim_szmax);

	if (txr->txr_agg_pktmax > 1 &&
	    txr->txr_agg_szmax > pktsz + HVN_PKTSIZE_MIN(txr->txr_agg_align)) {
		txr->txr_agg_txd = txd;
		txr->txr_agg_pktleft = txr->txr_agg_pktmax - 1;
		txr->txr_agg_szleft = txr->txr_agg_szmax - pktsz;
		txr->txr_agg_prevpkt = chim;
	}

	return chim;
}

static int
hvn_encap(struct hvn_tx_ring *txr, struct hvn_tx_desc *txd, struct mbuf *m,
    int l2hlen)
{
	/* Used to pad ethernet frames with < ETHER_MIN_LEN bytes */
	static const char zero_pad[ETHER_MIN_LEN];
	struct hvn_softc *sc = txr->txr_softc;
	struct rndis_packet_msg *pkt;
	bus_dma_segment_t *seg;
	void *chim = NULL;
	size_t pktlen, pktsize;
	int l3hlen;
	int i, rv;

	if (ISSET(sc->sc_caps, HVN_CAPS_VLAN) && !vlan_has_tag(m)) {
		struct ether_vlan_header *evl;

		m = hvn_pullup_hdr(m, sizeof(*evl));
		if (m == NULL) {
			DPRINTF("%s: failed to pullup mbuf\n",
			    device_xname(sc->sc_dev));
			return -1;
		}

		evl = mtod(m, struct ether_vlan_header *);
		if (evl->evl_encap_proto == htons(ETHERTYPE_VLAN)) {
			struct ether_header *eh;
			uint16_t proto = evl->evl_proto;

			vlan_set_tag(m, ntohs(evl->evl_tag));

			/*
			 * Trim VLAN tag from header.
			 */
			memmove((uint8_t *)evl + ETHER_VLAN_ENCAP_LEN,
			    evl, ETHER_HDR_LEN);
			m_adj(m, ETHER_VLAN_ENCAP_LEN);

			eh = mtod(m, struct ether_header *);
			eh->ether_type = proto;

			/*
			 * Re-padding.  See sys/net/if_vlan.c:vlan_start().
			 */
			if (m->m_pkthdr.len < (ETHER_MIN_LEN - ETHER_CRC_LEN +
			    ETHER_VLAN_ENCAP_LEN)) {
				m_copyback(m, m->m_pkthdr.len,
				    (ETHER_MIN_LEN - ETHER_CRC_LEN +
				     ETHER_VLAN_ENCAP_LEN) -
				    m->m_pkthdr.len, zero_pad);
			}

			txr->txr_evvlanfixup.ev_count++;
		}
	}

	pkt = txd->txd_req;
	pktsize = HVN_PKTSIZE(m, txr->txr_agg_align);
	if (pktsize < txr->txr_chim_size) {
		chim = hvn_try_txagg(txr, txd, pktsize);
		if (chim != NULL)
			pkt = chim;
	} else {
		if (txr->txr_agg_txd != NULL)
			hvn_flush_txagg(txr);
	}

	memset(pkt, 0, HVN_RNDIS_PKT_LEN);
	pkt->rm_type = REMOTE_NDIS_PACKET_MSG;
	pkt->rm_len = sizeof(*pkt) + m->m_pkthdr.len;
	pkt->rm_dataoffset = RNDIS_DATA_OFFSET;
	pkt->rm_datalen = m->m_pkthdr.len;
	pkt->rm_pktinfooffset = sizeof(*pkt); /* adjusted below */
	pkt->rm_pktinfolen = 0;

	if (txr->txr_flags & HVN_TXR_FLAG_UDP_HASH) {
		char *cp;

		/*
		 * Set the hash value for this packet, so that the host could
		 * dispatch the TX done event for this packet back to this TX
		 * ring's channel.
		 */
		cp = hvn_rndis_pktinfo_append(pkt, HVN_RNDIS_PKT_LEN,
		    HVN_NDIS_HASH_VALUE_SIZE, HVN_NDIS_PKTINFO_TYPE_HASHVAL);
		memcpy(cp, &txr->txr_id, HVN_NDIS_HASH_VALUE_SIZE);
	}

	if (vlan_has_tag(m)) {
		uint32_t vlan;
		char *cp;
		uint16_t tag;

		tag = vlan_get_tag(m);
		vlan = NDIS_VLAN_INFO_MAKE(EVL_VLANOFTAG(tag),
		    EVL_PRIOFTAG(tag), EVL_CFIOFTAG(tag));
		cp = hvn_rndis_pktinfo_append(pkt, HVN_RNDIS_PKT_LEN,
		    NDIS_VLAN_INFO_SIZE, NDIS_PKTINFO_TYPE_VLAN);
		memcpy(cp, &vlan, NDIS_VLAN_INFO_SIZE);
		txr->txr_evvlanhwtagging.ev_count++;
	}

	if (m->m_pkthdr.csum_flags & txr->txr_csum_assist) {
		uint32_t csum;
		char *cp;

		if (m->m_pkthdr.csum_flags & (M_CSUM_TCPv6 | M_CSUM_UDPv6)) {
			csum = NDIS_TXCSUM_INFO_IPV6;
			l3hlen = M_CSUM_DATA_IPv6_IPHL(m->m_pkthdr.csum_data);
			if (m->m_pkthdr.csum_flags & M_CSUM_TCPv6)
				csum |= NDIS_TXCSUM_INFO_MKTCPCS(l2hlen +
				    l3hlen);
			if (m->m_pkthdr.csum_flags & M_CSUM_UDPv6)
				csum |= NDIS_TXCSUM_INFO_MKUDPCS(l2hlen +
				    l3hlen);
		} else {
			csum = NDIS_TXCSUM_INFO_IPV4;
			l3hlen = M_CSUM_DATA_IPv4_IPHL(m->m_pkthdr.csum_data);
			if (m->m_pkthdr.csum_flags & M_CSUM_IPv4)
				csum |= NDIS_TXCSUM_INFO_IPCS;
			if (m->m_pkthdr.csum_flags & M_CSUM_TCPv4)
				csum |= NDIS_TXCSUM_INFO_MKTCPCS(l2hlen +
				    l3hlen);
			if (m->m_pkthdr.csum_flags & M_CSUM_UDPv4)
				csum |= NDIS_TXCSUM_INFO_MKUDPCS(l2hlen +
				    l3hlen);
		}
		cp = hvn_rndis_pktinfo_append(pkt, HVN_RNDIS_PKT_LEN,
		    NDIS_TXCSUM_INFO_SIZE, NDIS_PKTINFO_TYPE_CSUM);
		memcpy(cp, &csum, NDIS_TXCSUM_INFO_SIZE);
	}

	pktlen = pkt->rm_pktinfooffset + pkt->rm_pktinfolen;
	pkt->rm_pktinfooffset -= RNDIS_HEADER_OFFSET;

	/*
	 * Fast path: Chimney sending.
	 */
	if (chim != NULL) {
		struct hvn_tx_desc *tgt_txd;

		tgt_txd = (txr->txr_agg_txd != NULL) ? txr->txr_agg_txd : txd;

		KASSERTMSG(pkt == chim,
		    "RNDIS pkt not in chimney sending buffer");
		KASSERTMSG(tgt_txd->txd_chim_index != HVN_NVS_CHIM_IDX_INVALID,
		    "chimney sending buffer is not used");

		tgt_txd->txd_chim_size += pkt->rm_len;
		m_copydata(m, 0, m->m_pkthdr.len, (uint8_t *)chim + pktlen);

		txr->txr_sendpkt = hvn_rndis_output_chim;
		goto done;
	}

	KASSERTMSG(txr->txr_agg_txd == NULL, "aggregating sglist txdesc");
	KASSERTMSG(txd->txd_chim_index == HVN_NVS_CHIM_IDX_INVALID,
	    "chimney buffer is used");
	KASSERTMSG(pkt == txd->txd_req, "RNDIS pkt not in txdesc");

	rv = bus_dmamap_load_mbuf(sc->sc_dmat, txd->txd_dmap, m, BUS_DMA_READ |
	    BUS_DMA_NOWAIT);
	switch (rv) {
	case 0:
		break;
	case EFBIG:
		if (m_defrag(m, M_NOWAIT) != NULL) {
			txr->txr_evdefrag.ev_count++;
			if (bus_dmamap_load_mbuf(sc->sc_dmat, txd->txd_dmap, m,
			    BUS_DMA_READ | BUS_DMA_NOWAIT) == 0)
				break;
		}
		/* FALLTHROUGH */
	default:
		DPRINTF("%s: failed to load mbuf\n", device_xname(sc->sc_dev));
		txr->txr_evdmafailed.ev_count++;
		return -1;
	}
	bus_dmamap_sync(sc->sc_dmat, txd->txd_dmap,
	    0, txd->txd_dmap->dm_mapsize, BUS_DMASYNC_PREWRITE);
	SET(txd->txd_flags, HVN_TXD_FLAG_DMAMAP);

	/* Attach an RNDIS message to the first slot */
	txd->txd_sgl[0].gpa_page = txd->txd_gpa.gpa_page;
	txd->txd_sgl[0].gpa_ofs = txd->txd_gpa.gpa_ofs;
	txd->txd_sgl[0].gpa_len = pktlen;
	txd->txd_nsge = txd->txd_dmap->dm_nsegs + 1;

	for (i = 0; i < txd->txd_dmap->dm_nsegs; i++) {
		seg = &txd->txd_dmap->dm_segs[i];
		txd->txd_sgl[1 + i].gpa_page = atop(seg->ds_addr);
		txd->txd_sgl[1 + i].gpa_ofs = seg->ds_addr & PAGE_MASK;
		txd->txd_sgl[1 + i].gpa_len = seg->ds_len;
	}

	txd->txd_chim_index = HVN_NVS_CHIM_IDX_INVALID;
	txd->txd_chim_size = 0;
	txr->txr_sendpkt = hvn_rndis_output_sgl;
done:
	txd->txd_buf = m;

	/* Update temporary stats for later use. */
	txr->txr_stat_pkts++;
	txr->txr_stat_size += m->m_pkthdr.len;
	if (m->m_flags & M_MCAST)
		txr->txr_stat_mcasts++;

	return 0;
}

static void
hvn_bpf_mtap(struct hvn_tx_ring *txr, struct mbuf *m, u_int direction)
{
	struct hvn_softc *sc = txr->txr_softc;
	struct ifnet *ifp = SC2IFP(sc);
	struct ether_header *eh;
	struct ether_vlan_header evl;

	if (!vlan_has_tag(m)) {
		bpf_mtap(ifp, m, direction);
		return;
	}

	if (ifp->if_bpf == NULL)
		return;

	txr->txr_evvlantap.ev_count++;

	/*
	 * Restore a VLAN tag for bpf.
	 *
	 * Do not modify contents of the original mbuf,
	 * because Tx processing on the mbuf is still in progress.
	 */

	eh = mtod(m, struct ether_header *);
	memcpy(evl.evl_dhost, eh->ether_dhost, ETHER_ADDR_LEN * 2);
	evl.evl_encap_proto = htons(ETHERTYPE_VLAN);
	evl.evl_tag = htons(vlan_get_tag(m));
	evl.evl_proto = eh->ether_type;

	/* Do not tap ether header of the original mbuf. */
	m_adj(m, sizeof(*eh));

	bpf_mtap2(ifp->if_bpf, &evl, sizeof(evl), m, direction);

	/* Cannot restore ether header of the original mbuf,
	 * but do not worry about it because just free it. */
}

static int
hvn_txpkt(struct hvn_tx_ring *txr, struct hvn_tx_desc *txd)
{
	struct hvn_softc *sc = txr->txr_softc;
	struct ifnet *ifp = SC2IFP(sc);
	const struct hvn_tx_desc *tmp_txd;
	int error;

	/*
	 * Make sure that this txd and any aggregated txds are not
	 * freed before bpf_mtap.
	 */
	hvn_txd_hold(txd);

	error = (*txr->txr_sendpkt)(txr, txd);
	if (error == 0) {
		hvn_bpf_mtap(txr, txd->txd_buf, BPF_D_OUT);
		STAILQ_FOREACH(tmp_txd, &txd->txd_agg_list, txd_agg_entry)
			hvn_bpf_mtap(txr, tmp_txd->txd_buf, BPF_D_OUT);

		if_statadd(ifp, if_opackets, txr->txr_stat_pkts);
		if_statadd(ifp, if_obytes, txr->txr_stat_size);
		if (txr->txr_stat_mcasts != 0)
			if_statadd(ifp, if_omcasts, txr->txr_stat_mcasts);
		txr->txr_evpkts.ev_count += txr->txr_stat_pkts;
		txr->txr_evsends.ev_count++;
	}

	hvn_txd_put(txr, txd);

	if (__predict_false(error)) {
		/*
		 * Caller will perform further processing on the
		 * associated mbuf, so don't free it in hvn_txd_put();
		 * only unload it from the DMA map in hvn_txd_put(),
		 * if it was loaded.
		 */
		txd->txd_buf = NULL;
		hvn_txd_put(txr, txd);
	}

	/* Reset temporary stats, after this sending is done. */
	txr->txr_stat_pkts = 0;
	txr->txr_stat_size = 0;
	txr->txr_stat_mcasts = 0;

	return error;
}

static void
hvn_txeof(struct hvn_tx_ring *txr, uint64_t tid)
{
	struct hvn_softc *sc = txr->txr_softc;
	struct hvn_tx_desc *txd;
	uint32_t id = tid >> 32;

	if ((tid & 0xffffffffU) != 0)
		return;

	id -= HVN_NVS_CHIM_SIG;
	if (id >= HVN_TX_DESC) {
		device_printf(sc->sc_dev, "tx packet index too large: %u", id);
		return;
	}

	txd = &txr->txr_desc[id];

	if (txd->txd_buf == NULL)
		device_printf(sc->sc_dev, "no mbuf @%u\n", id);

	hvn_txd_put(txr, txd);
}

static int
hvn_rx_ring_create(struct hvn_softc *sc, int ring_cnt)
{
	struct hvn_rx_ring *rxr;
	int i;

	if (sc->sc_proto <= HVN_NVS_PROTO_VERSION_2)
		sc->sc_rx_size = 15 * 1024 * 1024;	/* 15MB */
	else
		sc->sc_rx_size = 16 * 1024 * 1024; 	/* 16MB */
	sc->sc_rx_ring = hyperv_dma_alloc(sc->sc_dmat, &sc->sc_rx_dma,
	    sc->sc_rx_size, PAGE_SIZE, PAGE_SIZE, sc->sc_rx_size / PAGE_SIZE);
	if (sc->sc_rx_ring == NULL) {
		DPRINTF("%s: failed to allocate Rx ring buffer\n",
		    device_xname(sc->sc_dev));
		return -1;
	}

	sc->sc_rxr = kmem_zalloc(sizeof(*rxr) * ring_cnt, KM_SLEEP);
	sc->sc_nrxr_inuse = sc->sc_nrxr = ring_cnt;

	for (i = 0; i < sc->sc_nrxr; i++) {
		rxr = &sc->sc_rxr[i];
		rxr->rxr_softc = sc;
		if (i < sc->sc_ntxr) {
			rxr->rxr_txr = &sc->sc_txr[i];
			rxr->rxr_txr->txr_rxr = rxr;
		}

		mutex_init(&rxr->rxr_lock, MUTEX_DEFAULT, IPL_NET);
		mutex_init(&rxr->rxr_onwork_lock, MUTEX_DEFAULT, IPL_NET);
		cv_init(&rxr->rxr_onwork_cv, "waitonwk");

		snprintf(rxr->rxr_name, sizeof(rxr->rxr_name),
		    "%s-rx%d", device_xname(sc->sc_dev), i);
		evcnt_attach_dynamic(&rxr->rxr_evpkts, EVCNT_TYPE_MISC,
		    NULL, rxr->rxr_name, "packets received");
		evcnt_attach_dynamic(&rxr->rxr_evcsum_ip, EVCNT_TYPE_MISC,
		    NULL, rxr->rxr_name, "IP checksum");
		evcnt_attach_dynamic(&rxr->rxr_evcsum_tcp, EVCNT_TYPE_MISC,
		    NULL, rxr->rxr_name, "TCP checksum");
		evcnt_attach_dynamic(&rxr->rxr_evcsum_udp, EVCNT_TYPE_MISC,
		    NULL, rxr->rxr_name, "UDP checksum");
		evcnt_attach_dynamic(&rxr->rxr_evvlanhwtagging, EVCNT_TYPE_MISC,
		    NULL, rxr->rxr_name, "VLAN H/W tagging");
		evcnt_attach_dynamic(&rxr->rxr_evintr, EVCNT_TYPE_INTR,
		    NULL, rxr->rxr_name, "interrupt on ring");
		evcnt_attach_dynamic(&rxr->rxr_evdefer, EVCNT_TYPE_MISC,
		    NULL, rxr->rxr_name, "handled queue in workqueue");
		evcnt_attach_dynamic(&rxr->rxr_evdeferreq, EVCNT_TYPE_MISC,
		    NULL, rxr->rxr_name, "requested defer on ring");
		evcnt_attach_dynamic(&rxr->rxr_evredeferreq, EVCNT_TYPE_MISC,
		    NULL, rxr->rxr_name, "requested defer in workqueue");

		rxr->rxr_nvsbuf = kmem_zalloc(HVN_NVS_BUFSIZE, KM_SLEEP);
		if (rxr->rxr_nvsbuf == NULL) {
			DPRINTF("%s: failed to allocate channel data buffer\n",
			    device_xname(sc->sc_dev));
			goto errout;
		}

		rxr->rxr_si = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
		    hvn_nvs_softintr, rxr);
		if (rxr->rxr_si == NULL) {
			DPRINTF("%s: failed to establish rx softint\n",
			    device_xname(sc->sc_dev));
			goto errout;
		}
	}

	return 0;

 errout:
	hvn_rx_ring_destroy(sc);
	return -1;
}

static int
hvn_rx_ring_destroy(struct hvn_softc *sc)
{
	struct hvn_rx_ring *rxr;
	int i;

	if (sc->sc_rxr != NULL) {
		for (i = 0; i < sc->sc_nrxr; i++) {
			rxr = &sc->sc_rxr[i];

			if (rxr->rxr_si != NULL) {
				softint_disestablish(rxr->rxr_si);
				rxr->rxr_si = NULL;
			}

			if (rxr->rxr_nvsbuf != NULL) {
				kmem_free(rxr->rxr_nvsbuf, HVN_NVS_BUFSIZE);
				rxr->rxr_nvsbuf = NULL;
			}

			evcnt_detach(&rxr->rxr_evpkts);
			evcnt_detach(&rxr->rxr_evcsum_ip);
			evcnt_detach(&rxr->rxr_evcsum_tcp);
			evcnt_detach(&rxr->rxr_evcsum_udp);
			evcnt_detach(&rxr->rxr_evvlanhwtagging);
			evcnt_detach(&rxr->rxr_evintr);
			evcnt_detach(&rxr->rxr_evdefer);
			evcnt_detach(&rxr->rxr_evdeferreq);
			evcnt_detach(&rxr->rxr_evredeferreq);

			cv_destroy(&rxr->rxr_onwork_cv);
			mutex_destroy(&rxr->rxr_onwork_lock);
			mutex_destroy(&rxr->rxr_lock);
		}
		kmem_free(sc->sc_rxr, sizeof(*rxr) * sc->sc_nrxr);
		sc->sc_rxr = NULL;
		sc->sc_nrxr = 0;
	}
	if (sc->sc_rx_ring != NULL) {
		hyperv_dma_free(sc->sc_dmat, &sc->sc_rx_dma);
		sc->sc_rx_ring = NULL;
	}

	return 0;
}

static void
hvn_fixup_rx_data(struct hvn_softc *sc)
{
	struct hvn_rx_ring *rxr;
	int i;

	if (sc->sc_caps & HVN_CAPS_UDPHASH) {
		for (i = 0; i < sc->sc_nrxr; i++) {
			rxr = &sc->sc_rxr[i];
			rxr->rxr_flags |= HVN_RXR_FLAG_UDP_HASH;
		}
	}
}

static int
hvn_tx_ring_create(struct hvn_softc *sc, int ring_cnt)
{
	struct hvn_tx_ring *txr;
	struct hvn_tx_desc *txd;
	bus_dma_segment_t *seg;
	size_t msgsize;
	int i, j;
	paddr_t pa;

	/*
	 * Create TXBUF for chimney sending.
	 *
	 * NOTE: It is shared by all channels.
	 */
	sc->sc_chim = hyperv_dma_alloc(sc->sc_dmat, &sc->sc_chim_dma,
	    HVN_CHIM_SIZE, PAGE_SIZE, 0, 1);
	if (sc->sc_chim == NULL) {
		DPRINTF("%s: failed to allocate chimney sending memory",
		    device_xname(sc->sc_dev));
		goto errout;
	}

	sc->sc_txr = kmem_zalloc(sizeof(*txr) * ring_cnt, KM_SLEEP);
	sc->sc_ntxr_inuse = sc->sc_ntxr = ring_cnt;

	msgsize = roundup(HVN_RNDIS_PKT_LEN, 128);

	for (j = 0; j < ring_cnt; j++) {
		txr = &sc->sc_txr[j];
		txr->txr_softc = sc;
		txr->txr_id = j;

		mutex_init(&txr->txr_lock, MUTEX_DEFAULT, IPL_NET);
		txr->txr_interq = pcq_create(HVN_TX_DESC, KM_SLEEP);

		snprintf(txr->txr_name, sizeof(txr->txr_name),
		    "%s-tx%d", device_xname(sc->sc_dev), j);
		evcnt_attach_dynamic(&txr->txr_evpkts, EVCNT_TYPE_MISC,
		    NULL, txr->txr_name, "packets transmit");
		evcnt_attach_dynamic(&txr->txr_evsends, EVCNT_TYPE_MISC,
		    NULL, txr->txr_name, "sends");
		evcnt_attach_dynamic(&txr->txr_evnodesc, EVCNT_TYPE_MISC,
		    NULL, txr->txr_name, "descriptor shortage");
		evcnt_attach_dynamic(&txr->txr_evdmafailed, EVCNT_TYPE_MISC,
		    NULL, txr->txr_name, "DMA failure");
		evcnt_attach_dynamic(&txr->txr_evdefrag, EVCNT_TYPE_MISC,
		    NULL, txr->txr_name, "mbuf defraged");
		evcnt_attach_dynamic(&txr->txr_evpcqdrop, EVCNT_TYPE_MISC,
		    NULL, txr->txr_name, "dropped in pcq");
		evcnt_attach_dynamic(&txr->txr_evtransmitdefer, EVCNT_TYPE_MISC,
		    NULL, txr->txr_name, "deferred transmit");
		evcnt_attach_dynamic(&txr->txr_evflushfailed, EVCNT_TYPE_MISC,
		    NULL, txr->txr_name, "aggregation flush failure");
		evcnt_attach_dynamic(&txr->txr_evchimneytried, EVCNT_TYPE_MISC,
		    NULL, txr->txr_name, "chimney send tried");
		evcnt_attach_dynamic(&txr->txr_evchimney, EVCNT_TYPE_MISC,
		    NULL, txr->txr_name, "chimney send");
		evcnt_attach_dynamic(&txr->txr_evvlanfixup, EVCNT_TYPE_MISC,
		    NULL, txr->txr_name, "VLAN fixup");
		evcnt_attach_dynamic(&txr->txr_evvlanhwtagging, EVCNT_TYPE_MISC,
		    NULL, txr->txr_name, "VLAN H/W tagging");
		evcnt_attach_dynamic(&txr->txr_evvlantap, EVCNT_TYPE_MISC,
		    NULL, txr->txr_name, "VLAN bpf_mtap fixup");

		txr->txr_si = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
		    hvn_deferred_transmit, txr);
		if (txr->txr_si == NULL) {
			aprint_error_dev(sc->sc_dev,
			    "failed to establish softint for tx ring\n");
			goto errout;
		}

		/* Allocate memory to store RNDIS messages */
		txr->txr_msgs = hyperv_dma_alloc(sc->sc_dmat, &txr->txr_dma,
		    msgsize * HVN_TX_DESC, PAGE_SIZE, 0, 1);
		if (txr->txr_msgs == NULL) {
			DPRINTF("%s: failed to allocate memory for RDNIS "
			    "messages\n", device_xname(sc->sc_dev));
			goto errout;
		}

		TAILQ_INIT(&txr->txr_list);
		for (i = 0; i < HVN_TX_DESC; i++) {
			txd = &txr->txr_desc[i];
			txd->txd_chim_index = HVN_NVS_CHIM_IDX_INVALID;
			txd->txd_chim_size = 0;
			STAILQ_INIT(&txd->txd_agg_list);
			if (bus_dmamap_create(sc->sc_dmat, HVN_TX_PKT_SIZE,
			    HVN_TX_FRAGS, HVN_TX_FRAG_SIZE, PAGE_SIZE,
			    BUS_DMA_WAITOK, &txd->txd_dmap)) {
				DPRINTF("%s: failed to create map for TX "
				    "descriptors\n", device_xname(sc->sc_dev));
				goto errout;
			}
			seg = &txr->txr_dma.map->dm_segs[0];
			pa = seg->ds_addr + (msgsize * i);
			txd->txd_gpa.gpa_page = atop(pa);
			txd->txd_gpa.gpa_ofs = pa & PAGE_MASK;
			txd->txd_gpa.gpa_len = msgsize;
			txd->txd_req = (void *)(txr->txr_msgs + (msgsize * i));
			txd->txd_id = i + HVN_NVS_CHIM_SIG;
			TAILQ_INSERT_TAIL(&txr->txr_list, txd, txd_entry);
		}
		txr->txr_avail = HVN_TX_DESC;
	}

	return 0;

 errout:
	hvn_tx_ring_destroy(sc);
	return -1;
}

static void
hvn_tx_ring_destroy(struct hvn_softc *sc)
{
	struct hvn_tx_ring *txr;
	struct hvn_tx_desc *txd;
	int i, j;

	if (sc->sc_txr != NULL) {
		for (j = 0; j < sc->sc_ntxr; j++) {
			txr = &sc->sc_txr[j];

			mutex_enter(&txr->txr_lock);
			for (i = 0; i < HVN_TX_DESC; i++) {
				txd = &txr->txr_desc[i];
				hvn_txd_gc(txr, txd);
			}
			mutex_exit(&txr->txr_lock);
			for (i = 0; i < HVN_TX_DESC; i++) {
				txd = &txr->txr_desc[i];
				if (txd->txd_dmap != NULL) {
					bus_dmamap_destroy(sc->sc_dmat,
					    txd->txd_dmap);
					txd->txd_dmap = NULL;
				}
			}
			if (txr->txr_msgs != NULL) {
				hyperv_dma_free(sc->sc_dmat, &txr->txr_dma);
				txr->txr_msgs = NULL;
			}
			if (txr->txr_si != NULL) {
				softint_disestablish(txr->txr_si);
				txr->txr_si = NULL;
			}
			if (txr->txr_interq != NULL) {
				hvn_tx_ring_qflush(sc, txr);
				pcq_destroy(txr->txr_interq);
				txr->txr_interq = NULL;
			}

			evcnt_detach(&txr->txr_evpkts);
			evcnt_detach(&txr->txr_evsends);
			evcnt_detach(&txr->txr_evnodesc);
			evcnt_detach(&txr->txr_evdmafailed);
			evcnt_detach(&txr->txr_evdefrag);
			evcnt_detach(&txr->txr_evpcqdrop);
			evcnt_detach(&txr->txr_evtransmitdefer);
			evcnt_detach(&txr->txr_evflushfailed);
			evcnt_detach(&txr->txr_evchimneytried);
			evcnt_detach(&txr->txr_evchimney);
			evcnt_detach(&txr->txr_evvlanfixup);
			evcnt_detach(&txr->txr_evvlanhwtagging);
			evcnt_detach(&txr->txr_evvlantap);

			mutex_destroy(&txr->txr_lock);
		}

		kmem_free(sc->sc_txr, sizeof(*txr) * sc->sc_ntxr);
		sc->sc_txr = NULL;
	}

	if (sc->sc_chim != NULL) {
		hyperv_dma_free(sc->sc_dmat, &sc->sc_chim_dma);
		sc->sc_chim = NULL;
	}
}

static void
hvn_set_chim_size(struct hvn_softc *sc, int chim_size)
{
	struct hvn_tx_ring *txr;
	int i;

	for (i = 0; i < sc->sc_ntxr_inuse; i++) {
		txr = &sc->sc_txr[i];
		txr->txr_chim_size = chim_size;
	}
}

#if LONG_BIT == 64
#define ffsl(v)	ffs64(v)
#elif LONG_BIT == 32
#define ffsl(v)	ffs32(v)
#else
#error unsupport LONG_BIT
#endif  /* LONG_BIT */

static uint32_t
hvn_chim_alloc(struct hvn_softc *sc)
{
	uint32_t chim_idx = HVN_NVS_CHIM_IDX_INVALID;
	int i, idx;

	mutex_spin_enter(&sc->sc_chim_bmap_lock);
	for (i = 0; i < sc->sc_chim_bmap_cnt; i++) {
		idx = ffsl(~sc->sc_chim_bmap[i]);
		if (idx == 0)
			continue;

		--idx;	/* ffsl is 1-based */
		SET(sc->sc_chim_bmap[i], __BIT(idx));

		chim_idx = i * LONG_BIT + idx;
		break;
	}
	mutex_spin_exit(&sc->sc_chim_bmap_lock);

	return chim_idx;
}

static void
hvn_chim_free(struct hvn_softc *sc, uint32_t chim_idx)
{
	u_long mask;
	uint32_t idx;

	idx = chim_idx / LONG_BIT;
	mask = __BIT(chim_idx % LONG_BIT);

	mutex_spin_enter(&sc->sc_chim_bmap_lock);
	CLR(sc->sc_chim_bmap[idx], mask);
	mutex_spin_exit(&sc->sc_chim_bmap_lock);
}

static void
hvn_fixup_tx_data(struct hvn_softc *sc)
{
	struct hvn_tx_ring *txr;
	uint64_t caps_assist;
	int csum_assist;
	int i;

	hvn_set_chim_size(sc, sc->sc_chim_szmax);
	if (hvn_tx_chimney_size > 0 && hvn_tx_chimney_size < sc->sc_chim_szmax)
		hvn_set_chim_size(sc, hvn_tx_chimney_size);

	caps_assist = 0;
	csum_assist = 0;
	if (sc->sc_caps & HVN_CAPS_IPCS) {
		caps_assist |= IFCAP_CSUM_IPv4_Tx;
		caps_assist |= IFCAP_CSUM_IPv4_Rx;
		csum_assist |= M_CSUM_IPv4;
	}
	if (sc->sc_caps & HVN_CAPS_TCP4CS) {
		caps_assist |= IFCAP_CSUM_TCPv4_Tx;
		caps_assist |= IFCAP_CSUM_TCPv4_Rx;
		csum_assist |= M_CSUM_TCPv4;
	}
	if (sc->sc_caps &  HVN_CAPS_TCP6CS) {
		caps_assist |= IFCAP_CSUM_TCPv6_Tx;
		csum_assist |= M_CSUM_TCPv6;
	}
	if (sc->sc_caps & HVN_CAPS_UDP4CS) {
		caps_assist |= IFCAP_CSUM_UDPv4_Tx;
		caps_assist |= IFCAP_CSUM_UDPv4_Rx;
		csum_assist |= M_CSUM_UDPv4;
	}
	if (sc->sc_caps & HVN_CAPS_UDP6CS) {
		caps_assist |= IFCAP_CSUM_UDPv6_Tx;
		csum_assist |= M_CSUM_UDPv6;
	}
	for (i = 0; i < sc->sc_ntxr; i++) {
		txr = &sc->sc_txr[i];
		txr->txr_caps_assist = caps_assist;
		txr->txr_csum_assist = csum_assist;
	}

	if (sc->sc_caps & HVN_CAPS_UDPHASH) {
		for (i = 0; i < sc->sc_ntxr; i++) {
			txr = &sc->sc_txr[i];
			txr->txr_flags |= HVN_TXR_FLAG_UDP_HASH;
		}
	}
}

static int
hvn_txd_peek(struct hvn_tx_ring *txr)
{

	KASSERT(mutex_owned(&txr->txr_lock));

	return txr->txr_avail;
}

static struct hvn_tx_desc *
hvn_txd_get(struct hvn_tx_ring *txr)
{
	struct hvn_tx_desc *txd;

	KASSERT(mutex_owned(&txr->txr_lock));

	txd = TAILQ_FIRST(&txr->txr_list);
	KASSERT(txd != NULL);
	TAILQ_REMOVE(&txr->txr_list, txd, txd_entry);
	txr->txr_avail--;

	txd->txd_refs = 1;

	return txd;
}

static void
hvn_txd_put(struct hvn_tx_ring *txr, struct hvn_tx_desc *txd)
{
	struct hvn_softc *sc = txr->txr_softc;
	struct hvn_tx_desc *tmp_txd;

	KASSERT(mutex_owned(&txr->txr_lock));
	KASSERTMSG(!ISSET(txd->txd_flags, HVN_TXD_FLAG_ONAGG),
	    "put an onagg txd %#x", txd->txd_flags);

	KASSERTMSG(txd->txd_refs > 0, "invalid txd refs %d", txd->txd_refs);
	if (atomic_dec_uint_nv(&txd->txd_refs) != 0)
		return;

	if (!STAILQ_EMPTY(&txd->txd_agg_list)) {
		while ((tmp_txd = STAILQ_FIRST(&txd->txd_agg_list)) != NULL) {
			KASSERTMSG(STAILQ_EMPTY(&tmp_txd->txd_agg_list),
			    "resursive aggregation on aggregated txdesc");
			KASSERTMSG(
			    ISSET(tmp_txd->txd_flags, HVN_TXD_FLAG_ONAGG),
			    "not aggregated txdesc");
			KASSERTMSG(
			    tmp_txd->txd_chim_index == HVN_NVS_CHIM_IDX_INVALID,
			    "aggregated txdesc consumes chimney sending "
			    "buffer: idx %u", tmp_txd->txd_chim_index);
			KASSERTMSG(tmp_txd->txd_chim_size == 0,
			    "aggregated txdesc has non-zero chimney sending "
			    "size: sz %u", tmp_txd->txd_chim_size);

			STAILQ_REMOVE_HEAD(&txd->txd_agg_list, txd_agg_entry);
			CLR(tmp_txd->txd_flags, HVN_TXD_FLAG_ONAGG);
			hvn_txd_put(txr, tmp_txd);
		}
	}

	if (txd->txd_chim_index != HVN_NVS_CHIM_IDX_INVALID) {
		KASSERTMSG(!ISSET(txd->txd_flags, HVN_TXD_FLAG_DMAMAP),
		    "chim txd uses dmamap");
		hvn_chim_free(sc, txd->txd_chim_index);
		txd->txd_chim_index = HVN_NVS_CHIM_IDX_INVALID;
		txd->txd_chim_size = 0;
	} else if (ISSET(txd->txd_flags, HVN_TXD_FLAG_DMAMAP)) {
		bus_dmamap_sync(sc->sc_dmat, txd->txd_dmap,
		    0, txd->txd_dmap->dm_mapsize,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, txd->txd_dmap);
		CLR(txd->txd_flags, HVN_TXD_FLAG_DMAMAP);
	}

	if (txd->txd_buf != NULL) {
		m_freem(txd->txd_buf);
		txd->txd_buf = NULL;
	}

	TAILQ_INSERT_TAIL(&txr->txr_list, txd, txd_entry);
	txr->txr_avail++;
	txr->txr_oactive = 0;
}

static void
hvn_txd_gc(struct hvn_tx_ring *txr, struct hvn_tx_desc *txd)
{

	KASSERTMSG(txd->txd_refs == 0 || txd->txd_refs == 1,
	    "invalid txd refs %d", txd->txd_refs);

	/* Aggregated txds will be freed by their aggregating txd. */
	if (txd->txd_refs > 0 && !ISSET(txd->txd_flags, HVN_TXD_FLAG_ONAGG))
		hvn_txd_put(txr, txd);
}

static void
hvn_txd_hold(struct hvn_tx_desc *txd)
{

	/* 0->1 transition will never work */
	KASSERTMSG(txd->txd_refs > 0, "invalid txd refs %d", txd->txd_refs);

	atomic_inc_uint(&txd->txd_refs);
}

static void
hvn_txd_agg(struct hvn_tx_desc *agg_txd, struct hvn_tx_desc *txd)
{

	KASSERTMSG(!ISSET(agg_txd->txd_flags, HVN_TXD_FLAG_ONAGG),
	    "recursive aggregation on aggregating txdesc");
	KASSERTMSG(!ISSET(txd->txd_flags, HVN_TXD_FLAG_ONAGG),
	    "already aggregated");
	KASSERTMSG(STAILQ_EMPTY(&txd->txd_agg_list),
	    "recursive aggregation on to-be-aggregated txdesc");

	SET(txd->txd_flags, HVN_TXD_FLAG_ONAGG);
	STAILQ_INSERT_TAIL(&agg_txd->txd_agg_list, txd, txd_agg_entry);
}

static int
hvn_tx_ring_pending(struct hvn_tx_ring *txr)
{
	int pending = 0;

	mutex_enter(&txr->txr_lock);
	if (hvn_txd_peek(txr) != HVN_TX_DESC)
		pending = 1;
	mutex_exit(&txr->txr_lock);

	return pending;
}

static void
hvn_tx_ring_qflush(struct hvn_softc *sc, struct hvn_tx_ring *txr)
{
	struct mbuf *m;

	while ((m = pcq_get(txr->txr_interq)) != NULL)
		m_freem(m);
}

static int
hvn_get_lladdr(struct hvn_softc *sc, uint8_t *enaddr)
{
	size_t addrlen = ETHER_ADDR_LEN;
	int rv;

	rv = hvn_rndis_query(sc, OID_802_3_PERMANENT_ADDRESS, enaddr, &addrlen);
	if (rv == 0 && addrlen != ETHER_ADDR_LEN)
		rv = -1;
	return rv;
}

static void
hvn_update_link_status(struct hvn_softc *sc)
{
	struct ifnet *ifp = SC2IFP(sc);
	uint32_t state, old_link_state;
	size_t len = sizeof(state);
	int rv;

	rv = hvn_rndis_query(sc, OID_GEN_MEDIA_CONNECT_STATUS, &state, &len);
	if (rv != 0 || len != sizeof(state))
		return;

	old_link_state = sc->sc_link_state;
	sc->sc_link_state = (state == NDIS_MEDIA_STATE_CONNECTED) ?
	    LINK_STATE_UP : LINK_STATE_DOWN;
	if (old_link_state != sc->sc_link_state) {
		if_link_state_change(ifp, sc->sc_link_state);
	}
}

static int
hvn_get_mtu(struct hvn_softc *sc, uint32_t *mtu)
{
	size_t mtusz = sizeof(*mtu);
	int rv;

	rv = hvn_rndis_query(sc, OID_GEN_MAXIMUM_FRAME_SIZE, mtu, &mtusz);
	if (rv == 0 && mtusz != sizeof(*mtu))
		rv = -1;
	return rv;
}

static int
hvn_channel_attach(struct hvn_softc *sc, struct vmbus_channel *chan)
{
	struct hvn_rx_ring *rxr;
	struct hvn_tx_ring *txr;
	int idx;

	idx = chan->ch_subidx;
	if (idx < 0 || idx >= sc->sc_nrxr_inuse) {
		DPRINTF("%s: invalid sub-channel %u\n",
		    device_xname(sc->sc_dev), idx);
		return -1;
	}

	rxr = &sc->sc_rxr[idx];
	rxr->rxr_chan = chan;

	if (idx < sc->sc_ntxr_inuse) {
		txr = &sc->sc_txr[idx];
		txr->txr_chan = chan;
	}

	/* Bind this channel to a proper CPU. */
	vmbus_channel_cpu_set(chan, HVN_RING_IDX2CPU(sc, idx));

	chan->ch_flags &= ~CHF_BATCHED;

	/* Associate our interrupt handler with the channel */
	if (vmbus_channel_open(chan,
	    HVN_RING_BUFSIZE - sizeof(struct vmbus_bufring), NULL, 0,
	    hvn_nvs_intr, rxr)) {
		DPRINTF("%s: failed to open channel\n",
		    device_xname(sc->sc_dev));
		return -1;
	}

	return 0;
}

static void
hvn_channel_detach(struct hvn_softc *sc, struct vmbus_channel *chan)
{

	vmbus_channel_close_direct(chan);
}

static void
hvn_channel_detach_all(struct hvn_softc *sc)
{
	struct vmbus_channel **subchans;
	int i, subchan_cnt = sc->sc_nrxr_inuse - 1;

	if (subchan_cnt > 0) {
		/* Detach the sub-channels. */
		subchans = vmbus_subchannel_get(sc->sc_prichan, subchan_cnt);
		for (i = 0; i < subchan_cnt; i++)
			hvn_channel_detach(sc, subchans[i]);
		vmbus_subchannel_rel(subchans, subchan_cnt);
	}

	/*
	 * Detach the primary channel, _after_ all sub-channels
	 * are detached.
	 */
	hvn_channel_detach(sc, sc->sc_prichan);

	/* Wait for sub-channels to be destroyed, if any. */
	vmbus_subchannel_drain(sc->sc_prichan);
}

static int
hvn_subchannel_attach(struct hvn_softc *sc)
{
	struct vmbus_channel **subchans;
	int subchan_cnt = sc->sc_nrxr_inuse - 1;
	int i, error = 0;

	KASSERTMSG(subchan_cnt > 0, "no sub-channels");

	/* Attach the sub-channels. */
	subchans = vmbus_subchannel_get(sc->sc_prichan, subchan_cnt);
	for (i = 0; i < subchan_cnt; ++i) {
		int error1;

		error1 = hvn_channel_attach(sc, subchans[i]);
		if (error1) {
			error = error1;
			/* Move on; all channels will be detached later. */
		}
	}
	vmbus_subchannel_rel(subchans, subchan_cnt);

	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "sub-channels attach failed: %d\n", error);
		return error;
	}

	aprint_debug_dev(sc->sc_dev, "%d sub-channels attached\n",
	    subchan_cnt);
	return 0;
}

static int
hvn_synth_alloc_subchannels(struct hvn_softc *sc, int *nsubch)
{
	struct vmbus_channel **subchans;
	int error, nchan, rxr_cnt;

	nchan = *nsubch + 1;
	if (nchan < 2) {
		/* Multiple RX/TX rings are not requested. */
		*nsubch = 0;
		return 0;
	}

	/*
	 * Query RSS capabilities, e.g. # of RX rings, and # of indirect
	 * table entries.
	 */
	if (hvn_get_rsscaps(sc, &rxr_cnt)) {
		/* No RSS. */
		*nsubch = 0;
		return 0;
	}

	aprint_debug_dev(sc->sc_dev, "RX rings offered %u, requested %d\n",
	    rxr_cnt, nchan);

	if (nchan > rxr_cnt)
		nchan = rxr_cnt;
	if (nchan == 1) {
		aprint_debug_dev(sc->sc_dev,
		    "only 1 channel is supported, no vRSS\n");
		*nsubch = 0;
		return 0;
	}

	*nsubch = nchan - 1;
	error = hvn_nvs_alloc_subchannels(sc, nsubch);
	if (error || *nsubch == 0) {
		/* Failed to allocate sub-channels. */
		*nsubch = 0;
		return 0;
	}

	/*
	 * Wait for all sub-channels to become ready before moving on.
	 */
	subchans = vmbus_subchannel_get(sc->sc_prichan, *nsubch);
	vmbus_subchannel_rel(subchans, *nsubch);
	return 0;
}

static int
hvn_synth_attachable(const struct hvn_softc *sc)
{
#if 0
	const struct hvn_rx_ring *rxr;
	int i;

	for (i = 0; i < sc->sc_nrxr; i++) {
		rxr = &sc->sc_rxr[i];
		if (rxr->rxr_flags)
			return 0;
	}
#endif
	return 1;
}

/*
 * Make sure that the RX filter is zero after the successful
 * RNDIS initialization.
 *
 * NOTE:
 * Under certain conditions on certain versions of Hyper-V,
 * the RNDIS rxfilter is _not_ zero on the hypervisor side
 * after the successful RNDIS initialization, which breaks
 * the assumption of any following code (well, it breaks the
 * RNDIS API contract actually).  Clear the RNDIS rxfilter
 * explicitly, drain packets sneaking through, and drain the
 * interrupt taskqueues scheduled due to the stealth packets.
 */
static void
hvn_init_fixat(struct hvn_softc *sc, int nchan)
{

	hvn_disable_rx(sc);
	hvn_drain_rxtx(sc, nchan);
}

static void
hvn_set_txagg(struct hvn_softc *sc)
{
	struct hvn_tx_ring *txr;
	uint32_t size, pkts;
	int i;

	/*
	 * Setup aggregation size.
	 */
	if (sc->sc_agg_size < 0)
		size = UINT32_MAX;
	else
		size = sc->sc_agg_size;

	if (size > sc->sc_rndis_agg_size)
		size = sc->sc_rndis_agg_size;

	/* NOTE: We only aggregate packets using chimney sending buffers. */
	if (size > (uint32_t)sc->sc_chim_szmax)
		size = sc->sc_chim_szmax;

	if (size <= 2 * HVN_PKTSIZE_MIN(sc->sc_rndis_agg_align)) {
		/* Disable */
		size = 0;
		pkts = 0;
		goto done;
	}

	/* NOTE: Type of the per TX ring setting is 'int'. */
	if (size > INT_MAX)
		size = INT_MAX;

	/*
	 * Setup aggregation packet count.
	 */
	if (sc->sc_agg_pkts < 0)
		pkts = UINT32_MAX;
	else
		pkts = sc->sc_agg_pkts;

	if (pkts > sc->sc_rndis_agg_pkts)
		pkts = sc->sc_rndis_agg_pkts;

	if (pkts <= 1) {
		/* Disable */
		size = 0;
		pkts = 0;
		goto done;
	}

	/* NOTE: Type of the per TX ring setting is 'short'. */
	if (pkts > SHRT_MAX)
		pkts = SHRT_MAX;

done:
	/* NOTE: Type of the per TX ring setting is 'short'. */
	if (sc->sc_rndis_agg_align > SHRT_MAX) {
		/* Disable */
		size = 0;
		pkts = 0;
	}

	aprint_verbose_dev(sc->sc_dev,
	    "TX aggregate size %u, pkts %u, align %u\n",
	    size, pkts, sc->sc_rndis_agg_align);

	for (i = 0; i < sc->sc_ntxr_inuse; ++i) {
		txr = &sc->sc_txr[i];

		mutex_enter(&txr->txr_lock);
		txr->txr_agg_szmax = size;
		txr->txr_agg_pktmax = pkts;
		txr->txr_agg_align = sc->sc_rndis_agg_align;
		mutex_exit(&txr->txr_lock);
	}
}

static int
hvn_synth_attach(struct hvn_softc *sc, int mtu)
{
	uint8_t rss_key[RSS_KEYSIZE];
	uint32_t old_caps;
	int nchan = 1, nsubch;
	int i, error;

	if (!hvn_synth_attachable(sc))
		return ENXIO;

	/* Save capabilities for later verification. */
	old_caps = sc->sc_caps;
	sc->sc_caps = 0;

	/* Clear RSS stuffs. */
	sc->sc_rss_ind_size = 0;
	sc->sc_rss_hash = 0;
	sc->sc_rss_hcap = 0;

	/*
	 * Attach the primary channel _before_ attaching NVS and RNDIS.
	 */
	error = hvn_channel_attach(sc, sc->sc_prichan);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "failed to attach primary channel\n");
		goto failed;
	}

	/*
	 * Attach NVS.
	 */
	error = hvn_nvs_attach(sc, mtu);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to init NVSP\n");
		goto detach_channel;
	}

	/*
	 * Attach RNDIS _after_ NVS is attached.
	 */
	error = hvn_rndis_attach(sc, mtu);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to init RNDIS\n");
		goto detach_nvs;
	}

	error = hvn_set_capabilities(sc, mtu);
	if (error) {
		aprint_error_dev(sc->sc_dev, "failed to setup offloading\n");
		goto detach_rndis;
	}

	if ((sc->sc_flags & HVN_SCF_ATTACHED) && old_caps != sc->sc_caps) {
		device_printf(sc->sc_dev, "caps mismatch "
		    "old 0x%08x, new 0x%08x\n", old_caps, sc->sc_caps);
		error = ENXIO;
		goto detach_rndis;
	}

	/*
	 * Allocate sub-channels for multi-TX/RX rings.
	 *
	 * NOTE:
	 * The # of RX rings that can be used is equivalent to the # of
	 * channels to be requested.
	 */
	nsubch = sc->sc_nrxr - 1;
	error = hvn_synth_alloc_subchannels(sc, &nsubch);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "failed to allocate sub channels\n");
		goto detach_synth;
	}

	/*
	 * Set the # of TX/RX rings that could be used according to
	 * the # of channels that NVS offered.
	 */
	nchan = nsubch + 1;
	hvn_set_ring_inuse(sc, nchan);

	if (nchan > 1) {
		/*
		 * Attach the sub-channels.
		 *
		 * NOTE: hvn_set_ring_inuse() _must_ have been called.
		 */
		error = hvn_subchannel_attach(sc);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "failed to attach sub channels\n");
			goto detach_synth;
		}

		/*
		 * Configure RSS key and indirect table _after_ all sub-channels
		 * are attached.
		 */
		if (!(sc->sc_flags & HVN_SCF_HAS_RSSKEY)) {
			/* Set the default RSS key. */
			CTASSERT(sizeof(sc->sc_rss.rss_key) == sizeof(rss_key));
			rss_getkey(rss_key);
			memcpy(&sc->sc_rss.rss_key, rss_key,
			    sizeof(sc->sc_rss.rss_key));
			sc->sc_flags |= HVN_SCF_HAS_RSSKEY;
		}

		if (!(sc->sc_flags & HVN_SCF_HAS_RSSIND)) {
			/* Setup RSS indirect table in round-robin fashion. */
			for (i = 0; i < NDIS_HASH_INDCNT; i++) {
				sc->sc_rss.rss_ind[i] = i % nchan;
			}
			sc->sc_flags |= HVN_SCF_HAS_RSSIND;
		} else {
			/*
			 * # of usable channels may be changed, so we have to
			 * make sure that all entries in RSS indirect table
			 * are valid.
			 *
			 * NOTE: hvn_set_ring_inuse() _must_ have been called.
			 */
			hvn_fixup_rss_ind(sc);
		}

		sc->sc_rss_hash = sc->sc_rss_hcap;
		error = hvn_set_rss(sc, NDIS_RSS_FLAG_NONE);
		if (error) {
			aprint_error_dev(sc->sc_dev, "failed to setup RSS\n");
			goto detach_synth;
		}
	}

	/*
	 * Fixup transmission aggregation setup.
	 */
	hvn_set_txagg(sc);
	hvn_init_fixat(sc, nchan);
	return 0;

detach_synth:
	hvn_init_fixat(sc, nchan);
	hvn_synth_detach(sc);
	return error;

detach_rndis:
	hvn_init_fixat(sc, nchan);
	hvn_rndis_detach(sc);
detach_nvs:
	hvn_nvs_detach(sc);
detach_channel:
	hvn_channel_detach(sc, sc->sc_prichan);
failed:
	/* Restore old capabilities. */
	sc->sc_caps = old_caps;
	return error;
}

static void
hvn_synth_detach(struct hvn_softc *sc)
{

	/* Detach the RNDIS first. */
	hvn_rndis_detach(sc);

	/* Detach NVS. */
	hvn_nvs_detach(sc);

	/* Detach all of the channels. */
	hvn_channel_detach_all(sc);

	if (sc->sc_prichan->ch_sc->sc_proto >= VMBUS_VERSION_WIN10 &&
	    sc->sc_rx_hndl) {
		/*
		 * Host is post-Win2016, disconnect RXBUF from primary channel
		 * here.
		 */
		vmbus_handle_free(sc->sc_prichan, sc->sc_rx_hndl);
		sc->sc_rx_hndl = 0;
	}

	if (sc->sc_prichan->ch_sc->sc_proto >= VMBUS_VERSION_WIN10 &&
	    sc->sc_chim_hndl) {
		/*
		 * Host is post-Win2016, disconnect chimney sending buffer
		 * from primary channel here.
		 */
		vmbus_handle_free(sc->sc_prichan, sc->sc_chim_hndl);
		sc->sc_chim_hndl = 0;
	}
}

static void
hvn_set_ring_inuse(struct hvn_softc *sc, int ring_cnt)
{

	if (sc->sc_ntxr > ring_cnt)
		sc->sc_ntxr_inuse = ring_cnt;
	else
		sc->sc_ntxr_inuse = sc->sc_ntxr;
	sc->sc_nrxr_inuse = ring_cnt;
}

static void
hvn_channel_drain(struct hvn_softc *sc, struct vmbus_channel *chan)
{
	struct hvn_rx_ring *rxr;
	int i, s;

	for (rxr = NULL, i = 0; i < sc->sc_nrxr_inuse; i++) {
		rxr = &sc->sc_rxr[i];
		if (rxr->rxr_chan == chan)
			break;
	}
	KASSERT(i < sc->sc_nrxr_inuse);

	/*
	 * NOTE:
	 * The TX bufring will not be drained by the hypervisor,
	 * if the primary channel is revoked.
	 */
	while (!vmbus_channel_rx_empty(chan) ||
	    (!vmbus_channel_is_revoked(sc->sc_prichan) &&
	     !vmbus_channel_tx_empty(chan))) {
		DELAY(20);
		s = splnet();
		hvn_nvs_intr1(rxr, sc->sc_tx_process_limit,
		    sc->sc_rx_process_limit);
		splx(s);
	}

	mutex_enter(&rxr->rxr_onwork_lock);
	while (rxr->rxr_onlist || rxr->rxr_onproc)
		cv_wait(&rxr->rxr_onwork_cv, &rxr->rxr_onwork_lock);
	mutex_exit(&rxr->rxr_onwork_lock);
}

static void
hvn_disable_rx(struct hvn_softc *sc)
{

	/*
	 * Disable RX by clearing RX filter forcefully.
	 */
	(void)hvn_rndis_close(sc);	/* ignore error */

	/*
	 * Give RNDIS enough time to flush all pending data packets.
	 */
	DELAY(200);
}

static void
hvn_drain_rxtx(struct hvn_softc *sc, int nchan)
{
	struct vmbus_channel **subchans = NULL;
	int i, nsubch;

	/*
	 * Drain RX/TX bufrings and interrupts.
	 */
	nsubch = nchan - 1;
	if (nsubch > 0)
		subchans = vmbus_subchannel_get(sc->sc_prichan, nsubch);

	if (subchans != NULL) {
		for (i = 0; i < nsubch; ++i)
			hvn_channel_drain(sc, subchans[i]);
	}
	hvn_channel_drain(sc, sc->sc_prichan);

	if (subchans != NULL)
		vmbus_subchannel_rel(subchans, nsubch);
}

static void
hvn_suspend_data(struct hvn_softc *sc)
{
	struct hvn_tx_ring *txr;
	int i, s;

	/*
	 * Suspend TX.
	 */
	for (i = 0; i < sc->sc_ntxr_inuse; i++) {
		txr = &sc->sc_txr[i];

		mutex_enter(&txr->txr_lock);
		txr->txr_suspended = 1;
		mutex_exit(&txr->txr_lock);
		/* No one is able send more packets now. */

		/*
		 * Wait for all pending sends to finish.
		 *
		 * NOTE:
		 * We will _not_ receive all pending send-done, if the
		 * primary channel is revoked.
		 */
		while (hvn_tx_ring_pending(txr) &&
		    !vmbus_channel_is_revoked(sc->sc_prichan)) {
			DELAY(20);
			s = splnet();
			hvn_nvs_intr1(txr->txr_rxr, sc->sc_tx_process_limit,
			    sc->sc_rx_process_limit);
			splx(s);
		}
	}

	/*
	 * Disable RX.
	 */
	hvn_disable_rx(sc);

	/*
	 * Drain RX/TX.
	 */
	hvn_drain_rxtx(sc, sc->sc_nrxr_inuse);
}

static void
hvn_suspend_mgmt(struct hvn_softc *sc)
{

	sc->sc_link_suspend = true;
	callout_halt(&sc->sc_link_tmout, NULL);

	/* Drain link state task */
	mutex_enter(&sc->sc_link_lock);
	for (;;) {
		if (!sc->sc_link_onproc)
			break;
		mutex_exit(&sc->sc_link_lock);
		DELAY(20);
		mutex_enter(&sc->sc_link_lock);
	}
	mutex_exit(&sc->sc_link_lock);
}

static void
hvn_suspend(struct hvn_softc *sc)
{
	struct ifnet *ifp = SC2IFP(sc);

	if (ifp->if_flags & IFF_RUNNING)
		hvn_suspend_data(sc);
	hvn_suspend_mgmt(sc);
}

static void
hvn_resume_tx(struct hvn_softc *sc, int ring_cnt)
{
	struct hvn_tx_ring *txr;
	int i;

	for (i = 0; i < ring_cnt; i++) {
		txr = &sc->sc_txr[i];
		mutex_enter(&txr->txr_lock);
		txr->txr_suspended = 0;
		mutex_exit(&txr->txr_lock);
	}
}

static void
hvn_resume_data(struct hvn_softc *sc)
{
	struct ifnet *ifp = SC2IFP(sc);
	struct hvn_tx_ring *txr;
	int i;

	/*
	 * Re-enable RX.
	 */
	hvn_rndis_open(sc);

	/*
	 * Make sure to clear suspend status on "all" TX rings,
	 * since sc_ntxr_inuse can be changed after hvn_suspend_data().
	 */
	hvn_resume_tx(sc, sc->sc_ntxr);

	/*
	 * Flush unused mbuf, since sc_ntxr_inuse may be reduced.
	 */
	for (i = sc->sc_ntxr_inuse; i < sc->sc_ntxr; i++)
		hvn_tx_ring_qflush(sc, &sc->sc_txr[i]);

	/*
	 * Kick start TX.
	 */
	for (i = 0; i < sc->sc_ntxr_inuse; i++) {
		txr = &sc->sc_txr[i];
		mutex_enter(&txr->txr_lock);
		txr->txr_oactive = 0;

		/* ALTQ */
		if (txr->txr_id == 0)
			if_schedule_deferred_start(ifp);
		softint_schedule(txr->txr_si);
		mutex_exit(&txr->txr_lock);
	}
}

static void
hvn_resume_mgmt(struct hvn_softc *sc)
{

	sc->sc_link_suspend = false;
	hvn_link_event(sc, HVN_LINK_EV_RESUME_NETWORK);
}

static void
hvn_resume(struct hvn_softc *sc)
{
	struct ifnet *ifp = SC2IFP(sc);

	if (ifp->if_flags & IFF_RUNNING)
		hvn_resume_data(sc);
	hvn_resume_mgmt(sc);
}

static int
hvn_nvs_init(struct hvn_softc *sc)
{

	mutex_init(&sc->sc_nvsrsp_lock, MUTEX_DEFAULT, IPL_NET);
	cv_init(&sc->sc_nvsrsp_cv, "nvsrspcv");

	return 0;
}

static void
hvn_nvs_destroy(struct hvn_softc *sc)
{

	mutex_destroy(&sc->sc_nvsrsp_lock);
	cv_destroy(&sc->sc_nvsrsp_cv);
}

static int
hvn_nvs_doinit(struct hvn_softc *sc, uint32_t proto)
{
	struct hvn_nvs_init cmd;
	struct hvn_nvs_init_resp *rsp;
	uint64_t tid;
	int error;

	memset(&cmd, 0, sizeof(cmd));
	cmd.nvs_type = HVN_NVS_TYPE_INIT;
	cmd.nvs_ver_min = cmd.nvs_ver_max = proto;

	tid = atomic_inc_uint_nv(&sc->sc_nvstid);
	mutex_enter(&sc->sc_nvsrsp_lock);
	error = hvn_nvs_cmd(sc, &cmd, sizeof(cmd), tid, 0);
	if (error == 0) {
		rsp = (struct hvn_nvs_init_resp *)&sc->sc_nvsrsp;
		if (rsp->nvs_status != HVN_NVS_STATUS_OK)
			error = EINVAL;
	}
	mutex_exit(&sc->sc_nvsrsp_lock);

	return error;
}

static int
hvn_nvs_conf_ndis(struct hvn_softc *sc, int mtu)
{
	struct hvn_nvs_ndis_conf cmd;
	uint64_t tid;
	int error;

	memset(&cmd, 0, sizeof(cmd));
	cmd.nvs_type = HVN_NVS_TYPE_NDIS_CONF;
	cmd.nvs_mtu = mtu + ETHER_HDR_LEN;
	cmd.nvs_caps = HVN_NVS_NDIS_CONF_VLAN;

	tid = atomic_inc_uint_nv(&sc->sc_nvstid);
	mutex_enter(&sc->sc_nvsrsp_lock);
	/* NOTE: No response. */
	error = hvn_nvs_cmd(sc, &cmd, sizeof(cmd), tid, 0);
	mutex_exit(&sc->sc_nvsrsp_lock);

	if (error == 0)
		sc->sc_caps |= HVN_CAPS_MTU | HVN_CAPS_VLAN;
	return error;
}

static int
hvn_nvs_init_ndis(struct hvn_softc *sc)
{
	struct hvn_nvs_ndis_init cmd;
	uint64_t tid;
	int error;

	memset(&cmd, 0, sizeof(cmd));
	cmd.nvs_type = HVN_NVS_TYPE_NDIS_INIT;
	cmd.nvs_ndis_major = (sc->sc_ndisver & 0xffff0000) >> 16;
	cmd.nvs_ndis_minor = sc->sc_ndisver & 0x0000ffff;

	tid = atomic_inc_uint_nv(&sc->sc_nvstid);
	mutex_enter(&sc->sc_nvsrsp_lock);
	/* NOTE: No response. */
	error = hvn_nvs_cmd(sc, &cmd, sizeof(cmd), tid, 0);
	mutex_exit(&sc->sc_nvsrsp_lock);

	return error;
}

static int
hvn_nvs_attach(struct hvn_softc *sc, int mtu)
{
	static const uint32_t protos[] = {
		HVN_NVS_PROTO_VERSION_5,
		HVN_NVS_PROTO_VERSION_4,
		HVN_NVS_PROTO_VERSION_2,
		HVN_NVS_PROTO_VERSION_1
	};
	int i;

	if (hyperv_ver_major >= 10)
		sc->sc_caps |= HVN_CAPS_UDPHASH;

	/*
	 * Initialize NVS.
	 */
	if (sc->sc_flags & HVN_SCF_ATTACHED) {
		/*
		 * NVS version and NDIS version MUST NOT be changed.
		 */
		DPRINTF("%s: reinit NVS version %#x, NDIS version %u.%u\n",
		    device_xname(sc->sc_dev), sc->sc_proto,
		    (sc->sc_ndisver >> 16), sc->sc_ndisver & 0xffff);

		if (hvn_nvs_doinit(sc, sc->sc_proto)) {
			DPRINTF("%s: failed to reinit NVSP version %#x\n",
			    device_xname(sc->sc_dev), sc->sc_proto);
			return -1;
		}
	} else {
		/*
		 * Find the supported NVS version and set NDIS version
		 * accordingly.
		 */
		for (i = 0; i < __arraycount(protos); i++) {
			if (hvn_nvs_doinit(sc, protos[i]) == 0)
				break;
		}
		if (i == __arraycount(protos)) {
			DPRINTF("%s: failed to negotiate NVSP version\n",
			    device_xname(sc->sc_dev));
			return -1;
		}

		sc->sc_proto = protos[i];
		if (sc->sc_proto <= HVN_NVS_PROTO_VERSION_4)
			sc->sc_ndisver = NDIS_VERSION_6_1;
		else
			sc->sc_ndisver = NDIS_VERSION_6_30;

		DPRINTF("%s: NVS version %#x, NDIS version %u.%u\n",
		    device_xname(sc->sc_dev), sc->sc_proto,
		    (sc->sc_ndisver >> 16), sc->sc_ndisver & 0xffff);
	}

	if (sc->sc_proto >= HVN_NVS_PROTO_VERSION_5)
		sc->sc_caps |= HVN_CAPS_HASHVAL;

	if (sc->sc_proto >= HVN_NVS_PROTO_VERSION_2) {
		/*
		 * Configure NDIS before initializing it.
		 */
		if (hvn_nvs_conf_ndis(sc, mtu))
			return -1;
	}

	/*
	 * Initialize NDIS.
	 */
	if (hvn_nvs_init_ndis(sc))
		return -1;

	/*
	 * Connect RXBUF.
	 */
	if (hvn_nvs_connect_rxbuf(sc))
		return -1;

	/*
	 * Connect chimney sending buffer.
	 */
	if (hvn_nvs_connect_chim(sc))
		return -1;

	return 0;
}

static int
hvn_nvs_connect_rxbuf(struct hvn_softc *sc)
{
	struct hvn_nvs_rxbuf_conn cmd;
	struct hvn_nvs_rxbuf_conn_resp *rsp;
	uint64_t tid;

	if (vmbus_handle_alloc(sc->sc_prichan, &sc->sc_rx_dma, sc->sc_rx_size,
	    &sc->sc_rx_hndl)) {
		DPRINTF("%s: failed to obtain a PA handle\n",
		    device_xname(sc->sc_dev));
		return -1;
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.nvs_type = HVN_NVS_TYPE_RXBUF_CONN;
	cmd.nvs_gpadl = sc->sc_rx_hndl;
	cmd.nvs_sig = HVN_NVS_RXBUF_SIG;

	tid = atomic_inc_uint_nv(&sc->sc_nvstid);
	mutex_enter(&sc->sc_nvsrsp_lock);
	if (hvn_nvs_cmd(sc, &cmd, sizeof(cmd), tid, 0))
		goto errout;

	rsp = (struct hvn_nvs_rxbuf_conn_resp *)&sc->sc_nvsrsp;
	if (rsp->nvs_status != HVN_NVS_STATUS_OK) {
		DPRINTF("%s: failed to set up the Rx ring\n",
		    device_xname(sc->sc_dev));
		goto errout;
	}

	SET(sc->sc_flags, HVN_SCF_RXBUF_CONNECTED);

	if (rsp->nvs_nsect > 1) {
		DPRINTF("%s: invalid number of Rx ring sections: %u\n",
		    device_xname(sc->sc_dev), rsp->nvs_nsect);
		goto errout;
	}
	mutex_exit(&sc->sc_nvsrsp_lock);

	return 0;

 errout:
	mutex_exit(&sc->sc_nvsrsp_lock);
	hvn_nvs_disconnect_rxbuf(sc);
	return -1;
}

static int
hvn_nvs_disconnect_rxbuf(struct hvn_softc *sc)
{
	struct hvn_nvs_rxbuf_disconn cmd;
	uint64_t tid;
	int s, error;

	if (ISSET(sc->sc_flags, HVN_SCF_RXBUF_CONNECTED)) {
		memset(&cmd, 0, sizeof(cmd));
		cmd.nvs_type = HVN_NVS_TYPE_RXBUF_DISCONN;
		cmd.nvs_sig = HVN_NVS_RXBUF_SIG;

		tid = atomic_inc_uint_nv(&sc->sc_nvstid);
		mutex_enter(&sc->sc_nvsrsp_lock);
		error = hvn_nvs_cmd(sc, &cmd, sizeof(cmd), tid,
		    HVN_NVS_CMD_NORESP);
		if (error) {
			device_printf(sc->sc_dev,
			    "failed to send rxbuf disconn: %d", error);
		}
		CLR(sc->sc_flags, HVN_SCF_RXBUF_CONNECTED);
		mutex_exit(&sc->sc_nvsrsp_lock);

		/*
		 * Wait for the hypervisor to receive this NVS request.
		 *
		 * NOTE:
		 * The TX bufring will not be drained by the hypervisor,
		 * if the primary channel is revoked.
		 */
		while (!vmbus_channel_tx_empty(sc->sc_prichan) &&
		    !vmbus_channel_is_revoked(sc->sc_prichan)) {
			DELAY(20);
			s = splnet();
			hvn_nvs_intr1(&sc->sc_rxr[0], sc->sc_tx_process_limit,
			    sc->sc_rx_process_limit);
			splx(s);
		}
		/*
		 * Linger long enough for NVS to disconnect RXBUF.
		 */
		DELAY(200);
	}

	if (sc->sc_prichan->ch_sc->sc_proto < VMBUS_VERSION_WIN10 &&
	    sc->sc_rx_hndl) {
		/*
		 * Disconnect RXBUF from primary channel.
		 */
		vmbus_handle_free(sc->sc_prichan, sc->sc_rx_hndl);
		sc->sc_rx_hndl = 0;
	}

	return 0;
}

static int
hvn_nvs_connect_chim(struct hvn_softc *sc)
{
	struct hvn_nvs_chim_conn cmd;
	const struct hvn_nvs_chim_conn_resp *rsp;
	uint64_t tid;

	mutex_init(&sc->sc_chim_bmap_lock, MUTEX_DEFAULT, IPL_NET);

	/*
	 * Connect chimney sending buffer GPADL to the primary channel.
	 *
	 * NOTE:
	 * Only primary channel has chimney sending buffer connected to it.
	 * Sub-channels just share this chimney sending buffer.
	 */
	if (vmbus_handle_alloc(sc->sc_prichan, &sc->sc_chim_dma, HVN_CHIM_SIZE,
	    &sc->sc_chim_hndl)) {
		DPRINTF("%s: failed to obtain a PA handle for chimney\n",
		    device_xname(sc->sc_dev));
		return -1;
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.nvs_type = HVN_NVS_TYPE_CHIM_CONN;
	cmd.nvs_gpadl = sc->sc_chim_hndl;
	cmd.nvs_sig = HVN_NVS_CHIM_SIG;

	tid = atomic_inc_uint_nv(&sc->sc_nvstid);
	mutex_enter(&sc->sc_nvsrsp_lock);
	if (hvn_nvs_cmd(sc, &cmd, sizeof(cmd), tid, 0))
		goto errout;

	rsp = (struct hvn_nvs_chim_conn_resp *)&sc->sc_nvsrsp;
	if (rsp->nvs_status != HVN_NVS_STATUS_OK) {
		DPRINTF("%s: failed to set up chimney sending buffer\n",
		    device_xname(sc->sc_dev));
		goto errout;
	}

	if (rsp->nvs_sectsz == 0 ||
	    (rsp->nvs_sectsz % sizeof(uint32_t)) != 0) {
		/*
		 * Can't use chimney sending buffer; done!
		 */
		if (rsp->nvs_sectsz == 0) {
			device_printf(sc->sc_dev,
			    "zero chimney sending buffer section size\n");
		} else {
			device_printf(sc->sc_dev,
			    "misaligned chimney sending buffers,"
			    " section size: %d", rsp->nvs_sectsz);
		}
		sc->sc_chim_szmax = 0;
		sc->sc_chim_cnt = 0;
	} else {
		sc->sc_chim_szmax = rsp->nvs_sectsz;
		sc->sc_chim_cnt = HVN_CHIM_SIZE / sc->sc_chim_szmax;
	}

	if (sc->sc_chim_szmax > 0) {
		if ((HVN_CHIM_SIZE % sc->sc_chim_szmax) != 0) {
			device_printf(sc->sc_dev,
			    "chimney sending sections are not properly "
			    "aligned\n");
		}
		if ((sc->sc_chim_cnt % LONG_BIT) != 0) {
			device_printf(sc->sc_dev,
			    "discard %d chimney sending sections\n",
			    sc->sc_chim_cnt % LONG_BIT);
		}

		sc->sc_chim_bmap_cnt = sc->sc_chim_cnt / LONG_BIT;
		sc->sc_chim_bmap = kmem_zalloc(sc->sc_chim_bmap_cnt *
		    sizeof(u_long), KM_SLEEP);
	}

	/* Done! */
	SET(sc->sc_flags, HVN_SCF_CHIM_CONNECTED);

	aprint_verbose_dev(sc->sc_dev, "chimney sending buffer %d/%d\n",
	    sc->sc_chim_szmax, sc->sc_chim_cnt);

	mutex_exit(&sc->sc_nvsrsp_lock);

	return 0;

errout:
	mutex_exit(&sc->sc_nvsrsp_lock);
	hvn_nvs_disconnect_chim(sc);
	return -1;
}

static int
hvn_nvs_disconnect_chim(struct hvn_softc *sc)
{
	struct hvn_nvs_chim_disconn cmd;
	uint64_t tid;
	int s, error;

	if (ISSET(sc->sc_flags, HVN_SCF_CHIM_CONNECTED)) {
		memset(&cmd, 0, sizeof(cmd));
		cmd.nvs_type = HVN_NVS_TYPE_CHIM_DISCONN;
		cmd.nvs_sig = HVN_NVS_CHIM_SIG;

		tid = atomic_inc_uint_nv(&sc->sc_nvstid);
		mutex_enter(&sc->sc_nvsrsp_lock);
		error = hvn_nvs_cmd(sc, &cmd, sizeof(cmd), tid,
		    HVN_NVS_CMD_NORESP);
		if (error) {
			device_printf(sc->sc_dev,
			    "failed to send chim disconn: %d", error);
		}
		CLR(sc->sc_flags, HVN_SCF_CHIM_CONNECTED);
		mutex_exit(&sc->sc_nvsrsp_lock);

		/*
		 * Wait for the hypervisor to receive this NVS request.
		 *
		 * NOTE:
		 * The TX bufring will not be drained by the hypervisor,
		 * if the primary channel is revoked.
		 */
		while (!vmbus_channel_tx_empty(sc->sc_prichan) &&
		    !vmbus_channel_is_revoked(sc->sc_prichan)) {
			DELAY(20);
			s = splnet();
			hvn_nvs_intr1(&sc->sc_rxr[0], sc->sc_tx_process_limit,
			    sc->sc_rx_process_limit);
			splx(s);
		}
		/*
		 * Linger long enough for NVS to disconnect chimney
		 * sending buffer.
		 */
		DELAY(200);
	}

	if (sc->sc_prichan->ch_sc->sc_proto < VMBUS_VERSION_WIN10 &&
	    sc->sc_chim_hndl) {
		/*
		 * Disconnect chimney sending buffer from primary channel.
		 */
		vmbus_handle_free(sc->sc_prichan, sc->sc_chim_hndl);
		sc->sc_chim_hndl = 0;
	}

	if (sc->sc_chim_bmap != NULL) {
		kmem_free(sc->sc_chim_bmap, sc->sc_chim_cnt / LONG_BIT);
		sc->sc_chim_bmap = NULL;
		sc->sc_chim_bmap_cnt = 0;
	}

	mutex_destroy(&sc->sc_chim_bmap_lock);

	return 0;
}

#define HVN_HANDLE_RING_DOTX	__BIT(0)

static int
hvn_handle_ring(struct hvn_rx_ring *rxr, int txlimit, int rxlimit)
{
	struct hvn_softc *sc = rxr->rxr_softc;
	struct vmbus_chanpkt_hdr *cph;
	const struct hvn_nvs_hdr *nvs;
	uint64_t rid;
	uint32_t rlen;
	int n, tx = 0, rx = 0;
	int result = 0;
	int rv;

	mutex_enter(&rxr->rxr_lock);
	for (;;) {
		rv = vmbus_channel_recv(rxr->rxr_chan, rxr->rxr_nvsbuf,
		    HVN_NVS_BUFSIZE, &rlen, &rid, 1);
		if (rv != 0 || rlen == 0) {
			if (rv != EAGAIN)
				device_printf(sc->sc_dev,
				    "failed to receive an NVSP packet\n");
			break;
		}
		cph = (struct vmbus_chanpkt_hdr *)rxr->rxr_nvsbuf;
		nvs = (const struct hvn_nvs_hdr *)VMBUS_CHANPKT_CONST_DATA(cph);

		if (cph->cph_type == VMBUS_CHANPKT_TYPE_COMP) {
			switch (nvs->nvs_type) {
			case HVN_NVS_TYPE_INIT_RESP:
			case HVN_NVS_TYPE_RXBUF_CONNRESP:
			case HVN_NVS_TYPE_CHIM_CONNRESP:
			case HVN_NVS_TYPE_SUBCH_RESP:
				mutex_enter(&sc->sc_nvsrsp_lock);
				/* copy the response back */
				memcpy(&sc->sc_nvsrsp, nvs, HVN_NVS_MSGSIZE);
				sc->sc_nvsdone = 1;
				cv_signal(&sc->sc_nvsrsp_cv);
				mutex_exit(&sc->sc_nvsrsp_lock);
				break;
			case HVN_NVS_TYPE_RNDIS_ACK:
				if (rxr->rxr_txr == NULL)
					break;

				result |= HVN_HANDLE_RING_DOTX;
				mutex_enter(&rxr->rxr_txr->txr_lock);
				hvn_txeof(rxr->rxr_txr, cph->cph_tid);
				mutex_exit(&rxr->rxr_txr->txr_lock);
				if (txlimit > 0 && ++tx >= txlimit)
					goto out;
				break;
			default:
				device_printf(sc->sc_dev,
				    "unhandled NVSP packet type %u "
				    "on completion\n", nvs->nvs_type);
				break;
			}
		} else if (cph->cph_type == VMBUS_CHANPKT_TYPE_RXBUF) {
			switch (nvs->nvs_type) {
			case HVN_NVS_TYPE_RNDIS:
				n = hvn_rndis_input(rxr, cph->cph_tid, cph);
				if (rxlimit > 0) {
					if (n < 0)
						goto out;
					rx += n;
					if (rx >= rxlimit)
						goto out;
				}
				break;
			default:
				device_printf(sc->sc_dev,
				    "unhandled NVSP packet type %u "
				    "on receive\n", nvs->nvs_type);
				break;
			}
		} else if (cph->cph_type == VMBUS_CHANPKT_TYPE_INBAND) {
			switch (nvs->nvs_type) {
			case HVN_NVS_TYPE_TXTBL_NOTE:
				/* Useless; ignore */
				break;
			default:
				device_printf(sc->sc_dev,
				    "got notify, nvs type %u\n", nvs->nvs_type);
				break;
			}
		} else
			device_printf(sc->sc_dev,
			    "unknown NVSP packet type %u\n", cph->cph_type);
	}
out:
	mutex_exit(&rxr->rxr_lock);

	return result;
}

static void
hvn_nvs_intr1(struct hvn_rx_ring *rxr, int txlimit, int rxlimit)
{
	struct hvn_softc *sc = rxr->rxr_softc;
	struct ifnet *ifp = SC2IFP(sc);
	struct hvn_tx_ring *txr = rxr->rxr_txr;
	int result;

	rxr->rxr_workqueue = sc->sc_txrx_workqueue;

	result = hvn_handle_ring(rxr, txlimit, rxlimit);

	if ((result & HVN_HANDLE_RING_DOTX) && txr != NULL) {
		mutex_enter(&txr->txr_lock);
		/* ALTQ */
		if (txr->txr_id == 0) {
			ifp->if_flags &= ~IFF_OACTIVE;
			if_schedule_deferred_start(ifp);
		}
		softint_schedule(txr->txr_si);
		mutex_exit(&txr->txr_lock);
	}
}

static void
hvn_schedule_handle_ring(struct hvn_softc *sc, struct hvn_rx_ring *rxr,
    bool intr)
{

	KASSERT(mutex_owned(&rxr->rxr_onwork_lock));

	if (rxr->rxr_workqueue) {
		if (!rxr->rxr_onlist) {
			rxr->rxr_onlist = true;
			if (intr)
				rxr->rxr_evdeferreq.ev_count++;
			else
				rxr->rxr_evredeferreq.ev_count++;
			workqueue_enqueue(sc->sc_wq, &rxr->rxr_wk, NULL);
		}
	} else {
		rxr->rxr_onlist = true;
		if (intr)
			rxr->rxr_evdeferreq.ev_count++;
		else
			rxr->rxr_evredeferreq.ev_count++;
		softint_schedule(rxr->rxr_si);
	}
}

static void
hvn_handle_ring_common(struct hvn_rx_ring *rxr)
{
	struct hvn_softc *sc = rxr->rxr_softc;
	int txlimit = sc->sc_tx_process_limit;
	int rxlimit = sc->sc_rx_process_limit;

	rxr->rxr_evdefer.ev_count++;

	mutex_enter(&rxr->rxr_onwork_lock);
	rxr->rxr_onproc = true;
	rxr->rxr_onlist = false;
	mutex_exit(&rxr->rxr_onwork_lock);

	hvn_nvs_intr1(rxr, txlimit, rxlimit);

	mutex_enter(&rxr->rxr_onwork_lock);
	if (vmbus_channel_unpause(rxr->rxr_chan)) {
		vmbus_channel_pause(rxr->rxr_chan);
		hvn_schedule_handle_ring(sc, rxr, false);
	}
	rxr->rxr_onproc = false;
	cv_broadcast(&rxr->rxr_onwork_cv);
	mutex_exit(&rxr->rxr_onwork_lock);
}

static void
hvn_handle_ring_work(struct work *wk, void *arg)
{
	struct hvn_rx_ring *rxr = container_of(wk, struct hvn_rx_ring, rxr_wk);

	hvn_handle_ring_common(rxr);
}

static void
hvn_nvs_softintr(void *arg)
{
	struct hvn_rx_ring *rxr = arg;

	hvn_handle_ring_common(rxr);
}

static void
hvn_nvs_intr(void *arg)
{
	struct hvn_rx_ring *rxr = arg;
	struct hvn_softc *sc = rxr->rxr_softc;
	int txlimit = cold ? 0 : sc->sc_tx_intr_process_limit;
	int rxlimit = cold ? 0 : sc->sc_rx_intr_process_limit;

	rxr->rxr_evintr.ev_count++;

	KASSERT(!rxr->rxr_onproc);
	KASSERT(!rxr->rxr_onlist);

	vmbus_channel_pause(rxr->rxr_chan);

	hvn_nvs_intr1(rxr, txlimit, rxlimit);

	if (vmbus_channel_unpause(rxr->rxr_chan) && !cold) {
		vmbus_channel_pause(rxr->rxr_chan);
		mutex_enter(&rxr->rxr_onwork_lock);
		hvn_schedule_handle_ring(sc, rxr, true);
		mutex_exit(&rxr->rxr_onwork_lock);
	}
}

static int
hvn_nvs_cmd(struct hvn_softc *sc, void *cmd, size_t cmdsize, uint64_t tid,
    u_int flags)
{
	struct hvn_rx_ring *rxr = &sc->sc_rxr[0];	/* primary channel */
	struct hvn_nvs_hdr *hdr = cmd;
	int tries = 10;
	int rv, s;

	KASSERT(mutex_owned(&sc->sc_nvsrsp_lock));

	sc->sc_nvsdone = 0;

	do {
		rv = vmbus_channel_send(rxr->rxr_chan, cmd, cmdsize,
		    tid, VMBUS_CHANPKT_TYPE_INBAND,
		    ISSET(flags, HVN_NVS_CMD_NORESP) ? 0 :
		      VMBUS_CHANPKT_FLAG_RC);
		if (rv == EAGAIN) {
			DELAY(1000);
		} else if (rv) {
			DPRINTF("%s: NVSP operation %u send error %d\n",
			    device_xname(sc->sc_dev), hdr->nvs_type, rv);
			return rv;
		}
	} while (rv != 0 && --tries > 0);

	if (tries == 0 && rv != 0) {
		device_printf(sc->sc_dev,
		    "NVSP operation %u send error %d\n", hdr->nvs_type, rv);
		return rv;
	}

	if (ISSET(flags, HVN_NVS_CMD_NORESP))
		return 0;

	while (!sc->sc_nvsdone && !ISSET(sc->sc_flags, HVN_SCF_REVOKED)) {
		mutex_exit(&sc->sc_nvsrsp_lock);
		DELAY(1000);
		s = splnet();
		hvn_nvs_intr1(rxr, 0, 0);
		splx(s);
		mutex_enter(&sc->sc_nvsrsp_lock);
	}

	return 0;
}

static int
hvn_nvs_ack(struct hvn_rx_ring *rxr, uint64_t tid)
{
	struct hvn_softc *sc __unused = rxr->rxr_softc;
	struct hvn_nvs_rndis_ack cmd;
	int tries = 5;
	int rv;

	cmd.nvs_type = HVN_NVS_TYPE_RNDIS_ACK;
	cmd.nvs_status = HVN_NVS_STATUS_OK;
	do {
		rv = vmbus_channel_send(rxr->rxr_chan, &cmd, sizeof(cmd),
		    tid, VMBUS_CHANPKT_TYPE_COMP, 0);
		if (rv == EAGAIN)
			DELAY(10);
		else if (rv) {
			DPRINTF("%s: NVSP acknowledgement error %d\n",
			    device_xname(sc->sc_dev), rv);
			return rv;
		}
	} while (rv != 0 && --tries > 0);
	return rv;
}

static void
hvn_nvs_detach(struct hvn_softc *sc)
{

	hvn_nvs_disconnect_rxbuf(sc);
	hvn_nvs_disconnect_chim(sc);
}

static int
hvn_nvs_alloc_subchannels(struct hvn_softc *sc, int *nsubchp)
{
	struct hvn_nvs_subch_req cmd;
	struct hvn_nvs_subch_resp *rsp;
	uint64_t tid;
	int nsubch, nsubch_req;

	nsubch_req = *nsubchp;
	KASSERTMSG(nsubch_req > 0, "invalid # of sub-channels %d", nsubch_req);

	memset(&cmd, 0, sizeof(cmd));
	cmd.nvs_type = HVN_NVS_TYPE_SUBCH_REQ;
	cmd.nvs_op = HVN_NVS_SUBCH_OP_ALLOC;
	cmd.nvs_nsubch = nsubch_req;

	tid = atomic_inc_uint_nv(&sc->sc_nvstid);
	mutex_enter(&sc->sc_nvsrsp_lock);
	if (hvn_nvs_cmd(sc, &cmd, sizeof(cmd), tid, 0)) {
		mutex_exit(&sc->sc_nvsrsp_lock);
		return EIO;
	}

	rsp = (struct hvn_nvs_subch_resp *)&sc->sc_nvsrsp;
	if (rsp->nvs_status != HVN_NVS_STATUS_OK) {
		mutex_exit(&sc->sc_nvsrsp_lock);
		DPRINTF("%s: failed to alloc sub-channels\n",
		    device_xname(sc->sc_dev));
		return EIO;
	}

	nsubch = rsp->nvs_nsubch;
	if (nsubch > nsubch_req) {
		aprint_debug_dev(sc->sc_dev,
		    "%u subchans are allocated, requested %d\n",
		    nsubch, nsubch_req);
		nsubch = nsubch_req;
	}
	mutex_exit(&sc->sc_nvsrsp_lock);

	*nsubchp = nsubch;

	return 0;
}

static inline struct rndis_cmd *
hvn_alloc_cmd(struct hvn_softc *sc)
{
	struct rndis_cmd *rc;

	mutex_enter(&sc->sc_cntl_fqlck);
	while ((rc = TAILQ_FIRST(&sc->sc_cntl_fq)) == NULL)
		cv_wait(&sc->sc_cntl_fqcv, &sc->sc_cntl_fqlck);
	TAILQ_REMOVE(&sc->sc_cntl_fq, rc, rc_entry);
	mutex_exit(&sc->sc_cntl_fqlck);
	return rc;
}

static inline void
hvn_submit_cmd(struct hvn_softc *sc, struct rndis_cmd *rc)
{

	mutex_enter(&sc->sc_cntl_sqlck);
	TAILQ_INSERT_TAIL(&sc->sc_cntl_sq, rc, rc_entry);
	mutex_exit(&sc->sc_cntl_sqlck);
}

static inline struct rndis_cmd *
hvn_complete_cmd(struct hvn_softc *sc, uint32_t id)
{
	struct rndis_cmd *rc;

	mutex_enter(&sc->sc_cntl_sqlck);
	TAILQ_FOREACH(rc, &sc->sc_cntl_sq, rc_entry) {
		if (rc->rc_id == id) {
			TAILQ_REMOVE(&sc->sc_cntl_sq, rc, rc_entry);
			break;
		}
	}
	mutex_exit(&sc->sc_cntl_sqlck);
	if (rc != NULL) {
		mutex_enter(&sc->sc_cntl_cqlck);
		TAILQ_INSERT_TAIL(&sc->sc_cntl_cq, rc, rc_entry);
		mutex_exit(&sc->sc_cntl_cqlck);
	}
	return rc;
}

static inline void
hvn_release_cmd(struct hvn_softc *sc, struct rndis_cmd *rc)
{

	mutex_enter(&sc->sc_cntl_cqlck);
	TAILQ_REMOVE(&sc->sc_cntl_cq, rc, rc_entry);
	mutex_exit(&sc->sc_cntl_cqlck);
}

static inline int
hvn_rollback_cmd(struct hvn_softc *sc, struct rndis_cmd *rc)
{
	struct rndis_cmd *rn;

	mutex_enter(&sc->sc_cntl_sqlck);
	TAILQ_FOREACH(rn, &sc->sc_cntl_sq, rc_entry) {
		if (rn == rc) {
			TAILQ_REMOVE(&sc->sc_cntl_sq, rc, rc_entry);
			mutex_exit(&sc->sc_cntl_sqlck);
			return 0;
		}
	}
	mutex_exit(&sc->sc_cntl_sqlck);
	return -1;
}

static inline void
hvn_free_cmd(struct hvn_softc *sc, struct rndis_cmd *rc)
{

	memset(rc->rc_req, 0, sizeof(struct rndis_packet_msg));
	memset(&rc->rc_cmp, 0, sizeof(rc->rc_cmp));
	memset(&rc->rc_msg, 0, sizeof(rc->rc_msg));
	mutex_enter(&sc->sc_cntl_fqlck);
	TAILQ_INSERT_TAIL(&sc->sc_cntl_fq, rc, rc_entry);
	cv_signal(&sc->sc_cntl_fqcv);
	mutex_exit(&sc->sc_cntl_fqlck);
}

static int
hvn_rndis_init(struct hvn_softc *sc)
{
	struct rndis_cmd *rc;
	int i;

	/* RNDIS control message queues */
	TAILQ_INIT(&sc->sc_cntl_sq);
	TAILQ_INIT(&sc->sc_cntl_cq);
	TAILQ_INIT(&sc->sc_cntl_fq);
	mutex_init(&sc->sc_cntl_sqlck, MUTEX_DEFAULT, IPL_NET);
	mutex_init(&sc->sc_cntl_cqlck, MUTEX_DEFAULT, IPL_NET);
	mutex_init(&sc->sc_cntl_fqlck, MUTEX_DEFAULT, IPL_NET);
	cv_init(&sc->sc_cntl_fqcv, "nvsalloc");

	for (i = 0; i < HVN_RNDIS_CTLREQS; i++) {
		rc = &sc->sc_cntl_msgs[i];
		if (bus_dmamap_create(sc->sc_dmat, PAGE_SIZE, 1, PAGE_SIZE, 0,
		    BUS_DMA_WAITOK, &rc->rc_dmap)) {
			DPRINTF("%s: failed to create RNDIS command map\n",
			    device_xname(sc->sc_dev));
			goto errout;
		}
		if (bus_dmamem_alloc(sc->sc_dmat, PAGE_SIZE, PAGE_SIZE,
		    0, &rc->rc_segs, 1, &rc->rc_nsegs, BUS_DMA_WAITOK)) {
			DPRINTF("%s: failed to allocate RNDIS command\n",
			    device_xname(sc->sc_dev));
			bus_dmamap_destroy(sc->sc_dmat, rc->rc_dmap);
			goto errout;
		}
		if (bus_dmamem_map(sc->sc_dmat, &rc->rc_segs, rc->rc_nsegs,
		    PAGE_SIZE, (void **)&rc->rc_req, BUS_DMA_WAITOK)) {
			DPRINTF("%s: failed to allocate RNDIS command\n",
			    device_xname(sc->sc_dev));
			bus_dmamem_free(sc->sc_dmat, &rc->rc_segs,
			    rc->rc_nsegs);
			bus_dmamap_destroy(sc->sc_dmat, rc->rc_dmap);
			goto errout;
		}
		memset(rc->rc_req, 0, PAGE_SIZE);
		if (bus_dmamap_load(sc->sc_dmat, rc->rc_dmap, rc->rc_req,
		    PAGE_SIZE, NULL, BUS_DMA_WAITOK)) {
			DPRINTF("%s: failed to load RNDIS command map\n",
			    device_xname(sc->sc_dev));
			bus_dmamem_unmap(sc->sc_dmat, rc->rc_req, PAGE_SIZE);
			rc->rc_req = NULL;
			bus_dmamem_free(sc->sc_dmat, &rc->rc_segs,
			    rc->rc_nsegs);
			bus_dmamap_destroy(sc->sc_dmat, rc->rc_dmap);
			goto errout;
		}
		rc->rc_gpa = atop(rc->rc_dmap->dm_segs[0].ds_addr);
		mutex_init(&rc->rc_lock, MUTEX_DEFAULT, IPL_NET);
		cv_init(&rc->rc_cv, "rndiscmd");
		TAILQ_INSERT_TAIL(&sc->sc_cntl_fq, rc, rc_entry);
	}

	/* Initialize RNDIS Data command */
	memset(&sc->sc_data_msg, 0, sizeof(sc->sc_data_msg));
	sc->sc_data_msg.nvs_type = HVN_NVS_TYPE_RNDIS;
	sc->sc_data_msg.nvs_rndis_mtype = HVN_NVS_RNDIS_MTYPE_DATA;
	sc->sc_data_msg.nvs_chim_idx = HVN_NVS_CHIM_IDX_INVALID;

	return 0;

errout:
	hvn_rndis_destroy(sc);
	return -1;
}

static void
hvn_rndis_destroy(struct hvn_softc *sc)
{
	struct rndis_cmd *rc;
	int i;

	for (i = 0; i < HVN_RNDIS_CTLREQS; i++) {
		rc = &sc->sc_cntl_msgs[i];
		if (rc->rc_req == NULL)
			continue;

		TAILQ_REMOVE(&sc->sc_cntl_fq, rc, rc_entry);
		bus_dmamap_unload(sc->sc_dmat, rc->rc_dmap);
		bus_dmamem_unmap(sc->sc_dmat, rc->rc_req, PAGE_SIZE);
		rc->rc_req = NULL;
		bus_dmamem_free(sc->sc_dmat, &rc->rc_segs, rc->rc_nsegs);
		bus_dmamap_destroy(sc->sc_dmat, rc->rc_dmap);
		mutex_destroy(&rc->rc_lock);
		cv_destroy(&rc->rc_cv);
	}

	mutex_destroy(&sc->sc_cntl_sqlck);
	mutex_destroy(&sc->sc_cntl_cqlck);
	mutex_destroy(&sc->sc_cntl_fqlck);
	cv_destroy(&sc->sc_cntl_fqcv);
}

static int
hvn_rndis_attach(struct hvn_softc *sc, int mtu)
{
	struct rndis_init_req *req;
	struct rndis_init_comp *cmp;
	struct rndis_cmd *rc;
	int rv;

	rc = hvn_alloc_cmd(sc);

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREREAD);

	rc->rc_id = atomic_inc_uint_nv(&sc->sc_rndisrid);

	req = rc->rc_req;
	req->rm_type = REMOTE_NDIS_INITIALIZE_MSG;
	req->rm_len = sizeof(*req);
	req->rm_rid = rc->rc_id;
	req->rm_ver_major = RNDIS_VERSION_MAJOR;
	req->rm_ver_minor = RNDIS_VERSION_MINOR;
	req->rm_max_xfersz = HVN_RNDIS_XFER_SIZE;

	rc->rc_cmplen = sizeof(*cmp);

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREWRITE);

	if ((rv = hvn_rndis_cmd(sc, rc, 0)) != 0) {
		DPRINTF("%s: INITIALIZE_MSG failed, error %d\n",
		    device_xname(sc->sc_dev), rv);
		hvn_free_cmd(sc, rc);
		return -1;
	}
	cmp = (struct rndis_init_comp *)&rc->rc_cmp;
	if (cmp->rm_status != RNDIS_STATUS_SUCCESS) {
		DPRINTF("%s: failed to init RNDIS, error %#x\n",
		    device_xname(sc->sc_dev), cmp->rm_status);
		hvn_free_cmd(sc, rc);
		return -1;
	}

	sc->sc_rndis_agg_size = cmp->rm_pktmaxsz;
	sc->sc_rndis_agg_pkts = cmp->rm_pktmaxcnt;
	sc->sc_rndis_agg_align = __BIT(cmp->rm_align);

	if (sc->sc_rndis_agg_align < sizeof(uint32_t)) {
		/*
		 * The RNDIS packet messsage encap assumes that the RNDIS
		 * packet message is at least 4 bytes aligned.  Fix up the
		 * alignment here, if the remote side sets the alignment
		 * too low.
		 */
		aprint_verbose_dev(sc->sc_dev,
		    "fixup RNDIS aggpkt align: %u -> %zu\n",
		    sc->sc_rndis_agg_align, sizeof(uint32_t));
		sc->sc_rndis_agg_align = sizeof(uint32_t);
	}

	aprint_verbose_dev(sc->sc_dev,
	    "RNDIS ver %u.%u, aggpkt size %u, aggpkt cnt %u, aggpkt align %u\n",
	    cmp->rm_ver_major, cmp->rm_ver_minor, sc->sc_rndis_agg_size,
	    sc->sc_rndis_agg_pkts, sc->sc_rndis_agg_align);

	hvn_free_cmd(sc, rc);

	return 0;
}

static int
hvn_get_rsscaps(struct hvn_softc *sc, int *nrxr)
{
	struct ndis_rss_caps in, caps;
	size_t caps_len;
	int error, rxr_cnt, indsz, hash_fnidx;
	uint32_t hash_func = 0, hash_types = 0;

	*nrxr = 0;

	if (sc->sc_ndisver < NDIS_VERSION_6_20)
		return EOPNOTSUPP;

	memset(&in, 0, sizeof(in));
	in.ndis_hdr.ndis_type = NDIS_OBJTYPE_RSS_CAPS;
	in.ndis_hdr.ndis_rev = NDIS_RSS_CAPS_REV_2;
	in.ndis_hdr.ndis_size = NDIS_RSS_CAPS_SIZE;

	caps_len = NDIS_RSS_CAPS_SIZE;
	error = hvn_rndis_query2(sc, OID_GEN_RECEIVE_SCALE_CAPABILITIES,
	    &in, NDIS_RSS_CAPS_SIZE, &caps, &caps_len, NDIS_RSS_CAPS_SIZE_6_0);
	if (error)
		return error;

	/*
	 * Preliminary verification.
	 */
	if (caps.ndis_hdr.ndis_type != NDIS_OBJTYPE_RSS_CAPS) {
		DPRINTF("%s: invalid NDIS objtype 0x%02x\n",
		    device_xname(sc->sc_dev), caps.ndis_hdr.ndis_type);
		return EINVAL;
	}
	if (caps.ndis_hdr.ndis_rev < NDIS_RSS_CAPS_REV_1) {
		DPRINTF("%s: invalid NDIS objrev 0x%02x\n",
		    device_xname(sc->sc_dev), caps.ndis_hdr.ndis_rev);
		return EINVAL;
	}
	if (caps.ndis_hdr.ndis_size > caps_len) {
		DPRINTF("%s: invalid NDIS objsize %u, data size %zu\n",
		    device_xname(sc->sc_dev), caps.ndis_hdr.ndis_size,
		    caps_len);
		return EINVAL;
	} else if (caps.ndis_hdr.ndis_size < NDIS_RSS_CAPS_SIZE_6_0) {
		DPRINTF("%s: invalid NDIS objsize %u\n",
		    device_xname(sc->sc_dev), caps.ndis_hdr.ndis_size);
		return EINVAL;
	}

	/*
	 * Save information for later RSS configuration.
	 */
	if (caps.ndis_nrxr == 0) {
		DPRINTF("%s: 0 RX rings!?\n", device_xname(sc->sc_dev));
		return EINVAL;
	}
	rxr_cnt = caps.ndis_nrxr;
	aprint_debug_dev(sc->sc_dev, "%u Rx rings\n", rxr_cnt);

	if (caps.ndis_hdr.ndis_size == NDIS_RSS_CAPS_SIZE &&
	    caps.ndis_hdr.ndis_rev >= NDIS_RSS_CAPS_REV_2) {
		if (caps.ndis_nind > NDIS_HASH_INDCNT) {
			DPRINTF("%s: too many RSS indirect table entries %u\n",
			    device_xname(sc->sc_dev), caps.ndis_nind);
			return EOPNOTSUPP;
		}
		if (!powerof2(caps.ndis_nind)) {
			DPRINTF("%s: RSS indirect table size is not power-of-2:"
			    " %u\n", device_xname(sc->sc_dev), caps.ndis_nind);
			return EOPNOTSUPP;
		}

		indsz = caps.ndis_nind;
	} else {
		indsz = NDIS_HASH_INDCNT;
	}
	if (rxr_cnt > indsz) {
		aprint_debug_dev(sc->sc_dev,
		    "# of RX rings (%u) > RSS indirect table size %u\n",
		    rxr_cnt, indsz);
		rxr_cnt = indsz;
	}

	/*
	 * NOTE:
	 * Toeplitz is at the lowest bit, and it is prefered; so ffs(),
	 * instead of fls(), is used here.
	 */
	hash_fnidx = ffs(caps.ndis_caps & NDIS_RSS_CAP_HASHFUNC_MASK);
	if (hash_fnidx == 0) {
		DPRINTF("%s: no hash functions, caps 0x%08x\n",
		    device_xname(sc->sc_dev), caps.ndis_caps);
		return EOPNOTSUPP;
	}
	hash_func = 1 << (hash_fnidx - 1);	/* ffs is 1-based */

	if (caps.ndis_caps & NDIS_RSS_CAP_IPV4)
		hash_types |= NDIS_HASH_IPV4 | NDIS_HASH_TCP_IPV4;
	if (caps.ndis_caps & NDIS_RSS_CAP_IPV6)
		hash_types |= NDIS_HASH_IPV6 | NDIS_HASH_TCP_IPV6;
	if (caps.ndis_caps & NDIS_RSS_CAP_IPV6_EX)
		hash_types |= NDIS_HASH_IPV6_EX | NDIS_HASH_TCP_IPV6_EX;
	if (hash_types == 0) {
		DPRINTF("%s: no hash types, caps 0x%08x\n",
		    device_xname(sc->sc_dev), caps.ndis_caps);
		return EOPNOTSUPP;
	}
	aprint_debug_dev(sc->sc_dev, "RSS caps %#x\n", caps.ndis_caps);

	sc->sc_rss_ind_size = indsz;
	sc->sc_rss_hcap = hash_func | hash_types;
	if (sc->sc_caps & HVN_CAPS_UDPHASH) {
		/* UDP 4-tuple hash is unconditionally enabled. */
		sc->sc_rss_hcap |= NDIS_HASH_UDP_IPV4_X;
	}
	*nrxr = rxr_cnt;

	return 0;
}

static int
hvn_set_rss(struct hvn_softc *sc, uint16_t flags)
{
	struct ndis_rssprm_toeplitz *rss = &sc->sc_rss;
	struct ndis_rss_params *params = &rss->rss_params;
	int len;

	/*
	 * Only NDIS 6.20+ is supported:
	 * We only support 4bytes element in indirect table, which has been
	 * adopted since NDIS 6.20.
	 */
	if (sc->sc_ndisver < NDIS_VERSION_6_20)
		return 0;

	/* XXX only one can be specified through, popcnt? */
	KASSERTMSG((sc->sc_rss_hash & NDIS_HASH_FUNCTION_MASK),
	    "no hash func %08x", sc->sc_rss_hash);
	KASSERTMSG((sc->sc_rss_hash & NDIS_HASH_STD),
	    "no standard hash types %08x", sc->sc_rss_hash);
	KASSERTMSG(sc->sc_rss_ind_size > 0, "no indirect table size");

	aprint_debug_dev(sc->sc_dev, "RSS indirect table size %d, hash %#x\n",
	    sc->sc_rss_ind_size, sc->sc_rss_hash);

	len = NDIS_RSSPRM_TOEPLITZ_SIZE(sc->sc_rss_ind_size);

	memset(params, 0, sizeof(*params));
	params->ndis_hdr.ndis_type = NDIS_OBJTYPE_RSS_PARAMS;
	params->ndis_hdr.ndis_rev = NDIS_RSS_PARAMS_REV_2;
	params->ndis_hdr.ndis_size = len;
	params->ndis_flags = flags;
	params->ndis_hash =
	    sc->sc_rss_hash & (NDIS_HASH_FUNCTION_MASK | NDIS_HASH_STD);
	params->ndis_indsize = sizeof(rss->rss_ind[0]) * sc->sc_rss_ind_size;
	params->ndis_indoffset =
	    offsetof(struct ndis_rssprm_toeplitz, rss_ind[0]);
	params->ndis_keysize = sizeof(rss->rss_key);
	params->ndis_keyoffset =
	    offsetof(struct ndis_rssprm_toeplitz, rss_key[0]);

	return hvn_rndis_set(sc, OID_GEN_RECEIVE_SCALE_PARAMETERS, rss, len);
}

static void
hvn_fixup_rss_ind(struct hvn_softc *sc)
{
	struct ndis_rssprm_toeplitz *rss = &sc->sc_rss;
	int i, nchan;

	nchan = sc->sc_nrxr_inuse;
	KASSERTMSG(nchan > 1, "invalid # of channels %d", nchan);

	/*
	 * Check indirect table to make sure that all channels in it
	 * can be used.
	 */
	for (i = 0; i < NDIS_HASH_INDCNT; i++) {
		if (rss->rss_ind[i] >= nchan) {
			DPRINTF("%s: RSS indirect table %d fixup: %u -> %d\n",
			    device_xname(sc->sc_dev), i, rss->rss_ind[i],
			    nchan - 1);
			rss->rss_ind[i] = nchan - 1;
		}
	}
}

static int
hvn_get_hwcaps(struct hvn_softc *sc, struct ndis_offload *caps)
{
	struct ndis_offload in;
	size_t caps_len, len;
	int error;

	memset(&in, 0, sizeof(in));
	in.ndis_hdr.ndis_type = NDIS_OBJTYPE_OFFLOAD;
	if (sc->sc_ndisver >= NDIS_VERSION_6_30) {
		in.ndis_hdr.ndis_rev = NDIS_OFFLOAD_REV_3;
		len = in.ndis_hdr.ndis_size = NDIS_OFFLOAD_SIZE;
	} else if (sc->sc_ndisver >= NDIS_VERSION_6_1) {
		in.ndis_hdr.ndis_rev = NDIS_OFFLOAD_REV_2;
		len = in.ndis_hdr.ndis_size = NDIS_OFFLOAD_SIZE_6_1;
	} else {
		in.ndis_hdr.ndis_rev = NDIS_OFFLOAD_REV_1;
		len = in.ndis_hdr.ndis_size = NDIS_OFFLOAD_SIZE_6_0;
	}

	caps_len = NDIS_OFFLOAD_SIZE;
	error = hvn_rndis_query2(sc, OID_TCP_OFFLOAD_HARDWARE_CAPABILITIES,
	    &in, len, caps, &caps_len, NDIS_OFFLOAD_SIZE_6_0);
	if (error)
		return error;

	/*
	 * Preliminary verification.
	 */
	if (caps->ndis_hdr.ndis_type != NDIS_OBJTYPE_OFFLOAD) {
		DPRINTF("%s: invalid NDIS objtype 0x%02x\n",
		    device_xname(sc->sc_dev), caps->ndis_hdr.ndis_type);
		return EINVAL;
	}
	if (caps->ndis_hdr.ndis_rev < NDIS_OFFLOAD_REV_1) {
		DPRINTF("%s: invalid NDIS objrev 0x%02x\n",
		    device_xname(sc->sc_dev), caps->ndis_hdr.ndis_rev);
		return EINVAL;
	}
	if (caps->ndis_hdr.ndis_size > caps_len) {
		DPRINTF("%s: invalid NDIS objsize %u, data size %zu\n",
		    device_xname(sc->sc_dev), caps->ndis_hdr.ndis_size,
		    caps_len);
		return EINVAL;
	} else if (caps->ndis_hdr.ndis_size < NDIS_OFFLOAD_SIZE_6_0) {
		DPRINTF("%s: invalid NDIS objsize %u\n",
		    device_xname(sc->sc_dev), caps->ndis_hdr.ndis_size);
		return EINVAL;
	}

	/*
	 * NOTE:
	 * caps->ndis_hdr.ndis_size MUST be checked before accessing
	 * NDIS 6.1+ specific fields.
	 */
	aprint_debug_dev(sc->sc_dev, "hwcaps rev %u\n",
	    caps->ndis_hdr.ndis_rev);

	aprint_debug_dev(sc->sc_dev, "hwcaps csum: "
	    "ip4 tx 0x%x/0x%x rx 0x%x/0x%x, "
	    "ip6 tx 0x%x/0x%x rx 0x%x/0x%x\n",
	    caps->ndis_csum.ndis_ip4_txcsum, caps->ndis_csum.ndis_ip4_txenc,
	    caps->ndis_csum.ndis_ip4_rxcsum, caps->ndis_csum.ndis_ip4_rxenc,
	    caps->ndis_csum.ndis_ip6_txcsum, caps->ndis_csum.ndis_ip6_txenc,
	    caps->ndis_csum.ndis_ip6_rxcsum, caps->ndis_csum.ndis_ip6_rxenc);
	aprint_debug_dev(sc->sc_dev, "hwcaps lsov2: "
	    "ip4 maxsz %u minsg %u encap 0x%x, "
	    "ip6 maxsz %u minsg %u encap 0x%x opts 0x%x\n",
	    caps->ndis_lsov2.ndis_ip4_maxsz, caps->ndis_lsov2.ndis_ip4_minsg,
	    caps->ndis_lsov2.ndis_ip4_encap, caps->ndis_lsov2.ndis_ip6_maxsz,
	    caps->ndis_lsov2.ndis_ip6_minsg, caps->ndis_lsov2.ndis_ip6_encap,
	    caps->ndis_lsov2.ndis_ip6_opts);

	return 0;
}

static int
hvn_set_capabilities(struct hvn_softc *sc, int mtu)
{
	struct ndis_offload hwcaps;
	struct ndis_offload_params params;
	size_t len;
	uint32_t caps = 0;
	int error, tso_maxsz, tso_minsg;

	error = hvn_get_hwcaps(sc, &hwcaps);
	if (error) {
		DPRINTF("%s: failed to query hwcaps\n",
		    device_xname(sc->sc_dev));
		return error;
	}

	/* NOTE: 0 means "no change" */
	memset(&params, 0, sizeof(params));

	params.ndis_hdr.ndis_type = NDIS_OBJTYPE_DEFAULT;
	if (sc->sc_ndisver < NDIS_VERSION_6_30) {
		params.ndis_hdr.ndis_rev = NDIS_OFFLOAD_PARAMS_REV_2;
		len = params.ndis_hdr.ndis_size = NDIS_OFFLOAD_PARAMS_SIZE_6_1;
	} else {
		params.ndis_hdr.ndis_rev = NDIS_OFFLOAD_PARAMS_REV_3;
		len = params.ndis_hdr.ndis_size = NDIS_OFFLOAD_PARAMS_SIZE;
	}

	/*
	 * TSO4/TSO6 setup.
	 */
	tso_maxsz = IP_MAXPACKET;
	tso_minsg = 2;
	if (hwcaps.ndis_lsov2.ndis_ip4_encap & NDIS_OFFLOAD_ENCAP_8023) {
		caps |= HVN_CAPS_TSO4;
		params.ndis_lsov2_ip4 = NDIS_OFFLOAD_LSOV2_ON;

		if (hwcaps.ndis_lsov2.ndis_ip4_maxsz < tso_maxsz)
			tso_maxsz = hwcaps.ndis_lsov2.ndis_ip4_maxsz;
		if (hwcaps.ndis_lsov2.ndis_ip4_minsg > tso_minsg)
			tso_minsg = hwcaps.ndis_lsov2.ndis_ip4_minsg;
	}
	if ((hwcaps.ndis_lsov2.ndis_ip6_encap & NDIS_OFFLOAD_ENCAP_8023) &&
	    (hwcaps.ndis_lsov2.ndis_ip6_opts & HVN_NDIS_LSOV2_CAP_IP6) ==
	    HVN_NDIS_LSOV2_CAP_IP6) {
		caps |= HVN_CAPS_TSO6;
		params.ndis_lsov2_ip6 = NDIS_OFFLOAD_LSOV2_ON;

		if (hwcaps.ndis_lsov2.ndis_ip6_maxsz < tso_maxsz)
			tso_maxsz = hwcaps.ndis_lsov2.ndis_ip6_maxsz;
		if (hwcaps.ndis_lsov2.ndis_ip6_minsg > tso_minsg)
			tso_minsg = hwcaps.ndis_lsov2.ndis_ip6_minsg;
	}
	sc->sc_tso_szmax = 0;
	sc->sc_tso_sgmin = 0;
	if (caps & (HVN_CAPS_TSO4 | HVN_CAPS_TSO6)) {
		KASSERTMSG(tso_maxsz <= IP_MAXPACKET,
		    "invalid NDIS TSO maxsz %d", tso_maxsz);
		KASSERTMSG(tso_minsg >= 2,
		    "invalid NDIS TSO minsg %d", tso_minsg);
		if (tso_maxsz < tso_minsg * mtu) {
			DPRINTF("%s: invalid NDIS TSO config: "
			    "maxsz %d, minsg %d, mtu %d; "
			    "disable TSO4 and TSO6\n", device_xname(sc->sc_dev),
			    tso_maxsz, tso_minsg, mtu);
			caps &= ~(HVN_CAPS_TSO4 | HVN_CAPS_TSO6);
			params.ndis_lsov2_ip4 = NDIS_OFFLOAD_LSOV2_OFF;
			params.ndis_lsov2_ip6 = NDIS_OFFLOAD_LSOV2_OFF;
		} else {
			sc->sc_tso_szmax = tso_maxsz;
			sc->sc_tso_sgmin = tso_minsg;
			aprint_debug_dev(sc->sc_dev,
			    "NDIS TSO szmax %d sgmin %d\n",
			    sc->sc_tso_szmax, sc->sc_tso_sgmin);
		}
	}

	/* IPv4 checksum */
	if ((hwcaps.ndis_csum.ndis_ip4_txcsum & HVN_NDIS_TXCSUM_CAP_IP4) ==
	    HVN_NDIS_TXCSUM_CAP_IP4) {
		caps |= HVN_CAPS_IPCS;
		params.ndis_ip4csum = NDIS_OFFLOAD_PARAM_TX;
	}
	if (hwcaps.ndis_csum.ndis_ip4_rxcsum & NDIS_RXCSUM_CAP_IP4) {
		if (params.ndis_ip4csum == NDIS_OFFLOAD_PARAM_TX)
			params.ndis_ip4csum = NDIS_OFFLOAD_PARAM_TXRX;
		else
			params.ndis_ip4csum = NDIS_OFFLOAD_PARAM_RX;
	}

	/* TCP4 checksum */
	if ((hwcaps.ndis_csum.ndis_ip4_txcsum & HVN_NDIS_TXCSUM_CAP_TCP4) ==
	    HVN_NDIS_TXCSUM_CAP_TCP4) {
		caps |= HVN_CAPS_TCP4CS;
		params.ndis_tcp4csum = NDIS_OFFLOAD_PARAM_TX;
	}
	if (hwcaps.ndis_csum.ndis_ip4_rxcsum & NDIS_RXCSUM_CAP_TCP4) {
		if (params.ndis_tcp4csum == NDIS_OFFLOAD_PARAM_TX)
			params.ndis_tcp4csum = NDIS_OFFLOAD_PARAM_TXRX;
		else
			params.ndis_tcp4csum = NDIS_OFFLOAD_PARAM_RX;
	}

	/* UDP4 checksum */
	if (hwcaps.ndis_csum.ndis_ip4_txcsum & NDIS_TXCSUM_CAP_UDP4) {
		caps |= HVN_CAPS_UDP4CS;
		params.ndis_udp4csum = NDIS_OFFLOAD_PARAM_TX;
	}
	if (hwcaps.ndis_csum.ndis_ip4_rxcsum & NDIS_RXCSUM_CAP_UDP4) {
		if (params.ndis_udp4csum == NDIS_OFFLOAD_PARAM_TX)
			params.ndis_udp4csum = NDIS_OFFLOAD_PARAM_TXRX;
		else
			params.ndis_udp4csum = NDIS_OFFLOAD_PARAM_RX;
	}

	/* TCP6 checksum */
	if ((hwcaps.ndis_csum.ndis_ip6_txcsum & HVN_NDIS_TXCSUM_CAP_TCP6) ==
	    HVN_NDIS_TXCSUM_CAP_TCP6) {
		caps |= HVN_CAPS_TCP6CS;
		params.ndis_tcp6csum = NDIS_OFFLOAD_PARAM_TX;
	}
	if (hwcaps.ndis_csum.ndis_ip6_rxcsum & NDIS_RXCSUM_CAP_TCP6) {
		if (params.ndis_tcp6csum == NDIS_OFFLOAD_PARAM_TX)
			params.ndis_tcp6csum = NDIS_OFFLOAD_PARAM_TXRX;
		else
			params.ndis_tcp6csum = NDIS_OFFLOAD_PARAM_RX;
	}

	/* UDP6 checksum */
	if ((hwcaps.ndis_csum.ndis_ip6_txcsum & HVN_NDIS_TXCSUM_CAP_UDP6) ==
	    HVN_NDIS_TXCSUM_CAP_UDP6) {
		caps |= HVN_CAPS_UDP6CS;
		params.ndis_udp6csum = NDIS_OFFLOAD_PARAM_TX;
	}
	if (hwcaps.ndis_csum.ndis_ip6_rxcsum & NDIS_RXCSUM_CAP_UDP6) {
		if (params.ndis_udp6csum == NDIS_OFFLOAD_PARAM_TX)
			params.ndis_udp6csum = NDIS_OFFLOAD_PARAM_TXRX;
		else
			params.ndis_udp6csum = NDIS_OFFLOAD_PARAM_RX;
	}

	aprint_debug_dev(sc->sc_dev, "offload csum: "
	    "ip4 %u, tcp4 %u, udp4 %u, tcp6 %u, udp6 %u\n",
	    params.ndis_ip4csum, params.ndis_tcp4csum, params.ndis_udp4csum,
	    params.ndis_tcp6csum, params.ndis_udp6csum);
	aprint_debug_dev(sc->sc_dev, "offload lsov2: ip4 %u, ip6 %u\n",
	    params.ndis_lsov2_ip4, params.ndis_lsov2_ip6);

	error = hvn_rndis_set(sc, OID_TCP_OFFLOAD_PARAMETERS, &params, len);
	if (error) {
		DPRINTF("%s: offload config failed: %d\n",
		    device_xname(sc->sc_dev), error);
		return error;
	}

	aprint_debug_dev(sc->sc_dev, "offload config done\n");
	sc->sc_caps |= caps;

	return 0;
}

static int
hvn_rndis_cmd(struct hvn_softc *sc, struct rndis_cmd *rc, u_int flags)
{
	struct hvn_rx_ring *rxr = &sc->sc_rxr[0];	/* primary channel */
	struct hvn_nvs_rndis *msg = &rc->rc_msg;
	struct rndis_msghdr *hdr = rc->rc_req;
	struct vmbus_gpa sgl[1];
	int tries = 10;
	int rv, s;

	msg->nvs_type = HVN_NVS_TYPE_RNDIS;
	msg->nvs_rndis_mtype = HVN_NVS_RNDIS_MTYPE_CTRL;
	msg->nvs_chim_idx = HVN_NVS_CHIM_IDX_INVALID;

	sgl[0].gpa_page = rc->rc_gpa;
	sgl[0].gpa_len = hdr->rm_len;
	sgl[0].gpa_ofs = 0;

	rc->rc_done = 0;

	mutex_enter(&rc->rc_lock);

	hvn_submit_cmd(sc, rc);

	do {
		rv = vmbus_channel_send_sgl(rxr->rxr_chan, sgl, 1, &rc->rc_msg,
		    sizeof(*msg), rc->rc_id);
		if (rv == EAGAIN) {
			DELAY(1000);
		} else if (rv) {
			mutex_exit(&rc->rc_lock);
			DPRINTF("%s: RNDIS operation %u send error %d\n",
			    device_xname(sc->sc_dev), hdr->rm_type, rv);
			hvn_rollback_cmd(sc, rc);
			return rv;
		}
	} while (rv != 0 && --tries > 0);

	if (tries == 0 && rv != 0) {
		mutex_exit(&rc->rc_lock);
		device_printf(sc->sc_dev,
		    "RNDIS operation %u send error %d\n", hdr->rm_type, rv);
		hvn_rollback_cmd(sc, rc);
		return rv;
	}
	if (vmbus_channel_is_revoked(rxr->rxr_chan) ||
	    ISSET(flags, HVN_RNDIS_CMD_NORESP)) {
		/* No response */
		mutex_exit(&rc->rc_lock);
		if (hvn_rollback_cmd(sc, rc))
			hvn_release_cmd(sc, rc);
		return 0;
	}

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_POSTWRITE);

	while (!rc->rc_done && !ISSET(sc->sc_flags, HVN_SCF_REVOKED)) {
		mutex_exit(&rc->rc_lock);
		DELAY(1000);
		s = splnet();
		hvn_nvs_intr1(rxr, 0, 0);
		splx(s);
		mutex_enter(&rc->rc_lock);
	}
	mutex_exit(&rc->rc_lock);

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_POSTREAD);

	if (!rc->rc_done) {
		rv = EINTR;
		if (hvn_rollback_cmd(sc, rc)) {
			hvn_release_cmd(sc, rc);
			rv = 0;
		}
		return rv;
	}

	hvn_release_cmd(sc, rc);
	return 0;
}

static int
hvn_rndis_input(struct hvn_rx_ring *rxr, uint64_t tid, void *arg)
{
	struct hvn_softc *sc = rxr->rxr_softc;
	struct vmbus_chanpkt_prplist *cp = arg;
	uint32_t off, len, type;
	int i, rv, rx = 0;
	bool qfull = false;

	if (sc->sc_rx_ring == NULL) {
		DPRINTF("%s: invalid rx ring\n", device_xname(sc->sc_dev));
		return 0;
	}

	for (i = 0; i < cp->cp_range_cnt; i++) {
		off = cp->cp_range[i].gpa_ofs;
		len = cp->cp_range[i].gpa_len;

		KASSERT(off + len <= sc->sc_rx_size);
		KASSERT(len >= RNDIS_HEADER_OFFSET + 4);

		memcpy(&type, sc->sc_rx_ring + off, sizeof(type));
		switch (type) {
		/* data message */
		case REMOTE_NDIS_PACKET_MSG:
			rv = hvn_rxeof(rxr, sc->sc_rx_ring + off, len);
			if (rv == 1)
				rx++;
			else if (rv == -1)	/* The receive queue is full. */
				qfull = true;
			break;
		/* completion messages */
		case REMOTE_NDIS_INITIALIZE_CMPLT:
		case REMOTE_NDIS_QUERY_CMPLT:
		case REMOTE_NDIS_SET_CMPLT:
		case REMOTE_NDIS_RESET_CMPLT:
		case REMOTE_NDIS_KEEPALIVE_CMPLT:
			hvn_rndis_complete(sc, sc->sc_rx_ring + off, len);
			break;
		/* notification message */
		case REMOTE_NDIS_INDICATE_STATUS_MSG:
			hvn_rndis_status(sc, sc->sc_rx_ring + off, len);
			break;
		default:
			device_printf(sc->sc_dev,
			    "unhandled RNDIS message type %u\n", type);
			break;
		}
	}

	hvn_nvs_ack(rxr, tid);

	if (qfull)
		return -1;
	return rx;
}

static inline struct mbuf *
hvn_devget(struct hvn_softc *sc, void *buf, uint32_t len)
{
	struct ifnet *ifp = SC2IFP(sc);
	struct mbuf *m;
	size_t size = len + ETHER_ALIGN + ETHER_VLAN_ENCAP_LEN;

	MGETHDR(m, M_NOWAIT, MT_DATA);
	if (m == NULL)
		return NULL;

	if (size > MHLEN) {
		if (size <= MCLBYTES)
			MCLGET(m, M_NOWAIT);
		else
			MEXTMALLOC(m, size, M_NOWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			return NULL;
		}
	}

	m->m_len = m->m_pkthdr.len = size;
	m_adj(m, ETHER_ALIGN + ETHER_VLAN_ENCAP_LEN);
	m_copyback(m, 0, len, buf);
	m_set_rcvif(m, ifp);
	return m;
}

#define HVN_RXINFO_CSUM		__BIT(NDIS_PKTINFO_TYPE_CSUM)
#define HVN_RXINFO_VLAN		__BIT(NDIS_PKTINFO_TYPE_VLAN)
#define HVN_RXINFO_HASHVAL	__BIT(HVN_NDIS_PKTINFO_TYPE_HASHVAL)
#define HVN_RXINFO_HASHINFO	__BIT(HVN_NDIS_PKTINFO_TYPE_HASHINF)
#define HVN_RXINFO_ALL		(HVN_RXINFO_CSUM | \
				 HVN_RXINFO_VLAN | \
				 HVN_RXINFO_HASHVAL | \
				 HVN_RXINFO_HASHINFO)

static int
hvn_rxeof(struct hvn_rx_ring *rxr, uint8_t *buf, uint32_t len)
{
	struct hvn_softc *sc = rxr->rxr_softc;
	struct ifnet *ifp = SC2IFP(sc);
	struct rndis_packet_msg *pkt;
	struct rndis_pktinfo *pi;
	struct mbuf *m;
	uint32_t mask, csum, vlan, hashval, hashinfo;

	if (!(ifp->if_flags & IFF_RUNNING))
		return 0;

	if (len < sizeof(*pkt)) {
		device_printf(sc->sc_dev, "data packet too short: %u\n",
		    len);
		return 0;
	}

	pkt = (struct rndis_packet_msg *)buf;
	if (pkt->rm_dataoffset + pkt->rm_datalen > len) {
		device_printf(sc->sc_dev,
		    "data packet out of bounds: %u@%u\n", pkt->rm_dataoffset,
		    pkt->rm_datalen);
		return 0;
	}

	if ((m = hvn_devget(sc, buf + RNDIS_HEADER_OFFSET + pkt->rm_dataoffset,
	    pkt->rm_datalen)) == NULL) {
		if_statinc(ifp, if_ierrors);
		return 0;
	}

	if (pkt->rm_pktinfooffset + pkt->rm_pktinfolen > len) {
		device_printf(sc->sc_dev,
		    "pktinfo is out of bounds: %u@%u vs %u\n",
		    pkt->rm_pktinfolen, pkt->rm_pktinfooffset, len);
		goto done;
	}

	mask = csum = hashval = hashinfo = 0;
	vlan = 0xffffffff;
	pi = (struct rndis_pktinfo *)(buf + RNDIS_HEADER_OFFSET +
	    pkt->rm_pktinfooffset);
	while (pkt->rm_pktinfolen > 0) {
		if (pi->rm_size > pkt->rm_pktinfolen) {
			device_printf(sc->sc_dev,
			    "invalid pktinfo size: %u/%u\n", pi->rm_size,
			    pkt->rm_pktinfolen);
			break;
		}

		switch (pi->rm_type) {
		case NDIS_PKTINFO_TYPE_CSUM:
			memcpy(&csum, pi->rm_data, sizeof(csum));
			SET(mask, HVN_RXINFO_CSUM);
			break;
		case NDIS_PKTINFO_TYPE_VLAN:
			memcpy(&vlan, pi->rm_data, sizeof(vlan));
			SET(mask, HVN_RXINFO_VLAN);
			break;
		case HVN_NDIS_PKTINFO_TYPE_HASHVAL:
			memcpy(&hashval, pi->rm_data, sizeof(hashval));
			SET(mask, HVN_RXINFO_HASHVAL);
			break;
		case HVN_NDIS_PKTINFO_TYPE_HASHINF:
			memcpy(&hashinfo, pi->rm_data, sizeof(hashinfo));
			SET(mask, HVN_RXINFO_HASHINFO);
			break;
		default:
			DPRINTF("%s: unhandled pktinfo type %u\n",
			    device_xname(sc->sc_dev), pi->rm_type);
			goto next;
		}

		if (mask == HVN_RXINFO_ALL) {
			/* All found; done */
			break;
		}
 next:
		pkt->rm_pktinfolen -= pi->rm_size;
		pi = (struct rndis_pktinfo *)((char *)pi + pi->rm_size);
	}

	/*
	 * Final fixup.
	 * - If there is no hash value, invalidate the hash info.
	 */
	if (!ISSET(mask, HVN_RXINFO_HASHVAL))
		hashinfo = 0;

	if (csum != 0) {
		if (ISSET(csum, NDIS_RXCSUM_INFO_IPCS_OK) &&
			ISSET(ifp->if_csum_flags_rx, M_CSUM_IPv4)) {
			SET(m->m_pkthdr.csum_flags, M_CSUM_IPv4);
			rxr->rxr_evcsum_ip.ev_count++;
		}
		if (ISSET(csum, NDIS_RXCSUM_INFO_TCPCS_OK) &&
			ISSET(ifp->if_csum_flags_rx, M_CSUM_TCPv4)) {
			SET(m->m_pkthdr.csum_flags, M_CSUM_TCPv4);
			rxr->rxr_evcsum_tcp.ev_count++;
		}
		if (ISSET(csum, NDIS_RXCSUM_INFO_UDPCS_OK) &&
			ISSET(ifp->if_csum_flags_rx, M_CSUM_UDPv4)) {
			SET(m->m_pkthdr.csum_flags, M_CSUM_UDPv4);
			rxr->rxr_evcsum_udp.ev_count++;
		}
	}

	if (vlan != 0xffffffff) {
		uint16_t t = NDIS_VLAN_INFO_ID(vlan);
		t |= NDIS_VLAN_INFO_PRI(vlan) << EVL_PRIO_BITS;
		t |= NDIS_VLAN_INFO_CFI(vlan) << EVL_CFI_BITS;

		if (ISSET(sc->sc_ec.ec_capenable, ETHERCAP_VLAN_HWTAGGING)) {
			vlan_set_tag(m, t);
			rxr->rxr_evvlanhwtagging.ev_count++;
		} else {
			struct ether_header eh;
			struct ether_vlan_header *evl;

			KDASSERT(m->m_pkthdr.len >= sizeof(eh));
			m_copydata(m, 0, sizeof(eh), &eh);
			M_PREPEND(m, ETHER_VLAN_ENCAP_LEN, M_NOWAIT);
			KDASSERT(m != NULL);

			evl = mtod(m, struct ether_vlan_header *);
			memcpy(evl->evl_dhost, eh.ether_dhost,
			    ETHER_ADDR_LEN * 2);
			evl->evl_encap_proto = htons(ETHERTYPE_VLAN);
			evl->evl_tag = htons(t);
			evl->evl_proto = eh.ether_type;
		}
	}

	/* XXX RSS hash is not supported. */

 done:
	rxr->rxr_evpkts.ev_count++;
	if_percpuq_enqueue(sc->sc_ipq, m);
	/* XXX Unable to detect that the receive queue is full. */
	return 1;
}

static void
hvn_rndis_complete(struct hvn_softc *sc, uint8_t *buf, uint32_t len)
{
	struct rndis_cmd *rc;
	uint32_t id;

	memcpy(&id, buf + RNDIS_HEADER_OFFSET, sizeof(id));
	if ((rc = hvn_complete_cmd(sc, id)) != NULL) {
		mutex_enter(&rc->rc_lock);
		if (len < rc->rc_cmplen)
			device_printf(sc->sc_dev,
			    "RNDIS response %u too short: %u\n", id, len);
		else
			memcpy(&rc->rc_cmp, buf, rc->rc_cmplen);
		if (len > rc->rc_cmplen &&
		    len - rc->rc_cmplen > HVN_RNDIS_BUFSIZE)
			device_printf(sc->sc_dev,
			    "RNDIS response %u too large: %u\n", id, len);
		else if (len > rc->rc_cmplen)
			memcpy(&rc->rc_cmpbuf, buf + rc->rc_cmplen,
			    len - rc->rc_cmplen);
		rc->rc_done = 1;
		cv_signal(&rc->rc_cv);
		mutex_exit(&rc->rc_lock);
	} else {
		DPRINTF("%s: failed to complete RNDIS request id %u\n",
		    device_xname(sc->sc_dev), id);
	}
}

static int
hvn_rndis_output_sgl(struct hvn_tx_ring *txr, struct hvn_tx_desc *txd)
{
	struct hvn_softc *sc = txr->txr_softc;
	uint64_t rid = (uint64_t)txd->txd_id << 32;
	int rv;

	rv = vmbus_channel_send_sgl(txr->txr_chan, txd->txd_sgl, txd->txd_nsge,
	    &sc->sc_data_msg, sizeof(sc->sc_data_msg), rid);
	if (rv) {
		DPRINTF("%s: RNDIS data send error %d\n",
		    device_xname(sc->sc_dev), rv);
		return rv;
	}
	return 0;
}

static int
hvn_rndis_output_chim(struct hvn_tx_ring *txr, struct hvn_tx_desc *txd)
{
	struct hvn_nvs_rndis rndis;
	uint64_t rid = (uint64_t)txd->txd_id << 32;
	int rv;

	memset(&rndis, 0, sizeof(rndis));
	rndis.nvs_type = HVN_NVS_TYPE_RNDIS;
	rndis.nvs_rndis_mtype = HVN_NVS_RNDIS_MTYPE_DATA;
	rndis.nvs_chim_idx = txd->txd_chim_index;
	rndis.nvs_chim_sz = txd->txd_chim_size;

	rv = vmbus_channel_send(txr->txr_chan, &rndis, sizeof(rndis),
	    rid, VMBUS_CHANPKT_TYPE_INBAND, VMBUS_CHANPKT_FLAG_RC);
	if (rv) {
		DPRINTF("%s: RNDIS chimney data send error %d: idx %u, sz %u\n",
		    device_xname(sc->sc_dev), rv, rndis.nvs_chim_idx,
		    rndis.nvs_chim_sz);
		return rv;
	}
	return 0;
}

static void
hvn_rndis_status(struct hvn_softc *sc, uint8_t *buf, uint32_t len)
{
	uint32_t status;

	memcpy(&status, buf + RNDIS_HEADER_OFFSET, sizeof(status));
	switch (status) {
	case RNDIS_STATUS_MEDIA_CONNECT:
	case RNDIS_STATUS_MEDIA_DISCONNECT:
		hvn_link_event(sc, HVN_LINK_EV_STATE_CHANGE);
		break;
	case RNDIS_STATUS_NETWORK_CHANGE:
		hvn_link_event(sc, HVN_LINK_EV_NETWORK_CHANGE);
		break;
	/* Ignore these */
	case RNDIS_STATUS_OFFLOAD_CURRENT_CONFIG:
	case RNDIS_STATUS_LINK_SPEED_CHANGE:
		return;
	default:
		DPRINTF("%s: unhandled status %#x\n", device_xname(sc->sc_dev),
		    status);
		return;
	}
}

static int
hvn_rndis_query(struct hvn_softc *sc, uint32_t oid, void *res, size_t *length)
{

	return hvn_rndis_query2(sc, oid, NULL, 0, res, length, 0);
}

static int
hvn_rndis_query2(struct hvn_softc *sc, uint32_t oid, const void *idata,
    size_t idlen, void *odata, size_t *odlen, size_t min_odlen)
{
	struct rndis_cmd *rc;
	struct rndis_query_req *req;
	struct rndis_query_comp *cmp;
	size_t olength = *odlen;
	int rv;

	rc = hvn_alloc_cmd(sc);

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREREAD);

	rc->rc_id = atomic_inc_uint_nv(&sc->sc_rndisrid);

	req = rc->rc_req;
	req->rm_type = REMOTE_NDIS_QUERY_MSG;
	req->rm_len = sizeof(*req) + idlen;
	req->rm_rid = rc->rc_id;
	req->rm_oid = oid;
	req->rm_infobufoffset = sizeof(*req) - RNDIS_HEADER_OFFSET;
	if (idlen > 0) {
		KASSERT(sizeof(*req) + idlen <= PAGE_SIZE);
		req->rm_infobuflen = idlen;
		memcpy(req + 1, idata, idlen);
	}

	rc->rc_cmplen = sizeof(*cmp);

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREWRITE);

	if ((rv = hvn_rndis_cmd(sc, rc, 0)) != 0) {
		DPRINTF("%s: QUERY_MSG failed, error %d\n",
		    device_xname(sc->sc_dev), rv);
		hvn_free_cmd(sc, rc);
		return rv;
	}

	cmp = (struct rndis_query_comp *)&rc->rc_cmp;
	switch (cmp->rm_status) {
	case RNDIS_STATUS_SUCCESS:
		if (cmp->rm_infobuflen > olength ||
		    (min_odlen > 0 && cmp->rm_infobuflen < min_odlen)) {
			rv = EINVAL;
			break;
		}
		memcpy(odata, rc->rc_cmpbuf, cmp->rm_infobuflen);
		*odlen = cmp->rm_infobuflen;
		break;
	default:
		*odlen = 0;
		rv = EIO;
		break;
	}

	hvn_free_cmd(sc, rc);
	return rv;
}

static int
hvn_rndis_set(struct hvn_softc *sc, uint32_t oid, void *data, size_t length)
{
	struct rndis_cmd *rc;
	struct rndis_set_req *req;
	struct rndis_set_comp *cmp;
	int rv;

	rc = hvn_alloc_cmd(sc);

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREREAD);

	rc->rc_id = atomic_inc_uint_nv(&sc->sc_rndisrid);

	req = rc->rc_req;
	req->rm_type = REMOTE_NDIS_SET_MSG;
	req->rm_len = sizeof(*req) + length;
	req->rm_rid = rc->rc_id;
	req->rm_oid = oid;
	req->rm_infobufoffset = sizeof(*req) - RNDIS_HEADER_OFFSET;

	rc->rc_cmplen = sizeof(*cmp);

	if (length > 0) {
		KASSERT(sizeof(*req) + length < PAGE_SIZE);
		req->rm_infobuflen = length;
		memcpy(req + 1, data, length);
	}

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREWRITE);

	if ((rv = hvn_rndis_cmd(sc, rc, 0)) != 0) {
		DPRINTF("%s: SET_MSG failed, error %d\n",
		    device_xname(sc->sc_dev), rv);
		hvn_free_cmd(sc, rc);
		return rv;
	}

	cmp = (struct rndis_set_comp *)&rc->rc_cmp;
	if (cmp->rm_status != RNDIS_STATUS_SUCCESS)
		rv = EIO;

	hvn_free_cmd(sc, rc);
	return rv;
}

static int
hvn_rndis_open(struct hvn_softc *sc)
{
	struct ifnet *ifp = SC2IFP(sc);
	uint32_t filter;
	int rv;

	if (ifp->if_flags & IFF_PROMISC) {
		filter = RNDIS_PACKET_TYPE_PROMISCUOUS;
	} else {
		filter = RNDIS_PACKET_TYPE_DIRECTED;
		if (ifp->if_flags & IFF_BROADCAST)
			filter |= RNDIS_PACKET_TYPE_BROADCAST;
		if (ifp->if_flags & IFF_ALLMULTI)
			filter |= RNDIS_PACKET_TYPE_ALL_MULTICAST;
		else {
			struct ethercom *ec = &sc->sc_ec;
			struct ether_multi *enm;
			struct ether_multistep step;

			ETHER_LOCK(ec);
			ETHER_FIRST_MULTI(step, ec, enm);
			/* TODO: support multicast list */
			if (enm != NULL)
				filter |= RNDIS_PACKET_TYPE_ALL_MULTICAST;
			ETHER_UNLOCK(ec);
		}
	}

	rv = hvn_rndis_set(sc, OID_GEN_CURRENT_PACKET_FILTER,
	    &filter, sizeof(filter));
	if (rv) {
		DPRINTF("%s: failed to set RNDIS filter to %#x\n",
		    device_xname(sc->sc_dev), filter);
	}
	return rv;
}

static int
hvn_rndis_close(struct hvn_softc *sc)
{
	uint32_t filter = 0;
	int rv;

	rv = hvn_rndis_set(sc, OID_GEN_CURRENT_PACKET_FILTER,
	    &filter, sizeof(filter));
	if (rv) {
		DPRINTF("%s: failed to clear RNDIS filter\n",
		    device_xname(sc->sc_dev));
	}
	return rv;
}

static void
hvn_rndis_detach(struct hvn_softc *sc)
{
	struct rndis_cmd *rc;
	struct rndis_halt_req *req;
	int rv;

	rc = hvn_alloc_cmd(sc);

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREREAD);

	rc->rc_id = atomic_inc_uint_nv(&sc->sc_rndisrid);

	req = rc->rc_req;
	req->rm_type = REMOTE_NDIS_HALT_MSG;
	req->rm_len = sizeof(*req);
	req->rm_rid = rc->rc_id;

	bus_dmamap_sync(sc->sc_dmat, rc->rc_dmap, 0, PAGE_SIZE,
	    BUS_DMASYNC_PREWRITE);

	/* No RNDIS completion; rely on NVS message send completion */
	if ((rv = hvn_rndis_cmd(sc, rc, HVN_RNDIS_CMD_NORESP)) != 0) {
		DPRINTF("%s: HALT_MSG failed, error %d\n",
		    device_xname(sc->sc_dev), rv);
	}
	hvn_free_cmd(sc, rc);
}

static void
hvn_init_sysctls(struct hvn_softc *sc)
{
	struct sysctllog **log;
	const struct sysctlnode *rnode, *cnode, *rxnode, *txnode;
	const char *dvname;
	int error;

	log = &sc->sc_sysctllog;
	dvname = device_xname(sc->sc_dev);

	error = sysctl_createv(log, 0, NULL, &rnode,
	    0, CTLTYPE_NODE, dvname,
	    SYSCTL_DESCR("hvn information and settings"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);
	if (error)
		goto err;

	error = sysctl_createv(log, 0, &rnode, &cnode,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "txrx_workqueue",
	    SYSCTL_DESCR("Use workqueue for packet processing"),
	    NULL, 0, &sc->sc_txrx_workqueue, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rnode, &rxnode,
	    0, CTLTYPE_NODE, "rx",
	    SYSCTL_DESCR("hvn information and settings for Rx"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
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
	    SYSCTL_DESCR("hvn information and settings for Tx"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
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

	return;

out:
	sysctl_teardown(log);
	sc->sc_sysctllog = NULL;
err:
	aprint_error_dev(sc->sc_dev, "sysctl_createv failed (err = %d)\n",
	    error);
}

SYSCTL_SETUP(sysctl_hw_hvn_setup, "sysctl hw.hvn setup")
{
	const struct sysctlnode *rnode;
	const struct sysctlnode *cnode;
	int error;

	error = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "hvn",
	    SYSCTL_DESCR("hvn global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);
	if (error)
		goto fail;

	error = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "udp_csum_fixup_mtu",
	    SYSCTL_DESCR("UDP checksum offloding fixup MTU"),
	    NULL, 0, &hvn_udpcs_fixup_mtu, sizeof(hvn_udpcs_fixup_mtu),
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto fail;

	error = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "chimney_size",
	    SYSCTL_DESCR("Chimney send packet size limit"),
	    NULL, 0, &hvn_tx_chimney_size, sizeof(hvn_tx_chimney_size),
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto fail;

	error = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "channel_count",
	    SYSCTL_DESCR("# of channels to use"),
	    NULL, 0, &hvn_channel_cnt, sizeof(hvn_channel_cnt),
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto fail;

	error = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "tx_ring_count",
	    SYSCTL_DESCR("# of transmit rings to use"),
	    NULL, 0, &hvn_tx_ring_cnt, sizeof(hvn_tx_ring_cnt),
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto fail;

	return;

fail:
	aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, error);
}
