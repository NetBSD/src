/*	$NetBSD: raw_ip.c,v 1.116.2.2 2013/08/28 15:21:48 rmind Exp $	*/

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

/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)raw_ip.c	8.7 (Berkeley) 5/15/95
 */

/*
 * Raw interface to IP protocol.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: raw_ip.c,v 1.116.2.2 2013/08/28 15:21:48 rmind Exp $");

#include "opt_inet.h"
#include "opt_compat_netbsd.h"
#include "opt_ipsec.h"
#include "opt_mrouting.h"

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/socketvar.h>
#include <sys/systm.h>
#include <sys/kauth.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_private.h>
#include <netinet/ip_mroute.h>
#include <netinet/ip_icmp.h>
#include <netinet/in_pcb.h>
#include <netinet/in_proto.h>
#include <netinet/in_var.h>

#ifdef IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/ipsec_var.h>
#include <netipsec/ipsec_private.h>
#endif

#ifdef COMPAT_50
#include <compat/sys/socket.h>
#endif

static inpcbtable_t *	rawcbtable __read_mostly;

static void		sysctl_net_inet_raw_setup(struct sysctllog **);

/*
 * Nominal space allocated to a raw ip socket.
 */
#define	RIPSNDQ		8192
#define	RIPRCVQ		8192

static u_long		rip_sendspace = RIPSNDQ;
static u_long		rip_recvspace = RIPRCVQ;

struct rip_input_ctx {
	struct mbuf *		mbuf;
	struct ip *		ip;
	struct sockaddr_in	src;
	unsigned		hlen;
	unsigned		nfound;
};

struct rip_ctlinput_ctx {
	struct ip *		ip;
	struct in_addr		addr;
	int			errno;
};

void
rip_init(void)
{
	rawcbtable = inpcb_init(1, 1, 0);
	sysctl_net_inet_raw_setup(NULL);
}

/*
 * rip_append: pass the received datagram to the process.
 */
static void
rip_append(inpcb_t *inp, struct rip_input_ctx *rctx)
{
	struct socket *so = inpcb_get_socket(inp);
	int inpflags = inpcb_get_flags(inp);
	struct mbuf *n, *opts = NULL;

	/* XXX: Might optimise this, but not with a silly loop! */
	if ((n = m_copypacket(rctx->mbuf, M_DONTWAIT)) == NULL) {
		return;
	}

	if (inpflags & INP_NOHEADER) {
		m_adj(n, rctx->hlen);
	}

	if ((inpflags & INP_CONTROLOPTS) != 0
#ifdef SO_OTIMESTAMP
	    || (so->so_options & SO_OTIMESTAMP) != 0
#endif
	    || (so->so_options & SO_TIMESTAMP) != 0) {
		struct ip *ip = rctx->ip;
		ip_savecontrol(inp, &opts, ip, n);
	}

	if (sbappendaddr(&so->so_rcv, sintosa(&rctx->src), n, opts) == 0) {
		/* Should notify about lost packet. */
		if (opts) {
			m_freem(opts);
		}
		m_freem(n);
	} else {
		sorwakeup(so);
	}
}

static int
rip_pcb_process(inpcb_t *inp, void *arg)
{
	struct rip_input_ctx *rctx = arg;
	const struct ip *ip = rctx->ip;
	struct ip *inp_ip = in_getiphdr(inp);
	struct in_addr laddr, faddr;

	if (inp_ip->ip_p && inp_ip->ip_p != ip->ip_p) {
		return 0;
	}
	inpcb_get_addrs(inp, &laddr, &faddr);

	if (!in_nullhost(laddr) && !in_hosteq(laddr, ip->ip_dst)) {
		return 0;
	}
	if (!in_nullhost(faddr) && !in_hosteq(faddr, ip->ip_src)) {
		return 0;
	}

#if defined(IPSEC)
	/* Check AH/ESP integrity. */
	if (ipsec4_in_reject_so(rctx->mbuf, inpcb_get_socket(inp))) {
		/* Do not inject data into PCB. */
		IPSEC_STATINC(IPSEC_STAT_IN_POLVIO);
		return 0;
	}
#endif
	rip_append(inp, rctx);
	rctx->nfound++;
	return 0;
}

