/*	$NetBSD: sunxi_can.c,v 1.1 2018/03/07 20:55:31 bouyer Exp $	*/

/*-
 * Copyright (c) 2017,2018 The NetBSD Foundation, Inc.
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

__KERNEL_RCSID(1, "$NetBSD: sunxi_can.c,v 1.1 2018/03/07 20:55:31 bouyer Exp $");

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

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_can.h>

/* shortcut for all error interrupts */
#define SUNXI_CAN_INT_ALLERRS (\
	SUNXI_CAN_INT_BERR | \
	SUNXI_CAN_INT_ARB_LOST | \
	SUNXI_CAN_INT_ERR_PASSIVE | \
	SUNXI_CAN_INT_DATA_OR | \
	SUNXI_CAN_INT_ERR \
    )

struct sunxi_can_softc {
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

static const struct of_compat_data compat_data[] = {
	{"allwinner,sun4i-a10-can", 0},
	{NULL}
};

static int sunxi_can_match(device_t, cfdata_t, void *);
static void sunxi_can_attach(device_t, device_t, void *);

static int sunxi_can_intr(void *);

static void sunxi_can_ifstart(struct ifnet *);
static int sunxi_can_ifioctl(struct ifnet *, u_long, void *);
static void sunxi_can_ifwatchdog(struct ifnet *);

static void sunxi_can_enter_reset(struct sunxi_can_softc *);
static void sunxi_can_exit_reset(struct sunxi_can_softc *);

CFATTACH_DECL_NEW(sunxi_can, sizeof(struct sunxi_can_softc),
	sunxi_can_match, sunxi_can_attach, NULL, NULL);

static inline uint32_t
sunxi_can_read(struct sunxi_can_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
}

static inline void
sunxi_can_write(struct sunxi_can_softc *sc, bus_size_t o, uint32_t v)
{
	return bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, v);
}

