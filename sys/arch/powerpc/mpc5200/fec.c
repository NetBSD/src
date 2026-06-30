/*	$NetBSD: fec.c,v 1.2 2026/06/30 21:31:31 rkujawa Exp $	*/

/*-
 * Copyright (c) 2008, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa and Robert Swindells.
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

/*
 * Driver for the MPC5200B Fast Ethernet Controller (FEC).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fec.c,v 1.2 2026/06/30 21:31:31 rkujawa Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/bus.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/bpf.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>

#include <powerpc/mpc5200/obiovar.h>
#include <powerpc/mpc5200/mpc5200reg.h>
#include <powerpc/mpc5200/fecreg.h>
#include <powerpc/mpc5200/fecvar.h>
#include <powerpc/mpc5200/cdmvar.h>
#include <powerpc/mpc5200/bestcommvar.h>

#define	FEC_READ(sc, r)		bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (r))
#define	FEC_WRITE(sc, r, v)	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (r), (v))

#define	FEC_RX_TASK		3	/* TASK_FEC_RX image slot	*/
#define	FEC_TX_TASK		2	/* TASK_FEC_TX image slot	*/
#define	FEC_RX_INITIATOR	3	/* INITIATOR_FEC_RX		*/
#define	FEC_TX_INITIATOR	4	/* INITIATOR_FEC_TX		*/

static const struct bestcomm_bd_layout fec_rx_layout = BESTCOMM_LAYOUT_FEC_RX;
static const struct bestcomm_bd_layout fec_tx_layout = BESTCOMM_LAYOUT_FEC_TX;

static int	fec_match(device_t, cfdata_t, void *);
static void	fec_attach(device_t, device_t, void *);

static int	fec_miibus_readreg(device_t, int, int, uint16_t *);
static int	fec_miibus_writereg(device_t, int, int, uint16_t);
static void	fec_miibus_statchg(struct ifnet *);

static int	fec_ioctl(struct ifnet *, u_long, void *);
static int	fec_init(struct ifnet *);
static void	fec_stop(struct ifnet *, int);
static void	fec_start(struct ifnet *);
static void	fec_tick(void *);

static void	fec_reset(struct fec_softc *);
static void	fec_mac_setup(struct fec_softc *);
static void	fec_set_filter(struct fec_softc *);
static void	fec_mib_sync(struct fec_softc *, bool);
static int	fec_newbuf(struct fec_softc *, u_int);
static int	fec_intr(void *);
static int	fec_rxintr(void *);
static int	fec_txintr(void *);

CFATTACH_DECL_NEW(fec, sizeof(struct fec_softc),
    fec_match, fec_attach, NULL, NULL);

static int
fec_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args *oba = aux;
	char compat[40];
	int len;

	if (strcmp(oba->obio_name, "ethernet") == 0)
		return 1;

	len = OF_getprop(oba->obio_node, "compatible", compat, sizeof(compat));
	if (len > 0 &&
	    (strcmp(compat, "mpc5200-fec") == 0 ||
	     strcmp(compat, "mpc5200b-fec") == 0))
		return 1;

	return 0;
}

