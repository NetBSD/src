/* $NetBSD: if_veth.c,v 1.5.10.1 2017/12/03 11:36:47 jdolecek Exp $ */

/*-
 * Copyright (c) 2011 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: if_veth.c,v 1.5.10.1 2017/12/03 11:36:47 jdolecek Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <net/bpf.h>

#include <machine/mainbus.h>
#include <machine/thunk.h>

static int	veth_match(device_t, cfdata_t, void *);
static void	veth_attach(device_t, device_t, void *);
static bool	veth_shutdown(device_t, int);

static int	veth_init(struct ifnet *);
static void	veth_start(struct ifnet *);
static void	veth_stop(struct ifnet *, int);
static void	veth_watchdog(struct ifnet *);
static int	veth_ioctl(struct ifnet *, u_long, void *);

static int	veth_rx(void *);
static void	veth_softrx(void *);
static void	veth_softtx(void *);

static int	veth_ifmedia_change(struct ifnet *);
static void	veth_ifmedia_status(struct ifnet *, struct ifmediareq *);

#ifdef VETH_DEBUG
#define vethprintf printf
#else
static inline void vethprintf(const char *fmt, ...) { }
#endif

struct veth_softc {
	device_t		sc_dev;
	struct ethercom		sc_ec;
	struct ifmedia		sc_ifmedia;
	int			sc_tapfd;
	uint8_t			sc_eaddr[ETHER_ADDR_LEN];
	uint8_t			sc_rx_buf[4096 + 65536];
	uint8_t			sc_tx_buf[4096 + 65536];
	void			*sc_rx_ih;
	void			*sc_rx_intr;
	void			*sc_tx_intr;
};

CFATTACH_DECL_NEW(veth, sizeof(struct veth_softc),
    veth_match, veth_attach, NULL, NULL);

static int
veth_match(device_t parent, cfdata_t match, void *opaque)
{
	struct thunkbus_attach_args *taa = opaque;

	if (taa->taa_type != THUNKBUS_TYPE_VETH)
		return 0;

	return 1;
}

static void
veth_attach(device_t parent, device_t self, void *opaque)
{
	struct veth_softc *sc = device_private(self);
	struct thunkbus_attach_args *taa = opaque;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	int rv;

	sc->sc_dev = self;

	pmf_device_register1(self, NULL, NULL, veth_shutdown);

	sc->sc_tapfd = thunk_open_tap(taa->u.veth.device);
	if (sc->sc_tapfd == -1) {
		aprint_error(": couldn't open %s: %d\n",
		    taa->u.veth.device, thunk_geterrno());
		return;
	}
	if (ether_aton_r(sc->sc_eaddr, sizeof(sc->sc_eaddr),
	    taa->u.veth.eaddr) != 0) {
		aprint_error(": couldn't parse hw address '%s'\n",
		    taa->u.veth.eaddr);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Virtual Ethernet (device = %s)\n", taa->u.veth.device);

	aprint_normal_dev(self, "Ethernet address %s\n",
	    ether_sprintf(sc->sc_eaddr));

	strlcpy(ifp->if_xname, device_xname(self), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = veth_ioctl;
	ifp->if_watchdog = veth_watchdog;
	ifp->if_start = veth_start;
	ifp->if_init = veth_init;
	ifp->if_stop = veth_stop;
	IFQ_SET_MAXLEN(&ifp->if_snd, IFQ_MAXLEN);
	IFQ_SET_READY(&ifq->if_snd);

	rv = if_initialize(ifp);
	if (rv != 0) {
		aprint_error_dev(self, "if_initialize failed(%d)\n", rv);
		thunk_close(sc->sc_tapfd);
		pmf_device_deregister(self);
		return; /* Error */
	}
	ether_ifattach(ifp, sc->sc_eaddr);
	if_register(ifp);

	ifmedia_init(&sc->sc_ifmedia, 0,
	    veth_ifmedia_change,
	    veth_ifmedia_status);
	ifmedia_add(&sc->sc_ifmedia, IFM_ETHER|IFM_100_TX, 0, NULL);
	ifmedia_set(&sc->sc_ifmedia, IFM_ETHER|IFM_100_TX);

	sc->sc_rx_intr = softint_establish(SOFTINT_NET, veth_softrx, sc);
	if (sc->sc_rx_intr == NULL)
		panic("couldn't establish veth rx softint");
	sc->sc_tx_intr = softint_establish(SOFTINT_NET, veth_softtx, sc);
	if (sc->sc_tx_intr == NULL)
		panic("couldn't establish veth tx softint");

	thunk_setown(sc->sc_tapfd);

	sc->sc_rx_ih = sigio_intr_establish(veth_rx, sc);
	if (sc->sc_rx_ih == NULL)
		panic("couldn't establish veth rx interrupt");
}

