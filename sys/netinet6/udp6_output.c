/*	$NetBSD: udp6_output.c,v 1.12 2003/08/15 03:42:06 jonathan Exp $	*/
/*	$KAME: udp6_output.c,v 1.43 2001/10/15 09:19:52 itojun Exp $	*/

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
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)udp_var.h	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: udp6_output.c,v 1.12 2003/08/15 03:42:06 jonathan Exp $");

#include "opt_ipsec.h"
#include "opt_inet.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/route.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_pcb.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#include <netinet6/udp6_var.h>
#include <netinet/icmp6.h>
#include <netinet6/ip6protosw.h>

#ifdef IPSEC
#include <netinet6/ipsec.h>
#endif /* IPSEC */

#include "faith.h"

#include <net/net_osdep.h>

/*
 * UDP protocol inplementation.
 * Per RFC 768, August, 1980.
 */

int
udp6_output(in6p, m, addr6, control, p)
	struct in6pcb *in6p;
	struct mbuf *m;
	struct mbuf *addr6, *control;
	struct proc *p;
{
	u_int32_t ulen = m->m_pkthdr.len;
	u_int32_t plen = sizeof(struct udphdr) + ulen;
	struct ip6_hdr *ip6;
	struct udphdr *udp6;
	struct in6_addr *laddr, *faddr;
	struct in6_addr laddr_mapped; /* XXX ugly */
	u_short fport;
	int error = 0;
	struct ip6_pktopts opt, *stickyopt = in6p->in6p_outputopts;
	int priv;
	int af = AF_INET6, hlen = sizeof(struct ip6_hdr);
#ifdef INET
	struct ip *ip;
	struct udpiphdr *ui;
#endif
	int flags = 0;
	struct sockaddr_in6 tmp;

	priv = 0;
	if (p && !suser(p->p_ucred, &p->p_acflag))
		priv = 1;
	if (control) {
		if ((error = ip6_setpktoptions(control, &opt, priv)) != 0)
			goto release;
		in6p->in6p_outputopts = &opt;
	}

	if (addr6) {
		/*
		 * IPv4 version of udp_output calls in_pcbconnect in this case,
		 * which needs splnet and affects performance.
		 * Since we saw no essential reason for calling in_pcbconnect,
		 * we get rid of such kind of logic, and call in6_selectsrc
		 * and in6_pcbsetport in order to fill in the local address
		 * and the local port.
		 */
		struct sockaddr_in6 *sin6 = mtod(addr6, struct sockaddr_in6 *);

		if (addr6->m_len != sizeof(*sin6)) {
			error = EINVAL;
			goto release;
		}
		if (sin6->sin6_family != AF_INET6) {
			error = EAFNOSUPPORT;
			goto release;
		}
		if (sin6->sin6_port == 0) {
			error = EADDRNOTAVAIL;
			goto release;
		}

		if (!IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_faddr)) {
			/* how about ::ffff:0.0.0.0 case? */
			error = EISCONN;
			goto release;
		}

		/* protect *sin6 from overwrites */
		tmp = *sin6;
		sin6 = &tmp;

		faddr = &sin6->sin6_addr;
		fport = sin6->sin6_port; /* allow 0 port */

		if (IN6_IS_ADDR_V4MAPPED(faddr)) {
			if ((in6p->in6p_flags & IN6P_IPV6_V6ONLY))
			{
				/*
				 * I believe we should explicitly discard the
				 * packet when mapped addresses are disabled,
				 * rather than send the packet as an IPv6 one.
				 * If we chose the latter approach, the packet
				 * might be sent out on the wire based on the
				 * default route, the situation which we'd
				 * probably want to avoid.
				 * (20010421 jinmei@kame.net)
				 */
				error = EINVAL;
				goto release;
			}
			if (!IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_laddr)
			    && !IN6_IS_ADDR_V4MAPPED(&in6p->in6p_laddr)) {
				/*
				 * when remote addr is an IPv4-mapped address,
				 * local addr should not be an IPv6 address,
				 * since you cannot determine how to map IPv6
				 * source address to IPv4.
				 */
				error = EINVAL;
				goto release;
			}

