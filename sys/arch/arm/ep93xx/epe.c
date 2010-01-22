/*	$NetBSD: epe.c,v 1.24 2010/01/22 08:56:04 martin Exp $	*/

/*
 * Copyright (c) 2004 Jesse Off
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
__KERNEL_RCSID(0, "$NetBSD: epe.c,v 1.24 2010/01/22 08:56:04 martin Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/time.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/ep93xx/epsocvar.h> 
#include <arm/ep93xx/ep93xxvar.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include <net/bpf.h>
#include <net/bpfdesc.h>

#include <arm/ep93xx/ep93xxreg.h>
#include <arm/ep93xx/epereg.h> 
#include <arm/ep93xx/epevar.h> 

#define DEFAULT_MDCDIV	32

#ifndef EPE_FAST
#define EPE_FAST
#endif

#ifndef EPE_FAST
#define EPE_READ(x) \
	bus_space_read_4(sc->sc_iot, sc->sc_ioh, (EPE_ ## x))
#define EPE_WRITE(x, y) \
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, (EPE_ ## x), (y))
#define CTRLPAGE_DMASYNC(x, y, z) \
	bus_dmamap_sync(sc->sc_dmat, sc->ctrlpage_dmamap, (x), (y), (z))
#else
#define EPE_READ(x) *(volatile u_int32_t *) \
	(EP93XX_AHB_VBASE + EP93XX_AHB_EPE + (EPE_ ## x))
#define EPE_WRITE(x, y) *(volatile u_int32_t *) \
	(EP93XX_AHB_VBASE + EP93XX_AHB_EPE + (EPE_ ## x)) = y
#define CTRLPAGE_DMASYNC(x, y, z)
#endif /* ! EPE_FAST */

static int	epe_match(struct device *, struct cfdata *, void *);
static void	epe_attach(struct device *, struct device *, void *);
static void	epe_init(struct epe_softc *);
static int      epe_intr(void* arg);
static int	epe_gctx(struct epe_softc *);
static int	epe_mediachange(struct ifnet *);
int		epe_mii_readreg (struct device *, int, int);
void		epe_mii_writereg (struct device *, int, int, int);
void		epe_statchg (struct device *);
void		epe_tick (void *);
static int	epe_ifioctl (struct ifnet *, u_long, void *);
static void	epe_ifstart (struct ifnet *);
static void	epe_ifwatchdog (struct ifnet *);
static int	epe_ifinit (struct ifnet *);
static void	epe_ifstop (struct ifnet *, int);
static void	epe_setaddr (struct ifnet *);

CFATTACH_DECL(epe, sizeof(struct epe_softc),
    epe_match, epe_attach, NULL, NULL);

static int
epe_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 2;
}

static void
epe_attach(struct device *parent, struct device *self, void *aux)
{
	struct epe_softc		*sc;
	struct epsoc_attach_args	*sa;
	prop_data_t			 enaddr;

	printf("\n");
	sc = (struct epe_softc*) self;
	sa = aux;
	sc->sc_iot = sa->sa_iot;
	sc->sc_intr = sa->sa_intr;
	sc->sc_dmat = sa->sa_dmat;

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 
		0, &sc->sc_ioh))
		panic("%s: Cannot map registers", self->dv_xname);

	/* Fetch the Ethernet address from property if set. */
	enaddr = prop_dictionary_get(device_properties(self), "mac-address");
	if (enaddr != NULL) {
		KASSERT(prop_object_type(enaddr) == PROP_TYPE_DATA);
		KASSERT(prop_data_size(enaddr) == ETHER_ADDR_LEN);
		memcpy(sc->sc_enaddr, prop_data_data_nocopy(enaddr),
		       ETHER_ADDR_LEN);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, EPE_AFP, 0);
		bus_space_write_region_1(sc->sc_iot, sc->sc_ioh, EPE_IndAd,
					 sc->sc_enaddr, ETHER_ADDR_LEN);
	}

        ep93xx_intr_establish(sc->sc_intr, IPL_NET, epe_intr, sc);
	epe_init(sc);
}

