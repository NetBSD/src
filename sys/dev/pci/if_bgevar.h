/*	$NetBSD: if_bgevar.h,v 1.40.4.1 2024/10/13 15:44:54 martin Exp $	*/
/*
 * Copyright (c) 2001 Wind River Systems
 * Copyright (c) 1997, 1998, 1999, 2001
 *	Bill Paul <wpaul@windriver.com>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: if_bgereg.h,v 1.1.2.7 2002/11/02 18:17:55 mp Exp $
 */

/*
 * BCM570x memory map. The internal memory layout varies somewhat
 * depending on whether or not we have external SSRAM attached.
 * The BCM5700 can have up to 16MB of external memory. The BCM5701
 * is apparently not designed to use external SSRAM. The mappings
 * up to the first 4 send rings are the same for both internal and
 * external memory configurations. Note that mini RX ring space is
 * only available with external SSRAM configurations, which means
 * the mini RX ring is not supported on the BCM5701.
 *
 * The NIC's memory can be accessed by the host in one of 3 ways:
 *
 * 1) Indirect register access. The MEMWIN_BASEADDR and MEMWIN_DATA
 *    registers in PCI config space can be used to read any 32-bit
 *    address within the NIC's memory.
 *
 * 2) Memory window access. The MEMWIN_BASEADDR register in PCI config
 *    space can be used in conjunction with the memory window in the
 *    device register space at offset 0x8000 to read any 32K chunk
 *    of NIC memory.
 *
 * 3) Flat mode. If the 'flat mode' bit in the PCI state register is
 *    set, the device I/O mapping consumes 32MB of host address space,
 *    allowing all of the registers and internal NIC memory to be
 *    accessed directly. NIC memory addresses are offset by 0x01000000.
 *    Flat mode consumes so much host address space that it is not
 *    recommended.
 */

#ifndef _DEV_PCI_IF_BGEVAR_H_
#define _DEV_PCI_IF_BGEVAR_H_

#include <sys/bus.h>
#include <sys/rndsource.h>
#include <sys/time.h>

#include <net/if_ether.h>

#include <dev/pci/pcivar.h>

#define BGE_HOSTADDR(x, y)						      \
	do {								      \
		(x).bge_addr_lo = BUS_ADDR_LO32(y);			      \
		if (sizeof (bus_addr_t) == 8)				      \
			(x).bge_addr_hi = BUS_ADDR_HI32(y);		      \
		else							      \
			(x).bge_addr_hi = 0;				      \
	} while(0)

#define RCB_WRITE_4(sc, rcb, offset, val)				      \
	bus_space_write_4(sc->bge_btag, sc->bge_bhandle,		      \
			  rcb + offsetof(struct bge_rcb, offset), val)

/*
 * Other utility macros.
 */
#define BGE_INC(x, y)	(x) = (x + 1) % y

/*
 * Register access macros. The Tigon always uses memory mapped register
 * accesses and all registers must be accessed with 32 bit operations.
 */

#define CSR_WRITE_4(sc, reg, val)					      \
	bus_space_write_4(sc->bge_btag, sc->bge_bhandle, reg, val)

#define CSR_READ_4(sc, reg)						      \
	bus_space_read_4(sc->bge_btag, sc->bge_bhandle, reg)

#define CSR_WRITE_4_FLUSH(sc, reg, val)					      \
	do {								      \
		CSR_WRITE_4(sc, reg, val);				      \
		CSR_READ_4(sc, reg);					      \
	} while (0)

#define BGE_SETBIT(sc, reg, x)						      \
	CSR_WRITE_4(sc, reg, (CSR_READ_4(sc, reg) | (x)))
#define BGE_SETBIT_FLUSH(sc, reg, x)					      \
	do {								      \
		BGE_SETBIT(sc, reg, x);					      \
		CSR_READ_4(sc, reg);					      \
	} while (0)
#define BGE_CLRBIT(sc, reg, x)						      \
	CSR_WRITE_4(sc, reg, (CSR_READ_4(sc, reg) & ~(x)))
#define BGE_CLRBIT_FLUSH(sc, reg, x)					      \
	do {								      \
		BGE_CLRBIT(sc, reg, x);					      \
		CSR_READ_4(sc, reg);					      \
	} while (0)

