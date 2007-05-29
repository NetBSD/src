/*	$NetBSD: if_wm.c,v 1.141 2007/05/29 07:17:23 simonb Exp $	*/

/*
 * Copyright (c) 2001, 2002, 2003, 2004 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*******************************************************************************

  Copyright (c) 2001-2005, Intel Corporation 
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

*******************************************************************************/
/*
 * Device driver for the Intel i8254x family of Gigabit Ethernet chips.
 *
 * TODO (in order of importance):
 *
 *	- Rework how parameters are loaded from the EEPROM.
 *	- Figure out what to do with the i82545GM and i82546GB
 *	  SERDES controllers.
 *	- Fix hw VLAN assist.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_wm.c,v 1.141 2007/05/29 07:17:23 simonb Exp $");

#include "bpfilter.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/syslog.h>

#include <uvm/uvm_extern.h>		/* for PAGE_SIZE */

#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <netinet/in.h>			/* XXX for struct ip */
#include <netinet/in_systm.h>		/* XXX for struct ip */
#include <netinet/ip.h>			/* XXX for struct ip */
#include <netinet/ip6.h>		/* XXX for struct ip6_hdr */
#include <netinet/tcp.h>		/* XXX for struct tcphdr */

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/endian.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <dev/mii/mii_bitbang.h>
#include <dev/mii/ikphyreg.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/if_wmreg.h>

#ifdef WM_DEBUG
#define	WM_DEBUG_LINK		0x01
#define	WM_DEBUG_TX		0x02
#define	WM_DEBUG_RX		0x04
#define	WM_DEBUG_GMII		0x08
int	wm_debug = WM_DEBUG_TX|WM_DEBUG_RX|WM_DEBUG_LINK|WM_DEBUG_GMII;

#define	DPRINTF(x, y)	if (wm_debug & (x)) printf y
#else
#define	DPRINTF(x, y)	/* nothing */
#endif /* WM_DEBUG */

/*
 * Transmit descriptor list size.  Due to errata, we can only have
 * 256 hardware descriptors in the ring on < 82544, but we use 4096
 * on >= 82544.  We tell the upper layers that they can queue a lot
 * of packets, and we go ahead and manage up to 64 (16 for the i82547)
 * of them at a time.
 *
 * We allow up to 256 (!) DMA segments per packet.  Pathological packet
 * chains containing many small mbufs have been observed in zero-copy
 * situations with jumbo frames.
 */
#define	WM_NTXSEGS		256
#define	WM_IFQUEUELEN		256
#define	WM_TXQUEUELEN_MAX	64
#define	WM_TXQUEUELEN_MAX_82547	16
#define	WM_TXQUEUELEN(sc)	((sc)->sc_txnum)
#define	WM_TXQUEUELEN_MASK(sc)	(WM_TXQUEUELEN(sc) - 1)
#define	WM_TXQUEUE_GC(sc)	(WM_TXQUEUELEN(sc) / 8)
#define	WM_NTXDESC_82542	256
#define	WM_NTXDESC_82544	4096
#define	WM_NTXDESC(sc)		((sc)->sc_ntxdesc)
#define	WM_NTXDESC_MASK(sc)	(WM_NTXDESC(sc) - 1)
#define	WM_TXDESCSIZE(sc)	(WM_NTXDESC(sc) * sizeof(wiseman_txdesc_t))
#define	WM_NEXTTX(sc, x)	(((x) + 1) & WM_NTXDESC_MASK(sc))
#define	WM_NEXTTXS(sc, x)	(((x) + 1) & WM_TXQUEUELEN_MASK(sc))

#define	WM_MAXTXDMA		round_page(IP_MAXPACKET) /* for TSO */

/*
 * Receive descriptor list size.  We have one Rx buffer for normal
 * sized packets.  Jumbo packets consume 5 Rx buffers for a full-sized
 * packet.  We allocate 256 receive descriptors, each with a 2k
 * buffer (MCLBYTES), which gives us room for 50 jumbo packets.
 */
#define	WM_NRXDESC		256
#define	WM_NRXDESC_MASK		(WM_NRXDESC - 1)
#define	WM_NEXTRX(x)		(((x) + 1) & WM_NRXDESC_MASK)
#define	WM_PREVRX(x)		(((x) - 1) & WM_NRXDESC_MASK)

/*
 * Control structures are DMA'd to the i82542 chip.  We allocate them in
 * a single clump that maps to a single DMA segment to make several things
 * easier.
 */
struct wm_control_data_82544 {
	/*
	 * The receive descriptors.
	 */
	wiseman_rxdesc_t wcd_rxdescs[WM_NRXDESC];

	/*
	 * The transmit descriptors.  Put these at the end, because
	 * we might use a smaller number of them.
	 */
	wiseman_txdesc_t wcd_txdescs[WM_NTXDESC_82544];
};

struct wm_control_data_82542 {
	wiseman_rxdesc_t wcd_rxdescs[WM_NRXDESC];
	wiseman_txdesc_t wcd_txdescs[WM_NTXDESC_82542];
};

#define	WM_CDOFF(x)	offsetof(struct wm_control_data_82544, x)
#define	WM_CDTXOFF(x)	WM_CDOFF(wcd_txdescs[(x)])
#define	WM_CDRXOFF(x)	WM_CDOFF(wcd_rxdescs[(x)])

/*
 * Software state for transmit jobs.
 */
struct wm_txsoft {
	struct mbuf *txs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t txs_dmamap;	/* our DMA map */
	int txs_firstdesc;		/* first descriptor in packet */
	int txs_lastdesc;		/* last descriptor in packet */
	int txs_ndesc;			/* # of descriptors used */
};

/*
 * Software state for receive buffers.  Each descriptor gets a
 * 2k (MCLBYTES) buffer and a DMA map.  For packets which fill
 * more than one buffer, we chain them together.
 */
struct wm_rxsoft {
	struct mbuf *rxs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t rxs_dmamap;	/* our DMA map */
};

typedef enum {
	WM_T_unknown		= 0,
	WM_T_82542_2_0,			/* i82542 2.0 (really old) */
	WM_T_82542_2_1,			/* i82542 2.1+ (old) */
	WM_T_82543,			/* i82543 */
	WM_T_82544,			/* i82544 */
	WM_T_82540,			/* i82540 */
	WM_T_82545,			/* i82545 */
	WM_T_82545_3,			/* i82545 3.0+ */
	WM_T_82546,			/* i82546 */
	WM_T_82546_3,			/* i82546 3.0+ */
	WM_T_82541,			/* i82541 */
	WM_T_82541_2,			/* i82541 2.0+ */
	WM_T_82547,			/* i82547 */
	WM_T_82547_2,			/* i82547 2.0+ */
	WM_T_82571,			/* i82571 */
	WM_T_82572,			/* i82572 */
	WM_T_82573,			/* i82573 */
	WM_T_80003,			/* i80003 */
	WM_T_ICH8,			/* ICH8 LAN */
} wm_chip_type;

/*
 * Software state per device.
 */
struct wm_softc {
	struct device sc_dev;		/* generic device information */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	bus_space_tag_t sc_iot;		/* I/O space tag */
	bus_space_handle_t sc_ioh;	/* I/O space handle */
	bus_space_tag_t sc_flasht;	/* flash registers space tag */
	bus_space_handle_t sc_flashh;	/* flash registers space handle */
	bus_dma_tag_t sc_dmat;		/* bus DMA tag */
	struct ethercom sc_ethercom;	/* ethernet common data */
	void *sc_sdhook;		/* shutdown hook */
	void *sc_powerhook;		/* power hook */
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;
	struct pci_conf_state sc_pciconf;

	wm_chip_type sc_type;		/* chip type */
	int sc_flags;			/* flags; see below */
	int sc_bus_speed;		/* PCI/PCIX bus speed */
	int sc_pcix_offset;		/* PCIX capability register offset */
	int sc_flowflags;		/* 802.3x flow control flags */

	void *sc_ih;			/* interrupt cookie */

	int sc_ee_addrbits;		/* EEPROM address bits */

	struct mii_data sc_mii;		/* MII/media information */

	struct callout sc_tick_ch;	/* tick callout */

	bus_dmamap_t sc_cddmamap;	/* control data DMA map */
#define	sc_cddma	sc_cddmamap->dm_segs[0].ds_addr

	int		sc_align_tweak;

	/*
	 * Software state for the transmit and receive descriptors.
	 */
	int			sc_txnum;	/* must be a power of two */
	struct wm_txsoft	sc_txsoft[WM_TXQUEUELEN_MAX];
	struct wm_rxsoft	sc_rxsoft[WM_NRXDESC];

	/*
	 * Control data structures.
	 */
	int			sc_ntxdesc;	/* must be a power of two */
	struct wm_control_data_82544 *sc_control_data;
#define	sc_txdescs	sc_control_data->wcd_txdescs
#define	sc_rxdescs	sc_control_data->wcd_rxdescs

#ifdef WM_EVENT_COUNTERS
	/* Event counters. */
	struct evcnt sc_ev_txsstall;	/* Tx stalled due to no txs */
	struct evcnt sc_ev_txdstall;	/* Tx stalled due to no txd */
	struct evcnt sc_ev_txfifo_stall;/* Tx FIFO stalls (82547) */
	struct evcnt sc_ev_txdw;	/* Tx descriptor interrupts */
	struct evcnt sc_ev_txqe;	/* Tx queue empty interrupts */
	struct evcnt sc_ev_rxintr;	/* Rx interrupts */
	struct evcnt sc_ev_linkintr;	/* Link interrupts */

	struct evcnt sc_ev_rxipsum;	/* IP checksums checked in-bound */
	struct evcnt sc_ev_rxtusum;	/* TCP/UDP cksums checked in-bound */
	struct evcnt sc_ev_txipsum;	/* IP checksums comp. out-bound */
	struct evcnt sc_ev_txtusum;	/* TCP/UDP cksums comp. out-bound */
	struct evcnt sc_ev_txtusum6;	/* TCP/UDP v6 cksums comp. out-bound */
	struct evcnt sc_ev_txtso;	/* TCP seg offload out-bound (IPv4) */
	struct evcnt sc_ev_txtso6;	/* TCP seg offload out-bound (IPv6) */
	struct evcnt sc_ev_txtsopain;	/* painful header manip. for TSO */

	struct evcnt sc_ev_txseg[WM_NTXSEGS]; /* Tx packets w/ N segments */
	struct evcnt sc_ev_txdrop;	/* Tx packets dropped (too many segs) */

	struct evcnt sc_ev_tu;		/* Tx underrun */

	struct evcnt sc_ev_tx_xoff;	/* Tx PAUSE(!0) frames */
	struct evcnt sc_ev_tx_xon;	/* Tx PAUSE(0) frames */
	struct evcnt sc_ev_rx_xoff;	/* Rx PAUSE(!0) frames */
	struct evcnt sc_ev_rx_xon;	/* Rx PAUSE(0) frames */
	struct evcnt sc_ev_rx_macctl;	/* Rx Unsupported */
#endif /* WM_EVENT_COUNTERS */

	bus_addr_t sc_tdt_reg;		/* offset of TDT register */

	int	sc_txfree;		/* number of free Tx descriptors */
	int	sc_txnext;		/* next ready Tx descriptor */

	int	sc_txsfree;		/* number of free Tx jobs */
	int	sc_txsnext;		/* next free Tx job */
	int	sc_txsdirty;		/* dirty Tx jobs */

	/* These 5 variables are used only on the 82547. */
	int	sc_txfifo_size;		/* Tx FIFO size */
	int	sc_txfifo_head;		/* current head of FIFO */
	uint32_t sc_txfifo_addr;	/* internal address of start of FIFO */
	int	sc_txfifo_stall;	/* Tx FIFO is stalled */
	struct callout sc_txfifo_ch;	/* Tx FIFO stall work-around timer */

	bus_addr_t sc_rdt_reg;		/* offset of RDT register */

	int	sc_rxptr;		/* next ready Rx descriptor/queue ent */
	int	sc_rxdiscard;
	int	sc_rxlen;
	struct mbuf *sc_rxhead;
	struct mbuf *sc_rxtail;
	struct mbuf **sc_rxtailp;

	uint32_t sc_ctrl;		/* prototype CTRL register */
#if 0
	uint32_t sc_ctrl_ext;		/* prototype CTRL_EXT register */
#endif
	uint32_t sc_icr;		/* prototype interrupt bits */
	uint32_t sc_itr;		/* prototype intr throttling reg */
	uint32_t sc_tctl;		/* prototype TCTL register */
	uint32_t sc_rctl;		/* prototype RCTL register */
	uint32_t sc_txcw;		/* prototype TXCW register */
	uint32_t sc_tipg;		/* prototype TIPG register */
	uint32_t sc_fcrtl;		/* prototype FCRTL register */
	uint32_t sc_pba;		/* prototype PBA register */

	int sc_tbi_linkup;		/* TBI link status */
	int sc_tbi_anstate;		/* autonegotiation state */

	int sc_mchash_type;		/* multicast filter offset */

#if NRND > 0
	rndsource_element_t rnd_source;	/* random source */
#endif
	int sc_ich8_flash_base;
	int sc_ich8_flash_bank_size;
};

#define	WM_RXCHAIN_RESET(sc)						\
do {									\
	(sc)->sc_rxtailp = &(sc)->sc_rxhead;				\
	*(sc)->sc_rxtailp = NULL;					\
	(sc)->sc_rxlen = 0;						\
} while (/*CONSTCOND*/0)

#define	WM_RXCHAIN_LINK(sc, m)						\
do {									\
	*(sc)->sc_rxtailp = (sc)->sc_rxtail = (m);			\
	(sc)->sc_rxtailp = &(m)->m_next;				\
} while (/*CONSTCOND*/0)

/* sc_flags */
#define	WM_F_HAS_MII		0x0001	/* has MII */
#define	WM_F_EEPROM_HANDSHAKE	0x0002	/* requires EEPROM handshake */
#define	WM_F_EEPROM_SEMAPHORE	0x0004	/* EEPROM with semaphore */
#define	WM_F_EEPROM_EERDEEWR	0x0008	/* EEPROM access via EERD/EEWR */
#define	WM_F_EEPROM_SPI		0x0010	/* EEPROM is SPI */
#define	WM_F_EEPROM_FLASH	0x0020	/* EEPROM is FLASH */
#define	WM_F_EEPROM_INVALID	0x0040	/* EEPROM not present (bad checksum) */
#define	WM_F_IOH_VALID		0x0080	/* I/O handle is valid */
#define	WM_F_BUS64		0x0100	/* bus is 64-bit */
#define	WM_F_PCIX		0x0200	/* bus is PCI-X */
#define	WM_F_CSA		0x0400	/* bus is CSA */
#define	WM_F_PCIE		0x0800	/* bus is PCI-Express */
#define WM_F_SWFW_SYNC		0x1000  /* Software-Firmware synchronisation */
#define WM_F_SWFWHW_SYNC	0x2000  /* Software-Firmware synchronisation */

#ifdef WM_EVENT_COUNTERS
#define	WM_EVCNT_INCR(ev)	(ev)->ev_count++
#define	WM_EVCNT_ADD(ev, val)	(ev)->ev_count += (val)
#else
#define	WM_EVCNT_INCR(ev)	/* nothing */
#define	WM_EVCNT_ADD(ev, val)	/* nothing */
#endif

#define	CSR_READ(sc, reg)						\
	bus_space_read_4((sc)->sc_st, (sc)->sc_sh, (reg))
#define	CSR_WRITE(sc, reg, val)						\
	bus_space_write_4((sc)->sc_st, (sc)->sc_sh, (reg), (val))
#define	CSR_WRITE_FLUSH(sc)						\
	(void) CSR_READ((sc), WMREG_STATUS)

#define ICH8_FLASH_READ32(sc, reg) \
	bus_space_read_4((sc)->sc_flasht, (sc)->sc_flashh, (reg))
#define ICH8_FLASH_WRITE32(sc, reg, data) \
	bus_space_write_4((sc)->sc_flasht, (sc)->sc_flashh, (reg), (data))

#define ICH8_FLASH_READ16(sc, reg) \
	bus_space_read_2((sc)->sc_flasht, (sc)->sc_flashh, (reg))
#define ICH8_FLASH_WRITE16(sc, reg, data) \
	bus_space_write_2((sc)->sc_flasht, (sc)->sc_flashh, (reg), (data))

#define	WM_CDTXADDR(sc, x)	((sc)->sc_cddma + WM_CDTXOFF((x)))
#define	WM_CDRXADDR(sc, x)	((sc)->sc_cddma + WM_CDRXOFF((x)))

#define	WM_CDTXADDR_LO(sc, x)	(WM_CDTXADDR((sc), (x)) & 0xffffffffU)
#define	WM_CDTXADDR_HI(sc, x)						\
	(sizeof(bus_addr_t) == 8 ?					\
	 (uint64_t)WM_CDTXADDR((sc), (x)) >> 32 : 0)

#define	WM_CDRXADDR_LO(sc, x)	(WM_CDRXADDR((sc), (x)) & 0xffffffffU)
#define	WM_CDRXADDR_HI(sc, x)						\
	(sizeof(bus_addr_t) == 8 ?					\
	 (uint64_t)WM_CDRXADDR((sc), (x)) >> 32 : 0)

#define	WM_CDTXSYNC(sc, x, n, ops)					\
do {									\
	int __x, __n;							\
									\
	__x = (x);							\
	__n = (n);							\
									\
	/* If it will wrap around, sync to the end of the ring. */	\
	if ((__x + __n) > WM_NTXDESC(sc)) {				\
		bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,	\
		    WM_CDTXOFF(__x), sizeof(wiseman_txdesc_t) *		\
		    (WM_NTXDESC(sc) - __x), (ops));			\
		__n -= (WM_NTXDESC(sc) - __x);				\
		__x = 0;						\
	}								\
									\
	/* Now sync whatever is left. */				\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    WM_CDTXOFF(__x), sizeof(wiseman_txdesc_t) * __n, (ops));	\
} while (/*CONSTCOND*/0)

#define	WM_CDRXSYNC(sc, x, ops)						\
do {									\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	   WM_CDRXOFF((x)), sizeof(wiseman_rxdesc_t), (ops));		\
} while (/*CONSTCOND*/0)

#define	WM_INIT_RXDESC(sc, x)						\
do {									\
	struct wm_rxsoft *__rxs = &(sc)->sc_rxsoft[(x)];		\
	wiseman_rxdesc_t *__rxd = &(sc)->sc_rxdescs[(x)];		\
	struct mbuf *__m = __rxs->rxs_mbuf;				\
									\
	/*								\
	 * Note: We scoot the packet forward 2 bytes in the buffer	\
	 * so that the payload after the Ethernet header is aligned	\
	 * to a 4-byte boundary.					\
	 *								\
	 * XXX BRAINDAMAGE ALERT!					\
	 * The stupid chip uses the same size for every buffer, which	\
	 * is set in the Receive Control register.  We are using the 2K	\
	 * size option, but what we REALLY want is (2K - 2)!  For this	\
	 * reason, we can't "scoot" packets longer than the standard	\
	 * Ethernet MTU.  On strict-alignment platforms, if the total	\
	 * size exceeds (2K - 2) we set align_tweak to 0 and let	\
	 * the upper layer copy the headers.				\
	 */								\
	__m->m_data = __m->m_ext.ext_buf + (sc)->sc_align_tweak;	\
									\
	wm_set_dma_addr(&__rxd->wrx_addr,				\
	    __rxs->rxs_dmamap->dm_segs[0].ds_addr + (sc)->sc_align_tweak); \
	__rxd->wrx_len = 0;						\
	__rxd->wrx_cksum = 0;						\
	__rxd->wrx_status = 0;						\
	__rxd->wrx_errors = 0;						\
	__rxd->wrx_special = 0;						\
	WM_CDRXSYNC((sc), (x), BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE); \
									\
	CSR_WRITE((sc), (sc)->sc_rdt_reg, (x));				\
} while (/*CONSTCOND*/0)

static void	wm_start(struct ifnet *);
static void	wm_watchdog(struct ifnet *);
static int	wm_ioctl(struct ifnet *, u_long, void *);
static int	wm_init(struct ifnet *);
static void	wm_stop(struct ifnet *, int);

static void	wm_shutdown(void *);
static void	wm_powerhook(int, void *);

static void	wm_reset(struct wm_softc *);
static void	wm_rxdrain(struct wm_softc *);
static int	wm_add_rxbuf(struct wm_softc *, int);
static int	wm_read_eeprom(struct wm_softc *, int, int, u_int16_t *);
static int	wm_read_eeprom_eerd(struct wm_softc *, int, int, u_int16_t *);
static int	wm_validate_eeprom_checksum(struct wm_softc *);
static void	wm_tick(void *);

static void	wm_set_filter(struct wm_softc *);

static int	wm_intr(void *);
static void	wm_txintr(struct wm_softc *);
static void	wm_rxintr(struct wm_softc *);
static void	wm_linkintr(struct wm_softc *, uint32_t);

static void	wm_tbi_mediainit(struct wm_softc *);
static int	wm_tbi_mediachange(struct ifnet *);
static void	wm_tbi_mediastatus(struct ifnet *, struct ifmediareq *);

static void	wm_tbi_set_linkled(struct wm_softc *);
static void	wm_tbi_check_link(struct wm_softc *);

static void	wm_gmii_reset(struct wm_softc *);

static int	wm_gmii_i82543_readreg(struct device *, int, int);
static void	wm_gmii_i82543_writereg(struct device *, int, int, int);

static int	wm_gmii_i82544_readreg(struct device *, int, int);
static void	wm_gmii_i82544_writereg(struct device *, int, int, int);

static int	wm_gmii_i80003_readreg(struct device *, int, int);
static void	wm_gmii_i80003_writereg(struct device *, int, int, int);

static void	wm_gmii_statchg(struct device *);

static void	wm_gmii_mediainit(struct wm_softc *);
static int	wm_gmii_mediachange(struct ifnet *);
static void	wm_gmii_mediastatus(struct ifnet *, struct ifmediareq *);

static int	wm_kmrn_i80003_readreg(struct wm_softc *, int);
static void	wm_kmrn_i80003_writereg(struct wm_softc *, int, int);

static int	wm_match(struct device *, struct cfdata *, void *);
static void	wm_attach(struct device *, struct device *, void *);
static int	wm_is_onboard_nvm_eeprom(struct wm_softc *);
static int	wm_get_swsm_semaphore(struct wm_softc *);
static void	wm_put_swsm_semaphore(struct wm_softc *);
static int	wm_poll_eerd_eewr_done(struct wm_softc *, int);
static int	wm_get_swfw_semaphore(struct wm_softc *, uint16_t);
static void	wm_put_swfw_semaphore(struct wm_softc *, uint16_t);
static int	wm_get_swfwhw_semaphore(struct wm_softc *);
static void	wm_put_swfwhw_semaphore(struct wm_softc *);

static int	wm_read_eeprom_ich8(struct wm_softc *, int, int, uint16_t *);
static int32_t	wm_ich8_cycle_init(struct wm_softc *);
static int32_t	wm_ich8_flash_cycle(struct wm_softc *, uint32_t);
static int32_t	wm_read_ich8_data(struct wm_softc *, uint32_t,
                     uint32_t, uint16_t *);
static int32_t	wm_read_ich8_word(struct wm_softc *sc, uint32_t, uint16_t *);

