/* $NetBSD: ixgbe.h,v 1.32.2.7 2018/06/25 07:26:01 pgoyette Exp $ */

/******************************************************************************
  SPDX-License-Identifier: BSD-3-Clause

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
/*$FreeBSD: head/sys/dev/ixgbe/ixgbe.h 327031 2017-12-20 18:15:06Z erj $*/

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


#ifndef _IXGBE_H_
#define _IXGBE_H_


#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/pcq.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/sockio.h>
#include <sys/percpu.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/bpf.h>
#include <net/if_ether.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#include <net/if_types.h>
#include <net/if_vlanvar.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <sys/bus.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/endian.h>
#include <sys/workqueue.h>
#include <sys/cpu.h>
#include <sys/interrupt.h>
#include <sys/bitops.h>

#include "ixgbe_netbsd.h"
#include "ixgbe_api.h"
#include "ixgbe_common.h"
#include "ixgbe_vf.h"
#include "ixgbe_features.h"

/* Tunables */

/*
 * TxDescriptors Valid Range: 64-4096 Default Value: 256 This value is the
 * number of transmit descriptors allocated by the driver. Increasing this
 * value allows the driver to queue more transmits. Each descriptor is 16
 * bytes. Performance tests have show the 2K value to be optimal for top
 * performance.
 */
#define DEFAULT_TXD	1024
#define PERFORM_TXD	2048
#define MAX_TXD		4096
#define MIN_TXD		64

/*
 * RxDescriptors Valid Range: 64-4096 Default Value: 256 This value is the
 * number of receive descriptors allocated for each RX queue. Increasing this
 * value allows the driver to buffer more incoming packets. Each descriptor
 * is 16 bytes.  A receive buffer is also allocated for each descriptor. 
 * 
 * Note: with 8 rings and a dual port card, it is possible to bump up 
 *	against the system mbuf pool limit, you can tune nmbclusters
 *	to adjust for this.
 */
#define DEFAULT_RXD	1024
#define PERFORM_RXD	2048
#define MAX_RXD		4096
#define MIN_RXD		64

/* Alignment for rings */
#define DBA_ALIGN	128

/*
 * This is the max watchdog interval, ie. the time that can
 * pass between any two TX clean operations, such only happening
 * when the TX hardware is functioning.
 */
#define IXGBE_WATCHDOG  (10 * hz)

/*
 * This parameters control when the driver calls the routine to reclaim
 * transmit descriptors.
 */
#define IXGBE_TX_CLEANUP_THRESHOLD(_a)  ((_a)->num_tx_desc / 8)
#define IXGBE_TX_OP_THRESHOLD(_a)       ((_a)->num_tx_desc / 32)

/* These defines are used in MTU calculations */
#define IXGBE_MAX_FRAME_SIZE  9728
#define IXGBE_MTU_HDR         (ETHER_HDR_LEN + ETHER_CRC_LEN)
#define IXGBE_MTU_HDR_VLAN    (ETHER_HDR_LEN + ETHER_CRC_LEN + \
                               ETHER_VLAN_ENCAP_LEN)
#define IXGBE_MAX_MTU         (IXGBE_MAX_FRAME_SIZE - IXGBE_MTU_HDR)
#define IXGBE_MAX_MTU_VLAN    (IXGBE_MAX_FRAME_SIZE - IXGBE_MTU_HDR_VLAN)

/* Flow control constants */
#define IXGBE_FC_PAUSE        0xFFFF

/*
 * Used for optimizing small rx mbufs.  Effort is made to keep the copy
 * small and aligned for the CPU L1 cache.
 *
 * MHLEN is typically 168 bytes, giving us 8-byte alignment.  Getting
 * 32 byte alignment needed for the fast bcopy results in 8 bytes being
 * wasted.  Getting 64 byte alignment, which _should_ be ideal for
 * modern Intel CPUs, results in 40 bytes wasted and a significant drop
 * in observed efficiency of the optimization, 97.9% -> 81.8%.
 */
#define	MPKTHSIZE		(offsetof(struct _mbuf_dummy, m_pktdat))
#define IXGBE_RX_COPY_HDR_PADDED  ((((MPKTHSIZE - 1) / 32) + 1) * 32)
#define IXGBE_RX_COPY_LEN         (MSIZE - IXGBE_RX_COPY_HDR_PADDED)
#define IXGBE_RX_COPY_ALIGN       (IXGBE_RX_COPY_HDR_PADDED - MPKTHSIZE)

