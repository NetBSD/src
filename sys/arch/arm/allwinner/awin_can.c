/*	$NetBSD: awin_can.c,v 1.3.6.2 2017/12/03 11:35:50 jdolecek Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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


#include "locators.h"
#include "opt_can.h"


#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: awin_can.c,v 1.3.6.2 2017/12/03 11:35:50 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/ioctl.h>
#include <sys/mutex.h>
#include <sys/rndsource.h>
#include <sys/mbuf.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/bpf.h>

#ifdef CAN
#include <netcan/can.h>
#include <netcan/can_var.h>
#endif

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

/* shortcut for all error interrupts */
#define AWIN_CAN_INT_ALLERRS (\
	AWIN_CAN_INT_BERR | \
	AWIN_CAN_INT_ARB_LOST | \
	AWIN_CAN_INT_ERR_PASSIVE | \
	AWIN_CAN_INT_DATA_OR | \
	AWIN_CAN_INT_ERR \
    )

struct awin_can_softc {
	struct canif_softc sc_cansc;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	kmutex_t sc_intr_lock;
	void *sc_ih;
	struct ifnet *sc_ifp;
	krndsource_t sc_rnd_source;	/* random source */
	struct mbuf *sc_m_transmit; /* mbuf being transmitted */
};
#define sc_dev		sc_cansc.csc_dev
#define sc_timecaps	sc_cansc.csc_timecaps
#define sc_timings	sc_cansc.csc_timings
#define sc_linkmodes	sc_cansc.csc_linkmodes

static int awin_can_match(device_t, cfdata_t, void *);
static void awin_can_attach(device_t, device_t, void *);

static int awin_can_intr(void *);

static void awin_can_ifstart(struct ifnet *);
static int awin_can_ifioctl(struct ifnet *, u_long, void *);
static void awin_can_ifwatchdog(struct ifnet *);

static void awin_can_enter_reset(struct awin_can_softc *);
static void awin_can_exit_reset(struct awin_can_softc *);

CFATTACH_DECL_NEW(awin_can, sizeof(struct awin_can_softc),
	awin_can_match, awin_can_attach, NULL, NULL);

static const struct awin_gpio_pinset awin_can_pinsets[2] = {
        [0] = { 'A', AWIN_PIO_PA_CAN_FUNC, AWIN_PIO_PA_CAN_PINS },
        [1] = { 'H', AWIN_PIO_PH_CAN_FUNC, AWIN_PIO_PH_CAN_PINS },
};

static inline uint32_t
awin_can_read(struct awin_can_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
}

static inline void
awin_can_write(struct awin_can_softc *sc, bus_size_t o, uint32_t v)
{
	return bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, v);
}

static int
awin_can_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio __diagused = aux;
	const struct awin_locators * const loc __diagused = &aio->aio_loc;
	const struct awin_gpio_pinset * const pinset =
	    &awin_can_pinsets[cf->cf_flags & 1];

	KASSERT(!strcmp(cf->cf_name, loc->loc_name));
	KASSERT(cf->cf_loc[AWINIOCF_PORT] == AWINIOCF_PORT_DEFAULT
	    || cf->cf_loc[AWINIOCF_PORT] == loc->loc_port);

	if (!awin_gpio_pinset_available(pinset)) {
		return 0;
	}

	return 1;
}