CFATTACH_DECL(wm, sizeof(struct wm_softc),
    wm_match, wm_attach, NULL, NULL);

static void	wm_82547_txfifo_stall(void *);

/*
 * Devices supported by this driver.
 */
static const struct wm_product {
	pci_vendor_id_t		wmp_vendor;
	pci_product_id_t	wmp_product;
	const char		*wmp_name;
	wm_chip_type		wmp_type;
	int			wmp_flags;
#define	WMP_F_1000X		0x01
#define	WMP_F_1000T		0x02
} wm_products[] = {
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82542,
	  "Intel i82542 1000BASE-X Ethernet",
	  WM_T_82542_2_1,	WMP_F_1000X },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82543GC_FIBER,
	  "Intel i82543GC 1000BASE-X Ethernet",
	  WM_T_82543,		WMP_F_1000X },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82543GC_COPPER,
	  "Intel i82543GC 1000BASE-T Ethernet",
	  WM_T_82543,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82544EI_COPPER,
	  "Intel i82544EI 1000BASE-T Ethernet",
	  WM_T_82544,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82544EI_FIBER,
	  "Intel i82544EI 1000BASE-X Ethernet",
	  WM_T_82544,		WMP_F_1000X },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82544GC_COPPER,
	  "Intel i82544GC 1000BASE-T Ethernet",
	  WM_T_82544,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82544GC_LOM,
	  "Intel i82544GC (LOM) 1000BASE-T Ethernet",
	  WM_T_82544,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82540EM,
	  "Intel i82540EM 1000BASE-T Ethernet",
	  WM_T_82540,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82540EM_LOM,
	  "Intel i82540EM (LOM) 1000BASE-T Ethernet",
	  WM_T_82540,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82540EP_LOM,
	  "Intel i82540EP 1000BASE-T Ethernet",
	  WM_T_82540,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82540EP,
	  "Intel i82540EP 1000BASE-T Ethernet",
	  WM_T_82540,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82540EP_LP,
	  "Intel i82540EP 1000BASE-T Ethernet",
	  WM_T_82540,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82545EM_COPPER,
	  "Intel i82545EM 1000BASE-T Ethernet",
	  WM_T_82545,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82545GM_COPPER,
	  "Intel i82545GM 1000BASE-T Ethernet",
	  WM_T_82545_3,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82545GM_FIBER,
	  "Intel i82545GM 1000BASE-X Ethernet",
	  WM_T_82545_3,		WMP_F_1000X },
#if 0
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82545GM_SERDES,
	  "Intel i82545GM Gigabit Ethernet (SERDES)",
	  WM_T_82545_3,		WMP_F_SERDES },
#endif
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82546EB_COPPER,
	  "Intel i82546EB 1000BASE-T Ethernet",
	  WM_T_82546,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,     PCI_PRODUCT_INTEL_82546EB_QUAD,
	  "Intel i82546EB 1000BASE-T Ethernet",
	  WM_T_82546,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82545EM_FIBER,
	  "Intel i82545EM 1000BASE-X Ethernet",
	  WM_T_82545,		WMP_F_1000X },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82546EB_FIBER,
	  "Intel i82546EB 1000BASE-X Ethernet",
	  WM_T_82546,		WMP_F_1000X },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82546GB_COPPER,
	  "Intel i82546GB 1000BASE-T Ethernet",
	  WM_T_82546_3,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82546GB_FIBER,
	  "Intel i82546GB 1000BASE-X Ethernet",
	  WM_T_82546_3,		WMP_F_1000X },
#if 0
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82546GB_SERDES,
	  "Intel i82546GB Gigabit Ethernet (SERDES)",
	  WM_T_82546_3,		WMP_F_SERDES },
#endif
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82546GB_QUAD_COPPER,
	  "i82546GB quad-port Gigabit Ethernet",
	  WM_T_82546_3,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82546GB_QUAD_COPPER_KSP3,
	  "i82546GB quad-port Gigabit Ethernet (KSP3)",
	  WM_T_82546_3,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82546GB_PCIE,
	  "Intel PRO/1000MT (82546GB)",
	  WM_T_82546_3,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82541EI,
	  "Intel i82541EI 1000BASE-T Ethernet",
	  WM_T_82541,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82541ER_LOM,
	  "Intel i82541ER (LOM) 1000BASE-T Ethernet",
	  WM_T_82541,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82541EI_MOBILE,
	  "Intel i82541EI Mobile 1000BASE-T Ethernet",
	  WM_T_82541,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82541ER,
	  "Intel i82541ER 1000BASE-T Ethernet",
	  WM_T_82541_2,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82541GI,
	  "Intel i82541GI 1000BASE-T Ethernet",
	  WM_T_82541_2,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82541GI_MOBILE,
	  "Intel i82541GI Mobile 1000BASE-T Ethernet",
	  WM_T_82541_2,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82541PI,
	  "Intel i82541PI 1000BASE-T Ethernet",
	  WM_T_82541_2,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82547EI,
	  "Intel i82547EI 1000BASE-T Ethernet",
	  WM_T_82547,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82547EI_MOBILE,
	  "Intel i82547EI Mobile 1000BASE-T Ethernet",
	  WM_T_82547,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82547GI,
	  "Intel i82547GI 1000BASE-T Ethernet",
	  WM_T_82547_2,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82571EB_COPPER,
	  "Intel PRO/1000 PT (82571EB)",
	  WM_T_82571,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82571EB_FIBER,
	  "Intel PRO/1000 PF (82571EB)",
	  WM_T_82571,		WMP_F_1000X },
#if 0
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82571EB_SERDES,
	  "Intel PRO/1000 PB (82571EB)",
	  WM_T_82571,		WMP_F_SERDES },
#endif
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82571EB_QUAD_COPPER,
	  "Intel PRO/1000 QT (82571EB)",
	  WM_T_82571,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82572EI_COPPER,
	  "Intel i82572EI 1000baseT Ethernet",
	  WM_T_82572,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82572EI_FIBER,
	  "Intel i82572EI 1000baseX Ethernet",
	  WM_T_82572,		WMP_F_1000X },
#if 0
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82572EI_SERDES,
	  "Intel i82572EI Gigabit Ethernet (SERDES)",
	  WM_T_82572,		WMP_F_SERDES },
#endif

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82572EI,
	  "Intel i82572EI 1000baseT Ethernet",
	  WM_T_82572,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82573E,
	  "Intel i82573E",
	  WM_T_82573,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82573E_IAMT,
	  "Intel i82573E IAMT",
	  WM_T_82573,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82573L,
	  "Intel i82573L Gigabit Ethernet",
	  WM_T_82573,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_80K3LAN_CPR_DPT,
	  "i80003 dual 1000baseT Ethernet",
	  WM_T_80003,		WMP_F_1000T },

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_80K3LAN_FIB_DPT,
	  "i80003 dual 1000baseX Ethernet",
	  WM_T_80003,		WMP_F_1000T },
#if 0
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_80K3LAN_SDS_DPT,
	  "Intel i80003ES2 dual Gigabit Ethernet (SERDES)",
	  WM_T_80003,		WMP_F_SERDES },
#endif

	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_80K3LAN_CPR_SPT,
	  "Intel i80003 1000baseT Ethernet",
	  WM_T_80003,		WMP_F_1000T },
#if 0
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_80K3LAN_SDS_SPT,
	  "Intel i80003 Gigabit Ethernet (SERDES)",
	  WM_T_80003,		WMP_F_SERDES },
#endif
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82801H_M_AMT,
	  "Intel i82801H (M_AMT) LAN Controller",
	  WM_T_ICH8,		WMP_F_1000T },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82801H_AMT,
	  "Intel i82801H (AMT) LAN Controller",
	  WM_T_ICH8,		WMP_F_1000T },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82801H_LAN,
	  "Intel i82801H LAN Controller",
	  WM_T_ICH8,		WMP_F_1000T },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82801H_IFE_LAN,
	  "Intel i82801H (IFE) LAN Controller",
	  WM_T_ICH8,		WMP_F_1000T },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82801H_M_LAN,
	  "Intel i82801H (M) LAN Controller",
	  WM_T_ICH8,		WMP_F_1000T },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82801H_IFE_GT,
	  "Intel i82801H IFE (GT) LAN Controller",
	  WM_T_ICH8,		WMP_F_1000T },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82801H_IFE_G,
	  "Intel i82801H IFE (G) LAN Controller",
	  WM_T_ICH8,		WMP_F_1000T },

	{ 0,			0,
	  NULL,
	  0,			0 },
};

#ifdef WM_EVENT_COUNTERS
static char wm_txseg_evcnt_names[WM_NTXSEGS][sizeof("txsegXXX")];
#endif /* WM_EVENT_COUNTERS */

#if 0 /* Not currently used */
static inline uint32_t
wm_io_read(struct wm_softc *sc, int reg)
{

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, 0, reg);
	return (bus_space_read_4(sc->sc_iot, sc->sc_ioh, 4));
}
#endif

static inline void
wm_io_write(struct wm_softc *sc, int reg, uint32_t val)
{

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, 0, reg);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, 4, val);
}

static inline void
wm_set_dma_addr(volatile wiseman_addr_t *wa, bus_addr_t v)
{
	wa->wa_low = htole32(v & 0xffffffffU);
	if (sizeof(bus_addr_t) == 8)
		wa->wa_high = htole32((uint64_t) v >> 32);
	else
		wa->wa_high = 0;
}

static const struct wm_product *
wm_lookup(const struct pci_attach_args *pa)
{
	const struct wm_product *wmp;

	for (wmp = wm_products; wmp->wmp_name != NULL; wmp++) {
		if (PCI_VENDOR(pa->pa_id) == wmp->wmp_vendor &&
		    PCI_PRODUCT(pa->pa_id) == wmp->wmp_product)
			return (wmp);
	}
	return (NULL);
}

static int
wm_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (wm_lookup(pa) != NULL)
		return (1);

	return (0);
}