/* Keep older OS drivers building... */
#if !defined(SYSCTL_ADD_UQUAD)
#define SYSCTL_ADD_UQUAD SYSCTL_ADD_QUAD
#endif

/* Defines for printing debug information */
#define DEBUG_INIT  0
#define DEBUG_IOCTL 0
#define DEBUG_HW    0

#define INIT_DEBUGOUT(S)            if (DEBUG_INIT)  printf(S "\n")
#define INIT_DEBUGOUT1(S, A)        if (DEBUG_INIT)  printf(S "\n", A)
#define INIT_DEBUGOUT2(S, A, B)     if (DEBUG_INIT)  printf(S "\n", A, B)
#define IOCTL_DEBUGOUT(S)           if (DEBUG_IOCTL) printf(S "\n")
#define IOCTL_DEBUGOUT1(S, A)       if (DEBUG_IOCTL) printf(S "\n", A)
#define IOCTL_DEBUGOUT2(S, A, B)    if (DEBUG_IOCTL) printf(S "\n", A, B)
#define HW_DEBUGOUT(S)              if (DEBUG_HW) printf(S "\n")
#define HW_DEBUGOUT1(S, A)          if (DEBUG_HW) printf(S "\n", A)
#define HW_DEBUGOUT2(S, A, B)       if (DEBUG_HW) printf(S "\n", A, B)

#define MAX_NUM_MULTICAST_ADDRESSES     128
#define IXGBE_82598_SCATTER             100
#define IXGBE_82599_SCATTER             32
#define MSIX_82598_BAR                  3
#define MSIX_82599_BAR                  4
#define IXGBE_TSO_SIZE                  262140
#define IXGBE_RX_HDR                    128
#define IXGBE_VFTA_SIZE                 128
#define IXGBE_BR_SIZE                   4096
#define IXGBE_QUEUE_MIN_FREE            32
#define IXGBE_MAX_TX_BUSY               10
#define IXGBE_QUEUE_HUNG                0x80000000

#define IXGBE_EITR_DEFAULT		128

/* IOCTL define to gather SFP+ Diagnostic data */
#define SIOCGI2C	SIOCGIFGENERIC

/* Offload bits in mbuf flag */
#define	M_CSUM_OFFLOAD	\
    (M_CSUM_IPv4|M_CSUM_UDPv4|M_CSUM_TCPv4|M_CSUM_UDPv6|M_CSUM_TCPv6)

/* Backward compatibility items for very old versions */
#ifndef pci_find_cap
#define pci_find_cap pci_find_extcap
#endif

#ifndef DEVMETHOD_END
#define DEVMETHOD_END { NULL, NULL }
#endif

/*
 * Interrupt Moderation parameters 
 */
#define IXGBE_LOW_LATENCY	128
#define IXGBE_AVE_LATENCY	400
#define IXGBE_BULK_LATENCY	1200

/* Using 1FF (the max value), the interval is ~1.05ms */
#define IXGBE_LINK_ITR_QUANTA  0x1FF
#define IXGBE_LINK_ITR         ((IXGBE_LINK_ITR_QUANTA << 3) & \
                                IXGBE_EITR_ITR_INT_MASK)


/************************************************************************
 * vendor_info_array
 *
 *   Contains the list of Subvendor/Subdevice IDs on
 *   which the driver should load.
 ************************************************************************/
typedef struct _ixgbe_vendor_info_t {
	unsigned int vendor_id;
	unsigned int device_id;
	unsigned int subvendor_id;
	unsigned int subdevice_id;
	unsigned int index;
} ixgbe_vendor_info_t;

/* This is used to get SFP+ module data */
struct ixgbe_i2c_req {
        u8 dev_addr;
        u8 offset;
        u8 len;
        u8 data[8];
};

struct ixgbe_bp_data {
	u32 low;
	u32 high;
	u32 log;
};

struct ixgbe_tx_buf {
	union ixgbe_adv_tx_desc	*eop;
	struct mbuf             *m_head;
	bus_dmamap_t            map;
};

struct ixgbe_rx_buf {
	struct mbuf    *buf;
	struct mbuf    *fmp;
	bus_dmamap_t   pmap;
	u_int          flags;
#define IXGBE_RX_COPY  0x01
	uint64_t       addr;
};

/*
 * Bus dma allocation structure used by ixgbe_dma_malloc and ixgbe_dma_free
 */
