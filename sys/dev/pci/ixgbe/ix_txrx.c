/* $NetBSD: ix_txrx.c,v 1.97 2022/04/25 07:51:12 msaitoh Exp $ */

/******************************************************************************

  Copyright (c) 2001-2017, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   3. Neither the name of the Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/
/*$FreeBSD: head/sys/dev/ixgbe/ix_txrx.c 327031 2017-12-20 18:15:06Z erj $*/

/*
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Coyote Point Systems, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: ix_txrx.c,v 1.97 2022/04/25 07:51:12 msaitoh Exp $");

#include "opt_inet.h"
#include "opt_inet6.h"

#include "ixgbe.h"

/*
 * HW RSC control:
 *  this feature only works with
 *  IPv4, and only on 82599 and later.
 *  Also this will cause IP forwarding to
 *  fail and that can't be controlled by
 *  the stack as LRO can. For all these
 *  reasons I've deemed it best to leave
 *  this off and not bother with a tuneable
 *  interface, this would need to be compiled
 *  to enable.
 */
static bool ixgbe_rsc_enable = FALSE;

/*
 * For Flow Director: this is the
 * number of TX packets we sample
 * for the filter pool, this means
 * every 20th packet will be probed.
 *
 * This feature can be disabled by
 * setting this to 0.
 */
static int atr_sample_rate = 20;

#define IXGBE_M_ADJ(adapter, rxr, mp)					\
	if (adapter->max_frame_size <= (rxr->mbuf_sz - ETHER_ALIGN))	\
		m_adj(mp, ETHER_ALIGN)

/************************************************************************
 *  Local Function prototypes
 ************************************************************************/
static void          ixgbe_setup_transmit_ring(struct tx_ring *);
static void          ixgbe_free_transmit_buffers(struct tx_ring *);
static int           ixgbe_setup_receive_ring(struct rx_ring *);
static void          ixgbe_free_receive_buffers(struct rx_ring *);
static void          ixgbe_rx_checksum(u32, struct mbuf *, u32,
                                       struct ixgbe_hw_stats *);
static void          ixgbe_refresh_mbufs(struct rx_ring *, int);
static void          ixgbe_drain(struct ifnet *, struct tx_ring *);
static int           ixgbe_xmit(struct tx_ring *, struct mbuf *);
static int           ixgbe_tx_ctx_setup(struct tx_ring *,
                                        struct mbuf *, u32 *, u32 *);
static int           ixgbe_tso_setup(struct tx_ring *,
                                     struct mbuf *, u32 *, u32 *);
static __inline void ixgbe_rx_discard(struct rx_ring *, int);
static __inline void ixgbe_rx_input(struct rx_ring *, struct ifnet *,
                                    struct mbuf *, u32);
static int           ixgbe_dma_malloc(struct adapter *, bus_size_t,
                                      struct ixgbe_dma_alloc *, int);
static void          ixgbe_dma_free(struct adapter *, struct ixgbe_dma_alloc *);

static void	ixgbe_setup_hw_rsc(struct rx_ring *);

/************************************************************************
 * ixgbe_legacy_start_locked - Transmit entry point
 *
 *   Called by the stack to initiate a transmit.
 *   The driver will remain in this routine as long as there are
 *   packets to transmit and transmit resources are available.
 *   In case resources are not available, the stack is notified
 *   and the packet is requeued.
 ************************************************************************/
int
ixgbe_legacy_start_locked(struct ifnet *ifp, struct tx_ring *txr)
{
	int rc;
	struct mbuf    *m_head;
	struct adapter *adapter = txr->adapter;

	IXGBE_TX_LOCK_ASSERT(txr);

	if (adapter->link_active != LINK_STATE_UP) {
		/*
		 * discard all packets buffered in IFQ to avoid
		 * sending old packets at next link up timing.
		 */
		ixgbe_drain(ifp, txr);
		return (ENETDOWN);
	}
	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return (ENETDOWN);
	if (txr->txr_no_space)
		return (ENETDOWN);

	while (!IFQ_IS_EMPTY(&ifp->if_snd)) {
		if (txr->tx_avail <= IXGBE_QUEUE_MIN_FREE)
			break;

		IFQ_POLL(&ifp->if_snd, m_head);
		if (m_head == NULL)
			break;

		if ((rc = ixgbe_xmit(txr, m_head)) == EAGAIN) {
			break;
		}
		IFQ_DEQUEUE(&ifp->if_snd, m_head);
		if (rc != 0) {
			m_freem(m_head);
			continue;
		}

		/* Send a copy of the frame to the BPF listener */
		bpf_mtap(ifp, m_head, BPF_D_OUT);
	}

	return IXGBE_SUCCESS;
} /* ixgbe_legacy_start_locked */

/************************************************************************
 * ixgbe_legacy_start
 *
 *   Called by the stack, this always uses the first tx ring,
 *   and should not be used with multiqueue tx enabled.
 ************************************************************************/
void
ixgbe_legacy_start(struct ifnet *ifp)
{
	struct adapter *adapter = ifp->if_softc;
	struct tx_ring *txr = adapter->tx_rings;

	if (ifp->if_flags & IFF_RUNNING) {
		IXGBE_TX_LOCK(txr);
		ixgbe_legacy_start_locked(ifp, txr);
		IXGBE_TX_UNLOCK(txr);
	}
} /* ixgbe_legacy_start */

/************************************************************************
 * ixgbe_mq_start - Multiqueue Transmit Entry Point
 *
 *   (if_transmit function)
 ************************************************************************/
int
ixgbe_mq_start(struct ifnet *ifp, struct mbuf *m)
{
	struct adapter	*adapter = ifp->if_softc;
	struct tx_ring	*txr;
	int		i;
#ifdef RSS
	uint32_t bucket_id;
#endif

	/*
	 * When doing RSS, map it to the same outbound queue
	 * as the incoming flow would be mapped to.
	 *
	 * If everything is setup correctly, it should be the
	 * same bucket that the current CPU we're on is.
	 */
#ifdef RSS
	if (M_HASHTYPE_GET(m) != M_HASHTYPE_NONE) {
		if ((adapter->feat_en & IXGBE_FEATURE_RSS) &&
		    (rss_hash2bucket(m->m_pkthdr.flowid, M_HASHTYPE_GET(m),
		    &bucket_id) == 0)) {
			i = bucket_id % adapter->num_queues;
#ifdef IXGBE_DEBUG
			if (bucket_id > adapter->num_queues)
				if_printf(ifp,
				    "bucket_id (%d) > num_queues (%d)\n",
				    bucket_id, adapter->num_queues);
#endif
		} else
			i = m->m_pkthdr.flowid % adapter->num_queues;
	} else
#endif /* 0 */
		i = (cpu_index(curcpu()) % ncpu) % adapter->num_queues;

	/* Check for a hung queue and pick alternative */
	if (((1ULL << i) & adapter->active_queues) == 0)
		i = ffs64(adapter->active_queues);

	txr = &adapter->tx_rings[i];

	if (__predict_false(!pcq_put(txr->txr_interq, m))) {
		m_freem(m);
		IXGBE_EVC_ADD(&txr->pcq_drops, 1);
		return ENOBUFS;
	}
	if (IXGBE_TX_TRYLOCK(txr)) {
		ixgbe_mq_start_locked(ifp, txr);
		IXGBE_TX_UNLOCK(txr);
	} else {
		if (adapter->txrx_use_workqueue) {
			u_int *enqueued;

			/*
			 * This function itself is not called in interrupt
			 * context, however it can be called in fast softint
			 * context right after receiving forwarding packets.
			 * So, it is required to protect workqueue from twice
			 * enqueuing when the machine uses both spontaneous
			 * packets and forwarding packets.
			 */
			enqueued = percpu_getref(adapter->txr_wq_enqueued);
			if (*enqueued == 0) {
				*enqueued = 1;
				percpu_putref(adapter->txr_wq_enqueued);
				workqueue_enqueue(adapter->txr_wq,
				    &txr->wq_cookie, curcpu());
			} else
				percpu_putref(adapter->txr_wq_enqueued);
		} else {
			kpreempt_disable();
			softint_schedule(txr->txr_si);
			kpreempt_enable();
		}
	}

	return (0);
} /* ixgbe_mq_start */

/************************************************************************
 * ixgbe_mq_start_locked
 ************************************************************************/
int
ixgbe_mq_start_locked(struct ifnet *ifp, struct tx_ring *txr)
{
	struct mbuf    *next;
	int            enqueued = 0, err = 0;

	if (txr->adapter->link_active != LINK_STATE_UP) {
		/*
		 * discard all packets buffered in txr_interq to avoid
		 * sending old packets at next link up timing.
		 */
		ixgbe_drain(ifp, txr);
		return (ENETDOWN);
	}
	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return (ENETDOWN);
	if (txr->txr_no_space)
		return (ENETDOWN);

	/* Process the queue */
	while ((next = pcq_get(txr->txr_interq)) != NULL) {
		if ((err = ixgbe_xmit(txr, next)) != 0) {
			m_freem(next);
			/* All errors are counted in ixgbe_xmit() */
			break;
		}
		enqueued++;
#if __FreeBSD_version >= 1100036
		/*
		 * Since we're looking at the tx ring, we can check
		 * to see if we're a VF by examing our tail register
		 * address.
		 */
		if ((txr->adapter->feat_en & IXGBE_FEATURE_VF) &&
		    (next->m_flags & M_MCAST))
			if_inc_counter(ifp, IFCOUNTER_OMCASTS, 1);
#endif
		/* Send a copy of the frame to the BPF listener */
		bpf_mtap(ifp, next, BPF_D_OUT);
		if ((ifp->if_flags & IFF_RUNNING) == 0)
			break;
	}

	if (txr->tx_avail < IXGBE_TX_CLEANUP_THRESHOLD(txr->adapter))
		ixgbe_txeof(txr);

	return (err);
} /* ixgbe_mq_start_locked */

/************************************************************************
 * ixgbe_deferred_mq_start
 *
 *   Called from a softint and workqueue (indirectly) to drain queued
 *   transmit packets.
 ************************************************************************/
void
ixgbe_deferred_mq_start(void *arg)
{
	struct tx_ring *txr = arg;
	struct adapter *adapter = txr->adapter;
	struct ifnet   *ifp = adapter->ifp;

	IXGBE_TX_LOCK(txr);
	if (pcq_peek(txr->txr_interq) != NULL)
		ixgbe_mq_start_locked(ifp, txr);
	IXGBE_TX_UNLOCK(txr);
} /* ixgbe_deferred_mq_start */

/************************************************************************
 * ixgbe_deferred_mq_start_work
 *
 *   Called from a workqueue to drain queued transmit packets.
 ************************************************************************/
void
ixgbe_deferred_mq_start_work(struct work *wk, void *arg)
{
	struct tx_ring *txr = container_of(wk, struct tx_ring, wq_cookie);
	struct adapter *adapter = txr->adapter;
	u_int *enqueued = percpu_getref(adapter->txr_wq_enqueued);
	*enqueued = 0;
	percpu_putref(adapter->txr_wq_enqueued);

	ixgbe_deferred_mq_start(txr);
} /* ixgbe_deferred_mq_start */

/************************************************************************
 * ixgbe_drain_all
 ************************************************************************/
