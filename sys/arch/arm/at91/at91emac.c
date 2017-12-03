/*	$Id: at91emac.c,v 1.11.2.2 2017/12/03 11:35:51 jdolecek Exp $	*/
/*	$NetBSD: at91emac.c,v 1.11.2.2 2017/12/03 11:35:51 jdolecek Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy
 * All rights reserved.
 *
 * Based on arch/arm/ep93xx/epe.c
 *
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
__KERNEL_RCSID(0, "$NetBSD: at91emac.c,v 1.11.2.2 2017/12/03 11:35:51 jdolecek Exp $");

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

#include <sys/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

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

#include <net/bpf.h>
#include <net/bpfdesc.h>

#ifdef IPKDB_AT91	// @@@
#include <ipkdb/ipkdb.h>
#endif

#include <arm/at91/at91var.h>
#include <arm/at91/at91emacreg.h>
#include <arm/at91/at91emacvar.h>

#define DEFAULT_MDCDIV	32

#ifndef EMAC_FAST
#define EMAC_FAST
#endif

#ifndef EMAC_FAST
#define EMAC_READ(x) \
	bus_space_read_4(sc->sc_iot, sc->sc_ioh, (EPE_ ## x))
#define EMAC_WRITE(x, y) \
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, (EPE_ ## x), (y))
#else
#define EMAC_READ(x) ETHREG(x)
#define EMAC_WRITE(x, y) ETHREG(x) = (y)
#endif /* ! EMAC_FAST */

static int	emac_match(device_t, cfdata_t, void *);
static void	emac_attach(device_t, device_t, void *);
static void	emac_init(struct emac_softc *);
static int      emac_intr(void* arg);
static int	emac_gctx(struct emac_softc *);
static int	emac_mediachange(struct ifnet *);
static void	emac_mediastatus(struct ifnet *, struct ifmediareq *);
int		emac_mii_readreg (device_t, int, int);
void		emac_mii_writereg (device_t, int, int, int);
void		emac_statchg (struct ifnet *);
void		emac_tick (void *);
static int	emac_ifioctl (struct ifnet *, u_long, void *);
static void	emac_ifstart (struct ifnet *);
static void	emac_ifwatchdog (struct ifnet *);
static int	emac_ifinit (struct ifnet *);
static void	emac_ifstop (struct ifnet *, int);
static void	emac_setaddr (struct ifnet *);

CFATTACH_DECL_NEW(at91emac, sizeof(struct emac_softc),
    emac_match, emac_attach, NULL, NULL);

#ifdef	EMAC_DEBUG
int emac_debug = EMAC_DEBUG;
#define	DPRINTFN(n,fmt)	if (emac_debug >= (n)) printf fmt
#else
#define	DPRINTFN(n,fmt)
#endif

static int
emac_match(device_t parent, cfdata_t match, void *aux)
{
	if (strcmp(match->cf_name, "at91emac") == 0)
		return 2;
	return 0;
}

static void
emac_attach(device_t parent, device_t self, void *aux)
{
	struct emac_softc		*sc = device_private(self);
	struct at91bus_attach_args	*sa = aux;
	prop_data_t			enaddr;
	uint32_t			u;

	printf("\n");
	sc->sc_dev = self;
	sc->sc_iot = sa->sa_iot;
	sc->sc_pid = sa->sa_pid;
	sc->sc_dmat = sa->sa_dmat;

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0, &sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(self));

	/* enable peripheral clock */
	at91_peripheral_clock(sc->sc_pid, 1);

	/* configure emac: */
	EMAC_WRITE(ETH_CTL, 0);			// disable everything
	EMAC_WRITE(ETH_IDR, -1);		// disable interrupts
	EMAC_WRITE(ETH_RBQP, 0);		// clear receive
	EMAC_WRITE(ETH_CFG, ETH_CFG_CLK_32 | ETH_CFG_SPD | ETH_CFG_FD | ETH_CFG_BIG);
	EMAC_WRITE(ETH_TCR, 0);			// send nothing
	//(void)EMAC_READ(ETH_ISR);
	u = EMAC_READ(ETH_TSR);
	EMAC_WRITE(ETH_TSR, (u & (ETH_TSR_UND | ETH_TSR_COMP | ETH_TSR_BNQ
				  | ETH_TSR_IDLE | ETH_TSR_RLE
				  | ETH_TSR_COL|ETH_TSR_OVR)));
	u = EMAC_READ(ETH_RSR);
	EMAC_WRITE(ETH_RSR, (u & (ETH_RSR_OVR|ETH_RSR_REC|ETH_RSR_BNA)));

	/* Fetch the Ethernet address from property if set. */
	enaddr = prop_dictionary_get(device_properties(self), "mac-address");

	if (enaddr != NULL) {
		KASSERT(prop_object_type(enaddr) == PROP_TYPE_DATA);
		KASSERT(prop_data_size(enaddr) == ETHER_ADDR_LEN);
		memcpy(sc->sc_enaddr, prop_data_data_nocopy(enaddr),
		       ETHER_ADDR_LEN);
	} else {
		static const uint8_t hardcoded[ETHER_ADDR_LEN] = {
		  0x00, 0x0d, 0x10, 0x81, 0x0c, 0x94
		};
		memcpy(sc->sc_enaddr, hardcoded, ETHER_ADDR_LEN);
	}

        at91_intr_establish(sc->sc_pid, IPL_NET, INTR_HIGH_LEVEL, emac_intr, sc);
	emac_init(sc);
}

