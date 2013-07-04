/*	$NetBSD: if_virt.c,v 1.36 2013/07/04 11:46:51 pooka Exp $	*/

/*
 * Copyright (c) 2008, 2013 Antti Kantee.  All Rights Reserved.
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
__KERNEL_RCSID(0, "$NetBSD: if_virt.c,v 1.36 2013/07/04 11:46:51 pooka Exp $");

#include <sys/param.h>
#include <sys/condvar.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/mutex.h>
#include <sys/poll.h>
#include <sys/sockio.h>
#include <sys/socketvar.h>
#include <sys/cprng.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_tap.h>

#include <netinet/in.h>
#include <netinet/in_var.h>

#include <rump/rump.h>

#include "rump_private.h"
#include "rump_net_private.h"

#include "if_virt.h"
#include "rumpcomp_user.h"

/*
 * Virtual interface.  Uses hypercalls to shovel packets back
 * and forth.  The exact method for shoveling depends on the
 * hypercall implementation.
 */

static int	virtif_init(struct ifnet *);
static int	virtif_ioctl(struct ifnet *, u_long, void *);
static void	virtif_start(struct ifnet *);
static void	virtif_stop(struct ifnet *, int);

struct virtif_sc {
	struct ethercom sc_ec;
	struct virtif_user *sc_viu;
	bool sc_dying;
	struct lwp *sc_l_snd, *sc_l_rcv;
	kmutex_t sc_mtx;
	kcondvar_t sc_cv;
};

static void virtif_receiver(void *);
static void virtif_sender(void *);
static int  virtif_clone(struct if_clone *, int);
static int  virtif_unclone(struct ifnet *);

struct if_clone VIF_CLONER =
    IF_CLONE_INITIALIZER(VIF_NAME, virtif_clone, virtif_unclone);

static int
virtif_clone(struct if_clone *ifc, int num)
{
	struct virtif_sc *sc;
	struct virtif_user *viu;
	struct ifnet *ifp;
	uint8_t enaddr[ETHER_ADDR_LEN] = { 0xb2, 0x0a, 0x00, 0x0b, 0x0e, 0x01 };
	int error = 0;

	if (num >= 0x100)
		return E2BIG;

	if ((error = VIFHYPER_CREATE(num, &viu)) != 0)
		return error;

	enaddr[2] = cprng_fast32() & 0xff;
	enaddr[5] = num;

	sc = kmem_zalloc(sizeof(*sc), KM_SLEEP);
	sc->sc_dying = false;
	sc->sc_viu = viu;

	mutex_init(&sc->sc_mtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_cv, VIF_NAME "snd");
	ifp = &sc->sc_ec.ec_if;
	sprintf(ifp->if_xname, "%s%d", VIF_NAME, num);
	ifp->if_softc = sc;

	if (rump_threads) {
		if ((error = kthread_create(PRI_NONE, KTHREAD_MUSTJOIN, NULL,
		    virtif_receiver, ifp, &sc->sc_l_rcv, VIF_NAME "ifr")) != 0)
			goto out;

		if ((error = kthread_create(PRI_NONE,
		    KTHREAD_MUSTJOIN | KTHREAD_MPSAFE, NULL,
		    virtif_sender, ifp, &sc->sc_l_snd, VIF_NAME "ifs")) != 0)
			goto out;
	} else {
		printf("WARNING: threads not enabled, receive NOT working\n");
	}

	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = virtif_init;
	ifp->if_ioctl = virtif_ioctl;
	ifp->if_start = virtif_start;
	ifp->if_stop = virtif_stop;
	IFQ_SET_READY(&ifp->if_snd);

	if_attach(ifp);
	ether_ifattach(ifp, enaddr);

 out:
	if (error) {
		virtif_unclone(ifp);
	}

	return error;
}

static int
virtif_unclone(struct ifnet *ifp)
{
	struct virtif_sc *sc = ifp->if_softc;

	mutex_enter(&sc->sc_mtx);
	if (sc->sc_dying) {
		mutex_exit(&sc->sc_mtx);
		return EINPROGRESS;
	}
	sc->sc_dying = true;
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_mtx);

	VIFHYPER_DYING(sc->sc_viu);

	virtif_stop(ifp, 1);
	if_down(ifp);

	if (sc->sc_l_snd) {
		kthread_join(sc->sc_l_snd);
		sc->sc_l_snd = NULL;
	}
	if (sc->sc_l_rcv) {
		kthread_join(sc->sc_l_rcv);
		sc->sc_l_rcv = NULL;
	}

	VIFHYPER_DESTROY(sc->sc_viu);

	mutex_destroy(&sc->sc_mtx);
	cv_destroy(&sc->sc_cv);
	kmem_free(sc, sizeof(*sc));

	ether_ifdetach(ifp);
	if_detach(ifp);

	return 0;
}