struct ixgbe_dma_alloc {
	bus_addr_t        dma_paddr;
	void              *dma_vaddr;
	ixgbe_dma_tag_t   *dma_tag;
	bus_dmamap_t      dma_map;
	bus_dma_segment_t dma_seg;
	bus_size_t        dma_size;
};

struct ixgbe_mc_addr {
	u8  addr[IXGBE_ETH_LENGTH_OF_ADDRESS];
	u32 vmdq;
};

/*
 * Driver queue struct: this is the interrupt container
 *                      for the associated tx and rx ring.
 */
struct ix_queue {
	struct adapter   *adapter;
	u32              msix;           /* This queue's MSI-X vector */
	u32              eitr_setting;
	u32              me;
	struct resource  *res;
	int              busy;
	struct tx_ring   *txr;
	struct rx_ring   *rxr;
	struct work      wq_cookie;
	void             *que_si;
	/* Per queue event conters */
	struct evcnt     irqs;		/* Hardware interrupt */
	struct evcnt     handleq;	/* software_interrupt */
	struct evcnt     req;		/* deferred */
	char             namebuf[32];
	char             evnamebuf[32];

	kmutex_t         dc_mtx;	/* lock for disabled_count and this queue's EIMS/EIMC bit */
	int              disabled_count;/*
					 * means
					 *     0   : this queue is enabled
					 *     > 0 : this queue is disabled
					 *           the value is ixgbe_disable_queue() called count
					 */
	bool             txrx_use_workqueue;
};

/*
 * The transmit ring, one per queue
 */
struct tx_ring {
        struct adapter		*adapter;
	kmutex_t		tx_mtx;
	u32			me;
	u32			tail;
	int			busy;
	union ixgbe_adv_tx_desc	*tx_base;
	struct ixgbe_tx_buf	*tx_buffers;
	struct ixgbe_dma_alloc	txdma;
	volatile u16		tx_avail;
	u16			next_avail_desc;
	u16			next_to_clean;
	u16			num_desc;
	ixgbe_dma_tag_t		*txtag;
#if 0
	char			mtx_name[16]; /* NetBSD has no mutex name */
#endif
	pcq_t			*txr_interq;
	struct work		wq_cookie;
	void			*txr_si;
	bool			txr_no_space; /* Like IFF_OACTIVE */

	/* Flow Director */
	u16			atr_sample;
	u16			atr_count;

	u64			bytes;  /* used for AIM */
	u64			packets;
	/* Soft Stats */
	struct evcnt	   	tso_tx;
	struct evcnt		no_desc_avail;
	struct evcnt		total_packets;
	struct evcnt		pcq_drops;
	/* Per queue conters.  The adapter total is in struct adapter */
	u64              q_efbig_tx_dma_setup;
	u64              q_mbuf_defrag_failed;
	u64              q_efbig2_tx_dma_setup;
	u64              q_einval_tx_dma_setup;
	u64              q_other_tx_dma_setup;
	u64              q_eagain_tx_dma_setup;
	u64              q_enomem_tx_dma_setup;
	u64              q_tso_err;
};


/*
 * The Receive ring, one per rx queue
 */
struct rx_ring {
        struct adapter		*adapter;
	kmutex_t		rx_mtx;
	u32			me;
	u32			tail;
	union ixgbe_adv_rx_desc	*rx_base;
	struct ixgbe_dma_alloc	rxdma;
#ifdef LRO
	struct lro_ctrl		lro;
#endif /* LRO */
	bool			lro_enabled;
	bool			hw_rsc;
	bool			vtag_strip;
        u16			next_to_refresh;
        u16 			next_to_check;
	u16			num_desc;
	u16			mbuf_sz;
#if 0
	char			mtx_name[16]; /* NetBSD has no mutex name */
#endif
	struct ixgbe_rx_buf	*rx_buffers;
	ixgbe_dma_tag_t		*ptag;

	u64			bytes; /* Used for AIM calc */
	u64			packets;

	/* Soft stats */
	struct evcnt		rx_copies;
	struct evcnt		rx_packets;
	struct evcnt 		rx_bytes;
	struct evcnt 		rx_discarded;
	struct evcnt 		no_jmbuf;
	u64 			rsc_num;

	/* Flow Director */
	u64			flm;
};

#define IXGBE_MAX_VF_MC 30  /* Max number of multicast entries */