static int
emac_gctx(struct emac_softc *sc)
{
	struct ifnet * ifp = &sc->sc_ec.ec_if;
	uint32_t tsr;

	tsr = EMAC_READ(ETH_TSR);
	if (!(tsr & ETH_TSR_BNQ)) {
		// no space left
		return 0;
	}

	// free sent frames
	while (sc->txqc > (tsr & ETH_TSR_IDLE ? 0 : 1)) {
		int i = sc->txqi % TX_QLEN;
		bus_dmamap_sync(sc->sc_dmat, sc->txq[i].m_dmamap, 0,
				sc->txq[i].m->m_pkthdr.len, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, sc->txq[i].m_dmamap);
		m_freem(sc->txq[i].m);
		DPRINTFN(2,("%s: freed idx #%i mbuf %p (txqc=%i)\n", __FUNCTION__, i, sc->txq[i].m, sc->txqc));
		sc->txq[i].m = NULL;
		sc->txqi = (i + 1) % TX_QLEN;
		sc->txqc--;
	}

	// mark we're free
	if (ifp->if_flags & IFF_OACTIVE) {
		ifp->if_flags &= ~IFF_OACTIVE;
		/* Disable transmit-buffer-free interrupt */
		/*EMAC_WRITE(ETH_IDR, ETH_ISR_TBRE);*/
	}

	return 1;
}

