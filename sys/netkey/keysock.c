/*
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

/* KAME @(#)$Id: keysock.c,v 1.1.2.1 1999/06/28 06:37:10 itojun Exp $ */

#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include "opt_inet.h"
#endif

/* This code has derived from sys/net/rtsock.c on FreeBSD2.2.5 */

#ifdef __NetBSD__
# ifdef _KERNEL
#  define KERNEL
# endif
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#ifdef __FreeBSD__
#include <sys/sysctl.h>
#endif
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#ifdef __NetBSD__
#include <sys/proc.h>
#include <sys/queue.h>
#endif

#ifdef __FreeBSD__
#include <machine/spl.h>
#endif

#include <net/raw_cb.h>
#include <net/route.h>

#include <netkey/keyv2.h>
#include <netkey/keydb.h>
#include <netkey/key.h>
#include <netkey/keysock.h>
#include <netkey/key_debug.h>

#include <machine/stdarg.h>

struct sockaddr key_dst = { 2, PF_KEY, };
struct sockaddr key_src = { 2, PF_KEY, };
struct sockproto key_proto = { PF_KEY, PF_KEY_V2 };

static int key_sendup0 __P((struct rawcb *, struct mbuf *, int));

#if 1
#define KMALLOC(p, t, n) \
	((p) = (t) malloc((unsigned long)(n), M_SECA, M_NOWAIT))
#define KFREE(p) \
	free((caddr_t)(p), M_SECA);
