/*	$NetBSD: if_de.c,v 1.1 2000/04/30 11:43:26 ragge Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989 Regents of the University of California.
 * Copyright (c) 2000 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)if_de.c	7.12 (Berkeley) 12/16/90
 */

/*
 * DEC DEUNA interface
 *
 *	Lou Salkind
 *	New York University
 *
 *	Rewritten by Ragge 000430 to match new world.
 *
 * TODO:
 *	timeout routine (get statistics)
 */

#include "opt_inet.h"
#include "opt_iso.h"
#include "opt_ns.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_dl.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#ifdef ISO
#include <netiso/iso.h>
#include <netiso/iso_var.h>
extern char all_es_snpa[], all_is_snpa[];
#endif

#include <machine/bus.h>

#include <dev/qbus/ubavar.h>
#include <dev/qbus/if_dereg.h>

#include "ioconf.h"

/*
 * Be careful with transmit/receive buffers, each entry steals 4 map
 * registers, and there is only 496 on one unibus...
 */
#define NRCV	10	/* number of receive buffers (must be > 1) */
#define NXMT	20	/* number of transmit buffers */
#define NFRAGS	8	/* Number of frags per transmit buffer */

/*
 * Structure containing the elements that must be in DMA-safe memory.
 */
struct	de_cdata {
	/* the following structures are always mapped in */
	struct	de_pcbb dc_pcbb;	/* port control block */
	struct	de_ring dc_xrent[NXMT]; /* transmit ring entrys */
	struct	de_ring dc_rrent[NRCV]; /* receive ring entrys */
	struct	de_udbbuf dc_udbbuf;	/* UNIBUS data buffer */
	/* end mapped area */
};

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * ds_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 * We also have, for each interface, a UBA interface structure, which
 * contains information about the UNIBUS resources held by the interface:
 * map registers, buffered data paths, etc.  Information is cached in this
 * structure for use by the if_uba.c routines in running the interface
 * efficiently.
 */
struct	de_softc {
	struct	device sc_dev;		/* Configuration common part */
	struct	ethercom sc_ec;		/* Ethernet common part */
#define sc_if	sc_ec.ec_if		/* network-visible interface */
	int	sc_flags;
#define DSF_RUNNING	2		/* board is enabled */
#define DSF_SETADDR	4		/* physical address is changed */
	bus_space_tag_t sc_iot;
	bus_addr_t sc_ioh;
	bus_dma_tag_t sc_dmat;
	struct de_cdata *sc_dedata;	/* Control structure */
	struct de_cdata *sc_pdedata;	/* Bus-mapped control structure */
	bus_dmamap_t sc_xmtmap[NXMT];	/* unibus receive maps */
	bus_dmamap_t sc_rcvmap[NRCV];	/* unibus xmt maps */
	struct mbuf *sc_txmbuf[NXMT];
	struct mbuf *sc_rxmbuf[NRCV];
	int sc_nexttx;
	int sc_nextrx;
	int sc_inq;
	int sc_lastack;
};

static	int dematch(struct device *, struct cfdata *, void *);
static	void deattach(struct device *, struct device *, void *);
static	int dewait(struct de_softc *, char *);
static	void deinit(struct de_softc *);
static	int deioctl(struct ifnet *, u_long, caddr_t);
static	void dereset(struct device *);
static	void destart(struct ifnet *);
static	void derecv(struct de_softc *);
static	void dexmit(struct de_softc *);
static	void de_setaddr(u_char *, struct de_softc *);
static	void deintr(void *);
static	int de_add_rxbuf(struct de_softc *, int);

struct	cfattach de_ca = {
	sizeof(struct de_softc), dematch, deattach
};

#define DE_WCSR(csr, val) \
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, csr, val)
#define DE_WLOW(val) \
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, DE_PCSR0, val)
#define DE_WHIGH(val) \
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, DE_PCSR0 + 1, val)
#define DE_RCSR(csr) \
	bus_space_read_2(sc->sc_iot, sc->sc_ioh, csr)

#define LOWORD(x)	((int)(x) & 0xffff)
#define HIWORD(x)	(((int)(x) >> 16) & 0x3)
/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.  We get the ethernet address here.
 */