static int
emac_intr(void *arg)
{
	struct emac_softc *sc = (struct emac_softc *)arg;
	struct ifnet * ifp = &sc->sc_ec.ec_if;
	uint32_t imr, isr, ctl;
	int bi;

	imr = ~EMAC_READ(ETH_IMR);
	if (!(imr & (ETH_ISR_RCOM|ETH_ISR_TBRE|ETH_ISR_TIDLE|ETH_ISR_RBNA|ETH_ISR_ROVR))) {
		// interrupt not enabled, can't be us
		return 0;
	}

	isr = EMAC_READ(ETH_ISR) & imr;
#ifdef EMAC_DEBUG 
	uint32_t rsr = 
#endif
	EMAC_READ(ETH_RSR);		// get receive status register

	DPRINTFN(2, ("%s: isr=0x%08X rsr=0x%08X imr=0x%08X\n", __FUNCTION__, isr, rsr, imr));

	if (isr & ETH_ISR_RBNA) {		// out of receive buffers
		EMAC_WRITE(ETH_RSR, ETH_RSR_BNA);	// clear interrupt
		ctl = EMAC_READ(ETH_CTL);		// get current control register value
		EMAC_WRITE(ETH_CTL, ctl & ~ETH_CTL_RE);	// disable receiver
		EMAC_WRITE(ETH_RSR, ETH_RSR_BNA);	// clear BNA bit
		EMAC_WRITE(ETH_CTL, ctl |  ETH_CTL_RE);	// re-enable receiver
		ifp->if_ierrors++;
		ifp->if_ipackets++;
		DPRINTFN(1,("%s: out of receive buffers\n", __FUNCTION__));
	}
	if (isr & ETH_ISR_ROVR) {
		EMAC_WRITE(ETH_RSR, ETH_RSR_OVR);	// clear interrupt
		ifp->if_ierrors++;
		ifp->if_ipackets++;
		DPRINTFN(1,("%s: receive overrun\n", __FUNCTION__));
	}
	
	if (isr & ETH_ISR_RCOM) {			// packet has been received!
		uint32_t nfo;
		// @@@ if memory is NOT coherent, then we're in trouble @@@@
//		bus_dmamap_sync(sc->sc_dmat, sc->rbqpage_dmamap, 0, sc->rbqlen, BUS_DMASYNC_POSTWRITE | BUS_DMASYNC_POSTREAD);
//		printf("## RDSC[%i].ADDR=0x%08X\n", sc->rxqi % RX_QLEN, sc->RDSC[sc->rxqi % RX_QLEN].Addr);
		DPRINTFN(2,("#2 RDSC[%i].INFO=0x%08X\n", sc->rxqi % RX_QLEN, sc->RDSC[sc->rxqi % RX_QLEN].Info));
		while (sc->RDSC[(bi = sc->rxqi % RX_QLEN)].Addr & ETH_RDSC_F_USED) {
			int fl;
			struct mbuf *m;

			nfo = sc->RDSC[bi].Info;
		  	fl = (nfo & ETH_RDSC_I_LEN) - 4;
			DPRINTFN(2,("## nfo=0x%08X\n", nfo));

			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m != NULL) MCLGET(m, M_DONTWAIT);
			if (m != NULL && (m->m_flags & M_EXT)) {
				bus_dmamap_sync(sc->sc_dmat, sc->rxq[bi].m_dmamap, 0,
						MCLBYTES, BUS_DMASYNC_POSTREAD);
				bus_dmamap_unload(sc->sc_dmat, 
					sc->rxq[bi].m_dmamap);
				m_set_rcvif(sc->rxq[bi].m, ifp);
				sc->rxq[bi].m->m_pkthdr.len = 
					sc->rxq[bi].m->m_len = fl;
				DPRINTFN(2,("received %u bytes packet\n", fl));
				if_percpuq_enqueue(ifp->if_percpuq, sc->rxq[bi].m);
				if (mtod(m, intptr_t) & 3) {
					m_adj(m, mtod(m, intptr_t) & 3);
				}
				sc->rxq[bi].m = m;
				bus_dmamap_load(sc->sc_dmat, 
					sc->rxq[bi].m_dmamap, 
					m->m_ext.ext_buf, MCLBYTES,
					NULL, BUS_DMA_NOWAIT);
				bus_dmamap_sync(sc->sc_dmat, sc->rxq[bi].m_dmamap, 0,
						MCLBYTES, BUS_DMASYNC_PREREAD);
				sc->RDSC[bi].Info = 0;
				sc->RDSC[bi].Addr =
					sc->rxq[bi].m_dmamap->dm_segs[0].ds_addr
					| (bi == (RX_QLEN-1) ? ETH_RDSC_F_WRAP : 0);
			} else {
				/* Drop packets until we can get replacement
				 * empty mbufs for the RXDQ.
				 */
				if (m != NULL) {
					m_freem(m);
				}
				ifp->if_ierrors++;
			} 
			sc->rxqi++;
		}
//		bus_dmamap_sync(sc->sc_dmat, sc->rbqpage_dmamap, 0, sc->rbqlen, BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	}

	if (emac_gctx(sc) > 0)
		if_schedule_deferred_start(ifp);
#if 0 // reloop
	irq = EMAC_READ(IntStsC);
	if ((irq & (IntSts_RxSQ|IntSts_ECI)) != 0)
		goto begin;
#endif

	return (1);
}