static void
wm_attach(struct device *parent, struct device *self, void *aux)
{
	struct wm_softc *sc = (void *) self;
	struct pci_attach_args *pa = aux;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	size_t cdata_size;
	const char *intrstr = NULL;
	const char *eetype;
	bus_space_tag_t memt;
	bus_space_handle_t memh;
	bus_dma_segment_t seg;
	int memh_valid;
	int i, rseg, error;
	const struct wm_product *wmp;
	prop_data_t ea;
	prop_number_t pn;
	uint8_t enaddr[ETHER_ADDR_LEN];
	uint16_t myea[ETHER_ADDR_LEN / 2], cfg1, cfg2, swdpin;
	pcireg_t preg, memtype;
	uint32_t reg;

	callout_init(&sc->sc_tick_ch);

	wmp = wm_lookup(pa);
	if (wmp == NULL) {
		printf("\n");
		panic("wm_attach: impossible");
	}

	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;

	if (pci_dma64_available(pa))
		sc->sc_dmat = pa->pa_dmat64;
	else
		sc->sc_dmat = pa->pa_dmat;

	preg = PCI_REVISION(pci_conf_read(pc, pa->pa_tag, PCI_CLASS_REG));
	aprint_naive(": Ethernet controller\n");
	aprint_normal(": %s, rev. %d\n", wmp->wmp_name, preg);

	sc->sc_type = wmp->wmp_type;
	if (sc->sc_type < WM_T_82543) {
		if (preg < 2) {
			aprint_error("%s: i82542 must be at least rev. 2\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		if (preg < 3)
			sc->sc_type = WM_T_82542_2_0;
	}

	/*
	 * Map the device.  All devices support memory-mapped acccess,
	 * and it is really required for normal operation.
	 */
	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, WM_PCI_MMBA);
	switch (memtype) {
	case PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT:
	case PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_64BIT:
		memh_valid = (pci_mapreg_map(pa, WM_PCI_MMBA,
		    memtype, 0, &memt, &memh, NULL, NULL) == 0);
		break;
	default:
		memh_valid = 0;
	}

	if (memh_valid) {
		sc->sc_st = memt;
		sc->sc_sh = memh;
	} else {
		aprint_error("%s: unable to map device registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * In addition, i82544 and later support I/O mapped indirect
	 * register access.  It is not desirable (nor supported in
	 * this driver) to use it for normal operation, though it is
	 * required to work around bugs in some chip versions.
	 */
	if (sc->sc_type >= WM_T_82544) {
		/* First we have to find the I/O BAR. */
		for (i = PCI_MAPREG_START; i < PCI_MAPREG_END; i += 4) {
			if (pci_mapreg_type(pa->pa_pc, pa->pa_tag, i) ==
			    PCI_MAPREG_TYPE_IO)
				break;
		}
		if (i == PCI_MAPREG_END)
			aprint_error("%s: WARNING: unable to find I/O BAR\n",
			    sc->sc_dev.dv_xname);
		else {
			/*
			 * The i8254x doesn't apparently respond when the
			 * I/O BAR is 0, which looks somewhat like it's not
			 * been configured.
			 */
			preg = pci_conf_read(pc, pa->pa_tag, i);
			if (PCI_MAPREG_MEM_ADDR(preg) == 0) {
				aprint_error("%s: WARNING: I/O BAR at zero.\n",
				    sc->sc_dev.dv_xname);
			} else if (pci_mapreg_map(pa, i, PCI_MAPREG_TYPE_IO,
					0, &sc->sc_iot, &sc->sc_ioh,
					NULL, NULL) == 0) {
				sc->sc_flags |= WM_F_IOH_VALID;
			} else {
				aprint_error("%s: WARNING: unable to map "
				    "I/O space\n", sc->sc_dev.dv_xname);
			}
		}

	}

	/* Enable bus mastering.  Disable MWI on the i82542 2.0. */
	preg = pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	preg |= PCI_COMMAND_MASTER_ENABLE;
	if (sc->sc_type < WM_T_82542_2_1)
		preg &= ~PCI_COMMAND_INVALIDATE_ENABLE;
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, preg);

	/* power up chip */
	if ((error = pci_activate(pa->pa_pc, pa->pa_tag, sc,
	    NULL)) && error != EOPNOTSUPP) {
		aprint_error("%s: cannot activate %d\n", sc->sc_dev.dv_xname,
		    error);
		return;
	}

	/*
	 * Map and establish our interrupt.
	 */
	if (pci_intr_map(pa, &ih)) {
		aprint_error("%s: unable to map interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, wm_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error("%s: unable to establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			aprint_normal(" at %s", intrstr);
		aprint_normal("\n");
		return;
	}
	aprint_normal("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	/*
	 * Determine a few things about the bus we're connected to.
	 */
	if (sc->sc_type < WM_T_82543) {
		/* We don't really know the bus characteristics here. */
		sc->sc_bus_speed = 33;
	} else if (sc->sc_type == WM_T_82547 || sc->sc_type == WM_T_82547_2) {
		/*
		 * CSA (Communication Streaming Architecture) is about as fast
		 * a 32-bit 66MHz PCI Bus.
		 */
		sc->sc_flags |= WM_F_CSA;
		sc->sc_bus_speed = 66;
		aprint_verbose("%s: Communication Streaming Architecture\n",
		    sc->sc_dev.dv_xname);
		if (sc->sc_type == WM_T_82547) {
			callout_init(&sc->sc_txfifo_ch);
			callout_setfunc(&sc->sc_txfifo_ch,
					wm_82547_txfifo_stall, sc);
			aprint_verbose("%s: using 82547 Tx FIFO stall "
				       "work-around\n", sc->sc_dev.dv_xname);
		}
	} else if (sc->sc_type >= WM_T_82571) {
		sc->sc_flags |= WM_F_PCIE;
		if (sc->sc_type != WM_T_ICH8)
			sc->sc_flags |= WM_F_EEPROM_SEMAPHORE;
		aprint_verbose("%s: PCI-Express bus\n", sc->sc_dev.dv_xname);
	} else {
		reg = CSR_READ(sc, WMREG_STATUS);
		if (reg & STATUS_BUS64)
			sc->sc_flags |= WM_F_BUS64;
		if (sc->sc_type >= WM_T_82544 &&
		    (reg & STATUS_PCIX_MODE) != 0) {
			pcireg_t pcix_cmd, pcix_sts, bytecnt, maxb;

			sc->sc_flags |= WM_F_PCIX;
			if (pci_get_capability(pa->pa_pc, pa->pa_tag,
					       PCI_CAP_PCIX,
					       &sc->sc_pcix_offset, NULL) == 0)
				aprint_error("%s: unable to find PCIX "
				    "capability\n", sc->sc_dev.dv_xname);
			else if (sc->sc_type != WM_T_82545_3 &&
				 sc->sc_type != WM_T_82546_3) {
				/*
				 * Work around a problem caused by the BIOS
				 * setting the max memory read byte count
				 * incorrectly.
				 */
				pcix_cmd = pci_conf_read(pa->pa_pc, pa->pa_tag,
				    sc->sc_pcix_offset + PCI_PCIX_CMD);
				pcix_sts = pci_conf_read(pa->pa_pc, pa->pa_tag,
				    sc->sc_pcix_offset + PCI_PCIX_STATUS);

				bytecnt =
				    (pcix_cmd & PCI_PCIX_CMD_BYTECNT_MASK) >>
				    PCI_PCIX_CMD_BYTECNT_SHIFT;
				maxb =
				    (pcix_sts & PCI_PCIX_STATUS_MAXB_MASK) >>
				    PCI_PCIX_STATUS_MAXB_SHIFT;
				if (bytecnt > maxb) {
					aprint_verbose("%s: resetting PCI-X "
					    "MMRBC: %d -> %d\n",
					    sc->sc_dev.dv_xname,
					    512 << bytecnt, 512 << maxb);
					pcix_cmd = (pcix_cmd &
					    ~PCI_PCIX_CMD_BYTECNT_MASK) |
					   (maxb << PCI_PCIX_CMD_BYTECNT_SHIFT);
					pci_conf_write(pa->pa_pc, pa->pa_tag,
					    sc->sc_pcix_offset + PCI_PCIX_CMD,
					    pcix_cmd);
				}
			}
		}
		/*
		 * The quad port adapter is special; it has a PCIX-PCIX
		 * bridge on the board, and can run the secondary bus at
		 * a higher speed.
		 */
		if (wmp->wmp_product == PCI_PRODUCT_INTEL_82546EB_QUAD) {
			sc->sc_bus_speed = (sc->sc_flags & WM_F_PCIX) ? 120
								      : 66;
		} else if (sc->sc_flags & WM_F_PCIX) {
			switch (reg & STATUS_PCIXSPD_MASK) {
			case STATUS_PCIXSPD_50_66:
				sc->sc_bus_speed = 66;
				break;
			case STATUS_PCIXSPD_66_100:
				sc->sc_bus_speed = 100;
				break;
			case STATUS_PCIXSPD_100_133:
				sc->sc_bus_speed = 133;
				break;
			default:
				aprint_error(
				    "%s: unknown PCIXSPD %d; assuming 66MHz\n",
				    sc->sc_dev.dv_xname,
				    reg & STATUS_PCIXSPD_MASK);
				sc->sc_bus_speed = 66;
			}
		} else
			sc->sc_bus_speed = (reg & STATUS_PCI66) ? 66 : 33;
		aprint_verbose("%s: %d-bit %dMHz %s bus\n", sc->sc_dev.dv_xname,
		    (sc->sc_flags & WM_F_BUS64) ? 64 : 32, sc->sc_bus_speed,
		    (sc->sc_flags & WM_F_PCIX) ? "PCIX" : "PCI");
	}

	/*
	 * Allocate the control data structures, and create and load the
	 * DMA map for it.
	 *
	 * NOTE: All Tx descriptors must be in the same 4G segment of
	 * memory.  So must Rx descriptors.  We simplify by allocating
	 * both sets within the same 4G segment.
	 */
	WM_NTXDESC(sc) = sc->sc_type < WM_T_82544 ?
	    WM_NTXDESC_82542 : WM_NTXDESC_82544;
	cdata_size = sc->sc_type < WM_T_82544 ?
	    sizeof(struct wm_control_data_82542) :
	    sizeof(struct wm_control_data_82544);
	if ((error = bus_dmamem_alloc(sc->sc_dmat, cdata_size, PAGE_SIZE,
				      (bus_size_t) 0x100000000ULL,
				      &seg, 1, &rseg, 0)) != 0) {
		aprint_error(
		    "%s: unable to allocate control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_0;
	}

	if ((error = bus_dmamem_map(sc->sc_dmat, &seg, rseg, cdata_size,
				    (void **)&sc->sc_control_data, 0)) != 0) {
		aprint_error("%s: unable to map control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_1;
	}

	if ((error = bus_dmamap_create(sc->sc_dmat, cdata_size, 1, cdata_size,
				       0, 0, &sc->sc_cddmamap)) != 0) {
		aprint_error("%s: unable to create control data DMA map, "
		    "error = %d\n", sc->sc_dev.dv_xname, error);
		goto fail_2;
	}

	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_cddmamap,
				     sc->sc_control_data, cdata_size, NULL,
				     0)) != 0) {
		aprint_error(
		    "%s: unable to load control data DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_3;
	}


	/*
	 * Create the transmit buffer DMA maps.
	 */
	WM_TXQUEUELEN(sc) =
	    (sc->sc_type == WM_T_82547 || sc->sc_type == WM_T_82547_2) ?
	    WM_TXQUEUELEN_MAX_82547 : WM_TXQUEUELEN_MAX;
	for (i = 0; i < WM_TXQUEUELEN(sc); i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, WM_MAXTXDMA,
					       WM_NTXSEGS, WTX_MAX_LEN, 0, 0,
					  &sc->sc_txsoft[i].txs_dmamap)) != 0) {
			aprint_error("%s: unable to create Tx DMA map %d, "
			    "error = %d\n", sc->sc_dev.dv_xname, i, error);
			goto fail_4;
		}
	}

	/*
	 * Create the receive buffer DMA maps.
	 */
	for (i = 0; i < WM_NRXDESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
					       MCLBYTES, 0, 0,
					  &sc->sc_rxsoft[i].rxs_dmamap)) != 0) {
			aprint_error("%s: unable to create Rx DMA map %d, "
			    "error = %d\n", sc->sc_dev.dv_xname, i, error);
			goto fail_5;
		}
		sc->sc_rxsoft[i].rxs_mbuf = NULL;
	}

	/* clear interesting stat counters */
	CSR_READ(sc, WMREG_COLC);
	CSR_READ(sc, WMREG_RXERRC);

	/*
	 * Reset the chip to a known state.
	 */
	wm_reset(sc);

	/*
	 * Get some information about the EEPROM.
	 */
	if (sc->sc_type == WM_T_ICH8) {
		uint32_t flash_size;
		sc->sc_flags |= WM_F_SWFWHW_SYNC | WM_F_EEPROM_FLASH;
		memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, WM_ICH8_FLASH);
		if (pci_mapreg_map(pa, WM_ICH8_FLASH, memtype, 0,
		    &sc->sc_flasht, &sc->sc_flashh, NULL, NULL)) {
			printf("%s: can't map FLASH registers\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		flash_size = ICH8_FLASH_READ32(sc, ICH_FLASH_GFPREG);
		sc->sc_ich8_flash_base = (flash_size & ICH_GFPREG_BASE_MASK) *
						ICH_FLASH_SECTOR_SIZE;
		sc->sc_ich8_flash_bank_size = 
			((flash_size >> 16) & ICH_GFPREG_BASE_MASK) + 1;
		sc->sc_ich8_flash_bank_size -=
			(flash_size & ICH_GFPREG_BASE_MASK);
		sc->sc_ich8_flash_bank_size *= ICH_FLASH_SECTOR_SIZE;
		sc->sc_ich8_flash_bank_size /= 2 * sizeof(uint16_t);
	} else if (sc->sc_type == WM_T_80003)
		sc->sc_flags |= WM_F_EEPROM_EERDEEWR |  WM_F_SWFW_SYNC;
	else if (sc->sc_type == WM_T_82573)
		sc->sc_flags |= WM_F_EEPROM_EERDEEWR;
	else if (sc->sc_type > WM_T_82544)
		sc->sc_flags |= WM_F_EEPROM_HANDSHAKE;

	if (sc->sc_type <= WM_T_82544)
		sc->sc_ee_addrbits = 6;
	else if (sc->sc_type <= WM_T_82546_3) {
		reg = CSR_READ(sc, WMREG_EECD);
		if (reg & EECD_EE_SIZE)
			sc->sc_ee_addrbits = 8;
		else
			sc->sc_ee_addrbits = 6;
	} else if (sc->sc_type <= WM_T_82547_2) {
		reg = CSR_READ(sc, WMREG_EECD);
		if (reg & EECD_EE_TYPE) {
			sc->sc_flags |= WM_F_EEPROM_SPI;
			sc->sc_ee_addrbits = (reg & EECD_EE_ABITS) ? 16 : 8;
		} else
			sc->sc_ee_addrbits = (reg & EECD_EE_ABITS) ? 8 : 6;
	} else if ((sc->sc_type == WM_T_82573) &&
	    (wm_is_onboard_nvm_eeprom(sc) == 0)) {
		sc->sc_flags |= WM_F_EEPROM_FLASH;
	} else {
		/* Assume everything else is SPI. */
		reg = CSR_READ(sc, WMREG_EECD);
		sc->sc_flags |= WM_F_EEPROM_SPI;
		sc->sc_ee_addrbits = (reg & EECD_EE_ABITS) ? 16 : 8;
	}

	/*
	 * Defer printing the EEPROM type until after verifying the checksum
	 * This allows the EEPROM type to be printed correctly in the case
	 * that no EEPROM is attached.
	 */

 
	/*
	 * Validate the EEPROM checksum. If the checksum fails, flag this for
	 * later, so we can fail future reads from the EEPROM.
	 */
	if (wm_validate_eeprom_checksum(sc))
		sc->sc_flags |= WM_F_EEPROM_INVALID;

	if (sc->sc_flags & WM_F_EEPROM_INVALID)
		aprint_verbose("%s: No EEPROM\n", sc->sc_dev.dv_xname);
	else if (sc->sc_flags & WM_F_EEPROM_FLASH) {
		aprint_verbose("%s: FLASH\n", sc->sc_dev.dv_xname);
	} else {
		if (sc->sc_flags & WM_F_EEPROM_SPI)
			eetype = "SPI";
		else
			eetype = "MicroWire";
		aprint_verbose("%s: %u word (%d address bits) %s EEPROM\n",
		    sc->sc_dev.dv_xname, 1U << sc->sc_ee_addrbits,
		    sc->sc_ee_addrbits, eetype);
	}

	/*
	 * Read the Ethernet address from the EEPROM, if not first found
	 * in device properties.
	 */
	ea = prop_dictionary_get(device_properties(&sc->sc_dev), "mac-addr");
	if (ea != NULL) {
		KASSERT(prop_object_type(ea) == PROP_TYPE_DATA);
		KASSERT(prop_data_size(ea) == ETHER_ADDR_LEN);
		memcpy(enaddr, prop_data_data_nocopy(ea), ETHER_ADDR_LEN);
	} else {
		if (wm_read_eeprom(sc, EEPROM_OFF_MACADDR,
		    sizeof(myea) / sizeof(myea[0]), myea)) {
			aprint_error("%s: unable to read Ethernet address\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		enaddr[0] = myea[0] & 0xff;
		enaddr[1] = myea[0] >> 8;
		enaddr[2] = myea[1] & 0xff;
		enaddr[3] = myea[1] >> 8;
		enaddr[4] = myea[2] & 0xff;
		enaddr[5] = myea[2] >> 8;
	}

	/*
	 * Toggle the LSB of the MAC address on the second port
	 * of the dual port controller.
	 */
	if (sc->sc_type == WM_T_82546 || sc->sc_type == WM_T_82546_3
	    || sc->sc_type ==  WM_T_82571 || sc->sc_type == WM_T_80003) {
		if ((CSR_READ(sc, WMREG_STATUS) >> STATUS_FUNCID_SHIFT) & 1)
			enaddr[5] ^= 1;
	}

	aprint_normal("%s: Ethernet address %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(enaddr));

	/*
	 * Read the config info from the EEPROM, and set up various
	 * bits in the control registers based on their contents.
	 */
	pn = prop_dictionary_get(device_properties(&sc->sc_dev),
				 "i82543-cfg1");
	if (pn != NULL) {
		KASSERT(prop_object_type(pn) == PROP_TYPE_NUMBER);
		cfg1 = (uint16_t) prop_number_integer_value(pn);
	} else {
		if (wm_read_eeprom(sc, EEPROM_OFF_CFG1, 1, &cfg1)) {
			aprint_error("%s: unable to read CFG1\n",
			    sc->sc_dev.dv_xname);
			return;
		}
	}

	pn = prop_dictionary_get(device_properties(&sc->sc_dev),
				 "i82543-cfg2");
	if (pn != NULL) {
		KASSERT(prop_object_type(pn) == PROP_TYPE_NUMBER);
		cfg2 = (uint16_t) prop_number_integer_value(pn);
	} else {
		if (wm_read_eeprom(sc, EEPROM_OFF_CFG2, 1, &cfg2)) {
			aprint_error("%s: unable to read CFG2\n",
			    sc->sc_dev.dv_xname);
			return;
		}
	}

	if (sc->sc_type >= WM_T_82544) {
		pn = prop_dictionary_get(device_properties(&sc->sc_dev),
					 "i82543-swdpin");
		if (pn != NULL) {
			KASSERT(prop_object_type(pn) == PROP_TYPE_NUMBER);
			swdpin = (uint16_t) prop_number_integer_value(pn);
		} else {
			if (wm_read_eeprom(sc, EEPROM_OFF_SWDPIN, 1, &swdpin)) {
				aprint_error("%s: unable to read SWDPIN\n",
				    sc->sc_dev.dv_xname);
				return;
			}
		}
	}

	if (cfg1 & EEPROM_CFG1_ILOS)
		sc->sc_ctrl |= CTRL_ILOS;
	if (sc->sc_type >= WM_T_82544) {
		sc->sc_ctrl |=
		    ((swdpin >> EEPROM_SWDPIN_SWDPIO_SHIFT) & 0xf) <<
		    CTRL_SWDPIO_SHIFT;
		sc->sc_ctrl |=
		    ((swdpin >> EEPROM_SWDPIN_SWDPIN_SHIFT) & 0xf) <<
		    CTRL_SWDPINS_SHIFT;
	} else {
		sc->sc_ctrl |=
		    ((cfg1 >> EEPROM_CFG1_SWDPIO_SHIFT) & 0xf) <<
		    CTRL_SWDPIO_SHIFT;
	}

#if 0
	if (sc->sc_type >= WM_T_82544) {
		if (cfg1 & EEPROM_CFG1_IPS0)
			sc->sc_ctrl_ext |= CTRL_EXT_IPS;
		if (cfg1 & EEPROM_CFG1_IPS1)
			sc->sc_ctrl_ext |= CTRL_EXT_IPS1;
		sc->sc_ctrl_ext |=
		    ((swdpin >> (EEPROM_SWDPIN_SWDPIO_SHIFT + 4)) & 0xd) <<
		    CTRL_EXT_SWDPIO_SHIFT;
		sc->sc_ctrl_ext |=
		    ((swdpin >> (EEPROM_SWDPIN_SWDPIN_SHIFT + 4)) & 0xd) <<
		    CTRL_EXT_SWDPINS_SHIFT;
	} else {
		sc->sc_ctrl_ext |=
		    ((cfg2 >> EEPROM_CFG2_SWDPIO_SHIFT) & 0xf) <<
		    CTRL_EXT_SWDPIO_SHIFT;
	}
#endif

	CSR_WRITE(sc, WMREG_CTRL, sc->sc_ctrl);
#if 0
	CSR_WRITE(sc, WMREG_CTRL_EXT, sc->sc_ctrl_ext);
#endif

	/*
	 * Set up some register offsets that are different between
	 * the i82542 and the i82543 and later chips.
	 */
	if (sc->sc_type < WM_T_82543) {
		sc->sc_rdt_reg = WMREG_OLD_RDT0;
		sc->sc_tdt_reg = WMREG_OLD_TDT;
	} else {
		sc->sc_rdt_reg = WMREG_RDT;
		sc->sc_tdt_reg = WMREG_TDT;
	}

	/*
	 * Determine if we're TBI or GMII mode, and initialize the
	 * media structures accordingly.
	 */
	if (sc->sc_type == WM_T_ICH8 || sc->sc_type == WM_T_82573) {
		/* STATUS_TBIMODE reserved/reused, can't rely on it */
		wm_gmii_mediainit(sc);
	} else if (sc->sc_type < WM_T_82543 ||
	    (CSR_READ(sc, WMREG_STATUS) & STATUS_TBIMODE) != 0) {
		if (wmp->wmp_flags & WMP_F_1000T)
			aprint_error("%s: WARNING: TBIMODE set on 1000BASE-T "
			    "product!\n", sc->sc_dev.dv_xname);
		wm_tbi_mediainit(sc);
	} else {
		if (wmp->wmp_flags & WMP_F_1000X)
			aprint_error("%s: WARNING: TBIMODE clear on 1000BASE-X "
			    "product!\n", sc->sc_dev.dv_xname);
		wm_gmii_mediainit(sc);
	}

	ifp = &sc->sc_ethercom.ec_if;
	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = wm_ioctl;
	ifp->if_start = wm_start;
	ifp->if_watchdog = wm_watchdog;
	ifp->if_init = wm_init;
	ifp->if_stop = wm_stop;
	IFQ_SET_MAXLEN(&ifp->if_snd, max(WM_IFQUEUELEN, IFQ_MAXLEN));
	IFQ_SET_READY(&ifp->if_snd);

	if (sc->sc_type != WM_T_82573 && sc->sc_type != WM_T_ICH8)
		sc->sc_ethercom.ec_capabilities |= ETHERCAP_JUMBO_MTU;

	/*
	 * If we're a i82543 or greater, we can support VLANs.
	 */
	if (sc->sc_type >= WM_T_82543)
		sc->sc_ethercom.ec_capabilities |=
		    ETHERCAP_VLAN_MTU /* XXXJRT | ETHERCAP_VLAN_HWTAGGING */;

	/*
	 * We can perform TCPv4 and UDPv4 checkums in-bound.  Only
	 * on i82543 and later.
	 */
	if (sc->sc_type >= WM_T_82543) {
		ifp->if_capabilities |=
		    IFCAP_CSUM_IPv4_Tx | IFCAP_CSUM_IPv4_Rx |
		    IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_TCPv4_Rx |
		    IFCAP_CSUM_UDPv4_Tx | IFCAP_CSUM_UDPv4_Rx |
		    IFCAP_CSUM_TCPv6_Tx |
		    IFCAP_CSUM_UDPv6_Tx;
	}

	/*
	 * XXXyamt: i'm not sure which chips support RXCSUM_IPV6OFL.
	 *
	 *	82541GI (8086:1076) ... no
	 *	82572EI (8086:10b9) ... yes
	 */
	if (sc->sc_type >= WM_T_82571) {
		ifp->if_capabilities |=
		    IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_UDPv6_Rx;
	}

	/* 
	 * If we're a i82544 or greater (except i82547), we can do
	 * TCP segmentation offload.
	 */
	if (sc->sc_type >= WM_T_82544 && sc->sc_type != WM_T_82547) {
		ifp->if_capabilities |= IFCAP_TSOv4;
	}

	if (sc->sc_type >= WM_T_82571) {
		ifp->if_capabilities |= IFCAP_TSOv6;
	}

	/*
	 * Attach the interface.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, enaddr);
#if NRND > 0
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
	    RND_TYPE_NET, 0);
#endif

#ifdef WM_EVENT_COUNTERS
	/* Attach event counters. */
	evcnt_attach_dynamic(&sc->sc_ev_txsstall, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txsstall");
	evcnt_attach_dynamic(&sc->sc_ev_txdstall, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txdstall");
	evcnt_attach_dynamic(&sc->sc_ev_txfifo_stall, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txfifo_stall");
	evcnt_attach_dynamic(&sc->sc_ev_txdw, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "txdw");
	evcnt_attach_dynamic(&sc->sc_ev_txqe, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "txqe");
	evcnt_attach_dynamic(&sc->sc_ev_rxintr, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "rxintr");
	evcnt_attach_dynamic(&sc->sc_ev_linkintr, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "linkintr");

	evcnt_attach_dynamic(&sc->sc_ev_rxipsum, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "rxipsum");
	evcnt_attach_dynamic(&sc->sc_ev_rxtusum, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "rxtusum");
	evcnt_attach_dynamic(&sc->sc_ev_txipsum, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txipsum");
	evcnt_attach_dynamic(&sc->sc_ev_txtusum, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txtusum");
	evcnt_attach_dynamic(&sc->sc_ev_txtusum6, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txtusum6");

	evcnt_attach_dynamic(&sc->sc_ev_txtso, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txtso");
	evcnt_attach_dynamic(&sc->sc_ev_txtso6, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txtso6");
	evcnt_attach_dynamic(&sc->sc_ev_txtsopain, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txtsopain");

	for (i = 0; i < WM_NTXSEGS; i++) {
		sprintf(wm_txseg_evcnt_names[i], "txseg%d", i);
		evcnt_attach_dynamic(&sc->sc_ev_txseg[i], EVCNT_TYPE_MISC,
		    NULL, sc->sc_dev.dv_xname, wm_txseg_evcnt_names[i]);
	}

	evcnt_attach_dynamic(&sc->sc_ev_txdrop, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txdrop");

	evcnt_attach_dynamic(&sc->sc_ev_tu, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "tu");

	evcnt_attach_dynamic(&sc->sc_ev_tx_xoff, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "tx_xoff");
	evcnt_attach_dynamic(&sc->sc_ev_tx_xon, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "tx_xon");
	evcnt_attach_dynamic(&sc->sc_ev_rx_xoff, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "rx_xoff");
	evcnt_attach_dynamic(&sc->sc_ev_rx_xon, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "rx_xon");
	evcnt_attach_dynamic(&sc->sc_ev_rx_macctl, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "rx_macctl");
#endif /* WM_EVENT_COUNTERS */

	/*
	 * Make sure the interface is shutdown during reboot.
	 */
	sc->sc_sdhook = shutdownhook_establish(wm_shutdown, sc);
	if (sc->sc_sdhook == NULL)
		aprint_error("%s: WARNING: unable to establish shutdown hook\n",
		    sc->sc_dev.dv_xname);

	sc->sc_powerhook = powerhook_establish(sc->sc_dev.dv_xname,
	    wm_powerhook, sc);
	if (sc->sc_powerhook == NULL)
		aprint_error("%s: can't establish powerhook\n",
		    sc->sc_dev.dv_xname);
	return;

	/*
	 * Free any resources we've allocated during the failed attach
	 * attempt.  Do this in reverse order and fall through.
	 */
 fail_5:
	for (i = 0; i < WM_NRXDESC; i++) {
		if (sc->sc_rxsoft[i].rxs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_rxsoft[i].rxs_dmamap);
	}
 fail_4:
	for (i = 0; i < WM_TXQUEUELEN(sc); i++) {
		if (sc->sc_txsoft[i].txs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_txsoft[i].txs_dmamap);
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_cddmamap);
 fail_3:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_cddmamap);
 fail_2:
	bus_dmamem_unmap(sc->sc_dmat, (void *)sc->sc_control_data,
	    cdata_size);
 fail_1:
	bus_dmamem_free(sc->sc_dmat, &seg, rseg);
 fail_0:
	return;
}

/*
 * wm_shutdown:
 *
 *	Make sure the interface is stopped at reboot time.
 */
static void
wm_shutdown(void *arg)
{
	struct wm_softc *sc = arg;

	wm_stop(&sc->sc_ethercom.ec_if, 1);
}

static void
wm_powerhook(int why, void *arg)
{
	struct wm_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	pci_chipset_tag_t pc = sc->sc_pc;
	pcitag_t tag = sc->sc_pcitag;

	switch (why) {
	case PWR_SOFTSUSPEND:
		wm_shutdown(sc);
		break;
	case PWR_SOFTRESUME:
		ifp->if_flags &= ~IFF_RUNNING;
		wm_init(ifp);
		if (ifp->if_flags & IFF_RUNNING)
			wm_start(ifp);
		break;
	case PWR_SUSPEND:
		pci_conf_capture(pc, tag, &sc->sc_pciconf);
		break;
	case PWR_RESUME:
		pci_conf_restore(pc, tag, &sc->sc_pciconf);
		break;
	}

	return;
}

/*
 * wm_tx_offload:
 *
 *	Set up TCP/IP checksumming parameters for the
 *	specified packet.
 */
static int
wm_tx_offload(struct wm_softc *sc, struct wm_txsoft *txs, uint32_t *cmdp,
    uint8_t *fieldsp)
{
	struct mbuf *m0 = txs->txs_mbuf;
	struct livengood_tcpip_ctxdesc *t;
	uint32_t ipcs, tucs, cmd, cmdlen, seg;
	uint32_t ipcse;
	struct ether_header *eh;
	int offset, iphl;
	uint8_t fields;

	/*
	 * XXX It would be nice if the mbuf pkthdr had offset
	 * fields for the protocol headers.
	 */

	eh = mtod(m0, struct ether_header *);
	switch (htons(eh->ether_type)) {
	case ETHERTYPE_IP:
	case ETHERTYPE_IPV6:
		offset = ETHER_HDR_LEN;
		break;

	case ETHERTYPE_VLAN:
		offset = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
		break;

	default:
		/*
		 * Don't support this protocol or encapsulation.
		 */
		*fieldsp = 0;
		*cmdp = 0;
		return (0);
	}

	if ((m0->m_pkthdr.csum_flags &
	    (M_CSUM_TSOv4|M_CSUM_UDPv4|M_CSUM_TCPv4)) != 0) {
		iphl = M_CSUM_DATA_IPv4_IPHL(m0->m_pkthdr.csum_data);
	} else {
		iphl = M_CSUM_DATA_IPv6_HL(m0->m_pkthdr.csum_data);
	}
	ipcse = offset + iphl - 1;

	cmd = WTX_CMD_DEXT | WTX_DTYP_D;
	cmdlen = WTX_CMD_DEXT | WTX_DTYP_C | WTX_CMD_IDE;
	seg = 0;
	fields = 0;

	if ((m0->m_pkthdr.csum_flags & (M_CSUM_TSOv4 | M_CSUM_TSOv6)) != 0) {
		int hlen = offset + iphl;
		bool v4 = (m0->m_pkthdr.csum_flags & M_CSUM_TSOv4) != 0;

		if (__predict_false(m0->m_len <
				    (hlen + sizeof(struct tcphdr)))) {
			/*
			 * TCP/IP headers are not in the first mbuf; we need
			 * to do this the slow and painful way.  Let's just
			 * hope this doesn't happen very often.
			 */
			struct tcphdr th;

			WM_EVCNT_INCR(&sc->sc_ev_txtsopain);

			m_copydata(m0, hlen, sizeof(th), &th);
			if (v4) {
				struct ip ip;

				m_copydata(m0, offset, sizeof(ip), &ip);
				ip.ip_len = 0;
				m_copyback(m0,
				    offset + offsetof(struct ip, ip_len),
				    sizeof(ip.ip_len), &ip.ip_len);
				th.th_sum = in_cksum_phdr(ip.ip_src.s_addr,
				    ip.ip_dst.s_addr, htons(IPPROTO_TCP));
			} else {
				struct ip6_hdr ip6;

				m_copydata(m0, offset, sizeof(ip6), &ip6);
				ip6.ip6_plen = 0;
				m_copyback(m0,
				    offset + offsetof(struct ip6_hdr, ip6_plen),
				    sizeof(ip6.ip6_plen), &ip6.ip6_plen);
				th.th_sum = in6_cksum_phdr(&ip6.ip6_src,
				    &ip6.ip6_dst, 0, htonl(IPPROTO_TCP));
			}
			m_copyback(m0, hlen + offsetof(struct tcphdr, th_sum),
			    sizeof(th.th_sum), &th.th_sum);

			hlen += th.th_off << 2;
		} else {
			/*
			 * TCP/IP headers are in the first mbuf; we can do
			 * this the easy way.
			 */
			struct tcphdr *th;

			if (v4) {
				struct ip *ip =
				    (void *)(mtod(m0, char *) + offset);
				th = (void *)(mtod(m0, char *) + hlen);

				ip->ip_len = 0;
				th->th_sum = in_cksum_phdr(ip->ip_src.s_addr,
				    ip->ip_dst.s_addr, htons(IPPROTO_TCP));
			} else {
				struct ip6_hdr *ip6 =
				    (void *)(mtod(m0, char *) + offset);
				th = (void *)(mtod(m0, char *) + hlen);

				ip6->ip6_plen = 0;
				th->th_sum = in6_cksum_phdr(&ip6->ip6_src,
				    &ip6->ip6_dst, 0, htonl(IPPROTO_TCP));
			}
			hlen += th->th_off << 2;
		}

		if (v4) {
			WM_EVCNT_INCR(&sc->sc_ev_txtso);
			cmdlen |= WTX_TCPIP_CMD_IP;
		} else {
			WM_EVCNT_INCR(&sc->sc_ev_txtso6);
			ipcse = 0;
		}
		cmd |= WTX_TCPIP_CMD_TSE;
		cmdlen |= WTX_TCPIP_CMD_TSE |
		    WTX_TCPIP_CMD_TCP | (m0->m_pkthdr.len - hlen);
		seg = WTX_TCPIP_SEG_HDRLEN(hlen) |
		    WTX_TCPIP_SEG_MSS(m0->m_pkthdr.segsz);
	}

	/*
	 * NOTE: Even if we're not using the IP or TCP/UDP checksum
	 * offload feature, if we load the context descriptor, we
	 * MUST provide valid values for IPCSS and TUCSS fields.
	 */

	ipcs = WTX_TCPIP_IPCSS(offset) |
	    WTX_TCPIP_IPCSO(offset + offsetof(struct ip, ip_sum)) |
	    WTX_TCPIP_IPCSE(ipcse);
	if (m0->m_pkthdr.csum_flags & (M_CSUM_IPv4|M_CSUM_TSOv4)) {
		WM_EVCNT_INCR(&sc->sc_ev_txipsum);
		fields |= WTX_IXSM;
	}

	offset += iphl;

	if (m0->m_pkthdr.csum_flags &
	    (M_CSUM_TCPv4|M_CSUM_UDPv4|M_CSUM_TSOv4)) {
		WM_EVCNT_INCR(&sc->sc_ev_txtusum);
		fields |= WTX_TXSM;
		tucs = WTX_TCPIP_TUCSS(offset) |
		    WTX_TCPIP_TUCSO(offset +
		    M_CSUM_DATA_IPv4_OFFSET(m0->m_pkthdr.csum_data)) |
		    WTX_TCPIP_TUCSE(0) /* rest of packet */;
	} else if ((m0->m_pkthdr.csum_flags &
	    (M_CSUM_TCPv6|M_CSUM_UDPv6|M_CSUM_TSOv6)) != 0) {
		WM_EVCNT_INCR(&sc->sc_ev_txtusum6);
		fields |= WTX_TXSM;
		tucs = WTX_TCPIP_TUCSS(offset) |
		    WTX_TCPIP_TUCSO(offset +
		    M_CSUM_DATA_IPv6_OFFSET(m0->m_pkthdr.csum_data)) |
		    WTX_TCPIP_TUCSE(0) /* rest of packet */;
	} else {
		/* Just initialize it to a valid TCP context. */
		tucs = WTX_TCPIP_TUCSS(offset) |
		    WTX_TCPIP_TUCSO(offset + offsetof(struct tcphdr, th_sum)) |
		    WTX_TCPIP_TUCSE(0) /* rest of packet */;
	}

	/* Fill in the context descriptor. */
	t = (struct livengood_tcpip_ctxdesc *)
	    &sc->sc_txdescs[sc->sc_txnext];
	t->tcpip_ipcs = htole32(ipcs);
	t->tcpip_tucs = htole32(tucs);
	t->tcpip_cmdlen = htole32(cmdlen);
	t->tcpip_seg = htole32(seg);
	WM_CDTXSYNC(sc, sc->sc_txnext, 1, BUS_DMASYNC_PREWRITE);

	sc->sc_txnext = WM_NEXTTX(sc, sc->sc_txnext);
	txs->txs_ndesc++;

	*cmdp = cmd;
	*fieldsp = fields;

	return (0);
}

static void
wm_dump_mbuf_chain(struct wm_softc *sc, struct mbuf *m0)
{
	struct mbuf *m;
	int i;

	log(LOG_DEBUG, "%s: mbuf chain:\n", sc->sc_dev.dv_xname);
	for (m = m0, i = 0; m != NULL; m = m->m_next, i++)
		log(LOG_DEBUG, "%s:\tm_data = %p, m_len = %d, "
		    "m_flags = 0x%08x\n", sc->sc_dev.dv_xname,
		    m->m_data, m->m_len, m->m_flags);
	log(LOG_DEBUG, "%s:\t%d mbuf%s in chain\n", sc->sc_dev.dv_xname,
	    i, i == 1 ? "" : "s");
}

/*
 * wm_82547_txfifo_stall:
 *
 *	Callout used to wait for the 82547 Tx FIFO to drain,
 *	reset the FIFO pointers, and restart packet transmission.
 */
static void
wm_82547_txfifo_stall(void *arg)
{
	struct wm_softc *sc = arg;
	int s;

	s = splnet();

	if (sc->sc_txfifo_stall) {
		if (CSR_READ(sc, WMREG_TDT) == CSR_READ(sc, WMREG_TDH) &&
		    CSR_READ(sc, WMREG_TDFT) == CSR_READ(sc, WMREG_TDFH) &&
		    CSR_READ(sc, WMREG_TDFTS) == CSR_READ(sc, WMREG_TDFHS)) {
			/*
			 * Packets have drained.  Stop transmitter, reset
			 * FIFO pointers, restart transmitter, and kick
			 * the packet queue.
			 */
			uint32_t tctl = CSR_READ(sc, WMREG_TCTL);
			CSR_WRITE(sc, WMREG_TCTL, tctl & ~TCTL_EN);
			CSR_WRITE(sc, WMREG_TDFT, sc->sc_txfifo_addr);
			CSR_WRITE(sc, WMREG_TDFH, sc->sc_txfifo_addr);
			CSR_WRITE(sc, WMREG_TDFTS, sc->sc_txfifo_addr);
			CSR_WRITE(sc, WMREG_TDFHS, sc->sc_txfifo_addr);
			CSR_WRITE(sc, WMREG_TCTL, tctl);
			CSR_WRITE_FLUSH(sc);

			sc->sc_txfifo_head = 0;
			sc->sc_txfifo_stall = 0;
			wm_start(&sc->sc_ethercom.ec_if);
		} else {
			/*
			 * Still waiting for packets to drain; try again in
			 * another tick.
			 */
			callout_schedule(&sc->sc_txfifo_ch, 1);
		}
	}

	splx(s);
}

/*
 * wm_82547_txfifo_bugchk:
 *
 *	Check for bug condition in the 82547 Tx FIFO.  We need to
 *	prevent enqueueing a packet that would wrap around the end
 *	if the Tx FIFO ring buffer, otherwise the chip will croak.
 *
 *	We do this by checking the amount of space before the end
 *	of the Tx FIFO buffer.  If the packet will not fit, we "stall"
 *	the Tx FIFO, wait for all remaining packets to drain, reset
 *	the internal FIFO pointers to the beginning, and restart
 *	transmission on the interface.
 */
#define	WM_FIFO_HDR		0x10
#define	WM_82547_PAD_LEN	0x3e0
static int
wm_82547_txfifo_bugchk(struct wm_softc *sc, struct mbuf *m0)
{
	int space = sc->sc_txfifo_size - sc->sc_txfifo_head;
	int len = roundup(m0->m_pkthdr.len + WM_FIFO_HDR, WM_FIFO_HDR);

	/* Just return if already stalled. */
	if (sc->sc_txfifo_stall)
		return (1);

	if (sc->sc_mii.mii_media_active & IFM_FDX) {
		/* Stall only occurs in half-duplex mode. */
		goto send_packet;
	}

	if (len >= WM_82547_PAD_LEN + space) {
		sc->sc_txfifo_stall = 1;
		callout_schedule(&sc->sc_txfifo_ch, 1);
		return (1);
	}

 send_packet:
	sc->sc_txfifo_head += len;
	if (sc->sc_txfifo_head >= sc->sc_txfifo_size)
		sc->sc_txfifo_head -= sc->sc_txfifo_size;

	return (0);
}

/*
 * wm_start:		[ifnet interface function]
 *
 *	Start packet transmission on the interface.
 */
static void
wm_start(struct ifnet *ifp)
{
	struct wm_softc *sc = ifp->if_softc;
	struct mbuf *m0;
#if 0 /* XXXJRT */
	struct m_tag *mtag;
#endif
	struct wm_txsoft *txs;
	bus_dmamap_t dmamap;
	int error, nexttx, lasttx = -1, ofree, seg, segs_needed, use_tso;
	bus_addr_t curaddr;
	bus_size_t seglen, curlen;
	uint32_t cksumcmd;
	uint8_t cksumfields;

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	/*
	 * Remember the previous number of free descriptors.
	 */
	ofree = sc->sc_txfree;

	/*
	 * Loop through the send queue, setting up transmit descriptors
	 * until we drain the queue, or use up all available transmit
	 * descriptors.
	 */
	for (;;) {
		/* Grab a packet off the queue. */
		IFQ_POLL(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;

		DPRINTF(WM_DEBUG_TX,
		    ("%s: TX: have packet to transmit: %p\n",
		    sc->sc_dev.dv_xname, m0));

		/* Get a work queue entry. */
		if (sc->sc_txsfree < WM_TXQUEUE_GC(sc)) {
			wm_txintr(sc);
			if (sc->sc_txsfree == 0) {
				DPRINTF(WM_DEBUG_TX,
				    ("%s: TX: no free job descriptors\n",
					sc->sc_dev.dv_xname));
				WM_EVCNT_INCR(&sc->sc_ev_txsstall);
				break;
			}
		}

		txs = &sc->sc_txsoft[sc->sc_txsnext];
		dmamap = txs->txs_dmamap;

		use_tso = (m0->m_pkthdr.csum_flags &
		    (M_CSUM_TSOv4 | M_CSUM_TSOv6)) != 0;

		/*
		 * So says the Linux driver:
		 * The controller does a simple calculation to make sure
		 * there is enough room in the FIFO before initiating the
		 * DMA for each buffer.  The calc is:
		 *	4 = ceil(buffer len / MSS)
		 * To make sure we don't overrun the FIFO, adjust the max
		 * buffer len if the MSS drops.
		 */
		dmamap->dm_maxsegsz =
		    (use_tso && (m0->m_pkthdr.segsz << 2) < WTX_MAX_LEN)
		    ? m0->m_pkthdr.segsz << 2
		    : WTX_MAX_LEN;

		/*
		 * Load the DMA map.  If this fails, the packet either
		 * didn't fit in the allotted number of segments, or we
		 * were short on resources.  For the too-many-segments
		 * case, we simply report an error and drop the packet,
		 * since we can't sanely copy a jumbo packet to a single
		 * buffer.
		 */
		error = bus_dmamap_load_mbuf(sc->sc_dmat, dmamap, m0,
		    BUS_DMA_WRITE|BUS_DMA_NOWAIT);
		if (error) {
			if (error == EFBIG) {
				WM_EVCNT_INCR(&sc->sc_ev_txdrop);
				log(LOG_ERR, "%s: Tx packet consumes too many "
				    "DMA segments, dropping...\n",
				    sc->sc_dev.dv_xname);
				IFQ_DEQUEUE(&ifp->if_snd, m0);
				wm_dump_mbuf_chain(sc, m0);
				m_freem(m0);
				continue;
			}
			/*
			 * Short on resources, just stop for now.
			 */
			DPRINTF(WM_DEBUG_TX,
			    ("%s: TX: dmamap load failed: %d\n",
			    sc->sc_dev.dv_xname, error));
			break;
		}

		segs_needed = dmamap->dm_nsegs;
		if (use_tso) {
			/* For sentinel descriptor; see below. */
			segs_needed++;
		}

		/*
		 * Ensure we have enough descriptors free to describe
		 * the packet.  Note, we always reserve one descriptor
		 * at the end of the ring due to the semantics of the
		 * TDT register, plus one more in the event we need
		 * to load offload context.
		 */
		if (segs_needed > sc->sc_txfree - 2) {
			/*
			 * Not enough free descriptors to transmit this
			 * packet.  We haven't committed anything yet,
			 * so just unload the DMA map, put the packet
			 * pack on the queue, and punt.  Notify the upper
			 * layer that there are no more slots left.
			 */
			DPRINTF(WM_DEBUG_TX,
			    ("%s: TX: need %d (%d) descriptors, have %d\n",
			    sc->sc_dev.dv_xname, dmamap->dm_nsegs, segs_needed,
			    sc->sc_txfree - 1));
			ifp->if_flags |= IFF_OACTIVE;
			bus_dmamap_unload(sc->sc_dmat, dmamap);
			WM_EVCNT_INCR(&sc->sc_ev_txdstall);
			break;
		}

		/*
		 * Check for 82547 Tx FIFO bug.  We need to do this
		 * once we know we can transmit the packet, since we
		 * do some internal FIFO space accounting here.
		 */
		if (sc->sc_type == WM_T_82547 &&
		    wm_82547_txfifo_bugchk(sc, m0)) {
			DPRINTF(WM_DEBUG_TX,
			    ("%s: TX: 82547 Tx FIFO bug detected\n",
			    sc->sc_dev.dv_xname));
			ifp->if_flags |= IFF_OACTIVE;
			bus_dmamap_unload(sc->sc_dmat, dmamap);
			WM_EVCNT_INCR(&sc->sc_ev_txfifo_stall);
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m0);

		/*
		 * WE ARE NOW COMMITTED TO TRANSMITTING THE PACKET.
		 */

		DPRINTF(WM_DEBUG_TX,
		    ("%s: TX: packet has %d (%d) DMA segments\n",
		    sc->sc_dev.dv_xname, dmamap->dm_nsegs, segs_needed));

		WM_EVCNT_INCR(&sc->sc_ev_txseg[dmamap->dm_nsegs - 1]);

		/*
		 * Store a pointer to the packet so that we can free it
		 * later.
		 *
		 * Initially, we consider the number of descriptors the
		 * packet uses the number of DMA segments.  This may be
		 * incremented by 1 if we do checksum offload (a descriptor
		 * is used to set the checksum context).
		 */
		txs->txs_mbuf = m0;
		txs->txs_firstdesc = sc->sc_txnext;
		txs->txs_ndesc = segs_needed;

		/* Set up offload parameters for this packet. */
		if (m0->m_pkthdr.csum_flags &
		    (M_CSUM_TSOv4|M_CSUM_TSOv6|
		    M_CSUM_IPv4|M_CSUM_TCPv4|M_CSUM_UDPv4|
		    M_CSUM_TCPv6|M_CSUM_UDPv6)) {
			if (wm_tx_offload(sc, txs, &cksumcmd,
					  &cksumfields) != 0) {
				/* Error message already displayed. */
				bus_dmamap_unload(sc->sc_dmat, dmamap);
				continue;
			}
		} else {
			cksumcmd = 0;
			cksumfields = 0;
		}

		cksumcmd |= WTX_CMD_IDE | WTX_CMD_IFCS;

		/* Sync the DMA map. */
		bus_dmamap_sync(sc->sc_dmat, dmamap, 0, dmamap->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);

		/*
		 * Initialize the transmit descriptor.
		 */
		for (nexttx = sc->sc_txnext, seg = 0;
		     seg < dmamap->dm_nsegs; seg++) {
			for (seglen = dmamap->dm_segs[seg].ds_len,
			     curaddr = dmamap->dm_segs[seg].ds_addr;
			     seglen != 0;
			     curaddr += curlen, seglen -= curlen,
			     nexttx = WM_NEXTTX(sc, nexttx)) {
				curlen = seglen;

				/*
				 * So says the Linux driver:
				 * Work around for premature descriptor
				 * write-backs in TSO mode.  Append a
				 * 4-byte sentinel descriptor.
				 */
				if (use_tso &&
				    seg == dmamap->dm_nsegs - 1 &&
				    curlen > 8)
					curlen -= 4;

				wm_set_dma_addr(
				    &sc->sc_txdescs[nexttx].wtx_addr,
				    curaddr);
				sc->sc_txdescs[nexttx].wtx_cmdlen =
				    htole32(cksumcmd | curlen);
				sc->sc_txdescs[nexttx].wtx_fields.wtxu_status =
				    0;
				sc->sc_txdescs[nexttx].wtx_fields.wtxu_options =
				    cksumfields;
				sc->sc_txdescs[nexttx].wtx_fields.wtxu_vlan = 0;
				lasttx = nexttx;

				DPRINTF(WM_DEBUG_TX,
				    ("%s: TX: desc %d: low 0x%08lx, "
				     "len 0x%04x\n",
				    sc->sc_dev.dv_xname, nexttx,
				    curaddr & 0xffffffffUL, (unsigned)curlen));
			}
		}

		KASSERT(lasttx != -1);

		/*
		 * Set up the command byte on the last descriptor of
		 * the packet.  If we're in the interrupt delay window,
		 * delay the interrupt.
		 */
		sc->sc_txdescs[lasttx].wtx_cmdlen |=
		    htole32(WTX_CMD_EOP | WTX_CMD_RS);

#if 0 /* XXXJRT */
		/*
		 * If VLANs are enabled and the packet has a VLAN tag, set
		 * up the descriptor to encapsulate the packet for us.
		 *
		 * This is only valid on the last descriptor of the packet.
		 */
		if ((mtag = VLAN_OUTPUT_TAG(&sc->sc_ethercom, m0)) != NULL) {
			sc->sc_txdescs[lasttx].wtx_cmdlen |=
			    htole32(WTX_CMD_VLE);
			sc->sc_txdescs[lasttx].wtx_fields.wtxu_vlan
			    = htole16(VLAN_TAG_VALUE(mtag) & 0xffff);
		}
#endif /* XXXJRT */

		txs->txs_lastdesc = lasttx;

		DPRINTF(WM_DEBUG_TX,
		    ("%s: TX: desc %d: cmdlen 0x%08x\n", sc->sc_dev.dv_xname,
		    lasttx, le32toh(sc->sc_txdescs[lasttx].wtx_cmdlen)));

		/* Sync the descriptors we're using. */
		WM_CDTXSYNC(sc, sc->sc_txnext, txs->txs_ndesc,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/* Give the packet to the chip. */
		CSR_WRITE(sc, sc->sc_tdt_reg, nexttx);

		DPRINTF(WM_DEBUG_TX,
		    ("%s: TX: TDT -> %d\n", sc->sc_dev.dv_xname, nexttx));

		DPRINTF(WM_DEBUG_TX,
		    ("%s: TX: finished transmitting packet, job %d\n",
		    sc->sc_dev.dv_xname, sc->sc_txsnext));

		/* Advance the tx pointer. */
		sc->sc_txfree -= txs->txs_ndesc;
		sc->sc_txnext = nexttx;

		sc->sc_txsfree--;
		sc->sc_txsnext = WM_NEXTTXS(sc, sc->sc_txsnext);

#if NBPFILTER > 0
		/* Pass the packet to any BPF listeners. */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m0);
#endif /* NBPFILTER > 0 */
	}

	if (sc->sc_txsfree == 0 || sc->sc_txfree <= 2) {
		/* No more slots; notify upper layer. */
		ifp->if_flags |= IFF_OACTIVE;
	}

	if (sc->sc_txfree != ofree) {
		/* Set a watchdog timer in case the chip flakes out. */
		ifp->if_timer = 5;
	}
}

/*
 * wm_watchdog:		[ifnet interface function]
 *
 *	Watchdog timer handler.
 */
static void
wm_watchdog(struct ifnet *ifp)
{
	struct wm_softc *sc = ifp->if_softc;

	/*
	 * Since we're using delayed interrupts, sweep up
	 * before we report an error.
	 */
	wm_txintr(sc);

	if (sc->sc_txfree != WM_NTXDESC(sc)) {
		log(LOG_ERR,
		    "%s: device timeout (txfree %d txsfree %d txnext %d)\n",
		    sc->sc_dev.dv_xname, sc->sc_txfree, sc->sc_txsfree,
		    sc->sc_txnext);
		ifp->if_oerrors++;

		/* Reset the interface. */
		(void) wm_init(ifp);
	}

	/* Try to get more packets going. */
	wm_start(ifp);
}

/*
 * wm_ioctl:		[ifnet interface function]
 *
 *	Handle control requests from the operator.
 */
static int
wm_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct wm_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *) data;
	int s, error;

	s = splnet();

	switch (cmd) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		/* Flow control requires full-duplex mode. */
		if (IFM_SUBTYPE(ifr->ifr_media) == IFM_AUTO ||
		    (ifr->ifr_media & IFM_FDX) == 0)
			ifr->ifr_media &= ~IFM_ETH_FMASK;
		if (IFM_SUBTYPE(ifr->ifr_media) != IFM_AUTO) {
			if ((ifr->ifr_media & IFM_ETH_FMASK) == IFM_FLOW) {
				/* We can do both TXPAUSE and RXPAUSE. */
				ifr->ifr_media |=
				    IFM_ETH_TXPAUSE | IFM_ETH_RXPAUSE;
			}
			sc->sc_flowflags = ifr->ifr_media & IFM_ETH_FMASK;
		}
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, cmd);
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			if (ifp->if_flags & IFF_RUNNING)
				wm_set_filter(sc);
			error = 0;
		}
		break;
	}

	/* Try to get more packets going. */
	wm_start(ifp);

	splx(s);
	return (error);
}

/*
 * wm_intr:
 *
 *	Interrupt service routine.
 */
static int
wm_intr(void *arg)
{
	struct wm_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint32_t icr;
	int handled = 0;

	while (1 /* CONSTCOND */) {
		icr = CSR_READ(sc, WMREG_ICR);
		if ((icr & sc->sc_icr) == 0)
			break;
#if 0 /*NRND > 0*/
		if (RND_ENABLED(&sc->rnd_source))
			rnd_add_uint32(&sc->rnd_source, icr);
#endif

		handled = 1;

#if defined(WM_DEBUG) || defined(WM_EVENT_COUNTERS)
		if (icr & (ICR_RXDMT0|ICR_RXT0)) {
			DPRINTF(WM_DEBUG_RX,
			    ("%s: RX: got Rx intr 0x%08x\n",
			    sc->sc_dev.dv_xname,
			    icr & (ICR_RXDMT0|ICR_RXT0)));
			WM_EVCNT_INCR(&sc->sc_ev_rxintr);
		}
#endif
		wm_rxintr(sc);

#if defined(WM_DEBUG) || defined(WM_EVENT_COUNTERS)
		if (icr & ICR_TXDW) {
			DPRINTF(WM_DEBUG_TX,
			    ("%s: TX: got TXDW interrupt\n",
			    sc->sc_dev.dv_xname));
			WM_EVCNT_INCR(&sc->sc_ev_txdw);
		}
#endif
		wm_txintr(sc);

		if (icr & (ICR_LSC|ICR_RXSEQ|ICR_RXCFG)) {
			WM_EVCNT_INCR(&sc->sc_ev_linkintr);
			wm_linkintr(sc, icr);
		}

		if (icr & ICR_RXO) {
			ifp->if_ierrors++;
#if defined(WM_DEBUG)
			log(LOG_WARNING, "%s: Receive overrun\n",
			    sc->sc_dev.dv_xname);
#endif /* defined(WM_DEBUG) */
		}
	}

	if (handled) {
		/* Try to get more packets going. */
		wm_start(ifp);
	}

	return (handled);
}

/*
 * wm_txintr:
 *
 *	Helper; handle transmit interrupts.
 */
static void
wm_txintr(struct wm_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct wm_txsoft *txs;
	uint8_t status;
	int i;

	ifp->if_flags &= ~IFF_OACTIVE;

	/*
	 * Go through the Tx list and free mbufs for those
	 * frames which have been transmitted.
	 */
	for (i = sc->sc_txsdirty; sc->sc_txsfree != WM_TXQUEUELEN(sc);
	     i = WM_NEXTTXS(sc, i), sc->sc_txsfree++) {
		txs = &sc->sc_txsoft[i];

		DPRINTF(WM_DEBUG_TX,
		    ("%s: TX: checking job %d\n", sc->sc_dev.dv_xname, i));

		WM_CDTXSYNC(sc, txs->txs_firstdesc, txs->txs_ndesc,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		status =
		    sc->sc_txdescs[txs->txs_lastdesc].wtx_fields.wtxu_status;
		if ((status & WTX_ST_DD) == 0) {
			WM_CDTXSYNC(sc, txs->txs_lastdesc, 1,
			    BUS_DMASYNC_PREREAD);
			break;
		}

		DPRINTF(WM_DEBUG_TX,
		    ("%s: TX: job %d done: descs %d..%d\n",
		    sc->sc_dev.dv_xname, i, txs->txs_firstdesc,
		    txs->txs_lastdesc));

		/*
		 * XXX We should probably be using the statistics
		 * XXX registers, but I don't know if they exist
		 * XXX on chips before the i82544.
		 */

#ifdef WM_EVENT_COUNTERS
		if (status & WTX_ST_TU)
			WM_EVCNT_INCR(&sc->sc_ev_tu);
#endif /* WM_EVENT_COUNTERS */

		if (status & (WTX_ST_EC|WTX_ST_LC)) {
			ifp->if_oerrors++;
			if (status & WTX_ST_LC)
				log(LOG_WARNING, "%s: late collision\n",
				    sc->sc_dev.dv_xname);
			else if (status & WTX_ST_EC) {
				ifp->if_collisions += 16;
				log(LOG_WARNING, "%s: excessive collisions\n",
				    sc->sc_dev.dv_xname);
			}
		} else
			ifp->if_opackets++;

		sc->sc_txfree += txs->txs_ndesc;
		bus_dmamap_sync(sc->sc_dmat, txs->txs_dmamap,
		    0, txs->txs_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
		m_freem(txs->txs_mbuf);
		txs->txs_mbuf = NULL;
	}

	/* Update the dirty transmit buffer pointer. */
	sc->sc_txsdirty = i;
	DPRINTF(WM_DEBUG_TX,
	    ("%s: TX: txsdirty -> %d\n", sc->sc_dev.dv_xname, i));

	/*
	 * If there are no more pending transmissions, cancel the watchdog
	 * timer.
	 */
	if (sc->sc_txsfree == WM_TXQUEUELEN(sc))
		ifp->if_timer = 0;
}

/*
 * wm_rxintr:
 *
 *	Helper; handle receive interrupts.
 */
static void
wm_rxintr(struct wm_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct wm_rxsoft *rxs;
	struct mbuf *m;
	int i, len;
	uint8_t status, errors;

	for (i = sc->sc_rxptr;; i = WM_NEXTRX(i)) {
		rxs = &sc->sc_rxsoft[i];

		DPRINTF(WM_DEBUG_RX,
		    ("%s: RX: checking descriptor %d\n",
		    sc->sc_dev.dv_xname, i));

		WM_CDRXSYNC(sc, i, BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		status = sc->sc_rxdescs[i].wrx_status;
		errors = sc->sc_rxdescs[i].wrx_errors;
		len = le16toh(sc->sc_rxdescs[i].wrx_len);

		if ((status & WRX_ST_DD) == 0) {
			/*
			 * We have processed all of the receive descriptors.
			 */
			WM_CDRXSYNC(sc, i, BUS_DMASYNC_PREREAD);
			break;
		}

		if (__predict_false(sc->sc_rxdiscard)) {
			DPRINTF(WM_DEBUG_RX,
			    ("%s: RX: discarding contents of descriptor %d\n",
			    sc->sc_dev.dv_xname, i));
			WM_INIT_RXDESC(sc, i);
			if (status & WRX_ST_EOP) {
				/* Reset our state. */
				DPRINTF(WM_DEBUG_RX,
				    ("%s: RX: resetting rxdiscard -> 0\n",
				    sc->sc_dev.dv_xname));
				sc->sc_rxdiscard = 0;
			}
			continue;
		}

		bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
		    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);

		m = rxs->rxs_mbuf;

		/*
		 * Add a new receive buffer to the ring, unless of
		 * course the length is zero. Treat the latter as a
		 * failed mapping.
		 */
		if ((len == 0) || (wm_add_rxbuf(sc, i) != 0)) {
			/*
			 * Failed, throw away what we've done so
			 * far, and discard the rest of the packet.
			 */
			ifp->if_ierrors++;
			bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
			    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);
			WM_INIT_RXDESC(sc, i);
			if ((status & WRX_ST_EOP) == 0)
				sc->sc_rxdiscard = 1;
			if (sc->sc_rxhead != NULL)
				m_freem(sc->sc_rxhead);
			WM_RXCHAIN_RESET(sc);
			DPRINTF(WM_DEBUG_RX,
			    ("%s: RX: Rx buffer allocation failed, "
			    "dropping packet%s\n", sc->sc_dev.dv_xname,
			    sc->sc_rxdiscard ? " (discard)" : ""));
			continue;
		}

		WM_RXCHAIN_LINK(sc, m);

		m->m_len = len;

		DPRINTF(WM_DEBUG_RX,
		    ("%s: RX: buffer at %p len %d\n",
		    sc->sc_dev.dv_xname, m->m_data, len));

		/*
		 * If this is not the end of the packet, keep
		 * looking.
		 */
		if ((status & WRX_ST_EOP) == 0) {
			sc->sc_rxlen += len;
			DPRINTF(WM_DEBUG_RX,
			    ("%s: RX: not yet EOP, rxlen -> %d\n",
			    sc->sc_dev.dv_xname, sc->sc_rxlen));
			continue;
		}

		/*
		 * Okay, we have the entire packet now.  The chip is
		 * configured to include the FCS (not all chips can
		 * be configured to strip it), so we need to trim it.
		 */
		m->m_len -= ETHER_CRC_LEN;

		*sc->sc_rxtailp = NULL;
		len = m->m_len + sc->sc_rxlen;
		m = sc->sc_rxhead;

		WM_RXCHAIN_RESET(sc);

		DPRINTF(WM_DEBUG_RX,
		    ("%s: RX: have entire packet, len -> %d\n",
		    sc->sc_dev.dv_xname, len));

		/*
		 * If an error occurred, update stats and drop the packet.
		 */
		if (errors &
		     (WRX_ER_CE|WRX_ER_SE|WRX_ER_SEQ|WRX_ER_CXE|WRX_ER_RXE)) {
			ifp->if_ierrors++;
			if (errors & WRX_ER_SE)
				log(LOG_WARNING, "%s: symbol error\n",
				    sc->sc_dev.dv_xname);
			else if (errors & WRX_ER_SEQ)
				log(LOG_WARNING, "%s: receive sequence error\n",
				    sc->sc_dev.dv_xname);
			else if (errors & WRX_ER_CE)
				log(LOG_WARNING, "%s: CRC error\n",
				    sc->sc_dev.dv_xname);
			m_freem(m);
			continue;
		}

		/*
		 * No errors.  Receive the packet.
		 */
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = len;

#if 0 /* XXXJRT */
		/*
		 * If VLANs are enabled, VLAN packets have been unwrapped
		 * for us.  Associate the tag with the packet.
		 */
		if ((status & WRX_ST_VP) != 0) {
			VLAN_INPUT_TAG(ifp, m,
			    le16toh(sc->sc_rxdescs[i].wrx_special,
			    continue);
		}
#endif /* XXXJRT */

		/*
		 * Set up checksum info for this packet.
		 */
		if ((status & WRX_ST_IXSM) == 0) {
			if (status & WRX_ST_IPCS) {
				WM_EVCNT_INCR(&sc->sc_ev_rxipsum);
				m->m_pkthdr.csum_flags |= M_CSUM_IPv4;
				if (errors & WRX_ER_IPE)
					m->m_pkthdr.csum_flags |=
					    M_CSUM_IPv4_BAD;
			}
			if (status & WRX_ST_TCPCS) {
				/*
				 * Note: we don't know if this was TCP or UDP,
				 * so we just set both bits, and expect the
				 * upper layers to deal.
				 */
				WM_EVCNT_INCR(&sc->sc_ev_rxtusum);
				m->m_pkthdr.csum_flags |=
				    M_CSUM_TCPv4 | M_CSUM_UDPv4 |
				    M_CSUM_TCPv6 | M_CSUM_UDPv6;
				if (errors & WRX_ER_TCPE)
					m->m_pkthdr.csum_flags |=
					    M_CSUM_TCP_UDP_BAD;
			}
		}

		ifp->if_ipackets++;

#if NBPFILTER > 0
		/* Pass this up to any BPF listeners. */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif /* NBPFILTER > 0 */

		/* Pass it on. */
		(*ifp->if_input)(ifp, m);
	}

	/* Update the receive pointer. */
	sc->sc_rxptr = i;

	DPRINTF(WM_DEBUG_RX,
	    ("%s: RX: rxptr -> %d\n", sc->sc_dev.dv_xname, i));
}

/*
 * wm_linkintr:
 *
 *	Helper; handle link interrupts.
 */
static void
wm_linkintr(struct wm_softc *sc, uint32_t icr)
{
	uint32_t status;

	/*
	 * If we get a link status interrupt on a 1000BASE-T
	 * device, just fall into the normal MII tick path.
	 */
	if (sc->sc_flags & WM_F_HAS_MII) {
		if (icr & ICR_LSC) {
			DPRINTF(WM_DEBUG_LINK,
			    ("%s: LINK: LSC -> mii_tick\n",
			    sc->sc_dev.dv_xname));
			mii_tick(&sc->sc_mii);
		} else if (icr & ICR_RXSEQ) {
			DPRINTF(WM_DEBUG_LINK,
			    ("%s: LINK Receive sequence error\n",
			    sc->sc_dev.dv_xname));
		}
		return;
	}

	/*
	 * If we are now receiving /C/, check for link again in
	 * a couple of link clock ticks.
	 */
	if (icr & ICR_RXCFG) {
		DPRINTF(WM_DEBUG_LINK, ("%s: LINK: receiving /C/\n",
		    sc->sc_dev.dv_xname));
		sc->sc_tbi_anstate = 2;
	}

	if (icr & ICR_LSC) {
		status = CSR_READ(sc, WMREG_STATUS);
		if (status & STATUS_LU) {
			DPRINTF(WM_DEBUG_LINK, ("%s: LINK: LSC -> up %s\n",
			    sc->sc_dev.dv_xname,
			    (status & STATUS_FD) ? "FDX" : "HDX"));
			sc->sc_tctl &= ~TCTL_COLD(0x3ff);
			sc->sc_fcrtl &= ~FCRTL_XONE;
			if (status & STATUS_FD)
				sc->sc_tctl |=
				    TCTL_COLD(TX_COLLISION_DISTANCE_FDX);
			else
				sc->sc_tctl |=
				    TCTL_COLD(TX_COLLISION_DISTANCE_HDX);
			if (CSR_READ(sc, WMREG_CTRL) & CTRL_TFCE)
				sc->sc_fcrtl |= FCRTL_XONE;
			CSR_WRITE(sc, WMREG_TCTL, sc->sc_tctl);
			CSR_WRITE(sc, (sc->sc_type < WM_T_82543) ?
				      WMREG_OLD_FCRTL : WMREG_FCRTL,
				      sc->sc_fcrtl);
			sc->sc_tbi_linkup = 1;
		} else {
			DPRINTF(WM_DEBUG_LINK, ("%s: LINK: LSC -> down\n",
			    sc->sc_dev.dv_xname));
			sc->sc_tbi_linkup = 0;
		}
		sc->sc_tbi_anstate = 2;
		wm_tbi_set_linkled(sc);
	} else if (icr & ICR_RXSEQ) {
		DPRINTF(WM_DEBUG_LINK,
		    ("%s: LINK: Receive sequence error\n",
		    sc->sc_dev.dv_xname));
	}
}

/*
 * wm_tick:
 *
 *	One second timer, used to check link status, sweep up
 *	completed transmit jobs, etc.
 */
static void
wm_tick(void *arg)
{
	struct wm_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int s;

	s = splnet();

	if (sc->sc_type >= WM_T_82542_2_1) {
		WM_EVCNT_ADD(&sc->sc_ev_rx_xon, CSR_READ(sc, WMREG_XONRXC));
		WM_EVCNT_ADD(&sc->sc_ev_tx_xon, CSR_READ(sc, WMREG_XONTXC));
		WM_EVCNT_ADD(&sc->sc_ev_rx_xoff, CSR_READ(sc, WMREG_XOFFRXC));
		WM_EVCNT_ADD(&sc->sc_ev_tx_xoff, CSR_READ(sc, WMREG_XOFFTXC));
		WM_EVCNT_ADD(&sc->sc_ev_rx_macctl, CSR_READ(sc, WMREG_FCRUC));
	}

	ifp->if_collisions += CSR_READ(sc, WMREG_COLC);
	ifp->if_ierrors += CSR_READ(sc, WMREG_RXERRC);


	if (sc->sc_flags & WM_F_HAS_MII)
		mii_tick(&sc->sc_mii);
	else
		wm_tbi_check_link(sc);

	splx(s);

	callout_reset(&sc->sc_tick_ch, hz, wm_tick, sc);
}

/*
 * wm_reset:
 *
 *	Reset the i82542 chip.
 */
static void
wm_reset(struct wm_softc *sc)
{
	int i;

	/*
	 * Allocate on-chip memory according to the MTU size.
	 * The Packet Buffer Allocation register must be written
	 * before the chip is reset.
	 */
	switch (sc->sc_type) {
	case WM_T_82547:
	case WM_T_82547_2:
		sc->sc_pba = sc->sc_ethercom.ec_if.if_mtu > 8192 ?
		    PBA_22K : PBA_30K;
		sc->sc_txfifo_head = 0;
		sc->sc_txfifo_addr = sc->sc_pba << PBA_ADDR_SHIFT;
		sc->sc_txfifo_size =
		    (PBA_40K - sc->sc_pba) << PBA_BYTE_SHIFT;
		sc->sc_txfifo_stall = 0;
		break;
	case WM_T_82571:
	case WM_T_82572:		
	case WM_T_80003:		
		sc->sc_pba = PBA_32K;
		break;
	case WM_T_82573:
		sc->sc_pba = PBA_12K;
		break;
	case WM_T_ICH8:
		sc->sc_pba = PBA_8K;
		CSR_WRITE(sc, WMREG_PBS, PBA_16K);
		break;
	default:
		sc->sc_pba = sc->sc_ethercom.ec_if.if_mtu > 8192 ?
		    PBA_40K : PBA_48K;
		break;
	}
	CSR_WRITE(sc, WMREG_PBA, sc->sc_pba);

	/*
	 * 82541 Errata 29? & 82547 Errata 28?
	 * See also the description about PHY_RST bit in CTRL register
	 * in 8254x_GBe_SDM.pdf.
	 */
	if ((sc->sc_type == WM_T_82541) || (sc->sc_type == WM_T_82547)) {
		CSR_WRITE(sc, WMREG_CTRL,
		    CSR_READ(sc, WMREG_CTRL) | CTRL_PHY_RESET);
		delay(5000);
	}

	switch (sc->sc_type) {
	case WM_T_82544:
	case WM_T_82540:
	case WM_T_82545:
	case WM_T_82546:
	case WM_T_82541:
	case WM_T_82541_2:
		/*
		 * On some chipsets, a reset through a memory-mapped write
		 * cycle can cause the chip to reset before completing the
		 * write cycle.  This causes major headache that can be
		 * avoided by issuing the reset via indirect register writes
		 * through I/O space.
		 *
		 * So, if we successfully mapped the I/O BAR at attach time,
		 * use that.  Otherwise, try our luck with a memory-mapped
		 * reset.
		 */
		if (sc->sc_flags & WM_F_IOH_VALID)
			wm_io_write(sc, WMREG_CTRL, CTRL_RST);
		else
			CSR_WRITE(sc, WMREG_CTRL, CTRL_RST);
		break;

	case WM_T_82545_3:
	case WM_T_82546_3:
		/* Use the shadow control register on these chips. */
		CSR_WRITE(sc, WMREG_CTRL_SHADOW, CTRL_RST);
		break;

	case WM_T_ICH8:
		wm_get_swfwhw_semaphore(sc);
		CSR_WRITE(sc, WMREG_CTRL, CTRL_RST | CTRL_PHY_RESET);

	default:
		/* Everything else can safely use the documented method. */
		CSR_WRITE(sc, WMREG_CTRL, CTRL_RST);
		break;
	}
	delay(10000);

	for (i = 0; i < 1000; i++) {
		if ((CSR_READ(sc, WMREG_CTRL) & CTRL_RST) == 0)
			return;
		delay(20);
	}

	if (CSR_READ(sc, WMREG_CTRL) & CTRL_RST)
		log(LOG_ERR, "%s: reset failed to complete\n",
		    sc->sc_dev.dv_xname);

	if (sc->sc_type >= WM_T_80003) {
		/* wait for eeprom to reload */
		for (i = 1000; i > 0; i--) {
			if (CSR_READ(sc, WMREG_EECD) & EECD_EE_AUTORD)
				break;
		}
		if (i == 0) {
			log(LOG_ERR, "%s: auto read from eeprom failed to "
			    "complete\n", sc->sc_dev.dv_xname);
		}
	}
}

/*
 * wm_init:		[ifnet interface function]
 *
 *	Initialize the interface.  Must be called at splnet().
 */
static int
wm_init(struct ifnet *ifp)
{
	struct wm_softc *sc = ifp->if_softc;
	struct wm_rxsoft *rxs;
	int i, error = 0;
	uint32_t reg;

	/*
	 * *_HDR_ALIGNED_P is constant 1 if __NO_STRICT_ALIGMENT is set.
	 * There is a small but measurable benefit to avoiding the adjusment
	 * of the descriptor so that the headers are aligned, for normal mtu,
	 * on such platforms.  One possibility is that the DMA itself is
	 * slightly more efficient if the front of the entire packet (instead
	 * of the front of the headers) is aligned.
	 *
	 * Note we must always set align_tweak to 0 if we are using
	 * jumbo frames.
	 */
#ifdef __NO_STRICT_ALIGNMENT
	sc->sc_align_tweak = 0;
#else
	if ((ifp->if_mtu + ETHER_HDR_LEN + ETHER_CRC_LEN) > (MCLBYTES - 2))
		sc->sc_align_tweak = 0;
	else
		sc->sc_align_tweak = 2;
#endif /* __NO_STRICT_ALIGNMENT */

	/* Cancel any pending I/O. */
	wm_stop(ifp, 0);

	/* update statistics before reset */
	ifp->if_collisions += CSR_READ(sc, WMREG_COLC);
	ifp->if_ierrors += CSR_READ(sc, WMREG_RXERRC);

	/* Reset the chip to a known state. */
	wm_reset(sc);

	/* Initialize the transmit descriptor ring. */
	memset(sc->sc_txdescs, 0, WM_TXDESCSIZE(sc));
	WM_CDTXSYNC(sc, 0, WM_NTXDESC(sc),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	sc->sc_txfree = WM_NTXDESC(sc);
	sc->sc_txnext = 0;

	if (sc->sc_type < WM_T_82543) {
		CSR_WRITE(sc, WMREG_OLD_TBDAH, WM_CDTXADDR_HI(sc, 0));
		CSR_WRITE(sc, WMREG_OLD_TBDAL, WM_CDTXADDR_LO(sc, 0));
		CSR_WRITE(sc, WMREG_OLD_TDLEN, WM_TXDESCSIZE(sc));
		CSR_WRITE(sc, WMREG_OLD_TDH, 0);
		CSR_WRITE(sc, WMREG_OLD_TDT, 0);
		CSR_WRITE(sc, WMREG_OLD_TIDV, 128);
	} else {
		CSR_WRITE(sc, WMREG_TBDAH, WM_CDTXADDR_HI(sc, 0));
		CSR_WRITE(sc, WMREG_TBDAL, WM_CDTXADDR_LO(sc, 0));
		CSR_WRITE(sc, WMREG_TDLEN, WM_TXDESCSIZE(sc));
		CSR_WRITE(sc, WMREG_TDH, 0);
		CSR_WRITE(sc, WMREG_TDT, 0);
		CSR_WRITE(sc, WMREG_TIDV, 64);
		CSR_WRITE(sc, WMREG_TADV, 128);

		CSR_WRITE(sc, WMREG_TXDCTL, TXDCTL_PTHRESH(0) |
		    TXDCTL_HTHRESH(0) | TXDCTL_WTHRESH(0));
		CSR_WRITE(sc, WMREG_RXDCTL, RXDCTL_PTHRESH(0) |
		    RXDCTL_HTHRESH(0) | RXDCTL_WTHRESH(1));
	}
	CSR_WRITE(sc, WMREG_TQSA_LO, 0);
	CSR_WRITE(sc, WMREG_TQSA_HI, 0);

	/* Initialize the transmit job descriptors. */
	for (i = 0; i < WM_TXQUEUELEN(sc); i++)
		sc->sc_txsoft[i].txs_mbuf = NULL;
	sc->sc_txsfree = WM_TXQUEUELEN(sc);
	sc->sc_txsnext = 0;
	sc->sc_txsdirty = 0;

	/*
	 * Initialize the receive descriptor and receive job
	 * descriptor rings.
	 */
	if (sc->sc_type < WM_T_82543) {
		CSR_WRITE(sc, WMREG_OLD_RDBAH0, WM_CDRXADDR_HI(sc, 0));
		CSR_WRITE(sc, WMREG_OLD_RDBAL0, WM_CDRXADDR_LO(sc, 0));
		CSR_WRITE(sc, WMREG_OLD_RDLEN0, sizeof(sc->sc_rxdescs));
		CSR_WRITE(sc, WMREG_OLD_RDH0, 0);
		CSR_WRITE(sc, WMREG_OLD_RDT0, 0);
		CSR_WRITE(sc, WMREG_OLD_RDTR0, 28 | RDTR_FPD);

		CSR_WRITE(sc, WMREG_OLD_RDBA1_HI, 0);
		CSR_WRITE(sc, WMREG_OLD_RDBA1_LO, 0);
		CSR_WRITE(sc, WMREG_OLD_RDLEN1, 0);
		CSR_WRITE(sc, WMREG_OLD_RDH1, 0);
		CSR_WRITE(sc, WMREG_OLD_RDT1, 0);
		CSR_WRITE(sc, WMREG_OLD_RDTR1, 0);
	} else {
		CSR_WRITE(sc, WMREG_RDBAH, WM_CDRXADDR_HI(sc, 0));
		CSR_WRITE(sc, WMREG_RDBAL, WM_CDRXADDR_LO(sc, 0));
		CSR_WRITE(sc, WMREG_RDLEN, sizeof(sc->sc_rxdescs));
		CSR_WRITE(sc, WMREG_RDH, 0);
		CSR_WRITE(sc, WMREG_RDT, 0);
		CSR_WRITE(sc, WMREG_RDTR, 0 | RDTR_FPD);
		CSR_WRITE(sc, WMREG_RADV, 128);
	}
	for (i = 0; i < WM_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];
		if (rxs->rxs_mbuf == NULL) {
			if ((error = wm_add_rxbuf(sc, i)) != 0) {
				log(LOG_ERR, "%s: unable to allocate or map rx "
				    "buffer %d, error = %d\n",
				    sc->sc_dev.dv_xname, i, error);
				/*
				 * XXX Should attempt to run with fewer receive
				 * XXX buffers instead of just failing.
				 */
				wm_rxdrain(sc);
				goto out;
			}
		} else
			WM_INIT_RXDESC(sc, i);
	}
	sc->sc_rxptr = 0;
	sc->sc_rxdiscard = 0;
	WM_RXCHAIN_RESET(sc);

	/*
	 * Clear out the VLAN table -- we don't use it (yet).
	 */
	CSR_WRITE(sc, WMREG_VET, 0);
	for (i = 0; i < WM_VLAN_TABSIZE; i++)
		CSR_WRITE(sc, WMREG_VFTA + (i << 2), 0);

	/*
	 * Set up flow-control parameters.
	 *
	 * XXX Values could probably stand some tuning.
	 */
	if (sc->sc_type != WM_T_ICH8) {
		CSR_WRITE(sc, WMREG_FCAL, FCAL_CONST);
		CSR_WRITE(sc, WMREG_FCAH, FCAH_CONST);
		CSR_WRITE(sc, WMREG_FCT, ETHERTYPE_FLOWCONTROL);
	}

	sc->sc_fcrtl = FCRTL_DFLT;
	if (sc->sc_type < WM_T_82543) {
		CSR_WRITE(sc, WMREG_OLD_FCRTH, FCRTH_DFLT);
		CSR_WRITE(sc, WMREG_OLD_FCRTL, sc->sc_fcrtl);
	} else {
		CSR_WRITE(sc, WMREG_FCRTH, FCRTH_DFLT);
		CSR_WRITE(sc, WMREG_FCRTL, sc->sc_fcrtl);
	}
	CSR_WRITE(sc, WMREG_FCTTV, FCTTV_DFLT);

#if 0 /* XXXJRT */
	/* Deal with VLAN enables. */
	if (VLAN_ATTACHED(&sc->sc_ethercom))
		sc->sc_ctrl |= CTRL_VME;
	else
#endif /* XXXJRT */
		sc->sc_ctrl &= ~CTRL_VME;

	/* Write the control registers. */
	CSR_WRITE(sc, WMREG_CTRL, sc->sc_ctrl);
	if (sc->sc_type >= WM_T_80003 && (sc->sc_flags & WM_F_HAS_MII)) {
		int val;
		val = CSR_READ(sc, WMREG_CTRL_EXT);
		val &= ~CTRL_EXT_LINK_MODE_MASK;
		CSR_WRITE(sc, WMREG_CTRL_EXT, val);

		/* Bypass RX and TX FIFO's */
		wm_kmrn_i80003_writereg(sc, KUMCTRLSTA_OFFSET_FIFO_CTRL,
		    KUMCTRLSTA_FIFO_CTRL_RX_BYPASS | 
		    KUMCTRLSTA_FIFO_CTRL_TX_BYPASS);
		
		wm_kmrn_i80003_writereg(sc, KUMCTRLSTA_OFFSET_INB_CTRL,
		    KUMCTRLSTA_INB_CTRL_DIS_PADDING |
		    KUMCTRLSTA_INB_CTRL_LINK_TMOUT_DFLT);
		/*
		 * Set the mac to wait the maximum time between each
		 * iteration and increase the max iterations when
		 * polling the phy; this fixes erroneous timeouts at 10Mbps.
		 */
		wm_kmrn_i80003_writereg(sc, KUMCTRLSTA_OFFSET_TIMEOUTS, 0xFFFF);
		val = wm_kmrn_i80003_readreg(sc, KUMCTRLSTA_OFFSET_INB_PARAM);
		val |= 0x3F;
		wm_kmrn_i80003_writereg(sc, KUMCTRLSTA_OFFSET_INB_PARAM, val);
	}
#if 0
	CSR_WRITE(sc, WMREG_CTRL_EXT, sc->sc_ctrl_ext);
#endif

	/*
	 * Set up checksum offload parameters.
	 */
	reg = CSR_READ(sc, WMREG_RXCSUM);
	reg &= ~(RXCSUM_IPOFL | RXCSUM_IPV6OFL | RXCSUM_TUOFL);
	if (ifp->if_capenable & IFCAP_CSUM_IPv4_Rx)
		reg |= RXCSUM_IPOFL;
	if (ifp->if_capenable & (IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_UDPv4_Rx))
		reg |= RXCSUM_IPOFL | RXCSUM_TUOFL;
	if (ifp->if_capenable & (IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_UDPv6_Rx))
		reg |= RXCSUM_IPV6OFL | RXCSUM_TUOFL;
	CSR_WRITE(sc, WMREG_RXCSUM, reg);

	/*
	 * Set up the interrupt registers.
	 */
	CSR_WRITE(sc, WMREG_IMC, 0xffffffffU);
	sc->sc_icr = ICR_TXDW | ICR_LSC | ICR_RXSEQ | ICR_RXDMT0 |
	    ICR_RXO | ICR_RXT0;
	if ((sc->sc_flags & WM_F_HAS_MII) == 0)
		sc->sc_icr |= ICR_RXCFG;
	CSR_WRITE(sc, WMREG_IMS, sc->sc_icr);

	/* Set up the inter-packet gap. */
	CSR_WRITE(sc, WMREG_TIPG, sc->sc_tipg);

	if (sc->sc_type >= WM_T_82543) {
		/* Set up the interrupt throttling register (units of 256ns) */
		sc->sc_itr = 1000000000 / (7000 * 256);
		CSR_WRITE(sc, WMREG_ITR, sc->sc_itr);
	}

#if 0 /* XXXJRT */
	/* Set the VLAN ethernetype. */
	CSR_WRITE(sc, WMREG_VET, ETHERTYPE_VLAN);
#endif

	/*
	 * Set up the transmit control register; we start out with
	 * a collision distance suitable for FDX, but update it whe
	 * we resolve the media type.
	 */
	sc->sc_tctl = TCTL_EN | TCTL_PSP | TCTL_CT(TX_COLLISION_THRESHOLD) |
	    TCTL_COLD(TX_COLLISION_DISTANCE_FDX);
	if (sc->sc_type >= WM_T_82571)
		sc->sc_tctl |= TCTL_MULR;
	if (sc->sc_type >= WM_T_80003)
		sc->sc_tctl |= TCTL_RTLC;
	CSR_WRITE(sc, WMREG_TCTL, sc->sc_tctl);

	/* Set the media. */
	(void) (*sc->sc_mii.mii_media.ifm_change)(ifp);

	/*
	 * Set up the receive control register; we actually program
	 * the register when we set the receive filter.  Use multicast
	 * address offset type 0.
	 *
	 * Only the i82544 has the ability to strip the incoming
	 * CRC, so we don't enable that feature.
	 */
	sc->sc_mchash_type = 0;
	sc->sc_rctl = RCTL_EN | RCTL_LBM_NONE | RCTL_RDMTS_1_2 | RCTL_DPF
	    | RCTL_MO(sc->sc_mchash_type);

	/* 82573 doesn't support jumbo frame */
	if (sc->sc_type != WM_T_82573 && sc->sc_type != WM_T_ICH8)
		sc->sc_rctl |= RCTL_LPE;

	if (MCLBYTES == 2048) {
		sc->sc_rctl |= RCTL_2k;
	} else {
		if (sc->sc_type >= WM_T_82543) {
			switch(MCLBYTES) {
			case 4096:
				sc->sc_rctl |= RCTL_BSEX | RCTL_BSEX_4k;
				break;
			case 8192:
				sc->sc_rctl |= RCTL_BSEX | RCTL_BSEX_8k;
				break;
			case 16384:
				sc->sc_rctl |= RCTL_BSEX | RCTL_BSEX_16k;
				break;
			default:
				panic("wm_init: MCLBYTES %d unsupported",
				    MCLBYTES);
				break;
			}
		} else panic("wm_init: i82542 requires MCLBYTES = 2048");
	}

	/* Set the receive filter. */
	wm_set_filter(sc);

	/* Start the one second link check clock. */
	callout_reset(&sc->sc_tick_ch, hz, wm_tick, sc);

	/* ...all done! */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

 out:
	if (error)
		log(LOG_ERR, "%s: interface not running\n",
		    sc->sc_dev.dv_xname);
	return (error);
}

/*
 * wm_rxdrain:
 *
 *	Drain the receive queue.
 */
static void
wm_rxdrain(struct wm_softc *sc)
{
	struct wm_rxsoft *rxs;
	int i;

	for (i = 0; i < WM_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];
		if (rxs->rxs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);
			m_freem(rxs->rxs_mbuf);
			rxs->rxs_mbuf = NULL;
		}
	}
}

/*
 * wm_stop:		[ifnet interface function]
 *
 *	Stop transmission on the interface.
 */
static void
wm_stop(struct ifnet *ifp, int disable)
{
	struct wm_softc *sc = ifp->if_softc;
	struct wm_txsoft *txs;
	int i;

	/* Stop the one second clock. */
	callout_stop(&sc->sc_tick_ch);

	/* Stop the 82547 Tx FIFO stall check timer. */
	if (sc->sc_type == WM_T_82547)
		callout_stop(&sc->sc_txfifo_ch);

	if (sc->sc_flags & WM_F_HAS_MII) {
		/* Down the MII. */
		mii_down(&sc->sc_mii);
	}

	/* Stop the transmit and receive processes. */
	CSR_WRITE(sc, WMREG_TCTL, 0);
	CSR_WRITE(sc, WMREG_RCTL, 0);

	/*
	 * Clear the interrupt mask to ensure the device cannot assert its
	 * interrupt line.
	 * Clear sc->sc_icr to ensure wm_intr() makes no attempt to service
	 * any currently pending or shared interrupt.
	 */
	CSR_WRITE(sc, WMREG_IMC, 0xffffffffU);
	sc->sc_icr = 0;

	/* Release any queued transmit buffers. */
	for (i = 0; i < WM_TXQUEUELEN(sc); i++) {
		txs = &sc->sc_txsoft[i];
		if (txs->txs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
			m_freem(txs->txs_mbuf);
			txs->txs_mbuf = NULL;
		}
	}

	if (disable)
		wm_rxdrain(sc);

	/* Mark the interface as down and cancel the watchdog timer. */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
}

/*
 * wm_acquire_eeprom:
 *
 *	Perform the EEPROM handshake required on some chips.
 */
static int
wm_acquire_eeprom(struct wm_softc *sc)
{
	uint32_t reg;
	int x;
	int ret = 0;

	/* always success */
	if ((sc->sc_flags & WM_F_EEPROM_FLASH) != 0)
		return 0;

	if (sc->sc_flags & WM_F_SWFWHW_SYNC) {
		ret = wm_get_swfwhw_semaphore(sc);
	} else if (sc->sc_flags & WM_F_SWFW_SYNC) {
		/* this will also do wm_get_swsm_semaphore() if needed */
		ret = wm_get_swfw_semaphore(sc, SWFW_EEP_SM);
	} else if (sc->sc_flags & WM_F_EEPROM_SEMAPHORE) {
		ret = wm_get_swsm_semaphore(sc);
	}

	if (ret)
		return 1;

	if (sc->sc_flags & WM_F_EEPROM_HANDSHAKE)  {
		reg = CSR_READ(sc, WMREG_EECD);

		/* Request EEPROM access. */
		reg |= EECD_EE_REQ;
		CSR_WRITE(sc, WMREG_EECD, reg);

		/* ..and wait for it to be granted. */
		for (x = 0; x < 1000; x++) {
			reg = CSR_READ(sc, WMREG_EECD);
			if (reg & EECD_EE_GNT)
				break;
			delay(5);
		}
		if ((reg & EECD_EE_GNT) == 0) {
			aprint_error("%s: could not acquire EEPROM GNT\n",
			    sc->sc_dev.dv_xname);
			reg &= ~EECD_EE_REQ;
			CSR_WRITE(sc, WMREG_EECD, reg);
			if (sc->sc_flags & WM_F_SWFWHW_SYNC)
				wm_put_swfwhw_semaphore(sc);
			if (sc->sc_flags & WM_F_SWFW_SYNC)
				wm_put_swfw_semaphore(sc, SWFW_EEP_SM);
			else if (sc->sc_flags & WM_F_EEPROM_SEMAPHORE)
				wm_put_swsm_semaphore(sc);
			return (1);
		}
	}

	return (0);
}

/*
 * wm_release_eeprom:
 *
 *	Release the EEPROM mutex.
 */
static void
wm_release_eeprom(struct wm_softc *sc)
{
	uint32_t reg;

	/* always success */
	if ((sc->sc_flags & WM_F_EEPROM_FLASH) != 0)
		return;

	if (sc->sc_flags & WM_F_EEPROM_HANDSHAKE) {
		reg = CSR_READ(sc, WMREG_EECD);
		reg &= ~EECD_EE_REQ;
		CSR_WRITE(sc, WMREG_EECD, reg);
	}

	if (sc->sc_flags & WM_F_SWFWHW_SYNC)
		wm_put_swfwhw_semaphore(sc);
	if (sc->sc_flags & WM_F_SWFW_SYNC)
		wm_put_swfw_semaphore(sc, SWFW_EEP_SM);
	else if (sc->sc_flags & WM_F_EEPROM_SEMAPHORE)
		wm_put_swsm_semaphore(sc);
}

/*
 * wm_eeprom_sendbits:
 *
 *	Send a series of bits to the EEPROM.
 */
static void
wm_eeprom_sendbits(struct wm_softc *sc, uint32_t bits, int nbits)
{
	uint32_t reg;
	int x;

	reg = CSR_READ(sc, WMREG_EECD);

	for (x = nbits; x > 0; x--) {
		if (bits & (1U << (x - 1)))
			reg |= EECD_DI;
		else
			reg &= ~EECD_DI;
		CSR_WRITE(sc, WMREG_EECD, reg);
		delay(2);
		CSR_WRITE(sc, WMREG_EECD, reg | EECD_SK);
		delay(2);
		CSR_WRITE(sc, WMREG_EECD, reg);
		delay(2);
	}
}

/*
 * wm_eeprom_recvbits:
 *
 *	Receive a series of bits from the EEPROM.
 */
static void
wm_eeprom_recvbits(struct wm_softc *sc, uint32_t *valp, int nbits)
{
	uint32_t reg, val;
	int x;

	reg = CSR_READ(sc, WMREG_EECD) & ~EECD_DI;

	val = 0;
	for (x = nbits; x > 0; x--) {
		CSR_WRITE(sc, WMREG_EECD, reg | EECD_SK);
		delay(2);
		if (CSR_READ(sc, WMREG_EECD) & EECD_DO)
			val |= (1U << (x - 1));
		CSR_WRITE(sc, WMREG_EECD, reg);
		delay(2);
	}
	*valp = val;
}

/*
 * wm_read_eeprom_uwire:
 *
 *	Read a word from the EEPROM using the MicroWire protocol.
 */
static int
wm_read_eeprom_uwire(struct wm_softc *sc, int word, int wordcnt, uint16_t *data)
{
	uint32_t reg, val;
	int i;

	for (i = 0; i < wordcnt; i++) {
		/* Clear SK and DI. */
		reg = CSR_READ(sc, WMREG_EECD) & ~(EECD_SK | EECD_DI);
		CSR_WRITE(sc, WMREG_EECD, reg);

		/* Set CHIP SELECT. */
		reg |= EECD_CS;
		CSR_WRITE(sc, WMREG_EECD, reg);
		delay(2);

		/* Shift in the READ command. */
		wm_eeprom_sendbits(sc, UWIRE_OPC_READ, 3);

		/* Shift in address. */
		wm_eeprom_sendbits(sc, word + i, sc->sc_ee_addrbits);

		/* Shift out the data. */
		wm_eeprom_recvbits(sc, &val, 16);
		data[i] = val & 0xffff;

		/* Clear CHIP SELECT. */
		reg = CSR_READ(sc, WMREG_EECD) & ~EECD_CS;
		CSR_WRITE(sc, WMREG_EECD, reg);
		delay(2);
	}

	return (0);
}

/*
 * wm_spi_eeprom_ready:
 *
 *	Wait for a SPI EEPROM to be ready for commands.
 */
static int
wm_spi_eeprom_ready(struct wm_softc *sc)
{
	uint32_t val;
	int usec;

	for (usec = 0; usec < SPI_MAX_RETRIES; delay(5), usec += 5) {
		wm_eeprom_sendbits(sc, SPI_OPC_RDSR, 8);
		wm_eeprom_recvbits(sc, &val, 8);
		if ((val & SPI_SR_RDY) == 0)
			break;
	}
	if (usec >= SPI_MAX_RETRIES) {
		aprint_error("%s: EEPROM failed to become ready\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}
	return (0);
}

/*
 * wm_read_eeprom_spi:
 *
 *	Read a work from the EEPROM using the SPI protocol.
 */
static int
wm_read_eeprom_spi(struct wm_softc *sc, int word, int wordcnt, uint16_t *data)
{
	uint32_t reg, val;
	int i;
	uint8_t opc;

	/* Clear SK and CS. */
	reg = CSR_READ(sc, WMREG_EECD) & ~(EECD_SK | EECD_CS);
	CSR_WRITE(sc, WMREG_EECD, reg);
	delay(2);

	if (wm_spi_eeprom_ready(sc))
		return (1);

	/* Toggle CS to flush commands. */
	CSR_WRITE(sc, WMREG_EECD, reg | EECD_CS);
	delay(2);
	CSR_WRITE(sc, WMREG_EECD, reg);
	delay(2);

	opc = SPI_OPC_READ;
	if (sc->sc_ee_addrbits == 8 && word >= 128)
		opc |= SPI_OPC_A8;

	wm_eeprom_sendbits(sc, opc, 8);
	wm_eeprom_sendbits(sc, word << 1, sc->sc_ee_addrbits);

	for (i = 0; i < wordcnt; i++) {
		wm_eeprom_recvbits(sc, &val, 16);
		data[i] = ((val >> 8) & 0xff) | ((val & 0xff) << 8);
	}

	/* Raise CS and clear SK. */
	reg = (CSR_READ(sc, WMREG_EECD) & ~EECD_SK) | EECD_CS;
	CSR_WRITE(sc, WMREG_EECD, reg);
	delay(2);

	return (0);
}

#define EEPROM_CHECKSUM		0xBABA
#define EEPROM_SIZE		0x0040

/*
 * wm_validate_eeprom_checksum
 *
 * The checksum is defined as the sum of the first 64 (16 bit) words.
 */
static int
wm_validate_eeprom_checksum(struct wm_softc *sc)
{   
	uint16_t checksum;
	uint16_t eeprom_data;
	int i;

	checksum = 0;

	for (i = 0; i < EEPROM_SIZE; i++) {
		if (wm_read_eeprom(sc, i, 1, &eeprom_data))
			return 1;
		checksum += eeprom_data;
	}

	if (checksum != (uint16_t) EEPROM_CHECKSUM)
		return 1;

	return 0;
}

/*
 * wm_read_eeprom:
 *
 *	Read data from the serial EEPROM.
 */
static int
wm_read_eeprom(struct wm_softc *sc, int word, int wordcnt, uint16_t *data)
{
	int rv;

	if (sc->sc_flags & WM_F_EEPROM_INVALID)
		return 1;

	if (wm_acquire_eeprom(sc))
		return 1;

	if (sc->sc_type == WM_T_ICH8)
		rv = wm_read_eeprom_ich8(sc, word, wordcnt, data);
	else if (sc->sc_flags & WM_F_EEPROM_EERDEEWR)
		rv = wm_read_eeprom_eerd(sc, word, wordcnt, data);
	else if (sc->sc_flags & WM_F_EEPROM_SPI)
		rv = wm_read_eeprom_spi(sc, word, wordcnt, data);
	else
		rv = wm_read_eeprom_uwire(sc, word, wordcnt, data);

	wm_release_eeprom(sc);
	return rv;
}

static int
wm_read_eeprom_eerd(struct wm_softc *sc, int offset, int wordcnt,
    uint16_t *data)
{
	int i, eerd = 0;
	int error = 0;

	for (i = 0; i < wordcnt; i++) {
		eerd = ((offset + i) << EERD_ADDR_SHIFT) | EERD_START;

		CSR_WRITE(sc, WMREG_EERD, eerd);
		error = wm_poll_eerd_eewr_done(sc, WMREG_EERD);
		if (error != 0)
			break;

		data[i] = (CSR_READ(sc, WMREG_EERD) >> EERD_DATA_SHIFT);
	}

	return error;
}

static int
wm_poll_eerd_eewr_done(struct wm_softc *sc, int rw)
{
	uint32_t attempts = 100000;
	uint32_t i, reg = 0;
	int32_t done = -1;

	for (i = 0; i < attempts; i++) {
		reg = CSR_READ(sc, rw);

		if (reg & EERD_DONE) {
			done = 0;
			break;
		}
		delay(5);
	}

	return done;
}

/*
 * wm_add_rxbuf:
 *
 *	Add a receive buffer to the indiciated descriptor.
 */
static int
wm_add_rxbuf(struct wm_softc *sc, int idx)
{
	struct wm_rxsoft *rxs = &sc->sc_rxsoft[idx];
	struct mbuf *m;
	int error;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return (ENOBUFS);

	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return (ENOBUFS);
	}

	if (rxs->rxs_mbuf != NULL)
		bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);

	rxs->rxs_mbuf = m;

	m->m_len = m->m_pkthdr.len = m->m_ext.ext_size;
	error = bus_dmamap_load_mbuf(sc->sc_dmat, rxs->rxs_dmamap, m,
	    BUS_DMA_READ|BUS_DMA_NOWAIT);
	if (error) {
		/* XXX XXX XXX */
		printf("%s: unable to load rx DMA map %d, error = %d\n",
		    sc->sc_dev.dv_xname, idx, error);
		panic("wm_add_rxbuf");
	}

	bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
	    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);

	WM_INIT_RXDESC(sc, idx);

	return (0);
}

/*
 * wm_set_ral:
 *
 *	Set an entery in the receive address list.
 */
static void
wm_set_ral(struct wm_softc *sc, const uint8_t *enaddr, int idx)
{
	uint32_t ral_lo, ral_hi;

	if (enaddr != NULL) {
		ral_lo = enaddr[0] | (enaddr[1] << 8) | (enaddr[2] << 16) |
		    (enaddr[3] << 24);
		ral_hi = enaddr[4] | (enaddr[5] << 8);
		ral_hi |= RAL_AV;
	} else {
		ral_lo = 0;
		ral_hi = 0;
	}

	if (sc->sc_type >= WM_T_82544) {
		CSR_WRITE(sc, WMREG_RAL_LO(WMREG_CORDOVA_RAL_BASE, idx),
		    ral_lo);
		CSR_WRITE(sc, WMREG_RAL_HI(WMREG_CORDOVA_RAL_BASE, idx),
		    ral_hi);
	} else {
		CSR_WRITE(sc, WMREG_RAL_LO(WMREG_RAL_BASE, idx), ral_lo);
		CSR_WRITE(sc, WMREG_RAL_HI(WMREG_RAL_BASE, idx), ral_hi);
	}
}

/*
 * wm_mchash:
 *
 *	Compute the hash of the multicast address for the 4096-bit
 *	multicast filter.
 */
static uint32_t
wm_mchash(struct wm_softc *sc, const uint8_t *enaddr)
{
	static const int lo_shift[4] = { 4, 3, 2, 0 };
	static const int hi_shift[4] = { 4, 5, 6, 8 };
	static const int ich8_lo_shift[4] = { 6, 5, 4, 2 };
	static const int ich8_hi_shift[4] = { 2, 3, 4, 6 };
	uint32_t hash;

	if (sc->sc_type == WM_T_ICH8) {
		hash = (enaddr[4] >> ich8_lo_shift[sc->sc_mchash_type]) |
		    (((uint16_t) enaddr[5]) << ich8_hi_shift[sc->sc_mchash_type]);
		return (hash & 0x3ff);
	}
	hash = (enaddr[4] >> lo_shift[sc->sc_mchash_type]) |
	    (((uint16_t) enaddr[5]) << hi_shift[sc->sc_mchash_type]);

	return (hash & 0xfff);
}

/*
 * wm_set_filter:
 *
 *	Set up the receive filter.
 */
static void
wm_set_filter(struct wm_softc *sc)
{
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	bus_addr_t mta_reg;
	uint32_t hash, reg, bit;
	int i, size;

	if (sc->sc_type >= WM_T_82544)
		mta_reg = WMREG_CORDOVA_MTA;
	else
		mta_reg = WMREG_MTA;

	sc->sc_rctl &= ~(RCTL_BAM | RCTL_UPE | RCTL_MPE);

	if (ifp->if_flags & IFF_BROADCAST)
		sc->sc_rctl |= RCTL_BAM;
	if (ifp->if_flags & IFF_PROMISC) {
		sc->sc_rctl |= RCTL_UPE;
		goto allmulti;
	}

	/*
	 * Set the station address in the first RAL slot, and
	 * clear the remaining slots.
	 */
	if (sc->sc_type == WM_T_ICH8)
		size = WM_ICH8_RAL_TABSIZE;
	else
		size = WM_RAL_TABSIZE;
	wm_set_ral(sc, LLADDR(ifp->if_sadl), 0);
	for (i = 1; i < size; i++)
		wm_set_ral(sc, NULL, i);

	if (sc->sc_type == WM_T_ICH8)
		size = WM_ICH8_MC_TABSIZE;
	else
		size = WM_MC_TABSIZE;
	/* Clear out the multicast table. */
	for (i = 0; i < size; i++)
		CSR_WRITE(sc, mta_reg + (i << 2), 0);

	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			goto allmulti;
		}

		hash = wm_mchash(sc, enm->enm_addrlo);

		reg = (hash >> 5);
		if (sc->sc_type == WM_T_ICH8)
			reg &= 0x1f;
		else
			reg &= 0x7f;
		bit = hash & 0x1f;

		hash = CSR_READ(sc, mta_reg + (reg << 2));
		hash |= 1U << bit;

		/* XXX Hardware bug?? */
		if (sc->sc_type == WM_T_82544 && (reg & 0xe) == 1) {
			bit = CSR_READ(sc, mta_reg + ((reg - 1) << 2));
			CSR_WRITE(sc, mta_reg + (reg << 2), hash);
			CSR_WRITE(sc, mta_reg + ((reg - 1) << 2), bit);
		} else
			CSR_WRITE(sc, mta_reg + (reg << 2), hash);

		ETHER_NEXT_MULTI(step, enm);
	}

	ifp->if_flags &= ~IFF_ALLMULTI;
	goto setit;

 allmulti:
	ifp->if_flags |= IFF_ALLMULTI;
	sc->sc_rctl |= RCTL_MPE;

 setit:
	CSR_WRITE(sc, WMREG_RCTL, sc->sc_rctl);
}

/*
 * wm_tbi_mediainit:
 *
 *	Initialize media for use on 1000BASE-X devices.
 */
static void
wm_tbi_mediainit(struct wm_softc *sc)
{
	const char *sep = "";

	if (sc->sc_type < WM_T_82543)
		sc->sc_tipg = TIPG_WM_DFLT;
	else
		sc->sc_tipg = TIPG_LG_DFLT;

	ifmedia_init(&sc->sc_mii.mii_media, IFM_IMASK, wm_tbi_mediachange,
	    wm_tbi_mediastatus);

	/*
	 * SWD Pins:
	 *
	 *	0 = Link LED (output)
	 *	1 = Loss Of Signal (input)
	 */
	sc->sc_ctrl |= CTRL_SWDPIO(0);
	sc->sc_ctrl &= ~CTRL_SWDPIO(1);

	CSR_WRITE(sc, WMREG_CTRL, sc->sc_ctrl);

#define	ADD(ss, mm, dd)							\
do {									\
	aprint_normal("%s%s", sep, ss);					\
	ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|(mm), (dd), NULL);	\
	sep = ", ";							\
} while (/*CONSTCOND*/0)

	aprint_normal("%s: ", sc->sc_dev.dv_xname);
	ADD("1000baseSX", IFM_1000_SX, ANAR_X_HD);
	ADD("1000baseSX-FDX", IFM_1000_SX|IFM_FDX, ANAR_X_FD);
	ADD("auto", IFM_AUTO, ANAR_X_FD|ANAR_X_HD);
	aprint_normal("\n");

#undef ADD

	ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);
}

/*
 * wm_tbi_mediastatus:	[ifmedia interface function]
 *
 *	Get the current interface media status on a 1000BASE-X device.
 */
static void
wm_tbi_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct wm_softc *sc = ifp->if_softc;
	uint32_t ctrl;

	ifmr->ifm_status = IFM_AVALID;
	ifmr->ifm_active = IFM_ETHER;

	if (sc->sc_tbi_linkup == 0) {
		ifmr->ifm_active |= IFM_NONE;
		return;
	}

	ifmr->ifm_status |= IFM_ACTIVE;
	ifmr->ifm_active |= IFM_1000_SX;
	if (CSR_READ(sc, WMREG_STATUS) & STATUS_FD)
		ifmr->ifm_active |= IFM_FDX;
	ctrl = CSR_READ(sc, WMREG_CTRL);
	if (ctrl & CTRL_RFCE)
		ifmr->ifm_active |= IFM_FLOW | IFM_ETH_RXPAUSE;
	if (ctrl & CTRL_TFCE)
		ifmr->ifm_active |= IFM_FLOW | IFM_ETH_TXPAUSE;
}

/*
 * wm_tbi_mediachange:	[ifmedia interface function]
 *
 *	Set hardware to newly-selected media on a 1000BASE-X device.
 */
static int
wm_tbi_mediachange(struct ifnet *ifp)
{
	struct wm_softc *sc = ifp->if_softc;
	struct ifmedia_entry *ife = sc->sc_mii.mii_media.ifm_cur;
	uint32_t status;
	int i;

	sc->sc_txcw = ife->ifm_data;
	DPRINTF(WM_DEBUG_LINK,("%s: sc_txcw = 0x%x on entry\n",
		    sc->sc_dev.dv_xname,sc->sc_txcw));
	if (IFM_SUBTYPE(ife->ifm_media) == IFM_AUTO ||
	    (sc->sc_mii.mii_media.ifm_media & IFM_FLOW) != 0)
		sc->sc_txcw |= ANAR_X_PAUSE_SYM | ANAR_X_PAUSE_ASYM;
	if (IFM_SUBTYPE(ife->ifm_media) == IFM_AUTO) { 
		sc->sc_txcw |= TXCW_ANE; 
	} else {
		/*If autonegotiation is turned off, force link up and turn on full duplex*/
		sc->sc_txcw &= ~TXCW_ANE;
		sc->sc_ctrl |= CTRL_SLU | CTRL_FD;
		CSR_WRITE(sc, WMREG_CTRL, sc->sc_ctrl);
		delay(1000);
	}

	DPRINTF(WM_DEBUG_LINK,("%s: sc_txcw = 0x%x after autoneg check\n",
		    sc->sc_dev.dv_xname,sc->sc_txcw));
	CSR_WRITE(sc, WMREG_TXCW, sc->sc_txcw);
	delay(10000);

	/* NOTE: CTRL will update TFCE and RFCE automatically. */

	sc->sc_tbi_anstate = 0;

	i = CSR_READ(sc, WMREG_CTRL) & CTRL_SWDPIN(1);
	DPRINTF(WM_DEBUG_LINK,("%s: i = 0x%x\n", sc->sc_dev.dv_xname,i));

	/* 
	 * On 82544 chips and later, the CTRL_SWDPIN(1) bit will be set if the
	 * optics detect a signal, 0 if they don't.
	 */
	if (((i != 0) && (sc->sc_type >= WM_T_82544)) || (i == 0)) {
		/* Have signal; wait for the link to come up. */

		if (IFM_SUBTYPE(ife->ifm_media) == IFM_AUTO) {
			/*
			 * Reset the link, and let autonegotiation do its thing
			 */
			sc->sc_ctrl |= CTRL_LRST;
			CSR_WRITE(sc, WMREG_CTRL, sc->sc_ctrl);
			delay(1000);
			sc->sc_ctrl &= ~CTRL_LRST;
			CSR_WRITE(sc, WMREG_CTRL, sc->sc_ctrl);
			delay(1000);
		}

		for (i = 0; i < 50; i++) {
			delay(10000);
			if (CSR_READ(sc, WMREG_STATUS) & STATUS_LU)
				break;
		}

		DPRINTF(WM_DEBUG_LINK,("%s: i = %d after waiting for link\n",
			    sc->sc_dev.dv_xname,i));

		status = CSR_READ(sc, WMREG_STATUS);
		DPRINTF(WM_DEBUG_LINK,
		    ("%s: status after final read = 0x%x, STATUS_LU = 0x%x\n",
			sc->sc_dev.dv_xname,status, STATUS_LU));
		if (status & STATUS_LU) {
			/* Link is up. */
			DPRINTF(WM_DEBUG_LINK,
			    ("%s: LINK: set media -> link up %s\n",
			    sc->sc_dev.dv_xname,
			    (status & STATUS_FD) ? "FDX" : "HDX"));
			sc->sc_tctl &= ~TCTL_COLD(0x3ff);
			sc->sc_fcrtl &= ~FCRTL_XONE;
			if (status & STATUS_FD)
				sc->sc_tctl |=
				    TCTL_COLD(TX_COLLISION_DISTANCE_FDX);
			else
				sc->sc_tctl |=
				    TCTL_COLD(TX_COLLISION_DISTANCE_HDX);
			if (CSR_READ(sc, WMREG_CTRL) & CTRL_TFCE)
				sc->sc_fcrtl |= FCRTL_XONE;
			CSR_WRITE(sc, WMREG_TCTL, sc->sc_tctl);
			CSR_WRITE(sc, (sc->sc_type < WM_T_82543) ?
				      WMREG_OLD_FCRTL : WMREG_FCRTL,
				      sc->sc_fcrtl);
			sc->sc_tbi_linkup = 1;
		} else {
			/* Link is down. */
			DPRINTF(WM_DEBUG_LINK,
			    ("%s: LINK: set media -> link down\n",
			    sc->sc_dev.dv_xname));
			sc->sc_tbi_linkup = 0;
		}
	} else {
		DPRINTF(WM_DEBUG_LINK, ("%s: LINK: set media -> no signal\n",
		    sc->sc_dev.dv_xname));
		sc->sc_tbi_linkup = 0;
	}

	wm_tbi_set_linkled(sc);

	return (0);
}

/*
 * wm_tbi_set_linkled:
 *
 *	Update the link LED on 1000BASE-X devices.
 */
static void
wm_tbi_set_linkled(struct wm_softc *sc)
{

	if (sc->sc_tbi_linkup)
		sc->sc_ctrl |= CTRL_SWDPIN(0);
	else
		sc->sc_ctrl &= ~CTRL_SWDPIN(0);

	CSR_WRITE(sc, WMREG_CTRL, sc->sc_ctrl);
}

/*
 * wm_tbi_check_link:
 *
 *	Check the link on 1000BASE-X devices.
 */
static void
wm_tbi_check_link(struct wm_softc *sc)
{
	uint32_t rxcw, ctrl, status;

	if (sc->sc_tbi_anstate == 0)
		return;
	else if (sc->sc_tbi_anstate > 1) {
		DPRINTF(WM_DEBUG_LINK,
		    ("%s: LINK: anstate %d\n", sc->sc_dev.dv_xname,
		    sc->sc_tbi_anstate));
		sc->sc_tbi_anstate--;
		return;
	}

	sc->sc_tbi_anstate = 0;

	rxcw = CSR_READ(sc, WMREG_RXCW);
	ctrl = CSR_READ(sc, WMREG_CTRL);
	status = CSR_READ(sc, WMREG_STATUS);

	if ((status & STATUS_LU) == 0) {
		DPRINTF(WM_DEBUG_LINK,
		    ("%s: LINK: checklink -> down\n", sc->sc_dev.dv_xname));
		sc->sc_tbi_linkup = 0;
	} else {
		DPRINTF(WM_DEBUG_LINK,
		    ("%s: LINK: checklink -> up %s\n", sc->sc_dev.dv_xname,
		    (status & STATUS_FD) ? "FDX" : "HDX"));
		sc->sc_tctl &= ~TCTL_COLD(0x3ff);
		sc->sc_fcrtl &= ~FCRTL_XONE;
		if (status & STATUS_FD)
			sc->sc_tctl |=
			    TCTL_COLD(TX_COLLISION_DISTANCE_FDX);
		else
			sc->sc_tctl |=
			    TCTL_COLD(TX_COLLISION_DISTANCE_HDX);
		if (ctrl & CTRL_TFCE)
			sc->sc_fcrtl |= FCRTL_XONE;
		CSR_WRITE(sc, WMREG_TCTL, sc->sc_tctl);
		CSR_WRITE(sc, (sc->sc_type < WM_T_82543) ?
			      WMREG_OLD_FCRTL : WMREG_FCRTL,
			      sc->sc_fcrtl);
		sc->sc_tbi_linkup = 1;
	}

	wm_tbi_set_linkled(sc);
}

/*
 * wm_gmii_reset:
 *
 *	Reset the PHY.
 */
static void
wm_gmii_reset(struct wm_softc *sc)
{
	uint32_t reg;
	int func = 0; /* XXX gcc */

	if (sc->sc_type == WM_T_ICH8) {
		if (wm_get_swfwhw_semaphore(sc))
			return;
	}
	if (sc->sc_type == WM_T_80003) {
		func = (CSR_READ(sc, WMREG_STATUS) >> STATUS_FUNCID_SHIFT) & 1;
		if (wm_get_swfw_semaphore(sc,
		    func ? SWFW_PHY1_SM : SWFW_PHY0_SM))
			return;
	}
	if (sc->sc_type >= WM_T_82544) {
		CSR_WRITE(sc, WMREG_CTRL, sc->sc_ctrl | CTRL_PHY_RESET);
		delay(20000);

		CSR_WRITE(sc, WMREG_CTRL, sc->sc_ctrl);
		delay(20000);
	} else {
                /*
                 * With 82543, we need to force speed and duplex on the MAC
                 * equal to what the PHY speed and duplex configuration is.
                 * In addition, we need to perform a hardware reset on the PHY
                 * to take it out of reset.
                 */
                sc->sc_ctrl |= CTRL_FRCSPD | CTRL_FRCFDX;
                CSR_WRITE(sc, WMREG_CTRL, sc->sc_ctrl);

		/* The PHY reset pin is active-low. */
		reg = CSR_READ(sc, WMREG_CTRL_EXT);
		reg &= ~((CTRL_EXT_SWDPIO_MASK << CTRL_EXT_SWDPIO_SHIFT) |
		    CTRL_EXT_SWDPIN(4));
		reg |= CTRL_EXT_SWDPIO(4);

		CSR_WRITE(sc, WMREG_CTRL_EXT, reg | CTRL_EXT_SWDPIN(4));
		delay(10);

		CSR_WRITE(sc, WMREG_CTRL_EXT, reg);
		delay(10000);

		CSR_WRITE(sc, WMREG_CTRL_EXT, reg | CTRL_EXT_SWDPIN(4));
		delay(10);
#if 0
		sc->sc_ctrl_ext = reg | CTRL_EXT_SWDPIN(4);
#endif
	}
	if (sc->sc_type == WM_T_ICH8)
		wm_put_swfwhw_semaphore(sc);
	if (sc->sc_type == WM_T_80003)
		wm_put_swfw_semaphore(sc, func ? SWFW_PHY1_SM : SWFW_PHY0_SM);
}

/*
 * wm_gmii_mediainit:
 *
 *	Initialize media for use on 1000BASE-T devices.
 */
static void
wm_gmii_mediainit(struct wm_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	/* We have MII. */
	sc->sc_flags |= WM_F_HAS_MII;

	if (sc->sc_type >= WM_T_80003)
		sc->sc_tipg =  TIPG_1000T_80003_DFLT;
	else
		sc->sc_tipg = TIPG_1000T_DFLT;

	/*
	 * Let the chip set speed/duplex on its own based on
	 * signals from the PHY.
	 * XXXbouyer - I'm not sure this is right for the 80003,
	 * the em driver only sets CTRL_SLU here - but it seems to work.
	 */
	sc->sc_ctrl |= CTRL_SLU;
	CSR_WRITE(sc, WMREG_CTRL, sc->sc_ctrl);

	/* Initialize our media structures and probe the GMII. */
	sc->sc_mii.mii_ifp = ifp;

	if (sc->sc_type >= WM_T_80003) {
		sc->sc_mii.mii_readreg = wm_gmii_i80003_readreg;
		sc->sc_mii.mii_writereg = wm_gmii_i80003_writereg;
	} else if (sc->sc_type >= WM_T_82544) {
		sc->sc_mii.mii_readreg = wm_gmii_i82544_readreg;
		sc->sc_mii.mii_writereg = wm_gmii_i82544_writereg;
	} else {
		sc->sc_mii.mii_readreg = wm_gmii_i82543_readreg;
		sc->sc_mii.mii_writereg = wm_gmii_i82543_writereg;
	}
	sc->sc_mii.mii_statchg = wm_gmii_statchg;

	wm_gmii_reset(sc);

	ifmedia_init(&sc->sc_mii.mii_media, IFM_IMASK, wm_gmii_mediachange,
	    wm_gmii_mediastatus);

	mii_attach(&sc->sc_dev, &sc->sc_mii, 0xffffffff, MII_PHY_ANY,
	    MII_OFFSET_ANY, MIIF_DOPAUSE);
	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE);
	} else
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);
}

