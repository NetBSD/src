/*	$NetBSD: if_de.c,v 1.2 2000/05/28 17:23:44 ragge Exp $	*/
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
 *	Rewritten by Ragge 30 April 2000 to match new world.
 *
 * TODO:
 *	timeout routine (get statistics)
 */

#include "opt_inet.h"
#include "opt_iso.h"
#include "bpfilter.h"

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

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
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
#define NXMT	10	/* number of transmit buffers */

/*
 * Structure containing the elements that must be in DMA-safe memory.
 */
struct	de_cdata {
	/* the following structures are always mapped in */
	struct	de_pcbb dc_pcbb;	/* port control block */
	struct	de_ring dc_xrent[NXMT]; /* transmit ring entrys */
	struct	de_ring dc_rrent[NRCV]; /* receive ring entrys */
	struct	de_udbbuf dc_udbbuf;	/* UNIBUS data buffer */
	char	dc_xbuf[NXMT][ETHER_MAX_LEN];
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
	bus_space_tag_t sc_iot;
	bus_addr_t sc_ioh;
	bus_dma_tag_t sc_dmat;
	bus_dmamap_t sc_cmap;
	struct de_cdata *sc_dedata;	/* Control structure */
	struct de_cdata *sc_pdedata;	/* Bus-mapped control structure */
#ifdef notdef
	bus_dmamap_t sc_xmtmap[NXMT];	/* unibus xmit maps */
	struct mbuf *sc_txmbuf[NXMT];
#endif
	bus_dmamap_t sc_rcvmap[NRCV];	/* unibus receive maps */
	struct mbuf *sc_rxmbuf[NRCV];
	int sc_nexttx;			/* next tx descriptor to put data on */
	int sc_nextrx;			/* next rx descriptor for recv */
	int sc_inq;			/* # if xmit packets in queue */
	int sc_lastack;			/* Last handled rx descriptor */
	void *sc_sh;			/* shutdownhook cookie */
};

static	int dematch(struct device *, struct cfdata *, void *);
static	void deattach(struct device *, struct device *, void *);
static	void dewait(struct de_softc *, char *);
static	void deinit(struct de_softc *);
static	int deioctl(struct ifnet *, u_long, caddr_t);
static	void dereset(struct device *);
static	void destart(struct ifnet *);
static	void derecv(struct de_softc *);
static	void dexmit(struct de_softc *);
static	void deintr(void *);
static	int de_add_rxbuf(struct de_softc *, int);
static	void desetup(struct de_softc *sc);
static	void deshutdown(void *);

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
	int csr1, rseg, error, i;
	bus_dma_segment_t seg;
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
	dewait(sc, "reset");

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

	if ((error = bus_dmamap_create(sc->sc_dmat, sizeof(struct de_cdata),
	    1, sizeof(struct de_cdata), 0, BUS_DMA_NOWAIT,
	    &sc->sc_cmap)) != 0) {
		printf(": unable to create control data DMA map, error = %d\n",
		    error);
		goto fail_2;
	}

	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_cmap,
	    sc->sc_dedata, sizeof(struct de_cdata), NULL,
	    BUS_DMA_NOWAIT)) != 0) {
		printf(": unable to load control data DMA map, error = %d\n",
		    error);
		goto fail_3;
	}

	bzero(sc->sc_dedata, sizeof(struct de_cdata));
	sc->sc_pdedata = (struct de_cdata *)sc->sc_cmap->dm_segs[0].ds_addr;

#ifdef notdef
	/*
	 * Create the transmit descriptor DMA maps.
	 *
	 * XXX - should allocate transmit map pages when needed, not here.
	 */
	for (i = 0; i < NXMT; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
		    MCLBYTES, 0, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW,
		    &sc->sc_xmtmap[i]))) {
			printf(": unable to create tx DMA map %d, error = %d\n",
			    i, error);
			goto fail_4;
		}
	}