static void
emac_init(struct emac_softc *sc)
{
	bus_dma_segment_t segs;
	void *addr;
	int rsegs, err, i;
	struct ifnet * ifp = &sc->sc_ec.ec_if;
	uint32_t u;
#if 0
	int mdcdiv = DEFAULT_MDCDIV;
#endif

	callout_init(&sc->emac_tick_ch, 0);

	// ok...
	EMAC_WRITE(ETH_CTL, ETH_CTL_MPE);	// disable everything
	EMAC_WRITE(ETH_IDR, -1);		// disable interrupts
	EMAC_WRITE(ETH_RBQP, 0);		// clear receive
	EMAC_WRITE(ETH_CFG, ETH_CFG_CLK_32 | ETH_CFG_SPD | ETH_CFG_FD | ETH_CFG_BIG);
	EMAC_WRITE(ETH_TCR, 0);			// send nothing
//	(void)EMAC_READ(ETH_ISR);
	u = EMAC_READ(ETH_TSR);
	EMAC_WRITE(ETH_TSR, (u & (ETH_TSR_UND | ETH_TSR_COMP | ETH_TSR_BNQ
				  | ETH_TSR_IDLE | ETH_TSR_RLE
				  | ETH_TSR_COL|ETH_TSR_OVR)));
	u = EMAC_READ(ETH_RSR);
	EMAC_WRITE(ETH_RSR, (u & (ETH_RSR_OVR|ETH_RSR_REC|ETH_RSR_BNA)));

	/* configure EMAC */
	EMAC_WRITE(ETH_CFG, ETH_CFG_CLK_32 | ETH_CFG_SPD | ETH_CFG_FD | ETH_CFG_BIG);
	EMAC_WRITE(ETH_CTL, ETH_CTL_MPE);
#if 0
	if (device_cfdata(sc->sc_dev)->cf_flags)
		mdcdiv = device_cfdata(sc->sc_dev)->cf_flags;
#endif
	/* set ethernet address */
	EMAC_WRITE(ETH_SA1L, (sc->sc_enaddr[3] << 24)
		   | (sc->sc_enaddr[2] << 16) | (sc->sc_enaddr[1] << 8)
		   | (sc->sc_enaddr[0]));
	EMAC_WRITE(ETH_SA1H, (sc->sc_enaddr[5] << 8)
		   | (sc->sc_enaddr[4]));
	EMAC_WRITE(ETH_SA2L, 0);
	EMAC_WRITE(ETH_SA2H, 0);
	EMAC_WRITE(ETH_SA3L, 0);
	EMAC_WRITE(ETH_SA3H, 0);
	EMAC_WRITE(ETH_SA4L, 0);
	EMAC_WRITE(ETH_SA4H, 0);

	/* Allocate a page of memory for receive queue descriptors */
	sc->rbqlen = (ETH_RDSC_SIZE * (RX_QLEN + 1) * 2 + PAGE_SIZE - 1) / PAGE_SIZE;
	sc->rbqlen *= PAGE_SIZE;
	DPRINTFN(1,("%s: rbqlen=%i\n", __FUNCTION__, sc->rbqlen));

	err = bus_dmamem_alloc(sc->sc_dmat, sc->rbqlen, 0,
		MAX(16384, PAGE_SIZE),	// see EMAC errata why forced to 16384 byte boundary
		&segs, 1, &rsegs, BUS_DMA_WAITOK);
	if (err == 0) {
		DPRINTFN(1,("%s: -> bus_dmamem_map\n", __FUNCTION__));
		err = bus_dmamem_map(sc->sc_dmat, &segs, 1, sc->rbqlen,
			&sc->rbqpage, (BUS_DMA_WAITOK|BUS_DMA_COHERENT));
	}
	if (err == 0) {
		DPRINTFN(1,("%s: -> bus_dmamap_create\n", __FUNCTION__));
		err = bus_dmamap_create(sc->sc_dmat, sc->rbqlen, 1,
			sc->rbqlen, MAX(16384, PAGE_SIZE), BUS_DMA_WAITOK,
			&sc->rbqpage_dmamap);
	}
	if (err == 0) {
		DPRINTFN(1,("%s: -> bus_dmamap_load\n", __FUNCTION__));
		err = bus_dmamap_load(sc->sc_dmat, sc->rbqpage_dmamap,
			sc->rbqpage, sc->rbqlen, NULL, BUS_DMA_WAITOK);
	}
	if (err != 0) {
		panic("%s: Cannot get DMA memory", device_xname(sc->sc_dev));
	}
	sc->rbqpage_dsaddr = sc->rbqpage_dmamap->dm_segs[0].ds_addr;

	memset(sc->rbqpage, 0, sc->rbqlen);

	/* Set up pointers to start of each queue in kernel addr space.
	 * Each descriptor queue or status queue entry uses 2 words
	 */
	sc->RDSC = (void*)sc->rbqpage;

	/* Populate the RXQ with mbufs */
	sc->rxqi = 0;
	for(i = 0; i < RX_QLEN; i++) {
		struct mbuf *m;

		err = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1, MCLBYTES, PAGE_SIZE,
			BUS_DMA_WAITOK, &sc->rxq[i].m_dmamap);
		if (err) {
			panic("%s: dmamap_create failed: %i\n", __FUNCTION__, err);
		}
		MGETHDR(m, M_WAIT, MT_DATA);
		MCLGET(m, M_WAIT);
		sc->rxq[i].m = m;
		if (mtod(m, intptr_t) & 3) {
			m_adj(m, mtod(m, intptr_t) & 3);
		}
		err = bus_dmamap_load(sc->sc_dmat, sc->rxq[i].m_dmamap, 
			m->m_ext.ext_buf, MCLBYTES, NULL, 
			BUS_DMA_WAITOK);
		if (err) {
			panic("%s: dmamap_load failed: %i\n", __FUNCTION__, err);
		}
		sc->RDSC[i].Addr = sc->rxq[i].m_dmamap->dm_segs[0].ds_addr
			| (i == (RX_QLEN-1) ? ETH_RDSC_F_WRAP : 0);
		sc->RDSC[i].Info = 0;
		bus_dmamap_sync(sc->sc_dmat, sc->rxq[i].m_dmamap, 0,
			MCLBYTES, BUS_DMASYNC_PREREAD);
	}

	/* prepare transmit queue */
	for (i = 0; i < TX_QLEN; i++) {
		err = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1, MCLBYTES, 0,
					(BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW),
					&sc->txq[i].m_dmamap);
		if (err)
			panic("ARGH #1");
		sc->txq[i].m = NULL;
	}

	/* Program each queue's start addr, cur addr, and len registers
	 * with the physical addresses. 
	 */
	bus_dmamap_sync(sc->sc_dmat, sc->rbqpage_dmamap, 0, sc->rbqlen,
			 BUS_DMASYNC_PREREAD);
	addr = (void *)sc->rbqpage_dmamap->dm_segs[0].ds_addr;
	EMAC_WRITE(ETH_RBQP, (uint32_t)addr);

	/* Divide HCLK by 32 for MDC clock */
	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = emac_mii_readreg;
	sc->sc_mii.mii_writereg = emac_mii_writereg;
	sc->sc_mii.mii_statchg = emac_statchg;
	ifmedia_init(&sc->sc_mii.mii_media, IFM_IMASK, emac_mediachange,
		emac_mediastatus);
	mii_attach((device_t )sc, &sc->sc_mii, 0xffffffff, MII_PHY_ANY,
		MII_OFFSET_ANY, 0);
	ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);

	// enable / disable interrupts