/*
 * wm_gmii_mediastatus:	[ifmedia interface function]
 *
 *	Get the current interface media status on a 1000BASE-T device.
 */
static void
wm_gmii_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct wm_softc *sc = ifp->if_softc;

	mii_pollstat(&sc->sc_mii);
	ifmr->ifm_status = sc->sc_mii.mii_media_status;
	ifmr->ifm_active = (sc->sc_mii.mii_media_active & ~IFM_ETH_FMASK) |
			   sc->sc_flowflags;
}

/*
 * wm_gmii_mediachange:	[ifmedia interface function]
 *
 *	Set hardware to newly-selected media on a 1000BASE-T device.
 */
static int
wm_gmii_mediachange(struct ifnet *ifp)
{
	struct wm_softc *sc = ifp->if_softc;
	struct ifmedia_entry *ife = sc->sc_mii.mii_media.ifm_cur;

	if (ifp->if_flags & IFF_UP) {
		sc->sc_ctrl &= ~(CTRL_SPEED_MASK | CTRL_FD);
		sc->sc_ctrl |= CTRL_SLU;
		if ((IFM_SUBTYPE(ife->ifm_media) == IFM_AUTO)
		    || (sc->sc_type > WM_T_82543)) {
			sc->sc_ctrl &= ~(CTRL_FRCSPD | CTRL_FRCFDX);
		} else {
			sc->sc_ctrl &= ~CTRL_ASDE;
			sc->sc_ctrl |= CTRL_FRCSPD | CTRL_FRCFDX;
			if (ife->ifm_media & IFM_FDX)
				sc->sc_ctrl |= CTRL_FD;
			switch(IFM_SUBTYPE(ife->ifm_media)) {
			case IFM_10_T:
				sc->sc_ctrl |= CTRL_SPEED_10;
				break;
			case IFM_100_TX:
				sc->sc_ctrl |= CTRL_SPEED_100;
				break;
			case IFM_1000_T:
				sc->sc_ctrl |= CTRL_SPEED_1000;
				break;
			default:
				panic("wm_gmii_mediachange: bad media 0x%x",
				    ife->ifm_media);
			}
		}
		CSR_WRITE(sc, WMREG_CTRL, sc->sc_ctrl);
		if (sc->sc_type <= WM_T_82543)
			wm_gmii_reset(sc);
		mii_mediachg(&sc->sc_mii);
	}
	return (0);
}