#else
#define KMALLOC(p, t, n) \
	do {								\
		((p) = (t)malloc((unsigned long)(n), M_SECA, M_NOWAIT));\
		printf("%s %d: %p <- KMALLOC(%s, %d)\n",		\
			__FILE__, __LINE__, (p), #t, n);		\
	} while (0)

#define KFREE(p) \
	do {								\
		printf("%s %d: %p -> KFREE()\n", __FILE__, __LINE__, (p));\
		free((caddr_t)(p), M_SECA);				\
	} while (0)
#endif

/*
 * key_usrreq()
 * derived from net/rtsock.c:route_usrreq()
 */
#ifndef __NetBSD__
int
key_usrreq(so, req, m, nam, control)
	register struct socket *so;
	int req;
	struct mbuf *m, *nam, *control;
#else
int
key_usrreq(so, req, m, nam, control, p)
	register struct socket *so;
	int req;
	struct mbuf *m, *nam, *control;
	struct proc *p;
#endif /*__NetBSD__*/
{
	register int error = 0;
	register struct keycb *kp = (struct keycb *)sotorawcb(so);
	int s;

	s = splnet();
	if (req == PRU_ATTACH) {
		MALLOC(kp, struct keycb *, sizeof(*kp), M_PCB, M_WAITOK);
		so->so_pcb = (caddr_t)kp;
		if (so->so_pcb)
			bzero(so->so_pcb, sizeof(*kp));
	}
	if (req == PRU_DETACH && kp) {
		int af = kp->kp_raw.rcb_proto.sp_protocol;
		if (af == PF_KEY) /* XXX: AF_KEY */
			key_cb.key_count--;
		key_cb.any_count--;

		key_freereg(so);
	}

#ifndef __NetBSD__
	error = raw_usrreq(so, req, m, nam, control);
#else
	error = raw_usrreq(so, req, m, nam, control, p);
#endif
	m = control = NULL;	/* reclaimed in raw_usrreq */
	kp = (struct keycb *)sotorawcb(so);
	if (req == PRU_ATTACH && kp) {
		int af = kp->kp_raw.rcb_proto.sp_protocol;
		if (error) {
			printf("key_usrreq: key_usrreq results %d\n", error);
			free((caddr_t)kp, M_PCB);
			so->so_pcb = (caddr_t) 0;
			splx(s);
			return(error);
		}

		kp->kp_promisc = kp->kp_registered = 0;

		if (af == PF_KEY) /* XXX: AF_KEY */
			key_cb.key_count++;
		key_cb.any_count++;
#ifndef __bsdi__
		kp->kp_raw.rcb_laddr = &key_src;
		kp->kp_raw.rcb_faddr = &key_dst;
#else
		/*
		 * XXX rcb_faddr must be dynamically allocated, otherwise
		 * raw_disconnect() will be angry.
		 */
	    {
		struct mbuf *m, *n;
		MGET(m, M_WAITOK, MT_DATA);
		if (!m) {
			error = ENOBUFS;
			printf("key_usrreq: key_usrreq results %d\n", error);
			free((caddr_t)kp, M_PCB);
			so->so_pcb = (caddr_t) 0;
			splx(s);
			return(error);
		}
		MGET(n, M_WAITOK, MT_DATA);
		if (!n) {
			error = ENOBUFS;
			m_freem(m);
			printf("key_usrreq: key_usrreq results %d\n", error);
			free((caddr_t)kp, M_PCB);
			so->so_pcb = (caddr_t) 0;
			splx(s);
			return(error);
		}
		m->m_len = sizeof(key_src);
		kp->kp_raw.rcb_laddr = mtod(m, struct sockaddr *);
		bcopy(&key_src, kp->kp_raw.rcb_laddr, sizeof(key_src));
		n->m_len = sizeof(key_dst);
		kp->kp_raw.rcb_faddr = mtod(n, struct sockaddr *);
		bcopy(&key_dst, kp->kp_raw.rcb_faddr, sizeof(key_dst));
	    }
#endif
		soisconnected(so);
		so->so_options |= SO_USELOOPBACK;
	}
	splx(s);
	return(error);
}

/*
 * key_output()
 */
int
#if __STDC__
key_output(struct mbuf *m, ...)
#else
key_output(m, va_alist)
	struct mbuf *m;
	va_dcl
#endif
{
	struct sadb_msg *msg = NULL;
	int len, error = 0;
	int s;
	int target;
	struct socket *so;
	va_list ap;

	va_start(ap, m);
	so = va_arg(ap, struct socket *);
	va_end(ap);

	if (m == 0)
		panic("key_output: NULL pointer was passed.\n");

	if (m->m_len < sizeof(long)
	 && (m = m_pullup(m, 8)) == 0) {
		printf("key_output: can't pullup mbuf\n");
		error = ENOBUFS;
		goto end;
	}

	if ((m->m_flags & M_PKTHDR) == 0)
		panic("key_output: not M_PKTHDR ??");

#if defined(IPSEC_DEBUG)
	KEYDEBUG(KEYDEBUG_KEY_DUMP, kdebug_mbuf(m));
#endif /* defined(IPSEC_DEBUG) */

	len = m->m_pkthdr.len;
	if (len < sizeof(struct sadb_msg)
	 || len != PFKEY_UNUNIT64(mtod(m, struct sadb_msg *)->sadb_msg_len)) {
		printf("key_output: Invalid message length.\n");
		error = EINVAL;
		goto end;
	}

	/*
	 * allocate memory for sadb_msg, and copy to sadb_msg from mbuf
	 * XXX: To be processed directly without a copy.
	 */
	KMALLOC(msg, struct sadb_msg *, len);
	if (msg == 0) {
		printf("key_output: No more memory.\n");
		error = ENOBUFS;
		goto end;
		/* or do panic ? */
	}
	m_copydata(m, 0, len, (caddr_t)msg);

	s = splnet();	/*XXX giant lock*/
	if ((len = key_parse(&msg, so, &target)) == 0) {
		/* discard. i.e. no need to reply. */
		error = 0;
		splx(s);
		goto end;
	}

	/* send up message to the socket */
	error = key_sendup(so, msg, len, target);
	splx(s);
	KFREE(msg);
end:
	m_freem(m);
	return (error);
}

/*
 * send message to the socket.
 */
static int
key_sendup0(rp, m, promisc)
	struct rawcb *rp;
	struct mbuf *m;
	int promisc;
{
	if (promisc) {
		struct sadb_msg *pmsg;

		M_PREPEND(m, sizeof(struct sadb_msg), M_NOWAIT);
		if (m && m->m_len < sizeof(struct sadb_msg))
			m = m_pullup(m, sizeof(struct sadb_msg));
		if (!m) {
			printf("key_sendup0: cannot pullup\n");
			m_freem(m);
			return ENOBUFS;
		}
		m->m_pkthdr.len += sizeof(*pmsg);

		pmsg = mtod(m, struct sadb_msg *);
		bzero(pmsg, sizeof(*pmsg));
		pmsg->sadb_msg_version = PF_KEY_V2;
		pmsg->sadb_msg_type = SADB_X_PROMISC;
		pmsg->sadb_msg_len = PFKEY_UNIT64(m->m_pkthdr.len);
		/* pid and seq? */
	}

	if (!sbappendaddr(&rp->rcb_socket->so_rcv,
			(struct sockaddr *)&key_src, m, NULL)) {
		printf("key_sendup0: sbappendaddr failed\n");
		m_freem(m);
		return ENOBUFS;
	}
	sorwakeup(rp->rcb_socket);
	return 0;
}

int
key_sendup(so, msg, len, target)
	struct socket *so;
	struct sadb_msg *msg;
	u_int len;
	int target;	/*target of the resulting message*/
{
	struct mbuf *m, *n, *mprev;
	struct keycb *kp;
	int sendup;
	struct rawcb *rp;
	int error;
	int tlen;

	/* sanity check */
	if (so == 0 || msg == 0)
		panic("key_sendup: NULL pointer was passed.\n");

	KEYDEBUG(KEYDEBUG_KEY_DUMP,
		printf("key_sendup: \n");
		kdebug_sadb(msg));

	/*
	 * Get mbuf chain whenever possible (not clusters),
	 * to save socket buffer.  We'll be generating many SADB_ACQUIRE
	 * messages to listening key sockets.  If we simmply allocate clusters,
	 * sbappendaddr() will raise ENOBUFS due to too little sbspace().
	 * sbspace() computes # of actual data bytes AND mbuf region.
	 *
	 * TODO: SADB_ACQUIRE filters should be implemented.
	 */
	tlen = len;
	m = mprev = NULL;
	while (tlen > 0) {
		if (tlen == len) {
			MGETHDR(n, M_DONTWAIT, MT_DATA);
			n->m_len = MHLEN;
		} else {
			MGET(n, M_DONTWAIT, MT_DATA);
			n->m_len = MLEN;
		}
		if (!n)
			return ENOBUFS;
		if (tlen > MCLBYTES) {	/*XXX better threshold? */
			MCLGET(n, M_DONTWAIT);
			if ((n->m_flags & M_EXT) == 0) {
				m_free(n);
				m_freem(m);
				return ENOBUFS;
			}
			n->m_len = MCLBYTES;
		}

		if (tlen < n->m_len)
			n->m_len = tlen;
		n->m_next = NULL;
		if (m == NULL)
			m = mprev = n;
		else {
			mprev->m_next = n;
			mprev = n;
		}
		tlen -= n->m_len;
		n = NULL;
	}
	m->m_pkthdr.len = len;
	m->m_pkthdr.rcvif = NULL;
	m_copyback(m, 0, len, (caddr_t)msg);

#ifndef __NetBSD__
	for (rp = rawcb.rcb_next; rp != &rawcb; rp = rp->rcb_next)
#else
	for (rp = rawcb.lh_first; rp; rp = rp->rcb_list.le_next)
#endif
	{
		if (rp->rcb_proto.sp_family != PF_KEY)
			continue;
		if (rp->rcb_proto.sp_protocol
		 && rp->rcb_proto.sp_protocol != PF_KEY_V2) {
			continue;
		}

		kp = (struct keycb *)rp;

		/*
		 * If you are in promiscuous mode, and when you get broadcasted
		 * reply, you'll get two PF_KEY messages.
		 * (based on pf_key@inner.net message on 14 Oct 1998)
		 */
		if (((struct keycb *)rp)->kp_promisc) {
			if ((n = m_copy(m, 0, (int)M_COPYALL)) != NULL) {
				(void)key_sendup0(rp, n, 1);
				n = NULL;
			}
		}

		/* the exact target will be processed later */
		if (sotorawcb(so) == rp)
			continue;

		sendup = 0;
		switch (target) {
		case KEY_SENDUP_ONE:
			/* the statement has no effect */
			if (sotorawcb(so) == rp)
				sendup++;
			break;
		case KEY_SENDUP_ALL:
			sendup++;
			break;
		case KEY_SENDUP_REGISTERED:
			if (kp->kp_registered)
				sendup++;
			break;
		}

		if (!sendup)
			continue;

		if ((n = m_copy(m, 0, (int)M_COPYALL)) == NULL) {
			printf("key_sendup: m_copy fail\n");
			m_freem(m);
			return ENOBUFS;
		}

		if ((error = key_sendup0(rp, n, 0)) != 0) {
			m_freem(m);
			return error;
		}

		n = NULL;
	}

	error = key_sendup0(sotorawcb(so), m, 0);
	m = NULL;
	return error;
}

#ifdef __FreeBSD__
/* sysctl */
SYSCTL_NODE(_net, PF_KEY, key, CTLFLAG_RW, 0, "Key Family");
#endif

/*
 * Definitions of protocols supported in the KEY domain.
 */

extern struct domain keydomain;

struct protosw keysw[] = {
{ SOCK_RAW,	&keydomain,	PF_KEY_V2,	PR_ATOMIC|PR_ADDR,
  0,		key_output,	raw_ctlinput,	0,
  key_usrreq,
  raw_init,	0,		0,		0,
#if defined(__bsdi__) || defined(__NetBSD__)
  key_sysctl,
#endif
}
};

struct domain keydomain =
    { PF_KEY, "key", key_init, 0, 0,
      keysw, &keysw[sizeof(keysw)/sizeof(keysw[0])] };

#ifdef __FreeBSD__
DOMAIN_SET(key);
#endif
