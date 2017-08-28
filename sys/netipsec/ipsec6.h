/*	$NetBSD: ipsec6.h,v 1.13.30.2 2017/08/28 17:53:13 skrll Exp $	*/
/*	$FreeBSD: src/sys/netipsec/ipsec6.h,v 1.1.4.1 2003/01/24 05:11:35 sam Exp $	*/
/*	$KAME: ipsec.h,v 1.44 2001/03/23 08:08:47 itojun Exp $	*/

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
 * IPsec controller part.
 */

#ifndef _NETIPSEC_IPSEC6_H_
#define _NETIPSEC_IPSEC6_H_

#include <net/pfkeyv2.h>
#include <netipsec/keydb.h>
#include <netinet6/in6_pcb.h>

#ifdef _KERNEL
extern int ip6_esp_trans_deflev;
extern int ip6_esp_net_deflev;
extern int ip6_ah_trans_deflev;
extern int ip6_ah_net_deflev;
extern int ip6_ipsec_ecn;
extern int ip6_esp_randpad;
extern struct secpolicy ip6_def_policy;

struct inpcb;
struct in6pcb;

/* KAME compatibility shims */
#define	ipsec6_getpolicybyaddr	ipsec_getpolicybyaddr
#define	ipsec6_getpolicybysock	ipsec_getpolicybysock

int ipsec6_delete_pcbpolicy (struct in6pcb *);
int ipsec6_set_policy (struct in6pcb *, int, const void *, size_t, kauth_cred_t);
int ipsec6_get_policy (struct in6pcb *, const void *, size_t, struct mbuf **);
struct secpolicy *ipsec6_checkpolicy (struct mbuf *, u_int, 
    u_int, int *, struct in6pcb *);
struct secpolicy * ipsec6_check_policy(struct mbuf *, 
				struct in6pcb *, int, int*,int*);
int ipsec6_in_reject (struct mbuf *, struct in6pcb *);

struct tcp6cb;

size_t ipsec6_hdrsiz (struct mbuf *, u_int, struct in6pcb *);
size_t ipsec6_hdrsiz_tcp (struct tcpcb*);

struct ip6_hdr;
const char *ipsec6_logpacketstr (struct ip6_hdr *, u_int32_t);

/* NetBSD protosw ctlin entrypoint */
void * esp6_ctlinput(int, const struct sockaddr *, void *);
void * ah6_ctlinput(int, const struct sockaddr *, void *);

struct m_tag;
int ipsec6_common_input(struct mbuf **, int *, int);
int ipsec6_common_input_cb(struct mbuf *, struct secasvar *, int, int);
int ipsec6_process_packet (struct mbuf*,struct ipsecrequest *);
#endif /*_KERNEL*/

#endif /* !_NETIPSEC_IPSEC6_H_ */