static void
fec_attach(device_t parent, device_t self, void *aux)
{
	struct fec_softc *sc = device_private(self);
	struct obio_attach_args *oba = aux;
	struct ifnet *ifp = &sc->sc_if;
	struct mii_data *mii = &sc->sc_mii;
	uint8_t enaddr[ETHER_ADDR_LEN];
	bus_size_t size;
	int ist;

	sc->sc_dev = self;
	sc->sc_iot = oba->obio_bst;
	sc->sc_dmat = oba->obio_dmat;
	sc->sc_pa = oba->obio_addr;
	callout_init(&sc->sc_tick_ch, 0);
	callout_setfunc(&sc->sc_tick_ch, fec_tick, sc);

	size = oba->obio_size != 0 ? oba->obio_size : FEC_REG_SIZE;
	if (bus_space_map(sc->sc_iot, oba->obio_addr, size, 0,
	    &sc->sc_ioh) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	if (OF_getprop(oba->obio_node, "local-mac-address", enaddr,
	    sizeof(enaddr)) != sizeof(enaddr) &&
	    OF_getprop(oba->obio_node, "mac-address", enaddr,
	    sizeof(enaddr)) != sizeof(enaddr)) {
		aprint_error(": no MAC address\n");
		return;
	}
	memcpy(sc->sc_enaddr, enaddr, ETHER_ADDR_LEN);

	if (obio_decode_interrupt(oba->obio_node, 0, &sc->sc_irq, &ist))
		sc->sc_ih = intr_establish(sc->sc_irq, ist, IPL_NET,
		    fec_intr, sc);

	aprint_normal(": MPC5200 FEC, address %s\n",
	    ether_sprintf(sc->sc_enaddr));

	/* Reset the controller and program the MII management clock. */
	sc->sc_ipb_freq = mpc5200_cdm_get_ipb_freq();
	fec_reset(sc);

	/* MII / PHY. */
	mii->mii_ifp = ifp;
	mii->mii_readreg = fec_miibus_readreg;
	mii->mii_writereg = fec_miibus_writereg;
	mii->mii_statchg = fec_miibus_statchg;
	sc->sc_ec.ec_mii = mii;
	ifmedia_init(&mii->mii_media, 0, ether_mediachange, ether_mediastatus);
	/* MIIF_DOPAUSE: advertise 802.3 PAUSE so we can honour rx flow control. */
	mii_attach(self, mii, 0xffffffff, MII_PHY_ANY, MII_OFFSET_ANY,
	    MIIF_DOPAUSE);
	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		aprint_error_dev(self, "no PHY found\n");
		ifmedia_add(&mii->mii_media, IFM_ETHER | IFM_MANUAL, 0, NULL);
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_MANUAL);
	} else
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_AUTO);

	/* The controller accepts VLAN-sized frames (see fec_mac_setup()). */
	sc->sc_ec.ec_capabilities |= ETHERCAP_VLAN_MTU;

	strlcpy(ifp->if_xname, device_xname(self), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = fec_init;
	ifp->if_stop = fec_stop;
	ifp->if_start = fec_start;
	ifp->if_ioctl = fec_ioctl;
	IFQ_SET_READY(&ifp->if_snd);

	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, sc->sc_enaddr);
}

/*
 * Soft-reset the controller and set the MII management clock divider so MDC
 * stays at or below 2.5 MHz, derived from the IPB clock.
 */
static void
fec_reset(struct fec_softc *sc)
{
	uint32_t div;
	int i;

	FEC_WRITE(sc, FEC_ECNTRL, FEC_ECNTRL_RESET);
	for (i = 0; i < 1000; i++) {
		if ((FEC_READ(sc, FEC_ECNTRL) & FEC_ECNTRL_RESET) == 0)
			break;
		DELAY(10);
	}
	FEC_WRITE(sc, FEC_IMASK, 0);			/* mask all events */
	FEC_WRITE(sc, FEC_IEVENT, 0xffffffff);		/* clear events	 */

	div = howmany(sc->sc_ipb_freq, 2 * FEC_MII_MAX_CLK);
	if (div > 0x3f)
		div = 0x3f;
	FEC_WRITE(sc, FEC_MII_SPEED, FEC_MII_SPEED_DIV(div));
}

static int
fec_mii_wait(struct fec_softc *sc)
{
	int i;

	for (i = 0; i < FEC_PHY_TIMEOUT; i++) {
		if (FEC_READ(sc, FEC_IEVENT) & FEC_INT_MII) {
			FEC_WRITE(sc, FEC_IEVENT, FEC_INT_MII);
			return 0;
		}
		DELAY(10);
	}
	return ETIMEDOUT;
}