#if 0
	// enable / disable interrupts
	EMAC_WRITE(ETH_IDR, -1);
	EMAC_WRITE(ETH_IER, ETH_ISR_RCOM | ETH_ISR_TBRE | ETH_ISR_TIDLE
		   | ETH_ISR_RBNA | ETH_ISR_ROVR);
//	(void)EMAC_READ(ETH_ISR); // why

	// enable transmitter / receiver
	EMAC_WRITE(ETH_CTL, ETH_CTL_TE | ETH_CTL_RE | ETH_CTL_ISR
		   | ETH_CTL_CSR | ETH_CTL_MPE);
#endif
	/*
	 * We can support 802.1Q VLAN-sized frames.
	 */
	sc->sc_ec.ec_capabilities |= ETHERCAP_VLAN_MTU;

        strcpy(ifp->if_xname, device_xname(sc->sc_dev));
        ifp->if_flags = IFF_BROADCAST|IFF_SIMPLEX|IFF_NOTRAILERS|IFF_MULTICAST;
        ifp->if_ioctl = emac_ifioctl;
        ifp->if_start = emac_ifstart;
        ifp->if_watchdog = emac_ifwatchdog;
        ifp->if_init = emac_ifinit;
        ifp->if_stop = emac_ifstop;
        ifp->if_timer = 0;
	ifp->if_softc = sc;
        IFQ_SET_READY(&ifp->if_snd);
        if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
        ether_ifattach(ifp, (sc)->sc_enaddr);
}

static int
emac_mediachange(struct ifnet *ifp)
{
	if (ifp->if_flags & IFF_UP)
		emac_ifinit(ifp);
	return (0);
}

static void
emac_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct emac_softc *sc = ifp->if_softc;

	mii_pollstat(&sc->sc_mii);
	ifmr->ifm_active = sc->sc_mii.mii_media_active;
	ifmr->ifm_status = sc->sc_mii.mii_media_status;
}


int
emac_mii_readreg(device_t self, int phy, int reg)
{
#ifndef EMAC_FAST
	struct emac_softc *sc = device_private(self);
#endif

	EMAC_WRITE(ETH_MAN, (ETH_MAN_HIGH | ETH_MAN_RW_RD
			     | ((phy << ETH_MAN_PHYA_SHIFT) & ETH_MAN_PHYA)
			     | ((reg << ETH_MAN_REGA_SHIFT) & ETH_MAN_REGA)
			     | ETH_MAN_CODE_IEEE802_3));
	while (!(EMAC_READ(ETH_SR) & ETH_SR_IDLE)) ;
	return (EMAC_READ(ETH_MAN) & ETH_MAN_DATA);
}