#define	MDI_IO		CTRL_SWDPIN(2)
#define	MDI_DIR		CTRL_SWDPIO(2)	/* host -> PHY */
#define	MDI_CLK		CTRL_SWDPIN(3)

static void
i82543_mii_sendbits(struct wm_softc *sc, uint32_t data, int nbits)
{
	uint32_t i, v;

	v = CSR_READ(sc, WMREG_CTRL);
	v &= ~(MDI_IO|MDI_CLK|(CTRL_SWDPIO_MASK << CTRL_SWDPIO_SHIFT));
	v |= MDI_DIR | CTRL_SWDPIO(3);

	for (i = 1 << (nbits - 1); i != 0; i >>= 1) {
		if (data & i)
			v |= MDI_IO;
		else
			v &= ~MDI_IO;
		CSR_WRITE(sc, WMREG_CTRL, v);
		delay(10);
		CSR_WRITE(sc, WMREG_CTRL, v | MDI_CLK);
		delay(10);
		CSR_WRITE(sc, WMREG_CTRL, v);
		delay(10);
	}
}

static uint32_t
i82543_mii_recvbits(struct wm_softc *sc)
{
	uint32_t v, i, data = 0;

	v = CSR_READ(sc, WMREG_CTRL);
	v &= ~(MDI_IO|MDI_CLK|(CTRL_SWDPIO_MASK << CTRL_SWDPIO_SHIFT));
	v |= CTRL_SWDPIO(3);

	CSR_WRITE(sc, WMREG_CTRL, v);
	delay(10);
	CSR_WRITE(sc, WMREG_CTRL, v | MDI_CLK);
	delay(10);
	CSR_WRITE(sc, WMREG_CTRL, v);
	delay(10);

	for (i = 0; i < 16; i++) {
		data <<= 1;
		CSR_WRITE(sc, WMREG_CTRL, v | MDI_CLK);
		delay(10);
		if (CSR_READ(sc, WMREG_CTRL) & MDI_IO)
			data |= 1;
		CSR_WRITE(sc, WMREG_CTRL, v);
		delay(10);
	}

	CSR_WRITE(sc, WMREG_CTRL, v | MDI_CLK);
	delay(10);
	CSR_WRITE(sc, WMREG_CTRL, v);
	delay(10);

	return (data);
}