			af = AF_INET;
		}

		/* KAME hack: embed scopeid */
		if (in6_embedscope(&sin6->sin6_addr, sin6, in6p, NULL) != 0) {
			error = EINVAL;
			goto release;
		}

		if (!IN6_IS_ADDR_V4MAPPED(faddr)) {
			laddr = in6_selectsrc(sin6, in6p->in6p_outputopts,
					      in6p->in6p_moptions,
					      &in6p->in6p_route,
					      &in6p->in6p_laddr, &error);
		} else {
			/*
			 * XXX: freebsd[34] does not have in_selectsrc, but
			 * we can omit the whole part because freebsd4 calls
			 * udp_output() directly in this case, and thus we'll
			 * never see this path.
			 */
			if (IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_laddr)) {
				struct sockaddr_in *sinp, sin_dst;

				bzero(&sin_dst, sizeof(sin_dst));
				sin_dst.sin_family = AF_INET;
				sin_dst.sin_len = sizeof(sin_dst);
				bcopy(&faddr->s6_addr[12], &sin_dst.sin_addr,
				      sizeof(sin_dst.sin_addr));
				sinp = in_selectsrc(&sin_dst,
						    (struct route *)&in6p->in6p_route,
						    in6p->in6p_socket->so_options,
						    NULL, &error);
				if (sinp == NULL) {
					if (error == 0)
						error = EADDRNOTAVAIL;
					goto release;
				}
				bzero(&laddr_mapped, sizeof(laddr_mapped));
				laddr_mapped.s6_addr16[5] = 0xffff; /* ugly */
				bcopy(&sinp->sin_addr,
				      &laddr_mapped.s6_addr[12],
				      sizeof(sinp->sin_addr));
				laddr = &laddr_mapped;
			} else
			{
				laddr = &in6p->in6p_laddr;	/* XXX */
			}
		}
		if (laddr == NULL) {
			if (error == 0)
				error = EADDRNOTAVAIL;
			goto release;
		}
		if (in6p->in6p_lport == 0 &&
		    (error = in6_pcbsetport(laddr, in6p, p)) != 0)
			goto release;
	} else {
		if (IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_faddr)) {
			error = ENOTCONN;
			goto release;
		}
		if (IN6_IS_ADDR_V4MAPPED(&in6p->in6p_faddr)) {
			if ((in6p->in6p_flags & IN6P_IPV6_V6ONLY))
			{
				/*
				 * XXX: this case would happen when the
				 * application sets the V6ONLY flag after
				 * connecting the foreign address.
				 * Such applications should be fixed,
				 * so we bark here.
				 */
				log(LOG_INFO, "udp6_output: IPV6_V6ONLY "
				    "option was set for a connected socket\n");
				error = EINVAL;
				goto release;
			} else
				af = AF_INET;
		}
		laddr = &in6p->in6p_laddr;
		faddr = &in6p->in6p_faddr;
		fport = in6p->in6p_fport;
	}

	if (af == AF_INET)
		hlen = sizeof(struct ip);

	/*
	 * Calculate data length and get a mbuf
	 * for UDP and IP6 headers.
	 */
	M_PREPEND(m, hlen + sizeof(struct udphdr), M_DONTWAIT);
	if (m == 0) {
		error = ENOBUFS;
		goto release;
	}

	/*
	 * Stuff checksum and output datagram.
	 */
	udp6 = (struct udphdr *)(mtod(m, caddr_t) + hlen);
	udp6->uh_sport = in6p->in6p_lport; /* lport is always set in the PCB */
	udp6->uh_dport = fport;
	if (plen <= 0xffff)
		udp6->uh_ulen = htons((u_short)plen);
	else
		udp6->uh_ulen = 0;
	udp6->uh_sum = 0;

	switch (af) {
	case AF_INET6:
		ip6 = mtod(m, struct ip6_hdr *);
		ip6->ip6_flow	= in6p->in6p_flowinfo & IPV6_FLOWINFO_MASK;
		ip6->ip6_vfc 	&= ~IPV6_VERSION_MASK;
		ip6->ip6_vfc 	|= IPV6_VERSION;
#if 0				/* ip6_plen will be filled in ip6_output. */
		ip6->ip6_plen	= htons((u_short)plen);
#endif
		ip6->ip6_nxt	= IPPROTO_UDP;
		ip6->ip6_hlim	= in6_selecthlim(in6p,
						 in6p->in6p_route.ro_rt ?
						 in6p->in6p_route.ro_rt->rt_ifp : NULL);
		ip6->ip6_src	= *laddr;
		ip6->ip6_dst	= *faddr;

		if ((udp6->uh_sum = in6_cksum(m, IPPROTO_UDP,
				sizeof(struct ip6_hdr), plen)) == 0) {
			udp6->uh_sum = 0xffff;
		}

#ifdef IN6P_MINMTU
		if (in6p->in6p_flags & IN6P_MINMTU)
			flags |= IPV6_MINMTU;
#endif

		udp6stat.udp6s_opackets++;
#ifdef IPSEC
		if (ipsec_setsocket(m, in6p->in6p_socket) != 0) {
			error = ENOBUFS;
			goto release;
		}
#endif /* IPSEC */
		error = ip6_output(m, in6p->in6p_outputopts, &in6p->in6p_route,
		    flags, in6p->in6p_moptions, NULL);
		break;
	case AF_INET:
#ifdef INET
		/* can't transmit jumbogram over IPv4 */
		if (plen > 0xffff) {
			error = EMSGSIZE;
			goto release;
		}

		ip = mtod(m, struct ip *);
		ui = (struct udpiphdr *)ip;
		bzero(ui->ui_x1, sizeof ui->ui_x1);
		ui->ui_pr = IPPROTO_UDP;
		ui->ui_len = htons(plen);
		bcopy(&laddr->s6_addr[12], &ui->ui_src, sizeof(ui->ui_src));
		ui->ui_ulen = ui->ui_len;

		flags = (in6p->in6p_socket->so_options &
			 (SO_DONTROUTE | SO_BROADCAST));
		bcopy(&faddr->s6_addr[12], &ui->ui_dst, sizeof(ui->ui_dst));
		udp6->uh_sum = in_cksum(m, hlen + plen);
		if (udp6->uh_sum == 0)
			udp6->uh_sum = 0xffff;

		ip->ip_len = htons(hlen + plen);
		ip->ip_ttl = in6_selecthlim(in6p, NULL); /* XXX */
		ip->ip_tos = 0;	/* XXX */

		udpstat.udps_opackets++;
#ifdef IPSEC
		(void)ipsec_setsocket(m, NULL);	/* XXX */
#endif /* IPSEC */
		error = ip_output(m, NULL, &in6p->in6p_route, flags /* XXX */,
				  (struct ip_moptions *)0, (struct inpcb *)0);
		break;
#else
		error = EAFNOSUPPORT;
		goto release;
#endif
	}
	goto releaseopt;

release:
	m_freem(m);

releaseopt:
	if (control) {
		in6p->in6p_outputopts = stickyopt;
		m_freem(control);
	}
	return (error);
}