/* BAR2 APE register access macros. */
#define	APE_WRITE_4(sc, reg, val)					      \
	bus_space_write_4(sc->bge_apetag, sc->bge_apehandle, reg, val)

#define	APE_READ_4(sc, reg)						      \
	bus_space_read_4(sc->bge_apetag, sc->bge_apehandle, reg)

#define	APE_WRITE_4_FLUSH(sc, reg, val)					      \
	do {								      \
		APE_WRITE_4(sc, reg, val);				      \
		APE_READ_4(sc, reg);					      \
	} while (0)

#define	APE_SETBIT(sc, reg, x)						      \
	APE_WRITE_4(sc, reg, (APE_READ_4(sc, reg) | (x)))
#define	APE_CLRBIT(sc, reg, x)						      \
	APE_WRITE_4(sc, reg, (APE_READ_4(sc, reg) & ~(x)))

#define PCI_SETBIT(pc, tag, reg, x)					      \
	pci_conf_write(pc, tag, reg, (pci_conf_read(pc, tag, reg) | (x)))
#define PCI_CLRBIT(pc, tag, reg, x)					      \
	pci_conf_write(pc, tag, reg, (pci_conf_read(pc, tag, reg) & ~(x)))

/*
 * Memory management stuff. Note: the SSLOTS, MSLOTS and JSLOTS
 * values are tuneable. They control the actual amount of buffers
 * allocated for the standard, mini and jumbo receive rings.
 */

#define BGE_SSLOTS	256
#define BGE_MSLOTS	256
#define BGE_JSLOTS	384

#define BGE_JRAWLEN	(BGE_JUMBO_FRAMELEN + ETHER_ALIGN)
#define BGE_JLEN	(BGE_JRAWLEN + (sizeof(uint64_t) - 		      \
			    (BGE_JRAWLEN % sizeof(uint64_t))))
#define BGE_JPAGESZ 	PAGE_SIZE
#define BGE_RESID 	(BGE_JPAGESZ - (BGE_JLEN * BGE_JSLOTS) % BGE_JPAGESZ)
#define BGE_JMEM 	((BGE_JLEN * BGE_JSLOTS) + BGE_RESID)

/*
 * Ring structures. Most of these reside in host memory and we tell
 * the NIC where they are via the ring control blocks. The exceptions
 * are the tx and command rings, which live in NIC memory and which
 * we access via the shared memory window.
 */
struct bge_ring_data {
	struct bge_rx_bd	bge_rx_std_ring[BGE_STD_RX_RING_CNT];
	struct bge_rx_bd	bge_rx_jumbo_ring[BGE_JUMBO_RX_RING_CNT];
	struct bge_rx_bd	bge_rx_return_ring[BGE_RETURN_RING_CNT];
	struct bge_tx_bd	bge_tx_ring[BGE_TX_RING_CNT];
	struct bge_status_block	bge_status_block;
	struct bge_tx_desc	*bge_tx_ring_nic;/* pointer to shared mem */
	struct bge_cmd_desc	*bge_cmd_ring;	/* pointer to shared mem */
	struct bge_gib		bge_info;
};

#define BGE_RING_DMA_ADDR(sc, offset)					      \
	((sc)->bge_ring_map->dm_segs[0].ds_addr +			      \
	offsetof(struct bge_ring_data, offset))

/*
 * Number of DMA segments in a TxCB. Note that this is carefully
 * chosen to make the total struct size an even power of two. It's
 * critical that no TxCB be split across a page boundary since
 * no attempt is made to allocate physically contiguous memory.
 *
 */
#if 0	/* pre-TSO values */
#define BGE_TXDMA_MAX	ETHER_MAX_LEN_JUMBO
#ifdef _LP64
#define BGE_NTXSEG	30
#else
#define BGE_NTXSEG	31
#endif
#else	/* TSO values */
#define BGE_TXDMA_MAX	(round_page(IP_MAXPACKET))	/* for TSO */
#ifdef _LP64
#define BGE_NTXSEG	120	/* XXX just a guess */
#else
#define BGE_NTXSEG	124	/* XXX just a guess */
#endif
#endif	/* TSO values */

#define	BGE_STATUS_BLK_SZ	sizeof (struct bge_status_block)