static int
sunxi_can_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_can_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_can_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct ifnet *ifp;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	char intrstr[128];
	struct clk *clk;
	struct fdtbus_reset *rst;

	sc->sc_dev = self;
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_NET);

	sc->sc_bst = faa->faa_bst;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	if ((clk = fdtbus_clock_get_index(phandle, 0)) != NULL) {
		if (clk_enable(clk) != 0) {
			aprint_error(": couldn't enable clock\n");
			return;
		}
	}

	if ((rst = fdtbus_reset_get_index(phandle, 0)) != NULL) {
		if (fdtbus_reset_deassert(rst) != 0) {
			aprint_error(": couldn't de-assert reset\n");
			return;
		}
	}

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
	sc->sc_timecaps.cltc_clock_freq = clk_get_rate(clk);
	sc->sc_timecaps.cltc_linkmode_caps =
	    CAN_LINKMODE_3SAMPLES | CAN_LINKMODE_LISTENONLY |
	    CAN_LINKMODE_LOOPBACK;
	can_ifinit_timings(&sc->sc_cansc);
	sc->sc_timings.clt_prop = 0;
	sc->sc_timings.clt_sjw = 1;

	aprint_naive("\n");
	aprint_normal(": CAN bus controller\n");
	aprint_debug_dev(self, ": clock freq %d\n",
	    sc->sc_timecaps.cltc_clock_freq);

	sunxi_can_enter_reset(sc);
	/*
	 * Disable and then clear all interrupts
	 */
	sunxi_can_write(sc, SUNXI_CAN_INTE_REG, 0);
	sunxi_can_write(sc, SUNXI_CAN_INT_REG,
	    sunxi_can_read(sc, SUNXI_CAN_INT_REG));

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_NET, 0,
	    sunxi_can_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	ifp = if_alloc(IFT_OTHER);
	sc->sc_ifp = ifp;
	strlcpy(ifp->if_xname, device_xname(self), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_capabilities = 0;
	ifp->if_flags = 0;
	ifp->if_start = sunxi_can_ifstart;
	ifp->if_ioctl = sunxi_can_ifioctl;
	ifp->if_watchdog = sunxi_can_ifwatchdog;

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
sunxi_can_rx_intr(struct sunxi_can_softc *sc)
{
	uint32_t reg0v;
	struct mbuf *m;
	struct ifnet  *ifp = sc->sc_ifp;
	struct can_frame *cf;
	int dlc;
	int regd, i;

	KASSERT(mutex_owned(&sc->sc_intr_lock));
	reg0v = sunxi_can_read(sc, SUNXI_CAN_TXBUF0_REG);
	dlc = reg0v & SUNXI_CAN_TXBUF0_DL;

	if (dlc > CAN_MAX_DLC) {
		ifp->if_ierrors++;
		sunxi_can_write(sc, SUNXI_CAN_CMD_REG, SUNXI_CAN_CMD_REL_RX_BUF);
		return;
	}
		
	m = m_gethdr(M_NOWAIT, MT_HEADER);
	if (m == NULL) {
		ifp->if_ierrors++;
		sunxi_can_write(sc, SUNXI_CAN_CMD_REG, SUNXI_CAN_CMD_REL_RX_BUF);
		return;
	}
	cf = mtod(m, struct can_frame *);
	memset(cf, 0, sizeof(struct can_frame));

	cf->can_dlc = dlc;

	if (reg0v & SUNXI_CAN_TXBUF0_EFF) {
		cf->can_id = 
		    (sunxi_can_read(sc, SUNXI_CAN_TXBUF1_REG) << 21) |
		    (sunxi_can_read(sc, SUNXI_CAN_TXBUF2_REG) << 13) |
		    (sunxi_can_read(sc, SUNXI_CAN_TXBUF3_REG) << 5) |
		    ((sunxi_can_read(sc, SUNXI_CAN_TXBUF4_REG) >> 3) & 0x1f);
		cf->can_id |= CAN_EFF_FLAG;
		regd = SUNXI_CAN_TXBUF5_REG;
	} else {
		cf->can_id = 
		    (sunxi_can_read(sc, SUNXI_CAN_TXBUF1_REG) << 3) |
		    ((sunxi_can_read(sc, SUNXI_CAN_TXBUF2_REG) << 5) & 0x7);
		regd = SUNXI_CAN_TXBUF3_REG;
	}
	if (reg0v & SUNXI_CAN_TXBUF0_RTR) {
		cf->can_id |= CAN_RTR_FLAG; 
	} else {
		for (i = 0; i < cf->can_dlc; i++) {
			cf->data[i] = sunxi_can_read(sc, regd + i * 4);
		}
	}
	sunxi_can_write(sc, SUNXI_CAN_CMD_REG, SUNXI_CAN_CMD_REL_RX_BUF);
	m->m_len = m->m_pkthdr.len = CAN_MTU;
	ifp->if_ipackets++;
	ifp->if_ibytes += m->m_len;
	m_set_rcvif(m, ifp);
	can_bpf_mtap(ifp, m, 1);
	can_input(ifp, m);
}

static void
sunxi_can_tx_intr(struct sunxi_can_softc *sc)
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
sunxi_can_tx_abort(struct sunxi_can_softc *sc)
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
		sunxi_can_write(sc, SUNXI_CAN_CMD_REG, SUNXI_CAN_CMD_ABT_REQ);
		return 1;
	}
	return 0;
}

static void
sunxi_can_err_intr(struct sunxi_can_softc *sc, uint32_t irq, uint32_t sts)
{
	struct ifnet * const ifp = sc->sc_ifp;
	KASSERT(mutex_owned(&sc->sc_intr_lock));
	int txerr = 0;
	uint32_t reg;

	if (irq & SUNXI_CAN_INT_DATA_OR) {
		ifp->if_ierrors++;
		sunxi_can_write(sc, SUNXI_CAN_CMD_REG, SUNXI_CAN_CMD_CLR_OR);
	}
	if (irq & SUNXI_CAN_INT_ERR) {
		reg = sunxi_can_read(sc, SUNXI_CAN_REC_REG);
		printf("%s: ERR interrupt status 0x%x counters 0x%x\n",
		    device_xname(sc->sc_dev), sts, reg);

	}
	if (irq & SUNXI_CAN_INT_BERR) {
		if (sts & SUNXI_CAN_STA_TX)
			txerr++;
		if (sts & SUNXI_CAN_STA_RX)
			ifp->if_ierrors++;
	}
	if (irq & SUNXI_CAN_INT_ERR_PASSIVE) {
		printf("%s: PASSV interrupt status 0x%x\n",
		    device_xname(sc->sc_dev), sts);
	}
	if (irq & SUNXI_CAN_INT_ARB_LOST) {
		txerr++;
	}
	if (txerr) {
		ifp->if_oerrors += txerr;
		(void) sunxi_can_tx_abort(sc);
	}
}