void
deattach(struct device *parent, struct device *self, void *aux)
{
	struct uba_attach_args *ua = aux;
	struct de_softc *sc = (struct de_softc *)self;
	struct ifnet *ifp = &sc->sc_if;
	u_int8_t myaddr[ETHER_ADDR_LEN];
	bus_dma_segment_t seg;
	int csr1, rseg, error, i;
	char *c;

	sc->sc_iot = ua->ua_iot;
	sc->sc_ioh = ua->ua_ioh;
	sc->sc_dmat = ua->ua_dmat;

	/*
	 * What kind of a board is this?
	 * The error bits 4-6 in pcsr1 are a device id as long as
	 * the high byte is zero.
	 */
	csr1 = DE_RCSR(DE_PCSR1);
	if (csr1 & 0xff60)
		c = "broken";
	else if (csr1 & 0x10)
		c = "delua";
	else
		c = "deuna";

	/*
	 * Reset the board and temporarily map
	 * the pcbb buffer onto the Unibus.
	 */
	DE_WCSR(DE_PCSR0, 0);		/* reset INTE */
	DELAY(100);
	DE_WCSR(DE_PCSR0, PCSR0_RSET);
	(void)dewait(sc, "reset");

	if ((error = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof(struct de_cdata), NBPG, 0, &seg, 1, &rseg,
	    BUS_DMA_NOWAIT)) != 0) {
		printf(": unable to allocate control data, error = %d\n",
		    error);
		goto fail_0;
	}
	if ((error = bus_dmamem_map(sc->sc_dmat, &seg, rseg,
	    sizeof(struct de_cdata), (caddr_t *)&sc->sc_dedata,
	    BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		printf(": unable to map control data, error = %d\n", error);
		goto fail_1;
	}

	/*
	 * Create the transmit descriptor DMA maps.
	 *
	 * XXX - should allocate transmit map pages when needed, not here.
	 */
	for (i = 0; i < NXMT; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, NFRAGS,
		    MCLBYTES, 0, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW,
		    &sc->sc_xmtmap[i]))) {
			printf(": unable to create tx DMA map %d, error = %d\n",
			    i, error);
			goto fail_4;
		}
	}

	/*
	 * Create receive buffer DMA maps.
	 */
	for (i = 0; i < NRCV; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
		    MCLBYTES, 0, BUS_DMA_NOWAIT,
		    &sc->sc_rcvmap[i]))) {
			printf(": unable to create rx DMA map %d, error = %d\n",
			    i, error);
			goto fail_5;
		}
	}

	/*
	 * Pre-allocate the receive buffers.
	 */
	for (i = 0; i < NRCV; i++) {
		if ((error = de_add_rxbuf(sc, i)) != 0) {
			printf(": unable to allocate or map rx buffer %d\n,"
			    " error = %d\n", i, error);
			goto fail_6;
		}
	}

	bzero(sc->sc_dedata, sizeof(struct de_cdata));
	sc->sc_pdedata = (struct de_cdata *)seg.ds_addr;

	/*
	 * Tell the DEUNA about our PCB
	 */
	DE_WCSR(DE_PCSR2, LOWORD(sc->sc_pdedata));
	DE_WCSR(DE_PCSR3, HIWORD(sc->sc_pdedata));
	DE_WLOW(CMD_GETPCBB);
	(void)dewait(sc, "pcbb");

	sc->sc_dedata->dc_pcbb.pcbb0 = FC_RDPHYAD;
	DE_WLOW(CMD_GETCMD);
	(void)dewait(sc, "read addr ");

	bcopy((caddr_t)&sc->sc_dedata->dc_pcbb.pcbb2, myaddr, sizeof (myaddr));
	printf("%s: %s, hardware address %s\n", c, sc->sc_dev.dv_xname,
		ether_sprintf(myaddr));

	uba_intr_establish(ua->ua_icookie, ua->ua_cvec, deintr, sc);
	uba_reset_establish(dereset, &sc->sc_dev);

	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_MULTICAST | IFF_SIMPLEX;
	ifp->if_ioctl = deioctl;
	ifp->if_start = destart;
	if_attach(ifp);
	ether_ifattach(ifp, myaddr);
#if NBPFILTER > 0
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif
	return;

	/*
	 * Free any resources we've allocated during the failed attach
	 * attempt.  Do this in reverse order and fall through.
	 */
fail_6:
	for (i = 0; i < NRCV; i++) {
		if (sc->sc_rxmbuf[i] != NULL) {
			bus_dmamap_unload(sc->sc_dmat, sc->sc_xmtmap[i]);
			m_freem(sc->sc_rxmbuf[i]);
		}
	}
fail_5:
	for (i = 0; i < NRCV; i++) {
		if (sc->sc_xmtmap[i] != NULL)
			bus_dmamap_destroy(sc->sc_dmat, sc->sc_xmtmap[i]);
	}
