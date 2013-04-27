/*	$NetBSD: sockin.c,v 1.28 2013/04/27 15:01:21 pooka Exp $	*/

/*
 * Copyright (c) 2008, 2009 Antti Kantee.  All Rights Reserved.
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
__KERNEL_RCSID(0, "$NetBSD: sockin.c,v 1.28 2013/04/27 15:01:21 pooka Exp $");

#include <sys/param.h>
#include <sys/condvar.h>
#include <sys/domain.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/poll.h>
#include <sys/protosw.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/time.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/radix.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <rump/rumpuser.h>

#include "rump_private.h"
#include "rumpcomp_user.h"

/*
 * An inet communication domain which uses the socket interface.
 * Currently supports only IPv4 UDP, but could easily be extended to
 * support IPv6 and TCP by adding more stuff to the protosw.
 */

DOMAIN_DEFINE(sockindomain);

static void	sockin_init(void);
static int	sockin_usrreq(struct socket *, int, struct mbuf *,
			      struct mbuf *, struct mbuf *, struct lwp *);
static int	sockin_ctloutput(int op, struct socket *, struct sockopt *);

const struct protosw sockinsw[] = {
{
	.pr_type = SOCK_DGRAM,
	.pr_domain = &sockindomain,
	.pr_protocol = IPPROTO_UDP,
	.pr_flags = PR_ATOMIC|PR_ADDR,
	.pr_usrreq = sockin_usrreq,
	.pr_ctloutput = sockin_ctloutput,
},
{
	.pr_type = SOCK_STREAM,
	.pr_domain = &sockindomain,
	.pr_protocol = IPPROTO_TCP,
	.pr_flags = PR_CONNREQUIRED|PR_WANTRCVD|PR_LISTEN|PR_ABRTACPTDIS,
	.pr_usrreq = sockin_usrreq,
	.pr_ctloutput = sockin_ctloutput,
}};

struct domain sockindomain = {
	.dom_family = PF_INET,
	.dom_name = "socket_inet",
	.dom_init = sockin_init,
	.dom_externalize = NULL,
	.dom_dispose = NULL,
	.dom_protosw = sockinsw,
	.dom_protoswNPROTOSW = &sockinsw[__arraycount(sockinsw)],
	.dom_rtattach = rt_inithead,
	.dom_rtoffset = 32,
	.dom_maxrtkey = sizeof(struct sockaddr_in),
	.dom_ifattach = NULL,
	.dom_ifdetach = NULL,
	.dom_ifqueues = { NULL },
	.dom_link = { NULL },
	.dom_mowner = MOWNER_INIT("",""),
	.dom_rtcache = { NULL },
	.dom_sockaddr_cmp = NULL
};

#define SO2S(so) ((intptr_t)(so->so_internal))
#define SOCKIN_SBSIZE 65536

struct sockin_unit {
	struct socket *su_so;

	LIST_ENTRY(sockin_unit) su_entries;
};
static LIST_HEAD(, sockin_unit) su_ent = LIST_HEAD_INITIALIZER(su_ent);
static kmutex_t su_mtx;
static bool rebuild;
static int nsock;

/* XXX: for the bpf hack */
static struct ifnet sockin_if;
int ifpromisc(struct ifnet *ifp, int pswitch) { return 0; }

static int
registersock(struct socket *so, int news)
{
	struct sockin_unit *su;

	su = kmem_alloc(sizeof(*su), KM_NOSLEEP);
	if (!su)
		return ENOMEM;

	so->so_internal = (void *)(intptr_t)news;
	su->su_so = so;

	mutex_enter(&su_mtx);
	LIST_INSERT_HEAD(&su_ent, su, su_entries);
	nsock++;
	rebuild = true;
	mutex_exit(&su_mtx);

	return 0;
}

static void
removesock(struct socket *so)
{
	struct sockin_unit *su_iter;
	int error;

	mutex_enter(&su_mtx);
	LIST_FOREACH(su_iter, &su_ent, su_entries) {
		if (su_iter->su_so == so)
			break;
	}
	if (!su_iter)
		panic("no such socket");

	LIST_REMOVE(su_iter, su_entries);
	nsock--;
	rebuild = true;
	mutex_exit(&su_mtx);

	rumpuser_close(SO2S(su_iter->su_so), &error);
	kmem_free(su_iter, sizeof(*su_iter));
}