#undef MDI_IO
#undef MDI_DIR
#undef MDI_CLK

/*
 * wm_gmii_i82543_readreg:	[mii interface function]
 *
 *	Read a PHY register on the GMII (i82543 version).
 */
static int
wm_gmii_i82543_readreg(struct device *self, int phy, int reg)
{
	struct wm_softc *sc = (void *) self;
	int rv;

	i82543_mii_sendbits(sc, 0xffffffffU, 32);
	i82543_mii_sendbits(sc, reg | (phy << 5) |
	    (MII_COMMAND_READ << 10) | (MII_COMMAND_START << 12), 14);
	rv = i82543_mii_recvbits(sc) & 0xffff;

	DPRINTF(WM_DEBUG_GMII,
	    ("%s: GMII: read phy %d reg %d -> 0x%04x\n",
	    sc->sc_dev.dv_xname, phy, reg, rv));

	return (rv);
}

/*
 * wm_gmii_i82543_writereg:	[mii interface function]
 *
 *	Write a PHY register on the GMII (i82543 version).
 */
static void
wm_gmii_i82543_writereg(struct device *self, int phy, int reg, int val)
{
	struct wm_softc *sc = (void *) self;

	i82543_mii_sendbits(sc, 0xffffffffU, 32);
	i82543_mii_sendbits(sc, val | (MII_COMMAND_ACK << 16) |
	    (reg << 18) | (phy << 23) | (MII_COMMAND_WRITE << 28) |
	    (MII_COMMAND_START << 30), 32);
}