fail_4: 
	for (i = 0; i < NXMT; i++) {
		if (sc->sc_rcvmap[i] != NULL)
			bus_dmamap_destroy(sc->sc_dmat, sc->sc_rcvmap[i]);
	}
	bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_dedata,
	    sizeof(struct de_cdata));
fail_1:
	bus_dmamem_free(sc->sc_dmat, &seg, rseg);
fail_0:
	return;
}

/*
 * Reset of interface after UNIBUS reset.
 */
void
dereset(struct device *dev)
{
	struct de_softc *sc = (void *)dev;

	sc->sc_if.if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	sc->sc_flags &= ~DSF_RUNNING;
	DE_WCSR(DE_PCSR0, PCSR0_RSET);
	(void)dewait(sc, "reset");
	deinit(sc);
}

/*
 * Initialization of interface; clear recorded pending
 * operations, and reinitialize UNIBUS usage.
 */
void
deinit(struct de_softc *sc)
{
	struct de_cdata *dc;
	int s, i;

	if (sc->sc_flags & DSF_RUNNING)
		return;
	/*
	 * Tell the DEUNA about our PCB
	 */
	DE_WCSR(DE_PCSR2, LOWORD(sc->sc_pdedata));
	DE_WCSR(DE_PCSR3, HIWORD(sc->sc_pdedata));
	DE_WLOW(0);		/* reset INTE */
	DELAY(500);
	DE_WLOW(CMD_GETPCBB);
	(void)dewait(sc, "pcbb");

	dc = sc->sc_dedata;
	/* set the transmit and receive ring header addresses */
	dc->dc_pcbb.pcbb0 = FC_WTRING;
	dc->dc_pcbb.pcbb2 = LOWORD(&sc->sc_pdedata->dc_udbbuf);
	dc->dc_pcbb.pcbb2 = HIWORD(&sc->sc_pdedata->dc_udbbuf);

	dc->dc_udbbuf.b_tdrbl = LOWORD(&sc->sc_pdedata->dc_xrent[0]);
	dc->dc_udbbuf.b_tdrbh = HIWORD(&sc->sc_pdedata->dc_xrent[0]);
	dc->dc_udbbuf.b_telen = sizeof (struct de_ring) / sizeof(u_int16_t);
	dc->dc_udbbuf.b_trlen = NXMT;
	dc->dc_udbbuf.b_rdrbl = LOWORD(&sc->sc_pdedata->dc_rrent[0]);
	dc->dc_udbbuf.b_rdrbh = HIWORD(&sc->sc_pdedata->dc_rrent[0]);
	dc->dc_udbbuf.b_relen = sizeof (struct de_ring) / sizeof(u_int16_t);
	dc->dc_udbbuf.b_rrlen = NRCV;

	DE_WLOW(CMD_GETCMD);
	(void)dewait(sc, "wtring");

	/* initialize the mode - enable hardware padding */
	dc->dc_pcbb.pcbb0 = FC_WTMODE;
	/* let hardware do padding - set MTCH bit on broadcast */
	dc->dc_pcbb.pcbb2 = MOD_TPAD|MOD_HDX;
	DE_WLOW(CMD_GETCMD);
	(void)dewait(sc, "wtmode");

	/* set up the receive and transmit ring entries */
	for (i = 0; i < NXMT; i++) {
		if (sc->sc_txmbuf[i]) {
			bus_dmamap_unload(sc->sc_dmat, sc->sc_xmtmap[i]);
			m_freem(sc->sc_txmbuf[i]);
			sc->sc_txmbuf[i] = 0;
		}
		dc->dc_xrent[i].r_flags = 0;
	}
	for (i = 0; i < NRCV; i++)
		dc->dc_rrent[i].r_flags = RFLG_OWN;
	sc->sc_nexttx = sc->sc_inq = sc->sc_lastack = sc->sc_nextrx = 0;

	/* start up the board (rah rah) */
	s = splnet();
	sc->sc_if.if_flags |= IFF_RUNNING;
	DE_WLOW(PCSR0_INTE);
	destart(&sc->sc_if);		/* queue output packets */
	sc->sc_flags |= DSF_RUNNING;		/* need before de_setaddr */
	if (sc->sc_flags & DSF_SETADDR)
		de_setaddr(LLADDR(sc->sc_if.if_sadl), sc);
	DE_WLOW(CMD_START|PCSR0_INTE);
	splx(s);
}