static void
awin_can_attach(device_t parent, device_t self, void *aux)
{
	cfdata_t cf = device_cfdata(self);
	struct awin_can_softc * const sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	const struct awin_gpio_pinset * const pinset =
	    &awin_can_pinsets[cf->cf_flags & 1];
	struct ifnet *ifp;

	sc->sc_dev = self;

	awin_gpio_pinset_acquire(pinset);
	awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_APB1_GATING_REG, AWIN_APB_GATING1_CAN, 0);

	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_NET);

	sc->sc_bst = aio->aio_core_bst;
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	sc->sc_timecaps.cltc_prop_min = 0;
	sc->sc_timecaps.cltc_prop_max = 0;
	sc->sc_timecaps.cltc_ps1_min = 1;
	sc->sc_timecaps.cltc_ps1_max = 16;
	sc->sc_timecaps.cltc_ps2_min = 1;
	sc->sc_timecaps.cltc_ps2_max = 8;
	sc->sc_timecaps.cltc_sjw_max = 4;
	sc->sc_timecaps.cltc_brp_min = 1;
	sc->sc_timecaps.cltc_brp_max = 64;
	sc->sc_timecaps.cltc_brp_inc = 1;
	sc->sc_timecaps.cltc_clock_freq = AWIN_REF_FREQ;
	sc->sc_timecaps.cltc_linkmode_caps =
	    CAN_LINKMODE_3SAMPLES | CAN_LINKMODE_LISTENONLY |
	    CAN_LINKMODE_LOOPBACK;
	can_ifinit_timings(&sc->sc_cansc);
	sc->sc_timings.clt_prop = 0;
	sc->sc_timings.clt_sjw = 1;

	aprint_naive("\n");
	aprint_normal(": CAN bus controller\n");

	awin_can_enter_reset(sc);
	/*
	 * Disable and then clear all interrupts
	 */
	awin_can_write(sc, AWIN_CAN_INTE_REG, 0);
	awin_can_write(sc, AWIN_CAN_INT_REG,
	    awin_can_read(sc, AWIN_CAN_INT_REG));

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_NET, IST_LEVEL,
	    awin_can_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

	ifp = if_alloc(IFT_OTHER);
	sc->sc_ifp = ifp;
	strlcpy(ifp->if_xname, device_xname(self), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_capabilities = 0;
	ifp->if_flags = 0;
	ifp->if_start = awin_can_ifstart;
	ifp->if_ioctl = awin_can_ifioctl;
	ifp->if_watchdog = awin_can_ifwatchdog;

	/*      
	 * Attach the interface.
	 */
	can_ifattach(ifp);
	if_deferred_start_init(ifp, NULL);
	bpf_mtap_softint_init(ifp);
	rnd_attach_source(&sc->sc_rnd_source, device_xname(self),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);
#ifdef MBUFTRACE
	ifp->if_mowner = malloc(sizeof(struct mowner), M_DEVBUF,
	    M_WAITOK | M_ZERO);
	strlcpy(ifp->if_mowner->mo_name, ifp->if_xname,
		sizeof(ifp->if_mowner->mo_name));
	MOWNER_ATTACH(ifp->if_mowner);
#endif
}

static void
awin_can_rx_intr(struct awin_can_softc *sc)
{
	uint32_t reg0v;
	struct mbuf *m;
	struct ifnet  *ifp = sc->sc_ifp;
	struct can_frame *cf;
	int dlc;
	int regd, i;

	KASSERT(mutex_owned(&sc->sc_intr_lock));
	reg0v = awin_can_read(sc, AWIN_CAN_TXBUF0_REG);
	dlc = reg0v & AWIN_CAN_TXBUF0_DL;

	if (dlc > CAN_MAX_DLC) {
		ifp->if_ierrors++;
		awin_can_write(sc, AWIN_CAN_CMD_REG, AWIN_CAN_CMD_REL_RX_BUF);
		return;
	}
		
	m = m_gethdr(M_NOWAIT, MT_HEADER);
	if (m == NULL) {
		ifp->if_ierrors++;
		awin_can_write(sc, AWIN_CAN_CMD_REG, AWIN_CAN_CMD_REL_RX_BUF);
		return;
	}
	cf = mtod(m, struct can_frame *);
	memset(cf, 0, sizeof(struct can_frame));

	cf->can_dlc = dlc;

	if (reg0v & AWIN_CAN_TXBUF0_EFF) {
		cf->can_id = 
		    (awin_can_read(sc, AWIN_CAN_TXBUF1_REG) << 21) |
		    (awin_can_read(sc, AWIN_CAN_TXBUF2_REG) << 13) |
		    (awin_can_read(sc, AWIN_CAN_TXBUF3_REG) << 5) |
		    ((awin_can_read(sc, AWIN_CAN_TXBUF4_REG) >> 3) & 0x1f);
		cf->can_id |= CAN_EFF_FLAG;
		regd = AWIN_CAN_TXBUF5_REG;
	} else {
		cf->can_id = 
		    (awin_can_read(sc, AWIN_CAN_TXBUF1_REG) << 3) |
		    ((awin_can_read(sc, AWIN_CAN_TXBUF2_REG) << 5) & 0x7);
		regd = AWIN_CAN_TXBUF3_REG;
	}
	if (reg0v & AWIN_CAN_TXBUF0_RTR) {
		cf->can_id |= CAN_RTR_FLAG; 
	} else {
		for (i = 0; i < cf->can_dlc; i++) {
			cf->data[i] = awin_can_read(sc, regd + i * 4);
		}
	}
	awin_can_write(sc, AWIN_CAN_CMD_REG, AWIN_CAN_CMD_REL_RX_BUF);
	m->m_len = m->m_pkthdr.len = CAN_MTU;
	ifp->if_ipackets++;
	ifp->if_ibytes += m->m_len;
	m_set_rcvif(m, ifp);
	can_bpf_mtap(ifp, m, 1);
	can_input(ifp, m);
}