static int
fec_miibus_readreg(device_t self, int phy, int reg, uint16_t *val)
{
	struct fec_softc *sc = device_private(self);

	FEC_WRITE(sc, FEC_IEVENT, FEC_INT_MII);
	FEC_WRITE(sc, FEC_MII_DATA, FEC_MII_READ(phy, reg));
	if (fec_mii_wait(sc) != 0)
		return ETIMEDOUT;

	*val = FEC_READ(sc, FEC_MII_DATA) & FEC_MII_DATA_MASK;
	return 0;
}

static int
fec_miibus_writereg(device_t self, int phy, int reg, uint16_t val)
{
	struct fec_softc *sc = device_private(self);

	FEC_WRITE(sc, FEC_IEVENT, FEC_INT_MII);
	FEC_WRITE(sc, FEC_MII_DATA, FEC_MII_WRITE(phy, reg, val));
	return fec_mii_wait(sc);
}

static void
fec_miibus_statchg(struct ifnet *ifp)
{
	struct fec_softc *sc = ifp->if_softc;
	uint32_t xcr, rcr;

	xcr = FEC_READ(sc, FEC_X_CNTRL);
	if (sc->sc_mii.mii_media_active & IFM_FDX)
		xcr |= FEC_X_CNTRL_FDEN;
	else
		xcr &= ~FEC_X_CNTRL_FDEN;
	FEC_WRITE(sc, FEC_X_CNTRL, xcr);

	rcr = FEC_READ(sc, FEC_R_CNTRL);
	if (sc->sc_mii.mii_media_active & IFM_ETH_RXPAUSE)
		rcr |= FEC_R_CNTRL_FCE;
	else
		rcr &= ~FEC_R_CNTRL_FCE;
	FEC_WRITE(sc, FEC_R_CNTRL, rcr);
}

static void
fec_tick(void *arg)
{
	struct fec_softc *sc = arg;
	int s;

	s = splnet();
	mii_tick(&sc->sc_mii);
	if (sc->sc_if.if_flags & IFF_RUNNING)
		fec_mib_sync(sc, false);	/* fold hw counters into if_stats */
	splx(s);

	/*
	 * Meh. Under load the receive FIFO can overrun, work it around.
	 */
	if ((sc->sc_if.if_flags & IFF_RUNNING) &&
	    (FEC_READ(sc, FEC_ECNTRL) & FEC_ECNTRL_ETHER_EN) == 0) {
		if_statinc(&sc->sc_if, if_ierrors);
		aprint_error_dev(sc->sc_dev,
		    "ETHER_EN cleared, resetting\n");
		s = splnet();
		(void)fec_init(&sc->sc_if);	/* reschedules the callout */
		splx(s);
		return;
	}

	callout_schedule(&sc->sc_tick_ch, hz);
}

/* Program the station address and receive/transmit MAC control. */
static void
fec_mac_setup(struct fec_softc *sc)
{
	FEC_WRITE(sc, FEC_PADDR1,
	    (sc->sc_enaddr[0] << 24) | (sc->sc_enaddr[1] << 16) |
	    (sc->sc_enaddr[2] << 8) | sc->sc_enaddr[3]);
	FEC_WRITE(sc, FEC_PADDR2,
	    (sc->sc_enaddr[4] << 24) | (sc->sc_enaddr[5] << 16) | 0x8808);
	FEC_WRITE(sc, FEC_IADDR1, 0);
	FEC_WRITE(sc, FEC_IADDR2, 0);
	/*
	 * Allow VLAN-tagged frames (ETHERCAP_VLAN_MTU)
	 */
	FEC_WRITE(sc, FEC_R_CNTRL,
	    FEC_R_CNTRL_MAX_FL(ETHER_MAX_LEN + ETHER_VLAN_ENCAP_LEN) |
	    FEC_R_CNTRL_MII_MODE);
	FEC_WRITE(sc, FEC_X_CNTRL, FEC_X_CNTRL_FDEN);

	/*
	 * Enable frame mode on both FIFOs
	 */
	FEC_WRITE(sc, FEC_RESET_CNTRL, FEC_RESET_CNTRL_FIFO_EN);

	FEC_WRITE(sc, FEC_RFIFO_CONTROL, FEC_FIFO_CTRL_INIT);
	FEC_WRITE(sc, FEC_TFIFO_CONTROL, FEC_FIFO_CTRL_INIT);

	/*
	 * The transmit FIFO requests data from BestComm only while it holds
	 * fewer than "alarm" bytes!
	 */
	FEC_WRITE(sc, FEC_TFIFO_ALARM, 0x80);
}