static void
sockin_process(struct socket *so)
{
	struct sockaddr_in from;
	struct iovec io;
	struct msghdr rmsg;
	struct mbuf *m;
	ssize_t n;
	size_t plen;
	int error;

	m = m_gethdr(M_WAIT, MT_DATA);
	if (so->so_proto->pr_type == SOCK_DGRAM) {
		plen = IP_MAXPACKET;
		MEXTMALLOC(m, plen, M_DONTWAIT);
	} else {
		plen = MCLBYTES;
		MCLGET(m, M_DONTWAIT);
	}
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return;
	}

	memset(&rmsg, 0, sizeof(rmsg));
	io.iov_base = mtod(m, void *);
	io.iov_len = plen;
	rmsg.msg_iov = &io;
	rmsg.msg_iovlen = 1;
	rmsg.msg_name = (struct sockaddr *)&from;
	rmsg.msg_namelen = sizeof(from);

	n = rumpcomp_sockin_recvmsg(SO2S(so), &rmsg, 0, &error);
	if (n <= 0) {
		m_freem(m);

		/* Treat a TCP socket a goner */
		if (error != EAGAIN && so->so_proto->pr_type == SOCK_STREAM) {
			mutex_enter(softnet_lock);
			soisdisconnected(so);
			mutex_exit(softnet_lock);
			removesock(so);
		}
		return;
	}
	m->m_len = m->m_pkthdr.len = n;

	bpf_mtap_af(&sockin_if, AF_UNSPEC, m);

	mutex_enter(softnet_lock);
	if (so->so_proto->pr_type == SOCK_DGRAM) {
		if (!sbappendaddr(&so->so_rcv, rmsg.msg_name, m, NULL)) {
			m_freem(m);
		}
	} else {
		sbappendstream(&so->so_rcv, m);
	}

	sorwakeup(so);
	mutex_exit(softnet_lock);
}

static void
sockin_accept(struct socket *so)
{
	struct socket *nso;
	struct sockaddr_in sin;
	int news, error, slen;

	slen = sizeof(sin);
	news = rumpcomp_sockin_accept(SO2S(so), (struct sockaddr *)&sin,
	    &slen, &error);
	if (news == -1)
		return;

	mutex_enter(softnet_lock);
	nso = sonewconn(so, SS_ISCONNECTED);
	if (nso == NULL)
		goto errout;
	if (registersock(nso, news) != 0)
		goto errout;
	mutex_exit(softnet_lock);
	return;

 errout:
	rumpuser_close(news, &error);
	if (nso)
		soclose(nso);
	mutex_exit(softnet_lock);
}

#define POLLTIMEOUT 100	/* check for new entries every 100ms */

/* XXX: doesn't handle socket (kernel) locking properly? */
static void
sockinworker(void *arg)
{
	struct pollfd *pfds = NULL, *npfds;
	struct sockin_unit *su_iter;
	struct socket *so;
	int cursock = 0, i, rv, error;

	/*
	 * Loop reading requests.  Check for new sockets periodically
	 * (could be smarter, but I'm lazy).
	 */
	for (;;) {
		if (rebuild) {
			npfds = NULL;
			mutex_enter(&su_mtx);
			if (nsock)
				npfds = kmem_alloc(nsock * sizeof(*npfds),
				    KM_NOSLEEP);
			if (npfds || nsock == 0) {
				if (pfds)
					kmem_free(pfds, cursock*sizeof(*pfds));
				pfds = npfds;
				cursock = nsock;
				rebuild = false;

				i = 0;
				LIST_FOREACH(su_iter, &su_ent, su_entries) {
					pfds[i].fd = SO2S(su_iter->su_so);
					pfds[i].events = POLLIN;
					pfds[i].revents = 0;
					i++;
				}
				KASSERT(i == nsock);
			}
			mutex_exit(&su_mtx);
		}

		/* find affected sockets & process */
		rv = rumpcomp_sockin_poll(pfds, cursock, POLLTIMEOUT, &error);
		for (i = 0; i < cursock && rv > 0; i++) {
			if (pfds[i].revents & POLLIN) {
				mutex_enter(&su_mtx);
				LIST_FOREACH(su_iter, &su_ent, su_entries) {
					if (SO2S(su_iter->su_so)==pfds[i].fd) {
						so = su_iter->su_so;
						mutex_exit(&su_mtx);
						if(so->so_options&SO_ACCEPTCONN)
							sockin_accept(so);
						else
							sockin_process(so);
						mutex_enter(&su_mtx);
						break;
					}
				}
				/* if we can't find it, just wing it */
				KASSERT(rebuild || su_iter);
				mutex_exit(&su_mtx);
				pfds[i].revents = 0;
				rv--;
				i = -1;
				continue;
			}

			/* something else?  ignore */
			if (pfds[i].revents) {
				pfds[i].revents = 0;
				rv--;
			}
		}
		KASSERT(rv <= 0);
	}
	
}

static void
sockin_init(void)
{
	int rv;

	if (rump_threads) {
		if ((rv = kthread_create(PRI_NONE, 0, NULL, sockinworker,
		    NULL, NULL, "sockwork")) != 0)
			panic("sockin_init: could not create worker thread\n");
	} else {
		printf("sockin_init: no threads => no worker thread\n");
	}
	mutex_init(&su_mtx, MUTEX_DEFAULT, IPL_NONE);
	strlcpy(sockin_if.if_xname, "sockin0", sizeof(sockin_if.if_xname));
	bpf_attach(&sockin_if, DLT_NULL, 0);
}