static void
awin_can_tx_intr(struct awin_can_softc *sc)
{
	struct ifnet * const ifp = sc->sc_ifp;
	struct mbuf *m;

	KASSERT(mutex_owned(&sc->sc_intr_lock));
	if ((m = sc->sc_m_transmit) != NULL) {
		ifp->if_obytes += m->m_len;
		ifp->if_opackets++;
		can_mbuf_tag_clean(m);
		m_set_rcvif(m, ifp);
		can_input(ifp, m); /* loopback */
		sc->sc_m_transmit = NULL;
		ifp->if_timer = 0;
	}
	ifp->if_flags &= ~IFF_OACTIVE;
	if_schedule_deferred_start(ifp);
}

static int
awin_can_tx_abort(struct awin_can_softc *sc)
{
	KASSERT(mutex_owned(&sc->sc_intr_lock));
	if (sc->sc_m_transmit) {
		m_freem(sc->sc_m_transmit);
		sc->sc_m_transmit = NULL;
		sc->sc_ifp->if_timer = 0;
		/*
		 * the transmit abort will trigger a TX interrupt
		 * which will restart the queue or cleae OACTIVE,
		 * as appropriate
		 */
		awin_can_write(sc, AWIN_CAN_CMD_REG, AWIN_CAN_CMD_ABT_REQ);
		return 1;
	}
	return 0;
}

static void
awin_can_err_intr(struct awin_can_softc *sc, uint32_t irq, uint32_t sts)
{
	struct ifnet * const ifp = sc->sc_ifp;
	KASSERT(mutex_owned(&sc->sc_intr_lock));
	int txerr = 0;
	uint32_t reg;

	if (irq & AWIN_CAN_INT_DATA_OR) {
		ifp->if_ierrors++;
		awin_can_write(sc, AWIN_CAN_CMD_REG, AWIN_CAN_CMD_CLR_OR);
	}
	if (irq & AWIN_CAN_INT_ERR) {
		reg = awin_can_read(sc, AWIN_CAN_REC_REG);
		printf("%s: ERR interrupt status 0x%x counters 0x%x\n",
		    device_xname(sc->sc_dev), sts, reg);

	}
	if (irq & AWIN_CAN_INT_BERR) {
		if (sts & AWIN_CAN_STA_TX)
			txerr++;
		if (sts & AWIN_CAN_STA_RX)
			ifp->if_ierrors++;
	}
	if (irq & AWIN_CAN_INT_ERR_PASSIVE) {
		printf("%s: PASSV interrupt status 0x%x\n",
		    device_xname(sc->sc_dev), sts);
	}
	if (irq & AWIN_CAN_INT_ARB_LOST) {
		txerr++;
	}
	if (txerr) {
		ifp->if_oerrors += txerr;
		(void) awin_can_tx_abort(sc);
	}
}

int
awin_can_intr(void *arg)
{
	struct awin_can_softc * const sc = arg;
	int rv = 0;
	int irq;

	mutex_enter(&sc->sc_intr_lock);

	while ((irq = awin_can_read(sc, AWIN_CAN_INT_REG)) != 0) {
		uint32_t sts = awin_can_read(sc, AWIN_CAN_STA_REG);
		rv = 1;

		if (irq & AWIN_CAN_INT_TX_FLAG) {
			awin_can_tx_intr(sc);
		}
		if (irq & AWIN_CAN_INT_RX_FLAG) {
			while (sts & AWIN_CAN_STA_RX_RDY) {
				awin_can_rx_intr(sc);
				sts = awin_can_read(sc, AWIN_CAN_STA_REG);
			}
		}
		if (irq & AWIN_CAN_INT_ALLERRS) {
			awin_can_err_intr(sc, irq, sts);
		}
		awin_can_write(sc, AWIN_CAN_INT_REG, irq);
                rnd_add_uint32(&sc->sc_rnd_source, irq);

	}
	mutex_exit(&sc->sc_intr_lock);

	return rv;
}