/*
 * Program the receive address filter
 */
static void
fec_set_filter(struct fec_softc *sc)
{
	struct ethercom *ec = &sc->sc_ec;
	struct ifnet *ifp = &sc->sc_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t gaddr[2] = { 0, 0 };
	uint32_t rcr;
	u_int h;

	rcr = FEC_READ(sc, FEC_R_CNTRL) & ~FEC_R_CNTRL_PROM;

	if (ifp->if_flags & IFF_PROMISC) {
		rcr |= FEC_R_CNTRL_PROM;
		gaddr[0] = gaddr[1] = 0xffffffff;
		goto done;
	}

	ETHER_LOCK(ec);
	ec->ec_flags &= ~ETHER_F_ALLMULTI;
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
		    ETHER_ADDR_LEN) != 0) {
			/*
			 * A range of addresses
			 */
			ec->ec_flags |= ETHER_F_ALLMULTI;
			gaddr[0] = gaddr[1] = 0xffffffff;
			break;
		}
		h = ether_crc32_le(enm->enm_addrlo, ETHER_ADDR_LEN) >> 26;
		gaddr[h >> 5] |= 1U << (h & 0x1f);
		ETHER_NEXT_MULTI(step, enm);
	}
	ETHER_UNLOCK(ec);

done:
	FEC_WRITE(sc, FEC_GADDR1, gaddr[1]);	/* buckets 32..63 */
	FEC_WRITE(sc, FEC_GADDR2, gaddr[0]);	/* buckets  0..31 */
	FEC_WRITE(sc, FEC_R_CNTRL, rcr);
}

/*
 * On-chip counters we dump into the interface stats.
 */
static const struct fec_mibcnt {
	bus_size_t	reg;
	if_stat_t	stat;
} fec_mibtab[] = {
	{ FEC_RMON_T_COL,    if_collisions },
	{ FEC_IEEE_T_LCOL,   if_oerrors },
	{ FEC_IEEE_T_EXCOL,  if_oerrors },
	{ FEC_IEEE_T_MACERR, if_oerrors },	/* Tx FIFO underrun	*/
	{ FEC_IEEE_R_DROP,   if_iqdrops },
	{ FEC_IEEE_R_CRC,    if_ierrors },
	{ FEC_IEEE_R_ALIGN,  if_ierrors },
	{ FEC_IEEE_R_MACERR, if_iqdrops },	/* Rx FIFO overflow	*/
};
__CTASSERT(__arraycount(fec_mibtab) == FEC_NMIB);

/*
 * Harvest the on-chip MIB counters.
 */
static void
fec_mib_sync(struct fec_softc *sc, bool prime)
{
	struct ifnet *ifp = &sc->sc_if;
	net_stat_ref_t nsr;
	u_int i;

	if (prime) {
		for (i = 0; i < FEC_NMIB; i++)
			sc->sc_mib_prev[i] = FEC_READ(sc, fec_mibtab[i].reg);
		return;
	}

	nsr = IF_STAT_GETREF(ifp);
	for (i = 0; i < FEC_NMIB; i++) {
		uint32_t cur = FEC_READ(sc, fec_mibtab[i].reg);
		uint32_t delta = cur - sc->sc_mib_prev[i];	/* wraps mod 2^32 */

		if (delta != 0)
			if_statadd_ref(ifp, nsr, fec_mibtab[i].stat, delta);
		sc->sc_mib_prev[i] = cur;
	}
	IF_STAT_PUTREF(ifp);
}