static int
virtif_init(struct ifnet *ifp)
{
	struct virtif_sc *sc = ifp->if_softc;

	ifp->if_flags |= IFF_RUNNING;

	mutex_enter(&sc->sc_mtx);
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_mtx);
	
	return 0;
}

static int
virtif_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	int s, rv;

	s = splnet();
	rv = ether_ioctl(ifp, cmd, data);
	if (rv == ENETRESET)
		rv = 0;
	splx(s);

	return rv;
}

static void
virtif_start(struct ifnet *ifp)
{
	struct virtif_sc *sc = ifp->if_softc;

	mutex_enter(&sc->sc_mtx);
	ifp->if_flags |= IFF_OACTIVE;
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_mtx);
}

static void
virtif_stop(struct ifnet *ifp, int disable)
{
	struct virtif_sc *sc = ifp->if_softc;

	ifp->if_flags &= ~IFF_RUNNING;

	mutex_enter(&sc->sc_mtx);
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_mtx);
}

#define POLLTIMO_MS 1
static void
virtif_receiver(void *arg)
{
	struct ifnet *ifp = arg;
	struct virtif_sc *sc = ifp->if_softc;
	struct mbuf *m;
	size_t plen = ETHER_MAX_LEN_JUMBO+1;
	size_t n;
	int error;

	for (;;) {
		m = m_gethdr(M_WAIT, MT_DATA);
		MEXTMALLOC(m, plen, M_WAIT);

 again:
		if (sc->sc_dying) {
			m_freem(m);
			break;
		}
		
		error = VIFHYPER_RECV(sc->sc_viu,
		    mtod(m, void *), plen, &n);
		if (error) {
			printf("%s: read hypercall failed %d. host if down?\n",
			    ifp->if_xname, error);
			mutex_enter(&sc->sc_mtx);
			/* could check if need go, done soon anyway */
			cv_timedwait(&sc->sc_cv, &sc->sc_mtx, hz);
			mutex_exit(&sc->sc_mtx);
			goto again;
		}

		/* tap sometimes returns EOF.  don't sweat it and plow on */
		if (__predict_false(n == 0))
			goto again;

		/* discard if we're not up */
		if ((ifp->if_flags & IFF_RUNNING) == 0)
			goto again;

		m->m_len = m->m_pkthdr.len = n;
		m->m_pkthdr.rcvif = ifp;
		bpf_mtap(ifp, m);
		ether_input(ifp, m);
	}

	kthread_exit(0);
}

/* lazy bum stetson-harrison magic value */
#define LB_SH 32
static void
virtif_sender(void *arg)
{
	struct ifnet *ifp = arg;
	struct virtif_sc *sc = ifp->if_softc;
	struct mbuf *m, *m0;
	struct iovec io[LB_SH];
	int i;

	mutex_enter(&sc->sc_mtx);
	KERNEL_LOCK(1, NULL);
	while (!sc->sc_dying) {
		if (!(ifp->if_flags & IFF_RUNNING)) {
			cv_wait(&sc->sc_cv, &sc->sc_mtx);
			continue;
		}
		IF_DEQUEUE(&ifp->if_snd, m0);
		if (!m0) {
			ifp->if_flags &= ~IFF_OACTIVE;
			cv_wait(&sc->sc_cv, &sc->sc_mtx);
			continue;
		}
		mutex_exit(&sc->sc_mtx);

		m = m0;
		for (i = 0; i < LB_SH && m; i++) {
			io[i].iov_base = mtod(m, void *);
			io[i].iov_len = m->m_len;
			m = m->m_next;
		}
		if (i == LB_SH)
			panic("lazy bum");
		bpf_mtap(ifp, m0);

		VIFHYPER_SEND(sc->sc_viu, io, i);

		m_freem(m0);
		mutex_enter(&sc->sc_mtx);
	}
	KERNEL_UNLOCK_LAST(curlwp);

	mutex_exit(&sc->sc_mtx);

	kthread_exit(0);
}
