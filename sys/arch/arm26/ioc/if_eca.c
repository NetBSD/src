/*	$NetBSD: if_eca.c,v 1.6 2001/09/22 14:42:51 bjh21 Exp $	*/

/*-
 * Copyright (c) 2001 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
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

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: if_eca.c,v 1.6 2001/09/22 14:42:51 bjh21 Exp $");

#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_eco.h>

#include <machine/bus.h>
#include <machine/fiq.h>
#include <machine/intr.h>
#include <machine/machdep.h>

#include <arch/arm26/iobus/iocvar.h>

#include <dev/ic/mc6854reg.h>
#include <arch/arm26/ioc/if_ecavar.h>

static int eca_match(struct device *, struct cfdata *, void *);
static void eca_attach(struct device *, struct device *, void *);

static int eca_init(struct ifnet *);
static void eca_stop(struct ifnet *ifp, int disable);

static int eca_claimwire(struct ifnet *);
static void eca_txframe(struct ifnet *, struct mbuf *);

static void eca_tx_downgrade(void);
static void eca_txdone(void *);

static int eca_init_rxbuf(struct eca_softc *sc, int flags);
static void eca_init_rx(struct eca_softc *sc);

static void eca_rx_downgrade(void);
static void eca_gotframe(void *);

struct eca_softc *eca_fiqowner;

struct cfattach eca_ca = {
	sizeof(struct eca_softc), eca_match, eca_attach
};

static int
eca_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct ioc_attach_args *ioc = aux;

	/* Econet never uses LOOP mode. */
	if ((bus_space_read_1(ioc->ioc_sync_t, ioc->ioc_sync_h, MC6854_SR1) &
	    MC6854_SR1_LOOP) != 0)
		return 0;

	return 1;
}