/* Allocate a cluster for receive descriptor idx and hand it to the engine. */
static int
fec_newbuf(struct fec_softc *sc, u_int idx)
{
	struct mbuf *m;
	int error;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return ENOBUFS;
	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return ENOBUFS;
	}
	m->m_len = m->m_pkthdr.len = MCLBYTES;
	error = bus_dmamap_load_mbuf(sc->sc_dmat, sc->sc_rxmap[idx], m,
	    BUS_DMA_NOWAIT | BUS_DMA_READ);
	if (error) {
		m_freem(m);
		return error;
	}
	bus_dmamap_sync(sc->sc_dmat, sc->sc_rxmap[idx], 0, MCLBYTES,
	    BUS_DMASYNC_PREREAD);
	sc->sc_rxmbuf[idx] = m;
	bestcomm_bd_post(&sc->sc_rxring, idx,
	    sc->sc_rxmap[idx]->dm_segs[0].ds_addr, MCLBYTES, 0);
	return 0;
}

static int
fec_init(struct ifnet *ifp)
{
	struct fec_softc *sc = ifp->if_softc;
	u_int i;
	int error;
	bool link_up;

	link_up = (sc->sc_mii.mii_media_status & IFM_ACTIVE) != 0;

	fec_stop(ifp, 0);

	if (!bestcomm_available()) {
		aprint_error_ifnet(ifp, "BestComm SDMA not available\n");
		return ENXIO;
	}

	fec_reset(sc);
	fec_mac_setup(sc);
	fec_set_filter(sc);

	/*
	 * Enable the on-chip MIB counter block
	 */
	FEC_WRITE(sc, FEC_MIB_CONTROL, 0);
	fec_mib_sync(sc, true);

	if (bestcomm_bd_setup(&sc->sc_rxring, FEC_RX_TASK, &fec_rx_layout,
	    sc->sc_pa + FEC_RFIFO_DATA, FEC_NRXDESC, MCLBYTES, 4,
	    FEC_RX_INITIATOR, 6) != 0 ||
	    bestcomm_bd_setup(&sc->sc_txring, FEC_TX_TASK, &fec_tx_layout,
	    sc->sc_pa + FEC_TFIFO_DATA, FEC_NTXDESC, MCLBYTES, 4,
	    FEC_TX_INITIATOR, 5) != 0) {
		aprint_error_ifnet(ifp, "cannot set up SDMA rings\n");
		error = ENOMEM;
		goto fail;
	}

	for (i = 0; i < FEC_NRXDESC; i++) {
		if (sc->sc_rxmap[i] == NULL &&
		    bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1, MCLBYTES, 0,
		    BUS_DMA_NOWAIT, &sc->sc_rxmap[i]) != 0) {
			error = ENOMEM;
			goto fail;
		}
		if ((error = fec_newbuf(sc, i)) != 0)
			goto fail;
	}
	for (i = 0; i < FEC_NTXDESC; i++) {
		if (sc->sc_txmap[i] == NULL &&
		    bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1, MCLBYTES, 0,
		    BUS_DMA_NOWAIT, &sc->sc_txmap[i]) != 0) {
			error = ENOMEM;
			goto fail;
		}
	}
	sc->sc_rxptr = 0;
	sc->sc_txnext = sc->sc_txdirty = sc->sc_txbusy = 0;

	/* Hook the BestComm task-completion interrupts (via the SIU cascade). */
	sc->sc_rxih = intr_establish(bestcomm_task_irq(FEC_RX_TASK), IST_LEVEL,
	    IPL_NET, fec_rxintr, sc);
	sc->sc_txih = intr_establish(bestcomm_task_irq(FEC_TX_TASK), IST_LEVEL,
	    IPL_NET, fec_txintr, sc);

	/*
	 * Bring the link up before enabling the controller.
	 */
	mii_mediachg(&sc->sc_mii);
	if (!link_up) {
		for (i = 0; i < 500; i++) {
			mii_pollstat(&sc->sc_mii);
			if (sc->sc_mii.mii_media_status & IFM_ACTIVE)
				break;
			DELAY(10000);
		}
	}

	FEC_WRITE(sc, FEC_ECNTRL, FEC_ECNTRL_ETHER_EN);
	bestcomm_task_start(FEC_RX_TASK);
	bestcomm_task_start(FEC_TX_TASK);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	callout_schedule(&sc->sc_tick_ch, hz);
	return 0;

