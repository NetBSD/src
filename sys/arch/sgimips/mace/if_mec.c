/* $NetBSD: if_mec.c,v 1.35 2009/05/09 18:31:46 tsutsui Exp $ */

/*-
 * Copyright (c) 2004, 2008 Izumi Tsutsui.  All rights reserved.
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
 * Copyright (c) 2003 Christopher SEKIYA
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * MACE MAC-110 Ethernet driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_mec.c,v 1.35 2009/05/09 18:31:46 tsutsui Exp $");

#include "opt_ddb.h"
#include "bpfilter.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/machtype.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <sgimips/mace/macevar.h>
#include <sgimips/mace/if_mecreg.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

/* #define MEC_DEBUG */

#ifdef MEC_DEBUG
#define MEC_DEBUG_RESET		0x01
#define MEC_DEBUG_START		0x02
#define MEC_DEBUG_STOP		0x04
#define MEC_DEBUG_INTR		0x08
#define MEC_DEBUG_RXINTR	0x10
#define MEC_DEBUG_TXINTR	0x20
#define MEC_DEBUG_TXSEGS	0x40
uint32_t mec_debug = 0;
#define DPRINTF(x, y)	if (mec_debug & (x)) printf y
#else
#define DPRINTF(x, y)	/* nothing */
#endif

/* #define MEC_EVENT_COUNTERS */

#ifdef MEC_EVENT_COUNTERS
#define MEC_EVCNT_INCR(ev)	(ev)->ev_count++
#else
#define MEC_EVCNT_INCR(ev)	do {} while (/* CONSTCOND */ 0)
#endif

/*
 * Transmit descriptor list size
 */
#define MEC_NTXDESC		64
#define MEC_NTXDESC_MASK	(MEC_NTXDESC - 1)
#define MEC_NEXTTX(x)		(((x) + 1) & MEC_NTXDESC_MASK)
#define MEC_NTXDESC_RSVD	4
#define MEC_NTXDESC_INTR	8

/*
 * software state for TX
 */
struct mec_txsoft {
	struct mbuf *txs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t txs_dmamap;	/* our DMA map */
	uint32_t txs_flags;
#define MEC_TXS_BUFLEN_MASK	0x0000007f	/* data len in txd_buf */
#define MEC_TXS_TXDPTR		0x00000080	/* concat txd_ptr is used */
};

/*
 * Transmit buffer descriptor
 */
#define MEC_TXDESCSIZE		128
#define MEC_NTXPTR		3
#define MEC_TXD_BUFOFFSET	sizeof(uint64_t)
#define MEC_TXD_BUFOFFSET1	\
	(sizeof(uint64_t) + sizeof(uint64_t) * MEC_NTXPTR)
#define MEC_TXD_BUFSIZE		(MEC_TXDESCSIZE - MEC_TXD_BUFOFFSET)
#define MEC_TXD_BUFSIZE1	(MEC_TXDESCSIZE - MEC_TXD_BUFOFFSET1)
#define MEC_TXD_BUFSTART(len)	(MEC_TXD_BUFSIZE - (len))
#define MEC_TXD_ALIGN		8
#define MEC_TXD_ALIGNMASK	(MEC_TXD_ALIGN - 1)
#define MEC_TXD_ROUNDUP(addr)	\
	(((addr) + MEC_TXD_ALIGNMASK) & ~(uint64_t)MEC_TXD_ALIGNMASK)
#define MEC_NTXSEG		16

struct mec_txdesc {
	volatile uint64_t txd_cmd;
#define MEC_TXCMD_DATALEN	0x000000000000ffff	/* data length */
#define MEC_TXCMD_BUFSTART	0x00000000007f0000	/* start byte offset */
#define  TXCMD_BUFSTART(x)	((x) << 16)
#define MEC_TXCMD_TERMDMA	0x0000000000800000	/* stop DMA on abort */
#define MEC_TXCMD_TXINT		0x0000000001000000	/* INT after TX done */
#define MEC_TXCMD_PTR1		0x0000000002000000	/* valid 1st txd_ptr */
#define MEC_TXCMD_PTR2		0x0000000004000000	/* valid 2nd txd_ptr */
#define MEC_TXCMD_PTR3		0x0000000008000000	/* valid 3rd txd_ptr */
#define MEC_TXCMD_UNUSED	0xfffffffff0000000ULL	/* should be zero */

#define txd_stat	txd_cmd
#define MEC_TXSTAT_LEN		0x000000000000ffff	/* TX length */
#define MEC_TXSTAT_COLCNT	0x00000000000f0000	/* collision count */
#define MEC_TXSTAT_COLCNT_SHIFT	16
#define MEC_TXSTAT_LATE_COL	0x0000000000100000	/* late collision */
#define MEC_TXSTAT_CRCERROR	0x0000000000200000	/* */
#define MEC_TXSTAT_DEFERRED	0x0000000000400000	/* */
#define MEC_TXSTAT_SUCCESS	0x0000000000800000	/* TX complete */
#define MEC_TXSTAT_TOOBIG	0x0000000001000000	/* */
#define MEC_TXSTAT_UNDERRUN	0x0000000002000000	/* */
#define MEC_TXSTAT_COLLISIONS	0x0000000004000000	/* */
#define MEC_TXSTAT_EXDEFERRAL	0x0000000008000000	/* */
#define MEC_TXSTAT_COLLIDED	0x0000000010000000	/* */
#define MEC_TXSTAT_UNUSED	0x7fffffffe0000000ULL	/* should be zero */
#define MEC_TXSTAT_SENT		0x8000000000000000ULL	/* packet sent */

	union {
		uint64_t txptr[MEC_NTXPTR];
#define MEC_TXPTR_UNUSED2	0x0000000000000007	/* should be zero */
#define MEC_TXPTR_DMAADDR	0x00000000fffffff8	/* TX DMA address */
#define MEC_TXPTR_LEN		0x0000ffff00000000ULL	/* buffer length */
#define  TXPTR_LEN(x)		((uint64_t)(x) << 32)
#define MEC_TXPTR_UNUSED1	0xffff000000000000ULL	/* should be zero */

		uint8_t txbuf[MEC_TXD_BUFSIZE];
	} txd_data;
#define txd_ptr		txd_data.txptr
#define txd_buf		txd_data.txbuf
};

/*
 * Receive buffer size
 */
#define MEC_NRXDESC		16
#define MEC_NRXDESC_MASK	(MEC_NRXDESC - 1)
#define MEC_NEXTRX(x)		(((x) + 1) & MEC_NRXDESC_MASK)

/*
 * Receive buffer description
 */
#define MEC_RXDESCSIZE		4096	/* umm, should be 4kbyte aligned */
#define MEC_RXD_NRXPAD		3
#define MEC_RXD_DMAOFFSET	(1 + MEC_RXD_NRXPAD)
#define MEC_RXD_BUFOFFSET	(MEC_RXD_DMAOFFSET * sizeof(uint64_t))
#define MEC_RXD_BUFSIZE		(MEC_RXDESCSIZE - MEC_RXD_BUFOFFSET)

struct mec_rxdesc {
	volatile uint64_t rxd_stat;
#define MEC_RXSTAT_LEN		0x000000000000ffff	/* data length */
#define MEC_RXSTAT_VIOLATION	0x0000000000010000	/* code violation (?) */
#define MEC_RXSTAT_UNUSED2	0x0000000000020000	/* unknown (?) */
#define MEC_RXSTAT_CRCERROR	0x0000000000040000	/* CRC error */
#define MEC_RXSTAT_MULTICAST	0x0000000000080000	/* multicast packet */
#define MEC_RXSTAT_BROADCAST	0x0000000000100000	/* broadcast packet */
#define MEC_RXSTAT_INVALID	0x0000000000200000	/* invalid preamble */
#define MEC_RXSTAT_LONGEVENT	0x0000000000400000	/* long packet */
#define MEC_RXSTAT_BADPACKET	0x0000000000800000	/* bad packet */
#define MEC_RXSTAT_CAREVENT	0x0000000001000000	/* carrier event */
#define MEC_RXSTAT_MATCHMCAST	0x0000000002000000	/* match multicast */
#define MEC_RXSTAT_MATCHMAC	0x0000000004000000	/* match MAC */
#define MEC_RXSTAT_SEQNUM	0x00000000f8000000	/* sequence number */
#define MEC_RXSTAT_CKSUM	0x0000ffff00000000ULL	/* IP checksum */
#define  RXSTAT_CKSUM(x)	(((uint64_t)(x) & MEC_RXSTAT_CKSUM) >> 32)
#define MEC_RXSTAT_UNUSED1	0x7fff000000000000ULL	/* should be zero */
#define MEC_RXSTAT_RECEIVED	0x8000000000000000ULL	/* set to 1 on RX */
	uint64_t rxd_pad1[MEC_RXD_NRXPAD];
	uint8_t  rxd_buf[MEC_RXD_BUFSIZE];
};

/*
 * control structures for DMA ops
 */