/*
 * Mbuf pointers. We need these to keep track of the virtual addresses
 * of our mbuf chains since we can only convert from physical to virtual,
 * not the other way around.
 */
struct bge_chain_data {
	struct mbuf		*bge_tx_chain[BGE_TX_RING_CNT];
	struct mbuf		*bge_rx_std_chain[BGE_STD_RX_RING_CNT];
	struct mbuf		*bge_rx_jumbo_chain[BGE_JUMBO_RX_RING_CNT];
	bus_dmamap_t		bge_rx_std_map[BGE_STD_RX_RING_CNT];
	bus_dmamap_t		bge_rx_jumbo_map;
	bus_dma_segment_t	bge_rx_jumbo_seg;
	/* Stick the jumbo mem management stuff here too. */
	void *			bge_jslots[BGE_JSLOTS];
	void *			bge_jumbo_buf;
};

#define BGE_JUMBO_DMA_ADDR(sc, m) \
	((sc)->bge_cdata.bge_rx_jumbo_map->dm_segs[0].ds_addr + \
	 (mtod((m), char *) - (char *)(sc)->bge_cdata.bge_jumbo_buf))

struct bge_type {
	uint16_t		bge_vid;
	uint16_t		bge_did;
	char			*bge_name;
};

#define BGE_TIMEOUT		100000
#define BGE_TXCONS_UNSET		0xFFFF	/* impossible value */

struct bge_jpool_entry {
	int				slot;
	SLIST_ENTRY(bge_jpool_entry)	jpool_entries;
};

struct bge_bcom_hack {
	int			reg;
	int			val;
};

struct txdmamap_pool_entry {
	bus_dmamap_t dmamap;
	bus_dmamap_t dmamap32;
	bool is_dma32;
	SLIST_ENTRY(txdmamap_pool_entry) link;
};

#define	ASF_ENABLE		1
#define	ASF_NEW_HANDSHAKE	2
#define	ASF_STACKUP		4

/*
 * Locking notes:
 *
 *	n		IFNET_LOCK
 *	m		sc_mcast_lock
 *	i		sc_intr_lock
 *	i/n		while down, IFNET_LOCK; while up, sc_intr_lock
 *
 * Otherwise, stable from attach to detach.
 *
 * Lock order:
 *
 *	IFNET_LOCK -> sc_intr_lock
 *	IFNET_LOCK -> sc_mcast_lock
 */
struct bge_softc {
	device_t		bge_dev;
	struct ethercom		ethercom;	/* interface info */
	bus_space_handle_t	bge_bhandle;
	bus_space_tag_t		bge_btag;
	bus_size_t		bge_bsize;
	bus_space_handle_t	bge_apehandle;
	bus_space_tag_t		bge_apetag;
	bus_size_t		bge_apesize;
	void			*bge_intrhand;
	pci_intr_handle_t	*bge_pihp;
	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_pcitag;