void
ixgbe_drain_all(struct adapter *adapter)
{
	struct ifnet *ifp = adapter->ifp;
	struct ix_queue *que = adapter->queues;

	for (int i = 0; i < adapter->num_queues; i++, que++) {
		struct tx_ring  *txr = que->txr;

		IXGBE_TX_LOCK(txr);
		ixgbe_drain(ifp, txr);
		IXGBE_TX_UNLOCK(txr);
	}
}

/************************************************************************
 * ixgbe_xmit
 *
 *   Maps the mbufs to tx descriptors, allowing the
 *   TX engine to transmit the packets.
 *
 *   Return 0 on success, positive on failure
 ************************************************************************/
static int
ixgbe_xmit(struct tx_ring *txr, struct mbuf *m_head)
{
	struct adapter          *adapter = txr->adapter;
	struct ixgbe_tx_buf     *txbuf;
	union ixgbe_adv_tx_desc *txd = NULL;
	struct ifnet	        *ifp = adapter->ifp;
	int                     i, j, error;
	int                     first;
	u32                     olinfo_status = 0, cmd_type_len;
	bool                    remap = TRUE;
	bus_dmamap_t            map;

	/* Basic descriptor defines */
	cmd_type_len = (IXGBE_ADVTXD_DTYP_DATA |
	    IXGBE_ADVTXD_DCMD_IFCS | IXGBE_ADVTXD_DCMD_DEXT);

	if (vlan_has_tag(m_head))
		cmd_type_len |= IXGBE_ADVTXD_DCMD_VLE;

	/*
	 * Important to capture the first descriptor
	 * used because it will contain the index of
	 * the one we tell the hardware to report back
	 */
	first = txr->next_avail_desc;
	txbuf = &txr->tx_buffers[first];
	map = txbuf->map;

	/*
	 * Map the packet for DMA.
	 */
retry:
	error = bus_dmamap_load_mbuf(txr->txtag->dt_dmat, map, m_head,
	    BUS_DMA_NOWAIT);

	if (__predict_false(error)) {
		struct mbuf *m;

		switch (error) {
		case EAGAIN:
			txr->q_eagain_tx_dma_setup++;
			return EAGAIN;
		case ENOMEM:
			txr->q_enomem_tx_dma_setup++;
			return EAGAIN;
		case EFBIG:
			/* Try it again? - one try */
			if (remap == TRUE) {
				remap = FALSE;
				/*
				 * XXX: m_defrag will choke on
				 * non-MCLBYTES-sized clusters
				 */
				txr->q_efbig_tx_dma_setup++;
				m = m_defrag(m_head, M_NOWAIT);
				if (m == NULL) {
					txr->q_mbuf_defrag_failed++;
					return ENOBUFS;
				}
				m_head = m;
				goto retry;
			} else {
				txr->q_efbig2_tx_dma_setup++;
				return error;
			}
		case EINVAL:
			txr->q_einval_tx_dma_setup++;
			return error;
		default:
			txr->q_other_tx_dma_setup++;
			return error;
		}
	}

	/* Make certain there are enough descriptors */
	if (txr->tx_avail < (map->dm_nsegs + 2)) {
		txr->txr_no_space = true;
		IXGBE_EVC_ADD(&txr->no_desc_avail, 1);
		ixgbe_dmamap_unload(txr->txtag, txbuf->map);
		return EAGAIN;
	}

	/*
	 * Set up the appropriate offload context
	 * this will consume the first descriptor
	 */
	error = ixgbe_tx_ctx_setup(txr, m_head, &cmd_type_len, &olinfo_status);
	if (__predict_false(error)) {
		return (error);
	}

#ifdef IXGBE_FDIR
	/* Do the flow director magic */
	if ((adapter->feat_en & IXGBE_FEATURE_FDIR) &&
	    (txr->atr_sample) && (!adapter->fdir_reinit)) {
		++txr->atr_count;
		if (txr->atr_count >= atr_sample_rate) {
			ixgbe_atr(txr, m_head);
			txr->atr_count = 0;
		}
	}
#endif

	olinfo_status |= IXGBE_ADVTXD_CC;
	i = txr->next_avail_desc;
	for (j = 0; j < map->dm_nsegs; j++) {
		bus_size_t seglen;
		uint64_t segaddr;

		txbuf = &txr->tx_buffers[i];
		txd = &txr->tx_base[i];
		seglen = map->dm_segs[j].ds_len;
		segaddr = htole64(map->dm_segs[j].ds_addr);

		txd->read.buffer_addr = segaddr;
		txd->read.cmd_type_len = htole32(cmd_type_len | seglen);
		txd->read.olinfo_status = htole32(olinfo_status);

		if (++i == txr->num_desc)
			i = 0;
	}

	txd->read.cmd_type_len |= htole32(IXGBE_TXD_CMD_EOP | IXGBE_TXD_CMD_RS);
	txr->tx_avail -= map->dm_nsegs;
	txr->next_avail_desc = i;

	txbuf->m_head = m_head;
	/*
	 * Here we swap the map so the last descriptor,
	 * which gets the completion interrupt has the
	 * real map, and the first descriptor gets the
	 * unused map from this descriptor.
	 */
	txr->tx_buffers[first].map = txbuf->map;
	txbuf->map = map;
	bus_dmamap_sync(txr->txtag->dt_dmat, map, 0, m_head->m_pkthdr.len,
	    BUS_DMASYNC_PREWRITE);

	/* Set the EOP descriptor that will be marked done */
	txbuf = &txr->tx_buffers[first];
	txbuf->eop = txd;

	ixgbe_dmamap_sync(txr->txdma.dma_tag, txr->txdma.dma_map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	/*
	 * Advance the Transmit Descriptor Tail (Tdt), this tells the
	 * hardware that this frame is available to transmit.
	 */
	IXGBE_EVC_ADD(&txr->total_packets, 1);
	IXGBE_WRITE_REG(&adapter->hw, txr->tail, i);

	net_stat_ref_t nsr = IF_STAT_GETREF(ifp);
	if_statadd_ref(nsr, if_obytes, m_head->m_pkthdr.len);
	if (m_head->m_flags & M_MCAST)
		if_statinc_ref(nsr, if_omcasts);
	IF_STAT_PUTREF(ifp);

	/* Mark queue as having work */
	if (txr->busy == 0)
		txr->busy = 1;

	return (0);
} /* ixgbe_xmit */

/************************************************************************
 * ixgbe_drain
 ************************************************************************/
static void
ixgbe_drain(struct ifnet *ifp, struct tx_ring *txr)
{
	struct mbuf *m;

	IXGBE_TX_LOCK_ASSERT(txr);

	if (txr->me == 0) {
		while (!IFQ_IS_EMPTY(&ifp->if_snd)) {
			IFQ_DEQUEUE(&ifp->if_snd, m);
			m_freem(m);
			IF_DROP(&ifp->if_snd);
		}
	}

	while ((m = pcq_get(txr->txr_interq)) != NULL) {
		m_freem(m);
		IXGBE_EVC_ADD(&txr->pcq_drops, 1);
	}
}

/************************************************************************
 * ixgbe_allocate_transmit_buffers
 *
 *   Allocate memory for tx_buffer structures. The tx_buffer stores all
 *   the information needed to transmit a packet on the wire. This is
 *   called only once at attach, setup is done every reset.
 ************************************************************************/
static int
ixgbe_allocate_transmit_buffers(struct tx_ring *txr)
{
	struct adapter      *adapter = txr->adapter;
	device_t            dev = adapter->dev;
	struct ixgbe_tx_buf *txbuf;
	int                 error, i;

	/*
	 * Setup DMA descriptor areas.
	 */
	error = ixgbe_dma_tag_create(
	         /*      parent */ adapter->osdep.dmat,
	         /*   alignment */ 1,
	         /*      bounds */ 0,
	         /*     maxsize */ IXGBE_TSO_SIZE,
	         /*   nsegments */ adapter->num_segs,
	         /*  maxsegsize */ PAGE_SIZE,
	         /*       flags */ 0,
	                           &txr->txtag);
	if (error != 0) {
		aprint_error_dev(dev,"Unable to allocate TX DMA tag\n");
		goto fail;
	}

	txr->tx_buffers = malloc(sizeof(struct ixgbe_tx_buf) *
	    adapter->num_tx_desc, M_DEVBUF, M_WAITOK | M_ZERO);

	/* Create the descriptor buffer dma maps */
	txbuf = txr->tx_buffers;
	for (i = 0; i < adapter->num_tx_desc; i++, txbuf++) {
		error = ixgbe_dmamap_create(txr->txtag, 0, &txbuf->map);
		if (error != 0) {
			aprint_error_dev(dev,
			    "Unable to create TX DMA map (%d)\n", error);
			goto fail;
		}
	}

	return 0;
fail:
	/* We free all, it handles case where we are in the middle */
#if 0 /* XXX was FreeBSD */
	ixgbe_free_transmit_structures(adapter);
#else
	ixgbe_free_transmit_buffers(txr);
#endif
	return (error);
} /* ixgbe_allocate_transmit_buffers */

/************************************************************************
 * ixgbe_setup_transmit_ring - Initialize a transmit ring.
 ************************************************************************/
static void
ixgbe_setup_transmit_ring(struct tx_ring *txr)
{
	struct adapter        *adapter = txr->adapter;
	struct ixgbe_tx_buf   *txbuf;
#ifdef DEV_NETMAP
	struct netmap_adapter *na = NA(adapter->ifp);
	struct netmap_slot    *slot;
#endif /* DEV_NETMAP */

	/* Clear the old ring contents */
	IXGBE_TX_LOCK(txr);

#ifdef DEV_NETMAP
	if (adapter->feat_en & IXGBE_FEATURE_NETMAP) {
		/*
		 * (under lock): if in netmap mode, do some consistency
		 * checks and set slot to entry 0 of the netmap ring.
		 */
		slot = netmap_reset(na, NR_TX, txr->me, 0);
	}
#endif /* DEV_NETMAP */

	bzero((void *)txr->tx_base,
	    (sizeof(union ixgbe_adv_tx_desc)) * adapter->num_tx_desc);
	/* Reset indices */
	txr->next_avail_desc = 0;
	txr->next_to_clean = 0;

	/* Free any existing tx buffers. */
	txbuf = txr->tx_buffers;
	for (int i = 0; i < txr->num_desc; i++, txbuf++) {
		if (txbuf->m_head != NULL) {
			bus_dmamap_sync(txr->txtag->dt_dmat, txbuf->map,
			    0, txbuf->m_head->m_pkthdr.len,
			    BUS_DMASYNC_POSTWRITE);
			ixgbe_dmamap_unload(txr->txtag, txbuf->map);
			m_freem(txbuf->m_head);
			txbuf->m_head = NULL;
		}

#ifdef DEV_NETMAP
		/*
		 * In netmap mode, set the map for the packet buffer.
		 * NOTE: Some drivers (not this one) also need to set
		 * the physical buffer address in the NIC ring.
		 * Slots in the netmap ring (indexed by "si") are
		 * kring->nkr_hwofs positions "ahead" wrt the
		 * corresponding slot in the NIC ring. In some drivers
		 * (not here) nkr_hwofs can be negative. Function
		 * netmap_idx_n2k() handles wraparounds properly.
		 */
		if ((adapter->feat_en & IXGBE_FEATURE_NETMAP) && slot) {
			int si = netmap_idx_n2k(na->tx_rings[txr->me], i);
			netmap_load_map(na, txr->txtag,
			    txbuf->map, NMB(na, slot + si));
		}
#endif /* DEV_NETMAP */

		/* Clear the EOP descriptor pointer */
		txbuf->eop = NULL;
	}

	/* Set the rate at which we sample packets */
	if (adapter->feat_en & IXGBE_FEATURE_FDIR)
		txr->atr_sample = atr_sample_rate;

	/* Set number of descriptors available */
	txr->tx_avail = adapter->num_tx_desc;

	ixgbe_dmamap_sync(txr->txdma.dma_tag, txr->txdma.dma_map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	IXGBE_TX_UNLOCK(txr);
} /* ixgbe_setup_transmit_ring */

/************************************************************************
 * ixgbe_setup_transmit_structures - Initialize all transmit rings.
 ************************************************************************/
int
ixgbe_setup_transmit_structures(struct adapter *adapter)
{
	struct tx_ring *txr = adapter->tx_rings;

	for (int i = 0; i < adapter->num_queues; i++, txr++)
		ixgbe_setup_transmit_ring(txr);

	return (0);
} /* ixgbe_setup_transmit_structures */

/************************************************************************
 * ixgbe_free_transmit_structures - Free all transmit rings.
 ************************************************************************/
void
ixgbe_free_transmit_structures(struct adapter *adapter)
{
	struct tx_ring *txr = adapter->tx_rings;

	for (int i = 0; i < adapter->num_queues; i++, txr++) {
		ixgbe_free_transmit_buffers(txr);
		ixgbe_dma_free(adapter, &txr->txdma);
		IXGBE_TX_LOCK_DESTROY(txr);
	}
	free(adapter->tx_rings, M_DEVBUF);
} /* ixgbe_free_transmit_structures */

/************************************************************************
 * ixgbe_free_transmit_buffers
 *
 *   Free transmit ring related data structures.
 ************************************************************************/
static void
ixgbe_free_transmit_buffers(struct tx_ring *txr)
{
	struct adapter      *adapter = txr->adapter;
	struct ixgbe_tx_buf *tx_buffer;
	int                 i;

	INIT_DEBUGOUT("ixgbe_free_transmit_buffers: begin");

	if (txr->tx_buffers == NULL)
		return;

	tx_buffer = txr->tx_buffers;
	for (i = 0; i < adapter->num_tx_desc; i++, tx_buffer++) {
		if (tx_buffer->m_head != NULL) {
			bus_dmamap_sync(txr->txtag->dt_dmat, tx_buffer->map,
			    0, tx_buffer->m_head->m_pkthdr.len,
			    BUS_DMASYNC_POSTWRITE);
			ixgbe_dmamap_unload(txr->txtag, tx_buffer->map);
			m_freem(tx_buffer->m_head);
			tx_buffer->m_head = NULL;
			if (tx_buffer->map != NULL) {
				ixgbe_dmamap_destroy(txr->txtag,
				    tx_buffer->map);
				tx_buffer->map = NULL;
			}
		} else if (tx_buffer->map != NULL) {
			ixgbe_dmamap_unload(txr->txtag, tx_buffer->map);
			ixgbe_dmamap_destroy(txr->txtag, tx_buffer->map);
			tx_buffer->map = NULL;
		}
	}
	if (txr->txr_interq != NULL) {
		struct mbuf *m;

		while ((m = pcq_get(txr->txr_interq)) != NULL)
			m_freem(m);
		pcq_destroy(txr->txr_interq);
	}
	if (txr->tx_buffers != NULL) {
		free(txr->tx_buffers, M_DEVBUF);
		txr->tx_buffers = NULL;
	}
	if (txr->txtag != NULL) {
		ixgbe_dma_tag_destroy(txr->txtag);
		txr->txtag = NULL;
	}
} /* ixgbe_free_transmit_buffers */

/************************************************************************
 * ixgbe_tx_ctx_setup
 *
 *   Advanced Context Descriptor setup for VLAN, CSUM or TSO
 ************************************************************************/
static int
ixgbe_tx_ctx_setup(struct tx_ring *txr, struct mbuf *mp,
    u32 *cmd_type_len, u32 *olinfo_status)
{
	struct adapter                   *adapter = txr->adapter;
	struct ixgbe_adv_tx_context_desc *TXD;
	struct ether_vlan_header         *eh;
#ifdef INET
	struct ip                        *ip;
#endif
#ifdef INET6
	struct ip6_hdr                   *ip6;
#endif
	int                              ehdrlen, ip_hlen = 0;
	int                              offload = TRUE;
	int                              ctxd = txr->next_avail_desc;
	u32                              vlan_macip_lens = 0;
	u32                              type_tucmd_mlhl = 0;
	u16                              vtag = 0;
	u16                              etype;
	u8                               ipproto = 0;
	char                             *l3d;


	/* First check if TSO is to be used */
	if (mp->m_pkthdr.csum_flags & (M_CSUM_TSOv4 | M_CSUM_TSOv6)) {
		int rv = ixgbe_tso_setup(txr, mp, cmd_type_len, olinfo_status);

		if (rv != 0)
			IXGBE_EVC_ADD(&adapter->tso_err, 1);
		return rv;
	}

	if ((mp->m_pkthdr.csum_flags & M_CSUM_OFFLOAD) == 0)
		offload = FALSE;

	/* Indicate the whole packet as payload when not doing TSO */
	*olinfo_status |= mp->m_pkthdr.len << IXGBE_ADVTXD_PAYLEN_SHIFT;

	/* Now ready a context descriptor */
	TXD = (struct ixgbe_adv_tx_context_desc *)&txr->tx_base[ctxd];

	/*
	 * In advanced descriptors the vlan tag must
	 * be placed into the context descriptor. Hence
	 * we need to make one even if not doing offloads.
	 */
	if (vlan_has_tag(mp)) {
		vtag = htole16(vlan_get_tag(mp));
		vlan_macip_lens |= (vtag << IXGBE_ADVTXD_VLAN_SHIFT);
	} else if (!(txr->adapter->feat_en & IXGBE_FEATURE_NEEDS_CTXD) &&
	           (offload == FALSE))
		return (0);

	/*
	 * Determine where frame payload starts.
	 * Jump over vlan headers if already present,
	 * helpful for QinQ too.
	 */
	KASSERT(mp->m_len >= offsetof(struct ether_vlan_header, evl_tag));
	eh = mtod(mp, struct ether_vlan_header *);
	if (eh->evl_encap_proto == htons(ETHERTYPE_VLAN)) {
		KASSERT(mp->m_len >= sizeof(struct ether_vlan_header));
		etype = ntohs(eh->evl_proto);
		ehdrlen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
	} else {
		etype = ntohs(eh->evl_encap_proto);
		ehdrlen = ETHER_HDR_LEN;
	}

	/* Set the ether header length */
	vlan_macip_lens |= ehdrlen << IXGBE_ADVTXD_MACLEN_SHIFT;

	if (offload == FALSE)
		goto no_offloads;

	/*
	 * If the first mbuf only includes the ethernet header,
	 * jump to the next one
	 * XXX: This assumes the stack splits mbufs containing headers
	 *      on header boundaries
	 * XXX: And assumes the entire IP header is contained in one mbuf
	 */
	if (mp->m_len == ehdrlen && mp->m_next)
		l3d = mtod(mp->m_next, char *);
	else
		l3d = mtod(mp, char *) + ehdrlen;

	switch (etype) {
#ifdef INET
	case ETHERTYPE_IP:
		ip = (struct ip *)(l3d);
		ip_hlen = ip->ip_hl << 2;
		ipproto = ip->ip_p;
		type_tucmd_mlhl |= IXGBE_ADVTXD_TUCMD_IPV4;
		KASSERT((mp->m_pkthdr.csum_flags & M_CSUM_IPv4) == 0 ||
		    ip->ip_sum == 0);
		break;
#endif
#ifdef INET6
	case ETHERTYPE_IPV6:
		ip6 = (struct ip6_hdr *)(l3d);
		ip_hlen = sizeof(struct ip6_hdr);
		ipproto = ip6->ip6_nxt;
		type_tucmd_mlhl |= IXGBE_ADVTXD_TUCMD_IPV6;
		break;
#endif
	default:
		offload = false;
		break;
	}

	if ((mp->m_pkthdr.csum_flags & M_CSUM_IPv4) != 0)
		*olinfo_status |= IXGBE_TXD_POPTS_IXSM << 8;

	vlan_macip_lens |= ip_hlen;

	/* No support for offloads for non-L4 next headers */
	switch (ipproto) {
	case IPPROTO_TCP:
		if (mp->m_pkthdr.csum_flags &
		    (M_CSUM_TCPv4 | M_CSUM_TCPv6))
			type_tucmd_mlhl |= IXGBE_ADVTXD_TUCMD_L4T_TCP;
		else
			offload = false;
		break;
	case IPPROTO_UDP:
		if (mp->m_pkthdr.csum_flags &
		    (M_CSUM_UDPv4 | M_CSUM_UDPv6))
			type_tucmd_mlhl |= IXGBE_ADVTXD_TUCMD_L4T_UDP;
		else
			offload = false;
		break;
	default:
		offload = false;
		break;
	}

	if (offload) /* Insert L4 checksum into data descriptors */
		*olinfo_status |= IXGBE_TXD_POPTS_TXSM << 8;

no_offloads:
	type_tucmd_mlhl |= IXGBE_ADVTXD_DCMD_DEXT | IXGBE_ADVTXD_DTYP_CTXT;

	/* Now copy bits into descriptor */
	TXD->vlan_macip_lens = htole32(vlan_macip_lens);
	TXD->type_tucmd_mlhl = htole32(type_tucmd_mlhl);
	TXD->seqnum_seed = htole32(0);
	TXD->mss_l4len_idx = htole32(0);

	/* We've consumed the first desc, adjust counters */
	if (++ctxd == txr->num_desc)
		ctxd = 0;
	txr->next_avail_desc = ctxd;
	--txr->tx_avail;

	return (0);
} /* ixgbe_tx_ctx_setup */

/************************************************************************
 * ixgbe_tso_setup
 *
 *   Setup work for hardware segmentation offload (TSO) on
 *   adapters using advanced tx descriptors
 ************************************************************************/
static int
ixgbe_tso_setup(struct tx_ring *txr, struct mbuf *mp, u32 *cmd_type_len,
    u32 *olinfo_status)
{
	struct ixgbe_adv_tx_context_desc *TXD;
	struct ether_vlan_header         *eh;
#ifdef INET6
	struct ip6_hdr                   *ip6;
#endif
#ifdef INET
	struct ip                        *ip;
#endif
	struct tcphdr                    *th;
	int                              ctxd, ehdrlen, ip_hlen, tcp_hlen;
	u32                              vlan_macip_lens = 0;
	u32                              type_tucmd_mlhl = 0;
	u32                              mss_l4len_idx = 0, paylen;
	u16                              vtag = 0, eh_type;

	/*
	 * Determine where frame payload starts.
	 * Jump over vlan headers if already present
	 */
	eh = mtod(mp, struct ether_vlan_header *);
	if (eh->evl_encap_proto == htons(ETHERTYPE_VLAN)) {
		ehdrlen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
		eh_type = eh->evl_proto;
	} else {
		ehdrlen = ETHER_HDR_LEN;
		eh_type = eh->evl_encap_proto;
	}

	switch (ntohs(eh_type)) {
#ifdef INET
	case ETHERTYPE_IP:
		ip = (struct ip *)(mp->m_data + ehdrlen);
		if (ip->ip_p != IPPROTO_TCP)
			return (ENXIO);
		ip->ip_sum = 0;
		ip_hlen = ip->ip_hl << 2;
		th = (struct tcphdr *)((char *)ip + ip_hlen);
		th->th_sum = in_cksum_phdr(ip->ip_src.s_addr,
		    ip->ip_dst.s_addr, htons(IPPROTO_TCP));
		type_tucmd_mlhl |= IXGBE_ADVTXD_TUCMD_IPV4;
		/* Tell transmit desc to also do IPv4 checksum. */
		*olinfo_status |= IXGBE_TXD_POPTS_IXSM << 8;
		break;
#endif
#ifdef INET6
	case ETHERTYPE_IPV6:
		ip6 = (struct ip6_hdr *)(mp->m_data + ehdrlen);
		/* XXX-BZ For now we do not pretend to support ext. hdrs. */
		if (ip6->ip6_nxt != IPPROTO_TCP)
			return (ENXIO);
		ip_hlen = sizeof(struct ip6_hdr);
		ip6 = (struct ip6_hdr *)(mp->m_data + ehdrlen);
		th = (struct tcphdr *)((char *)ip6 + ip_hlen);
		th->th_sum = in6_cksum_phdr(&ip6->ip6_src,
		    &ip6->ip6_dst, 0, htonl(IPPROTO_TCP));
		type_tucmd_mlhl |= IXGBE_ADVTXD_TUCMD_IPV6;
		break;
#endif
	default:
		panic("%s: CSUM_TSO but no supported IP version (0x%04x)",
		    __func__, ntohs(eh_type));
		break;
	}

	ctxd = txr->next_avail_desc;
	TXD = (struct ixgbe_adv_tx_context_desc *)&txr->tx_base[ctxd];

	tcp_hlen = th->th_off << 2;

	/* This is used in the transmit desc in encap */
	paylen = mp->m_pkthdr.len - ehdrlen - ip_hlen - tcp_hlen;

	/* VLAN MACLEN IPLEN */
	if (vlan_has_tag(mp)) {
		vtag = htole16(vlan_get_tag(mp));
		vlan_macip_lens |= (vtag << IXGBE_ADVTXD_VLAN_SHIFT);
	}

	vlan_macip_lens |= ehdrlen << IXGBE_ADVTXD_MACLEN_SHIFT;
	vlan_macip_lens |= ip_hlen;
	TXD->vlan_macip_lens = htole32(vlan_macip_lens);

	/* ADV DTYPE TUCMD */
	type_tucmd_mlhl |= IXGBE_ADVTXD_DCMD_DEXT | IXGBE_ADVTXD_DTYP_CTXT;
	type_tucmd_mlhl |= IXGBE_ADVTXD_TUCMD_L4T_TCP;
	TXD->type_tucmd_mlhl = htole32(type_tucmd_mlhl);

	/* MSS L4LEN IDX */
	mss_l4len_idx |= (mp->m_pkthdr.segsz << IXGBE_ADVTXD_MSS_SHIFT);
	mss_l4len_idx |= (tcp_hlen << IXGBE_ADVTXD_L4LEN_SHIFT);
	TXD->mss_l4len_idx = htole32(mss_l4len_idx);

	TXD->seqnum_seed = htole32(0);

	if (++ctxd == txr->num_desc)
		ctxd = 0;

	txr->tx_avail--;
	txr->next_avail_desc = ctxd;
	*cmd_type_len |= IXGBE_ADVTXD_DCMD_TSE;
	*olinfo_status |= IXGBE_TXD_POPTS_TXSM << 8;
	*olinfo_status |= paylen << IXGBE_ADVTXD_PAYLEN_SHIFT;
	IXGBE_EVC_ADD(&txr->tso_tx, 1);

	return (0);
} /* ixgbe_tso_setup */


/************************************************************************
 * ixgbe_txeof
 *
 *   Examine each tx_buffer in the used queue. If the hardware is done
 *   processing the packet then free associated resources. The
 *   tx_buffer is put back on the free queue.
 ************************************************************************/
bool
ixgbe_txeof(struct tx_ring *txr)
{
	struct adapter		*adapter = txr->adapter;
	struct ifnet		*ifp = adapter->ifp;
	struct ixgbe_tx_buf	*buf;
	union ixgbe_adv_tx_desc *txd;
	u32			work, processed = 0;
	u32			limit = adapter->tx_process_limit;

	KASSERT(mutex_owned(&txr->tx_mtx));

#ifdef DEV_NETMAP
	if ((adapter->feat_en & IXGBE_FEATURE_NETMAP) &&
	    (adapter->ifp->if_capenable & IFCAP_NETMAP)) {
		struct netmap_adapter *na = NA(adapter->ifp);
		struct netmap_kring *kring = na->tx_rings[txr->me];
		txd = txr->tx_base;
		bus_dmamap_sync(txr->txdma.dma_tag, txr->txdma.dma_map,
		    BUS_DMASYNC_POSTREAD);
		/*
		 * In netmap mode, all the work is done in the context
		 * of the client thread. Interrupt handlers only wake up
		 * clients, which may be sleeping on individual rings
		 * or on a global resource for all rings.
		 * To implement tx interrupt mitigation, we wake up the client
		 * thread roughly every half ring, even if the NIC interrupts
		 * more frequently. This is implemented as follows:
		 * - ixgbe_txsync() sets kring->nr_kflags with the index of
		 *   the slot that should wake up the thread (nkr_num_slots
		 *   means the user thread should not be woken up);
		 * - the driver ignores tx interrupts unless netmap_mitigate=0
		 *   or the slot has the DD bit set.
		 */
		if (kring->nr_kflags < kring->nkr_num_slots &&
		    le32toh(txd[kring->nr_kflags].wb.status) & IXGBE_TXD_STAT_DD) {
			netmap_tx_irq(ifp, txr->me);
		}
		return false;
	}
#endif /* DEV_NETMAP */

	if (txr->tx_avail == txr->num_desc) {
		txr->busy = 0;
		return false;
	}

	/* Get work starting point */
	work = txr->next_to_clean;
	buf = &txr->tx_buffers[work];
	txd = &txr->tx_base[work];
	work -= txr->num_desc; /* The distance to ring end */
	ixgbe_dmamap_sync(txr->txdma.dma_tag, txr->txdma.dma_map,
	    BUS_DMASYNC_POSTREAD);

	do {
		union ixgbe_adv_tx_desc *eop = buf->eop;
		if (eop == NULL) /* No work */
			break;

		if ((le32toh(eop->wb.status) & IXGBE_TXD_STAT_DD) == 0)
			break;	/* I/O not complete */

		if (buf->m_head) {
			txr->bytes += buf->m_head->m_pkthdr.len;
			bus_dmamap_sync(txr->txtag->dt_dmat, buf->map,
			    0, buf->m_head->m_pkthdr.len,
			    BUS_DMASYNC_POSTWRITE);
			ixgbe_dmamap_unload(txr->txtag, buf->map);
			m_freem(buf->m_head);
			buf->m_head = NULL;
		}
		buf->eop = NULL;
		txr->txr_no_space = false;
		++txr->tx_avail;

		/* We clean the range if multi segment */
		while (txd != eop) {
			++txd;
			++buf;
			++work;
			/* wrap the ring? */
			if (__predict_false(!work)) {
				work -= txr->num_desc;
				buf = txr->tx_buffers;
				txd = txr->tx_base;
			}
			if (buf->m_head) {
				txr->bytes +=
				    buf->m_head->m_pkthdr.len;
				bus_dmamap_sync(txr->txtag->dt_dmat,
				    buf->map,
				    0, buf->m_head->m_pkthdr.len,
				    BUS_DMASYNC_POSTWRITE);
				ixgbe_dmamap_unload(txr->txtag,
				    buf->map);
				m_freem(buf->m_head);
				buf->m_head = NULL;
			}
			++txr->tx_avail;
			buf->eop = NULL;

		}
		++txr->packets;
		++processed;
		if_statinc(ifp, if_opackets);

		/* Try the next packet */
		++txd;
		++buf;
		++work;
		/* reset with a wrap */
		if (__predict_false(!work)) {
			work -= txr->num_desc;
			buf = txr->tx_buffers;
			txd = txr->tx_base;
		}
		prefetch(txd);
	} while (__predict_true(--limit));

	ixgbe_dmamap_sync(txr->txdma.dma_tag, txr->txdma.dma_map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	work += txr->num_desc;
	txr->next_to_clean = work;

	/*
	 * Queue Hang detection, we know there's
	 * work outstanding or the first return
	 * would have been taken, so increment busy
	 * if nothing managed to get cleaned, then
	 * in local_timer it will be checked and
	 * marked as HUNG if it exceeds a MAX attempt.
	 */
	if ((processed == 0) && (txr->busy != IXGBE_QUEUE_HUNG))
		++txr->busy;
	/*
	 * If anything gets cleaned we reset state to 1,
	 * note this will turn off HUNG if its set.
	 */
	if (processed)
		txr->busy = 1;

	if (txr->tx_avail == txr->num_desc)
		txr->busy = 0;

	return ((limit > 0) ? false : true);
} /* ixgbe_txeof */

/************************************************************************
 * ixgbe_rsc_count
 *
 *   Used to detect a descriptor that has been merged by Hardware RSC.
 ************************************************************************/
static inline u32
ixgbe_rsc_count(union ixgbe_adv_rx_desc *rx)
{
	return (le32toh(rx->wb.lower.lo_dword.data) &
	    IXGBE_RXDADV_RSCCNT_MASK) >> IXGBE_RXDADV_RSCCNT_SHIFT;
} /* ixgbe_rsc_count */

/************************************************************************
 * ixgbe_setup_hw_rsc
 *
 *   Initialize Hardware RSC (LRO) feature on 82599
 *   for an RX ring, this is toggled by the LRO capability
 *   even though it is transparent to the stack.
 *
 *   NOTE: Since this HW feature only works with IPv4 and
 *         testing has shown soft LRO to be as effective,
 *         this feature will be disabled by default.
 ************************************************************************/
static void
ixgbe_setup_hw_rsc(struct rx_ring *rxr)
{
	struct	adapter  *adapter = rxr->adapter;
	struct	ixgbe_hw *hw = &adapter->hw;
	u32              rscctrl, rdrxctl;

	/* If turning LRO/RSC off we need to disable it */
	if ((adapter->ifp->if_capenable & IFCAP_LRO) == 0) {
		rscctrl = IXGBE_READ_REG(hw, IXGBE_RSCCTL(rxr->me));
		rscctrl &= ~IXGBE_RSCCTL_RSCEN;
		return;
	}

	rdrxctl = IXGBE_READ_REG(hw, IXGBE_RDRXCTL);
	rdrxctl &= ~IXGBE_RDRXCTL_RSCFRSTSIZE;
#ifdef DEV_NETMAP
	/* Always strip CRC unless Netmap disabled it */
	if (!(adapter->feat_en & IXGBE_FEATURE_NETMAP) ||
	    !(adapter->ifp->if_capenable & IFCAP_NETMAP) ||
	    ix_crcstrip)
#endif /* DEV_NETMAP */
		rdrxctl |= IXGBE_RDRXCTL_CRCSTRIP;
	rdrxctl |= IXGBE_RDRXCTL_RSCACKC;
	IXGBE_WRITE_REG(hw, IXGBE_RDRXCTL, rdrxctl);

	rscctrl = IXGBE_READ_REG(hw, IXGBE_RSCCTL(rxr->me));
	rscctrl |= IXGBE_RSCCTL_RSCEN;
	/*
	 * Limit the total number of descriptors that
	 * can be combined, so it does not exceed 64K
	 */
	if (rxr->mbuf_sz == MCLBYTES)
		rscctrl |= IXGBE_RSCCTL_MAXDESC_16;
	else if (rxr->mbuf_sz == MJUMPAGESIZE)
		rscctrl |= IXGBE_RSCCTL_MAXDESC_8;
	else if (rxr->mbuf_sz == MJUM9BYTES)
		rscctrl |= IXGBE_RSCCTL_MAXDESC_4;
	else  /* Using 16K cluster */
		rscctrl |= IXGBE_RSCCTL_MAXDESC_1;

	IXGBE_WRITE_REG(hw, IXGBE_RSCCTL(rxr->me), rscctrl);

	/* Enable TCP header recognition */
	IXGBE_WRITE_REG(hw, IXGBE_PSRTYPE(0),
	    (IXGBE_READ_REG(hw, IXGBE_PSRTYPE(0)) | IXGBE_PSRTYPE_TCPHDR));

	/* Disable RSC for ACK packets */
	IXGBE_WRITE_REG(hw, IXGBE_RSCDBU,
	    (IXGBE_RSCDBU_RSCACKDIS | IXGBE_READ_REG(hw, IXGBE_RSCDBU)));

	rxr->hw_rsc = TRUE;
} /* ixgbe_setup_hw_rsc */

/************************************************************************
 * ixgbe_refresh_mbufs
 *
 *   Refresh mbuf buffers for RX descriptor rings
 *    - now keeps its own state so discards due to resource
 *      exhaustion are unnecessary, if an mbuf cannot be obtained
 *      it just returns, keeping its placeholder, thus it can simply
 *      be recalled to try again.
 *
 *   XXX NetBSD TODO:
 *    - The ixgbe_rxeof() function always preallocates mbuf cluster,
 *      so the ixgbe_refresh_mbufs() function can be simplified.
 *
 ************************************************************************/
static void
ixgbe_refresh_mbufs(struct rx_ring *rxr, int limit)
{
	struct adapter      *adapter = rxr->adapter;
	struct ixgbe_rx_buf *rxbuf;
	struct mbuf         *mp;
	int                 i, error;
	bool                refreshed = false;

	i = rxr->next_to_refresh;
	/* next_to_refresh points to the previous one */
	if (++i == rxr->num_desc)
		i = 0;

	while (i != limit) {
		rxbuf = &rxr->rx_buffers[i];
		if (__predict_false(rxbuf->buf == NULL)) {
			mp = ixgbe_getcl();
			if (mp == NULL) {
				IXGBE_EVC_ADD(&rxr->no_mbuf, 1);
				goto update;
			}
			mp->m_pkthdr.len = mp->m_len = rxr->mbuf_sz;
			IXGBE_M_ADJ(adapter, rxr, mp);
		} else
			mp = rxbuf->buf;

		/* If we're dealing with an mbuf that was copied rather
		 * than replaced, there's no need to go through busdma.
		 */
		if ((rxbuf->flags & IXGBE_RX_COPY) == 0) {
			/* Get the memory mapping */
			ixgbe_dmamap_unload(rxr->ptag, rxbuf->pmap);
			error = bus_dmamap_load_mbuf(rxr->ptag->dt_dmat,
			    rxbuf->pmap, mp, BUS_DMA_NOWAIT);
			if (__predict_false(error != 0)) {
				device_printf(adapter->dev, "Refresh mbufs: "
				    "payload dmamap load failure - %d\n",
				    error);
				m_free(mp);
				rxbuf->buf = NULL;
				goto update;
			}
			rxbuf->buf = mp;
			bus_dmamap_sync(rxr->ptag->dt_dmat, rxbuf->pmap,
			    0, mp->m_pkthdr.len, BUS_DMASYNC_PREREAD);
			rxbuf->addr = rxr->rx_base[i].read.pkt_addr =
			    htole64(rxbuf->pmap->dm_segs[0].ds_addr);
		} else {
			rxr->rx_base[i].read.pkt_addr = rxbuf->addr;
			rxbuf->flags &= ~IXGBE_RX_COPY;
		}

		refreshed = true;
		/* next_to_refresh points to the previous one */
		rxr->next_to_refresh = i;
		if (++i == rxr->num_desc)
			i = 0;
	}

update:
	if (refreshed) /* Update hardware tail index */
		IXGBE_WRITE_REG(&adapter->hw, rxr->tail, rxr->next_to_refresh);

	return;
} /* ixgbe_refresh_mbufs */

/************************************************************************
 * ixgbe_allocate_receive_buffers
 *
 *   Allocate memory for rx_buffer structures. Since we use one
 *   rx_buffer per received packet, the maximum number of rx_buffer's
 *   that we'll need is equal to the number of receive descriptors
 *   that we've allocated.
 ************************************************************************/
static int
ixgbe_allocate_receive_buffers(struct rx_ring *rxr)
{
	struct adapter      *adapter = rxr->adapter;
	device_t            dev = adapter->dev;
	struct ixgbe_rx_buf *rxbuf;
	int                 bsize, error;

	bsize = sizeof(struct ixgbe_rx_buf) * rxr->num_desc;
	rxr->rx_buffers = malloc(bsize, M_DEVBUF, M_WAITOK | M_ZERO);

	error = ixgbe_dma_tag_create(
	         /*      parent */ adapter->osdep.dmat,
	         /*   alignment */ 1,
	         /*      bounds */ 0,
	         /*     maxsize */ MJUM16BYTES,
	         /*   nsegments */ 1,
	         /*  maxsegsize */ MJUM16BYTES,
	         /*       flags */ 0,
	                           &rxr->ptag);
	if (error != 0) {
		aprint_error_dev(dev, "Unable to create RX DMA tag\n");
		goto fail;
	}

	for (int i = 0; i < rxr->num_desc; i++, rxbuf++) {
		rxbuf = &rxr->rx_buffers[i];
		error = ixgbe_dmamap_create(rxr->ptag, 0, &rxbuf->pmap);
		if (error) {
			aprint_error_dev(dev, "Unable to create RX dma map\n");
			goto fail;
		}
	}

	return (0);

fail:
	/* Frees all, but can handle partial completion */
	ixgbe_free_receive_structures(adapter);

	return (error);
} /* ixgbe_allocate_receive_buffers */

/************************************************************************
 * ixgbe_free_receive_ring
 ************************************************************************/
static void
ixgbe_free_receive_ring(struct rx_ring *rxr)
{
	for (int i = 0; i < rxr->num_desc; i++) {
		ixgbe_rx_discard(rxr, i);
	}
} /* ixgbe_free_receive_ring */

/************************************************************************
 * ixgbe_setup_receive_ring
 *
 *   Initialize a receive ring and its buffers.
 ************************************************************************/
static int
ixgbe_setup_receive_ring(struct rx_ring *rxr)
{
	struct adapter        *adapter;
	struct ixgbe_rx_buf   *rxbuf;
#ifdef LRO
	struct ifnet          *ifp;
	struct lro_ctrl       *lro = &rxr->lro;
#endif /* LRO */
#ifdef DEV_NETMAP
	struct netmap_adapter *na = NA(rxr->adapter->ifp);
	struct netmap_slot    *slot;
#endif /* DEV_NETMAP */
	int                   rsize, error = 0;

	adapter = rxr->adapter;
#ifdef LRO
	ifp = adapter->ifp;
#endif /* LRO */

	/* Clear the ring contents */
	IXGBE_RX_LOCK(rxr);

#ifdef DEV_NETMAP
	if (adapter->feat_en & IXGBE_FEATURE_NETMAP)
		slot = netmap_reset(na, NR_RX, rxr->me, 0);
#endif /* DEV_NETMAP */

	rsize = roundup2(adapter->num_rx_desc *
	    sizeof(union ixgbe_adv_rx_desc), DBA_ALIGN);
	bzero((void *)rxr->rx_base, rsize);
	/* Cache the size */
	rxr->mbuf_sz = adapter->rx_mbuf_sz;

	/* Free current RX buffer structs and their mbufs */
	ixgbe_free_receive_ring(rxr);

	/* Now replenish the mbufs */
	for (int j = 0; j != rxr->num_desc; ++j) {
		struct mbuf *mp;

		rxbuf = &rxr->rx_buffers[j];

#ifdef DEV_NETMAP
		/*
		 * In netmap mode, fill the map and set the buffer
		 * address in the NIC ring, considering the offset
		 * between the netmap and NIC rings (see comment in
		 * ixgbe_setup_transmit_ring() ). No need to allocate
		 * an mbuf, so end the block with a continue;
		 */
		if ((adapter->feat_en & IXGBE_FEATURE_NETMAP) && slot) {
			int sj = netmap_idx_n2k(na->rx_rings[rxr->me], j);
			uint64_t paddr;
			void *addr;

			addr = PNMB(na, slot + sj, &paddr);
			netmap_load_map(na, rxr->ptag, rxbuf->pmap, addr);
			/* Update descriptor and the cached value */
			rxr->rx_base[j].read.pkt_addr = htole64(paddr);
			rxbuf->addr = htole64(paddr);
			continue;
		}
#endif /* DEV_NETMAP */

		rxbuf->flags = 0;
		rxbuf->buf = ixgbe_getcl();
		if (rxbuf->buf == NULL) {
			IXGBE_EVC_ADD(&rxr->no_mbuf, 1);
			error = ENOBUFS;
			goto fail;
		}
		mp = rxbuf->buf;
		mp->m_pkthdr.len = mp->m_len = rxr->mbuf_sz;
		IXGBE_M_ADJ(adapter, rxr, mp);
		/* Get the memory mapping */
		error = bus_dmamap_load_mbuf(rxr->ptag->dt_dmat, rxbuf->pmap,
		    mp, BUS_DMA_NOWAIT);
		if (error != 0) {
			/*
			 * Clear this entry for later cleanup in
			 * ixgbe_discard() which is called via
			 * ixgbe_free_receive_ring().
			 */
			m_freem(mp);
			rxbuf->buf = NULL;
			goto fail;
		}
		bus_dmamap_sync(rxr->ptag->dt_dmat, rxbuf->pmap,
		    0, mp->m_pkthdr.len, BUS_DMASYNC_PREREAD);
		/* Update the descriptor and the cached value */
		rxr->rx_base[j].read.pkt_addr =
		    htole64(rxbuf->pmap->dm_segs[0].ds_addr);
		rxbuf->addr = htole64(rxbuf->pmap->dm_segs[0].ds_addr);
	}

	/* Setup our descriptor indices */
	rxr->next_to_check = 0;
	rxr->next_to_refresh = adapter->num_rx_desc - 1; /* Fully allocated */
	rxr->lro_enabled = FALSE;
	rxr->discard_multidesc = false;
	IXGBE_EVC_STORE(&rxr->rx_copies, 0);
#if 0 /* NetBSD */
	IXGBE_EVC_STORE(&rxr->rx_bytes, 0);
#if 1	/* Fix inconsistency */
	IXGBE_EVC_STORE(&rxr->rx_packets, 0);
#endif
#endif
	rxr->vtag_strip = FALSE;

	ixgbe_dmamap_sync(rxr->rxdma.dma_tag, rxr->rxdma.dma_map,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	/*
	 * Now set up the LRO interface
	 */
	if (ixgbe_rsc_enable)
		ixgbe_setup_hw_rsc(rxr);
#ifdef LRO
	else if (ifp->if_capenable & IFCAP_LRO) {
		device_t dev = adapter->dev;
		int err = tcp_lro_init(lro);
		if (err) {
			device_printf(dev, "LRO Initialization failed!\n");
			goto fail;
		}
		INIT_DEBUGOUT("RX Soft LRO Initialized\n");
		rxr->lro_enabled = TRUE;
		lro->ifp = adapter->ifp;
	}
#endif /* LRO */

	IXGBE_RX_UNLOCK(rxr);

	return (0);

fail:
	ixgbe_free_receive_ring(rxr);
	IXGBE_RX_UNLOCK(rxr);

	return (error);
} /* ixgbe_setup_receive_ring */

/************************************************************************
 * ixgbe_setup_receive_structures - Initialize all receive rings.
 ************************************************************************/
int
ixgbe_setup_receive_structures(struct adapter *adapter)
{
	struct rx_ring *rxr = adapter->rx_rings;
	int            j;

	INIT_DEBUGOUT("ixgbe_setup_receive_structures");
	for (j = 0; j < adapter->num_queues; j++, rxr++)
		if (ixgbe_setup_receive_ring(rxr))
			goto fail;

	return (0);
fail:
	/*
	 * Free RX buffers allocated so far, we will only handle
	 * the rings that completed, the failing case will have
	 * cleaned up for itself. 'j' failed, so its the terminus.
	 */
	for (int i = 0; i < j; ++i) {
		rxr = &adapter->rx_rings[i];
		IXGBE_RX_LOCK(rxr);
		ixgbe_free_receive_ring(rxr);
		IXGBE_RX_UNLOCK(rxr);
	}

	return (ENOBUFS);
} /* ixgbe_setup_receive_structures */


/************************************************************************
 * ixgbe_free_receive_structures - Free all receive rings.
 ************************************************************************/
void
ixgbe_free_receive_structures(struct adapter *adapter)
{
	struct rx_ring *rxr = adapter->rx_rings;

	INIT_DEBUGOUT("ixgbe_free_receive_structures: begin");

	for (int i = 0; i < adapter->num_queues; i++, rxr++) {
		ixgbe_free_receive_buffers(rxr);
#ifdef LRO
		/* Free LRO memory */
		tcp_lro_free(&rxr->lro);
#endif /* LRO */
		/* Free the ring memory as well */
		ixgbe_dma_free(adapter, &rxr->rxdma);
		IXGBE_RX_LOCK_DESTROY(rxr);
	}

	free(adapter->rx_rings, M_DEVBUF);
} /* ixgbe_free_receive_structures */


/************************************************************************
 * ixgbe_free_receive_buffers - Free receive ring data structures
 ************************************************************************/
static void
ixgbe_free_receive_buffers(struct rx_ring *rxr)
{
	struct adapter      *adapter = rxr->adapter;
	struct ixgbe_rx_buf *rxbuf;

	INIT_DEBUGOUT("ixgbe_free_receive_buffers: begin");

	/* Cleanup any existing buffers */
	if (rxr->rx_buffers != NULL) {
		for (int i = 0; i < adapter->num_rx_desc; i++) {
			rxbuf = &rxr->rx_buffers[i];
			ixgbe_rx_discard(rxr, i);
			if (rxbuf->pmap != NULL) {
				ixgbe_dmamap_destroy(rxr->ptag, rxbuf->pmap);
				rxbuf->pmap = NULL;
			}
		}

		if (rxr->rx_buffers != NULL) {
			free(rxr->rx_buffers, M_DEVBUF);
			rxr->rx_buffers = NULL;
		}
	}

	if (rxr->ptag != NULL) {
		ixgbe_dma_tag_destroy(rxr->ptag);
		rxr->ptag = NULL;
	}

	return;
} /* ixgbe_free_receive_buffers */

/************************************************************************
 * ixgbe_rx_input
 ************************************************************************/
static __inline void
ixgbe_rx_input(struct rx_ring *rxr, struct ifnet *ifp, struct mbuf *m,
    u32 ptype)
{
	struct adapter	*adapter = ifp->if_softc;

#ifdef LRO
	struct ethercom *ec = &adapter->osdep.ec;

	/*
	 * ATM LRO is only for IP/TCP packets and TCP checksum of the packet
	 * should be computed by hardware. Also it should not have VLAN tag in
	 * ethernet header.  In case of IPv6 we do not yet support ext. hdrs.
	 */
        if (rxr->lro_enabled &&
            (ec->ec_capenable & ETHERCAP_VLAN_HWTAGGING) != 0 &&
            (ptype & IXGBE_RXDADV_PKTTYPE_ETQF) == 0 &&
            ((ptype & (IXGBE_RXDADV_PKTTYPE_IPV4 | IXGBE_RXDADV_PKTTYPE_TCP)) ==
            (IXGBE_RXDADV_PKTTYPE_IPV4 | IXGBE_RXDADV_PKTTYPE_TCP) ||
            (ptype & (IXGBE_RXDADV_PKTTYPE_IPV6 | IXGBE_RXDADV_PKTTYPE_TCP)) ==
            (IXGBE_RXDADV_PKTTYPE_IPV6 | IXGBE_RXDADV_PKTTYPE_TCP)) &&
            (m->m_pkthdr.csum_flags & (CSUM_DATA_VALID | CSUM_PSEUDO_HDR)) ==
            (CSUM_DATA_VALID | CSUM_PSEUDO_HDR)) {
                /*
                 * Send to the stack if:
                 **  - LRO not enabled, or
                 **  - no LRO resources, or
                 **  - lro enqueue fails
                 */
                if (rxr->lro.lro_cnt != 0)
                        if (tcp_lro_rx(&rxr->lro, m, 0) == 0)
                                return;
        }
#endif /* LRO */

	if_percpuq_enqueue(adapter->ipq, m);
} /* ixgbe_rx_input */

/************************************************************************
 * ixgbe_rx_discard
 ************************************************************************/
static __inline void
ixgbe_rx_discard(struct rx_ring *rxr, int i)
{
	struct ixgbe_rx_buf *rbuf;

	rbuf = &rxr->rx_buffers[i];

	/*
	 * With advanced descriptors the writeback clobbers the buffer addrs,
	 * so its easier to just free the existing mbufs and take the normal
	 * refresh path to get new buffers and mapping.
	 */

	if (rbuf->fmp != NULL) {/* Partial chain ? */
		bus_dmamap_sync(rxr->ptag->dt_dmat, rbuf->pmap, 0,
		    rbuf->buf->m_pkthdr.len, BUS_DMASYNC_POSTREAD);
		ixgbe_dmamap_unload(rxr->ptag, rbuf->pmap);
		m_freem(rbuf->fmp);
		rbuf->fmp = NULL;
		rbuf->buf = NULL; /* rbuf->buf is part of fmp's chain */
	} else if (rbuf->buf) {
		bus_dmamap_sync(rxr->ptag->dt_dmat, rbuf->pmap, 0,
		    rbuf->buf->m_pkthdr.len, BUS_DMASYNC_POSTREAD);
		ixgbe_dmamap_unload(rxr->ptag, rbuf->pmap);
		m_free(rbuf->buf);
		rbuf->buf = NULL;
	}

	rbuf->flags = 0;

	return;
} /* ixgbe_rx_discard */


/************************************************************************
 * ixgbe_rxeof
 *
 *   Executes in interrupt context. It replenishes the
 *   mbufs in the descriptor and sends data which has
 *   been dma'ed into host memory to upper layer.
 *
 *   Return TRUE for more work, FALSE for all clean.
 ************************************************************************/
bool
ixgbe_rxeof(struct ix_queue *que)
{
	struct adapter		*adapter = que->adapter;
	struct rx_ring		*rxr = que->rxr;
	struct ifnet		*ifp = adapter->ifp;
#ifdef LRO
	struct lro_ctrl		*lro = &rxr->lro;
#endif /* LRO */
	union ixgbe_adv_rx_desc	*cur;
	struct ixgbe_rx_buf	*rbuf, *nbuf;
	int			i, nextp, processed = 0;
	u32			staterr = 0;
	u32			loopcount = 0, numdesc;
	u32			limit = adapter->rx_process_limit;
	u32			rx_copy_len = adapter->rx_copy_len;
	bool			discard_multidesc = rxr->discard_multidesc;
	bool			wraparound = false;
	unsigned int		syncremain;
#ifdef RSS
	u16			pkt_info;
#endif

	IXGBE_RX_LOCK(rxr);

#ifdef DEV_NETMAP
	if (adapter->feat_en & IXGBE_FEATURE_NETMAP) {
		/* Same as the txeof routine: wakeup clients on intr. */
		if (netmap_rx_irq(ifp, rxr->me, &processed)) {
			IXGBE_RX_UNLOCK(rxr);
			return (FALSE);
		}
	}
#endif /* DEV_NETMAP */

	/* Sync the ring. The size is rx_process_limit or the first half */
	if ((rxr->next_to_check + limit) <= rxr->num_desc) {
		/* Non-wraparound */
		numdesc = limit;
		syncremain = 0;
	} else {
		/* Wraparound. Sync the first half. */
		numdesc = rxr->num_desc - rxr->next_to_check;

		/* Set the size of the last half */
		syncremain = limit - numdesc;
	}
	bus_dmamap_sync(rxr->rxdma.dma_tag->dt_dmat,
	    rxr->rxdma.dma_map,
	    sizeof(union ixgbe_adv_rx_desc) * rxr->next_to_check,
	    sizeof(union ixgbe_adv_rx_desc) * numdesc,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	/*
	 * The max number of loop is rx_process_limit. If discard_multidesc is
	 * true, continue processing to not to send broken packet to the upper
	 * layer.
	 */
	for (i = rxr->next_to_check;
	     (loopcount < limit) || (discard_multidesc == true);) {

		struct mbuf *sendmp, *mp;
		struct mbuf *newmp;
		u32         rsc, ptype;
		u16         len;
		u16         vtag = 0;
		bool        eop;
		bool        discard = false;

		if (wraparound) {
			/* Sync the last half. */
			KASSERT(syncremain != 0);
			numdesc = syncremain;
			wraparound = false;
		} else if (__predict_false(loopcount >= limit)) {
			KASSERT(discard_multidesc == true);
			numdesc = 1;
		} else
			numdesc = 0;

		if (numdesc != 0)
			bus_dmamap_sync(rxr->rxdma.dma_tag->dt_dmat,
			    rxr->rxdma.dma_map, 0,
			    sizeof(union ixgbe_adv_rx_desc) * numdesc,
			    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		cur = &rxr->rx_base[i];
		staterr = le32toh(cur->wb.upper.status_error);
#ifdef RSS
		pkt_info = le16toh(cur->wb.lower.lo_dword.hs_rss.pkt_info);
#endif

		if ((staterr & IXGBE_RXD_STAT_DD) == 0)
			break;

		loopcount++;
		sendmp = newmp = NULL;
		nbuf = NULL;
		rsc = 0;
		cur->wb.upper.status_error = 0;
		rbuf = &rxr->rx_buffers[i];
		mp = rbuf->buf;

		len = le16toh(cur->wb.upper.length);
		ptype = le32toh(cur->wb.lower.lo_dword.data) &
		    IXGBE_RXDADV_PKTTYPE_MASK;
		eop = ((staterr & IXGBE_RXD_STAT_EOP) != 0);

		/* Make sure bad packets are discarded */
		if (eop && (staterr & IXGBE_RXDADV_ERR_FRAME_ERR_MASK) != 0) {
#if __FreeBSD_version >= 1100036
			if (adapter->feat_en & IXGBE_FEATURE_VF)
				if_inc_counter(ifp, IFCOUNTER_IERRORS, 1);
#endif
			IXGBE_EVC_ADD(&rxr->rx_discarded, 1);
			ixgbe_rx_discard(rxr, i);
			discard_multidesc = false;
			goto next_desc;
		}

		if (__predict_false(discard_multidesc))
			discard = true;
		else {
			/* Pre-alloc new mbuf. */

			if ((rbuf->fmp == NULL) &&
			    eop && (len <= rx_copy_len)) {
				/* For short packet. See below. */
				sendmp = m_gethdr(M_NOWAIT, MT_DATA);
				if (__predict_false(sendmp == NULL)) {
					IXGBE_EVC_ADD(&rxr->no_mbuf, 1);
					discard = true;
				}
			} else {
				/* For long packet. */
				newmp = ixgbe_getcl();
				if (__predict_false(newmp == NULL)) {
					IXGBE_EVC_ADD(&rxr->no_mbuf, 1);
					discard = true;
				}
			}
		}

		if (__predict_false(discard)) {
			/*
			 * Descriptor initialization is already done by the
			 * above code (cur->wb.upper.status_error = 0).
			 * So, we can reuse current rbuf->buf for new packet.
			 *
			 * Rewrite the buffer addr, see comment in
			 * ixgbe_rx_discard().
			 */
			cur->read.pkt_addr = rbuf->addr;
			m_freem(rbuf->fmp);
			rbuf->fmp = NULL;
			if (!eop) {
				/* Discard the entire packet. */
				discard_multidesc = true;
			} else
				discard_multidesc = false;
			goto next_desc;
		}
		discard_multidesc = false;

		bus_dmamap_sync(rxr->ptag->dt_dmat, rbuf->pmap, 0,
		    rbuf->buf->m_pkthdr.len, BUS_DMASYNC_POSTREAD);

		/*
		 * On 82599 which supports a hardware
		 * LRO (called HW RSC), packets need
		 * not be fragmented across sequential
		 * descriptors, rather the next descriptor
		 * is indicated in bits of the descriptor.
		 * This also means that we might proceses
		 * more than one packet at a time, something
		 * that has never been true before, it
		 * required eliminating global chain pointers
		 * in favor of what we are doing here.  -jfv
		 */
		if (!eop) {
			/*
			 * Figure out the next descriptor
			 * of this frame.
			 */
			if (rxr->hw_rsc == TRUE) {
				rsc = ixgbe_rsc_count(cur);
				rxr->rsc_num += (rsc - 1);
			}
			if (rsc) { /* Get hardware index */
				nextp = ((staterr & IXGBE_RXDADV_NEXTP_MASK) >>
				    IXGBE_RXDADV_NEXTP_SHIFT);
			} else { /* Just sequential */
				nextp = i + 1;
				if (nextp == adapter->num_rx_desc)
					nextp = 0;
			}
			nbuf = &rxr->rx_buffers[nextp];
			prefetch(nbuf);
		}
		/*
		 * Rather than using the fmp/lmp global pointers
		 * we now keep the head of a packet chain in the
		 * buffer struct and pass this along from one
		 * descriptor to the next, until we get EOP.
		 */
		/*
		 * See if there is a stored head
		 * that determines what we are
		 */
		if (rbuf->fmp != NULL) {
			/* Secondary frag */
			sendmp = rbuf->fmp;

			/* Update new (used in future) mbuf */
			newmp->m_pkthdr.len = newmp->m_len = rxr->mbuf_sz;
			IXGBE_M_ADJ(adapter, rxr, newmp);
			rbuf->buf = newmp;
			rbuf->fmp = NULL;

			/* For secondary frag */
			mp->m_len = len;
			mp->m_flags &= ~M_PKTHDR;

			/* For sendmp */
			sendmp->m_pkthdr.len += mp->m_len;
		} else {
			/*
			 * It's the first segment of a multi descriptor
			 * packet or a single segment which contains a full
			 * packet.
			 */

			if (eop && (len <= rx_copy_len)) {
				/*
				 * Optimize.  This might be a small packet, may
				 * be just a TCP ACK. Copy into a new mbuf, and
				 * Leave the old mbuf+cluster for re-use.
				 */
				sendmp->m_data += ETHER_ALIGN;
				memcpy(mtod(sendmp, void *),
				    mtod(mp, void *), len);
				IXGBE_EVC_ADD(&rxr->rx_copies, 1);
				rbuf->flags |= IXGBE_RX_COPY;
			} else {
				/* For long packet */

				/* Update new (used in future) mbuf */
				newmp->m_pkthdr.len = newmp->m_len
				    = rxr->mbuf_sz;
				IXGBE_M_ADJ(adapter, rxr, newmp);
				rbuf->buf = newmp;
				rbuf->fmp = NULL;

				/* For sendmp */
				sendmp = mp;
			}

			/* first desc of a non-ps chain */
			sendmp->m_pkthdr.len = sendmp->m_len = len;
		}
		++processed;

		/* Pass the head pointer on */
		if (eop == 0) {
			nbuf->fmp = sendmp;
			sendmp = NULL;
			mp->m_next = nbuf->buf;
		} else { /* Sending this frame */
			m_set_rcvif(sendmp, ifp);
			++rxr->packets;
			IXGBE_EVC_ADD(&rxr->rx_packets, 1);
			/* capture data for AIM */
			rxr->bytes += sendmp->m_pkthdr.len;
			IXGBE_EVC_ADD(&rxr->rx_bytes, sendmp->m_pkthdr.len);
			/* Process vlan info */
			if ((rxr->vtag_strip) && (staterr & IXGBE_RXD_STAT_VP))
				vtag = le16toh(cur->wb.upper.vlan);
			if (vtag) {
				vlan_set_tag(sendmp, vtag);
			}
			if ((ifp->if_capenable & IFCAP_RXCSUM) != 0) {
				ixgbe_rx_checksum(staterr, sendmp, ptype,
				   &adapter->stats.pf);
			}

#if 0 /* FreeBSD */
			/*
			 * In case of multiqueue, we have RXCSUM.PCSD bit set
			 * and never cleared. This means we have RSS hash
			 * available to be used.
			 */
			if (adapter->num_queues > 1) {
				sendmp->m_pkthdr.flowid =
				    le32toh(cur->wb.lower.hi_dword.rss);
				switch (pkt_info & IXGBE_RXDADV_RSSTYPE_MASK) {
				case IXGBE_RXDADV_RSSTYPE_IPV4:
					M_HASHTYPE_SET(sendmp,
					    M_HASHTYPE_RSS_IPV4);
					break;
				case IXGBE_RXDADV_RSSTYPE_IPV4_TCP:
					M_HASHTYPE_SET(sendmp,
					    M_HASHTYPE_RSS_TCP_IPV4);
					break;
				case IXGBE_RXDADV_RSSTYPE_IPV6:
					M_HASHTYPE_SET(sendmp,
					    M_HASHTYPE_RSS_IPV6);
					break;
				case IXGBE_RXDADV_RSSTYPE_IPV6_TCP:
					M_HASHTYPE_SET(sendmp,
					    M_HASHTYPE_RSS_TCP_IPV6);
					break;
				case IXGBE_RXDADV_RSSTYPE_IPV6_EX:
					M_HASHTYPE_SET(sendmp,
					    M_HASHTYPE_RSS_IPV6_EX);
					break;
				case IXGBE_RXDADV_RSSTYPE_IPV6_TCP_EX:
					M_HASHTYPE_SET(sendmp,
					    M_HASHTYPE_RSS_TCP_IPV6_EX);
					break;
#if __FreeBSD_version > 1100000
				case IXGBE_RXDADV_RSSTYPE_IPV4_UDP:
					M_HASHTYPE_SET(sendmp,
					    M_HASHTYPE_RSS_UDP_IPV4);
					break;
				case IXGBE_RXDADV_RSSTYPE_IPV6_UDP:
					M_HASHTYPE_SET(sendmp,
					    M_HASHTYPE_RSS_UDP_IPV6);
					break;
				case IXGBE_RXDADV_RSSTYPE_IPV6_UDP_EX:
					M_HASHTYPE_SET(sendmp,
					    M_HASHTYPE_RSS_UDP_IPV6_EX);
					break;
#endif
				default:
					M_HASHTYPE_SET(sendmp,
					    M_HASHTYPE_OPAQUE_HASH);
				}
			} else {
				sendmp->m_pkthdr.flowid = que->msix;
				M_HASHTYPE_SET(sendmp, M_HASHTYPE_OPAQUE);
			}
#endif
		}
next_desc:
		ixgbe_dmamap_sync(rxr->rxdma.dma_tag, rxr->rxdma.dma_map,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		/* Advance our pointers to the next descriptor. */
		if (++i == rxr->num_desc) {
			wraparound = true;
			i = 0;
		}
		rxr->next_to_check = i;

		/* Now send to the stack or do LRO */
		if (sendmp != NULL)
			ixgbe_rx_input(rxr, ifp, sendmp, ptype);

		/* Every 8 descriptors we go to refresh mbufs */
		if (processed == 8) {
			ixgbe_refresh_mbufs(rxr, i);
			processed = 0;
		}
	}

	/* Save the current status */
	rxr->discard_multidesc = discard_multidesc;

	/* Refresh any remaining buf structs */
	if (ixgbe_rx_unrefreshed(rxr))
		ixgbe_refresh_mbufs(rxr, i);

	IXGBE_RX_UNLOCK(rxr);

#ifdef LRO
	/*
	 * Flush any outstanding LRO work
	 */
	tcp_lro_flush_all(lro);
#endif /* LRO */

	/*
	 * Still have cleaning to do?
	 */
	if ((staterr & IXGBE_RXD_STAT_DD) != 0)
		return (TRUE);

	return (FALSE);
} /* ixgbe_rxeof */


/************************************************************************
 * ixgbe_rx_checksum
 *
 *   Verify that the hardware indicated that the checksum is valid.
 *   Inform the stack about the status of checksum so that stack
 *   doesn't spend time verifying the checksum.
 ************************************************************************/
static void
ixgbe_rx_checksum(u32 staterr, struct mbuf * mp, u32 ptype,
    struct ixgbe_hw_stats *stats)
{
	u16  status = (u16)staterr;
	u8   errors = (u8)(staterr >> 24);
#if 0
	bool sctp = false;

	if ((ptype & IXGBE_RXDADV_PKTTYPE_ETQF) == 0 &&
	    (ptype & IXGBE_RXDADV_PKTTYPE_SCTP) != 0)
		sctp = true;
#endif

	/* IPv4 checksum */
	if (status & IXGBE_RXD_STAT_IPCS) {
		IXGBE_EVC_ADD(&stats->ipcs, 1);
		if (!(errors & IXGBE_RXD_ERR_IPE)) {
			/* IP Checksum Good */
			mp->m_pkthdr.csum_flags = M_CSUM_IPv4;
		} else {
			IXGBE_EVC_ADD(&stats->ipcs_bad, 1);
			mp->m_pkthdr.csum_flags = M_CSUM_IPv4|M_CSUM_IPv4_BAD;
		}
	}
	/* TCP/UDP/SCTP checksum */
	if (status & IXGBE_RXD_STAT_L4CS) {
		IXGBE_EVC_ADD(&stats->l4cs, 1);
		int type = M_CSUM_TCPv4|M_CSUM_TCPv6|M_CSUM_UDPv4|M_CSUM_UDPv6;
		if (!(errors & IXGBE_RXD_ERR_TCPE)) {
			mp->m_pkthdr.csum_flags |= type;
		} else {
			IXGBE_EVC_ADD(&stats->l4cs_bad, 1);
			mp->m_pkthdr.csum_flags |= type | M_CSUM_TCP_UDP_BAD;
		}
	}
} /* ixgbe_rx_checksum */

/************************************************************************
 * ixgbe_dma_malloc
 ************************************************************************/
int
ixgbe_dma_malloc(struct adapter *adapter, const bus_size_t size,
		struct ixgbe_dma_alloc *dma, const int mapflags)
{
	device_t dev = adapter->dev;
	int      r, rsegs;

	r = ixgbe_dma_tag_create(
	     /*      parent */ adapter->osdep.dmat,
	     /*   alignment */ DBA_ALIGN,
	     /*      bounds */ 0,
	     /*     maxsize */ size,
	     /*   nsegments */ 1,
	     /*  maxsegsize */ size,
	     /*       flags */ BUS_DMA_ALLOCNOW,
			       &dma->dma_tag);
	if (r != 0) {
		aprint_error_dev(dev,
		    "%s: ixgbe_dma_tag_create failed; error %d\n", __func__,
		    r);
		goto fail_0;
	}

	r = bus_dmamem_alloc(dma->dma_tag->dt_dmat, size,
	    dma->dma_tag->dt_alignment, dma->dma_tag->dt_boundary,
	    &dma->dma_seg, 1, &rsegs, BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(dev,
		    "%s: bus_dmamem_alloc failed; error %d\n", __func__, r);
		goto fail_1;
	}

	r = bus_dmamem_map(dma->dma_tag->dt_dmat, &dma->dma_seg, rsegs,
	    size, &dma->dma_vaddr, BUS_DMA_NOWAIT | BUS_DMA_COHERENT);
	if (r != 0) {
		aprint_error_dev(dev, "%s: bus_dmamem_map failed; error %d\n",
		    __func__, r);
		goto fail_2;
	}

	r = ixgbe_dmamap_create(dma->dma_tag, 0, &dma->dma_map);
	if (r != 0) {
		aprint_error_dev(dev, "%s: bus_dmamem_map failed; error %d\n",
		    __func__, r);
		goto fail_3;
	}

	r = bus_dmamap_load(dma->dma_tag->dt_dmat, dma->dma_map,
	    dma->dma_vaddr, size, NULL, mapflags | BUS_DMA_NOWAIT);
	if (r != 0) {
		aprint_error_dev(dev, "%s: bus_dmamap_load failed; error %d\n",
		    __func__, r);
		goto fail_4;
	}
	dma->dma_paddr = dma->dma_map->dm_segs[0].ds_addr;
	dma->dma_size = size;
	return 0;
fail_4:
	ixgbe_dmamap_destroy(dma->dma_tag, dma->dma_map);
fail_3:
	bus_dmamem_unmap(dma->dma_tag->dt_dmat, dma->dma_vaddr, size);
fail_2:
	bus_dmamem_free(dma->dma_tag->dt_dmat, &dma->dma_seg, rsegs);
fail_1:
	ixgbe_dma_tag_destroy(dma->dma_tag);
fail_0:

	return (r);
} /* ixgbe_dma_malloc */

/************************************************************************
 * ixgbe_dma_free
 ************************************************************************/
void
ixgbe_dma_free(struct adapter *adapter, struct ixgbe_dma_alloc *dma)
{
	bus_dmamap_sync(dma->dma_tag->dt_dmat, dma->dma_map, 0, dma->dma_size,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	ixgbe_dmamap_unload(dma->dma_tag, dma->dma_map);
	bus_dmamem_free(dma->dma_tag->dt_dmat, &dma->dma_seg, 1);
	ixgbe_dma_tag_destroy(dma->dma_tag);
} /* ixgbe_dma_free */


/************************************************************************
 * ixgbe_allocate_queues
 *
 *   Allocate memory for the transmit and receive rings, and then
 *   the descriptors associated with each, called only once at attach.
 ************************************************************************/
int
ixgbe_allocate_queues(struct adapter *adapter)
{
	device_t	dev = adapter->dev;
	struct ix_queue	*que;
	struct tx_ring	*txr;
	struct rx_ring	*rxr;
	int             rsize, tsize, error = IXGBE_SUCCESS;
	int             txconf = 0, rxconf = 0;

	/* First, allocate the top level queue structs */
	adapter->queues = (struct ix_queue *)malloc(sizeof(struct ix_queue) *
	    adapter->num_queues, M_DEVBUF, M_WAITOK | M_ZERO);

	/* Second, allocate the TX ring struct memory */
	adapter->tx_rings = malloc(sizeof(struct tx_ring) *
	    adapter->num_queues, M_DEVBUF, M_WAITOK | M_ZERO);

	/* Third, allocate the RX ring */
	adapter->rx_rings = (struct rx_ring *)malloc(sizeof(struct rx_ring) *
	    adapter->num_queues, M_DEVBUF, M_WAITOK | M_ZERO);

	/* For the ring itself */
	tsize = roundup2(adapter->num_tx_desc * sizeof(union ixgbe_adv_tx_desc),
	    DBA_ALIGN);

	/*
	 * Now set up the TX queues, txconf is needed to handle the
	 * possibility that things fail midcourse and we need to
	 * undo memory gracefully
	 */
	for (int i = 0; i < adapter->num_queues; i++, txconf++) {
		/* Set up some basics */
		txr = &adapter->tx_rings[i];
		txr->adapter = adapter;
		txr->txr_interq = NULL;
		/* In case SR-IOV is enabled, align the index properly */
#ifdef PCI_IOV
		txr->me = ixgbe_vf_que_index(adapter->iov_mode, adapter->pool,
		    i);
#else
		txr->me = i;
#endif
		txr->num_desc = adapter->num_tx_desc;

		/* Initialize the TX side lock */
		mutex_init(&txr->tx_mtx, MUTEX_DEFAULT, IPL_NET);

		if (ixgbe_dma_malloc(adapter, tsize, &txr->txdma,
		    BUS_DMA_NOWAIT)) {
			aprint_error_dev(dev,
			    "Unable to allocate TX Descriptor memory\n");
			error = ENOMEM;
			goto err_tx_desc;
		}
		txr->tx_base = (union ixgbe_adv_tx_desc *)txr->txdma.dma_vaddr;
		bzero((void *)txr->tx_base, tsize);

		/* Now allocate transmit buffers for the ring */
		if (ixgbe_allocate_transmit_buffers(txr)) {
			aprint_error_dev(dev,
			    "Critical Failure setting up transmit buffers\n");
			error = ENOMEM;
			goto err_tx_desc;
		}
		if (!(adapter->feat_en & IXGBE_FEATURE_LEGACY_TX)) {
			/* Allocate a buf ring */
			txr->txr_interq = pcq_create(IXGBE_BR_SIZE, KM_SLEEP);
			if (txr->txr_interq == NULL) {
				aprint_error_dev(dev,
				    "Critical Failure setting up buf ring\n");
				error = ENOMEM;
				goto err_tx_desc;
			}
		}
	}

	/*
	 * Next the RX queues...
	 */
	rsize = roundup2(adapter->num_rx_desc * sizeof(union ixgbe_adv_rx_desc),
	    DBA_ALIGN);
	for (int i = 0; i < adapter->num_queues; i++, rxconf++) {
		rxr = &adapter->rx_rings[i];
		/* Set up some basics */
		rxr->adapter = adapter;
#ifdef PCI_IOV
		/* In case SR-IOV is enabled, align the index properly */
		rxr->me = ixgbe_vf_que_index(adapter->iov_mode, adapter->pool,
		    i);
#else
		rxr->me = i;
#endif
		rxr->num_desc = adapter->num_rx_desc;

		/* Initialize the RX side lock */
		mutex_init(&rxr->rx_mtx, MUTEX_DEFAULT, IPL_NET);

		if (ixgbe_dma_malloc(adapter, rsize, &rxr->rxdma,
		    BUS_DMA_NOWAIT)) {
			aprint_error_dev(dev,
			    "Unable to allocate RxDescriptor memory\n");
			error = ENOMEM;
			goto err_rx_desc;
		}
		rxr->rx_base = (union ixgbe_adv_rx_desc *)rxr->rxdma.dma_vaddr;
		bzero((void *)rxr->rx_base, rsize);

		/* Allocate receive buffers for the ring */
		if (ixgbe_allocate_receive_buffers(rxr)) {
			aprint_error_dev(dev,
			    "Critical Failure setting up receive buffers\n");
			error = ENOMEM;
			goto err_rx_desc;
		}
	}

	/*
	 * Finally set up the queue holding structs
	 */
	for (int i = 0; i < adapter->num_queues; i++) {
		que = &adapter->queues[i];
		que->adapter = adapter;
		que->me = i;
		que->txr = &adapter->tx_rings[i];
		que->rxr = &adapter->rx_rings[i];

		mutex_init(&que->dc_mtx, MUTEX_DEFAULT, IPL_NET);
		que->disabled_count = 0;
	}

	return (0);

err_rx_desc:
	for (rxr = adapter->rx_rings; rxconf > 0; rxr++, rxconf--)
		ixgbe_dma_free(adapter, &rxr->rxdma);
err_tx_desc:
	for (txr = adapter->tx_rings; txconf > 0; txr++, txconf--)
		ixgbe_dma_free(adapter, &txr->txdma);
	free(adapter->rx_rings, M_DEVBUF);
	free(adapter->tx_rings, M_DEVBUF);
	free(adapter->queues, M_DEVBUF);
	return (error);
} /* ixgbe_allocate_queues */

/************************************************************************
 * ixgbe_free_queues
 *
 *   Free descriptors for the transmit and receive rings, and then
 *   the memory associated with each.
 ************************************************************************/
void
ixgbe_free_queues(struct adapter *adapter)
{
	struct ix_queue *que;
	int i;

	ixgbe_free_transmit_structures(adapter);
	ixgbe_free_receive_structures(adapter);
	for (i = 0; i < adapter->num_queues; i++) {
		que = &adapter->queues[i];
		mutex_destroy(&que->dc_mtx);
	}
	free(adapter->queues, M_DEVBUF);
} /* ixgbe_free_queues */