struct mec_control_data {
	/*
	 * TX descriptors and buffers
	 */
	struct mec_txdesc mcd_txdesc[MEC_NTXDESC];

	/*
	 * RX descriptors and buffers
	 */
	struct mec_rxdesc mcd_rxdesc[MEC_NRXDESC];
};

/*
 * It _seems_ there are some restrictions on descriptor address:
 *
 * - Base address of txdescs should be 8kbyte aligned
 * - Each txdesc should be 128byte aligned
 * - Each rxdesc should be 4kbyte aligned
 *
 * So we should specify 8k align to allocalte txdescs.
 * In this case, sizeof(struct mec_txdesc) * MEC_NTXDESC is 8192
 * so rxdescs are also allocated at 4kbyte aligned.
 */
#define MEC_CONTROL_DATA_ALIGN	(8 * 1024)

#define MEC_CDOFF(x)	offsetof(struct mec_control_data, x)
#define MEC_CDTXOFF(x)	MEC_CDOFF(mcd_txdesc[(x)])
#define MEC_CDRXOFF(x)	MEC_CDOFF(mcd_rxdesc[(x)])

/*
 * software state per device
 */
struct mec_softc {
	device_t sc_dev;		/* generic device structures */

	bus_space_tag_t sc_st;		/* bus_space tag */
	bus_space_handle_t sc_sh;	/* bus_space handle */
	bus_dma_tag_t sc_dmat;		/* bus_dma tag */
	void *sc_sdhook;		/* shutdown hook */

	struct ethercom sc_ethercom;	/* Ethernet common part */

	struct mii_data sc_mii;		/* MII/media information */
	int sc_phyaddr;			/* MII address */
	struct callout sc_tick_ch;	/* tick callout */

	uint8_t sc_enaddr[ETHER_ADDR_LEN]; /* MAC address */

	bus_dmamap_t sc_cddmamap;	/* bus_dma map for control data */
#define sc_cddma	sc_cddmamap->dm_segs[0].ds_addr

	/* pointer to allocated control data */
	struct mec_control_data *sc_control_data;
#define sc_txdesc	sc_control_data->mcd_txdesc
#define sc_rxdesc	sc_control_data->mcd_rxdesc

	/* software state for TX descs */
	struct mec_txsoft sc_txsoft[MEC_NTXDESC];

	int sc_txpending;		/* number of TX requests pending */
	int sc_txdirty;			/* first dirty TX descriptor */
	int sc_txlast;			/* last used TX descriptor */

	int sc_rxptr;			/* next ready RX buffer */

#if NRND > 0
	rndsource_element_t sc_rnd_source; /* random source */
#endif
#ifdef MEC_EVENT_COUNTERS
	struct evcnt sc_ev_txpkts;	/* TX packets queued total */
	struct evcnt sc_ev_txdpad;	/* TX packets padded in txdesc buf */
	struct evcnt sc_ev_txdbuf;	/* TX packets copied to txdesc buf */
	struct evcnt sc_ev_txptr1;	/* TX packets using concat ptr1 */
	struct evcnt sc_ev_txptr1a;	/* TX packets  w/ptr1  ~160bytes */
	struct evcnt sc_ev_txptr1b;	/* TX packets  w/ptr1  ~256bytes */
	struct evcnt sc_ev_txptr1c;	/* TX packets  w/ptr1  ~512bytes */
	struct evcnt sc_ev_txptr1d;	/* TX packets  w/ptr1 ~1024bytes */
	struct evcnt sc_ev_txptr1e;	/* TX packets  w/ptr1 >1024bytes */
	struct evcnt sc_ev_txptr2;	/* TX packets using concat ptr1,2 */
	struct evcnt sc_ev_txptr2a;	/* TX packets  w/ptr2  ~160bytes */
	struct evcnt sc_ev_txptr2b;	/* TX packets  w/ptr2  ~256bytes */
	struct evcnt sc_ev_txptr2c;	/* TX packets  w/ptr2  ~512bytes */
	struct evcnt sc_ev_txptr2d;	/* TX packets  w/ptr2 ~1024bytes */
	struct evcnt sc_ev_txptr2e;	/* TX packets  w/ptr2 >1024bytes */
	struct evcnt sc_ev_txptr3;	/* TX packets using concat ptr1,2,3 */
	struct evcnt sc_ev_txptr3a;	/* TX packets  w/ptr3  ~160bytes */
	struct evcnt sc_ev_txptr3b;	/* TX packets  w/ptr3  ~256bytes */
	struct evcnt sc_ev_txptr3c;	/* TX packets  w/ptr3  ~512bytes */
	struct evcnt sc_ev_txptr3d;	/* TX packets  w/ptr3 ~1024bytes */
	struct evcnt sc_ev_txptr3e;	/* TX packets  w/ptr3 >1024bytes */
	struct evcnt sc_ev_txmbuf;	/* TX packets copied to new mbufs */
	struct evcnt sc_ev_txmbufa;	/* TX packets  w/mbuf  ~160bytes */
	struct evcnt sc_ev_txmbufb;	/* TX packets  w/mbuf  ~256bytes */
	struct evcnt sc_ev_txmbufc;	/* TX packets  w/mbuf  ~512bytes */
	struct evcnt sc_ev_txmbufd;	/* TX packets  w/mbuf ~1024bytes */
	struct evcnt sc_ev_txmbufe;	/* TX packets  w/mbuf >1024bytes */
	struct evcnt sc_ev_txptrs;	/* TX packets using ptrs total */
	struct evcnt sc_ev_txptrc0;	/* TX packets  w/ptrs no hdr chain */
	struct evcnt sc_ev_txptrc1;	/* TX packets  w/ptrs  1 hdr chain */
	struct evcnt sc_ev_txptrc2;	/* TX packets  w/ptrs  2 hdr chains */
	struct evcnt sc_ev_txptrc3;	/* TX packets  w/ptrs  3 hdr chains */
	struct evcnt sc_ev_txptrc4;	/* TX packets  w/ptrs  4 hdr chains */
	struct evcnt sc_ev_txptrc5;	/* TX packets  w/ptrs  5 hdr chains */
	struct evcnt sc_ev_txptrc6;	/* TX packets  w/ptrs >5 hdr chains */
	struct evcnt sc_ev_txptrh0;	/* TX packets  w/ptrs  ~8bytes hdr */
	struct evcnt sc_ev_txptrh1;	/* TX packets  w/ptrs ~16bytes hdr */
	struct evcnt sc_ev_txptrh2;	/* TX packets  w/ptrs ~32bytes hdr */
	struct evcnt sc_ev_txptrh3;	/* TX packets  w/ptrs ~64bytes hdr */
	struct evcnt sc_ev_txptrh4;	/* TX packets  w/ptrs ~80bytes hdr */
	struct evcnt sc_ev_txptrh5;	/* TX packets  w/ptrs ~96bytes hdr */
	struct evcnt sc_ev_txdstall;	/* TX stalled due to no txdesc */
	struct evcnt sc_ev_txempty;	/* TX empty interrupts */
	struct evcnt sc_ev_txsent;	/* TX sent interrupts */
#endif
};

#define MEC_CDTXADDR(sc, x)	((sc)->sc_cddma + MEC_CDTXOFF(x))
#define MEC_CDRXADDR(sc, x)	((sc)->sc_cddma + MEC_CDRXOFF(x))

#define MEC_TXDESCSYNC(sc, x, ops)					\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    MEC_CDTXOFF(x), MEC_TXDESCSIZE, (ops))
#define MEC_TXCMDSYNC(sc, x, ops)					\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    MEC_CDTXOFF(x), sizeof(uint64_t), (ops))

#define MEC_RXSTATSYNC(sc, x, ops)					\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    MEC_CDRXOFF(x), sizeof(uint64_t), (ops))
#define MEC_RXBUFSYNC(sc, x, len, ops)					\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    MEC_CDRXOFF(x) + MEC_RXD_BUFOFFSET,				\
	    MEC_ETHER_ALIGN + (len), (ops))

/* XXX these values should be moved to <net/if_ether.h> ? */
#define ETHER_PAD_LEN	(ETHER_MIN_LEN - ETHER_CRC_LEN)
#define MEC_ETHER_ALIGN	2

static int	mec_match(device_t, cfdata_t, void *);
static void	mec_attach(device_t, device_t, void *);

static int	mec_mii_readreg(device_t, int, int);
static void	mec_mii_writereg(device_t, int, int, int);
static int	mec_mii_wait(struct mec_softc *);
static void	mec_statchg(device_t);

static void	enaddr_aton(const char *, uint8_t *);

static int	mec_init(struct ifnet * ifp);
static void	mec_start(struct ifnet *);
static void	mec_watchdog(struct ifnet *);
static void	mec_tick(void *);
static int	mec_ioctl(struct ifnet *, u_long, void *);
static void	mec_reset(struct mec_softc *);
static void	mec_setfilter(struct mec_softc *);
static int	mec_intr(void *arg);
static void	mec_stop(struct ifnet *, int);
static void	mec_rxintr(struct mec_softc *);
static void	mec_rxcsum(struct mec_softc *, struct mbuf *, uint16_t,
		    uint32_t);
