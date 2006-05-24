/*	$NetBSD: in_proto.c,v 1.71.12.1 2006/05/24 15:50:44 tron Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: in_proto.c,v 1.71.12.1 2006/05/24 15:50:44 tron Exp $");

#include "opt_mrouting.h"
#include "opt_eon.h"			/* ISO CLNL over IP */
#include "opt_iso.h"			/* ISO TP tunneled over IP */
#include "opt_ns.h"			/* NSIP: XNS tunneled over IP */
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

#ifdef NSIP
#include <netns/ns_var.h>
#include <netns/idp_var.h>
#endif /* NSIP */

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

#include "bridge.h"

DOMAIN_DEFINE(inetdomain);	/* forward declare and add to link set */

const struct protosw inetsw[] = {
{ 0,		&inetdomain,	0,		0,
  0,		ip_output,	0,		0,
  0,
  ip_init,	0,		ip_slowtimo,	ip_drain,	NULL
},
{ SOCK_DGRAM,	&inetdomain,	IPPROTO_UDP,	PR_ATOMIC|PR_ADDR|PR_PURGEIF,
  udp_input,	0,		udp_ctlinput,	udp_ctloutput,
  udp_usrreq,
  udp_init,	0,		0,		0,		NULL
},
{ SOCK_STREAM,	&inetdomain,	IPPROTO_TCP,	PR_CONNREQUIRED|PR_WANTRCVD|PR_LISTEN|PR_ABRTACPTDIS|PR_PURGEIF,
  tcp_input,	0,		tcp_ctlinput,	tcp_ctloutput,
  tcp_usrreq,
  tcp_init,	0,		tcp_slowtimo,	tcp_drain,	NULL
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_RAW,	PR_ATOMIC|PR_ADDR|PR_PURGEIF,
  rip_input,	rip_output,	rip_ctlinput,	rip_ctloutput,
  rip_usrreq,
  0,		0,		0,		0,
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_ICMP,	PR_ATOMIC|PR_ADDR|PR_LASTHDR,
  icmp_input,	rip_output,	rip_ctlinput,	rip_ctloutput,
  rip_usrreq,
  icmp_init,	0,		0,		0,		NULL
},
#ifdef IPSEC
{ SOCK_RAW,	&inetdomain,	IPPROTO_AH,	PR_ATOMIC|PR_ADDR,
  ah4_input,	0,	 	ah4_ctlinput,	0,
  0,
  0,		0,		0,		0,		NULL
},
#ifdef IPSEC_ESP
{ SOCK_RAW,	&inetdomain,	IPPROTO_ESP,	PR_ATOMIC|PR_ADDR,
  esp4_input,
	0,	 	esp4_ctlinput,	0,
  0,
  0,		0,		0,		0,		NULL
},
#endif
{ SOCK_RAW,	&inetdomain,	IPPROTO_IPCOMP,	PR_ATOMIC|PR_ADDR,
  ipcomp4_input,
 0,	 	0,		0,
  0,
  0,		0,		0,		0,		NULL
},
#endif /* IPSEC */
#ifdef FAST_IPSEC
{ SOCK_RAW,	&inetdomain,	IPPROTO_AH,	PR_ATOMIC|PR_ADDR,
  ipsec4_common_input,	0,	 	ah4_ctlinput,	0,
  0,
  0,		0,		0,		0,		NULL
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_ESP,	PR_ATOMIC|PR_ADDR,
  ipsec4_common_input,    0,	 	esp4_ctlinput,	0,
  0,
  0,		0,		0,		0,		NULL
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_IPCOMP,	PR_ATOMIC|PR_ADDR,
  ipsec4_common_input,    0,	 	0,		0,
  0,
  0,		0,		0,		0,		NULL
},
#endif /* FAST_IPSEC */
{ SOCK_RAW,	&inetdomain,	IPPROTO_IPV4,	PR_ATOMIC|PR_ADDR|PR_LASTHDR,
  encap4_input,	rip_output, 	rip_ctlinput,	rip_ctloutput,
  rip_usrreq,	/*XXX*/
  encap_init,	0,		0,		0,
},
#ifdef INET6
{ SOCK_RAW,	&inetdomain,	IPPROTO_IPV6,	PR_ATOMIC|PR_ADDR|PR_LASTHDR,
  encap4_input,	rip_output, 	rip_ctlinput,	rip_ctloutput,
  rip_usrreq,	/*XXX*/
  encap_init,	0,		0,		0,
},
#endif /* INET6 */
#if NBRIDGE > 0
{ SOCK_RAW,	&inetdomain,	IPPROTO_ETHERIP,	PR_ATOMIC|PR_ADDR|PR_LASTHDR,
  encap4_input,	rip_output,	rip_ctlinput,	rip_ctloutput,
  rip_usrreq,
  encap_init,		0,		0,		0,
},
#endif
#if NCARP > 0
{ SOCK_RAW,	&inetdomain,	IPPROTO_CARP,	PR_ATOMIC|PR_ADDR,
  carp_proto_input,	rip_output,	0,		rip_ctloutput,
  rip_usrreq,
  0,		0,		0,		0,		NULL,
},
#endif
#if NGRE > 0
{ SOCK_RAW,	&inetdomain,	IPPROTO_GRE,	PR_ATOMIC|PR_ADDR|PR_LASTHDR,
  gre_input,	rip_output,	rip_ctlinput,	rip_ctloutput,
  rip_usrreq,
  0,		0,		0,		0,
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_MOBILE,	PR_ATOMIC|PR_ADDR|PR_LASTHDR,
  gre_mobile_input,	rip_output,	rip_ctlinput,	rip_ctloutput,
  rip_usrreq,
  0,		0,		0,		0,
},
#endif /* NGRE > 0 */
{ SOCK_RAW,	&inetdomain,	IPPROTO_IGMP,	PR_ATOMIC|PR_ADDR|PR_LASTHDR,
  igmp_input,	rip_output,	rip_ctlinput,	rip_ctloutput,
  rip_usrreq,
  NULL,		igmp_fasttimo,	igmp_slowtimo,	0,
},
#ifdef PIM
{ SOCK_RAW,	&inetdomain,	IPPROTO_PIM,	PR_ATOMIC|PR_ADDR|PR_LASTHDR,
  pim_input,	rip_output,	rip_ctlinput,	rip_ctloutput,
  rip_usrreq,
  NULL,		0,		0,		0,
},
#endif /* PIM */
#ifdef TPIP
{ SOCK_SEQPACKET,&inetdomain,	IPPROTO_TP,	PR_CONNREQUIRED|PR_WANTRCVD|PR_LISTEN|PR_LASTHDR|PR_ABRTACPTDIS,
  tpip_input,	0,		tpip_ctlinput,	tp_ctloutput,
  tp_usrreq,
  tp_init,	0,		tp_slowtimo,	tp_drain,
},
#endif /* TPIP */
#ifdef ISO
/* EON (ISO CLNL over IP) */
#ifdef EON
{ SOCK_RAW,	&inetdomain,	IPPROTO_EON,	PR_LASTHDR,
  eoninput,	0,		eonctlinput,	0,
  0,
  eonprotoinit,	0,		0,		0,
},
#else
{ SOCK_RAW,	&inetdomain,	IPPROTO_EON,	PR_ATOMIC|PR_ADDR|PR_LASTHDR,
  encap4_input,	rip_output,	rip_ctlinput,	rip_ctloutput,
  rip_usrreq,	/*XXX*/
  encap_init,	0,		0,		0,
},
#endif /* EON */
#endif /* ISO */
#ifdef NSIP
{ SOCK_RAW,	&inetdomain,	IPPROTO_IDP,	PR_ATOMIC|PR_ADDR|PR_LASTHDR,
  idpip_input,	NULL,		nsip_ctlinput,	0,
  rip_usrreq,
  0,		0,		0,		0,
},
#endif /* NSIP */
/* raw wildcard */
{ SOCK_RAW,	&inetdomain,	0,		PR_ATOMIC|PR_ADDR|PR_LASTHDR,
  rip_input,	rip_output,	rip_ctlinput,	rip_ctloutput,
  rip_usrreq,
  rip_init,	0,		0,		0,
},
};

struct domain inetdomain =
    { PF_INET, "internet", 0, 0, 0,
      inetsw, &inetsw[sizeof(inetsw)/sizeof(inetsw[0])],
      rn_inithead, 32, sizeof(struct sockaddr_in) };

u_char	ip_protox[IPPROTO_MAX];

int icmperrppslim = 100;			/* 100pps */