static bool
veth_shutdown(device_t self, int flags)
{
	struct veth_softc *sc = device_private(self);

	if (sc->sc_tapfd != -1)
		thunk_close(sc->sc_tapfd);

	return true;
}

static int
veth_init(struct ifnet *ifp)
{
	vethprintf("%s: %s flags=%x\n", __func__, ifp->if_xname, ifp->if_flags);

	veth_stop(ifp, 0);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	return 0;
}

static int
veth_rx(void *priv)
{
	struct veth_softc *sc = priv;

	softint_schedule(sc->sc_rx_intr);
	return 0;
}

static void
veth_softrx(void *priv)
{
	struct veth_softc *sc = priv;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct mbuf *m;
	ssize_t len;
	int s, avail;

	for (;;) {
		avail = thunk_pollin_tap(sc->sc_tapfd, 0);
		if (avail == 0)
			break;

		len = thunk_read(sc->sc_tapfd, sc->sc_rx_buf,
		    min(avail, sizeof(sc->sc_rx_buf)));
		vethprintf("%s: read returned %d\n", __func__, len);
		if (len == -1)
			panic("read() from tap failed");

		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
			vethprintf("MGETHDR failed (input error)\n");
			++ifp->if_ierrors;
			continue;
		}
		if (len > MHLEN) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				m_freem(m);
				++ifp->if_ierrors;
				vethprintf("M_EXT not set (input error)\n");
				continue;
			}
		}
		m_set_rcvif(m, ifp);
		m->m_pkthdr.len = m->m_len = len;
		memcpy(mtod(m, void *), sc->sc_rx_buf, len);

		s = splnet();
		if_input(ifp, m);
		splx(s);
	}
}

static void
veth_softtx(void *priv)
{
	struct veth_softc *sc = priv;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	int s;

	if (ifp->if_flags & IFF_OACTIVE) {
		if (thunk_pollout_tap(sc->sc_tapfd, 0) == 1)
			ifp->if_flags &= ~IFF_OACTIVE;
	}

	s = splnet();
	veth_start(ifp);
	splx(s);
}

static void
veth_start(struct ifnet *ifp)
{
	struct veth_softc *sc = ifp->if_softc;
	struct mbuf *m0;
	ssize_t len;

	vethprintf("%s: %s flags=%x\n", __func__, ifp->if_xname, ifp->if_flags);

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (;;) {
		IFQ_POLL(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;

		if (thunk_pollout_tap(sc->sc_tapfd, 0) != 1) {
			printf("queue full\n");
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m0);
		bpf_mtap(ifp, m0);

		m_copydata(m0, 0, m0->m_pkthdr.len, sc->sc_tx_buf);

		vethprintf("write %d bytes...\n", m0->m_pkthdr.len);
		len = thunk_write(sc->sc_tapfd, sc->sc_tx_buf,
		    m0->m_pkthdr.len);
		vethprintf("write returned %d\n", len);
		if (len > 0)
			++ifp->if_opackets;
		else
			++ifp->if_oerrors;
		m_freem(m0);
	}
}

static void
veth_stop(struct ifnet *ifp, int disable)
{
	vethprintf("%s: %s flags=%x\n", __func__, ifp->if_xname, ifp->if_flags);
	ifp->if_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

static void
veth_watchdog(struct ifnet *ifp)
{
	vethprintf("%s: %s flags=%x\n", __func__, ifp->if_xname, ifp->if_flags);
	++ifp->if_oerrors;
	veth_init(ifp);
}

static int
veth_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct veth_softc *sc = ifp->if_softc;
	struct ifreq *ifr;
	int s, error;

	vethprintf("%s: %s flags=%x\n", __func__, ifp->if_xname, ifp->if_flags);

	s = splnet();

	switch (cmd) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		ifr = data;
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_ifmedia, cmd);
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if ((error = ether_ioctl(ifp, cmd, data)) == ENETRESET) {
			if (ifp->if_flags & IFF_RUNNING) {
				veth_init(ifp);
			}
			error = 0;
		}
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
		break;
	}

	splx(s);
	return error;
}

static int
veth_ifmedia_change(struct ifnet *ifp)
{
	vethprintf("%s: %s flags=%x\n", __func__, ifp->if_xname, ifp->if_flags);
	return 0;
}

static void
veth_ifmedia_status(struct ifnet *ifp, struct ifmediareq *req)
{
	vethprintf("%s: %s flags=%x\n", __func__, ifp->if_xname, ifp->if_flags);
	req->ifm_status |= IFM_ACTIVE | IFM_AVALID;
	req->ifm_active = IFM_100_TX | IFM_FDX | IFM_ETHER;
}
