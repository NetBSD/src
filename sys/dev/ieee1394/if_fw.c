/*	$NetBSD: if_fw.c,v 1.15 2002/03/06 05:33:05 nathanw Exp $	*/

/* XXX ALTQ XXX */

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Atsushi Onoe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: if_fw.c,v 1.15 2002/03/06 05:33:05 nathanw Exp $");

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ieee1394.h>
#include <net/if_types.h>
#include <net/if_media.h>
#include <net/ethertypes.h>
#include <net/route.h>

#ifdef INET
#include <netinet/in.h>
#endif /* INET */

#ifdef INET6
#include <netinet/in.h>
#include <netinet6/in6_var.h>
#endif /* INET6 */

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <machine/bus.h>

#include <dev/ieee1394/ieee1394reg.h>
#include <dev/ieee1394/ieee1394var.h>

struct fw_softc {
	struct ieee1394_softc sc_sc1394;
	struct ieee1394com sc_ic;

	int sc_flags;

	int (*sc_enable)(struct fw_softc *);
	void (*sc_disable)(struct fw_softc *);
};

#define	FWF_ATTACHED	0x0001
#define	FWF_ENABLED	0x0002

int  fw_match(struct device *, struct cfdata *, void *);
void fw_attach(struct device *, struct device *, void *);
int  fw_detach(struct device *, int);
int  fw_activate(struct device *, enum devact);

void fw_input(struct device *, struct mbuf *);
void fw_txint(struct device *, struct mbuf *);
int  fw_ioctl(struct ifnet *, u_long, caddr_t);
void fw_start(struct ifnet *);
int  fw_init(struct ifnet *);
void fw_stop(struct ifnet *, int);

struct cfattach fw_ca = {
	sizeof(struct fw_softc), fw_match, fw_attach,
	fw_detach, fw_activate
};

int
fw_match(struct device *parent, struct cfdata *match, void *aux)
{
	char *name = aux;

	if (strcmp(name, "fw") == 0)
		return 1;
	return 0;
}

void
fw_attach(struct device *parent, struct device *self, void *aux)
{
	struct fw_softc *sc = (struct fw_softc *)self;
	struct ieee1394_softc *psc = (struct ieee1394_softc *)parent;
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	int i, s;

	s = splnet();

	memcpy(&sc->sc_ic.ic_hwaddr.iha_uid, psc->sc1394_guid,
	    IEEE1394_ADDR_LEN);
	sc->sc_ic.ic_hwaddr.iha_speed = psc->sc1394_link_speed;
	for (i = 0; i < 32; i++) {
		if ((psc->sc1394_max_receive >> (i + 2)) == 0)
			break;
	}
	sc->sc_ic.ic_hwaddr.iha_maxrec = i;
	if (i < 8) {
		printf("%s: maximum receive packet (%d) is too small\n",
		    sc->sc_sc1394.sc1394_dev.dv_xname, psc->sc1394_max_receive);
		splx(s);
		return;
	}
	sc->sc_ic.ic_hwaddr.iha_offset[0] = (FW_FIFO_HI >> 8) & 0xff;
	sc->sc_ic.ic_hwaddr.iha_offset[1] = FW_FIFO_HI & 0xff;
	sc->sc_ic.ic_hwaddr.iha_offset[2] = (FW_FIFO_LO >> 24) & 0xff;
	sc->sc_ic.ic_hwaddr.iha_offset[3] = (FW_FIFO_LO >> 16) & 0xff;
	sc->sc_ic.ic_hwaddr.iha_offset[4] = (FW_FIFO_LO >>  8) & 0xff;
	sc->sc_ic.ic_hwaddr.iha_offset[5] = FW_FIFO_LO & 0xff;
	strcpy(ifp->if_xname, sc->sc_sc1394.sc1394_dev.dv_xname);
	ifp->if_softc = sc;
#if __NetBSD_Version__ >= 105080000
	ifp->if_init = fw_init;
	ifp->if_stop = fw_stop;
#endif
	ifp->if_start = fw_start;
	ifp->if_ioctl = fw_ioctl;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_baudrate = IF_Mbps(100 << sc->sc_ic.ic_hwaddr.iha_speed);
	ifp->if_addrlen = sizeof(struct ieee1394_hwaddr);
	IFQ_SET_READY(&ifp->if_snd);

	printf(":");
	for (i = 0; i < sizeof(sc->sc_ic.ic_hwaddr); i++)
		printf("%c%02x", (i == 0 ? ' ' : ':'),
		    ((u_int8_t *)&sc->sc_ic.ic_hwaddr)[i]);
	printf("\n");
	if_attach(ifp);
	ieee1394_ifattach(ifp, &sc->sc_ic.ic_hwaddr);

	sc->sc_flags |= FWF_ATTACHED;

	splx(s);
}