static void	mec_txintr(struct mec_softc *, uint32_t);
static void	mec_shutdown(void *);

CFATTACH_DECL_NEW(mec, sizeof(struct mec_softc),
    mec_match, mec_attach, NULL, NULL);

static int mec_matched = 0;

static int
mec_match(device_t parent, cfdata_t cf, void *aux)
{

	/* allow only one device */
	if (mec_matched)
		return 0;

	mec_matched = 1;
	return 1;
}

static void
mec_attach(device_t parent, device_t self, void *aux)
{
	struct mec_softc *sc = device_private(self);
	struct mace_attach_args *maa = aux;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint64_t address, command;
	const char *macaddr;
	struct mii_softc *child;
	bus_dma_segment_t seg;
	int i, err, rseg;
	bool mac_is_fake;

	sc->sc_dev = self;
	sc->sc_st = maa->maa_st;
	if (bus_space_subregion(sc->sc_st, maa->maa_sh,
	    maa->maa_offset, 0,	&sc->sc_sh) != 0) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	/* set up DMA structures */
	sc->sc_dmat = maa->maa_dmat;

	/*
	 * Allocate the control data structures, and create and load the
	 * DMA map for it.
	 */
	if ((err = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof(struct mec_control_data), MEC_CONTROL_DATA_ALIGN, 0,
	    &seg, 1, &rseg, BUS_DMA_NOWAIT)) != 0) {
		aprint_error(": unable to allocate control data, error = %d\n",
		    err);
		goto fail_0;
	}
	/*
	 * XXX needs re-think...
	 * control data structures contain whole RX data buffer, so
	 * BUS_DMA_COHERENT (which disables cache) may cause some performance
	 * issue on copying data from the RX buffer to mbuf on normal memory,
	 * though we have to make sure all bus_dmamap_sync(9) ops are called
	 * properly in that case.
	 */
	if ((err = bus_dmamem_map(sc->sc_dmat, &seg, rseg,
	    sizeof(struct mec_control_data),
	    (void **)&sc->sc_control_data, /*BUS_DMA_COHERENT*/ 0)) != 0) {
		aprint_error(": unable to map control data, error = %d\n", err);
		goto fail_1;
	}
	memset(sc->sc_control_data, 0, sizeof(struct mec_control_data));

	if ((err = bus_dmamap_create(sc->sc_dmat,
	    sizeof(struct mec_control_data), 1,
	    sizeof(struct mec_control_data), 0, 0, &sc->sc_cddmamap)) != 0) {
		aprint_error(": unable to create control data DMA map,"
		    " error = %d\n", err);
		goto fail_2;
	}
	if ((err = bus_dmamap_load(sc->sc_dmat, sc->sc_cddmamap,
	    sc->sc_control_data, sizeof(struct mec_control_data), NULL,
	    BUS_DMA_NOWAIT)) != 0) {
		aprint_error(": unable to load control data DMA map,"
		    " error = %d\n", err);
		goto fail_3;
	}

	/* create TX buffer DMA maps */
	for (i = 0; i < MEC_NTXDESC; i++) {
		if ((err = bus_dmamap_create(sc->sc_dmat,
		    MCLBYTES, MEC_NTXSEG, MCLBYTES, PAGE_SIZE, 0,
		    &sc->sc_txsoft[i].txs_dmamap)) != 0) {
			aprint_error(": unable to create tx DMA map %d,"
			    " error = %d\n", i, err);
			goto fail_4;
		}
	}

	callout_init(&sc->sc_tick_ch, 0);

	/* get Ethernet address from ARCBIOS */
	if ((macaddr = ARCBIOS->GetEnvironmentVariable("eaddr")) == NULL) {
		aprint_error(": unable to get MAC address!\n");
		goto fail_4;
	}
	/*
	 * On some machines the DS2502 chip storing the serial number/
	 * mac address is on the pci riser board - if this board is
	 * missing, ARCBIOS will not know a good ethernet address (but
	 * otherwise the machine will work fine).
	 */
	mac_is_fake = false;
	if (strcmp(macaddr, "ff:ff:ff:ff:ff:ff") == 0) {
		uint32_t ui = 0;
		const char * netaddr =
			ARCBIOS->GetEnvironmentVariable("netaddr");

		/*
		 * Create a MAC address by abusing the "netaddr" env var
		 */
		sc->sc_enaddr[0] = 0xf2;
		sc->sc_enaddr[1] = 0x0b;
		sc->sc_enaddr[2] = 0xa4;
		if (netaddr) {
			mac_is_fake = true;
			while (*netaddr) {
				int v = 0;
				while (*netaddr && *netaddr != '.') {
					if (*netaddr >= '0' && *netaddr <= '9')
						v = v*10 + (*netaddr - '0');
					netaddr++;
				}
				ui <<= 8;
				ui |= v;
				if (*netaddr == '.')
					netaddr++;
			}
		}
		memcpy(sc->sc_enaddr+3, ((uint8_t *)&ui)+1, 3);
	}
	if (!mac_is_fake)
		enaddr_aton(macaddr, sc->sc_enaddr);

	/* set the Ethernet address */
	address = 0;
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		address = address << 8;
		address |= sc->sc_enaddr[i];
	}
	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_STATION, address);

	/* reset device */
	mec_reset(sc);

	command = bus_space_read_8(sc->sc_st, sc->sc_sh, MEC_MAC_CONTROL);

	aprint_normal(": MAC-110 Ethernet, rev %u\n",
	    (u_int)((command & MEC_MAC_REVISION) >> MEC_MAC_REVISION_SHIFT));

	if (mac_is_fake)
		aprint_normal_dev(self,
		    "could not get ethernet address from firmware"
		    " - generated one from the \"netaddr\" environment"
		    " variable\n");
	aprint_normal_dev(self, "Ethernet address %s\n",
	    ether_sprintf(sc->sc_enaddr));

	/* Done, now attach everything */

	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = mec_mii_readreg;
	sc->sc_mii.mii_writereg = mec_mii_writereg;
	sc->sc_mii.mii_statchg = mec_statchg;

	/* Set up PHY properties */
	sc->sc_ethercom.ec_mii = &sc->sc_mii;
	ifmedia_init(&sc->sc_mii.mii_media, 0, ether_mediachange,
	    ether_mediastatus);
	mii_attach(self, &sc->sc_mii, 0xffffffff, MII_PHY_ANY,
	    MII_OFFSET_ANY, 0);

	child = LIST_FIRST(&sc->sc_mii.mii_phys);
	if (child == NULL) {
		/* No PHY attached */
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER | IFM_MANUAL,
		    0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER | IFM_MANUAL);
	} else {
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER | IFM_AUTO);
		sc->sc_phyaddr = child->mii_phy;
	}

	strcpy(ifp->if_xname, device_xname(self));
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = mec_ioctl;
	ifp->if_start = mec_start;
	ifp->if_watchdog = mec_watchdog;
	ifp->if_init = mec_init;
	ifp->if_stop = mec_stop;
	ifp->if_mtu = ETHERMTU;
	IFQ_SET_READY(&ifp->if_snd);

	/* mec has dumb RX cksum support */
	ifp->if_capabilities = IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_UDPv4_Rx;

	/* We can support 802.1Q VLAN-sized frames. */
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;

	/* attach the interface */
	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);

	/* establish interrupt */
	cpu_intr_establish(maa->maa_intr, maa->maa_intrmask, mec_intr, sc);

#if NRND > 0
	rnd_attach_source(&sc->sc_rnd_source, device_xname(self),
	    RND_TYPE_NET, 0);
#endif

