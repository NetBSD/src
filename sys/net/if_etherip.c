/*      $NetBSD: if_etherip.c,v 1.5.2.5 2007/07/15 15:52:58 ad Exp $        */

/*
 *  Copyright (c) 2006, Hans Rosenfeld <rosenfeld@grumpf.hope-2000.org>
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of Hans Rosenfeld nor the names of his
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 *
 *
 *  Copyright (c) 2003, 2004 The NetBSD Foundation.
 *  All rights reserved.
 *
 *  This code is derived from software contributed to the NetBSD Foundation
 *   by Quentin Garnier
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. All advertising materials mentioning features or use of this software
 *     must display the following acknowledgement:
 *         This product includes software developed by the NetBSD
 *         Foundation, Inc. and its contributors.
 *  4. Neither the name of The NetBSD Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/sysctl.h>
#include <sys/queue.h>
#include <sys/kauth.h>
#include <sys/socket.h>
#include <sys/intr.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/route.h>
#include <net/if_etherip.h>
#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#ifdef  INET
#include <netinet/in_var.h>
#endif  /* INET */
#include <netinet/ip_etherip.h>

#ifdef INET6
#include <netinet6/ip6_etherip.h>
#ifndef INET
#include <netinet/in.h>
#endif
#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_gif.h>
#include <netinet6/ip6protosw.h>
#endif /* INET6 */

#include <compat/sys/sockio.h>

static int etherip_node;
static int etherip_sysctl_handler(SYSCTLFN_PROTO);
SYSCTL_SETUP_PROTO(sysctl_etherip_setup);

void etheripattach(int);

static int  etherip_match(struct device *, struct cfdata *, void *);
static void etherip_attach(struct device *, struct device *, void *);
static int  etherip_detach(struct device *, int);

CFATTACH_DECL(etherip, sizeof(struct etherip_softc),
	      etherip_match, etherip_attach, etherip_detach, NULL);
extern struct cfdriver etherip_cd;

static void etherip_start(struct ifnet *);
static void etherip_stop(struct ifnet *, int);
static int  etherip_init(struct ifnet *);
static int  etherip_ioctl(struct ifnet *, u_long, void *);

static int  etherip_mediachange(struct ifnet *);
static void etherip_mediastatus(struct ifnet *, struct ifmediareq *);

static int  etherip_clone_create(struct if_clone *, int);
static int  etherip_clone_destroy(struct ifnet *);

static struct if_clone etherip_cloners = IF_CLONE_INITIALIZER(
	"etherip", etherip_clone_create, etherip_clone_destroy);

static int  etherip_set_tunnel(struct ifnet *,
			       struct sockaddr *, struct sockaddr *);
static void etherip_delete_tunnel(struct ifnet *);
static void etheripintr(void *);

void
etheripattach(int count)
{
	int error;

	error = config_cfattach_attach(etherip_cd.cd_name, &etherip_ca);

	if (error) {
		aprint_error("%s: unable to register cfattach\n",
			     etherip_cd.cd_name);
		(void)config_cfdriver_detach(&etherip_cd);
		return;
	}

	LIST_INIT(&etherip_softc_list);
	if_clone_attach(&etherip_cloners);
}

/* Pretty much useless for a pseudo-device */
static int
etherip_match(struct device *self, struct cfdata *cfdata, void *arg)
{
	return 1;
}