void
awin_can_ifstart(struct ifnet *ifp)
{
	struct awin_can_softc * const sc = ifp->if_softc;
	struct mbuf *m;
	struct can_frame *cf;
	int regd;
	uint32_t reg0val;
	int i;

	mutex_enter(&sc->sc_intr_lock);
	if (ifp->if_flags & IFF_OACTIVE)
		goto out;

	IF_DEQUEUE(&ifp->if_snd, m);

	if (m == NULL)
		goto out;

	MCLAIM(m, ifp->if_mowner);
	sc->sc_m_transmit = m;

	KASSERT((m->m_flags & M_PKTHDR) != 0);
	KASSERT(m->m_len == m->m_pkthdr.len);

	cf = mtod(m, struct can_frame *);
	reg0val = cf->can_dlc & AWIN_CAN_TXBUF0_DL;
	if (cf->can_id & CAN_RTR_FLAG)
		reg0val |= AWIN_CAN_TXBUF0_RTR;

	if (cf->can_id & CAN_EFF_FLAG) {
		reg0val |= AWIN_CAN_TXBUF0_EFF;
		awin_can_write(sc, AWIN_CAN_TXBUF1_REG,
		    (cf->can_id >> 21) & 0xff);
		awin_can_write(sc, AWIN_CAN_TXBUF2_REG,
		    (cf->can_id >> 13) & 0xff);
		awin_can_write(sc, AWIN_CAN_TXBUF3_REG,
		    (cf->can_id >> 5) & 0xff);
		awin_can_write(sc, AWIN_CAN_TXBUF4_REG,
		    (cf->can_id << 3) & 0xf8);
		regd = AWIN_CAN_TXBUF5_REG;
	} else {
		awin_can_write(sc, AWIN_CAN_TXBUF1_REG,
		    (cf->can_id >> 3) & 0xff);
		awin_can_write(sc, AWIN_CAN_TXBUF2_REG,
		    (cf->can_id << 5) & 0xe0);
		regd = AWIN_CAN_TXBUF3_REG;
	}

	for (i = 0; i < cf->can_dlc; i++) {
		awin_can_write(sc, regd + i * 4, cf->data[i]);
	}
	awin_can_write(sc, AWIN_CAN_TXBUF0_REG, reg0val);

	if (sc->sc_linkmodes & CAN_LINKMODE_LOOPBACK) {
		awin_can_write(sc, AWIN_CAN_CMD_REG,
			AWIN_CAN_CMD_TANS_REQ | AWIN_CAN_CMD_SELF_REQ);
	} else {
		awin_can_write(sc, AWIN_CAN_CMD_REG, AWIN_CAN_CMD_TANS_REQ);
	}
	ifp->if_flags |= IFF_OACTIVE;
	ifp->if_timer = 5;
	can_bpf_mtap(ifp, m, 0);
out:
	mutex_exit(&sc->sc_intr_lock);
}

static int
awin_can_ifup(struct awin_can_softc * const sc)
{
	uint32_t reg;

	/* setup timings and mode - has to be done in reset */
	reg = AWIN_CAN_MODSEL_RST;
	if (sc->sc_linkmodes & CAN_LINKMODE_LISTENONLY)
		reg |= AWIN_CAN_MODSEL_LST_ONLY;

	if (sc->sc_linkmodes & CAN_LINKMODE_LOOPBACK)
		reg |= AWIN_CAN_MODSEL_LB_MOD;

	awin_can_write(sc, AWIN_CAN_MODSEL_REG, reg);

	reg = 0;
	if (sc->sc_timings.clt_prop != 0)
		return EINVAL;

	if (sc->sc_timings.clt_brp > sc->sc_timecaps.cltc_brp_max ||
	   sc->sc_timings.clt_brp < sc->sc_timecaps.cltc_brp_min)
		return EINVAL;
	reg |= (sc->sc_timings.clt_brp - 1) << 0;

	if (sc->sc_timings.clt_ps1 > sc->sc_timecaps.cltc_ps1_max ||
	   sc->sc_timings.clt_ps1 < sc->sc_timecaps.cltc_ps1_min)
		return EINVAL;
	reg |= (sc->sc_timings.clt_ps1 - 1) << 16;

	if (sc->sc_timings.clt_ps2 > sc->sc_timecaps.cltc_ps2_max ||
	   sc->sc_timings.clt_ps2 < sc->sc_timecaps.cltc_ps2_min)
		return EINVAL;
	reg |= (sc->sc_timings.clt_ps2 - 1) << 20;

	if (sc->sc_timings.clt_sjw > sc->sc_timecaps.cltc_sjw_max ||
	   sc->sc_timings.clt_sjw < 1)
		return EINVAL;
	reg |= (sc->sc_timings.clt_sjw - 1) << 14;

	if (sc->sc_linkmodes & CAN_LINKMODE_3SAMPLES)
		reg |= AWIN_CAN_BUS_TIME_SAM;

	awin_can_write(sc, AWIN_CAN_BUS_TIME_REG, reg);

	/* set filters to accept all frames */
	awin_can_write(sc, AWIN_CAN_ACPC, 0x00000000);
	awin_can_write(sc, AWIN_CAN_ACPM, 0xffffffff);

	/* clear errors counter */
	awin_can_write(sc, AWIN_CAN_REC_REG, 0);

	/* leave reset mode and enable interrupts */
	awin_can_exit_reset(sc);
	awin_can_write(sc, AWIN_CAN_INTE_REG,
	    AWIN_CAN_INT_TX_FLAG | AWIN_CAN_INT_RX_FLAG | AWIN_CAN_INT_ALLERRS);
	sc->sc_ifp->if_flags |= IFF_RUNNING;
	return 0;
}

