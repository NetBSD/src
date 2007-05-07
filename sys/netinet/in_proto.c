/*	$NetBSD: in_proto.c,v 1.80.2.2 2007/05/07 10:55:58 yamt Exp $	*/

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
 * Copyright (c) 1982, 1986, 1993
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
 *	@(#)in_proto.c	8.2 (Berkeley) 2/9/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: in_proto.c,v 1.80.2.2 2007/05/07 10:55:58 yamt Exp $");

#include "opt_mrouting.h"
#include "opt_eon.h"			/* ISO CLNL over IP */
#include "opt_iso.h"			/* ISO TP tunneled over IP */
#include "opt_inet.h"
#include "opt_ipsec.h"
#include "opt_pim.h"

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <net/radix.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/in_ifattach.h>
#include <netinet/in_pcb.h>
#include <netinet/in_proto.h>

#ifdef INET6
#ifndef INET
#include <netinet/in.h>
#endif
#include <netinet/ip6.h>
#endif

#include <netinet/igmp_var.h>
#ifdef PIM
#include <netinet/pim_var.h>
#endif
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_debug.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/ip_encap.h>

/*
 * TCP/IP protocol family: IP, ICMP, UDP, TCP.
 */

#ifdef IPSEC
#include <netinet6/ipsec.h>
#include <netinet6/ah.h>
#ifdef IPSEC_ESP
#include <netinet6/esp.h>
#endif
#include <netinet6/ipcomp.h>
#endif /* IPSEC */

#ifdef FAST_IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/key.h>
#endif	/* FAST_IPSEC */

#ifdef TPIP
#include <netiso/tp_param.h>
#include <netiso/tp_var.h>
#endif /* TPIP */

#ifdef EON
#include <netiso/eonvar.h>
#endif /* EON */

#include "gre.h"
#if NGRE > 0
#include <netinet/ip_gre.h>
#endif

#include "carp.h"
#if NCARP > 0
#include <netinet/ip_carp.h>
#endif

#include "etherip.h"
#if NETHERIP > 0
#include <netinet/ip_etherip.h>
#endif

DOMAIN_DEFINE(inetdomain);	/* forward declare and add to link set */

const struct protosw inetsw[] = {
{	.pr_domain = &inetdomain,
	.pr_init = ip_init,
	.pr_output = ip_output,
	.pr_slowtimo = ip_slowtimo,
	.pr_drain = ip_drain,
},
{	.pr_type = SOCK_DGRAM,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_UDP,
	.pr_flags = PR_ATOMIC|PR_ADDR|PR_PURGEIF,
	.pr_input = udp_input,
	.pr_ctlinput = udp_ctlinput,
	.pr_ctloutput = udp_ctloutput,
	.pr_usrreq = udp_usrreq,
	.pr_init = udp_init,
},
{	.pr_type = SOCK_STREAM,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_TCP,
	.pr_flags = PR_CONNREQUIRED|PR_WANTRCVD|PR_LISTEN|PR_ABRTACPTDIS|PR_PURGEIF,
	.pr_input = tcp_input,
	.pr_ctlinput = tcp_ctlinput,
	.pr_ctloutput = tcp_ctloutput,
	.pr_usrreq = tcp_usrreq,
	.pr_init = tcp_init,
	.pr_slowtimo = tcp_slowtimo,
	.pr_drain = tcp_drain,
},
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_RAW,
	.pr_flags = PR_ATOMIC|PR_ADDR|PR_PURGEIF,
	.pr_input = rip_input,
	.pr_output = rip_output,
	.pr_ctlinput = rip_ctlinput,
	.pr_ctloutput = rip_ctloutput,
	.pr_usrreq = rip_usrreq,
},
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_ICMP,
	.pr_flags = PR_ATOMIC|PR_ADDR|PR_LASTHDR,
	.pr_input = icmp_input,
	.pr_output = rip_output,
	.pr_ctlinput = rip_ctlinput,
	.pr_ctloutput = rip_ctloutput,
	.pr_usrreq = rip_usrreq,
	.pr_init = icmp_init,
},
#ifdef GATEWAY
{	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_IP,
	.pr_slowtimo = ipflow_slowtimo,
},
#endif /* GATEWAY */
#ifdef IPSEC
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_AH,
	.pr_flags = PR_ATOMIC|PR_ADDR,
	.pr_input = ah4_input,
	.pr_ctlinput = ah4_ctlinput,
},
#ifdef IPSEC_ESP
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_ESP,
	.pr_flags = PR_ATOMIC|PR_ADDR,
	.pr_input = esp4_input,
	.pr_ctlinput = esp4_ctlinput,
},
#endif /* IPSEC_ESP */
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_IPCOMP,
	.pr_flags = PR_ATOMIC|PR_ADDR,
	.pr_input = ipcomp4_input,
},
#endif /* IPSEC */
#ifdef FAST_IPSEC
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_AH,
	.pr_flags = PR_ATOMIC|PR_ADDR,
	.pr_input = ipsec4_common_input,
	.pr_ctlinput = ah4_ctlinput,
},
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_ESP,
	.pr_flags = PR_ATOMIC|PR_ADDR,
	.pr_input = ipsec4_common_input,
	.pr_ctlinput = esp4_ctlinput,
},
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_IPCOMP,
	.pr_flags = PR_ATOMIC|PR_ADDR,
	.pr_input = ipsec4_common_input,
},
#endif /* FAST_IPSEC */
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_IPV4,
	.pr_flags = PR_ATOMIC|PR_ADDR|PR_LASTHDR,
	.pr_input = encap4_input,
	.pr_output = rip_output,
	.pr_ctlinput = rip_ctlinput,
	.pr_ctloutput = rip_ctloutput,
	.pr_usrreq = rip_usrreq,
	.pr_init = encap_init,
},
#ifdef INET6
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_IPV6,
	.pr_flags = PR_ATOMIC|PR_ADDR|PR_LASTHDR,
	.pr_input = encap4_input,
	.pr_output = rip_output,
	.pr_ctlinput = rip_ctlinput,
	.pr_ctloutput = rip_ctloutput,
	.pr_usrreq = rip_usrreq,
	.pr_init = encap_init,
},
#endif /* INET6 */
#if NETHERIP > 0
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_ETHERIP,
	.pr_flags = PR_ATOMIC|PR_ADDR|PR_LASTHDR,
	.pr_input = ip_etherip_input,
	.pr_output = rip_output,
	.pr_ctlinput = rip_ctlinput,
	.pr_ctloutput = rip_ctloutput,
	.pr_usrreq = rip_usrreq,
},
#endif /* NETHERIP > 0 */
#if NCARP > 0
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_CARP,
	.pr_flags = PR_ATOMIC|PR_ADDR,
	.pr_input = carp_proto_input,
	.pr_output = rip_output,
	.pr_ctloutput = rip_ctloutput,
	.pr_usrreq = rip_usrreq,
},
#endif /* NCARP > 0 */
#if NGRE > 0
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_GRE,
	.pr_flags = PR_ATOMIC|PR_ADDR|PR_LASTHDR,
	.pr_input = gre_input, 
	.pr_output = rip_output,
	.pr_ctloutput = rip_ctloutput,
	.pr_ctlinput = rip_ctlinput,
	.pr_usrreq = rip_usrreq,
},
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_MOBILE,
	.pr_flags = PR_ATOMIC|PR_ADDR|PR_LASTHDR,
	.pr_input = gre_mobile_input, 
	.pr_output = rip_output,
	.pr_ctloutput = rip_ctloutput,
	.pr_ctlinput = rip_ctlinput,
	.pr_usrreq = rip_usrreq,
},
#endif /* NGRE > 0 */
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_IGMP,
	.pr_flags = PR_ATOMIC|PR_ADDR|PR_LASTHDR,
	.pr_input = igmp_input, 
	.pr_output = rip_output,
	.pr_ctloutput = rip_ctloutput,
	.pr_ctlinput = rip_ctlinput,
	.pr_usrreq = rip_usrreq,
	.pr_fasttimo = igmp_fasttimo,
	.pr_slowtimo = igmp_slowtimo,
},
#ifdef PIM
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_PIM,
	.pr_flags = PR_ATOMIC|PR_ADDR|PR_LASTHDR,
	.pr_input = pim_input, 
	.pr_output = rip_output,
	.pr_ctloutput = rip_ctloutput,
	.pr_ctlinput = rip_ctlinput,
	.pr_usrreq = rip_usrreq,
},
#endif /* PIM */
#ifdef TPIP
{	.pr_type = SOCK_SEQPACKET,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_TP,
	.pr_flags = PR_CONNREQUIRED|PR_WANTRCVD|PR_LISTEN|PR_LASTHDR|PR_ABRTACPTDIS,
	.pr_input = tpip_input, 
	.pr_ctloutput = tp_ctloutput,
	.pr_ctlinput = tpip_ctlinput,
	.pr_usrreq = tp_usrreq,
	.pr_init = tp_init,
	.pr_slowtimo = tp_slowtimo,
	.pr_drain = tp_drain,
},
#endif /* TPIP */
#ifdef ISO
/* EON (ISO CLNL over IP) */
#ifdef EON
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_EON,
	.pr_flags = PR_LASTHDR,
	.pr_input = eoninput, 
	.pr_ctlinput = eonctlinput,
	.pr_init = eonprotoinit,
},
#else
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_protocol = IPPROTO_EON,
	.pr_flags = PR_ATOMIC|PR_ADDR|PR_LASTHDR,
	.pr_input = encap4_input, 
	.pr_output = rip_output,
	.pr_ctloutput = rip_ctloutput,
	.pr_ctlinput = rip_ctlinput,
	.pr_usrreq = rip_usrreq,
	.pr_init = encap_init,
},
#endif /* EON */
#endif /* ISO */
/* raw wildcard */
{	.pr_type = SOCK_RAW,
	.pr_domain = &inetdomain,
	.pr_flags = PR_ATOMIC|PR_ADDR|PR_LASTHDR,
	.pr_input = rip_input, 
	.pr_output = rip_output,
	.pr_ctloutput = rip_ctloutput,
	.pr_ctlinput = rip_ctlinput,
	.pr_usrreq = rip_usrreq,
	.pr_init = rip_init,
},
};