static void
etherip_attach(struct device *parent, struct device *self, void *aux)
{
	struct etherip_softc *sc = (struct etherip_softc *)self;
	struct ifnet *ifp;
	const struct sysctlnode *node;
	u_int8_t enaddr[ETHER_ADDR_LEN] =
		{ 0xf2, 0x0b, 0xa5, 0xff, 0xff, 0xff };
	char enaddrstr[3 * ETHER_ADDR_LEN];
	struct timeval tv;
	uint32_t ui;
	int error;

	sc->sc_si  = NULL;
	sc->sc_src = NULL;
	sc->sc_dst = NULL;

	/*
	 * In order to obtain unique initial Ethernet address on a host,
	 * do some randomisation using the current uptime.  It's not meant
	 * for anything but avoiding hard-coding an address.
	 */
	getmicrouptime(&tv);
	ui = (tv.tv_sec ^ tv.tv_usec) & 0xffffff;
	memcpy(enaddr+3, (u_int8_t *)&ui, 3);
	
	aprint_verbose("%s: Ethernet address %s\n", sc->sc_dev.dv_xname,
		       ether_snprintf(enaddrstr, sizeof(enaddrstr), enaddr));

	/*
	 * Why 1000baseT? Why not? You can add more.
	 *
	 * Note that there are 3 steps: init, one or several additions to
	 * list of supported media, and in the end, the selection of one
	 * of them.
	 */
	ifmedia_init(&sc->sc_im, 0, etherip_mediachange, etherip_mediastatus);
	ifmedia_add(&sc->sc_im, IFM_ETHER|IFM_1000_T, 0, NULL);
	ifmedia_add(&sc->sc_im, IFM_ETHER|IFM_1000_T|IFM_FDX, 0, NULL);
	ifmedia_add(&sc->sc_im, IFM_ETHER|IFM_100_TX, 0, NULL);
	ifmedia_add(&sc->sc_im, IFM_ETHER|IFM_100_TX|IFM_FDX, 0, NULL);
	ifmedia_add(&sc->sc_im, IFM_ETHER|IFM_10_T, 0, NULL);
	ifmedia_add(&sc->sc_im, IFM_ETHER|IFM_10_T|IFM_FDX, 0, NULL);
	ifmedia_add(&sc->sc_im, IFM_ETHER|IFM_AUTO, 0, NULL);
	ifmedia_set(&sc->sc_im, IFM_ETHER|IFM_AUTO);
	
	/*
	 * One should note that an interface must do multicast in order
	 * to support IPv6.
	 */
	ifp = &sc->sc_ec.ec_if;
	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = etherip_ioctl;
	ifp->if_start = etherip_start;
	ifp->if_stop  = etherip_stop;
	ifp->if_init  = etherip_init;
	IFQ_SET_READY(&ifp->if_snd);
	
	sc->sc_ec.ec_capabilities = ETHERCAP_VLAN_MTU | ETHERCAP_JUMBO_MTU;
	
	/* 
	 * Those steps are mandatory for an Ethernet driver, the first call
	 * being common to all network interface drivers.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, enaddr);

	/*
	 * Add a sysctl node for that interface.
	 *
	 * The pointer transmitted is not a string, but instead a pointer to
	 * the softc structure, which we can use to build the string value on
	 * the fly in the helper function of the node.  See the comments for
	 * etherip_sysctl_handler for details.
	 */
	error = sysctl_createv(NULL, 0, NULL, &node, CTLFLAG_READWRITE, 
			       CTLTYPE_STRING, sc->sc_dev.dv_xname, NULL,
			       etherip_sysctl_handler, 0, sc, 18, CTL_NET,
			       AF_LINK, etherip_node, device_unit(&sc->sc_dev),
			       CTL_EOL);
	if (error)
		aprint_error("%s: sysctl_createv returned %d, ignoring\n",
			     sc->sc_dev.dv_xname, error);

	/* insert into etherip_softc_list */
	LIST_INSERT_HEAD(&etherip_softc_list, sc, etherip_list);
}

/*
 * When detaching, we do the inverse of what is done in the attach
 * routine, in reversed order.
 */
static int
etherip_detach(struct device* self, int flags)
{
	struct etherip_softc *sc = (struct etherip_softc *)self;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	int error, s;

	s = splnet();
	etherip_stop(ifp, 1);
	if_down(ifp);
	splx(s);

	/*
	 * Destroying a single leaf is a very straightforward operation using
	 * sysctl_destroyv.  One should be sure to always end the path with
	 * CTL_EOL.
	 */
	error = sysctl_destroyv(NULL, CTL_NET, AF_LINK, etherip_node,
				device_unit(&sc->sc_dev), CTL_EOL);
	if (error)
		aprint_error("%s: sysctl_destroyv returned %d, ignoring\n",
			     sc->sc_dev.dv_xname, error);

	LIST_REMOVE(sc, etherip_list);
	etherip_delete_tunnel(ifp);
	ether_ifdetach(ifp);
	if_detach(ifp);
	rtcache_free(&sc->sc_ro);
	ifmedia_delete_instance(&sc->sc_im, IFM_INST_ANY);

	return 0;
}

/*
 * This function is called by the ifmedia layer to notify the driver
 * that the user requested a media change.  A real driver would
 * reconfigure the hardware.
 */
static int
etherip_mediachange(struct ifnet *ifp)
{
	return 0;
}

/*
 * Here the user asks for the currently used media.
 */
static void
etherip_mediastatus(struct ifnet *ifp, struct ifmediareq *imr)
{
	struct etherip_softc *sc = (struct etherip_softc *)ifp->if_softc;

	imr->ifm_active = sc->sc_im.ifm_cur->ifm_media;
}

static void
etherip_start(struct ifnet *ifp)
{
	struct etherip_softc *sc = (struct etherip_softc *)ifp->if_softc;

	if(sc->sc_si)
		softint_schedule(sc->sc_si);
}