void
rip_input(struct mbuf *m, ...)
{
	struct ip *ip = mtod(m, struct ip *);
	int error, hlen, proto;
	va_list ap;

	va_start(ap, m);
	(void)va_arg(ap, int);		/* ignore value, advance ap */
	proto = va_arg(ap, int);
	va_end(ap);

	KASSERTMSG((proto == ip->ip_p), "%s: protocol mismatch", __func__);

	/*
	 * Compatibility: programs using raw IP expect ip_len field to have
	 * the header length subtracted.  Also, ip_len and ip_off fields are
	 * expected to be in host order.
	 */
	hlen = ip->ip_hl << 2;
	ip->ip_len = ntohs(ip->ip_len) - hlen;
	NTOHS(ip->ip_off);

	/* Save some context for the iterator. */
	struct rip_input_ctx rctx = {
		.mbuf = m, .ip = ip, .hlen = hlen, .nfound = 0
	};
	sockaddr_in_init(&rctx.src, &ip->ip_src, 0);

	/* Scan all raw IP PCBs for matching entries. */
	error = inpcb_foreach(rawcbtable, AF_INET, rip_pcb_process, &rctx);
	KASSERT(error == 0);

	/* Done, if found any. */
	if (rctx.nfound) {
		return;
	}

	if (inetsw[ip_protox[ip->ip_p]].pr_input == rip_input) {
		uint64_t *ips;

		icmp_error(m, ICMP_UNREACH, ICMP_UNREACH_PROTOCOL, 0, 0);
		ips = IP_STAT_GETREF();
		ips[IP_STAT_NOPROTO]++;
		ips[IP_STAT_DELIVERED]--;
		IP_STAT_PUTREF();
	} else {
		m_freem(m);
	}
}

static int
rip_pcbnotify(inpcb_t *inp, void *arg)
{
	struct rip_ctlinput_ctx *rctx = arg;
	const struct ip *ip = rctx->ip;
	struct ip *inp_ip = in_getiphdr(inp);
	struct in_addr laddr, faddr;

	if (inp_ip->ip_p && inp_ip->ip_p != ip->ip_p) {
		return 0;
	}
	inpcb_get_addrs(inp, &laddr, &faddr);

	if (in_hosteq(faddr, rctx->addr) && in_hosteq(laddr, ip->ip_src)) {
		inpcb_rtchange(inp, rctx->errno);
	}
	return 0;
}

void *
rip_ctlinput(int cmd, const struct sockaddr *sa, void *v)
{
	struct ip *ip = v;
	int errno;

	if (sa->sa_family != AF_INET ||
	    sa->sa_len != sizeof(struct sockaddr_in))
		return NULL;
	if ((unsigned)cmd >= PRC_NCMDS)
		return NULL;
	errno = inetctlerrmap[cmd];

	if (PRC_IS_REDIRECT(cmd) || cmd == PRC_HOSTDEAD || ip == NULL) {
		inpcb_notifyall(rawcbtable, satocsin(sa)->sin_addr,
		    errno, inpcb_rtchange);
		return NULL;
	} else if (errno == 0) {
		return NULL;
	}

	/* Note: mapped address case. */
	struct rip_ctlinput_ctx rctx = {
		.ip = ip, .addr = satocsin(sa)->sin_addr, .errno = errno
	};
	(void)inpcb_foreach(rawcbtable, AF_INET, rip_pcbnotify, &rctx);

	return NULL;
}

/*
 * Generate IP header and pass packet to the IP output routine.
 * Tack on options user may have setup with control call.
 */
int
rip_output(struct mbuf *m, ...)
{
	inpcb_t *inp;
	struct socket *so;
	struct ip *ip;
	struct mbuf *opts;
	int flags, inpflags;
	va_list ap;

	va_start(ap, m);
	inp = va_arg(ap, inpcb_t *);
	va_end(ap);

	so = inpcb_get_socket(inp);
	KASSERT(solocked(so));

	flags = (so->so_options & SO_DONTROUTE) |
	    IP_ALLOWBROADCAST | IP_RETURNMTU;
	inpflags = inpcb_get_flags(inp);

	/*
	 * If the user handed us a complete IP packet, use it.
	 * Otherwise, allocate an mbuf for a header and fill it in.
	 */
	if ((inpflags & INP_HDRINCL) == 0) {
		struct ip *inp_ip = in_getiphdr(inp);

		if ((m->m_pkthdr.len + sizeof(struct ip)) > IP_MAXPACKET) {
			m_freem(m);
			return EMSGSIZE;
		}
		M_PREPEND(m, sizeof(struct ip), M_DONTWAIT);
		if (m == NULL) {
			return ENOBUFS;
		}
		ip = mtod(m, struct ip *);
		ip->ip_tos = 0;
		ip->ip_off = htons(0);
		ip->ip_p = inp_ip->ip_p;
		ip->ip_len = htons(m->m_pkthdr.len);
		inpcb_get_addrs(inp, &ip->ip_src, &ip->ip_dst);

		ip->ip_ttl = MAXTTL;
		opts = inpcb_get_options(inp);
	} else {
		if (m->m_pkthdr.len > IP_MAXPACKET) {
			m_freem(m);
			return EMSGSIZE;
		}
		ip = mtod(m, struct ip *);

		/*
		 * If the mbuf is read-only, we need to allocate a new mbuf
		 * for the header, since we need to modify the header.
		 */
		if (M_READONLY(m)) {
			const int hlen = ip->ip_hl << 2;

			m = m_copyup(m, hlen, (max_linkhdr + 3) & ~3);
			if (m == NULL) {
				return ENOMEM;	/* XXX */
			}
			ip = mtod(m, struct ip *);
		}

		/*
		 * Applications on raw sockets pass us packets
		 * in host byte order.
		 */
		if (m->m_pkthdr.len != ip->ip_len) {
			m_freem(m);
			return (EINVAL);
		}
		HTONS(ip->ip_len);
		HTONS(ip->ip_off);
		if (ip->ip_id || m->m_pkthdr.len < IP_MINFRAGSIZE) {
			flags |= IP_NOIPNEWID;
		}
		opts = NULL;

		/*
		 * Note: prevent IP output from overwriting header fields.
		 */
		flags |= IP_RAWOUTPUT;
		IP_STATINC(IP_STAT_RAWOUT);
	}

	return ip_output(m, opts, inpcb_get_route(inp), flags,
	    inpcb_get_moptions(inp), so);
}