#endif

	/*
	 * Create receive buffer DMA maps.
	 */
	for (i = 0; i < NRCV; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
		    MCLBYTES, 0, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW,
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

	/*
	 * Tell the DEUNA about our PCB
	 */
	DE_WCSR(DE_PCSR2, LOWORD(sc->sc_pdedata));
	DE_WCSR(DE_PCSR3, HIWORD(sc->sc_pdedata));
	DE_WLOW(CMD_GETPCBB);
	dewait(sc, "pcbb");

	sc->sc_dedata->dc_pcbb.pcbb0 = FC_RDPHYAD;
	DE_WLOW(CMD_GETCMD);
	dewait(sc, "read addr ");

	bcopy((caddr_t)&sc->sc_dedata->dc_pcbb.pcbb2, myaddr, sizeof (myaddr));
	printf("\n%s: %s, hardware address %s\n", sc->sc_dev.dv_xname, c,
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
	sc->sc_sh = shutdownhook_establish(deshutdown, sc);
	return;

	/*
	 * Free any resources we've allocated during the failed attach
	 * attempt.  Do this in reverse order and fall through.
	 */
fail_6:
	for (i = 0; i < NRCV; i++) {
		if (sc->sc_rxmbuf[i] != NULL) {
			bus_dmamap_unload(sc->sc_dmat, sc->sc_rcvmap[i]);
			m_freem(sc->sc_rxmbuf[i]);
		}
	}
fail_5:
	for (i = 0; i < NRCV; i++) {
		if (sc->sc_rcvmap[i] != NULL)
			bus_dmamap_destroy(sc->sc_dmat, sc->sc_rcvmap[i]);
	}
#ifdef notdef
fail_4: 
	for (i = 0; i < NXMT; i++) {
		if (sc->sc_xmtmap[i] != NULL)
			bus_dmamap_destroy(sc->sc_dmat, sc->sc_xmtmap[i]);
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_cmap);
#endif
fail_3:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_cmap);
fail_2:
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
	DE_WCSR(DE_PCSR0, PCSR0_RSET);
	dewait(sc, "reset");
	deinit(sc);
}

/*
 * Initialization of interface; clear recorded pending
 * operations, and reinitialize UNIBUS usage.
 */