int
fw_detach(struct device *self, int flags)
{
	struct fw_softc *sc = (struct fw_softc *)self;
	struct ifnet *ifp = &sc->sc_ic.ic_if;

	/* Succeed if there is no work to do. */
	if ((sc->sc_flags & FWF_ATTACHED) == 0)
		return 0;

	ieee1394_ifdetach(ifp);
	if_detach(ifp);
	if (sc->sc_flags & FWF_ENABLED) {
		if (sc->sc_disable)
			(*sc->sc_disable)(sc);
		sc->sc_flags &= ~FWF_ENABLED;
	}
	return 0;
}

int
fw_activate(struct device *self, enum devact act)
{
	struct fw_softc *sc = (struct fw_softc *)self;
	int s, error = 0;

	s = splnet();
	switch (act) {
	case DVACT_ACTIVATE:
		error = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		if_deactivate(&sc->sc_ic.ic_if);
		break;
	}
	splx(s);

	return error;
}

int
fw_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	default:
		error = ieee1394_ioctl(ifp, cmd, data);
		break;
	}
	splx(s);
	return error;
}

void
fw_start(struct ifnet *ifp)
{
	struct fw_softc *sc = (struct fw_softc *)ifp->if_softc;
	struct ieee1394_softc *psc =
	    (struct ieee1394_softc *)sc->sc_sc1394.sc1394_dev.dv_parent;
	struct mbuf *m0;
	int error;

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;
	for (;;) {
		IFQ_DEQUEUE(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;
		error = (*psc->sc1394_ifoutput)
		    (sc->sc_sc1394.sc1394_dev.dv_parent, m0, fw_txint);
		if (error)
			break;
	}
	if (m0 != NULL)
		ifp->if_flags |= IFF_OACTIVE;
}

void
fw_txint(struct device *self, struct mbuf *m0)
{
	struct fw_softc *sc = (struct fw_softc *)self;
	struct ifnet *ifp = &sc->sc_ic.ic_if;

	m_freem(m0);
	if (ifp->if_flags & IFF_OACTIVE) {
		ifp->if_flags &= ~IFF_OACTIVE;
		fw_start(ifp);
	}
}

int
fw_init(struct ifnet *ifp)
{
	struct fw_softc *sc = (struct fw_softc *)ifp->if_softc;
	struct ieee1394_softc *psc =
	    (struct ieee1394_softc *)sc->sc_sc1394.sc1394_dev.dv_parent;

	(*psc->sc1394_ifinreg)
	    (sc->sc_sc1394.sc1394_dev.dv_parent, FW_FIFO_HI, FW_FIFO_LO,
	    fw_input);
	ifp->if_flags |= IFF_RUNNING;
	return 0;
}

void
fw_stop(struct ifnet *ifp, int disable)
{
	struct fw_softc *sc = (struct fw_softc *)ifp->if_softc;
	struct ieee1394_softc *psc =
	    (struct ieee1394_softc *)sc->sc_sc1394.sc1394_dev.dv_parent;

	(*psc->sc1394_ifinreg)
	    (sc->sc_sc1394.sc1394_dev.dv_parent, FW_FIFO_HI, FW_FIFO_LO,
	    NULL);
	IFQ_PURGE(&ifp->if_snd);
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

void
fw_input(struct device *self, struct mbuf *m0)
{
	struct fw_softc *sc = (struct fw_softc *)self;
	struct ifnet *ifp = &sc->sc_ic.ic_if;

	m0->m_pkthdr.rcvif = ifp;
	(*ifp->if_input)(ifp, m0);
}