static int
epe_gctx(struct epe_softc *sc)
{
	struct ifnet * ifp = &sc->sc_ec.ec_if;
	u_int32_t *cur, ndq = 0;

	/* Handle transmit completions */
	cur = (u_int32_t *)(EPE_READ(TXStsQCurAdd) -
		sc->ctrlpage_dsaddr + (char*)sc->ctrlpage);

	if (sc->TXStsQ_cur != cur) { 
		CTRLPAGE_DMASYNC(TX_QLEN * 2 * sizeof(u_int32_t), 
			TX_QLEN * sizeof(u_int32_t), BUS_DMASYNC_PREREAD);
	} else {
		return 0;
	}

	do {
		u_int32_t tbi = *sc->TXStsQ_cur & 0x7fff;
		struct mbuf *m = sc->txq[tbi].m;

		if ((*sc->TXStsQ_cur & TXStsQ_TxWE) == 0) {
			ifp->if_oerrors++;
		}
		bus_dmamap_unload(sc->sc_dmat, sc->txq[tbi].m_dmamap);
		m_freem(m);
		do {
			sc->txq[tbi].m = NULL;
			ndq++;
			tbi = (tbi + 1) % TX_QLEN;
		} while (sc->txq[tbi].m == m);

		ifp->if_opackets++;
		sc->TXStsQ_cur++;
		if (sc->TXStsQ_cur >= sc->TXStsQ + TX_QLEN) {
			sc->TXStsQ_cur = sc->TXStsQ;
		}
	} while (sc->TXStsQ_cur != cur); 

	sc->TXDQ_avail += ndq;
	if (ifp->if_flags & IFF_OACTIVE) {
		ifp->if_flags &= ~IFF_OACTIVE;
		/* Disable end-of-tx-chain interrupt */
		EPE_WRITE(IntEn, IntEn_REOFIE);
	}
	return ndq;
}

static int
epe_intr(void *arg)
{
	struct epe_softc *sc = (struct epe_softc *)arg;
	struct ifnet * ifp = &sc->sc_ec.ec_if;
	u_int32_t ndq = 0, irq, *cur;

	irq = EPE_READ(IntStsC);
begin:
	cur = (u_int32_t *)(EPE_READ(RXStsQCurAdd) -
		sc->ctrlpage_dsaddr + (char*)sc->ctrlpage);
	CTRLPAGE_DMASYNC(TX_QLEN * 3 * sizeof(u_int32_t),
		RX_QLEN * 4 * sizeof(u_int32_t), 
		BUS_DMASYNC_PREREAD);
	while (sc->RXStsQ_cur != cur) {
		if ((sc->RXStsQ_cur[0] & (RXStsQ_RWE|RXStsQ_RFP|RXStsQ_EOB)) == 
			(RXStsQ_RWE|RXStsQ_RFP|RXStsQ_EOB)) {
			u_int32_t bi = (sc->RXStsQ_cur[1] >> 16) & 0x7fff;
			u_int32_t fl = sc->RXStsQ_cur[1] & 0xffff;
			struct mbuf *m;

			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m != NULL) MCLGET(m, M_DONTWAIT);
			if (m != NULL && (m->m_flags & M_EXT)) {
				bus_dmamap_unload(sc->sc_dmat, 
					sc->rxq[bi].m_dmamap);
				sc->rxq[bi].m->m_pkthdr.rcvif = ifp;
				sc->rxq[bi].m->m_pkthdr.len = 
					sc->rxq[bi].m->m_len = fl;
				if (ifp->if_bpf) 
					bpf_ops->bpf_mtap(ifp->if_bpf, sc->rxq[bi].m);
                                (*ifp->if_input)(ifp, sc->rxq[bi].m);
				sc->rxq[bi].m = m;
				bus_dmamap_load(sc->sc_dmat, 
					sc->rxq[bi].m_dmamap, 
					m->m_ext.ext_buf, MCLBYTES,
					NULL, BUS_DMA_NOWAIT);
				sc->RXDQ[bi * 2] = 
					sc->rxq[bi].m_dmamap->dm_segs[0].ds_addr;
			} else {
				/* Drop packets until we can get replacement
				 * empty mbufs for the RXDQ.
				 */
				if (m != NULL) {
					m_freem(m);
				}
				ifp->if_ierrors++;
			} 
		} else {
			ifp->if_ierrors++;
		}

		ndq++;

		sc->RXStsQ_cur += 2;
		if (sc->RXStsQ_cur >= sc->RXStsQ + (RX_QLEN * 2)) {
			sc->RXStsQ_cur = sc->RXStsQ;
		}
	}

	if (ndq > 0) {
		ifp->if_ipackets += ndq;
		CTRLPAGE_DMASYNC(TX_QLEN * 3 * sizeof(u_int32_t),
 			RX_QLEN * 4 * sizeof(u_int32_t), 
			BUS_DMASYNC_PREWRITE|BUS_DMASYNC_PREREAD);
		EPE_WRITE(RXStsEnq, ndq);
		EPE_WRITE(RXDEnq, ndq);
		ndq = 0;
	}

	if (epe_gctx(sc) > 0 && IFQ_IS_EMPTY(&ifp->if_snd) == 0) {
		epe_ifstart(ifp);
	} 

	irq = EPE_READ(IntStsC);
	if ((irq & (IntSts_RxSQ|IntSts_ECI)) != 0)
		goto begin;

	return (1);
}


