/*	$NetBSD: sockin.c,v 1.3.2.2 2008/10/19 22:18:08 haad Exp $	*/

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

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <rump/rumpuser.h>

/*
 * An inet communication domain which uses the socket interface.
 * Currently supports only IPv4 UDP, but could easily be extended to
 * support IPv6 and TCP by adding more stuff to the protosw.
 */

DOMAIN_DEFINE(sockindomain);

static void	sockin_init(void);
static int	sockin_usrreq(struct socket *, int, struct mbuf *,
			      struct mbuf *, struct mbuf *, struct lwp *);

const struct protosw sockinsw[] = {
{
	.pr_type = SOCK_DGRAM,
	.pr_domain = &sockindomain,
	.pr_protocol = IPPROTO_UDP,
	.pr_flags = PR_ATOMIC|PR_ADDR,
	.pr_usrreq = sockin_usrreq,
},
{
	.pr_type = SOCK_STREAM,
	.pr_domain = &sockindomain,
	.pr_protocol = IPPROTO_TCP,
	.pr_flags = PR_CONNREQUIRED|PR_WANTRCVD|PR_LISTEN|PR_ABRTACPTDIS,
	.pr_usrreq = sockin_usrreq,
}};

struct domain sockindomain = {
	.dom_family = PF_INET,
	.dom_name = "socket_inet",
	.dom_init = sockin_init,
	.dom_externalize = NULL,
	.dom_dispose = NULL,
	.dom_protosw = sockinsw,
	.dom_protoswNPROTOSW = &sockinsw[__arraycount(sockinsw)], 
	.dom_rtattach = NULL,
	.dom_rtoffset = 0,
	.dom_maxrtkey = 0,
	.dom_ifattach = NULL,
	.dom_ifdetach = NULL,
	.dom_ifqueues = { NULL },
	.dom_link = { NULL },
	.dom_mowner = MOWNER_INIT("",""),
	.dom_rtcache = { NULL },
	.dom_sockaddr_cmp = NULL
};

/* only for testing */
#if 0
#define SOCKIN_NOTHREAD
#endif

#define SO2S(so) ((intptr_t)(so->so_internal))
#define SOCKIN_SBSIZE 65536

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

	plen = IP_MAXPACKET;
	m = m_gethdr(M_WAIT, MT_DATA);
	MEXTMALLOC(m, plen, M_WAIT);

	memset(&rmsg, 0, sizeof(rmsg));
	io.iov_base = mtod(m, void *);
	io.iov_len = plen;
	rmsg.msg_iov = &io;
	rmsg.msg_iovlen = 1;
	rmsg.msg_name = (struct sockaddr *)&from;
	rmsg.msg_namelen = sizeof(from);

	n = rumpuser_net_recvmsg(SO2S(so), &rmsg, 0, &error);
	if (n <= 0) {
		m_freem(m);
		return;
	}
	m->m_len = m->m_pkthdr.len = n;

	if (so->so_proto->pr_type == SOCK_DGRAM) {
		if (!sbappendaddr(&so->so_rcv, rmsg.msg_name, m, NULL)) {
			m_freem(m);
		}
	} else {
		sbappendstream(&so->so_rcv, m);
	}

	sorwakeup(so);
}

struct sockin_unit {
	struct socket *su_so;

	LIST_ENTRY(sockin_unit) su_entries;
};
static LIST_HEAD(, sockin_unit) su_ent = LIST_HEAD_INITIALIZER(su_ent);
static kmutex_t su_mtx;
static bool rebuild;
static int nsock;

#ifndef SOCKIN_NOTHREAD
#define POLLTIMEOUT 100	/* check for new entries every 100ms */

/* XXX: doesn't handle socket (kernel) locking properly? */
static void
sockinworker(void *arg)
{
	struct pollfd *pfds = NULL, *npfds;
	struct sockin_unit *su_iter;
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
		rv = rumpuser_poll(pfds, cursock, POLLTIMEOUT, &error);
		for (i = 0; i < cursock && rv > 0; i++) {
			if (pfds[i].revents & POLLIN) {
				mutex_enter(&su_mtx);
				LIST_FOREACH(su_iter, &su_ent, su_entries) {
					if (SO2S(su_iter->su_so)==pfds[i].fd) {
						mutex_enter(softnet_lock);
						sockin_process(su_iter->su_so);
						mutex_exit(softnet_lock);
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
#endif /* SOCKIN_NOTHREAD */

static void
sockin_init()
{
#ifndef SOCKIN_NOTHREAD
	int rv;

	if ((rv = kthread_create(PRI_NONE, 0, NULL, sockinworker,
	    NULL, NULL, "sockwork")) != 0)
		panic("sockin_init: could not create worker thread\n");
#endif
	mutex_init(&su_mtx, MUTEX_DEFAULT, IPL_NONE);
}

static int
sockin_usrreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam,
	struct mbuf *control, struct lwp *l)
{
	int error = 0, rv;

	switch (req) {
	case PRU_ATTACH:
	{
		struct sockin_unit *su;
		int news;

		sosetlock(so);
		if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
			error = soreserve(so, SOCKIN_SBSIZE, SOCKIN_SBSIZE);
			if (error)
				break;
		}

		su = kmem_alloc(sizeof(*su), KM_NOSLEEP);
		if (!su) {
			error = ENOMEM;
			break;
		}

		news = rumpuser_net_socket(PF_INET, so->so_proto->pr_type,
		    0, &error);
		if (news == -1) {
			kmem_free(su, sizeof(*su));
			break;
		}
		so->so_internal = (void *)(intptr_t)news;
		su->su_so = so;

		mutex_enter(&su_mtx);
		LIST_INSERT_HEAD(&su_ent, su, su_entries);
		nsock++;
		rebuild = true;
		mutex_exit(&su_mtx);
		break;
	}

	case PRU_CONNECT:
		/* don't bother to connect udp sockets, always sendmsg */
		if (so->so_proto->pr_type == SOCK_DGRAM)
			break;

		rv = rumpuser_net_connect(SO2S(so),
		    mtod(nam, struct sockaddr *), sizeof(struct sockaddr_in),
		    &error);
		if (rv == 0)
		soisconnected(so);
		break;

	case PRU_SEND:
	{
		struct sockaddr *saddr;
		struct msghdr mhdr;
		struct iovec iov[16];
		struct mbuf *m2;
		size_t tot;
		int i, s;

		memset(&mhdr, 0, sizeof(mhdr));

		tot = 0;
		for (i = 0, m2 = m; m2; m2 = m2->m_next, i++) {
			if (i > 16)
				panic("lazy bum");
			iov[i].iov_base = m2->m_data;
			iov[i].iov_len = m2->m_len;
			tot += m2->m_len;

		}
		mhdr.msg_iov = iov;
		mhdr.msg_iovlen = i;
		s = SO2S(so);

		if (so->so_proto->pr_type == SOCK_DGRAM) {
			saddr = mtod(nam, struct sockaddr *);
			mhdr.msg_name = saddr;
			mhdr.msg_namelen = saddr->sa_len;
		}

		rumpuser_net_sendmsg(s, &mhdr, 0, &error);

		m_freem(m);
		m_freem(control);
#ifdef SOCKIN_NOTHREAD
		/* this assumes too many things to list.. buthey, testing */
		sockin_process(so);
#endif
	}
		break;

	case PRU_SHUTDOWN:
	{
		struct sockin_unit *su_iter;

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
		break;

	default:
		panic("sockin_usrreq: IMPLEMENT ME, req %d not supported", req);
	}

	return error;
}