/*
 * Raw IP socket option processing.
 */
int
rip_ctloutput(int op, struct socket *so, struct sockopt *sopt)
{
	inpcb_t *inp = sotoinpcb(so);
	int inpflags = inpcb_get_flags(inp);
	int error = 0, optval;

	KASSERT(solocked(so));

	if (sopt->sopt_level == SOL_SOCKET && sopt->sopt_name == SO_NOHEADER) {
		if (op == PRCO_GETOPT) {
			optval = (inpflags & INP_NOHEADER) ? 1 : 0;
			error = sockopt_set(sopt, &optval, sizeof(optval));
		} else if (op == PRCO_SETOPT) {
			error = sockopt_getint(sopt, &optval);
			if (error)
				goto out;
			if (optval) {
				inpflags &= ~INP_HDRINCL;
				inpflags |= INP_NOHEADER;
			} else
				inpflags &= ~INP_NOHEADER;
		}
		goto out;
	}

	if (sopt->sopt_level != IPPROTO_IP) {
		return ip_ctloutput(op, so, sopt);
	}

	switch (op) {
	case PRCO_SETOPT:
		switch (sopt->sopt_name) {
		case IP_HDRINCL:
			error = sockopt_getint(sopt, &optval);
			if (error)
				break;
			if (optval)
				inpflags |= INP_HDRINCL;
			else
				inpflags &= ~INP_HDRINCL;
			break;

#ifdef MROUTING
		case MRT_INIT:
		case MRT_DONE:
		case MRT_ADD_VIF:
		case MRT_DEL_VIF:
		case MRT_ADD_MFC:
		case MRT_DEL_MFC:
		case MRT_ASSERT:
		case MRT_API_CONFIG:
		case MRT_ADD_BW_UPCALL:
		case MRT_DEL_BW_UPCALL:
			error = ip_mrouter_set(so, sopt);
			break;
#endif

		default:
			error = ip_ctloutput(op, so, sopt);
			break;
		}
		break;

	case PRCO_GETOPT:
		switch (sopt->sopt_name) {
		case IP_HDRINCL:
			optval = inpflags & INP_HDRINCL;
			error = sockopt_set(sopt, &optval, sizeof(optval));
			break;

#ifdef MROUTING
		case MRT_VERSION:
		case MRT_ASSERT:
		case MRT_API_SUPPORT:
		case MRT_API_CONFIG:
			error = ip_mrouter_get(so, sopt);
			break;
#endif

		default:
			error = ip_ctloutput(op, so, sopt);
			break;
		}
		break;
	}
 out:
	if (!error) {
		inpcb_set_flags(inp, inpflags);
	}
	return error;
}

static int
rip_bind(inpcb_t *inp, struct mbuf *nam)
{
	struct sockaddr_in *addr = mtod(nam, struct sockaddr_in *);

	if (nam->m_len != sizeof(*addr))
		return EINVAL;
	if (!IFNET_FIRST())
		return EADDRNOTAVAIL;
	if (addr->sin_family != AF_INET)
		return EAFNOSUPPORT;
	if (!in_nullhost(addr->sin_addr) && !ifa_ifwithaddr(sintosa(addr)))
		return EADDRNOTAVAIL;

	inpcb_set_addrs(inp, &addr->sin_addr, NULL);
	return 0;
}

static int
rip_connect(inpcb_t *inp, struct mbuf *nam)
{
	struct sockaddr_in *addr = mtod(nam, struct sockaddr_in *);

	if (nam->m_len != sizeof(*addr))
		return EINVAL;
	if (!IFNET_FIRST())
		return EADDRNOTAVAIL;
	if (addr->sin_family != AF_INET)
		return EAFNOSUPPORT;

	inpcb_set_addrs(inp, NULL, &addr->sin_addr);
	return 0;
}