int
sunxi_can_intr(void *arg)
{
	struct sunxi_can_softc * const sc = arg;
	int rv = 0;
	int irq;

	mutex_enter(&sc->sc_intr_lock);

	while ((irq = sunxi_can_read(sc, SUNXI_CAN_INT_REG)) != 0) {
		uint32_t sts = sunxi_can_read(sc, SUNXI_CAN_STA_REG);
		rv = 1;

		if (irq & SUNXI_CAN_INT_TX_FLAG) {
			sunxi_can_tx_intr(sc);
		}
		if (irq & SUNXI_CAN_INT_RX_FLAG) {
			while (sts & SUNXI_CAN_STA_RX_RDY) {
				sunxi_can_rx_intr(sc);
				sts = sunxi_can_read(sc, SUNXI_CAN_STA_REG);
			}
		}
		if (irq & SUNXI_CAN_INT_ALLERRS) {
			sunxi_can_err_intr(sc, irq, sts);
		}
		sunxi_can_write(sc, SUNXI_CAN_INT_REG, irq);
                rnd_add_uint32(&sc->sc_rnd_source, irq);

	}
	mutex_exit(&sc->sc_intr_lock);

	return rv;
}

void
sunxi_can_ifstart(struct ifnet *ifp)
{
	struct sunxi_can_softc * const sc = ifp->if_softc;
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
	reg0val = cf->can_dlc & SUNXI_CAN_TXBUF0_DL;
	if (cf->can_id & CAN_RTR_FLAG)
		reg0val |= SUNXI_CAN_TXBUF0_RTR;

	if (cf->can_id & CAN_EFF_FLAG) {
		reg0val |= SUNXI_CAN_TXBUF0_EFF;
		sunxi_can_write(sc, SUNXI_CAN_TXBUF1_REG,
		    (cf->can_id >> 21) & 0xff);
		sunxi_can_write(sc, SUNXI_CAN_TXBUF2_REG,
		    (cf->can_id >> 13) & 0xff);
		sunxi_can_write(sc, SUNXI_CAN_TXBUF3_REG,
		    (cf->can_id >> 5) & 0xff);
		sunxi_can_write(sc, SUNXI_CAN_TXBUF4_REG,
		    (cf->can_id << 3) & 0xf8);
		regd = SUNXI_CAN_TXBUF5_REG;
	} else {
		sunxi_can_write(sc, SUNXI_CAN_TXBUF1_REG,
		    (cf->can_id >> 3) & 0xff);
		sunxi_can_write(sc, SUNXI_CAN_TXBUF2_REG,
		    (cf->can_id << 5) & 0xe0);
		regd = SUNXI_CAN_TXBUF3_REG;
	}

	for (i = 0; i < cf->can_dlc; i++) {
		sunxi_can_write(sc, regd + i * 4, cf->data[i]);
	}
	sunxi_can_write(sc, SUNXI_CAN_TXBUF0_REG, reg0val);

	if (sc->sc_linkmodes & CAN_LINKMODE_LOOPBACK) {
		sunxi_can_write(sc, SUNXI_CAN_CMD_REG,
			SUNXI_CAN_CMD_TANS_REQ | SUNXI_CAN_CMD_SELF_REQ);
	} else {
		sunxi_can_write(sc, SUNXI_CAN_CMD_REG, SUNXI_CAN_CMD_TANS_REQ);
	}
	ifp->if_flags |= IFF_OACTIVE;
	ifp->if_timer = 5;
	can_bpf_mtap(ifp, m, 0);
out:
	mutex_exit(&sc->sc_intr_lock);
}

