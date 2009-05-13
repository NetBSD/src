/*	$NetBSD: if_xe.c,v 1.17.92.1 2009/05/13 17:18:11 jym Exp $	*/
/*
 * Copyright (c) 1998 Darrin B. Jewell
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
 *      This product includes software developed by Darrin B. Jewell
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_xe.c,v 1.17.92.1 2009/05/13 17:18:11 jym Exp $");

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <next68k/next68k/isr.h>

#include <next68k/dev/mb8795reg.h>
#include <next68k/dev/mb8795var.h>

#include <next68k/dev/bmapreg.h>
#include <next68k/dev/intiovar.h>
#include <next68k/dev/nextdmareg.h>
#include <next68k/dev/nextdmavar.h>

#include <next68k/dev/if_xevar.h>
#include <next68k/dev/if_xereg.h>

#ifdef DEBUG
#define XE_DEBUG
#endif

#ifdef XE_DEBUG
int xe_debug = 0;
#define DPRINTF(x) if (xe_debug) printf x;
extern char *ndtracep;
extern char ndtrace[];
extern int ndtraceshow;
#define NDTRACEIF(x) if (10 && ndtracep < (ndtrace + 8192)) do {x;} while (0)
#else
#define DPRINTF(x)
#define NDTRACEIF(x)
#endif
#define PRINTF(x) printf x;

extern int turbo;

int	xe_match(struct device *, struct cfdata *, void *);
void	xe_attach(struct device *, struct device *, void *);
int	xe_tint(void *);
int	xe_rint(void *);

struct mbuf * xe_dma_rxmap_load(struct mb8795_softc *, bus_dmamap_t);

bus_dmamap_t xe_dma_rx_continue(void *);
void xe_dma_rx_completed(bus_dmamap_t, void *);
bus_dmamap_t xe_dma_tx_continue(void *);
void xe_dma_tx_completed(bus_dmamap_t, void *);
void xe_dma_rx_shutdown(void *);
void xe_dma_tx_shutdown(void *);

static void	findchannel_defer(struct device *);

CFATTACH_DECL(xe, sizeof(struct xe_softc),
    xe_match, xe_attach, NULL, NULL);

static int xe_dma_medias[] = {
	IFM_ETHER|IFM_AUTO,
	IFM_ETHER|IFM_10_T,
	IFM_ETHER|IFM_10_2,
};
static int nxe_dma_medias = (sizeof(xe_dma_medias)/sizeof(xe_dma_medias[0]));

static int attached = 0;

/*
 * Functions and the switch for the MI code.
 */
u_char		xe_read_reg(struct mb8795_softc *, int);
void		xe_write_reg(struct mb8795_softc *, int, u_char);
void		xe_dma_reset(struct mb8795_softc *);
void		xe_dma_rx_setup(struct mb8795_softc *);
void		xe_dma_rx_go(struct mb8795_softc *);
struct mbuf *	xe_dma_rx_mbuf(struct mb8795_softc *);
void		xe_dma_tx_setup(struct mb8795_softc *);
void		xe_dma_tx_go(struct mb8795_softc *);
int		xe_dma_tx_mbuf(struct mb8795_softc *, struct mbuf *);
int		xe_dma_tx_isactive(struct mb8795_softc *);

struct mb8795_glue xe_glue = {
	xe_read_reg,
	xe_write_reg,
	xe_dma_reset,
	xe_dma_rx_setup,
	xe_dma_rx_go,
	xe_dma_rx_mbuf,
	xe_dma_tx_setup,
	xe_dma_tx_go,
	xe_dma_tx_mbuf,
	xe_dma_tx_isactive,
};

int
xe_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct intio_attach_args *ia = (struct intio_attach_args *)aux;

	if (attached)
		return (0);

	ia->ia_addr = (void *)NEXT_P_ENET;

	return (1);
}