static void
epe_init(struct epe_softc *sc)
{
	bus_dma_segment_t segs;
	char *addr;
	int rsegs, err, i;
	struct ifnet * ifp = &sc->sc_ec.ec_if;
	int mdcdiv = DEFAULT_MDCDIV;

	callout_init(&sc->epe_tick_ch, 0);

	/* Select primary Individual Address in Address Filter Pointer */
	EPE_WRITE(AFP, 0);
	/* Read ethernet MAC, should already be set by bootrom */
	bus_space_read_region_1(sc->sc_iot, sc->sc_ioh, EPE_IndAd,
		sc->sc_enaddr, ETHER_ADDR_LEN);
	printf("%s: MAC address %s\n", sc->sc_dev.dv_xname,
		ether_sprintf(sc->sc_enaddr));

	/* Soft Reset the MAC */
	EPE_WRITE(SelfCtl, SelfCtl_RESET);
	while(EPE_READ(SelfCtl) & SelfCtl_RESET);

	/* suggested magic initialization values from datasheet */
	EPE_WRITE(RXBufThrshld, 0x800040);
	EPE_WRITE(TXBufThrshld, 0x200010);
	EPE_WRITE(RXStsThrshld, 0x40002);
	EPE_WRITE(TXStsThrshld, 0x40002);
	EPE_WRITE(RXDThrshld, 0x40002);
	EPE_WRITE(TXDThrshld, 0x40002);

	/* Allocate a page of memory for descriptor and status queues */
	err = bus_dmamem_alloc(sc->sc_dmat, PAGE_SIZE, 0, PAGE_SIZE, 
		&segs, 1, &rsegs, BUS_DMA_WAITOK);
	if (err == 0) {
		err = bus_dmamem_map(sc->sc_dmat, &segs, 1, PAGE_SIZE, 
			&sc->ctrlpage, (BUS_DMA_WAITOK|BUS_DMA_COHERENT));
	}
	if (err == 0) {
		err = bus_dmamap_create(sc->sc_dmat, PAGE_SIZE, 1, PAGE_SIZE,
			0, BUS_DMA_WAITOK, &sc->ctrlpage_dmamap);
	}
	if (err == 0) {
		err = bus_dmamap_load(sc->sc_dmat, sc->ctrlpage_dmamap,
			sc->ctrlpage, PAGE_SIZE, NULL, BUS_DMA_WAITOK);
	}
	if (err != 0) {
		panic("%s: Cannot get DMA memory", sc->sc_dev.dv_xname);
	}
	sc->ctrlpage_dsaddr = sc->ctrlpage_dmamap->dm_segs[0].ds_addr;
	memset(sc->ctrlpage, 0, PAGE_SIZE);
	
	/* Set up pointers to start of each queue in kernel addr space.
	 * Each descriptor queue or status queue entry uses 2 words
	 */
	sc->TXDQ = (u_int32_t *)sc->ctrlpage;
	sc->TXDQ_cur = sc->TXDQ;
	sc->TXDQ_avail = TX_QLEN - 1;
	sc->TXStsQ = &sc->TXDQ[TX_QLEN * 2];
	sc->TXStsQ_cur = sc->TXStsQ;
	sc->RXDQ = &sc->TXStsQ[TX_QLEN];
	sc->RXStsQ = &sc->RXDQ[RX_QLEN * 2];
	sc->RXStsQ_cur = sc->RXStsQ;

	/* Program each queue's start addr, cur addr, and len registers
	 * with the physical addresses. 
	 */
	addr = (char *)sc->ctrlpage_dmamap->dm_segs[0].ds_addr;
	EPE_WRITE(TXDQBAdd, (u_int32_t)addr);
	EPE_WRITE(TXDQCurAdd, (u_int32_t)addr);
	EPE_WRITE(TXDQBLen, TX_QLEN * 2 * sizeof(u_int32_t)); 

	addr += (sc->TXStsQ - sc->TXDQ) * sizeof(u_int32_t);
	EPE_WRITE(TXStsQBAdd, (u_int32_t)addr);
	EPE_WRITE(TXStsQCurAdd, (u_int32_t)addr);
	EPE_WRITE(TXStsQBLen, TX_QLEN * sizeof(u_int32_t));

	addr += (sc->RXDQ - sc->TXStsQ) * sizeof(u_int32_t);
	EPE_WRITE(RXDQBAdd, (u_int32_t)addr);
	EPE_WRITE(RXDCurAdd, (u_int32_t)addr);
	EPE_WRITE(RXDQBLen, RX_QLEN * 2 * sizeof(u_int32_t));
	
	addr += (sc->RXStsQ - sc->RXDQ) * sizeof(u_int32_t);
	EPE_WRITE(RXStsQBAdd, (u_int32_t)addr);
	EPE_WRITE(RXStsQCurAdd, (u_int32_t)addr);
	EPE_WRITE(RXStsQBLen, RX_QLEN * 2 * sizeof(u_int32_t));

	/* Populate the RXDQ with mbufs */
	for(i = 0; i < RX_QLEN; i++) {
		struct mbuf *m;

		bus_dmamap_create(sc->sc_dmat, MCLBYTES, TX_QLEN/4, MCLBYTES, 0,
			BUS_DMA_WAITOK, &sc->rxq[i].m_dmamap);
		MGETHDR(m, M_WAIT, MT_DATA);
		MCLGET(m, M_WAIT);
		sc->rxq[i].m = m;
		bus_dmamap_load(sc->sc_dmat, sc->rxq[i].m_dmamap, 
			m->m_ext.ext_buf, MCLBYTES, NULL, 
			BUS_DMA_WAITOK);

		sc->RXDQ[i * 2] = sc->rxq[i].m_dmamap->dm_segs[0].ds_addr;
		sc->RXDQ[i * 2 + 1] = (i << 16) | MCLBYTES;
		bus_dmamap_sync(sc->sc_dmat, sc->rxq[i].m_dmamap, 0,
			MCLBYTES, BUS_DMASYNC_PREREAD);
	}

	for(i = 0; i < TX_QLEN; i++) {
		bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1, MCLBYTES, 0,
			(BUS_DMA_WAITOK|BUS_DMA_ALLOCNOW),
			&sc->txq[i].m_dmamap);
		sc->txq[i].m = NULL;
		sc->TXDQ[i * 2 + 1] = (i << 16);
	}

	/* Divide HCLK by 32 for MDC clock */
	if (device_cfdata(&sc->sc_dev)->cf_flags)
		mdcdiv = device_cfdata(&sc->sc_dev)->cf_flags;
	EPE_WRITE(SelfCtl, (SelfCtl_MDCDIV(mdcdiv)|SelfCtl_PSPRS));

	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = epe_mii_readreg;
	sc->sc_mii.mii_writereg = epe_mii_writereg;
	sc->sc_mii.mii_statchg = epe_statchg;
	sc->sc_ec.ec_mii = &sc->sc_mii;
	ifmedia_init(&sc->sc_mii.mii_media, IFM_IMASK, epe_mediachange,
		ether_mediastatus);
	mii_attach((struct device *)sc, &sc->sc_mii, 0xffffffff, MII_PHY_ANY,
		MII_OFFSET_ANY, 0);
	ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);

	EPE_WRITE(BMCtl, BMCtl_RxEn|BMCtl_TxEn);
	EPE_WRITE(IntEn, IntEn_REOFIE);
	/* maximum valid max frame length */
	EPE_WRITE(MaxFrmLen, (0x7ff << 16)|MHLEN);
	/* wait for receiver ready */
	while((EPE_READ(BMSts) & BMSts_RxAct) == 0); 
	/* enqueue the entries in RXStsQ and RXDQ */
	CTRLPAGE_DMASYNC(0, sc->ctrlpage_dmamap->dm_mapsize, 
		BUS_DMASYNC_PREWRITE|BUS_DMASYNC_PREREAD);
	EPE_WRITE(RXDEnq, RX_QLEN - 1);
	EPE_WRITE(RXStsEnq, RX_QLEN - 1);

	/*
	 * We can support 802.1Q VLAN-sized frames.
	 */
	sc->sc_ec.ec_capabilities |= ETHERCAP_VLAN_MTU;

        strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
        ifp->if_flags = IFF_BROADCAST|IFF_SIMPLEX|IFF_NOTRAILERS|IFF_MULTICAST;
        ifp->if_ioctl = epe_ifioctl;
        ifp->if_start = epe_ifstart;
        ifp->if_watchdog = epe_ifwatchdog;
        ifp->if_init = epe_ifinit;
        ifp->if_stop = epe_ifstop;
        ifp->if_timer = 0;
	ifp->if_softc = sc;
        IFQ_SET_READY(&ifp->if_snd);
        if_attach(ifp);
        ether_ifattach(ifp, (sc)->sc_enaddr);
}