#ifdef MEC_EVENT_COUNTERS
	evcnt_attach_dynamic(&sc->sc_ev_txpkts , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts queued total");
	evcnt_attach_dynamic(&sc->sc_ev_txdpad , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts padded in txdesc buf");
	evcnt_attach_dynamic(&sc->sc_ev_txdbuf , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts copied to txdesc buf");
	evcnt_attach_dynamic(&sc->sc_ev_txptr1 , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts using concat ptr1");
	evcnt_attach_dynamic(&sc->sc_ev_txptr1a , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptr1  ~160bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txptr1b , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptr1  ~256bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txptr1c , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptr1  ~512bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txptr1d , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptr1 ~1024bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txptr1e , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptr1 >1024bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txptr2 , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts using concat ptr1,2");
	evcnt_attach_dynamic(&sc->sc_ev_txptr2a , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptr2  ~160bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txptr2b , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptr2  ~256bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txptr2c , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptr2  ~512bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txptr2d , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptr2 ~1024bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txptr2e , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptr2 >1024bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txptr3 , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts using concat ptr1,2,3");
	evcnt_attach_dynamic(&sc->sc_ev_txptr3a , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptr3  ~160bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txptr3b , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptr3  ~256bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txptr3c , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptr3  ~512bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txptr3d , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptr3 ~1024bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txptr3e , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptr3 >1024bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txmbuf , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts copied to new mbufs");
	evcnt_attach_dynamic(&sc->sc_ev_txmbufa , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/mbuf  ~160bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txmbufb , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/mbuf  ~256bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txmbufc , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/mbuf  ~512bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txmbufd , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/mbuf ~1024bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txmbufe , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/mbuf >1024bytes");
	evcnt_attach_dynamic(&sc->sc_ev_txptrs , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts using ptrs total");
	evcnt_attach_dynamic(&sc->sc_ev_txptrc0 , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptrs no hdr chain");
	evcnt_attach_dynamic(&sc->sc_ev_txptrc1 , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptrs  1 hdr chain");
	evcnt_attach_dynamic(&sc->sc_ev_txptrc2 , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptrs  2 hdr chains");
	evcnt_attach_dynamic(&sc->sc_ev_txptrc3 , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptrs  3 hdr chains");
	evcnt_attach_dynamic(&sc->sc_ev_txptrc4 , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptrs  4 hdr chains");
	evcnt_attach_dynamic(&sc->sc_ev_txptrc5 , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptrs  5 hdr chains");
	evcnt_attach_dynamic(&sc->sc_ev_txptrc6 , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptrs >5 hdr chains");
	evcnt_attach_dynamic(&sc->sc_ev_txptrh0 , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptrs  ~8bytes hdr");
	evcnt_attach_dynamic(&sc->sc_ev_txptrh1 , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptrs ~16bytes hdr");
	evcnt_attach_dynamic(&sc->sc_ev_txptrh2 , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptrs ~32bytes hdr");
	evcnt_attach_dynamic(&sc->sc_ev_txptrh3 , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptrs ~64bytes hdr");
	evcnt_attach_dynamic(&sc->sc_ev_txptrh4 , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptrs ~80bytes hdr");
	evcnt_attach_dynamic(&sc->sc_ev_txptrh5 , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX pkts  w/ptrs ~96bytes hdr");
	evcnt_attach_dynamic(&sc->sc_ev_txdstall , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX stalled due to no txdesc");
	evcnt_attach_dynamic(&sc->sc_ev_txempty , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX empty interrupts");
	evcnt_attach_dynamic(&sc->sc_ev_txsent , EVCNT_TYPE_MISC,
	    NULL, device_xname(self), "TX sent interrupts");
#endif

	/* set shutdown hook to reset interface on powerdown */
	sc->sc_sdhook = shutdownhook_establish(mec_shutdown, sc);

	return;

	/*
	 * Free any resources we've allocated during the failed attach
	 * attempt.  Do this in reverse order and fall though.
	 */
 fail_4:
	for (i = 0; i < MEC_NTXDESC; i++) {
		if (sc->sc_txsoft[i].txs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_txsoft[i].txs_dmamap);
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_cddmamap);
 fail_3:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_cddmamap);
 fail_2:
	bus_dmamem_unmap(sc->sc_dmat, (void *)sc->sc_control_data,
	    sizeof(struct mec_control_data));
 fail_1:
	bus_dmamem_free(sc->sc_dmat, &seg, rseg);
 fail_0:
	return;
}

static int
mec_mii_readreg(device_t self, int phy, int reg)
{
	struct mec_softc *sc = device_private(self);
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	uint64_t val;
	int i;

	if (mec_mii_wait(sc) != 0)
		return 0;

	bus_space_write_8(st, sh, MEC_PHY_ADDRESS,
	    (phy << MEC_PHY_ADDR_DEVSHIFT) | (reg & MEC_PHY_ADDR_REGISTER));
	delay(25);
	bus_space_write_8(st, sh, MEC_PHY_READ_INITIATE, 1);
	delay(25);
	mec_mii_wait(sc);

	for (i = 0; i < 20; i++) {
		delay(30);

		val = bus_space_read_8(st, sh, MEC_PHY_DATA);

		if ((val & MEC_PHY_DATA_BUSY) == 0)
			return val & MEC_PHY_DATA_VALUE;
	}
	return 0;
}

static void
mec_mii_writereg(device_t self, int phy, int reg, int val)
{
	struct mec_softc *sc = device_private(self);
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;

	if (mec_mii_wait(sc) != 0) {
		printf("timed out writing %x: %x\n", reg, val);
		return;
	}

	bus_space_write_8(st, sh, MEC_PHY_ADDRESS,
	    (phy << MEC_PHY_ADDR_DEVSHIFT) | (reg & MEC_PHY_ADDR_REGISTER));

	delay(60);

	bus_space_write_8(st, sh, MEC_PHY_DATA, val & MEC_PHY_DATA_VALUE);

	delay(60);

	mec_mii_wait(sc);
}

static int
mec_mii_wait(struct mec_softc *sc)
{
	uint32_t busy;
	int i, s;

	for (i = 0; i < 100; i++) {
		delay(30);

		s = splhigh();
		busy = bus_space_read_8(sc->sc_st, sc->sc_sh, MEC_PHY_DATA);
		splx(s);

		if ((busy & MEC_PHY_DATA_BUSY) == 0)
			return 0;
#if 0
		if (busy == 0xffff) /* XXX ? */
			return 0;
#endif
	}

	printf("%s: MII timed out\n", device_xname(sc->sc_dev));
	return 1;
}

static void
mec_statchg(device_t self)
{
	struct mec_softc *sc = device_private(self);
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	uint32_t control;

	control = bus_space_read_8(st, sh, MEC_MAC_CONTROL);
	control &= ~(MEC_MAC_IPGT | MEC_MAC_IPGR1 | MEC_MAC_IPGR2 |
	    MEC_MAC_FULL_DUPLEX | MEC_MAC_SPEED_SELECT);

	/* must also set IPG here for duplex stuff ... */
	if ((sc->sc_mii.mii_media_active & IFM_FDX) != 0) {
		control |= MEC_MAC_FULL_DUPLEX;
	} else {
		/* set IPG */
		control |= MEC_MAC_IPG_DEFAULT;
	}

	bus_space_write_8(st, sh, MEC_MAC_CONTROL, control);
}

/*
 * XXX
 * maybe this function should be moved to common part
 * (sgimips/machdep.c or elsewhere) for all on-board network devices.
 */
static void
enaddr_aton(const char *str, uint8_t *eaddr)
{
	int i;
	char c;

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		if (*str == ':')
			str++;

		c = *str++;
		if (isdigit(c)) {
			eaddr[i] = (c - '0');
		} else if (isxdigit(c)) {
			eaddr[i] = (toupper(c) + 10 - 'A');
		}
		c = *str++;
		if (isdigit(c)) {
			eaddr[i] = (eaddr[i] << 4) | (c - '0');
		} else if (isxdigit(c)) {
			eaddr[i] = (eaddr[i] << 4) | (toupper(c) + 10 - 'A');
		}
	}
}

static int
mec_init(struct ifnet *ifp)
{
	struct mec_softc *sc = ifp->if_softc;
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	struct mec_rxdesc *rxd;
	int i, rc;

	/* cancel any pending I/O */
	mec_stop(ifp, 0);

	/* reset device */
	mec_reset(sc);

	/* setup filter for multicast or promisc mode */
	mec_setfilter(sc);

	/* set the TX ring pointer to the base address */
	bus_space_write_8(st, sh, MEC_TX_RING_BASE, MEC_CDTXADDR(sc, 0));

	sc->sc_txpending = 0;
	sc->sc_txdirty = 0;
	sc->sc_txlast = MEC_NTXDESC - 1;

	/* put RX buffers into FIFO */
	for (i = 0; i < MEC_NRXDESC; i++) {
		rxd = &sc->sc_rxdesc[i];
		rxd->rxd_stat = 0;
		MEC_RXSTATSYNC(sc, i, BUS_DMASYNC_PREREAD);
		MEC_RXBUFSYNC(sc, i, ETHER_MAX_LEN, BUS_DMASYNC_PREREAD);
		bus_space_write_8(st, sh, MEC_MCL_RX_FIFO, MEC_CDRXADDR(sc, i));
	}
	sc->sc_rxptr = 0;

#if 0	/* XXX no info */
	bus_space_write_8(st, sh, MEC_TIMER, 0);
#endif

	/*
	 * MEC_DMA_TX_INT_ENABLE will be set later otherwise it causes
	 * spurious interrupts when TX buffers are empty
	 */
	bus_space_write_8(st, sh, MEC_DMA_CONTROL,
	    (MEC_RXD_DMAOFFSET << MEC_DMA_RX_DMA_OFFSET_SHIFT) |
	    (MEC_NRXDESC << MEC_DMA_RX_INT_THRESH_SHIFT) |
	    MEC_DMA_TX_DMA_ENABLE | /* MEC_DMA_TX_INT_ENABLE | */
	    MEC_DMA_RX_DMA_ENABLE | MEC_DMA_RX_INT_ENABLE);

	callout_reset(&sc->sc_tick_ch, hz, mec_tick, sc);

	if ((rc = ether_mediachange(ifp)) != 0)
		return rc;

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	mec_start(ifp);

	return 0;
}

static void
mec_reset(struct mec_softc *sc)
{
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	uint64_t control;

	/* stop DMA first */
	bus_space_write_8(st, sh, MEC_DMA_CONTROL, 0);

	/* reset chip */
	bus_space_write_8(st, sh, MEC_MAC_CONTROL, MEC_MAC_CORE_RESET);
	delay(1000);
	bus_space_write_8(st, sh, MEC_MAC_CONTROL, 0);
	delay(1000);

	/* Default to 100/half and let auto-negotiation work its magic */
	control = MEC_MAC_SPEED_SELECT | MEC_MAC_FILTER_MATCHMULTI |
	    MEC_MAC_IPG_DEFAULT;

	bus_space_write_8(st, sh, MEC_MAC_CONTROL, control);
	/* stop DMA again for sanity */
	bus_space_write_8(st, sh, MEC_DMA_CONTROL, 0);

	DPRINTF(MEC_DEBUG_RESET, ("mec: control now %llx\n",
	    bus_space_read_8(st, sh, MEC_MAC_CONTROL)));
}

static void
mec_start(struct ifnet *ifp)
{
	struct mec_softc *sc = ifp->if_softc;
	struct mbuf *m0, *m;
	struct mec_txdesc *txd;
	struct mec_txsoft *txs;
	bus_dmamap_t dmamap;
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	int error, firsttx, nexttx, opending;
	int len, bufoff, buflen, nsegs, align, resid, pseg, nptr, slen, i;
	uint32_t txdcmd;

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	/*
	 * Remember the previous txpending and the first transmit descriptor.
	 */
	opending = sc->sc_txpending;
	firsttx = MEC_NEXTTX(sc->sc_txlast);

	DPRINTF(MEC_DEBUG_START,
	    ("%s: opending = %d, firsttx = %d\n", __func__, opending, firsttx));

	while (sc->sc_txpending < MEC_NTXDESC - 1) {
		/* Grab a packet off the queue. */
		IFQ_POLL(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;
		m = NULL;

		/*
		 * Get the next available transmit descriptor.
		 */
		nexttx = MEC_NEXTTX(sc->sc_txlast);
		txd = &sc->sc_txdesc[nexttx];
		txs = &sc->sc_txsoft[nexttx];
		dmamap = txs->txs_dmamap;
		txs->txs_flags = 0;

		buflen = 0;
		bufoff = 0;
		resid = 0;
		nptr = 0;	/* XXX gcc */
		pseg = 0;	/* XXX gcc */

		len = m0->m_pkthdr.len;

		DPRINTF(MEC_DEBUG_START,
		    ("%s: len = %d, nexttx = %d, txpending = %d\n",
		    __func__, len, nexttx, sc->sc_txpending));

		if (len <= MEC_TXD_BUFSIZE) {
			/*
			 * If a TX packet will fit into small txdesc buffer,
			 * just copy it into there. Maybe it's faster than
			 * checking alignment and calling bus_dma(9) etc.
			 */
			DPRINTF(MEC_DEBUG_START, ("%s: short packet\n",
			    __func__));
			IFQ_DEQUEUE(&ifp->if_snd, m0);

			/*
			 * I don't know if MEC chip does auto padding,
			 * but do it manually for safety.
			 */
			if (len < ETHER_PAD_LEN) {
				MEC_EVCNT_INCR(&sc->sc_ev_txdpad);
				bufoff = MEC_TXD_BUFSTART(ETHER_PAD_LEN);
				m_copydata(m0, 0, len, txd->txd_buf + bufoff);
				memset(txd->txd_buf + bufoff + len, 0,
				    ETHER_PAD_LEN - len);
				len = buflen = ETHER_PAD_LEN;
			} else {
				MEC_EVCNT_INCR(&sc->sc_ev_txdbuf);
				bufoff = MEC_TXD_BUFSTART(len);
				m_copydata(m0, 0, len, txd->txd_buf + bufoff);
				buflen = len;
			}
		} else {
			/*
			 * If the packet won't fit the static buffer in txdesc,
			 * we have to use the concatenate pointers to handle it.
			 */
			DPRINTF(MEC_DEBUG_START, ("%s: long packet\n",
			    __func__));
			txs->txs_flags = MEC_TXS_TXDPTR;

			/*
			 * Call bus_dmamap_load_mbuf(9) first to see
			 * how many chains the TX mbuf has.
			 */
			error = bus_dmamap_load_mbuf(sc->sc_dmat, dmamap, m0,
			    BUS_DMA_WRITE | BUS_DMA_NOWAIT);
			if (error == 0) {
				/*
				 * Check chains which might contain headers.
				 * They might be so much fragmented and
				 * it's better to copy them into txdesc buffer
				 * since they would be small enough.
				 */
				nsegs = dmamap->dm_nsegs;
				for (pseg = 0; pseg < nsegs; pseg++) {
					slen = dmamap->dm_segs[pseg].ds_len;
					if (buflen + slen >
					    MEC_TXD_BUFSIZE1 - MEC_TXD_ALIGN)
						break;
					buflen += slen;
				}
				/*
				 * Check if the rest chains can be fit into
				 * the concatinate pointers.
				 */
				align = dmamap->dm_segs[pseg].ds_addr &
				    MEC_TXD_ALIGNMASK;
				if (align > 0) {
					/*
					 * If the first chain isn't uint64_t
					 * aligned, append the unaligned part
					 * into txdesc buffer too.
					 */
					resid = MEC_TXD_ALIGN - align;
					buflen += resid;
					for (; pseg < nsegs; pseg++) {
						slen =
						  dmamap->dm_segs[pseg].ds_len;
						if (slen > resid)
							break;
						resid -= slen;
					}
				} else if (pseg == 0) {
					/*
					 * In this case, the first chain is
					 * uint64_t aligned but it's too long
					 * to put into txdesc buf.
					 * We have to put some data into
					 * txdesc buf even in this case,
					 * so put MEC_TXD_ALIGN bytes there.
					 */
					buflen = resid = MEC_TXD_ALIGN;
				}
				nptr = nsegs - pseg;
				if (nptr <= MEC_NTXPTR) {
					bufoff = MEC_TXD_BUFSTART(buflen);

					/*
					 * Check if all the rest chains are
					 * uint64_t aligned.
					 */
					align = 0;
					for (i = pseg + 1; i < nsegs; i++)
						align |=
						    dmamap->dm_segs[i].ds_addr
						    & MEC_TXD_ALIGNMASK;
					if (align != 0) {
						/* chains are not aligned */
						error = -1;
					}
				} else {
					/* The TX mbuf chains doesn't fit. */
					error = -1;
				}
				if (error == -1)
					bus_dmamap_unload(sc->sc_dmat, dmamap);
			}
			if (error != 0) {
				/*
				 * The TX mbuf chains can't be put into
				 * the concatinate buffers. In this case,
				 * we have to allocate a new contiguous mbuf
				 * and copy data into it.
				 *
				 * Even in this case, the Ethernet header in
				 * the TX mbuf might be unaligned and trailing
				 * data might be word aligned, so put 2 byte
				 * (MEC_ETHER_ALIGN) padding at the top of the
				 * allocated mbuf and copy TX packets.
				 * 6 bytes (MEC_ALIGN_BYTES - MEC_ETHER_ALIGN)
				 * at the top of the new mbuf won't be uint64_t
				 * alignd, but we have to put some data into
				 * txdesc buffer anyway even if the buffer
				 * is uint64_t aligned.
				 */ 
				DPRINTF(MEC_DEBUG_START|MEC_DEBUG_TXSEGS,
				    ("%s: re-allocating mbuf\n", __func__));

				MGETHDR(m, M_DONTWAIT, MT_DATA);
				if (m == NULL) {
					printf("%s: unable to allocate "
					    "TX mbuf\n",
					    device_xname(sc->sc_dev));
					break;
				}
				if (len > (MHLEN - MEC_ETHER_ALIGN)) {
					MCLGET(m, M_DONTWAIT);
					if ((m->m_flags & M_EXT) == 0) {
						printf("%s: unable to allocate "
						    "TX cluster\n",
						    device_xname(sc->sc_dev));
						m_freem(m);
						break;
					}
				}
				m->m_data += MEC_ETHER_ALIGN;

				/*
				 * Copy whole data (including unaligned part)
				 * for following bpf_mtap().
				 */
				m_copydata(m0, 0, len, mtod(m, void *));
				m->m_pkthdr.len = m->m_len = len;
				error = bus_dmamap_load_mbuf(sc->sc_dmat,
				    dmamap, m, BUS_DMA_WRITE | BUS_DMA_NOWAIT);
				if (dmamap->dm_nsegs > 1) {
					/* should not happen, but for sanity */
					bus_dmamap_unload(sc->sc_dmat, dmamap);
					error = -1;
				}
				if (error != 0) {
					printf("%s: unable to load TX buffer, "
					    "error = %d\n",
					    device_xname(sc->sc_dev), error);
					m_freem(m);
					break;
				}
				/*
				 * Only the first segment should be put into
				 * the concatinate pointer in this case.
				 */
				pseg = 0;
				nptr = 1;

				/*
				 * Set lenght of unaligned part which will be
				 * copied into txdesc buffer.
				 */
				buflen = MEC_TXD_ALIGN - MEC_ETHER_ALIGN;
				bufoff = MEC_TXD_BUFSTART(buflen);
				resid = buflen;
#ifdef MEC_EVENT_COUNTERS
				MEC_EVCNT_INCR(&sc->sc_ev_txmbuf);
				if (len <= 160)
					MEC_EVCNT_INCR(&sc->sc_ev_txmbufa);
				else if (len <= 256)
					MEC_EVCNT_INCR(&sc->sc_ev_txmbufb);
				else if (len <= 512)
					MEC_EVCNT_INCR(&sc->sc_ev_txmbufc);
				else if (len <= 1024)
					MEC_EVCNT_INCR(&sc->sc_ev_txmbufd);
				else
					MEC_EVCNT_INCR(&sc->sc_ev_txmbufe);
#endif
			}
#ifdef MEC_EVENT_COUNTERS
			else {
				MEC_EVCNT_INCR(&sc->sc_ev_txptrs);
				if (nptr == 1) {
					MEC_EVCNT_INCR(&sc->sc_ev_txptr1);
					if (len <= 160)
						MEC_EVCNT_INCR(
						    &sc->sc_ev_txptr1a);
					else if (len <= 256)
						MEC_EVCNT_INCR(
						    &sc->sc_ev_txptr1b);
					else if (len <= 512)
						MEC_EVCNT_INCR(
						    &sc->sc_ev_txptr1c);
					else if (len <= 1024)
						MEC_EVCNT_INCR(
						    &sc->sc_ev_txptr1d);
					else
						MEC_EVCNT_INCR(
						    &sc->sc_ev_txptr1e);
				} else if (nptr == 2) {
					MEC_EVCNT_INCR(&sc->sc_ev_txptr2);
					if (len <= 160)
						MEC_EVCNT_INCR(
						    &sc->sc_ev_txptr2a);
					else if (len <= 256)
						MEC_EVCNT_INCR(
						    &sc->sc_ev_txptr2b);
					else if (len <= 512)
						MEC_EVCNT_INCR(
						    &sc->sc_ev_txptr2c);
					else if (len <= 1024)
						MEC_EVCNT_INCR(
						    &sc->sc_ev_txptr2d);
					else
						MEC_EVCNT_INCR(
						    &sc->sc_ev_txptr2e);
				} else if (nptr == 3) {
					MEC_EVCNT_INCR(&sc->sc_ev_txptr3);
					if (len <= 160)
						MEC_EVCNT_INCR(
						    &sc->sc_ev_txptr3a);
					else if (len <= 256)
						MEC_EVCNT_INCR(
						    &sc->sc_ev_txptr3b);
					else if (len <= 512)
						MEC_EVCNT_INCR(
						    &sc->sc_ev_txptr3c);
					else if (len <= 1024)
						MEC_EVCNT_INCR(
						    &sc->sc_ev_txptr3d);
					else
						MEC_EVCNT_INCR(
						    &sc->sc_ev_txptr3e);
				}
				if (pseg == 0)
					MEC_EVCNT_INCR(&sc->sc_ev_txptrc0);
				else if (pseg == 1)
					MEC_EVCNT_INCR(&sc->sc_ev_txptrc1);
				else if (pseg == 2)
					MEC_EVCNT_INCR(&sc->sc_ev_txptrc2);
				else if (pseg == 3)
					MEC_EVCNT_INCR(&sc->sc_ev_txptrc3);
				else if (pseg == 4)
					MEC_EVCNT_INCR(&sc->sc_ev_txptrc4);
				else if (pseg == 5)
					MEC_EVCNT_INCR(&sc->sc_ev_txptrc5);
				else
					MEC_EVCNT_INCR(&sc->sc_ev_txptrc6);
				if (buflen <= 8)
					MEC_EVCNT_INCR(&sc->sc_ev_txptrh0);
				else if (buflen <= 16)
					MEC_EVCNT_INCR(&sc->sc_ev_txptrh1);
				else if (buflen <= 32)
					MEC_EVCNT_INCR(&sc->sc_ev_txptrh2);
				else if (buflen <= 64)
					MEC_EVCNT_INCR(&sc->sc_ev_txptrh3);
				else if (buflen <= 80)
					MEC_EVCNT_INCR(&sc->sc_ev_txptrh4);
				else
					MEC_EVCNT_INCR(&sc->sc_ev_txptrh5);
			}
#endif
			m_copydata(m0, 0, buflen, txd->txd_buf + bufoff);

			IFQ_DEQUEUE(&ifp->if_snd, m0);
			if (m != NULL) {
				m_freem(m0);
				m0 = m;
			}

			/*
			 * sync the DMA map for TX mbuf
			 */
			bus_dmamap_sync(sc->sc_dmat, dmamap, buflen,
			    len - buflen, BUS_DMASYNC_PREWRITE);
		}

#if NBPFILTER > 0
		/*
		 * Pass packet to bpf if there is a listener.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m0);
#endif
		MEC_EVCNT_INCR(&sc->sc_ev_txpkts);

		/*
		 * setup the transmit descriptor.
		 */
		txdcmd = TXCMD_BUFSTART(MEC_TXDESCSIZE - buflen) | (len - 1);

		/*
		 * Set MEC_TXCMD_TXINT every MEC_NTXDESC_INTR packets
		 * if more than half txdescs have been queued
		 * because TX_EMPTY interrupts will rarely happen
		 * if TX queue is so stacked.
		 */
		if (sc->sc_txpending > (MEC_NTXDESC / 2) &&
		    (nexttx & (MEC_NTXDESC_INTR - 1)) == 0)
			txdcmd |= MEC_TXCMD_TXINT;

		if ((txs->txs_flags & MEC_TXS_TXDPTR) != 0) {
			bus_dma_segment_t *segs = dmamap->dm_segs;

			DPRINTF(MEC_DEBUG_TXSEGS,
			    ("%s: nsegs = %d, pseg = %d, nptr = %d\n",
			    __func__, dmamap->dm_nsegs, pseg, nptr));

			switch (nptr) {
			case 3:
				KASSERT((segs[pseg + 2].ds_addr &
				    MEC_TXD_ALIGNMASK) == 0);
				txdcmd |= MEC_TXCMD_PTR3;
				txd->txd_ptr[2] =
				    TXPTR_LEN(segs[pseg + 2].ds_len - 1) |
				    segs[pseg + 2].ds_addr;
				/* FALLTHROUGH */
			case 2:
				KASSERT((segs[pseg + 1].ds_addr &
				    MEC_TXD_ALIGNMASK) == 0);
				txdcmd |= MEC_TXCMD_PTR2;
				txd->txd_ptr[1] =
				    TXPTR_LEN(segs[pseg + 1].ds_len - 1) |
				    segs[pseg + 1].ds_addr;
				/* FALLTHROUGH */
			case 1:
				txdcmd |= MEC_TXCMD_PTR1;
				txd->txd_ptr[0] =
				    TXPTR_LEN(segs[pseg].ds_len - resid - 1) |
				    (segs[pseg].ds_addr + resid);
				break;
			default:
				panic("%s: impossible nptr in %s",
				    device_xname(sc->sc_dev), __func__);
				/* NOTREACHED */
			}
			/*
			 * Store a pointer to the packet so we can
			 * free it later.
			 */
			txs->txs_mbuf = m0;
		} else {
			/*
			 * In this case all data are copied to buffer in txdesc,
			 * we can free TX mbuf here.
			 */
			m_freem(m0);
		}
		txd->txd_cmd = txdcmd;

		DPRINTF(MEC_DEBUG_START,
		    ("%s: txd_cmd    = 0x%016llx\n",
		    __func__, txd->txd_cmd));
		DPRINTF(MEC_DEBUG_START,
		    ("%s: txd_ptr[0] = 0x%016llx\n",
		    __func__, txd->txd_ptr[0]));
		DPRINTF(MEC_DEBUG_START,
		    ("%s: txd_ptr[1] = 0x%016llx\n",
		    __func__, txd->txd_ptr[1]));
		DPRINTF(MEC_DEBUG_START,
		    ("%s: txd_ptr[2] = 0x%016llx\n",
		    __func__, txd->txd_ptr[2]));
		DPRINTF(MEC_DEBUG_START,
		    ("%s: len = %d (0x%04x), buflen = %d (0x%02x)\n",
		    __func__, len, len, buflen, buflen));

		/* sync TX descriptor */
		MEC_TXDESCSYNC(sc, nexttx,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/* start TX */
		bus_space_write_8(st, sh, MEC_TX_RING_PTR, MEC_NEXTTX(nexttx));

		/* advance the TX pointer. */
		sc->sc_txpending++;
		sc->sc_txlast = nexttx;
	}

	if (sc->sc_txpending == MEC_NTXDESC - 1) {
		/* No more slots; notify upper layer. */
		MEC_EVCNT_INCR(&sc->sc_ev_txdstall);
		ifp->if_flags |= IFF_OACTIVE;
	}

	if (sc->sc_txpending != opending) {
		/*
		 * If the transmitter was idle,
		 * reset the txdirty pointer and re-enable TX interrupt.
		 */
		if (opending == 0) {
			sc->sc_txdirty = firsttx;
			bus_space_write_8(st, sh, MEC_TX_ALIAS,
			    MEC_TX_ALIAS_INT_ENABLE);
		}

		/* Set a watchdog timer in case the chip flakes out. */
		ifp->if_timer = 5;
	}
}

static void
mec_stop(struct ifnet *ifp, int disable)
{
	struct mec_softc *sc = ifp->if_softc;
	struct mec_txsoft *txs;
	int i;

	DPRINTF(MEC_DEBUG_STOP, ("%s\n", __func__));

	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	callout_stop(&sc->sc_tick_ch);
	mii_down(&sc->sc_mii);

	/* release any TX buffers */
	for (i = 0; i < MEC_NTXDESC; i++) {
		txs = &sc->sc_txsoft[i];
		if ((txs->txs_flags & MEC_TXS_TXDPTR) != 0) {
			bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
			m_freem(txs->txs_mbuf);
			txs->txs_mbuf = NULL;
		}
	}
}

static int
mec_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	int s, error;

	s = splnet();

	error = ether_ioctl(ifp, cmd, data);
	if (error == ENETRESET) {
		/*
		 * Multicast list has changed; set the hardware filter
		 * accordingly.
		 */
		if (ifp->if_flags & IFF_RUNNING)
			error = mec_init(ifp);
		else
			error = 0;
	}

	/* Try to get more packets going. */
	mec_start(ifp);

	splx(s);
	return error;
}

static void
mec_watchdog(struct ifnet *ifp)
{
	struct mec_softc *sc = ifp->if_softc;

	printf("%s: device timeout\n", device_xname(sc->sc_dev));
	ifp->if_oerrors++;

	mec_init(ifp);
}

static void
mec_tick(void *arg)
{
	struct mec_softc *sc = arg;
	int s;

	s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	callout_reset(&sc->sc_tick_ch, hz, mec_tick, sc);
}

static void
mec_setfilter(struct mec_softc *sc)
{
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	uint64_t mchash;
	uint32_t control, hash;
	int mcnt;

	control = bus_space_read_8(st, sh, MEC_MAC_CONTROL);
	control &= ~MEC_MAC_FILTER_MASK;

	if (ifp->if_flags & IFF_PROMISC) {
		control |= MEC_MAC_FILTER_PROMISC;
		bus_space_write_8(st, sh, MEC_MULTICAST, 0xffffffffffffffffULL);
		bus_space_write_8(st, sh, MEC_MAC_CONTROL, control);
		return;
	}

	mcnt = 0;
	mchash = 0;
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			/* set allmulti for a range of multicast addresses */
			control |= MEC_MAC_FILTER_ALLMULTI;
			bus_space_write_8(st, sh, MEC_MULTICAST,
			    0xffffffffffffffffULL);
			bus_space_write_8(st, sh, MEC_MAC_CONTROL, control);
			return;
		}

#define mec_calchash(addr)	(ether_crc32_be((addr), ETHER_ADDR_LEN) >> 26)

		hash = mec_calchash(enm->enm_addrlo);
		mchash |= 1 << hash;
		mcnt++;
		ETHER_NEXT_MULTI(step, enm);
	}

	ifp->if_flags &= ~IFF_ALLMULTI;

	if (mcnt > 0)
		control |= MEC_MAC_FILTER_MATCHMULTI;

	bus_space_write_8(st, sh, MEC_MULTICAST, mchash);
	bus_space_write_8(st, sh, MEC_MAC_CONTROL, control);
}

static int
mec_intr(void *arg)
{
	struct mec_softc *sc = arg;
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint32_t statreg, statack, txptr;
	int handled, sent;

	DPRINTF(MEC_DEBUG_INTR, ("%s: called\n", __func__));

	handled = sent = 0;

	for (;;) {
		statreg = bus_space_read_8(st, sh, MEC_INT_STATUS);

		DPRINTF(MEC_DEBUG_INTR,
		    ("%s: INT_STAT = 0x%08x\n", __func__, statreg));

		statack = statreg & MEC_INT_STATUS_MASK;
		if (statack == 0)
			break;
		bus_space_write_8(st, sh, MEC_INT_STATUS, statack);

		handled = 1;

		if (statack &
		    (MEC_INT_RX_THRESHOLD |
		     MEC_INT_RX_FIFO_UNDERFLOW)) {
			mec_rxintr(sc);
		}

		if (statack &
		    (MEC_INT_TX_EMPTY |
		     MEC_INT_TX_PACKET_SENT |
		     MEC_INT_TX_ABORT)) {
			txptr = (statreg & MEC_INT_TX_RING_BUFFER_ALIAS)
			    >> MEC_INT_TX_RING_BUFFER_SHIFT;
			mec_txintr(sc, txptr);
			sent = 1;
			if ((statack & MEC_INT_TX_EMPTY) != 0) {
				/*
				 * disable TX interrupt to stop
				 * TX empty interrupt
				 */
				bus_space_write_8(st, sh, MEC_TX_ALIAS, 0);
				DPRINTF(MEC_DEBUG_INTR,
				    ("%s: disable TX_INT\n", __func__));
			}
#ifdef MEC_EVENT_COUNTERS
			if ((statack & MEC_INT_TX_EMPTY) != 0)
				MEC_EVCNT_INCR(&sc->sc_ev_txempty);
			if ((statack & MEC_INT_TX_PACKET_SENT) != 0)
				MEC_EVCNT_INCR(&sc->sc_ev_txsent);
#endif
		}

		if (statack &
		    (MEC_INT_TX_LINK_FAIL |
		     MEC_INT_TX_MEM_ERROR |
		     MEC_INT_TX_ABORT |
		     MEC_INT_RX_FIFO_UNDERFLOW |
		     MEC_INT_RX_DMA_UNDERFLOW)) {
			printf("%s: %s: interrupt status = 0x%08x\n",
			    device_xname(sc->sc_dev), __func__, statreg);
			mec_init(ifp);
			break;
		}
	}

	if (sent && !IFQ_IS_EMPTY(&ifp->if_snd)) {
		/* try to get more packets going */
		mec_start(ifp);
	}

#if NRND > 0
	if (handled)
		rnd_add_uint32(&sc->sc_rnd_source, statreg);
#endif

	return handled;
}

static void
mec_rxintr(struct mec_softc *sc)
{
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m;
	struct mec_rxdesc *rxd;
	uint64_t rxstat;
	u_int len;
	int i;
	uint32_t crc;

	DPRINTF(MEC_DEBUG_RXINTR, ("%s: called\n", __func__));

	for (i = sc->sc_rxptr;; i = MEC_NEXTRX(i)) {
		rxd = &sc->sc_rxdesc[i];

		MEC_RXSTATSYNC(sc, i, BUS_DMASYNC_POSTREAD);
		rxstat = rxd->rxd_stat;

		DPRINTF(MEC_DEBUG_RXINTR,
		    ("%s: rxstat = 0x%016llx, rxptr = %d\n",
		    __func__, rxstat, i));
		DPRINTF(MEC_DEBUG_RXINTR, ("%s: rxfifo = 0x%08x\n",
		    __func__, (u_int)bus_space_read_8(st, sh, MEC_RX_FIFO)));

		if ((rxstat & MEC_RXSTAT_RECEIVED) == 0) {
			MEC_RXSTATSYNC(sc, i, BUS_DMASYNC_PREREAD);
			break;
		}

		len = rxstat & MEC_RXSTAT_LEN;

		if (len < ETHER_MIN_LEN ||
		    len > (MCLBYTES - MEC_ETHER_ALIGN)) {
			/* invalid length packet; drop it. */
			DPRINTF(MEC_DEBUG_RXINTR,
			    ("%s: wrong packet\n", __func__));
 dropit:
			ifp->if_ierrors++;
			rxd->rxd_stat = 0;
			MEC_RXSTATSYNC(sc, i, BUS_DMASYNC_PREREAD);
			bus_space_write_8(st, sh, MEC_MCL_RX_FIFO,
			    MEC_CDRXADDR(sc, i));
			continue;
		}

		/*
		 * If 802.1Q VLAN MTU is enabled, ignore the bad packet errror.
		 */
		if ((sc->sc_ethercom.ec_capenable & ETHERCAP_VLAN_MTU) != 0)
			rxstat &= ~MEC_RXSTAT_BADPACKET;

		if (rxstat &
		    (MEC_RXSTAT_BADPACKET |
		     MEC_RXSTAT_LONGEVENT |
		     MEC_RXSTAT_INVALID   |
		     MEC_RXSTAT_CRCERROR  |
		     MEC_RXSTAT_VIOLATION)) {
			printf("%s: %s: status = 0x%016llx\n",
			    device_xname(sc->sc_dev), __func__, rxstat);
			goto dropit;
		}

		/*
		 * The MEC includes the CRC with every packet.  Trim
		 * it off here.
		 */
		len -= ETHER_CRC_LEN;

		/*
		 * now allocate an mbuf (and possibly a cluster) to hold
		 * the received packet.
		 */
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
			printf("%s: unable to allocate RX mbuf\n",
			    device_xname(sc->sc_dev));
			goto dropit;
		}
		if (len > (MHLEN - MEC_ETHER_ALIGN)) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				printf("%s: unable to allocate RX cluster\n",
				    device_xname(sc->sc_dev));
				m_freem(m);
				m = NULL;
				goto dropit;
			}
		}

		/*
		 * Note MEC chip seems to insert 2 byte padding at the top of
		 * RX buffer, but we copy whole buffer to avoid unaligned copy.
		 */
		MEC_RXBUFSYNC(sc, i, len + ETHER_CRC_LEN, BUS_DMASYNC_POSTREAD);
		memcpy(mtod(m, void *), rxd->rxd_buf, MEC_ETHER_ALIGN + len);
		crc = be32dec(rxd->rxd_buf + MEC_ETHER_ALIGN + len);
		MEC_RXBUFSYNC(sc, i, ETHER_MAX_LEN, BUS_DMASYNC_PREREAD);
		m->m_data += MEC_ETHER_ALIGN;

		/* put RX buffer into FIFO again */
		rxd->rxd_stat = 0;
		MEC_RXSTATSYNC(sc, i, BUS_DMASYNC_PREREAD);
		bus_space_write_8(st, sh, MEC_MCL_RX_FIFO, MEC_CDRXADDR(sc, i));

		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = len;
		if ((ifp->if_csum_flags_rx & (M_CSUM_TCPv4|M_CSUM_UDPv4)) != 0)
			mec_rxcsum(sc, m, RXSTAT_CKSUM(rxstat), crc);

		ifp->if_ipackets++;

#if NBPFILTER > 0
		/*
		 * Pass this up to any BPF listeners, but only
		 * pass it up the stack if it's for us.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif

		/* Pass it on. */
		(*ifp->if_input)(ifp, m);
	}

	/* update RX pointer */
	sc->sc_rxptr = i;
}

static void
mec_rxcsum(struct mec_softc *sc, struct mbuf *m, uint16_t rxcsum, uint32_t crc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ether_header *eh;
	struct ip *ip;
	struct udphdr *uh;
	u_int len, pktlen, hlen;
	uint32_t csum_data, dsum;
	int csum_flags;
	const uint16_t *dp;

	csum_data = 0;
	csum_flags = 0;

	len = m->m_len;
	if (len < ETHER_HDR_LEN + sizeof(struct ip))
		goto out;
	pktlen = len - ETHER_HDR_LEN;
	eh = mtod(m, struct ether_header *);
	if (ntohs(eh->ether_type) != ETHERTYPE_IP)
		goto out;
	ip = (struct ip *)((uint8_t *)eh + ETHER_HDR_LEN);
	if (ip->ip_v != IPVERSION)
		goto out;

	hlen = ip->ip_hl << 2;
	if (hlen < sizeof(struct ip))
		goto out;

	/*
	 * Bail if too short, has random trailing garbage, truncated,
	 * fragment, or has ethernet pad.
	 */
	if (ntohs(ip->ip_len) < hlen ||
	    ntohs(ip->ip_len) != pktlen ||
	    (ntohs(ip->ip_off) & (IP_MF | IP_OFFMASK)) != 0)
		goto out;

	switch (ip->ip_p) {
	case IPPROTO_TCP:
		if ((ifp->if_csum_flags_rx & M_CSUM_TCPv4) == 0 ||
		    pktlen < (hlen + sizeof(struct tcphdr)))
			goto out;
		csum_flags = M_CSUM_TCPv4 | M_CSUM_DATA | M_CSUM_NO_PSEUDOHDR;
		break;
	case IPPROTO_UDP:
		if ((ifp->if_csum_flags_rx & M_CSUM_UDPv4) == 0 ||
		    pktlen < (hlen + sizeof(struct udphdr)))
			goto out;
		uh = (struct udphdr *)((uint8_t *)ip + hlen);
		if (uh->uh_sum == 0)
			goto out;	/* no checksum */
		csum_flags = M_CSUM_UDPv4 | M_CSUM_DATA | M_CSUM_NO_PSEUDOHDR;
		break;
	default:
		goto out;
	}

	/*
	 * The computed checksum includes Ethernet header, IP headers,
	 * and CRC, so we have to deduct them.
	 * Note IP header cksum should be 0xffff so we don't have to
	 * dedecut them.
	 */
	dsum = 0;

	/* deduct Ethernet header */
	dp = (const uint16_t *)eh;
	for (hlen = 0; hlen < (ETHER_HDR_LEN / sizeof(uint16_t)); hlen++)
		dsum += ntohs(*dp++);

	/* deduct CRC */
	if (len & 1) {
		dsum += (crc >> 24) & 0x00ff;
		dsum += (crc >>  8) & 0xffff;
		dsum += (crc <<  8) & 0xff00;
	} else {
		dsum += (crc >> 16) & 0xffff;
		dsum += (crc >>  0) & 0xffff;
	}
	while (dsum >> 16)
		dsum = (dsum >> 16) + (dsum & 0xffff);

	csum_data = rxcsum;
	csum_data += (uint16_t)~dsum;

	while (csum_data >> 16)
		csum_data = (csum_data >> 16) + (csum_data & 0xffff);

 out:
	m->m_pkthdr.csum_flags = csum_flags;
	m->m_pkthdr.csum_data = csum_data;
}

static void
mec_txintr(struct mec_softc *sc, uint32_t txptr)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mec_txdesc *txd;
	struct mec_txsoft *txs;
	bus_dmamap_t dmamap;
	uint64_t txstat;
	int i;
	u_int col;

	DPRINTF(MEC_DEBUG_TXINTR, ("%s: called\n", __func__));

	for (i = sc->sc_txdirty; i != txptr && sc->sc_txpending != 0;
	    i = MEC_NEXTTX(i), sc->sc_txpending--) {
		txd = &sc->sc_txdesc[i];

		MEC_TXCMDSYNC(sc, i,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		txstat = txd->txd_stat;
		DPRINTF(MEC_DEBUG_TXINTR,
		    ("%s: dirty = %d, txstat = 0x%016llx\n",
		    __func__, i, txstat));
		if ((txstat & MEC_TXSTAT_SENT) == 0) {
			MEC_TXCMDSYNC(sc, i, BUS_DMASYNC_PREREAD);
			break;
		}

		txs = &sc->sc_txsoft[i];
		if ((txs->txs_flags & MEC_TXS_TXDPTR) != 0) {
			dmamap = txs->txs_dmamap;
			bus_dmamap_sync(sc->sc_dmat, dmamap, 0,
			    dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->sc_dmat, dmamap);
			m_freem(txs->txs_mbuf);
			txs->txs_mbuf = NULL;
		}

		col = (txstat & MEC_TXSTAT_COLCNT) >> MEC_TXSTAT_COLCNT_SHIFT;
		ifp->if_collisions += col;

		if ((txstat & MEC_TXSTAT_SUCCESS) == 0) {
			printf("%s: TX error: txstat = 0x%016llx\n",
			    device_xname(sc->sc_dev), txstat);
			ifp->if_oerrors++;
		} else
			ifp->if_opackets++;
	}

	/* update the dirty TX buffer pointer */
	sc->sc_txdirty = i;
	DPRINTF(MEC_DEBUG_INTR,
	    ("%s: sc_txdirty = %2d, sc_txpending = %2d\n",
	    __func__, sc->sc_txdirty, sc->sc_txpending));

	/* cancel the watchdog timer if there are no pending TX packets */
	if (sc->sc_txpending == 0)
		ifp->if_timer = 0;
	if (sc->sc_txpending < MEC_NTXDESC - MEC_NTXDESC_RSVD)
		ifp->if_flags &= ~IFF_OACTIVE;
}

static void
mec_shutdown(void *arg)
{
	struct mec_softc *sc = arg;

	mec_stop(&sc->sc_ethercom.ec_if, 1);
	/* make sure to stop DMA etc. */
	mec_reset(sc);
}