static void
eca_attach(struct device *parent, struct device *self, void *aux)
{
	struct eca_softc *sc = (void *)self;
	struct ioc_attach_args *ioc = aux;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	u_int8_t myaddr[ECO_ADDR_LEN];

	sc->sc_iot = ioc->ioc_sync_t;
	sc->sc_ioh = ioc->ioc_sync_h;

	myaddr[0] = cmos_read(0x40);
	myaddr[1] = 0;

	printf(": station %s", eco_sprintf(myaddr));
	/* It's traditional to print the clock state at boot. */
	if ((bus_space_read_1(sc->sc_iot, sc->sc_ioh, MC6854_SR2) &
	    MC6854_SR2_NDCD))
		printf(", no clock");

	/* Initialise ifnet structure. */

	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_init = eca_init;
	ifp->if_stop = eca_stop;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS;
	IFQ_SET_READY(&ifp->if_snd);
	sc->sc_ec.ec_claimwire = eca_claimwire;
	sc->sc_ec.ec_txframe = eca_txframe;

	sc->sc_rx_soft = softintr_establish(IPL_SOFTNET, eca_gotframe, sc);
	sc->sc_tx_soft = softintr_establish(IPL_SOFTNET, eca_txdone, sc);
	if (sc->sc_rx_soft == NULL || sc->sc_tx_soft == NULL) {
		printf("\n%s: failed to establish software interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	if_attach(ifp);
	eco_ifattach(ifp, myaddr);

	printf("\n");
}

static int
eca_init(struct ifnet *ifp)
{
	struct eca_softc *sc = ifp->if_softc;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int sr1, sr2;
	int err;

	if ((err = eco_init(ifp)) != 0)
		return err;

	/* Claim the FIQ early, in case we don't get it. */
	if (fiq_claim(eca_fiqhandler, eca_efiqhandler - eca_fiqhandler))
		return EBUSY;

	if (sc->sc_rcvmbuf == NULL) {
		err = eca_init_rxbuf(sc, M_WAIT);
		if (err != 0)
			return err;
	}

	sc->sc_transmitting = 0;

	/* Interrupts disabled, no DMA, hold Tx and Rx in reset. */
	sc->sc_cr1 = MC6854_CR1_RX_RS | MC6854_CR1_TX_RS;
	/* 1-byte transfers, mark idle. */
	sc->sc_cr2 = 0;
	/* Nothing exciting. */
	sc->sc_cr3 = 0;
	/* single flag, 8 data bits, NRZ */
	sc->sc_cr4 = MC6854_CR4_TX_WL_8BITS | MC6854_CR4_RX_WL_8BITS;

	bus_space_write_1(iot, ioh, MC6854_CR1, sc->sc_cr1);
	bus_space_write_1(iot, ioh, MC6854_CR2, sc->sc_cr2);
	bus_space_write_1(iot, ioh, MC6854_CR1, sc->sc_cr1 | MC6854_CR1_AC);
	bus_space_write_1(iot, ioh, MC6854_CR3, sc->sc_cr3);
	bus_space_write_1(iot, ioh, MC6854_CR4, sc->sc_cr4);
	bus_space_write_1(iot, ioh, MC6854_CR1, sc->sc_cr1);

	/* Everything's set up.  Take chip out of reset. */
	sc->sc_cr1 &= ~(MC6854_CR1_RX_RS | MC6854_CR1_TX_RS);
	bus_space_write_1(iot, ioh, MC6854_CR1, sc->sc_cr1);

	/* Read and clear status registers. */
	sr1 = bus_space_read_1(iot, ioh, MC6854_SR1);
	sr2 = bus_space_read_1(iot, ioh, MC6854_SR2);
	bus_space_write_1(iot, ioh, MC6854_CR2, sc->sc_cr2 | 
		MC6854_CR2_CLR_RX_ST | MC6854_CR2_CLR_TX_ST);

	/* Set up FIQ registers and enable FIQs */
	eca_init_rx(sc);

	ifp->if_flags |= IFF_RUNNING;

	return 0;
}

/*
 * Check if the network's idle, and if it is, start flag-filling.
 */
static int
eca_claimwire(struct ifnet *ifp)
{
	struct eca_softc *sc = ifp->if_softc;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	if (bus_space_read_1(iot, ioh, MC6854_SR2) & MC6854_SR2_RX_IDLE) {
		/* Start flag fill. */
		sc->sc_cr2 |= MC6854_CR2_RTS | MC6854_CR2_F_M_IDLE;
		bus_space_write_1(iot, ioh, MC6854_CR2, sc->sc_cr2);
		return 0;
	}
	return EBUSY;
}

static void
eca_txframe(struct ifnet *ifp, struct mbuf *m)
{
	struct eca_softc *sc = ifp->if_softc;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct fiq_regs fr;

	ioc_fiq_setmask(0);
	/* Start flag-filling while we work out what to do next. */
	sc->sc_cr2 |= MC6854_CR2_RTS | MC6854_CR2_F_M_IDLE;
	bus_space_write_1(iot, ioh, MC6854_CR2, sc->sc_cr2);
	sc->sc_fiqstate.efs_fiqhandler = eca_fiqhandler_tx;
	fiq_installhandler(eca_fiqhandler, eca_efiqhandler - eca_fiqhandler);
	sc->sc_transmitting = 1;
	sc->sc_txmbuf = m;
	fr.r8_fiq = (register_t)sc->sc_ioh.a1;
	fr.r9_fiq = (register_t)sc->sc_txmbuf->m_data;
	fr.r10_fiq = (register_t)sc->sc_txmbuf->m_len;
	fr.r11_fiq = (register_t)&sc->sc_fiqstate;
	fiq_setregs(&fr);
	sc->sc_fiqstate.efs_tx_curmbuf = sc->sc_txmbuf;
	fiq_downgrade_handler = eca_tx_downgrade;
	/* Read and clear Tx status. */
	bus_space_read_1(iot, ioh, MC6854_SR1);
	bus_space_write_1(iot, ioh, MC6854_CR2,
	    sc->sc_cr2 | MC6854_CR2_CLR_TX_ST);
	sc->sc_cr1 = MC6854_CR1_TIE;
	bus_space_write_1(iot, ioh, MC6854_CR1, sc->sc_cr1 |
	    MC6854_CR1_DISCONTINUE);
	ioc_fiq_setmask(IOC_FIQ_BIT(FIQ_EFIQ));
}

static void
eca_tx_downgrade(void)
{
	struct eca_softc *sc = eca_fiqowner;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int sr1;
	char buf[128];

	KASSERT(sc->sc_transmitting);
	sc->sc_cr2 = 0;
	if (__predict_true(sc->sc_fiqstate.efs_tx_curmbuf == NULL)) {
		/* Entire frame got transmitted. */
	} else {
		sr1 = bus_space_read_1(iot, ioh, MC6854_SR1);
		if (sr1 & MC6854_SR1_TXU)
			log(LOG_ERR, "%s: Tx underrun\n", sc->sc_dev.dv_xname);
		else if (sr1 & MC6854_SR1_NCTS)
			log(LOG_WARNING, "%s: collision\n",
			    sc->sc_dev.dv_xname);
		else {
			log(LOG_ERR, "%s: incomplete transmission\n",
			    sc->sc_dev.dv_xname);
			bitmask_snprintf(sr1, MC6854_SR1_BITS, buf, 128);
			log(LOG_ERR, "%s: SR1 = %s\n",
			    sc->sc_dev.dv_xname, buf);
		}
		bus_space_write_1(iot, ioh, MC6854_CR2,
		    sc->sc_cr2 | MC6854_CR2_CLR_TX_ST);
	}
	sc->sc_txmbuf = NULL;
	eca_init_rx(sc);
	softintr_schedule(sc->sc_tx_soft);
}

/*
 * Low-priority soft interrupt taken after a frame's been transmitted.
 */
static void
eca_txdone(void *arg)
{
	struct eca_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ec.ec_if;

	m_freem(sc->sc_txmbuf);
	sc->sc_txmbuf = NULL;
	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_start(ifp);
}

/*
 * Set up the Rx buffer for the interface.
 * flags is M_WAIT or M_DONTWAIT.  Returns an errno or 0 for success.
 */
static int
eca_init_rxbuf(struct eca_softc *sc, int flags)
{
	struct mbuf *m, *n;
	size_t totlen;

	/*
	 * The Rx buffer takes the form of a chain of mbufs, set up as
	 * if they contain data already.  The FIQ handler is
	 * responsible for filling the marked space, and indicating
	 * how much it's put in there.
	 */

	totlen = 0;
	n = NULL;
	while (totlen < sc->sc_ec.ec_if.if_mtu + ECO_HDR_LEN) {
		MGETHDR(m, flags, MT_DATA);
		if (m == NULL)
			return ENOBUFS;
		MCLGET(m, flags);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			return ENOBUFS;
		}
		/* XXX may want to tweak for payload alignment here. */
		m->m_len = m->m_ext.ext_size;
		m->m_next = n;
		totlen += m->m_len;
		n = m;
	}

	sc->sc_rcvmbuf = m;
	return 0;
}

/*
 * Set up the chip and FIQ handler for reception.  Assumes the Rx buffer is
 * set up already by eca_init_rxbuf().
 */
void
eca_init_rx(struct eca_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct fiq_regs fr;
	int sr2;

	sr2 = bus_space_read_1(iot, ioh, MC6854_SR2);
	sc->sc_fiqstate.efs_fiqhandler = eca_fiqhandler_rx;
	fiq_installhandler(eca_fiqhandler, eca_efiqhandler - eca_fiqhandler);
	sc->sc_transmitting = 0;
	sc->sc_cr1 = MC6854_CR1_RIE;
	bus_space_write_1(iot, ioh, MC6854_CR1, sc->sc_cr1);
	memset(&fr, 0, sizeof(fr));
	fr.r8_fiq = (register_t)sc->sc_ioh.a1;
	fr.r9_fiq = (register_t)sc->sc_rcvmbuf->m_data;
	fr.r10_fiq = (register_t)ECO_ADDR_LEN;
	fr.r11_fiq = (register_t)&sc->sc_fiqstate;
	sc->sc_fiqstate.efs_rx_curmbuf = sc->sc_rcvmbuf;
	sc->sc_fiqstate.efs_rx_flags = 0;
	sc->sc_fiqstate.efs_rx_myaddr = LLADDR(ifp->if_sadl)[0];
	fiq_setregs(&fr);
	fiq_downgrade_handler = eca_rx_downgrade;
	eca_fiqowner = sc;
	sr2 = bus_space_read_1(iot, ioh, MC6854_SR2);
	ioc_fiq_setmask(IOC_FIQ_BIT(FIQ_EFIQ));
}

static void
eca_stop(struct ifnet *ifp, int disable)
{
	struct eca_softc *sc = ifp->if_softc;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	ifp->if_flags &= ~IFF_RUNNING;

	eco_stop(ifp, disable);

	/* Interrupts disabled, no DMA, hold Tx and Rx in reset. */
	sc->sc_cr1 = MC6854_CR1_RX_RS | MC6854_CR1_TX_RS;
	bus_space_write_1(iot, ioh, MC6854_CR1, sc->sc_cr1);

	ioc_fiq_setmask(0);
	fiq_downgrade_handler = NULL;
	eca_fiqowner = NULL;
	fiq_release();
	if (sc->sc_rcvmbuf != NULL) {
		m_freem(sc->sc_rcvmbuf);
		sc->sc_rcvmbuf = NULL;
	}
}

/*
 * This is a FIQ downgrade handler, and as such is entered at IPL_HIGH with
 * FIQs disabled (but still owned by us).
 */
static void
eca_rx_downgrade(void)
{
	struct eca_softc *sc = eca_fiqowner;

	sc->sc_sr2 = bus_space_read_1(sc->sc_iot, sc->sc_ioh, MC6854_SR2);
	softintr_schedule(sc->sc_rx_soft);
}

/*
 * eca_gotframe() is called if there's something interesting on
 * reception.  The receiver is turned off now -- it's up to us to
 * start it again.
 */
static void
eca_gotframe(void *arg)
{
	struct eca_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct fiq_regs fr;
	int sr2;
	struct mbuf *m, *mtail, *n, *reply;

	reply = NULL;
	sr2 = sc->sc_sr2;

	/* 1: Is there a valid frame waiting? */
	if ((sr2 & MC6854_SR2_FV) && !(sr2 & MC6854_SR2_OVRN) &&
	    sc->sc_fiqstate.efs_rx_curmbuf != NULL) {
		fiq_getregs(&fr);
		m = sc->sc_rcvmbuf;
		mtail = sc->sc_fiqstate.efs_rx_curmbuf;
		/*
		 * Before we process this buffer, make sure we can get
		 * a new one.
		 */
		if (eca_init_rxbuf(sc, M_DONTWAIT) == 0) {
			ifp->if_ipackets++; /* XXX packet vs frame? */
			/* Trim the tail of the mbuf chain. */
			mtail->m_len = (caddr_t)(fr.r9_fiq) - mtail->m_data;
			m_freem(mtail->m_next);
			mtail->m_next = NULL;
			/* Set up the header of the chain. */
			m->m_pkthdr.rcvif = ifp;
			m->m_pkthdr.len = 0;
			for (n = m; n != NULL; n = n->m_next)
				m->m_pkthdr.len += n->m_len;
			reply = eco_inputframe(ifp, m);
		} else
			ifp->if_iqdrops++;
	}

	/* 2: Are there any errors waiting? */
	if (sr2 & MC6854_SR2_OVRN) {
		log(LOG_ERR, "%s: Rx overrun\n", sc->sc_dev.dv_xname);
		ifp->if_ierrors++;

		/* Discard the rest of the frame. */
		if (!sc->sc_transmitting)
			bus_space_write_1(iot, ioh, MC6854_CR1,
			    sc->sc_cr1 | MC6854_CR1_DISCONTINUE);
	} else if (sr2 & MC6854_SR2_RXABT) {
		log(LOG_NOTICE, "%s: Rx abort\n", sc->sc_dev.dv_xname);
		ifp->if_ierrors++;
	} else if (sr2 & MC6854_SR2_ERR) {
		log(LOG_NOTICE, "%s: CRC error\n", sc->sc_dev.dv_xname);
		ifp->if_ierrors++;
	}

	if (__predict_false(sr2 & MC6854_SR2_NDCD)) {
		log(LOG_ERR, "%s: No clock\n", sc->sc_dev.dv_xname);
		ifp->if_ierrors++;
	}
	if (sc->sc_fiqstate.efs_rx_curmbuf == NULL) {
		log(LOG_NOTICE, "%s: Oversized frame\n", sc->sc_dev.dv_xname);
		ifp->if_ierrors++;
		/* Discard the rest of the frame. */
		if (!sc->sc_transmitting)
			bus_space_write_1(iot, ioh, MC6854_CR1,
			    sc->sc_cr1 | MC6854_CR1_DISCONTINUE);
	}


	bus_space_write_1(iot, ioh, MC6854_CR2,
	    sc->sc_cr2 | MC6854_CR2_CLR_RX_ST);

	if (reply) {
		KASSERT(sc->sc_fiqstate.efs_rx_flags & ERXF_FLAGFILL);
		eca_txframe(ifp, reply);
	} else {
		KASSERT(!sc->sc_transmitting);
		if (sc->sc_fiqstate.efs_rx_flags & ERXF_FLAGFILL)
			/* Stop flag-filling: we have nothing to send. */
			bus_space_write_1(iot, ioh, MC6854_CR2,
			    sc->sc_cr2);
		/* Set the ADLC up to receive the next frame. */
		eca_init_rx(sc);
		/* 3: Is the network idle now? */
		if (sr2 & MC6854_SR2_RX_IDLE)
			eco_inputidle(ifp);
	}
}