static int
epe_mediachange(struct ifnet *ifp)
{
	if (ifp->if_flags & IFF_UP)
		epe_ifinit(ifp);
	return (0);
}

int
epe_mii_readreg(struct device *self, int phy, int reg)
{
	u_int32_t d, v;
	struct epe_softc *sc;

	sc = (struct epe_softc *)self;
	d = EPE_READ(SelfCtl);
	EPE_WRITE(SelfCtl, d & ~SelfCtl_PSPRS); /* no preamble suppress */
	EPE_WRITE(MIICmd, (MIICmd_READ | (phy << 5) | reg));
	while(EPE_READ(MIISts) & MIISts_BUSY);
	v = EPE_READ(MIIData);
	EPE_WRITE(SelfCtl, d); /* restore old value */
	return v;
}

void
epe_mii_writereg(struct device *self, int phy, int reg, int val)
{
	struct epe_softc *sc;
	u_int32_t d;

	sc = (struct epe_softc *)self;
	d = EPE_READ(SelfCtl);
	EPE_WRITE(SelfCtl, d & ~SelfCtl_PSPRS); /* no preamble suppress */
	EPE_WRITE(MIIData, val);
	EPE_WRITE(MIICmd, (MIICmd_WRITE | (phy << 5) | reg));
	while(EPE_READ(MIISts) & MIISts_BUSY);
	EPE_WRITE(SelfCtl, d); /* restore old value */
}

	
void
epe_statchg(struct device *self)
{
        struct epe_softc *sc = (struct epe_softc *)self;
        u_int32_t reg;

        /*
         * We must keep the MAC and the PHY in sync as
         * to the status of full-duplex!
         */
        reg = EPE_READ(TestCtl);
        if (sc->sc_mii.mii_media_active & IFM_FDX)
                reg |= TestCtl_MFDX;
        else
                reg &= ~TestCtl_MFDX;
	EPE_WRITE(TestCtl, reg);
}