static void
rip_disconnect(inpcb_t *inp)
{
	inpcb_set_addrs(inp, NULL, &zeroin_addr);
}

static int
rip_attach(struct socket *so, int proto)
{
	inpcb_t *inp;
	struct ip *ip;
	int error;

	KASSERT(sotoinpcb(so) == NULL);
	sosetlock(so);

	if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
		error = soreserve(so, rip_sendspace, rip_recvspace);
		if (error) {
			return error;
		}
	}

	solock(so);
	error = inpcb_create(so, rawcbtable);
	if (error) {
		sounlock(so);
		return error;
	}
	inp = sotoinpcb(so);
	ip = in_getiphdr(inp);
	ip->ip_p = proto;
	sounlock(so);

	return 0;
}
static void
rip_detach(struct socket *so)
{
	inpcb_t *inp;

	KASSERT(solocked(so));
	inp = sotoinpcb(so);
	KASSERT(inp != NULL);

#ifdef MROUTING
	extern struct socket *ip_mrouter;
	if (so == ip_mrouter) {
		ip_mrouter_done();
	}
#endif
	inpcb_destroy(inp);
}

int
rip_usrreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam,
    struct mbuf *control, struct lwp *l)
{
	inpcb_t *inp;
	int error = 0;

	KASSERT(req != PRU_ATTACH);
	KASSERT(req != PRU_DETACH);

	if (req == PRU_CONTROL) {
		return in_control(so, (long)m, nam, (ifnet_t *)control, l);
	}
	if (req == PRU_PURGEIF) {
		int s = splsoftnet();
		mutex_enter(softnet_lock);
		inpcb_purgeif0(rawcbtable, (ifnet_t *)control);
		in_purgeif((ifnet_t *)control);
		inpcb_purgeif(rawcbtable, (ifnet_t *)control);
		mutex_exit(softnet_lock);
		splx(s);
		return 0;
	}

	KASSERT(solocked(so));
	inp = sotoinpcb(so);

	KASSERT(!control || (req == PRU_SEND || req == PRU_SENDOOB));
	if (inp == NULL) {
		return EINVAL;
	}

	switch (req) {
	case PRU_BIND:
		error = rip_bind(inp, nam);
		break;

	case PRU_LISTEN:
		error = EOPNOTSUPP;
		break;

	case PRU_CONNECT:
		error = rip_connect(inp, nam);
		if (error)
			break;
		soisconnected(so);
		break;

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	case PRU_DISCONNECT:
		soisdisconnected(so);
		rip_disconnect(inp);
		break;

	/*
	 * Mark the connection as being incapable of further input.
	 */
	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	case PRU_RCVD:
		error = EOPNOTSUPP;
		break;

	/*
	 * Ship a packet out.  The appropriate raw output
	 * routine handles any massaging necessary.
	 */
	case PRU_SEND:
		if (control && control->m_len) {
			m_freem(control);
			m_freem(m);
			error = EINVAL;
			break;
		}
		if ((so->so_state & SS_ISCONNECTED) != 0) {
			error = nam ? EISCONN : ENOTCONN;
			m_freem(m);
			break;
		}
		if (nam && (error = rip_connect(inp, nam)) != 0) {
			m_freem(m);
			break;
		}
		error = rip_output(m, inp);
		if (nam) {
			rip_disconnect(inp);
		}
		break;

	case PRU_SENSE:
		/*
		 * Stat: do not bother with a blocksize.
		 */
		return 0;

	case PRU_RCVOOB:
		error = EOPNOTSUPP;
		break;

	case PRU_SENDOOB:
		m_freem(control);
		m_freem(m);
		error = EOPNOTSUPP;
		break;

	case PRU_SOCKADDR:
		inpcb_fetch_sockaddr(inp, nam);
		break;

	case PRU_PEERADDR:
		inpcb_fetch_peeraddr(inp, nam);
		break;

	default:
		KASSERT(false);
	}

	return error;
}

PR_WRAP_USRREQ(rip_usrreq)

#define	rip_usrreq	rip_usrreq_wrapper

const struct pr_usrreqs rip_usrreqs = {
	.pr_attach	= rip_attach,
	.pr_detach	= rip_detach,
	.pr_generic	= rip_usrreq,
};

static void
sysctl_net_inet_raw_setup(struct sysctllog **clog)
{
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "net", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "inet", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "raw",
		       SYSCTL_DESCR("Raw IPv4 settings"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, IPPROTO_RAW, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "pcblist",
		       SYSCTL_DESCR("Raw IPv4 control block list"),
		       sysctl_inpcblist, 0, rawcbtable, 0,
		       CTL_NET, PF_INET, IPPROTO_RAW,
		       CTL_CREATE, CTL_EOL);
}