fail:
	/* Unwind any partial setup so we leave a clean, fully-stopped state. */
	fec_stop(ifp, 1);
	return error;
}

static void
fec_stop(struct ifnet *ifp, int disable)
{
	struct fec_softc *sc = ifp->if_softc;
	u_int i;

	callout_stop(&sc->sc_tick_ch);

	/*
	 * Stop the SDMA tasks and release their buffer-descriptor rings back
	 * to the on-chip SRAM pool.
	 */
	bestcomm_bd_teardown(&sc->sc_rxring);
	bestcomm_bd_teardown(&sc->sc_txring);

	FEC_WRITE(sc, FEC_ECNTRL, 0);

	if (sc->sc_rxih != NULL) {
		intr_disestablish(sc->sc_rxih);
		sc->sc_rxih = NULL;
	}
	if (sc->sc_txih != NULL) {
		intr_disestablish(sc->sc_txih);
		sc->sc_txih = NULL;
	}

	mii_down(&sc->sc_mii);

	for (i = 0; i < FEC_NRXDESC; i++) {
		if (sc->sc_rxmbuf[i] != NULL) {
			bus_dmamap_unload(sc->sc_dmat, sc->sc_rxmap[i]);
			m_freem(sc->sc_rxmbuf[i]);
			sc->sc_rxmbuf[i] = NULL;
		}
	}
	for (i = 0; i < FEC_NTXDESC; i++) {
		if (sc->sc_txmbuf[i] != NULL) {
			bus_dmamap_unload(sc->sc_dmat, sc->sc_txmap[i]);
			m_freem(sc->sc_txmbuf[i]);
			sc->sc_txmbuf[i] = NULL;
		}
	}
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

static void
fec_start(struct ifnet *ifp)
{
	struct fec_softc *sc = ifp->if_softc;
	struct mbuf *m, *mn;
	u_int idx;
	int error, posted = 0;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	while (sc->sc_txbusy < FEC_NTXDESC) {
		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;
		idx = sc->sc_txnext;

		error = bus_dmamap_load_mbuf(sc->sc_dmat, sc->sc_txmap[idx], m,
		    BUS_DMA_NOWAIT | BUS_DMA_WRITE);
		if (error == EFBIG) {
			/* The buffer descriptor holds one segment; collapse. */
			mn = m_defrag(m, M_DONTWAIT);
			if (mn != NULL) {
				m = mn;
				error = bus_dmamap_load_mbuf(sc->sc_dmat,
				    sc->sc_txmap[idx], m,
				    BUS_DMA_NOWAIT | BUS_DMA_WRITE);
			}
		}
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (error) {
			m_freem(m);
			if_statinc(ifp, if_oerrors);
			continue;
		}

		bus_dmamap_sync(sc->sc_dmat, sc->sc_txmap[idx], 0,
		    m->m_pkthdr.len, BUS_DMASYNC_PREWRITE);
		bpf_mtap(ifp, m, BPF_D_OUT);
		sc->sc_txmbuf[idx] = m;
		bestcomm_bd_post(&sc->sc_txring, idx,
		    sc->sc_txmap[idx]->dm_segs[0].ds_addr, m->m_pkthdr.len,
		    BESTCOMM_BD_FLAG_LAST | BESTCOMM_BD_FLAG_INT);
		sc->sc_txnext = (idx + 1) % FEC_NTXDESC;
		sc->sc_txbusy++;
		posted = 1;
	}

	/*
	 * (Re-)enable the transmit task.
	 */
	if (posted)
		bestcomm_task_start(FEC_TX_TASK);

	if (sc->sc_txbusy == FEC_NTXDESC)
		ifp->if_flags |= IFF_OACTIVE;
}

/* BestComm FEC_RX task completion: hand received frames up, refill the ring. */
static int
fec_rxintr(void *arg)
{
	struct fec_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_if;
	struct mbuf *m;
	uint32_t st;
	u_int idx;
	int len, handled = 0;

	for (;;) {
		idx = sc->sc_rxptr;
		st = bestcomm_bd_status(&sc->sc_rxring, idx);
		if (st & BESTCOMM_BD_READY)
			break;
		handled = 1;

		bus_dmamap_sync(sc->sc_dmat, sc->sc_rxmap[idx], 0, MCLBYTES,
		    BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmat, sc->sc_rxmap[idx]);
		m = sc->sc_rxmbuf[idx];
		sc->sc_rxmbuf[idx] = NULL;

		len = st & BESTCOMM_BD_LEN_MASK;
		if (len > ETHER_CRC_LEN)
			len -= ETHER_CRC_LEN;	/* strip FCS */

		if (fec_newbuf(sc, idx) != 0) {
			/* No replacement: recycle this buffer, drop the frame. */
			sc->sc_rxmbuf[idx] = m;
			(void)bus_dmamap_load_mbuf(sc->sc_dmat,
			    sc->sc_rxmap[idx], m, BUS_DMA_NOWAIT | BUS_DMA_READ);
			bus_dmamap_sync(sc->sc_dmat, sc->sc_rxmap[idx], 0,
			    MCLBYTES, BUS_DMASYNC_PREREAD);
			bestcomm_bd_post(&sc->sc_rxring, idx,
			    sc->sc_rxmap[idx]->dm_segs[0].ds_addr, MCLBYTES, 0);
			if_statinc(ifp, if_ierrors);
		} else {
			m_set_rcvif(m, ifp);
			m->m_pkthdr.len = m->m_len = len;
			if_percpuq_enqueue(ifp->if_percpuq, m);
		}
		sc->sc_rxptr = (idx + 1) % FEC_NRXDESC;
	}

	return handled;
}

/* BestComm FEC_TX task completion: reap transmitted frames. */
static int
fec_txintr(void *arg)
{
	struct fec_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_if;
	uint32_t st;
	u_int idx;
	int handled = 0;

	while (sc->sc_txbusy > 0) {
		idx = sc->sc_txdirty;
		st = bestcomm_bd_status(&sc->sc_txring, idx);
		if (st & BESTCOMM_BD_READY)
			break;
		handled = 1;

		bus_dmamap_sync(sc->sc_dmat, sc->sc_txmap[idx], 0,
		    sc->sc_txmbuf[idx]->m_pkthdr.len, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, sc->sc_txmap[idx]);
		m_freem(sc->sc_txmbuf[idx]);
		sc->sc_txmbuf[idx] = NULL;
		if_statinc(ifp, if_opackets);

		sc->sc_txdirty = (idx + 1) % FEC_NTXDESC;
		sc->sc_txbusy--;
	}
	if (handled) {
		ifp->if_flags &= ~IFF_OACTIVE;
		if_schedule_deferred_start(ifp);
	}
	return handled;
}

static int
fec_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct fec_softc *sc = ifp->if_softc;
	int s, error;

	s = splnet();
	error = ether_ioctl(ifp, cmd, data);
	if (error == ENETRESET) {
		/*
		 * The multicast list or the promiscuous flag changed.
		 */
		if (ifp->if_flags & IFF_RUNNING)
			fec_set_filter(sc);
		error = 0;
	}
	splx(s);
	return error;
}

static int
fec_intr(void *arg)
{
	struct fec_softc *sc = arg;
	uint32_t ievent;

	ievent = FEC_READ(sc, FEC_IEVENT);
	FEC_WRITE(sc, FEC_IEVENT, ievent);	/* acknowledge */
	/* Frame rx/tx is serviced by the BestComm task-completion handlers. */
	return ievent != 0;
}