/*
 * wm_gmii_i82544_readreg:	[mii interface function]
 *
 *	Read a PHY register on the GMII.
 */
static int
wm_gmii_i82544_readreg(struct device *self, int phy, int reg)
{
	struct wm_softc *sc = (void *) self;
	uint32_t mdic = 0;
	int i, rv;

	CSR_WRITE(sc, WMREG_MDIC, MDIC_OP_READ | MDIC_PHYADD(phy) |
	    MDIC_REGADD(reg));

	for (i = 0; i < 320; i++) {
		mdic = CSR_READ(sc, WMREG_MDIC);
		if (mdic & MDIC_READY)
			break;
		delay(10);
	}

	if ((mdic & MDIC_READY) == 0) {
		log(LOG_WARNING, "%s: MDIC read timed out: phy %d reg %d\n",
		    sc->sc_dev.dv_xname, phy, reg);
		rv = 0;
	} else if (mdic & MDIC_E) {
#if 0 /* This is normal if no PHY is present. */
		log(LOG_WARNING, "%s: MDIC read error: phy %d reg %d\n",
		    sc->sc_dev.dv_xname, phy, reg);
#endif
		rv = 0;
	} else {
		rv = MDIC_DATA(mdic);
		if (rv == 0xffff)
			rv = 0;
	}

	return (rv);
}