static void
etheripintr(void *arg)
{
	struct etherip_softc *sc = (struct etherip_softc *)arg;;
	struct ifnet *ifp = &sc->sc_ec.ec_if;;
	struct mbuf *m;
	int s, error;

	for (;;) {
		s = splnet();
		IFQ_DEQUEUE(&ifp->if_snd, m);
		splx(s);
		if (m == NULL)
			return;
		
#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif
		
		ifp->if_opackets++;
		if (sc->sc_src && sc->sc_dst) {
			ifp->if_flags |= IFF_OACTIVE;
			switch (sc->sc_src->sa_family) {
#ifdef INET
			case AF_INET:
				error = ip_etherip_output(ifp, m);
				break;
#endif
#ifdef INET6
			case AF_INET6:
				error = ip6_etherip_output(ifp, m);
				break;
#endif
			default:
				error = ENETDOWN;
			}
			ifp->if_flags &= ~IFF_OACTIVE;
		} else  m_freem(m);
	}
}

static int
etherip_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct etherip_softc *sc = (struct etherip_softc *)ifp->if_softc;
	struct ifreq *ifr = data;
	struct sockaddr *src, *dst;
	int s, error;

	switch (cmd) {
	case SIOCSLIFPHYADDR:
		src = (struct sockaddr *)
			&(((struct if_laddrreq *)data)->addr);
		dst = (struct sockaddr *)
			&(((struct if_laddrreq *)data)->dstaddr);

		/* sa_family must be equal */
		if (src->sa_family != dst->sa_family)
			return EINVAL;

		/* validate sa_len */
		switch (src->sa_family) {
#ifdef INET
		case AF_INET:
			if (src->sa_len != sizeof(struct sockaddr_in) ||
			    dst->sa_len != sizeof(struct sockaddr_in))
				return EINVAL;
			break;
#endif
#ifdef INET6
		case AF_INET6:
			if (src->sa_len != sizeof(struct sockaddr_in6) ||
			    dst->sa_len != sizeof(struct sockaddr_in6))
				return EINVAL;
			break;
#endif
		default:
			return EAFNOSUPPORT;
		}
		
		error = etherip_set_tunnel(ifp, src, dst);
		break;

	case SIOCDIFPHYADDR:
		etherip_delete_tunnel(ifp);
		error = 0;
		break;

	case SIOCGLIFPHYADDR:
		if (sc->sc_src == NULL || sc->sc_dst == NULL)
			return EADDRNOTAVAIL;

		/* copy src */
		src = sc->sc_src;
		dst = (struct sockaddr *)
			&(((struct if_laddrreq *)data)->addr);
		if (src->sa_len > sizeof(((struct if_laddrreq *)data)->addr))
			return EINVAL;
		memcpy(dst, src, src->sa_len);

		/* copy dst */
		src = sc->sc_dst;
		dst = (struct sockaddr *)
			&(((struct if_laddrreq *)data)->dstaddr);
		if (src->sa_len > sizeof(((struct if_laddrreq *)data)->dstaddr))
			return EINVAL;
		memcpy(dst, src, src->sa_len);

		error = 0;
		break;

#ifdef OSIOCSIFMEDIA
	case OSIOCSIFMEDIA:
#endif
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		s = splnet();
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_im, cmd);
		splx(s);
		break;

	default:
		s = splnet();
		error = ether_ioctl(ifp, cmd, data);
		splx(s);
		if (error == ENETRESET)
			error = 0;
		break;
	}
	
	return (error);
}

static int
etherip_set_tunnel(struct ifnet *ifp, 
		   struct sockaddr *src, 
		   struct sockaddr *dst)
{
	struct etherip_softc *sc = (struct etherip_softc *)ifp->if_softc;
	struct etherip_softc *sc2;
	struct sockaddr *osrc, *odst;
	int s, error = 0;

	s = splsoftnet();

	LIST_FOREACH(sc2, &etherip_softc_list, etherip_list) {
		if (sc2 == sc)
			continue;
		if (!sc2->sc_dst || !sc2->sc_src)
			continue;
		if (sc2->sc_dst->sa_family != dst->sa_family ||
		    sc2->sc_dst->sa_len    != dst->sa_len    ||
		    sc2->sc_src->sa_family != src->sa_family ||
		    sc2->sc_src->sa_len    != src->sa_len)
			continue;
		/* can't configure same pair of address onto two tunnels */
		if (bcmp(sc2->sc_dst, dst, dst->sa_len) == 0 &&
		    bcmp(sc2->sc_src, src, src->sa_len) == 0) {
			error = EADDRNOTAVAIL;
			goto out;
		}
		/* XXX both end must be valid? (I mean, not 0.0.0.0) */
	}

	if (sc->sc_si) {
		softint_disestablish(sc->sc_si);
		sc->sc_si = NULL;
	}

	ifp->if_flags &= ~IFF_RUNNING;

	osrc = sc->sc_src; sc->sc_src = NULL;
	odst = sc->sc_dst; sc->sc_dst = NULL;