void
epe_tick(void *arg)
{
	struct epe_softc* sc = (struct epe_softc *)arg;
	struct ifnet * ifp = &sc->sc_ec.ec_if;
	int s;
	u_int32_t misses;

	ifp->if_collisions += EPE_READ(TXCollCnt);
	/* These misses are ok, they will happen if the RAM/CPU can't keep up */
	misses = EPE_READ(RXMissCnt);
	if (misses > 0) 
		printf("%s: %d rx misses\n", sc->sc_dev.dv_xname, misses);
	
	s = splnet();
	if (epe_gctx(sc) > 0 && IFQ_IS_EMPTY(&ifp->if_snd) == 0) {
		epe_ifstart(ifp);
	}
	splx(s);

	mii_tick(&sc->sc_mii);
	callout_reset(&sc->epe_tick_ch, hz, epe_tick, sc);
}


static int
epe_ifioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	int s, error;

	s = splnet();
	error = ether_ioctl(ifp, cmd, data);
	if (error == ENETRESET) {
		if (ifp->if_flags & IFF_RUNNING)
			epe_setaddr(ifp);
		error = 0;
	}
	splx(s);
	return error;
}

static void
epe_ifstart(struct ifnet *ifp)
{
	struct epe_softc *sc = (struct epe_softc *)ifp->if_softc;
	struct mbuf *m;
	bus_dma_segment_t *segs;
	int s, bi, err, nsegs, ndq;

	s = splnet();	
start:
	ndq = 0;
	if (sc->TXDQ_avail == 0) {
		if (epe_gctx(sc) == 0) {
			/* Enable End-Of-TX-Chain interrupt */
			EPE_WRITE(IntEn, IntEn_REOFIE|IntEn_ECIE);
			ifp->if_flags |= IFF_OACTIVE;
			ifp->if_timer = 10;
			splx(s);
			return;
		}
	} 

	bi = sc->TXDQ_cur - sc->TXDQ; 

	IFQ_POLL(&ifp->if_snd, m);
	if (m == NULL) {
		splx(s);
		return;
	}
more:
	if ((err = bus_dmamap_load_mbuf(sc->sc_dmat, sc->txq[bi].m_dmamap, m,
		BUS_DMA_NOWAIT)) || 
		sc->txq[bi].m_dmamap->dm_segs[0].ds_addr & 0x3 ||
		sc->txq[bi].m_dmamap->dm_nsegs > (sc->TXDQ_avail - ndq)) {
		/* Copy entire mbuf chain to new and 32-bit aligned storage */
		struct mbuf *mn;

		if (err == 0) 
			bus_dmamap_unload(sc->sc_dmat, sc->txq[bi].m_dmamap);

		MGETHDR(mn, M_DONTWAIT, MT_DATA);
		if (mn == NULL) goto stop;
		if (m->m_pkthdr.len > (MHLEN & (~0x3))) {
			MCLGET(mn, M_DONTWAIT);
			if ((mn->m_flags & M_EXT) == 0) {
				m_freem(mn);
				goto stop;
			}
		}
		mn->m_data = (void *)(((u_int32_t)mn->m_data + 0x3) & (~0x3)); 
		m_copydata(m, 0, m->m_pkthdr.len, mtod(mn, void *));
		mn->m_pkthdr.len = mn->m_len = m->m_pkthdr.len;
		IFQ_DEQUEUE(&ifp->if_snd, m);
		m_freem(m);
		m = mn;
		bus_dmamap_load_mbuf(sc->sc_dmat, sc->txq[bi].m_dmamap, m,
			BUS_DMA_NOWAIT);
	} else {
		IFQ_DEQUEUE(&ifp->if_snd, m);
	}

	if (ifp->if_bpf) 
		bpf_ops->bpf_mtap(ifp->if_bpf, m);

	nsegs = sc->txq[bi].m_dmamap->dm_nsegs;
	segs = sc->txq[bi].m_dmamap->dm_segs;
	bus_dmamap_sync(sc->sc_dmat, sc->txq[bi].m_dmamap, 0, 
		sc->txq[bi].m_dmamap->dm_mapsize, 
		BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	/* XXX: This driver hasn't been tested w/nsegs > 1 */
	while (nsegs > 0) {
		nsegs--;
		sc->txq[bi].m = m;
		sc->TXDQ[bi * 2] = segs->ds_addr;
		if (nsegs == 0)
			sc->TXDQ[bi * 2 + 1] = segs->ds_len | (bi << 16) |
				(1 << 31);
		else
			sc->TXDQ[bi * 2 + 1] = segs->ds_len | (bi << 16);
		segs++;
		bi = (bi + 1) % TX_QLEN;
		ndq++;
	}


	/*
	 * Enqueue another.  Don't do more than half the available
	 * descriptors before telling the MAC about them
	 */
	if ((sc->TXDQ_avail - ndq) > 0 && ndq < TX_QLEN / 2) {
		IFQ_POLL(&ifp->if_snd, m);
		if (m != NULL) {
			goto more;
		}
	} 
stop:
	if (ndq > 0) {
		sc->TXDQ_avail -= ndq;
		sc->TXDQ_cur = &sc->TXDQ[bi];
		CTRLPAGE_DMASYNC(0, TX_QLEN * 2 * sizeof(u_int32_t),
			BUS_DMASYNC_PREWRITE|BUS_DMASYNC_PREREAD);
		EPE_WRITE(TXDEnq, ndq);
	}

	if (IFQ_IS_EMPTY(&ifp->if_snd) == 0)
		goto start;

	splx(s);
	return;
}

static void
epe_ifwatchdog(struct ifnet *ifp)
{
	struct epe_softc *sc = (struct epe_softc *)ifp->if_softc;

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;
       	printf("%s: device timeout, BMCtl = 0x%08x, BMSts = 0x%08x\n", 
		sc->sc_dev.dv_xname, EPE_READ(BMCtl), EPE_READ(BMSts));
}

static int
epe_ifinit(struct ifnet *ifp)
{
	struct epe_softc *sc = ifp->if_softc;
	int rc, s = splnet();

	callout_stop(&sc->epe_tick_ch);
	EPE_WRITE(RXCtl, RXCtl_IA0|RXCtl_BA|RXCtl_RCRCA|RXCtl_SRxON);
	EPE_WRITE(TXCtl, TXCtl_STxON);
	EPE_WRITE(GIIntMsk, GIIntMsk_INT); /* start interrupting */

	if ((rc = mii_mediachg(&sc->sc_mii)) == ENXIO)
		rc = 0;
	else if (rc != 0)
		goto out;

	callout_reset(&sc->epe_tick_ch, hz, epe_tick, sc);
        ifp->if_flags |= IFF_RUNNING;
out:
	splx(s);
	return 0;
}

static void
epe_ifstop(struct ifnet *ifp, int disable)
{
	struct epe_softc *sc = ifp->if_softc;


	EPE_WRITE(RXCtl, 0);
	EPE_WRITE(TXCtl, 0);
	EPE_WRITE(GIIntMsk, 0);
	callout_stop(&sc->epe_tick_ch);

	/* Down the MII. */
	mii_down(&sc->sc_mii);

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
	sc->sc_mii.mii_media_status &= ~IFM_ACTIVE;
}

static void
epe_setaddr(struct ifnet *ifp)
{
	struct epe_softc *sc = ifp->if_softc;
	struct ethercom *ac = &sc->sc_ec;
	struct ether_multi *enm;
	struct ether_multistep step;
	u_int8_t ias[2][ETHER_ADDR_LEN];
	u_int32_t h, nma = 0, hashes[2] = { 0, 0 };
	u_int32_t rxctl = EPE_READ(RXCtl);

	/* disable receiver temporarily */
	EPE_WRITE(RXCtl, rxctl & ~RXCtl_SRxON);
	
	rxctl &= ~(RXCtl_MA|RXCtl_PA|RXCtl_IA2|RXCtl_IA3);

	if (ifp->if_flags & IFF_PROMISC) {
		rxctl |= RXCtl_PA;
	} 

	ifp->if_flags &= ~IFF_ALLMULTI;

	ETHER_FIRST_MULTI(step, ac, enm);
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
			rxctl &= ~(RXCtl_IA2|RXCtl_IA3);
			rxctl |= RXCtl_MA;
			hashes[0] = 0xffffffffUL;
			hashes[1] = 0xffffffffUL;
			ifp->if_flags |= IFF_ALLMULTI;
			break;
		}

		if (nma < 2) {
			/* We can program 2 perfect address filters for mcast */
			memcpy(ias[nma], enm->enm_addrlo, ETHER_ADDR_LEN);			
			rxctl |= (1 << (nma + 2));
		} else {
			/*
			 * XXX: Datasheet is not very clear here, I'm not sure
			 * if I'm doing this right.  --joff
			 */
			h = ether_crc32_le(enm->enm_addrlo, ETHER_ADDR_LEN);

			/* Just want the 6 most-significant bits. */
			h = h >> 26;

			hashes[ h / 32 ] |=  (1 << (h % 32));
			rxctl |= RXCtl_MA;
		}
		ETHER_NEXT_MULTI(step, enm);
		nma++;
	}
	
	EPE_WRITE(AFP, 0);
	bus_space_write_region_1(sc->sc_iot, sc->sc_ioh, EPE_IndAd, 
		sc->sc_enaddr, ETHER_ADDR_LEN);
	if (rxctl & RXCtl_IA2) {
		EPE_WRITE(AFP, 2);
		bus_space_write_region_1(sc->sc_iot, sc->sc_ioh, EPE_IndAd,
			ias[0], ETHER_ADDR_LEN);
	}
	if (rxctl & RXCtl_IA3) {
		EPE_WRITE(AFP, 3);
		bus_space_write_region_1(sc->sc_iot, sc->sc_ioh, EPE_IndAd,
			ias[1], ETHER_ADDR_LEN);
	}
	if (hashes[0] != 0 && hashes[1] != 0) {
		EPE_WRITE(AFP, 7);
		EPE_WRITE(HashTbl, hashes[0]);
		EPE_WRITE(HashTbl + 4, hashes[1]);
	}
	EPE_WRITE(RXCtl, rxctl);
}