/*
 * Setup output on interface.
 * Get another datagram to send off of the interface queue,
 * and map it to the interface before starting the output.
 * Must be called from ipl >= our interrupt level.
 */
void
destart(struct ifnet *ifp)
{
	struct de_softc *sc = ifp->if_softc;
	bus_dmamap_t map;
	struct de_ring *rp = 0;
	struct mbuf *m, *m0;
	int idx, i, err, seg, s;

	/*
	 * the following test is necessary, since
	 * the code is not reentrant and we have
	 * multiple transmission buffers.
	 */
	if (ifp->if_flags & IFF_OACTIVE)
		return;
	s = splimp();
	while (sc->sc_inq < (NXMT - 1)) {
		idx = sc->sc_nexttx;
		IF_DEQUEUE(&ifp->if_snd, m);
		if (m == 0)
			goto out;
		/*
		 * Count number of mbufs in chain.
		 * Always do DMA directly from mbufs.
		 */
		for (m0 = m, i = 0; m0; m0 = m0->m_next)
			if (m0->m_len)
				i++;
		if (i >= NFRAGS) { /* XXX pullup */
			panic("destart");
		}

		if ((i + sc->sc_inq) >= (NXMT - 1)) {
			IF_PREPEND(&sc->sc_if.if_snd, m);
			ifp->if_flags |= IFF_OACTIVE;
			goto out;
		}
#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif
		/*
		 * m now points to a mbuf chain that can be loaded.
		 * Loop around and set it.
		 */
		err = bus_dmamap_load_mbuf(sc->sc_dmat, sc->sc_xmtmap[idx],
		    m, BUS_DMA_NOWAIT);
		if (err) /* Can't fail here */
			panic("destart: load_mbuf failed, err %d", err);

		sc->sc_txmbuf[idx] = m;
		map = sc->sc_xmtmap[idx];
		for (seg = 0; seg < map->dm_nsegs; seg++) {
			rp = &sc->sc_dedata->dc_xrent[idx];

			rp->r_flags = seg == 0 ? XFLG_STP : 0;
			rp->r_slen = map->dm_segs[seg].ds_len;
			rp->r_segbl = LOWORD(map->dm_segs[seg].ds_addr);
			rp->r_segbh = HIWORD(map->dm_segs[seg].ds_addr);
			
			if (++idx == NXMT)
				idx = 0;
			sc->sc_inq++;
		}
		rp->r_flags |= XFLG_ENP|XFLG_OWN;
		sc->sc_nexttx = idx;
	}
	if (sc->sc_inq == (NXMT - 1))
		ifp->if_flags |= IFF_OACTIVE;

out:	if (sc->sc_inq)
		ifp->if_timer = 5; /* If transmit logic dies */

	if (sc->sc_flags & DSF_RUNNING)
		DE_WLOW(PCSR0_INTE|CMD_PDMD);
	splx(s);
}

/*
 * Command done interrupt.
 */
void
deintr(void *arg)
{
	struct de_softc *sc = arg;
	short csr0;

	/* save flags right away - clear out interrupt bits */
	csr0 = DE_RCSR(DE_PCSR0);
	DE_WHIGH(csr0 >> 8);

	if (csr0 & PCSR0_RXI)
		derecv(sc);

	if (csr0 & PCSR0_TXI)
		dexmit(sc);

#ifdef notyet
	Handle error interrupts
#endif
	destart(&sc->sc_if);
}

void
dexmit(struct de_softc *sc)
{
	struct ifnet *ifp = &sc->sc_if;
	struct de_ring *rp;
	int midx;

	/*
	 * Poll transmit ring and check status.
	 * Then free buffer space and check for
	 * more transmit requests.
	 */
	midx = -1;
	rp = &sc->sc_dedata->dc_xrent[sc->sc_lastack];
	while ((rp->r_flags & XFLG_OWN) == 0) {
		if (sc->sc_txmbuf[sc->sc_lastack])
			midx = sc->sc_lastack;
		if (rp->r_flags & XFLG_ENP) {
			if (midx >= 0)
				m_freem(sc->sc_txmbuf[midx]);
			ifp->if_opackets++;
		}
		if (rp->r_flags & (XFLG_ERRS|XFLG_MTCH|XFLG_ONE|XFLG_MORE)) {
			if (rp->r_flags & XFLG_ERRS) {
				ifp->if_oerrors++;
			} else if (rp->r_flags & XFLG_ONE) {
				ifp->if_collisions++;
			} else if (rp->r_flags & XFLG_MORE) {
				ifp->if_collisions += 3;
			}
			/* else if (rp->r_flags & XFLG_MTCH)
			 * Matches ourself, but why care?
			 * Let upper layer deal with this.
			 */
		}
		if (++sc->sc_lastack == NXMT)
			sc->sc_lastack = 0;
		sc->sc_inq--;
		rp = &sc->sc_dedata->dc_xrent[sc->sc_lastack];
	}
	ifp->if_flags &= ~IFF_OACTIVE;
	if (sc->sc_inq == 0)
		ifp->if_timer = 0;
}