	sc->sc_src = (struct sockaddr *)malloc(src->sa_len, M_IFADDR, M_WAITOK);
	memcpy(sc->sc_src, src, src->sa_len);
	if (osrc) FREE(osrc, M_IFADDR);

	sc->sc_dst = (struct sockaddr *)malloc(dst->sa_len, M_IFADDR, M_WAITOK);
	memcpy(sc->sc_dst, dst, dst->sa_len);
	if (odst) FREE(odst, M_IFADDR);

	ifp->if_flags |= IFF_RUNNING;

	sc->sc_si = softint_establish(SOFTINT_NET, etheripintr, sc);
	if (sc->sc_si == NULL)
		error = ENOMEM;

out:
	splx(s);

	return(error);
}

static void
etherip_delete_tunnel(struct ifnet *ifp)
{
	struct etherip_softc *sc = (struct etherip_softc *)ifp->if_softc;
	int s;

	s = splsoftnet();

	if (sc->sc_si) {
		softint_disestablish(sc->sc_si);
		sc->sc_si = NULL;
	}

	if (sc->sc_src) {
		FREE(sc->sc_src, M_IFADDR);
		sc->sc_src = NULL;
	}
	if (sc->sc_dst) {
		FREE(sc->sc_dst, M_IFADDR);
		sc->sc_dst = NULL;
	}

	ifp->if_flags &= ~IFF_RUNNING;
	splx(s);
}

static int
etherip_init(struct ifnet *ifp)
{
	struct etherip_softc *sc = ifp->if_softc;

	if (sc->sc_si == NULL)
		sc->sc_si = softint_establish(SOFTINT_NET, etheripintr, sc);

	if (sc->sc_si == NULL)
		return(ENOMEM);

	ifp->if_flags |= IFF_RUNNING;
	etherip_start(ifp);

	return 0;
}

static void
etherip_stop(struct ifnet *ifp, int disable)
{
	ifp->if_flags &= ~IFF_RUNNING;
}

static int
etherip_clone_create(struct if_clone *ifc, int unit)
{
	struct cfdata *cf;

	MALLOC(cf, struct cfdata *, sizeof(struct cfdata), M_DEVBUF, M_WAITOK);
	cf->cf_name   = etherip_cd.cd_name;
	cf->cf_atname = etherip_ca.ca_name;
	cf->cf_unit   = unit;
	cf->cf_fstate = FSTATE_STAR;

	if (config_attach_pseudo(cf) == NULL) {
		aprint_error("%s%d: unable to attach an instance\n",
			     etherip_cd.cd_name, unit);
		return (ENXIO);
	}

	return 0;
}

static int
etherip_clone_destroy(struct ifnet *ifp)
{
	struct device *dev = (struct device *)ifp->if_softc;
	struct cfdata *cf = device_cfdata(dev);
	int error;

	if ((error = config_detach(dev, 0)) != 0)
		aprint_error("%s: unable to detach instance\n",
			     dev->dv_xname);
	FREE(cf, M_DEVBUF);

	return error;
}

SYSCTL_SETUP(sysctl_etherip_setup, "sysctl net.link.etherip subtree setup")
{
	const struct sysctlnode *node;
	int error = 0;

	error = sysctl_createv(clog, 0, NULL, NULL,
			       CTLFLAG_PERMANENT,
			       CTLTYPE_NODE, "net", NULL,
			       NULL, 0, NULL, 0,
			       CTL_NET, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(clog, 0, NULL, NULL,
			       CTLFLAG_PERMANENT,
			       CTLTYPE_NODE, "link", NULL,
			       NULL, 0, NULL, 0,
			       CTL_NET, AF_LINK, CTL_EOL);
	if (error)
		return;

	error = sysctl_createv(clog, 0, NULL, &node,
			       CTLFLAG_PERMANENT,
			       CTLTYPE_NODE, "etherip", NULL,
			       NULL, 0, NULL, 0,
			       CTL_NET, AF_LINK, CTL_CREATE, CTL_EOL);
	if (error)
		return;

	etherip_node = node->sysctl_num;
}

static int
etherip_sysctl_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct etherip_softc *sc;
	struct ifnet *ifp;
	int error;
	size_t len;
	char addr[3 * ETHER_ADDR_LEN];

	node = *rnode;
	sc = node.sysctl_data;
	ifp = &sc->sc_ec.ec_if;
	(void)ether_snprintf(addr, sizeof(addr), LLADDR(ifp->if_sadl));
	node.sysctl_data = addr;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	len = strlen(addr);
	if (len < 11 || len > 17)
		return EINVAL;

	/* Commit change */
	if (ether_nonstatic_aton(LLADDR(ifp->if_sadl), addr) != 0)
		return EINVAL;

	return error;
}