struct ixgbe_vf {
	u_int    pool;
	u_int    rar_index;
	u_int    max_frame_size;
	uint32_t flags;
	uint8_t  ether_addr[ETHER_ADDR_LEN];
	uint16_t mc_hash[IXGBE_MAX_VF_MC];
	uint16_t num_mc_hashes;
	uint16_t default_vlan;
	uint16_t vlan_tag;
	uint16_t api_ver;
};

/* Our adapter structure */
struct adapter {
	struct ixgbe_hw		hw;
	struct ixgbe_osdep	osdep;

	device_t		dev;
	struct ifnet		*ifp;
	struct if_percpuq	*ipq;	/* softint-based input queues */

	struct resource		*pci_mem;
	struct resource		*msix_mem;

	/* NetBSD: Interrupt resources are in osdep */

	struct ifmedia		media;
	callout_t		timer;
	int			if_flags;

	kmutex_t		core_mtx;

	unsigned int		num_queues;

	/*
	 * Shadow VFTA table, this is needed because
	 * the real vlan filter table gets cleared during
	 * a soft reset and the driver needs to be able
	 * to repopulate it.
	 */
	u32			shadow_vfta[IXGBE_VFTA_SIZE];

	/* Info about the interface */
	int			advertise;  /* link speeds */
	bool			enable_aim; /* adaptive interrupt moderation */
	bool			link_active;
	u16			max_frame_size;
	u16			num_segs;
	u32			link_speed;
	bool			link_up;
	u32 			vector;
	u16			dmac;
	u32			phy_layer;

	/* Power management-related */
	bool			wol_support;
	u32			wufc;

	/* Mbuf cluster size */
	u32			rx_mbuf_sz;

	/* Support for pluggable optics */
	bool			sfp_probe;
	void			*link_si;  /* Link tasklet */
	void			*mod_si;   /* SFP tasklet */
	void			*msf_si;   /* Multispeed Fiber */
	void			*mbx_si;   /* VF -> PF mailbox interrupt */

	/* Flow Director */
	int			fdir_reinit;
	void			*fdir_si;

	void			*phy_si;   /* PHY intr tasklet */

	bool			txrx_use_workqueue;
	struct workqueue	*que_wq;    /* workqueue for ixgbe_handle_que_work() */
					    /*
					     * que_wq's "enqueued flag" is not required,
					     * because twice workqueue_enqueue() for
					     * ixgbe_handle_que_work() is avoided by masking
					     * the queue's interrupt by EIMC.
					     * See also ixgbe_msix_que().
					     */
	struct workqueue	*txr_wq;    /* workqueue for ixgbe_deferred_mq_start_work() */
	percpu_t		*txr_wq_enqueued;

	/*
	 * Queues:
	 *   This is the irq holder, it has
	 *   and RX/TX pair or rings associated
	 *   with it.
	 */
	struct ix_queue		*queues;

	/*
	 * Transmit rings
	 *      Allocated at run time, an array of rings
	 */
	struct tx_ring		*tx_rings;
	u32			num_tx_desc;
	u32			tx_process_limit;

	/*
	 * Receive rings
	 *      Allocated at run time, an array of rings
	 */
	struct rx_ring		*rx_rings;
	u64			active_queues;
	u32			num_rx_desc;
	u32			rx_process_limit;

	/* Multicast array memory */
	struct ixgbe_mc_addr	*mta;

	/* SR-IOV */
	int                     iov_mode;
	int			num_vfs;
	int			pool;
	struct ixgbe_vf		*vfs;

	/* Bypass */
	struct ixgbe_bp_data    bypass;

	/* Netmap */
	void 			(*init_locked)(struct adapter *);
	void 			(*stop_locked)(void *);

	/* Misc stats maintained by the driver */
	struct evcnt	   	efbig_tx_dma_setup;
	struct evcnt   		mbuf_defrag_failed;
	struct evcnt	   	efbig2_tx_dma_setup;
	struct evcnt	   	einval_tx_dma_setup;
	struct evcnt	   	other_tx_dma_setup;
	struct evcnt	   	eagain_tx_dma_setup;
	struct evcnt	   	enomem_tx_dma_setup;
	struct evcnt	   	tso_err;
	struct evcnt	   	watchdog_events;
	struct evcnt		link_irq;
	struct evcnt		link_sicount;
	struct evcnt		mod_sicount;
	struct evcnt		msf_sicount;
	struct evcnt		phy_sicount;