/*
 * Ethernet interface receiver interface.
 * If input error just drop packet.
 * Otherwise purge input buffered data path and examine 
 * packet to determine type.  If can't determine length
 * from type, then have to drop packet.	 Othewise decapsulate
 * packet based on type and pass to type specific higher-level
 * input routine.
 */
void
derecv(struct de_softc *sc)
{
	struct ifnet *ifp = &sc->sc_if;
	struct ether_header *eh;
	struct de_ring *rp;
	struct mbuf *m;
	int len;

	rp = &sc->sc_dedata->dc_rrent[sc->sc_nextrx];
	while ((rp->r_flags & RFLG_OWN) == 0) {
		ifp->if_ipackets++;
		len = (rp->r_lenerr&RERR_MLEN) - 4; /* don't forget checksum! */
		/* check for errors */
		if ((rp->r_flags & (RFLG_ERRS|RFLG_FRAM|RFLG_OFLO|RFLG_CRC)) ||
		    (rp->r_flags&(RFLG_STP|RFLG_ENP)) != (RFLG_STP|RFLG_ENP) ||
		    (rp->r_lenerr & (RERR_BUFL|RERR_UBTO|RERR_NCHN)) ||
		    len < ETHERMIN || len > ETHERMTU) {
			ifp->if_ierrors++;
			goto next;
		}
		m = sc->sc_rxmbuf[sc->sc_nextrx];
		de_add_rxbuf(sc, sc->sc_nextrx);
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = len;
		eh = mtod(m, struct ether_header *);
#if NBPFILTER > 0
		if (ifp->if_bpf) {
			bpf_mtap(ifp->if_bpf, m);
			if ((ifp->if_flags & IFF_PROMISC) != 0 &&
			    bcmp(LLADDR(ifp->if_sadl), eh->ether_dhost,
			    ETHER_ADDR_LEN) != 0 &&
			    ((eh->ether_dhost[0] & 1) == 0)) {
				m_freem(m);
				goto next;
			}
		}
#endif
		(*ifp->if_input)(ifp, m);

		/* hang the receive buffer again */
next:		rp->r_lenerr = 0;
		rp->r_flags = RFLG_OWN;

		/* check next receive buffer */
		if (++sc->sc_nextrx == NRCV)
			sc->sc_nextrx = 0;
		rp = &sc->sc_dedata->dc_rrent[sc->sc_nextrx];
	}
}

/*
 * Add a receive buffer to the indicated descriptor.
 */
int
de_add_rxbuf(sc, i) 
	struct de_softc *sc;
	int i;
{
	struct mbuf *m;
	struct de_ring *rp;
	vaddr_t addr;
	int error;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return (ENOBUFS);

	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return (ENOBUFS);
	}

	if (sc->sc_rxmbuf[i] != NULL)
		bus_dmamap_unload(sc->sc_dmat, sc->sc_rcvmap[i]);

	error = bus_dmamap_load(sc->sc_dmat, sc->sc_rcvmap[i],
	    m->m_ext.ext_buf, m->m_ext.ext_size, NULL, BUS_DMA_NOWAIT);
	if (error)
		panic("%s: can't load rx DMA map %d, error = %d\n",
		    sc->sc_dev.dv_xname, i, error);
	sc->sc_rxmbuf[i] = m;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_rcvmap[i], 0,
	    sc->sc_rcvmap[i]->dm_mapsize, BUS_DMASYNC_PREREAD);

	/*
	 * We know that the mbuf cluster is page aligned. Also, be sure
	 * that the IP header will be longword aligned.
	 */
	m->m_data += 2;
	addr = sc->sc_rcvmap[i]->dm_segs[0].ds_addr + 2;
	rp = &sc->sc_dedata->dc_rrent[i];
	rp->r_lenerr = 0;
	rp->r_segbl = LOWORD(addr);
	rp->r_segbh = HIWORD(addr);
	rp->r_slen = m->m_ext.ext_size - 2;
	rp->r_flags = RFLG_OWN;

	return (0);
}