static void
findchannel_defer(struct device *self)
{
	struct xe_softc *xsc = (struct xe_softc *)self;
	struct mb8795_softc *sc = &xsc->sc_mb8795;
	int i, error;

	if (!xsc->sc_txdma) {
		xsc->sc_txdma = nextdma_findchannel ("enetx");
		if (xsc->sc_txdma == NULL)
			panic ("%s: can't find enetx DMA channel",
			       sc->sc_dev.dv_xname);
	}
	if (!xsc->sc_rxdma) {
		xsc->sc_rxdma = nextdma_findchannel ("enetr");
		if (xsc->sc_rxdma == NULL)
			panic ("%s: can't find enetr DMA channel",
			       sc->sc_dev.dv_xname);
	}
	printf ("%s: using DMA channels %s %s\n", sc->sc_dev.dv_xname,
		xsc->sc_txdma->sc_dev.dv_xname, xsc->sc_rxdma->sc_dev.dv_xname);

	nextdma_setconf (xsc->sc_rxdma, continue_cb, xe_dma_rx_continue);
	nextdma_setconf (xsc->sc_rxdma, completed_cb, xe_dma_rx_completed);
	nextdma_setconf (xsc->sc_rxdma, shutdown_cb, xe_dma_rx_shutdown);
	nextdma_setconf (xsc->sc_rxdma, cb_arg, sc);

	nextdma_setconf (xsc->sc_txdma, continue_cb, xe_dma_tx_continue);
	nextdma_setconf (xsc->sc_txdma, completed_cb, xe_dma_tx_completed);
	nextdma_setconf (xsc->sc_txdma, shutdown_cb, xe_dma_tx_shutdown);
	nextdma_setconf (xsc->sc_txdma, cb_arg, sc);

	/* Initialize the DMA maps */
	error = bus_dmamap_create(xsc->sc_txdma->sc_dmat, MCLBYTES,
				  (MCLBYTES/MSIZE), MCLBYTES, 0, BUS_DMA_ALLOCNOW,
				  &xsc->sc_tx_dmamap);
	if (error) {
		panic("%s: can't create tx DMA map, error = %d",
		      sc->sc_dev.dv_xname, error);
	}

	for(i = 0; i < MB8795_NRXBUFS; i++) {
		error = bus_dmamap_create(xsc->sc_rxdma->sc_dmat, MCLBYTES,
					  (MCLBYTES/MSIZE), MCLBYTES, 0, BUS_DMA_ALLOCNOW,
					  &xsc->sc_rx_dmamap[i]);
		if (error) {
			panic("%s: can't create rx DMA map, error = %d",
			      sc->sc_dev.dv_xname, error);
		}
		xsc->sc_rx_mb_head[i] = NULL;
	}
	xsc->sc_rx_loaded_idx = 0;
	xsc->sc_rx_completed_idx = 0;
	xsc->sc_rx_handled_idx = 0;

	/* @@@ more next hacks 
	 * the  2000 covers at least a 1500 mtu + headers
	 * + DMA_BEGINALIGNMENT+ DMA_ENDALIGNMENT
	 */
	xsc->sc_txbuf = malloc(2000, M_DEVBUF, M_NOWAIT);
	if (!xsc->sc_txbuf)
		panic("%s: can't malloc tx DMA buffer", sc->sc_dev.dv_xname);
	
	xsc->sc_tx_mb_head = NULL;
	xsc->sc_tx_loaded = 0;
	
	mb8795_config(sc, xe_dma_medias, nxe_dma_medias, xe_dma_medias[0]);
	
	isrlink_autovec(xe_tint, sc, NEXT_I_IPL(NEXT_I_ENETX), 1, NULL);
	INTR_ENABLE(NEXT_I_ENETX);
	isrlink_autovec(xe_rint, sc, NEXT_I_IPL(NEXT_I_ENETR), 1, NULL);
	INTR_ENABLE(NEXT_I_ENETR);
}

void
xe_attach(struct device *parent, struct device *self, void *aux)
{
	struct intio_attach_args *ia = (struct intio_attach_args *)aux;
	struct xe_softc *xsc = (struct xe_softc *)self;
	struct mb8795_softc *sc = &xsc->sc_mb8795;

	DPRINTF(("%s: xe_attach()\n",sc->sc_dev.dv_xname));

	{
		extern u_char rom_enetaddr[6];     /* kludge from machdep.c:next68k_bootargs() */
		int i;
		for(i=0;i<6;i++) {
			sc->sc_enaddr[i] = rom_enetaddr[i];
		}
	}

	printf("\n%s: MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
	       sc->sc_dev.dv_xname,
	       sc->sc_enaddr[0],sc->sc_enaddr[1],sc->sc_enaddr[2],
	       sc->sc_enaddr[3],sc->sc_enaddr[4],sc->sc_enaddr[5]);

	xsc->sc_bst = ia->ia_bst;
	if (bus_space_map(xsc->sc_bst, NEXT_P_ENET,
			  XE_DEVICE_SIZE, 0, &xsc->sc_bsh)) {
		panic("\n%s: can't map mb8795 registers",
		      sc->sc_dev.dv_xname);
	}

	sc->sc_bmap_bst = ia->ia_bst;
	if (bus_space_map(sc->sc_bmap_bst, NEXT_P_BMAP,
			  BMAP_SIZE, 0, &sc->sc_bmap_bsh)) {
		panic("\n%s: can't map bmap registers",
		      sc->sc_dev.dv_xname);
	}

	/*
	 * Set up glue for MI code.
	 */
	sc->sc_glue = &xe_glue;

	xsc->sc_txdma = nextdma_findchannel ("enetx");
	xsc->sc_rxdma = nextdma_findchannel ("enetr");
	if (xsc->sc_rxdma && xsc->sc_txdma) {
		findchannel_defer (self);
	} else {
		config_defer (self, findchannel_defer);
	}

	attached = 1;
}