	union {
		struct ixgbe_hw_stats pf;
		struct ixgbevf_hw_stats	vf;
	} stats;
#if __FreeBSD_version >= 1100036
	/* counter(9) stats */
	u64			ipackets;
	u64			ierrors;
	u64			opackets;
	u64			oerrors;
	u64			ibytes;
	u64			obytes;
	u64			imcasts;
	u64			omcasts;
	u64			iqdrops;
	u64			noproto;
#endif
	/* Feature capable/enabled flags.  See ixgbe_features.h */
	u32                     feat_cap;
	u32                     feat_en;

	struct sysctllog	*sysctllog;
	const struct sysctlnode *sysctltop;
	ixgbe_extmem_head_t jcl_head;
};

/* Precision Time Sync (IEEE 1588) defines */
#define ETHERTYPE_IEEE1588      0x88F7
#define PICOSECS_PER_TICK       20833
#define TSYNC_UDP_PORT          319 /* UDP port for the protocol */
#define IXGBE_ADVTXD_TSTAMP     0x00080000


#define IXGBE_CORE_LOCK_INIT(_sc, _name) \
        mutex_init(&(_sc)->core_mtx, MUTEX_DEFAULT, IPL_SOFTNET)
#define IXGBE_CORE_LOCK_DESTROY(_sc)      mutex_destroy(&(_sc)->core_mtx)
#define IXGBE_TX_LOCK_DESTROY(_sc)        mutex_destroy(&(_sc)->tx_mtx)
#define IXGBE_RX_LOCK_DESTROY(_sc)        mutex_destroy(&(_sc)->rx_mtx)
#define IXGBE_CORE_LOCK(_sc)              mutex_enter(&(_sc)->core_mtx)
#define IXGBE_TX_LOCK(_sc)                mutex_enter(&(_sc)->tx_mtx)
#define IXGBE_TX_TRYLOCK(_sc)             mutex_tryenter(&(_sc)->tx_mtx)
#define IXGBE_RX_LOCK(_sc)                mutex_enter(&(_sc)->rx_mtx)
#define IXGBE_CORE_UNLOCK(_sc)            mutex_exit(&(_sc)->core_mtx)
#define IXGBE_TX_UNLOCK(_sc)              mutex_exit(&(_sc)->tx_mtx)
#define IXGBE_RX_UNLOCK(_sc)              mutex_exit(&(_sc)->rx_mtx)
#define IXGBE_CORE_LOCK_ASSERT(_sc)       KASSERT(mutex_owned(&(_sc)->core_mtx))
#define IXGBE_TX_LOCK_ASSERT(_sc)         KASSERT(mutex_owned(&(_sc)->tx_mtx))

/* Stats macros */
#if __FreeBSD_version >= 1100036
#define IXGBE_SET_IPACKETS(sc, count)    (sc)->ipackets = (count)
#define IXGBE_SET_IERRORS(sc, count)     (sc)->ierrors = (count)
#define IXGBE_SET_OPACKETS(sc, count)    (sc)->opackets = (count)
#define IXGBE_SET_OERRORS(sc, count)     (sc)->oerrors = (count)
#define IXGBE_SET_COLLISIONS(sc, count)
#define IXGBE_SET_IBYTES(sc, count)      (sc)->ibytes = (count)
#define IXGBE_SET_OBYTES(sc, count)      (sc)->obytes = (count)
#define IXGBE_SET_IMCASTS(sc, count)     (sc)->imcasts = (count)
#define IXGBE_SET_OMCASTS(sc, count)     (sc)->omcasts = (count)
#define IXGBE_SET_IQDROPS(sc, count)     (sc)->iqdrops = (count)
#else
#define IXGBE_SET_IPACKETS(sc, count)    (sc)->ifp->if_ipackets = (count)
#define IXGBE_SET_IERRORS(sc, count)     (sc)->ifp->if_ierrors = (count)
#define IXGBE_SET_OPACKETS(sc, count)    (sc)->ifp->if_opackets = (count)
#define IXGBE_SET_OERRORS(sc, count)     (sc)->ifp->if_oerrors = (count)
#define IXGBE_SET_COLLISIONS(sc, count)  (sc)->ifp->if_collisions = (count)
#define IXGBE_SET_IBYTES(sc, count)      (sc)->ifp->if_ibytes = (count)
#define IXGBE_SET_OBYTES(sc, count)      (sc)->ifp->if_obytes = (count)
#define IXGBE_SET_IMCASTS(sc, count)     (sc)->ifp->if_imcasts = (count)
#define IXGBE_SET_OMCASTS(sc, count)     (sc)->ifp->if_omcasts = (count)
#define IXGBE_SET_IQDROPS(sc, count)     (sc)->ifp->if_iqdrops = (count)
#endif