/*
 * Process an ioctl request.
 */
int
deioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct de_softc *sc = ifp->if_softc;
	int s = splnet(), error = 0;

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		deinit(sc);

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			arp_ifinit(ifp, ifa);
			break;
#endif
#ifdef NS
		case AF_NS:
		    {
			register struct ns_addr *ina = &(IA_SNS(ifa)->sns_addr);
			
			if (ns_nullhost(*ina))
				ina->x_host = 
				    *(union ns_host *)LLADDR(ifp->if_sadl);
			else
				de_setaddr(ina->x_host.c_host, ds);
			break;
		    }
#endif
		}
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    sc->sc_flags & DSF_RUNNING) {
			DE_WLOW(0);
			DELAY(5000);
			DE_WLOW(PCSR0_RSET);
			sc->sc_flags &= ~DSF_RUNNING;
			ifp->if_flags &= ~IFF_OACTIVE;
		} else if (ifp->if_flags & IFF_UP &&
		    (sc->sc_flags & DSF_RUNNING) == 0)
			deinit(sc);
		break;

	default:
		error = EINVAL;
	}
	splx(s);
	return (error);
}

/*
 * set ethernet address for unit
 */
void
de_setaddr(u_char *physaddr, struct de_softc *sc)
{

	if (! (sc->sc_flags & DSF_RUNNING))
		return;
		
	bcopy((caddr_t) physaddr, (caddr_t) &sc->sc_dedata->dc_pcbb.pcbb2, 6);
	sc->sc_dedata->dc_pcbb.pcbb0 = FC_WTPHYAD;
	DE_WLOW(PCSR0_INTE|CMD_GETCMD);
	if (dewait(sc, "address change") == 0) {
		sc->sc_flags |= DSF_SETADDR;
		bcopy((caddr_t) physaddr, LLADDR(sc->sc_if.if_sadl), 6);
	}
}

/*
 * Await completion of the named function
 * and check for errors.
 */
int
dewait(struct de_softc *sc, char *fn)
{
	int csr0;

	while ((DE_RCSR(DE_PCSR0) & PCSR0_INTR) == 0)
		;
	csr0 = DE_RCSR(DE_PCSR0);
	DE_WHIGH(csr0 >> 8);
	if (csr0 & PCSR0_PCEI) {
		char bits[64];

		printf("%s: %s failed, csr0=%s ", sc->sc_dev.dv_xname, fn,
		    bitmask_snprintf(csr0, PCSR0_BITS, bits, sizeof(bits)));
		printf("csr1=%s\n", bitmask_snprintf(DE_RCSR(DE_PCSR1),
		    PCSR1_BITS, bits, sizeof(bits)));
	}
	return (csr0 & PCSR0_PCEI);
}

int
dematch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct uba_attach_args *ua = aux;
	struct de_softc ssc;
	struct de_softc *sc = &ssc;
	int i;

	sc->sc_iot = ua->ua_iot;
	sc->sc_ioh = ua->ua_ioh;
	/*
	 * Make sure self-test is finished before we screw with the board.
	 * Self-test on a DELUA can take 15 seconds (argh).
	 */
	for (i = 0;
	    (i < 160) &&
	    (DE_RCSR(DE_PCSR0) & PCSR0_FATI) == 0 &&
	    (DE_RCSR(DE_PCSR1) & PCSR1_STMASK) == STAT_RESET;
	    ++i)
		DELAY(50000);
	if (((DE_RCSR(DE_PCSR0) & PCSR0_FATI) != 0) ||
	    (((DE_RCSR(DE_PCSR1) & PCSR1_STMASK) != STAT_READY) &&
	    ((DE_RCSR(DE_PCSR1) & PCSR1_STMASK) != STAT_RUN)))
		return(0);

	DE_WCSR(DE_PCSR0, 0);
	DELAY(5000);
	DE_WCSR(DE_PCSR0, PCSR0_RSET);
	while ((DE_RCSR(DE_PCSR0) & PCSR0_INTR) == 0)
		;
	/* make board interrupt by executing a GETPCBB command */
	DE_WCSR(DE_PCSR0, PCSR0_INTE);
	DE_WCSR(DE_PCSR2, 0);
	DE_WCSR(DE_PCSR3, 0);
	DE_WCSR(DE_PCSR0, PCSR0_INTE|CMD_GETPCBB);
	DELAY(50000);

	return 1;
}