int
xe_tint(void *arg)
{
	if (!INTR_OCCURRED(NEXT_I_ENETX))
		return 0;
	mb8795_tint((struct mb8795_softc *)arg);
	return(1);
}

int
xe_rint(void *arg)
{
	if (!INTR_OCCURRED(NEXT_I_ENETR))
		return(0);
	mb8795_rint((struct mb8795_softc *)arg);
	return(1);
}

/*
 * Glue functions.
 */

u_char
xe_read_reg(struct mb8795_softc *sc, int reg)
{
	struct xe_softc *xsc = (struct xe_softc *)sc;

	return(bus_space_read_1(xsc->sc_bst, xsc->sc_bsh, reg));
}

void
xe_write_reg(struct mb8795_softc *sc, int reg, u_char val)
{
	struct xe_softc *xsc = (struct xe_softc *)sc;

	bus_space_write_1(xsc->sc_bst, xsc->sc_bsh, reg, val);
}

void
xe_dma_reset(struct mb8795_softc *sc)
{
	struct xe_softc *xsc = (struct xe_softc *)sc;
	int i;

	DPRINTF(("xe DMA reset\n"));
	
	nextdma_reset(xsc->sc_rxdma);
	nextdma_reset(xsc->sc_txdma);

	if (xsc->sc_tx_loaded) {
		bus_dmamap_sync(xsc->sc_txdma->sc_dmat, xsc->sc_tx_dmamap,
				0, xsc->sc_tx_dmamap->dm_mapsize,
				BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(xsc->sc_txdma->sc_dmat, xsc->sc_tx_dmamap);
		xsc->sc_tx_loaded = 0;
	}
	if (xsc->sc_tx_mb_head) {
		m_freem(xsc->sc_tx_mb_head);
		xsc->sc_tx_mb_head = NULL;
	}

	for(i = 0; i < MB8795_NRXBUFS; i++) {
		if (xsc->sc_rx_mb_head[i]) {
			bus_dmamap_unload(xsc->sc_rxdma->sc_dmat, xsc->sc_rx_dmamap[i]);
			m_freem(xsc->sc_rx_mb_head[i]);
			xsc->sc_rx_mb_head[i] = NULL;
		}
	}
}

void
xe_dma_rx_setup(struct mb8795_softc *sc)
{
	struct xe_softc *xsc = (struct xe_softc *)sc;
	int i;

	DPRINTF(("xe DMA rx setup\n"));

	for(i = 0; i < MB8795_NRXBUFS; i++) {
		xsc->sc_rx_mb_head[i] = 
			xe_dma_rxmap_load(sc, xsc->sc_rx_dmamap[i]);
	}
	xsc->sc_rx_loaded_idx = 0;
	xsc->sc_rx_completed_idx = 0;
	xsc->sc_rx_handled_idx = 0;

	nextdma_init(xsc->sc_rxdma);
}

void
xe_dma_rx_go(struct mb8795_softc *sc)
{
	struct xe_softc *xsc = (struct xe_softc *)sc;

	DPRINTF(("xe DMA rx go\n"));

	nextdma_start(xsc->sc_rxdma, DMACSR_SETREAD);
}

struct mbuf *
xe_dma_rx_mbuf(struct mb8795_softc *sc)
{
	struct xe_softc *xsc = (struct xe_softc *)sc;
	bus_dmamap_t map;
	struct mbuf *m;

	m = NULL;
	if (xsc->sc_rx_handled_idx != xsc->sc_rx_completed_idx) {
		xsc->sc_rx_handled_idx++;
		xsc->sc_rx_handled_idx %= MB8795_NRXBUFS;

		map = xsc->sc_rx_dmamap[xsc->sc_rx_handled_idx];
		m = xsc->sc_rx_mb_head[xsc->sc_rx_handled_idx];
		
		m->m_len = map->dm_xfer_len;

		bus_dmamap_sync(xsc->sc_rxdma->sc_dmat, map,
				0, map->dm_mapsize, BUS_DMASYNC_POSTREAD);
		
		bus_dmamap_unload(xsc->sc_rxdma->sc_dmat, map);
		
		/* Install a fresh mbuf for next packet */
		
		xsc->sc_rx_mb_head[xsc->sc_rx_handled_idx] = 
			xe_dma_rxmap_load(sc,map);

		/* Punt runt packets
		 * DMA restarts create 0 length packets for example
		 */
		if (m->m_len < ETHER_MIN_LEN) {
			m_freem(m);
			m = NULL;
		}
	}
	return (m);
}

void
xe_dma_tx_setup(struct mb8795_softc *sc)
{
	struct xe_softc *xsc = (struct xe_softc *)sc;

	DPRINTF(("xe DMA tx setup\n"));

	nextdma_init(xsc->sc_txdma);
}

void
xe_dma_tx_go(struct mb8795_softc *sc)
{
	struct xe_softc *xsc = (struct xe_softc *)sc;

	DPRINTF(("xe DMA tx go\n"));

	nextdma_start(xsc->sc_txdma, DMACSR_SETWRITE);
}

int
xe_dma_tx_mbuf(struct mb8795_softc *sc, struct mbuf *m)
{
	struct xe_softc *xsc = (struct xe_softc *)sc;
	int error;

	xsc->sc_tx_mb_head = m;

/* The following is a next specific hack that should
 * probably be moved out of MI code.
 * This macro assumes it can move forward as needed
 * in the buffer.  Perhaps it should zero the extra buffer.
 */
#define REALIGN_DMABUF(s,l) \
	{ (s) = ((u_char *)(((unsigned)(s)+DMA_BEGINALIGNMENT-1) \
			&~(DMA_BEGINALIGNMENT-1))); \
    (l) = ((u_char *)(((unsigned)((s)+(l))+DMA_ENDALIGNMENT-1) \
				&~(DMA_ENDALIGNMENT-1)))-(s);}

#if 0
	error = bus_dmamap_load_mbuf(xsc->sc_txdma->sc_dmat,
				     xsc->sc_tx_dmamap, xsc->sc_tx_mb_head, BUS_DMA_NOWAIT);
#else
	{
		u_char *buf = xsc->sc_txbuf;
		int buflen = 0;

		buflen = m->m_pkthdr.len;

		{
			u_char *p = buf;
			for (m=xsc->sc_tx_mb_head; m; m = m->m_next) {
				if (m->m_len == 0) continue;
				memcpy( p, mtod(m, u_char *), m->m_len);
				p += m->m_len;
			}
			/* Fix runt packets */
			if (buflen < ETHER_MIN_LEN - ETHER_CRC_LEN) {
				memset(p, 0,
				    ETHER_MIN_LEN - ETHER_CRC_LEN - buflen);
				buflen = ETHER_MIN_LEN - ETHER_CRC_LEN;
			}
		}
		
		error = bus_dmamap_load(xsc->sc_txdma->sc_dmat, xsc->sc_tx_dmamap,
					buf,buflen,NULL,BUS_DMA_NOWAIT);
	}
#endif
	if (error) {
		printf("%s: can't load mbuf chain, error = %d\n",
		       sc->sc_dev.dv_xname, error);
		m_freem(xsc->sc_tx_mb_head);
		xsc->sc_tx_mb_head = NULL;
		return (error);
	}

#ifdef DIAGNOSTIC
	if (xsc->sc_tx_loaded != 0) {
		panic("%s: xsc->sc_tx_loaded is %d",sc->sc_dev.dv_xname,
		      xsc->sc_tx_loaded);
	}
#endif

	bus_dmamap_sync(xsc->sc_txdma->sc_dmat, xsc->sc_tx_dmamap, 0,
			xsc->sc_tx_dmamap->dm_mapsize, BUS_DMASYNC_PREWRITE);

	return (0);
}

int
xe_dma_tx_isactive(struct mb8795_softc *sc)
{
	struct xe_softc *xsc = (struct xe_softc *)sc;

	return (xsc->sc_tx_loaded != 0);
}

/****************************************************************/

void 
xe_dma_tx_completed(bus_dmamap_t map, void *arg)
{
#if defined (XE_DEBUG) || defined (DIAGNOSTIC)
	struct mb8795_softc *sc = arg;
#endif
#ifdef DIAGNOSTIC
	struct xe_softc *xsc = (struct xe_softc *)sc;
#endif

	DPRINTF(("%s: xe_dma_tx_completed()\n",sc->sc_dev.dv_xname));

#ifdef DIAGNOSTIC
	if (!xsc->sc_tx_loaded) {
		panic("%s: tx completed never loaded",sc->sc_dev.dv_xname);
	}
	if (map != xsc->sc_tx_dmamap) {
		panic("%s: unexpected tx completed map",sc->sc_dev.dv_xname);
	}

#endif
}

void 
xe_dma_tx_shutdown(void *arg)
{
	struct mb8795_softc *sc = arg;
	struct xe_softc *xsc = (struct xe_softc *)sc;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	DPRINTF(("%s: xe_dma_tx_shutdown()\n",sc->sc_dev.dv_xname));

#ifdef DIAGNOSTIC
	if (!xsc->sc_tx_loaded) {
		panic("%s: tx shutdown never loaded",sc->sc_dev.dv_xname);
	}
#endif

	if (turbo)
		MB_WRITE_REG(sc, MB8795_TXMODE, MB8795_TXMODE_TURBO1);
	if (xsc->sc_tx_loaded) {
		bus_dmamap_sync(xsc->sc_txdma->sc_dmat, xsc->sc_tx_dmamap,
				0, xsc->sc_tx_dmamap->dm_mapsize,
				BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(xsc->sc_txdma->sc_dmat, xsc->sc_tx_dmamap);
		m_freem(xsc->sc_tx_mb_head);
		xsc->sc_tx_mb_head = NULL;
		
		xsc->sc_tx_loaded--;
	}

#ifdef DIAGNOSTIC
	if (xsc->sc_tx_loaded != 0) {
		panic("%s: sc->sc_tx_loaded is %d",sc->sc_dev.dv_xname,
		      xsc->sc_tx_loaded);
	}
#endif

	ifp->if_timer = 0;

#if 1
	if ((ifp->if_flags & IFF_RUNNING) && !IF_IS_EMPTY(&sc->sc_tx_snd)) {
		void mb8795_start_dma(struct mb8795_softc *); /* XXXX */
		mb8795_start_dma(sc);
	}
#endif

#if 0
	/* Enable ready interrupt */
	MB_WRITE_REG(sc, MB8795_TXMASK, 
		     MB_READ_REG(sc, MB8795_TXMASK)
		     | MB8795_TXMASK_TXRXIE/* READYIE */);
#endif
}


void 
xe_dma_rx_completed(bus_dmamap_t map, void *arg)
{
	struct mb8795_softc *sc = arg;
	struct xe_softc *xsc = (struct xe_softc *)sc;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	if (ifp->if_flags & IFF_RUNNING) {
		xsc->sc_rx_completed_idx++;
		xsc->sc_rx_completed_idx %= MB8795_NRXBUFS;
		
		DPRINTF(("%s: xe_dma_rx_completed(), sc->sc_rx_completed_idx = %d\n",
			 sc->sc_dev.dv_xname, xsc->sc_rx_completed_idx));
		
#if (defined(DIAGNOSTIC))
		if (map != xsc->sc_rx_dmamap[xsc->sc_rx_completed_idx]) {
			panic("%s: Unexpected rx dmamap completed",
			      sc->sc_dev.dv_xname);
		}
#endif
	}
#ifdef DIAGNOSTIC
	else
		DPRINTF(("%s: Unexpected rx dmamap completed while if not running\n",
			 sc->sc_dev.dv_xname));
#endif
}

void 
xe_dma_rx_shutdown(void *arg)
{
	struct mb8795_softc *sc = arg;
	struct xe_softc *xsc = (struct xe_softc *)sc;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	if (ifp->if_flags & IFF_RUNNING) {
		DPRINTF(("%s: xe_dma_rx_shutdown(), restarting.\n",
			 sc->sc_dev.dv_xname));
		
		nextdma_start(xsc->sc_rxdma, DMACSR_SETREAD);
		if (turbo)
			MB_WRITE_REG(sc, MB8795_RXMODE, MB8795_RXMODE_TEST | MB8795_RXMODE_MULTICAST);
	}
#ifdef DIAGNOSTIC
	else
		DPRINTF(("%s: Unexpected rx DMA shutdown while if not running\n",
			 sc->sc_dev.dv_xname));
#endif
}

/*
 * load a dmamap with a freshly allocated mbuf
 */
struct mbuf *
xe_dma_rxmap_load(struct mb8795_softc *sc, bus_dmamap_t map)
{
	struct xe_softc *xsc = (struct xe_softc *)sc;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m;
	int error;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			m = NULL;
		} else {
			m->m_len = MCLBYTES;
		}
	}
	if (!m) {
		/* @@@ Handle this gracefully by reusing a scratch buffer
		 * or something.
		 */
		panic("Unable to get memory for incoming ethernet");
	}

	/* Align buffer, @@@ next specific.
	 * perhaps should be using M_ALIGN here instead? 
	 * First we give us a little room to align with.
	 */
	{
		u_char *buf = m->m_data;
		int buflen = m->m_len;
		buflen -= DMA_ENDALIGNMENT+DMA_BEGINALIGNMENT;
		REALIGN_DMABUF(buf, buflen);
		m->m_data = buf;
		m->m_len = buflen;
	}

	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = m->m_len;

	error = bus_dmamap_load_mbuf(xsc->sc_rxdma->sc_dmat,
			map, m, BUS_DMA_NOWAIT);

	bus_dmamap_sync(xsc->sc_rxdma->sc_dmat, map, 0,
			map->dm_mapsize, BUS_DMASYNC_PREREAD);
	
	if (error) {
		DPRINTF(("DEBUG: m->m_data = %p, m->m_len = %d\n",
				m->m_data, m->m_len));
		DPRINTF(("DEBUG: MCLBYTES = %d, map->_dm_size = %ld\n",
				MCLBYTES, map->_dm_size));

		panic("%s: can't load rx mbuf chain, error = %d",
				sc->sc_dev.dv_xname, error);
		m_freem(m);
		m = NULL;
	}

	return(m);
}