extern struct ifqueue ipintrq;

POOL_INIT(sockaddr_in_pool, sizeof(struct sockaddr_in), 0, 0, 0,
    "sockaddr_in_pool", NULL, IPL_NET);

struct domain inetdomain = {
	.dom_family = PF_INET, .dom_name = "internet", .dom_init = NULL,
	.dom_externalize = NULL, .dom_dispose = NULL,
	.dom_protosw = inetsw,
	.dom_protoswNPROTOSW = &inetsw[sizeof(inetsw)/sizeof(inetsw[0])],
	.dom_rtattach = rn_inithead,
	.dom_rtoffset = 32, .dom_maxrtkey = sizeof(struct sockaddr_in),
#ifdef IPSELSRC
	.dom_ifattach = in_domifattach,
	.dom_ifdetach = in_domifdetach,
#else
	.dom_ifattach = NULL,
	.dom_ifdetach = NULL,
#endif
	.dom_ifqueues = { &ipintrq, NULL },
	.dom_link = { NULL },
	.dom_mowner = MOWNER_INIT("",""),
	.dom_sa_pool = &sockaddr_in_pool,
	.dom_sa_len = sizeof(struct sockaddr_in),
	.dom_sa_cmpofs = offsetof(struct sockaddr_in, sin_addr),
	.dom_sa_cmplen = sizeof(struct in_addr),
	.dom_rtcache = LIST_HEAD_INITIALIZER(inetdomain.dom_rtcache)
};

u_char	ip_protox[IPPROTO_MAX];

int icmperrppslim = 100;			/* 100pps */

int
sockaddr_in_cmp(const struct sockaddr *sa1, const struct sockaddr *sa2)
{
	uint_fast8_t len;
	const uint_fast8_t addrofs = offsetof(struct sockaddr_in, sin_addr),
			   addrend = addrofs + sizeof(struct in_addr);
	int rc;
	const struct sockaddr_in *sin1, *sin2;

	sin1 = satocsin(sa1);
	sin2 = satocsin(sa2);

	len = MIN(addrend, MIN(sin1->sin_len, sin2->sin_len));

	if (len > addrofs &&
	     (rc = memcmp(&sin1->sin_addr, &sin2->sin_addr,
	                  len - addrofs)) != 0)
		return rc;

	return sin1->sin_len - sin2->sin_len;
}
