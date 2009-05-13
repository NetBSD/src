/*	$NetBSD: if_virt.c,v 1.5.4.1 2009/05/13 17:23:02 jym Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_virt.c,v 1.5.4.1 2009/05/13 17:23:02 jym Exp $");

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

static int	virtif_init(struct ifnet *);
static int	virtif_ioctl(struct ifnet *, u_long, void *);
static void	virtif_start(struct ifnet *);
static void	virtif_stop(struct ifnet *, int);

struct virtif_sc {
	struct ethercom sc_ec;
	int sc_tapfd;
	kmutex_t sc_sendmtx;
	kcondvar_t sc_sendcv;
};

static void virtif_worker(void *);
static void virtif_sender(void *);

#if 0
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
#endif

int
rump_virtif_create(int num)
{
	struct virtif_sc *sc;
	struct ifnet *ifp;
	uint8_t enaddr[ETHER_ADDR_LEN] = { 0xb2, 0x0a, 0x00, 0x0b, 0x0e, 0x01 };
	char tapdev[16];
	int fd, error;

	snprintf(tapdev, sizeof(tapdev), "/dev/tap%d", num);
	fd = rumpuser_open(tapdev, O_RDWR, &error);
	if (fd == -1) {
		printf("virtif_create: can't open /dev/tap %d\n", error);
		return error;
	}
	KASSERT(num < 0x100);
	enaddr[2] = arc4random() & 0xff;
	enaddr[5] = num;

	sc = kmem_zalloc(sizeof(*sc), KM_SLEEP);
	sc->sc_tapfd = fd;

	ifp = &sc->sc_ec.ec_if;
	sprintf(ifp->if_xname, "%s%d", VIRTIF_BASE, num);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = virtif_init;
	ifp->if_ioctl = virtif_ioctl;
	ifp->if_start = virtif_start;
	ifp->if_stop = virtif_stop;

	mutex_init(&sc->sc_sendmtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_sendcv, "virtsnd");

	if_attach(ifp);
	ether_ifattach(ifp, enaddr);

	return 0;
}

static int
virtif_init(struct ifnet *ifp)
{
	int rv;

	if (rump_threads) {
		rv = kthread_create(PRI_NONE, 0, NULL, virtif_worker, ifp,
		    NULL, "virtifi");
		/* XXX: should do proper cleanup */
		if (rv) {
			panic("if_virt: can't create worker");
		}
		rv = kthread_create(PRI_NONE, 0, NULL, virtif_sender, ifp,
		    NULL, "virtifs");
		if (rv) {
			panic("if_virt: can't create sender");
		}
	} else {
		printf("WARNING: threads not enabled, receive NOT working\n");
	}
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

	mutex_enter(&sc->sc_sendmtx);
	cv_signal(&sc->sc_sendcv);
	mutex_exit(&sc->sc_sendmtx);
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

static void
virtif_sender(void *arg)
{
	struct ifnet *ifp = arg;
	struct virtif_sc *sc = ifp->if_softc;
	struct mbuf *m, *m0;
	struct rumpuser_iovec io[16];
	int i, error;

	mutex_enter(&sc->sc_sendmtx);
	for (;;) {
		IF_DEQUEUE(&ifp->if_snd, m0);
		if (!m0) {
			cv_wait(&sc->sc_sendcv, &sc->sc_sendmtx);
			continue;
		}
		mutex_exit(&sc->sc_sendmtx);

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
		mutex_enter(&sc->sc_sendmtx);
	}

	mutex_exit(softnet_lock);
}