void
emac_mii_writereg(device_t self, int phy, int reg, int val)
{
#ifndef EMAC_FAST
	struct emac_softc *sc = device_private(self);
#endif

	EMAC_WRITE(ETH_MAN, (ETH_MAN_HIGH | ETH_MAN_RW_WR
			     | ((phy << ETH_MAN_PHYA_SHIFT) & ETH_MAN_PHYA)
			     | ((reg << ETH_MAN_REGA_SHIFT) & ETH_MAN_REGA)
			     | ETH_MAN_CODE_IEEE802_3
			     | (val & ETH_MAN_DATA)));
	while (!(EMAC_READ(ETH_SR) & ETH_SR_IDLE)) ;
}

	
void
emac_statchg(struct ifnet *ifp)
{
        struct emac_softc *sc = ifp->if_softc;
        uint32_t reg;

        /*
         * We must keep the MAC and the PHY in sync as
         * to the status of full-duplex!
         */
	reg = EMAC_READ(ETH_CFG);
        if (sc->sc_mii.mii_media_active & IFM_FDX)
                reg |= ETH_CFG_FD;
        else
                reg &= ~ETH_CFG_FD;
	EMAC_WRITE(ETH_CFG, reg);
}

void
emac_tick(void *arg)
{
	struct emac_softc* sc = (struct emac_softc *)arg;
	struct ifnet * ifp = &sc->sc_ec.ec_if;
	int s;
	uint32_t misses;

	ifp->if_collisions += EMAC_READ(ETH_SCOL) + EMAC_READ(ETH_MCOL);
	/* These misses are ok, they will happen if the RAM/CPU can't keep up */
	misses = EMAC_READ(ETH_DRFC);
	if (misses > 0) 
		printf("%s: %d rx misses\n", device_xname(sc->sc_dev), misses);

	s = splnet();
	if (emac_gctx(sc) > 0 && IFQ_IS_EMPTY(&ifp->if_snd) == 0) {
		emac_ifstart(ifp);
	}
	splx(s);

	mii_tick(&sc->sc_mii);
	callout_reset(&sc->emac_tick_ch, hz, emac_tick, sc);
}


static int
emac_ifioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct emac_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error;

	s = splnet();
	switch(cmd) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, cmd);
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			if (ifp->if_flags & IFF_RUNNING)
				emac_setaddr(ifp);
			error = 0;
		}
	}
	splx(s);
	return error;
}