static int
sunxi_can_ifup(struct sunxi_can_softc * const sc)
{
	uint32_t reg;

	/* setup timings and mode - has to be done in reset */
	reg = SUNXI_CAN_MODSEL_RST;
	if (sc->sc_linkmodes & CAN_LINKMODE_LISTENONLY)
		reg |= SUNXI_CAN_MODSEL_LST_ONLY;

	if (sc->sc_linkmodes & CAN_LINKMODE_LOOPBACK)
		reg |= SUNXI_CAN_MODSEL_LB_MOD;

	sunxi_can_write(sc, SUNXI_CAN_MODSEL_REG, reg);

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
		reg |= SUNXI_CAN_BUS_TIME_SAM;

	sunxi_can_write(sc, SUNXI_CAN_BUS_TIME_REG, reg);

	/* set filters to accept all frames */
	sunxi_can_write(sc, SUNXI_CAN_ACPC, 0x00000000);
	sunxi_can_write(sc, SUNXI_CAN_ACPM, 0xffffffff);

	/* clear errors counter */
	sunxi_can_write(sc, SUNXI_CAN_REC_REG, 0);

	/* leave reset mode and enable interrupts */
	sunxi_can_exit_reset(sc);
	sunxi_can_write(sc, SUNXI_CAN_INTE_REG,
	    SUNXI_CAN_INT_TX_FLAG | SUNXI_CAN_INT_RX_FLAG | SUNXI_CAN_INT_ALLERRS);
	sc->sc_ifp->if_flags |= IFF_RUNNING;
	return 0;
}

static void
sunxi_can_ifdown(struct sunxi_can_softc * const sc)
{
	sc->sc_ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	sc->sc_ifp->if_timer = 0;
	sunxi_can_enter_reset(sc);
	sunxi_can_write(sc, SUNXI_CAN_INTE_REG, 0);
	sunxi_can_write(sc, SUNXI_CAN_INT_REG,
	    sunxi_can_read(sc, SUNXI_CAN_INT_REG));
}

static int
sunxi_can_ifioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct sunxi_can_softc * const sc = ifp->if_softc;
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
				error = sunxi_can_ifup(sc);
				if (error) {
					ifp->if_flags &= ~IFF_UP;
				}
			} else if ((ifp->if_flags & IFF_UP) == 0 &&
			    (ifp->if_flags & IFF_RUNNING) != 0) {
				sunxi_can_ifdown(sc);
			}
		}
		break;
	}

	mutex_exit(&sc->sc_intr_lock);
	return error;
}

void
sunxi_can_ifwatchdog(struct ifnet *ifp)
{
	struct sunxi_can_softc * const sc = ifp->if_softc;
	printf("%s: watchdog timeout\n", device_xname(sc->sc_dev));

	mutex_enter(&sc->sc_intr_lock);
	printf("irq 0x%x en 0x%x mode 0x%x status 0x%x timings 0x%x err 0x%x\n",
	    sunxi_can_read(sc, SUNXI_CAN_INT_REG),
	    sunxi_can_read(sc, SUNXI_CAN_INTE_REG),
	    sunxi_can_read(sc, SUNXI_CAN_MODSEL_REG),
	    sunxi_can_read(sc, SUNXI_CAN_STA_REG),
	    sunxi_can_read(sc, SUNXI_CAN_BUS_TIME_REG),
	    sunxi_can_read(sc, SUNXI_CAN_REC_REG));
	/* if there is a transmit in progress abort */
	if (sunxi_can_tx_abort(sc)) {
		ifp->if_oerrors++;
	}
	mutex_exit(&sc->sc_intr_lock);
}

static void
sunxi_can_enter_reset(struct sunxi_can_softc *sc)
{
	int i;
	uint32_t val;

	for (i = 0; i < 1000; i++) {
		val = sunxi_can_read(sc, SUNXI_CAN_MODSEL_REG);
		val |= SUNXI_CAN_MODSEL_RST;
		sunxi_can_write(sc, SUNXI_CAN_MODSEL_REG, val);
		val = sunxi_can_read(sc, SUNXI_CAN_MODSEL_REG);
		if (val & SUNXI_CAN_MODSEL_RST)
			return;
	}
	printf("%s: couldn't enter reset mode\n", device_xname(sc->sc_dev));
}

static void
sunxi_can_exit_reset(struct sunxi_can_softc *sc)
{
	int i;
	uint32_t val;

	for (i = 0; i < 1000; i++) {
		val = sunxi_can_read(sc, SUNXI_CAN_MODSEL_REG);
		val &= ~SUNXI_CAN_MODSEL_RST;
		sunxi_can_write(sc, SUNXI_CAN_MODSEL_REG, val);
		val = sunxi_can_read(sc, SUNXI_CAN_MODSEL_REG);
		if ((val & SUNXI_CAN_MODSEL_RST) == 0)
			return;
	}
	printf("%s: couldn't leave reset mode\n", device_xname(sc->sc_dev));
}
