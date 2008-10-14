/*	$NetBSD: if_virt.c,v 1.3 2008/10/14 00:50:44 pooka Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/condvar.h>
#include <sys/fcntl.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/sockio.h>
#include <sys/socketvar.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_tap.h>

#include <netinet/in.h>
#include <netinet/in_var.h>

#include <rump/rump.h>
#include <rump/rumpuser.h>

#include "rump_private.h"

/*
 * Virtual interface for userspace purposes.  Uses tap(4) to
 * interface with the kernel and just simply shovels data
 * to/from /dev/tap.
 */

#define VIRTIF_BASE "virt"
static int nunits;

static int	virtif_init(struct ifnet *);
static int	virtif_ioctl(struct ifnet *, u_long, void *);
static void	virtif_start(struct ifnet *);
static void	virtif_stop(struct ifnet *, int);

struct virtif_sc {
	struct ifnet sc_if;
	char sc_tapname[IFNAMSIZ];
	int sc_tapfd;
};

static int virtif_create(struct ifaliasreq *, struct ifnet **);
static void virtif_worker(void *);

int
rump_virtif_create(struct ifaliasreq *ia, struct ifnet **ifpp)
{

	return virtif_create(ia, ifpp);
}

/*
 * Create a socket and call ifioctl() to configure the interface.
 * This trickles down to virtif_ioctl().
 */
static int
configaddr(struct ifnet *ifp, struct ifaliasreq *ia)
{
	struct socket *so;
	int error;

	strcpy(ia->ifra_name, ifp->if_xname);
	error = socreate(ia->ifra_addr.sa_family, &so, SOCK_DGRAM,
	    0, curlwp, NULL);
	if (error)
		return error;
	error = ifioctl(so, SIOCAIFADDR, ia, curlwp);
	soclose(so);

	return error;
}

static int
virtif_create(struct ifaliasreq *ia, struct ifnet **ifpp)
{
	struct virtif_sc *sc;
	struct ifreq ifr;
	struct ifnet *ifp;
	uint8_t enaddr[ETHER_ADDR_LEN] = { 0xb2, 0x0a, 0x00, 0x0b, 0x0e, 0x01 };
	int fd, rv, error;
	int mynum;

	/*
	 * XXX: this is currently un-sane.  Need to figure out a way
	 * to configure this with a bridge into a sane network without
	 * hardcoding it.
	 */
	fd = rumpuser_open("/dev/tap0", O_RDWR, &error);
	if (fd == -1) {
		printf("virtif_create: can't open /dev/tap %d\n", error);
		return error;
	}

	sc = kmem_zalloc(sizeof(*sc), KM_SLEEP);
	rv = rumpuser_ioctl(fd, TAPGIFNAME, &ifr, &error);
	if (rv == -1) {
		kmem_free(sc, sizeof(*sc));
		return error;
	}
	strlcpy(sc->sc_tapname, ifr.ifr_name, IFNAMSIZ);
	sc->sc_tapfd = fd;

	ifp = &sc->sc_if;
	mynum = nunits++;
	sprintf(ifp->if_xname, "%s%d", VIRTIF_BASE, mynum);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = virtif_init;
	ifp->if_ioctl = virtif_ioctl;
	ifp->if_start = virtif_start;
	ifp->if_stop = virtif_stop;

	if (rump_threads) {
		rv = kthread_create(PRI_NONE, 0, NULL, virtif_worker, ifp,
		    NULL, "virtifi");
		if (rv) {
			kmem_free(sc, sizeof(*sc));
			return rv;
		}
	} else {
		printf("WARNING: threads not enabled, receive NOT working\n");
	}

	if_attach(ifp);
	ether_ifattach(ifp, enaddr);

	if (ia)
		configaddr(ifp, ia);
	if (ifpp)
		*ifpp = ifp;

	return 0;
}

static int
virtif_init(struct ifnet *ifp)
{

	ifp->if_flags |= IFF_RUNNING;

	return 0;
}

static int
virtif_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	int s, rv;

	s = splnet();
	rv = ether_ioctl(ifp, cmd, data);
	splx(s);

	return rv;
}

/* just send everything in-context */
static void
virtif_start(struct ifnet *ifp)
{
	struct virtif_sc *sc = ifp->if_softc;
	struct iovec io[16];
	struct mbuf *m, *m0;
	int s, i, error;

	s = splnet();
	for (;;) {
		IF_DEQUEUE(&ifp->if_snd, m0);
		if (!m0)
			break;
		splx(s);

		m = m0;
		for (i = 0; i < 16 && m; i++) {
			io[i].iov_base = mtod(m, void *);
			io[i].iov_len = m->m_len;
			m = m->m_next;
		}
		if (i == 16)
			panic("lazy bum");
		rumpuser_writev(sc->sc_tapfd, io, i, &error);
		m_freem(m0);
		s = splnet();
	}

	ifp->if_flags &= ~IFF_OACTIVE;
	splx(s);
}

static void
virtif_stop(struct ifnet *ifp, int disable)
{

	panic("%s: unimpl", __func__);
}

static void
virtif_worker(void *arg)
{
	struct ifnet *ifp = arg;
	struct virtif_sc *sc = ifp->if_softc;
	struct mbuf *m;
	size_t plen = ETHER_MAX_LEN_JUMBO+1;
	ssize_t n;
	int error;

	for (;;) {
		m = m_gethdr(M_WAIT, MT_DATA);
		MEXTMALLOC(m, plen, M_WAIT);

		n = rumpuser_read(sc->sc_tapfd, mtod(m, void *), plen, &error);
		KASSERT(n < ETHER_MAX_LEN_JUMBO);
		if (n <= 0) {
			m_freem(m);
			break;
		}
		m->m_len = m->m_pkthdr.len = n;
		m->m_pkthdr.rcvif = ifp;
		ether_input(ifp, m);
	}

	panic("virtif_workin is a lazy boy %d\n", error);
}