static void
awin_can_ifdown(struct awin_can_softc * const sc)
{
	sc->sc_ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	sc->sc_ifp->if_timer = 0;
	awin_can_enter_reset(sc);
	awin_can_write(sc, AWIN_CAN_INTE_REG, 0);
	awin_can_write(sc, AWIN_CAN_INT_REG,
	    awin_can_read(sc, AWIN_CAN_INT_REG));
}

static int
awin_can_ifioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct awin_can_softc * const sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int error = 0;

	mutex_enter(&sc->sc_intr_lock);

	switch (cmd) {
	case SIOCINITIFADDR:
		error = EAFNOSUPPORT;
		break;
	case SIOCSIFMTU:
		if ((unsigned)ifr->ifr_mtu != sizeof(struct can_frame))
			error = EINVAL;
		break;
	case SIOCADDMULTI: 
	case SIOCDELMULTI:     
		error = EAFNOSUPPORT;
		break;
	default:
		error = ifioctl_common(ifp, cmd, data);
		if (error == 0) {
			if ((ifp->if_flags & IFF_UP) != 0 &&
			    (ifp->if_flags & IFF_RUNNING) == 0) {
				error = awin_can_ifup(sc);
				if (error) {
					ifp->if_flags &= ~IFF_UP;
				}
			} else if ((ifp->if_flags & IFF_UP) == 0 &&
			    (ifp->if_flags & IFF_RUNNING) != 0) {
				awin_can_ifdown(sc);
			}
		}
		break;
	}

	mutex_exit(&sc->sc_intr_lock);
	return error;
}

void
awin_can_ifwatchdog(struct ifnet *ifp)
{
	struct awin_can_softc * const sc = ifp->if_softc;
	printf("%s: watchdog timeout\n", device_xname(sc->sc_dev));

	mutex_enter(&sc->sc_intr_lock);
	printf("irq 0x%x en 0x%x mode 0x%x status 0x%x timings 0x%x err 0x%x\n",
	    awin_can_read(sc, AWIN_CAN_INT_REG),
	    awin_can_read(sc, AWIN_CAN_INTE_REG),
	    awin_can_read(sc, AWIN_CAN_MODSEL_REG),
	    awin_can_read(sc, AWIN_CAN_STA_REG),
	    awin_can_read(sc, AWIN_CAN_BUS_TIME_REG),
	    awin_can_read(sc, AWIN_CAN_REC_REG));
	/* if there is a transmit in progress abort */
	if (awin_can_tx_abort(sc)) {
		ifp->if_oerrors++;
	}
	mutex_exit(&sc->sc_intr_lock);
}

static void
awin_can_enter_reset(struct awin_can_softc *sc)
{
	int i;
	uint32_t val;

	for (i = 0; i < 1000; i++) {
		val = awin_can_read(sc, AWIN_CAN_MODSEL_REG);
		val |= AWIN_CAN_MODSEL_RST;
		awin_can_write(sc, AWIN_CAN_MODSEL_REG, val);
		val = awin_can_read(sc, AWIN_CAN_MODSEL_REG);
		if (val & AWIN_CAN_MODSEL_RST)
			return;
	}
	printf("%s: couldn't enter reset mode\n", device_xname(sc->sc_dev));
}

static void
awin_can_exit_reset(struct awin_can_softc *sc)
{
	int i;
	uint32_t val;

	for (i = 0; i < 1000; i++) {
		val = awin_can_read(sc, AWIN_CAN_MODSEL_REG);
		val &= ~AWIN_CAN_MODSEL_RST;
		awin_can_write(sc, AWIN_CAN_MODSEL_REG, val);
		val = awin_can_read(sc, AWIN_CAN_MODSEL_REG);
		if ((val & AWIN_CAN_MODSEL_RST) == 0)
			return;
	}
	printf("%s: couldn't leave reset mode\n", device_xname(sc->sc_dev));
}