/* External PHY register addresses */
#define IXGBE_PHY_CURRENT_TEMP		0xC820
#define IXGBE_PHY_OVERTEMP_STATUS	0xC830

/* Sysctl help messages; displayed with sysctl -d */
#define IXGBE_SYSCTL_DESC_ADV_SPEED \
	"\nControl advertised link speed using these flags:\n" \
	"\t0x01 - advertise 100M\n" \
	"\t0x02 - advertise 1G\n" \
        "\t0x04 - advertise 10G\n" \
        "\t0x08 - advertise 10M\n" \
        "\t0x10 - advertise 2.5G\n" \
        "\t0x20 - advertise 5G\n\n" \
        "\t5G, 2.5G, 100M and 10M are only supported on certain adapters."

#define IXGBE_SYSCTL_DESC_SET_FC \
	"\nSet flow control mode using these values:\n" \
	"\t0 - off\n" \
	"\t1 - rx pause\n" \
	"\t2 - tx pause\n" \
	"\t3 - tx and rx pause"

/* Workaround to make 8.0 buildable */
#if __FreeBSD_version >= 800000 && __FreeBSD_version < 800504
static __inline int
drbr_needs_enqueue(struct ifnet *ifp, struct buf_ring *br)
{
#ifdef ALTQ
	if (ALTQ_IS_ENABLED(&ifp->if_snd))
		return (1);
#endif
	return (!buf_ring_empty(br));
}
#endif

/*
 * Find the number of unrefreshed RX descriptors
 */
static __inline u16
ixgbe_rx_unrefreshed(struct rx_ring *rxr)
{
	if (rxr->next_to_check > rxr->next_to_refresh)
		return (rxr->next_to_check - rxr->next_to_refresh - 1);
	else
		return ((rxr->num_desc + rxr->next_to_check) -
		    rxr->next_to_refresh - 1);
}

static __inline int
ixgbe_legacy_ring_empty(struct ifnet *ifp, pcq_t *dummy)
{
	UNREFERENCED_1PARAMETER(dummy);

	return IFQ_IS_EMPTY(&ifp->if_snd);
}

static __inline int
ixgbe_mq_ring_empty(struct ifnet *dummy, pcq_t *interq)
{
	UNREFERENCED_1PARAMETER(dummy);

	return (pcq_peek(interq) == NULL);
}

/*
 * This checks for a zero mac addr, something that will be likely
 * unless the Admin on the Host has created one.
 */
static __inline bool
ixv_check_ether_addr(u8 *addr)
{
	bool status = TRUE;

	if ((addr[0] == 0 && addr[1]== 0 && addr[2] == 0 &&
	    addr[3] == 0 && addr[4]== 0 && addr[5] == 0))
		status = FALSE;

	return (status);
}

/* Shared Prototypes */
void ixgbe_legacy_start(struct ifnet *);
int  ixgbe_legacy_start_locked(struct ifnet *, struct tx_ring *);
int  ixgbe_mq_start(struct ifnet *, struct mbuf *);
int  ixgbe_mq_start_locked(struct ifnet *, struct tx_ring *);
void ixgbe_deferred_mq_start(void *);
void ixgbe_deferred_mq_start_work(struct work *, void *);
void ixgbe_drain_all(struct adapter *);

int  ixgbe_allocate_queues(struct adapter *);
int  ixgbe_setup_transmit_structures(struct adapter *);
void ixgbe_free_transmit_structures(struct adapter *);
int  ixgbe_setup_receive_structures(struct adapter *);
void ixgbe_free_receive_structures(struct adapter *);
bool ixgbe_txeof(struct tx_ring *);
bool ixgbe_rxeof(struct ix_queue *);

const struct sysctlnode *ixgbe_sysctl_instance(struct adapter *);

/* For NetBSD */
void ixgbe_jcl_reinit(struct adapter *, bus_dma_tag_t, int, size_t);

#include "ixgbe_bypass.h"
#include "ixgbe_fdir.h"
#include "ixgbe_rss.h"
#include "ixgbe_netmap.h"

#endif /* _IXGBE_H_ */