	struct pci_attach_args	bge_pa;
	struct mii_data		bge_mii;	/* i: mii data */
	struct ifmedia		bge_ifmedia;	/* i: media info */
	uint32_t		bge_return_ring_cnt;
	uint32_t		bge_tx_prodidx; /* i: tx producer idx */
	bus_dma_tag_t		bge_dmatag;
	bus_dma_tag_t		bge_dmatag32;
	bool			bge_dma64;
	uint32_t		bge_pcixcap;
	uint32_t		bge_pciecap;
	uint16_t		bge_mps;
	int			bge_expmrq;
	uint32_t		bge_lasttag;	/* i: last status tag */
	uint32_t		bge_mfw_flags;  /* Management F/W flags */
#define	BGE_MFW_ON_RXCPU	__BIT(0)
#define	BGE_MFW_ON_APE		__BIT(1)
#define	BGE_MFW_TYPE_NCSI	__BIT(2)
#define	BGE_MFW_TYPE_DASH	__BIT(3)
	int			bge_phy_ape_lock;
	int			bge_phy_addr;
	uint32_t		bge_chipid;
	uint8_t			bge_asf_mode;
	uint8_t			bge_asf_count;	/* i: XXX ??? */
	struct bge_ring_data	*bge_rdata;	/* rings */
	struct bge_chain_data	bge_cdata;	/* mbufs */
	bus_dmamap_t		bge_ring_map;
	bus_dma_segment_t	bge_ring_seg;
	int			bge_ring_rseg;
	uint16_t		bge_tx_saved_considx; /* i: tx consumer idx */
	uint16_t		bge_rx_saved_considx; /* i: rx consumer idx */
	uint16_t		bge_std;	/* i: current std ring head */
	uint16_t		bge_std_cnt;	/* i: number of std mbufs */
	uint16_t		bge_jumbo;
					/* i: current jumbo ring head */
	SLIST_HEAD(__bge_jfreehead, bge_jpool_entry)	bge_jfree_listhead;
					/* i: list of free jumbo mbufs */
	SLIST_HEAD(__bge_jinusehead, bge_jpool_entry)	bge_jinuse_listhead;
					/* i: list of jumbo mbufs in use */
	uint32_t		bge_stat_ticks;
	uint32_t		bge_rx_coal_ticks;	/* i */
	uint32_t		bge_tx_coal_ticks;	/* i */
	uint32_t		bge_rx_max_coal_bds;	/* i */
	uint32_t		bge_tx_max_coal_bds;	/* i */
	uint32_t		bge_sts;	/* i/n: link status */
#define BGE_STS_LINK		__BIT(0)	/* MAC link status */
#define BGE_STS_LINK_EVT	__BIT(1)	/* pending link event */
#define BGE_STS_AUTOPOLL	__BIT(2)	/* PHY auto-polling  */
#define BGE_STS_BIT(sc, x)	((sc)->bge_sts & (x))
#define BGE_STS_SETBIT(sc, x)	((sc)->bge_sts |= (x))
#define BGE_STS_CLRBIT(sc, x)	((sc)->bge_sts &= ~(x))
	u_short			bge_if_flags;	/* m: if_flags cache */
	uint32_t		bge_flags;	/* i/n */
	uint32_t		bge_phy_flags;
	int			bge_flowflags;	/* i */
	time_t			bge_tx_lastsent;
						/* i: time of last tx */
	bool			bge_txrx_stopping;
						/* i: true when going down */
	bool			bge_tx_sending;	/* i: true when tx inflight */

#ifdef BGE_EVENT_COUNTERS
	/*
	 * Event counters.
	 */
	struct evcnt bge_ev_intr;	/* i: interrupts */
	struct evcnt bge_ev_intr_spurious;
					/* i: spurious intr. (tagged status) */
	struct evcnt bge_ev_intr_spurious2; /* i: spurious interrupts */
	struct evcnt bge_ev_tx_xoff;	/* i: send PAUSE(len>0) packets */
	struct evcnt bge_ev_tx_xon;	/* i: send PAUSE(len=0) packets */
	struct evcnt bge_ev_rx_xoff;	/* i: receive PAUSE(len>0) packets */
	struct evcnt bge_ev_rx_xon;	/* i: receive PAUSE(len=0) packets */
	struct evcnt bge_ev_rx_macctl;	/* i: receive MAC control packets */
	struct evcnt bge_ev_xoffentered;/* i: XOFF state entered */
#endif /* BGE_EVENT_COUNTERS */
	uint64_t		bge_if_collisions;	/* i */
	int			bge_txcnt;	/* i: # tx descs in use */
	struct callout		bge_timeout;	/* i: tx timeout */
	bool			bge_pending_rxintr_change;
						/* i: change pending to
						 * rx_coal_ticks and
						 * rx_max_coal_bds */
	bool			bge_attached;
	bool			bge_detaching;	/* n */
	SLIST_HEAD(, txdmamap_pool_entry) txdma_list;		/* i */
	struct txdmamap_pool_entry *txdma[BGE_TX_RING_CNT];	/* i */

	struct sysctllog	*bge_log;

	krndsource_t	rnd_source;	/* random source */

	kmutex_t *sc_mcast_lock;	/* m: lock for SIOCADD/DELMULTI */
	kmutex_t *sc_intr_lock;		/* i: lock for interrupt operations */
	struct workqueue *sc_reset_wq;
	struct work sc_reset_work;	/* i */
	volatile unsigned sc_reset_pending;

	bool sc_trigger_reset;		/* i */
};

#endif /* _DEV_PCI_IF_BGEVAR_H_ */