void
deinit(struct de_softc *sc)
{
	struct de_cdata *dc, *pdc;
	int s, i;

	if (sc->sc_if.if_flags & IFF_RUNNING)
		return;
	/*
	 * Tell the DEUNA about our PCB
	 */
	DE_WCSR(DE_PCSR2, LOWORD(sc->sc_pdedata));
	DE_WCSR(DE_PCSR3, HIWORD(sc->sc_pdedata));
	DE_WLOW(0);		/* reset INTE */
	DELAY(500);
	DE_WLOW(CMD_GETPCBB);
	dewait(sc, "pcbb");

	dc = sc->sc_dedata;
	pdc = sc->sc_pdedata;
	/* set the transmit and receive ring header addresses */
	dc->dc_pcbb.pcbb0 = FC_WTRING;
	dc->dc_pcbb.pcbb2 = LOWORD(&pdc->dc_udbbuf);
	dc->dc_pcbb.pcbb4 = HIWORD(&pdc->dc_udbbuf);

	dc->dc_udbbuf.b_tdrbl = LOWORD(&pdc->dc_xrent[0]);
	dc->dc_udbbuf.b_tdrbh = HIWORD(&pdc->dc_xrent[0]);
	dc->dc_udbbuf.b_telen = sizeof (struct de_ring) / sizeof(u_int16_t);
	dc->dc_udbbuf.b_trlen = NXMT;
	dc->dc_udbbuf.b_rdrbl = LOWORD(&pdc->dc_rrent[0]);
	dc->dc_udbbuf.b_rdrbh = HIWORD(&pdc->dc_rrent[0]);
	dc->dc_udbbuf.b_relen = sizeof (struct de_ring) / sizeof(u_int16_t);
	dc->dc_udbbuf.b_rrlen = NRCV;

	DE_WLOW(CMD_GETCMD);
	dewait(sc, "wtring");

	desetup(sc);

	/* Link the transmit buffers to the descriptors */
	for (i = 0; i < NXMT; i++) {
		dc->dc_xrent[i].r_flags = 0;
		dc->dc_xrent[i].r_segbl = LOWORD(&pdc->dc_xbuf[i][0]);
		dc->dc_xrent[i].r_segbh = HIWORD(&pdc->dc_xbuf[i][0]);
	}

	for (i = 0; i < NRCV; i++)
		dc->dc_rrent[i].r_flags = RFLG_OWN;
	sc->sc_nexttx = sc->sc_inq = sc->sc_lastack = sc->sc_nextrx = 0;

	/* start up the board (rah rah) */
	s = splnet();
	sc->sc_if.if_flags |= IFF_RUNNING;
	DE_WLOW(PCSR0_INTE);		/* Change to interrupts */
	DELAY(500);
	DE_WLOW(CMD_START|PCSR0_INTE);
	dewait(sc, "start");
	DE_WLOW(CMD_PDMD|PCSR0_INTE);
	dewait(sc, "initpoll");
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
	struct de_cdata *dc;
	struct mbuf *m;
	int idx, s, running;

	/*
	 * the following test is necessary, since
	 * the code is not reentrant and we have
	 * multiple transmission buffers.
	 */
	if (ifp->if_flags & IFF_OACTIVE) /* Too much to do already */
		return;

	if (ifp->if_snd.ifq_head == 0)	 /* Nothing to do at all */
		return;

	s = splimp();
	dc = sc->sc_dedata;
	running = (sc->sc_inq != 0);
	while (sc->sc_inq < (NXMT - 1)) {

		idx = sc->sc_nexttx;
		IF_DEQUEUE(&ifp->if_snd, m);
		if (m == 0)
			goto out;

#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif
		m_copydata(m, 0, m->m_pkthdr.len, &dc->dc_xbuf[idx][0]);
		dc->dc_xrent[idx].r_slen = m->m_pkthdr.len;
		dc->dc_xrent[idx].r_tdrerr = 0;
		dc->dc_xrent[idx].r_flags = XFLG_STP|XFLG_ENP|XFLG_OWN;
		m_freem(m);

		sc->sc_inq++;
		if (++sc->sc_nexttx == NXMT)
			sc->sc_nexttx = 0;
		ifp->if_timer = 5; /* If transmit logic dies */
	}
	if (sc->sc_inq == (NXMT - 1))
		ifp->if_flags |= IFF_OACTIVE;

out:	if (running == 0) {
		DE_WLOW(PCSR0_INTE|CMD_PDMD);
		dewait(sc, "poll");
	}

	splx(s);
}

/*
 * Command done interrupt.
 */
void
deintr(void *arg)
{
	struct de_softc *sc = arg;
	short csr0, csr1;

	/* save flags right away - clear out interrupt bits */
	csr0 = DE_RCSR(DE_PCSR0);
	csr1 = DE_RCSR(DE_PCSR1);
	DE_WHIGH(csr0 >> 8);

	if (csr0 & PCSR0_RXI)
		derecv(sc);

	if (csr0 & PCSR0_TXI)
		dexmit(sc);

	/* Should never end up here */
	if (csr0 & PCSR0_PCEI) {
		printf("%s: Port command error interrupt\n",
		    sc->sc_dev.dv_xname);
	}

	if (csr0 & PCSR0_SERI) {
		printf("%s: Status error interrupt\n", sc->sc_dev.dv_xname);
	}

	if (csr0 & PCSR0_RCBI) {
		printf("%s: Receive buffer unavail interrupt\n", 
		    sc->sc_dev.dv_xname);
		DE_WLOW(PCSR0_INTE|CMD_PDMD);
		dewait(sc, "repoll");
	}
	destart(&sc->sc_if);
}