static int
sockin_usrreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam,
	struct mbuf *control, struct lwp *l)
{
	int error = 0, rv;

	switch (req) {
	case PRU_ATTACH:
	{
		int news, dummy;
		int sbsize;

		sosetlock(so);
		if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
			error = soreserve(so, SOCKIN_SBSIZE, SOCKIN_SBSIZE);
			if (error)
				break;
		}

		news = rumpcomp_sockin_socket(PF_INET, so->so_proto->pr_type,
		    0, &error);
		if (news == -1)
			break;

		/* for UDP sockets, make sure we can send&recv max */
		if (so->so_proto->pr_type == SOCK_DGRAM) {
			sbsize = SOCKIN_SBSIZE;
			rumpcomp_sockin_setsockopt(news,
			    SOL_SOCKET, SO_SNDBUF,
			    &sbsize, sizeof(sbsize), &error);
			sbsize = SOCKIN_SBSIZE;
			rumpcomp_sockin_setsockopt(news,
			    SOL_SOCKET, SO_RCVBUF,
			    &sbsize, sizeof(sbsize), &error);
		}

		if ((error = registersock(so, news)) != 0)
			rumpuser_close(news, &dummy);

		break;
	}

	case PRU_ACCEPT:
		/* we do all the work in the worker thread */
		break;

	case PRU_BIND:
		rumpcomp_sockin_bind(SO2S(so),
		    mtod(nam, const struct sockaddr *),
		    sizeof(struct sockaddr_in), &error);
		break;

	case PRU_CONNECT:
		rv = rumpcomp_sockin_connect(SO2S(so),
		    mtod(nam, struct sockaddr *), sizeof(struct sockaddr_in),
		    &error);
		if (rv == 0)
			soisconnected(so);
		break;

	case PRU_LISTEN:
		rumpcomp_sockin_listen(SO2S(so), so->so_qlimit, &error);
		break;

	case PRU_SEND:
	{
		struct sockaddr *saddr;
		struct msghdr mhdr;
		size_t iov_max, i;
		struct iovec iov_buf[32], *iov;
		struct mbuf *m2;
		size_t tot;
		int s;

		bpf_mtap_af(&sockin_if, AF_UNSPEC, m);

		memset(&mhdr, 0, sizeof(mhdr));

		iov_max = 0;
		for (m2 = m; m2 != NULL; m2 = m2->m_next) {
			iov_max++;
		}

		if (iov_max <= __arraycount(iov_buf)) {
			iov = iov_buf;
		} else {
			iov = kmem_alloc(sizeof(struct iovec) * iov_max,
			    KM_SLEEP);
		}

		tot = 0;
		for (i = 0, m2 = m; m2 != NULL; m2 = m2->m_next, i++) {
			iov[i].iov_base = m2->m_data;
			iov[i].iov_len = m2->m_len;
			tot += m2->m_len;
		}
		mhdr.msg_iov = iov;
		mhdr.msg_iovlen = i;
		s = SO2S(so);

		if (nam != NULL) {
			saddr = mtod(nam, struct sockaddr *);
			mhdr.msg_name = saddr;
			mhdr.msg_namelen = saddr->sa_len;
		}

		rumpcomp_sockin_sendmsg(s, &mhdr, 0, &error);

		if (iov != iov_buf)
			kmem_free(iov, sizeof(struct iovec) * iov_max);

		m_freem(m);
		m_freem(control);

		/* this assumes too many things to list.. buthey, testing */
		if (!rump_threads)
			sockin_process(so);
	}
		break;

	case PRU_SHUTDOWN:
		removesock(so);
		break;

	case PRU_SOCKADDR:
	case PRU_PEERADDR:
	{
		int slen = nam->m_len;
		enum rumpcomp_sockin_getnametype which;

		if (req == PRU_SOCKADDR)
			which = RUMPCOMP_SOCKIN_SOCKNAME;
		else
			which = RUMPCOMP_SOCKIN_PEERNAME;
		rumpcomp_sockin_getname(SO2S(so),
		    mtod(nam, struct sockaddr *), &slen, which, &error);
		if (error == 0)
			nam->m_len = slen;
		break;
	}

	case PRU_CONTROL:
		error = ENOTTY;
		break;

	default:
		panic("sockin_usrreq: IMPLEMENT ME, req %d not supported", req);
	}

	return error;
}

static int
sockin_ctloutput(int op, struct socket *so, struct sockopt *sopt)
{
	int error;

	rumpcomp_sockin_setsockopt(SO2S(so), sopt->sopt_level,
	    sopt->sopt_name, sopt->sopt_data, sopt->sopt_size, &error);
	return error;
}