static void
emac_ifstart(struct ifnet *ifp)
{
	struct emac_softc *sc = (struct emac_softc *)ifp->if_softc;
	struct mbuf *m;
	bus_dma_segment_t *segs;
	int s, bi, err, nsegs;

	s = splnet();	
start:
	if (emac_gctx(sc) == 0) {
		/* Enable transmit-buffer-free interrupt */
		EMAC_WRITE(ETH_IER, ETH_ISR_TBRE);
		ifp->if_flags |= IFF_OACTIVE;
		ifp->if_timer = 10;
		splx(s);
		return;
	}

	ifp->if_timer = 0;

	IFQ_POLL(&ifp->if_snd, m);
	if (m == NULL) {
		splx(s);
		return;
	}
//more:
	bi = (sc->txqi + sc->txqc) % TX_QLEN;
	if ((err = bus_dmamap_load_mbuf(sc->sc_dmat, sc->txq[bi].m_dmamap, m,
		BUS_DMA_NOWAIT)) || 
		sc->txq[bi].m_dmamap->dm_segs[0].ds_addr & 0x3 ||
		sc->txq[bi].m_dmamap->dm_nsegs > 1) {
		/* Copy entire mbuf chain to new single */
		struct mbuf *mn;

		if (err == 0) 
			bus_dmamap_unload(sc->sc_dmat, sc->txq[bi].m_dmamap);

		MGETHDR(mn, M_DONTWAIT, MT_DATA);
		if (mn == NULL) goto stop;
		if (m->m_pkthdr.len > MHLEN) {
			MCLGET(mn, M_DONTWAIT);
			if ((mn->m_flags & M_EXT) == 0) {
				m_freem(mn);
				goto stop;
			}
		}
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

	bpf_mtap(ifp, m);

	nsegs = sc->txq[bi].m_dmamap->dm_nsegs;
	segs = sc->txq[bi].m_dmamap->dm_segs;
	if (nsegs > 1) {
		panic("#### ARGH #2");
	}

	sc->txq[bi].m = m;
	sc->txqc++;

	DPRINTFN(2,("%s: start sending idx #%i mbuf %p (txqc=%i, phys %p), len=%u\n", __FUNCTION__, bi, sc->txq[bi].m, sc->txqc, (void*)segs->ds_addr,
		       (unsigned)m->m_pkthdr.len));
#ifdef	DIAGNOSTIC
	if (sc->txqc > TX_QLEN) {
		panic("%s: txqc %i > %i", __FUNCTION__, sc->txqc, TX_QLEN);
	}
#endif

	bus_dmamap_sync(sc->sc_dmat, sc->txq[bi].m_dmamap, 0, 
		sc->txq[bi].m_dmamap->dm_mapsize, 
		BUS_DMASYNC_PREWRITE);

	EMAC_WRITE(ETH_TAR, segs->ds_addr);
	EMAC_WRITE(ETH_TCR, m->m_pkthdr.len);
	if (IFQ_IS_EMPTY(&ifp->if_snd) == 0)
		goto start;
stop:

	splx(s);
	return;
}

static void
emac_ifwatchdog(struct ifnet *ifp)
{
	struct emac_softc *sc = (struct emac_softc *)ifp->if_softc;

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;
       	printf("%s: device timeout, CTL = 0x%08x, CFG = 0x%08x\n",
		device_xname(sc->sc_dev), EMAC_READ(ETH_CTL), EMAC_READ(ETH_CFG));
}

static int
emac_ifinit(struct ifnet *ifp)
{
	struct emac_softc *sc = ifp->if_softc;
	int s = splnet();

	callout_stop(&sc->emac_tick_ch);

	// enable interrupts
	EMAC_WRITE(ETH_IDR, -1);
	EMAC_WRITE(ETH_IER, ETH_ISR_RCOM | ETH_ISR_TBRE | ETH_ISR_TIDLE
		   | ETH_ISR_RBNA | ETH_ISR_ROVR);

	// enable transmitter / receiver
	EMAC_WRITE(ETH_CTL, ETH_CTL_TE | ETH_CTL_RE | ETH_CTL_ISR
		   | ETH_CTL_CSR | ETH_CTL_MPE);

	mii_mediachg(&sc->sc_mii);
	callout_reset(&sc->emac_tick_ch, hz, emac_tick, sc);
        ifp->if_flags |= IFF_RUNNING;
	splx(s);
	return 0;
}

static void
emac_ifstop(struct ifnet *ifp, int disable)
{
//	uint32_t u;
	struct emac_softc *sc = ifp->if_softc;

#if 0
	EMAC_WRITE(ETH_CTL, ETH_CTL_MPE);	// disable everything
	EMAC_WRITE(ETH_IDR, -1);		// disable interrupts
//	EMAC_WRITE(ETH_RBQP, 0);		// clear receive
	EMAC_WRITE(ETH_CFG, ETH_CFG_CLK_32 | ETH_CFG_SPD | ETH_CFG_FD | ETH_CFG_BIG);
	EMAC_WRITE(ETH_TCR, 0);			// send nothing
//	(void)EMAC_READ(ETH_ISR);
	u = EMAC_READ(ETH_TSR);
	EMAC_WRITE(ETH_TSR, (u & (ETH_TSR_UND | ETH_TSR_COMP | ETH_TSR_BNQ
				  | ETH_TSR_IDLE | ETH_TSR_RLE
				  | ETH_TSR_COL|ETH_TSR_OVR)));
	u = EMAC_READ(ETH_RSR);
	EMAC_WRITE(ETH_RSR, (u & (ETH_RSR_OVR|ETH_RSR_REC|ETH_RSR_BNA)));
#endif
	callout_stop(&sc->emac_tick_ch);

	/* Down the MII. */
	mii_down(&sc->sc_mii);

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
	sc->sc_mii.mii_media_status &= ~IFM_ACTIVE;
}

static void
emac_setaddr(struct ifnet *ifp)
{
	struct emac_softc *sc = ifp->if_softc;
	struct ethercom *ac = &sc->sc_ec;
	struct ether_multi *enm;
	struct ether_multistep step;
	uint8_t ias[3][ETHER_ADDR_LEN];
	uint32_t h, nma = 0, hashes[2] = { 0, 0 };
	uint32_t ctl = EMAC_READ(ETH_CTL);
	uint32_t cfg = EMAC_READ(ETH_CFG);

	/* disable receiver temporarily */
	EMAC_WRITE(ETH_CTL, ctl & ~ETH_CTL_RE);

	cfg &= ~(ETH_CFG_MTI | ETH_CFG_UNI | ETH_CFG_CAF | ETH_CFG_UNI);

	if (ifp->if_flags & IFF_PROMISC) {
		cfg |=  ETH_CFG_CAF;
	} else {
		cfg &= ~ETH_CFG_CAF;
	}

	// ETH_CFG_BIG?

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
			cfg |= ETH_CFG_CAF;
			hashes[0] = 0xffffffffUL;
			hashes[1] = 0xffffffffUL;
			ifp->if_flags |= IFF_ALLMULTI;
			nma = 0;
			break;
		}

		if (nma < 3) {
			/* We can program 3 perfect address filters for mcast */
			memcpy(ias[nma], enm->enm_addrlo, ETHER_ADDR_LEN);
		} else {
			/*
			 * XXX: Datasheet is not very clear here, I'm not sure
			 * if I'm doing this right.  --joff
			 */
			h = ether_crc32_le(enm->enm_addrlo, ETHER_ADDR_LEN);

			/* Just want the 6 most-significant bits. */
			h = h >> 26;

			hashes[ h / 32 ] |=  (1 << (h % 32));
			cfg |= ETH_CFG_MTI;
		}
		ETHER_NEXT_MULTI(step, enm);
		nma++;
	}

	// program...
	DPRINTFN(1,("%s: en0 %02x:%02x:%02x:%02x:%02x:%02x\n", __FUNCTION__,
		    sc->sc_enaddr[0], sc->sc_enaddr[1], sc->sc_enaddr[2],
		    sc->sc_enaddr[3], sc->sc_enaddr[4], sc->sc_enaddr[5]));
	EMAC_WRITE(ETH_SA1L, (sc->sc_enaddr[3] << 24)
		   | (sc->sc_enaddr[2] << 16) | (sc->sc_enaddr[1] << 8)
		   | (sc->sc_enaddr[0]));
	EMAC_WRITE(ETH_SA1H, (sc->sc_enaddr[5] << 8)
		   | (sc->sc_enaddr[4]));
	if (nma > 1) {
		DPRINTFN(1,("%s: en1 %02x:%02x:%02x:%02x:%02x:%02x\n", __FUNCTION__,
		       ias[0][0], ias[0][1], ias[0][2],
		       ias[0][3], ias[0][4], ias[0][5]));
		EMAC_WRITE(ETH_SA2L, (ias[0][3] << 24)
			   | (ias[0][2] << 16) | (ias[0][1] << 8)
			   | (ias[0][0]));
		EMAC_WRITE(ETH_SA2H, (ias[0][4] << 8)
			   | (ias[0][5]));
	}
	if (nma > 2) {
		DPRINTFN(1,("%s: en2 %02x:%02x:%02x:%02x:%02x:%02x\n", __FUNCTION__,
		       ias[1][0], ias[1][1], ias[1][2],
		       ias[1][3], ias[1][4], ias[1][5]));
		EMAC_WRITE(ETH_SA3L, (ias[1][3] << 24)
			   | (ias[1][2] << 16) | (ias[1][1] << 8)
			   | (ias[1][0]));
		EMAC_WRITE(ETH_SA3H, (ias[1][4] << 8)
			   | (ias[1][5]));
	}
	if (nma > 3) {
		DPRINTFN(1,("%s: en3 %02x:%02x:%02x:%02x:%02x:%02x\n", __FUNCTION__,
		       ias[2][0], ias[2][1], ias[2][2],
		       ias[2][3], ias[2][4], ias[2][5]));
		EMAC_WRITE(ETH_SA3L, (ias[2][3] << 24)
			   | (ias[2][2] << 16) | (ias[2][1] << 8)
			   | (ias[2][0]));
		EMAC_WRITE(ETH_SA3H, (ias[2][4] << 8)
			   | (ias[2][5]));
	}
	EMAC_WRITE(ETH_HSH, hashes[0]);
	EMAC_WRITE(ETH_HSL, hashes[1]);
	EMAC_WRITE(ETH_CFG, cfg);
	EMAC_WRITE(ETH_CTL, ctl | ETH_CTL_RE);
}