void
dexmit(struct de_softc *sc)
{
	struct ifnet *ifp = &sc->sc_if;
	struct de_ring *rp;

	/*
	 * Poll transmit ring and check status.
	 * Then free buffer space and check for
	 * more transmit requests.
	 */
	rp = &sc->sc_dedata->dc_xrent[sc->sc_lastack];
	while ((rp->r_flags & XFLG_OWN) == 0) {
		int idx = sc->sc_lastack;

		if (idx == sc->sc_nexttx)
			break;
		if (rp->r_flags & XFLG_ENP)
			ifp->if_opackets++;
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
	struct de_ring *rp;
	struct mbuf *m;
	int len;

	rp = &sc->sc_dedata->dc_rrent[sc->sc_nextrx];
	while ((rp->r_flags & RFLG_OWN) == 0) {
		ifp->if_ipackets++;
		/* check for errors */
		if ((rp->r_flags & (RFLG_ERRS|RFLG_FRAM|RFLG_OFLO|RFLG_CRC)) ||
		    (rp->r_flags&(RFLG_STP|RFLG_ENP)) != (RFLG_STP|RFLG_ENP) ||
		    (rp->r_lenerr & (RERR_BUFL|RERR_UBTO))) {
			ifp->if_ierrors++;
			goto next;
		}
		m = sc->sc_rxmbuf[sc->sc_nextrx];
		len = (rp->r_lenerr&RERR_MLEN) - ETHER_CRC_LEN;
		de_add_rxbuf(sc, sc->sc_nextrx);
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = len;

#if NBPFILTER > 0
		if (ifp->if_bpf) {
			struct ether_header *eh;

			eh = mtod(m, struct ether_header *);
			bpf_mtap(ifp->if_bpf, m);
			if ((ifp->if_flags & IFF_PROMISC) != 0 &&
			    bcmp(LLADDR(ifp->if_sadl), eh->ether_dhost,
			    ETHER_ADDR_LEN) != 0 &&
			    (ETHER_IS_MULTICAST(eh->ether_dhost) == 0)) {
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
	struct ifreq *ifr = (struct ifreq *)data;
	struct de_softc *sc = ifp->if_softc;
	int s = splnet(), error = 0;

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			deinit(sc);
			arp_ifinit(ifp, ifa);
			break;
#endif
		}
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running,
			 * stop it.
			 */
			ifp->if_flags &= ~(IFF_RUNNING|IFF_OACTIVE);
			DE_WCSR(DE_PCSR0, PCSR0_RSET);
			dewait(sc, "down");
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
			   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface it marked up and it is stopped, then
			 * start it.
			 */
			deinit(sc);
		} else if ((ifp->if_flags & IFF_UP) != 0) {
			/*
			 * Send a new setup packet to match any new changes.
			 * (Like IFF_PROMISC etc)
			 */
			desetup(sc);
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		/*
		 * Update our multicast list.
		 */
		error = (cmd == SIOCADDMULTI) ?
			ether_addmulti(ifr, &sc->sc_ec):
			ether_delmulti(ifr, &sc->sc_ec);

		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			desetup(sc);
			error = 0;
		}
		break;

	default:
		error = EINVAL;
	}
	splx(s);
	return (error);
}

/*
 * Await completion of the named function
 * and check for errors.
 */
void
dewait(struct de_softc *sc, char *fn)
{
	int csr0, s;

	s = splimp();
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
	splx(s);
}

/*
 * Changes multicast filter list/promiscous modes etc...
 */
void
desetup(struct de_softc *sc)
{
	short mode, intr;

	/*
	 * XXX - so far use ALLMULTI to receive multicast packets.
	 */
	sc->sc_if.if_flags &= ~IFF_ALLMULTI;
	if (sc->sc_ec.ec_multiaddrs.lh_first)
		sc->sc_if.if_flags |= IFF_ALLMULTI;

	mode = MOD_TPAD|MOD_HDX|MOD_DRDC;
	if (sc->sc_if.if_flags & IFF_PROMISC)
		mode |= MOD_PROM;
	else if (sc->sc_if.if_flags & IFF_ALLMULTI)
		mode |= MOD_ENAL;

	sc->sc_dedata->dc_pcbb.pcbb0 = FC_WTMODE;
	sc->sc_dedata->dc_pcbb.pcbb2 = mode;
	intr = DE_RCSR(DE_PCSR0) & PCSR0_INTE;
	DE_WLOW(CMD_GETCMD | intr);
	dewait(sc, "wtmode");
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

void
deshutdown(void *arg)
{
	struct de_softc *sc = arg;

	DE_WCSR(DE_PCSR0, PCSR0_RSET);
	dewait(sc, "shutdown");
}