/*
 * wm_gmii_i82544_writereg:	[mii interface function]
 *
 *	Write a PHY register on the GMII.
 */
static void
wm_gmii_i82544_writereg(struct device *self, int phy, int reg, int val)
{
	struct wm_softc *sc = (void *) self;
	uint32_t mdic = 0;
	int i;

	CSR_WRITE(sc, WMREG_MDIC, MDIC_OP_WRITE | MDIC_PHYADD(phy) |
	    MDIC_REGADD(reg) | MDIC_DATA(val));

	for (i = 0; i < 320; i++) {
		mdic = CSR_READ(sc, WMREG_MDIC);
		if (mdic & MDIC_READY)
			break;
		delay(10);
	}

	if ((mdic & MDIC_READY) == 0)
		log(LOG_WARNING, "%s: MDIC write timed out: phy %d reg %d\n",
		    sc->sc_dev.dv_xname, phy, reg);
	else if (mdic & MDIC_E)
		log(LOG_WARNING, "%s: MDIC write error: phy %d reg %d\n",
		    sc->sc_dev.dv_xname, phy, reg);
}

/*
 * wm_gmii_i80003_readreg:	[mii interface function]
 *
 *	Read a PHY register on the kumeran
 * This could be handled by the PHY layer if we didn't have to lock the
 * ressource ...
 */
static int
wm_gmii_i80003_readreg(struct device *self, int phy, int reg)
{
	struct wm_softc *sc = (void *) self;
	int func = ((CSR_READ(sc, WMREG_STATUS) >> STATUS_FUNCID_SHIFT) & 1);
	int rv;

	if (phy != 1) /* only one PHY on kumeran bus */
		return 0;

	if (wm_get_swfw_semaphore(sc, func ? SWFW_PHY1_SM : SWFW_PHY0_SM))
		return 0;

	if ((reg & GG82563_MAX_REG_ADDRESS) < GG82563_MIN_ALT_REG) {
		wm_gmii_i82544_writereg(self, phy, GG82563_PHY_PAGE_SELECT,
		    reg >> GG82563_PAGE_SHIFT);
	} else {
		wm_gmii_i82544_writereg(self, phy, GG82563_PHY_PAGE_SELECT_ALT,
		    reg >> GG82563_PAGE_SHIFT);
	}

	rv = wm_gmii_i82544_readreg(self, phy, reg & GG82563_MAX_REG_ADDRESS);
	wm_put_swfw_semaphore(sc, func ? SWFW_PHY1_SM : SWFW_PHY0_SM);
	return (rv);
}

/*
 * wm_gmii_i80003_writereg:	[mii interface function]
 *
 *	Write a PHY register on the kumeran.
 * This could be handled by the PHY layer if we didn't have to lock the
 * ressource ...
 */
static void
wm_gmii_i80003_writereg(struct device *self, int phy, int reg, int val)
{
	struct wm_softc *sc = (void *) self;
	int func = ((CSR_READ(sc, WMREG_STATUS) >> STATUS_FUNCID_SHIFT) & 1);

	if (phy != 1) /* only one PHY on kumeran bus */
		return;

	if (wm_get_swfw_semaphore(sc, func ? SWFW_PHY1_SM : SWFW_PHY0_SM))
		return;

	if ((reg & GG82563_MAX_REG_ADDRESS) < GG82563_MIN_ALT_REG) {
		wm_gmii_i82544_writereg(self, phy, GG82563_PHY_PAGE_SELECT,
		    reg >> GG82563_PAGE_SHIFT);
	} else {
		wm_gmii_i82544_writereg(self, phy, GG82563_PHY_PAGE_SELECT_ALT,
		    reg >> GG82563_PAGE_SHIFT);
	}

	wm_gmii_i82544_writereg(self, phy, reg & GG82563_MAX_REG_ADDRESS, val);
	wm_put_swfw_semaphore(sc, func ? SWFW_PHY1_SM : SWFW_PHY0_SM);
}

/*
 * wm_gmii_statchg:	[mii interface function]
 *
 *	Callback from MII layer when media changes.
 */
static void
wm_gmii_statchg(struct device *self)
{
	struct wm_softc *sc = (void *) self;
	struct mii_data *mii = &sc->sc_mii;

	sc->sc_ctrl &= ~(CTRL_TFCE | CTRL_RFCE);
	sc->sc_tctl &= ~TCTL_COLD(0x3ff);
	sc->sc_fcrtl &= ~FCRTL_XONE;

	/*
	 * Get flow control negotiation result.
	 */
	if (IFM_SUBTYPE(mii->mii_media.ifm_cur->ifm_media) == IFM_AUTO &&
	    (mii->mii_media_active & IFM_ETH_FMASK) != sc->sc_flowflags) {
		sc->sc_flowflags = mii->mii_media_active & IFM_ETH_FMASK;
		mii->mii_media_active &= ~IFM_ETH_FMASK;
	}

	if (sc->sc_flowflags & IFM_FLOW) {
		if (sc->sc_flowflags & IFM_ETH_TXPAUSE) {
			sc->sc_ctrl |= CTRL_TFCE;
			sc->sc_fcrtl |= FCRTL_XONE;
		}
		if (sc->sc_flowflags & IFM_ETH_RXPAUSE)
			sc->sc_ctrl |= CTRL_RFCE;
	}

	if (sc->sc_mii.mii_media_active & IFM_FDX) {
		DPRINTF(WM_DEBUG_LINK,
		    ("%s: LINK: statchg: FDX\n", sc->sc_dev.dv_xname));
		sc->sc_tctl |= TCTL_COLD(TX_COLLISION_DISTANCE_FDX);
	} else  {
		DPRINTF(WM_DEBUG_LINK,
		    ("%s: LINK: statchg: HDX\n", sc->sc_dev.dv_xname));
		sc->sc_tctl |= TCTL_COLD(TX_COLLISION_DISTANCE_HDX);
	}

	CSR_WRITE(sc, WMREG_CTRL, sc->sc_ctrl);
	CSR_WRITE(sc, WMREG_TCTL, sc->sc_tctl);
	CSR_WRITE(sc, (sc->sc_type < WM_T_82543) ? WMREG_OLD_FCRTL
						 : WMREG_FCRTL, sc->sc_fcrtl);
	if (sc->sc_type >= WM_T_80003) {
		switch(IFM_SUBTYPE(sc->sc_mii.mii_media_active)) {
		case IFM_1000_T:
			wm_kmrn_i80003_writereg(sc, KUMCTRLSTA_OFFSET_HD_CTRL,
			    KUMCTRLSTA_HD_CTRL_1000_DEFAULT);
			sc->sc_tipg =  TIPG_1000T_80003_DFLT;
			break;
		default:
			wm_kmrn_i80003_writereg(sc, KUMCTRLSTA_OFFSET_HD_CTRL,
			    KUMCTRLSTA_HD_CTRL_10_100_DEFAULT);
			sc->sc_tipg =  TIPG_10_100_80003_DFLT;
			break;
		}
		CSR_WRITE(sc, WMREG_TIPG, sc->sc_tipg);
	}
}

/*
 * wm_kmrn_i80003_readreg:
 *
 *	Read a kumeran register
 */
static int
wm_kmrn_i80003_readreg(struct wm_softc *sc, int reg)
{
	int func = ((CSR_READ(sc, WMREG_STATUS) >> STATUS_FUNCID_SHIFT) & 1);
	int rv;

	if (wm_get_swfw_semaphore(sc, func ? SWFW_PHY1_SM : SWFW_PHY0_SM))
		return 0;

	CSR_WRITE(sc, WMREG_KUMCTRLSTA,
	    ((reg << KUMCTRLSTA_OFFSET_SHIFT) & KUMCTRLSTA_OFFSET) |
	    KUMCTRLSTA_REN);
	delay(2);

	rv = CSR_READ(sc, WMREG_KUMCTRLSTA) & KUMCTRLSTA_MASK;
	wm_put_swfw_semaphore(sc, func ? SWFW_PHY1_SM : SWFW_PHY0_SM);
	return (rv);
}

/*
 * wm_kmrn_i80003_writereg:
 *
 *	Write a kumeran register
 */
static void
wm_kmrn_i80003_writereg(struct wm_softc *sc, int reg, int val)
{
	int func = ((CSR_READ(sc, WMREG_STATUS) >> STATUS_FUNCID_SHIFT) & 1);

	if (wm_get_swfw_semaphore(sc, func ? SWFW_PHY1_SM : SWFW_PHY0_SM))
		return;

	CSR_WRITE(sc, WMREG_KUMCTRLSTA,
	    ((reg << KUMCTRLSTA_OFFSET_SHIFT) & KUMCTRLSTA_OFFSET) |
	    (val & KUMCTRLSTA_MASK));
	wm_put_swfw_semaphore(sc, func ? SWFW_PHY1_SM : SWFW_PHY0_SM);
}

static int
wm_is_onboard_nvm_eeprom(struct wm_softc *sc)
{
	uint32_t eecd = 0;

	if (sc->sc_type == WM_T_82573) {
		eecd = CSR_READ(sc, WMREG_EECD);

		/* Isolate bits 15 & 16 */
		eecd = ((eecd >> 15) & 0x03);

		/* If both bits are set, device is Flash type */
		if (eecd == 0x03) {
			return 0;
		}
	}
	return 1;
}

static int
wm_get_swsm_semaphore(struct wm_softc *sc)
{
	int32_t timeout;
	uint32_t swsm;

	/* Get the FW semaphore. */
	timeout = 1000 + 1; /* XXX */
	while (timeout) {
		swsm = CSR_READ(sc, WMREG_SWSM);
		swsm |= SWSM_SWESMBI;
		CSR_WRITE(sc, WMREG_SWSM, swsm);
		/* if we managed to set the bit we got the semaphore. */
		swsm = CSR_READ(sc, WMREG_SWSM);
		if (swsm & SWSM_SWESMBI)
			break;

		delay(50);
		timeout--;
	}

	if (timeout == 0) {
		aprint_error("%s: could not acquire EEPROM GNT\n",
		    sc->sc_dev.dv_xname);
		/* Release semaphores */
		wm_put_swsm_semaphore(sc);
		return 1;
	}
	return 0;
}

static void
wm_put_swsm_semaphore(struct wm_softc *sc)
{
	uint32_t swsm;

	swsm = CSR_READ(sc, WMREG_SWSM);
	swsm &= ~(SWSM_SWESMBI);
	CSR_WRITE(sc, WMREG_SWSM, swsm);
}

static int
wm_get_swfw_semaphore(struct wm_softc *sc, uint16_t mask)
{
	uint32_t swfw_sync;
	uint32_t swmask = mask << SWFW_SOFT_SHIFT;
	uint32_t fwmask = mask << SWFW_FIRM_SHIFT;
	int timeout = 200;

	for(timeout = 0; timeout < 200; timeout++) {
		if (sc->sc_flags & WM_F_EEPROM_SEMAPHORE) {
			if (wm_get_swsm_semaphore(sc))
				return 1;
		}
		swfw_sync = CSR_READ(sc, WMREG_SW_FW_SYNC);
		if ((swfw_sync & (swmask | fwmask)) == 0) {
			swfw_sync |= swmask;
			CSR_WRITE(sc, WMREG_SW_FW_SYNC, swfw_sync);
			if (sc->sc_flags & WM_F_EEPROM_SEMAPHORE)
				wm_put_swsm_semaphore(sc);
			return 0;
		}
		if (sc->sc_flags & WM_F_EEPROM_SEMAPHORE)
			wm_put_swsm_semaphore(sc);
		delay(5000);
	}
	printf("%s: failed to get swfw semaphore mask 0x%x swfw 0x%x\n",
	    sc->sc_dev.dv_xname, mask, swfw_sync);
	return 1;
}

static void
wm_put_swfw_semaphore(struct wm_softc *sc, uint16_t mask)
{
	uint32_t swfw_sync;

	if (sc->sc_flags & WM_F_EEPROM_SEMAPHORE) {
		while (wm_get_swsm_semaphore(sc) != 0)
			continue;
	}
	swfw_sync = CSR_READ(sc, WMREG_SW_FW_SYNC);
	swfw_sync &= ~(mask << SWFW_SOFT_SHIFT);
	CSR_WRITE(sc, WMREG_SW_FW_SYNC, swfw_sync);
	if (sc->sc_flags & WM_F_EEPROM_SEMAPHORE)
		wm_put_swsm_semaphore(sc);
}

static int
wm_get_swfwhw_semaphore(struct wm_softc *sc)
{
	uint32_t ext_ctrl;
	int timeout = 200;

	for(timeout = 0; timeout < 200; timeout++) {
		ext_ctrl = CSR_READ(sc, WMREG_EXTCNFCTR);
		ext_ctrl |= E1000_EXTCNF_CTRL_SWFLAG;
		CSR_WRITE(sc, WMREG_EXTCNFCTR, ext_ctrl);

		ext_ctrl = CSR_READ(sc, WMREG_EXTCNFCTR);
		if (ext_ctrl & E1000_EXTCNF_CTRL_SWFLAG)
			return 0;
		delay(5000);
	}
	printf("%s: failed to get swfwgw semaphore ext_ctrl 0x%x\n",
	    sc->sc_dev.dv_xname, ext_ctrl);
	return 1;
}

static void
wm_put_swfwhw_semaphore(struct wm_softc *sc)
{
	uint32_t ext_ctrl;
	ext_ctrl = CSR_READ(sc, WMREG_EXTCNFCTR);
	ext_ctrl &= ~E1000_EXTCNF_CTRL_SWFLAG;
	CSR_WRITE(sc, WMREG_EXTCNFCTR, ext_ctrl);
}

/******************************************************************************
 * Reads a 16 bit word or words from the EEPROM using the ICH8's flash access
 * register.
 *
 * sc - Struct containing variables accessed by shared code
 * offset - offset of word in the EEPROM to read
 * data - word read from the EEPROM
 * words - number of words to read
 *****************************************************************************/
static int
wm_read_eeprom_ich8(struct wm_softc *sc, int offset, int words, uint16_t *data)
{
    int32_t  error = 0;
    uint32_t flash_bank = 0;
    uint32_t act_offset = 0;
    uint32_t bank_offset = 0;
    uint16_t word = 0;
    uint16_t i = 0;

    /* We need to know which is the valid flash bank.  In the event
     * that we didn't allocate eeprom_shadow_ram, we may not be
     * managing flash_bank.  So it cannot be trusted and needs
     * to be updated with each read.
     */
    /* Value of bit 22 corresponds to the flash bank we're on. */
    flash_bank = (CSR_READ(sc, WMREG_EECD) & EECD_SEC1VAL) ? 1 : 0;

    /* Adjust offset appropriately if we're on bank 1 - adjust for word size */
    bank_offset = flash_bank * (sc->sc_ich8_flash_bank_size * 2);

    error = wm_get_swfwhw_semaphore(sc);
    if (error)
        return error;

    for (i = 0; i < words; i++) {
            /* The NVM part needs a byte offset, hence * 2 */
            act_offset = bank_offset + ((offset + i) * 2);
            error = wm_read_ich8_word(sc, act_offset, &word);
            if (error)
                break;
            data[i] = word;
    }

    wm_put_swfwhw_semaphore(sc);
    return error;
}

/******************************************************************************
 * This function does initial flash setup so that a new read/write/erase cycle
 * can be started.
 *
 * sc - The pointer to the hw structure
 ****************************************************************************/
static int32_t
wm_ich8_cycle_init(struct wm_softc *sc)
{
    uint16_t hsfsts;
    int32_t error = 1;
    int32_t i     = 0;

    hsfsts = ICH8_FLASH_READ16(sc, ICH_FLASH_HSFSTS);

    /* May be check the Flash Des Valid bit in Hw status */
    if ((hsfsts & HSFSTS_FLDVAL) == 0) {
        return error;
    }

    /* Clear FCERR in Hw status by writing 1 */
    /* Clear DAEL in Hw status by writing a 1 */
    hsfsts |= HSFSTS_ERR | HSFSTS_DAEL;

    ICH8_FLASH_WRITE16(sc, ICH_FLASH_HSFSTS, hsfsts);

    /* Either we should have a hardware SPI cycle in progress bit to check
     * against, in order to start a new cycle or FDONE bit should be changed
     * in the hardware so that it is 1 after harware reset, which can then be
     * used as an indication whether a cycle is in progress or has been
     * completed .. we should also have some software semaphore mechanism to
     * guard FDONE or the cycle in progress bit so that two threads access to
     * those bits can be sequentiallized or a way so that 2 threads dont
     * start the cycle at the same time */

    if ((hsfsts & HSFSTS_FLINPRO) == 0) {
        /* There is no cycle running at present, so we can start a cycle */
        /* Begin by setting Flash Cycle Done. */
        hsfsts |= HSFSTS_DONE;
        ICH8_FLASH_WRITE16(sc, ICH_FLASH_HSFSTS, hsfsts);
        error = 0;
    } else {
        /* otherwise poll for sometime so the current cycle has a chance
         * to end before giving up. */
        for (i = 0; i < ICH_FLASH_COMMAND_TIMEOUT; i++) {
            hsfsts = ICH8_FLASH_READ16(sc, ICH_FLASH_HSFSTS);
            if ((hsfsts & HSFSTS_FLINPRO) == 0) {
                error = 0;
                break;
            }
            delay(1);
        }
        if (error == 0) {
            /* Successful in waiting for previous cycle to timeout,
             * now set the Flash Cycle Done. */
            hsfsts |= HSFSTS_DONE;
            ICH8_FLASH_WRITE16(sc, ICH_FLASH_HSFSTS, hsfsts);
        }
    }
    return error;
}

/******************************************************************************
 * This function starts a flash cycle and waits for its completion
 *
 * sc - The pointer to the hw structure
 ****************************************************************************/
static int32_t
wm_ich8_flash_cycle(struct wm_softc *sc, uint32_t timeout)
{
    uint16_t hsflctl;
    uint16_t hsfsts;
    int32_t error = 1;
    uint32_t i = 0;

    /* Start a cycle by writing 1 in Flash Cycle Go in Hw Flash Control */
    hsflctl = ICH8_FLASH_READ16(sc, ICH_FLASH_HSFCTL);
    hsflctl |= HSFCTL_GO;
    ICH8_FLASH_WRITE16(sc, ICH_FLASH_HSFCTL, hsflctl);

    /* wait till FDONE bit is set to 1 */
    do {
        hsfsts = ICH8_FLASH_READ16(sc, ICH_FLASH_HSFSTS);
        if (hsfsts & HSFSTS_DONE)
            break;
        delay(1);
        i++;
    } while (i < timeout);
    if ((hsfsts & HSFSTS_DONE) == 1 && (hsfsts & HSFSTS_ERR) == 0) {
        error = 0;
    }
    return error;
}

/******************************************************************************
 * Reads a byte or word from the NVM using the ICH8 flash access registers.
 *
 * sc - The pointer to the hw structure
 * index - The index of the byte or word to read.
 * size - Size of data to read, 1=byte 2=word
 * data - Pointer to the word to store the value read.
 *****************************************************************************/
static int32_t
wm_read_ich8_data(struct wm_softc *sc, uint32_t index,
                     uint32_t size, uint16_t* data)
{
    uint16_t hsfsts;
    uint16_t hsflctl;
    uint32_t flash_linear_address;
    uint32_t flash_data = 0;
    int32_t error = 1;
    int32_t count = 0;

    if (size < 1  || size > 2 || data == 0x0 ||
        index > ICH_FLASH_LINEAR_ADDR_MASK)
        return error;

    flash_linear_address = (ICH_FLASH_LINEAR_ADDR_MASK & index) +
                           sc->sc_ich8_flash_base;

    do {
        delay(1);
        /* Steps */
        error = wm_ich8_cycle_init(sc);
        if (error)
            break;

        hsflctl = ICH8_FLASH_READ16(sc, ICH_FLASH_HSFCTL);
        /* 0b/1b corresponds to 1 or 2 byte size, respectively. */
        hsflctl |=  ((size - 1) << HSFCTL_BCOUNT_SHIFT) & HSFCTL_BCOUNT_MASK;
        hsflctl |= ICH_CYCLE_READ << HSFCTL_CYCLE_SHIFT;
        ICH8_FLASH_WRITE16(sc, ICH_FLASH_HSFCTL, hsflctl);

        /* Write the last 24 bits of index into Flash Linear address field in
         * Flash Address */
        /* TODO: TBD maybe check the index against the size of flash */

        ICH8_FLASH_WRITE32(sc, ICH_FLASH_FADDR, flash_linear_address);

        error = wm_ich8_flash_cycle(sc, ICH_FLASH_COMMAND_TIMEOUT);

        /* Check if FCERR is set to 1, if set to 1, clear it and try the whole
         * sequence a few more times, else read in (shift in) the Flash Data0,
         * the order is least significant byte first msb to lsb */
        if (error == 0) {
            flash_data = ICH8_FLASH_READ32(sc, ICH_FLASH_FDATA0);
            if (size == 1) {
                *data = (uint8_t)(flash_data & 0x000000FF);
            } else if (size == 2) {
                *data = (uint16_t)(flash_data & 0x0000FFFF);
            }
            break;
        } else {
            /* If we've gotten here, then things are probably completely hosed,
             * but if the error condition is detected, it won't hurt to give
             * it another try...ICH_FLASH_CYCLE_REPEAT_COUNT times.
             */
            hsfsts = ICH8_FLASH_READ16(sc, ICH_FLASH_HSFSTS);
            if (hsfsts & HSFSTS_ERR) {
                /* Repeat for some time before giving up. */
                continue;
            } else if ((hsfsts & HSFSTS_DONE) == 0) {
                break;
            }
        }
    } while (count++ < ICH_FLASH_CYCLE_REPEAT_COUNT);

    return error;
}

#if 0
/******************************************************************************
 * Reads a single byte from the NVM using the ICH8 flash access registers.
 *
 * sc - pointer to wm_hw structure
 * index - The index of the byte to read.
 * data - Pointer to a byte to store the value read.
 *****************************************************************************/
static int32_t
wm_read_ich8_byte(struct wm_softc *sc, uint32_t index, uint8_t* data)
{
    int32_t status = 0;
    uint16_t word = 0;

    status = wm_read_ich8_data(sc, index, 1, &word);
    if (status == 0) {
        *data = (uint8_t)word;
    }

    return status;
}
#endif

/******************************************************************************
 * Reads a word from the NVM using the ICH8 flash access registers.
 *
 * sc - pointer to wm_hw structure
 * index - The starting byte index of the word to read.
 * data - Pointer to a word to store the value read.
 *****************************************************************************/
static int32_t
wm_read_ich8_word(struct wm_softc *sc, uint32_t index, uint16_t *data)
{
    int32_t status = 0;
    status = wm_read_ich8_data(sc, index, 2, data);
    return status;
}