bus_dmamap_t 
xe_dma_rx_continue(void *arg)
{
	struct mb8795_softc *sc = arg;
	struct xe_softc *xsc = (struct xe_softc *)sc;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	bus_dmamap_t map = NULL;

	if (ifp->if_flags & IFF_RUNNING) {
		if (((xsc->sc_rx_loaded_idx+1)%MB8795_NRXBUFS) == xsc->sc_rx_handled_idx) {
			/* make space for one packet by dropping one */
			struct mbuf *m;
			m = xe_dma_rx_mbuf (sc);
			if (m)
				m_freem(m);
#if (defined(DIAGNOSTIC))
			DPRINTF(("%s: out of receive DMA buffers\n",sc->sc_dev.dv_xname));
#endif
		}
		xsc->sc_rx_loaded_idx++;
		xsc->sc_rx_loaded_idx %= MB8795_NRXBUFS;
		map = xsc->sc_rx_dmamap[xsc->sc_rx_loaded_idx];
		
		DPRINTF(("%s: xe_dma_rx_continue() xsc->sc_rx_loaded_idx = %d\nn",
			 sc->sc_dev.dv_xname,xsc->sc_rx_loaded_idx));
	}
#ifdef DIAGNOSTIC
	else
		panic("%s: Unexpected rx DMA continue while if not running",
		      sc->sc_dev.dv_xname);
#endif
	
	return(map);
}

bus_dmamap_t 
xe_dma_tx_continue(void *arg)
{
	struct mb8795_softc *sc = arg;
	struct xe_softc *xsc = (struct xe_softc *)sc;
	bus_dmamap_t map;

	DPRINTF(("%s: xe_dma_tx_continue()\n",sc->sc_dev.dv_xname));

	if (xsc->sc_tx_loaded) {
		map = NULL;
	} else {
		map = xsc->sc_tx_dmamap;
		xsc->sc_tx_loaded++;
	}

#ifdef DIAGNOSTIC
	if (xsc->sc_tx_loaded != 1) {
		panic("%s: sc->sc_tx_loaded is %d",sc->sc_dev.dv_xname,
				xsc->sc_tx_loaded);
	}
#endif

	return(map);
}
