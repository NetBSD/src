/*	$NetBSD: ipsec.c,v 1.10.8.1 1999/12/27 18:36:26 wrstuden Exp $	*/

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

#if (defined(__FreeBSD__) && __FreeBSD__ >= 3) || defined(__NetBSD__)
#include "opt_inet.h"
#ifdef __NetBSD__	/*XXX*/
#include "opt_ipsec.h"
#endif
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#ifdef __NetBSD__
#include <sys/proc.h>
#include <vm/vm.h>
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <sys/sysctl.h>
#endif

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/in_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/ip_ecn.h>

#ifdef INET6
#include <netinet6/ip6.h>
#include <netinet6/in6_pcb.h>
#include <netinet6/ip6_var.h>
#include <netinet6/icmp6.h>
#endif /*INET6*/

#include <netinet6/ipsec.h>
#include <netinet6/ah.h>
#ifdef IPSEC_ESP
#include <netinet6/esp.h>
#endif
#include <netinet6/ipcomp.h>
#include <netkey/key.h>
#include <netkey/keydb.h>
#include <netkey/key_debug.h>

#ifdef __NetBSD__
#define ovbcopy	bcopy
#endif

struct ipsecstat ipsecstat;
int ip4_inbound_call_ike = 0;
int ip4_ah_cleartos = 1;
int ip4_ah_offsetmask = 0;	/* maybe IP_DF? */
int ip4_ipsec_dfbit = 0;	/* DF bit on encap. 0: clear 1: set 2: copy */
int ip4_esp_trans_deflev = IPSEC_LEVEL_USE;
int ip4_esp_net_deflev = IPSEC_LEVEL_USE;
int ip4_ah_trans_deflev = IPSEC_LEVEL_USE;
int ip4_ah_net_deflev = IPSEC_LEVEL_USE;
struct secpolicy ip4_def_policy;
int ip4_ipsec_ecn = 0;		/* ECN ignore(-1)/forbidden(0)/allowed(1) */

#ifdef __FreeBSD__
/* net.inet.ipsec */
SYSCTL_STRUCT(_net_inet_ipsec, IPSECCTL_STATS,
	stats, CTLFLAG_RD,	&ipsecstat,	ipsecstat, "");
SYSCTL_INT(_net_inet_ipsec, IPSECCTL_DEF_POLICY,
	def_policy, CTLFLAG_RW,	&ip4_def_policy.policy,	0, "");
SYSCTL_INT(_net_inet_ipsec, IPSECCTL_DEF_ESP_TRANSLEV, esp_trans_deflev,
	CTLFLAG_RW, &ip4_esp_trans_deflev,	0, "");
SYSCTL_INT(_net_inet_ipsec, IPSECCTL_DEF_ESP_NETLEV, esp_net_deflev,
	CTLFLAG_RW, &ip4_esp_net_deflev,	0, "");
SYSCTL_INT(_net_inet_ipsec, IPSECCTL_DEF_AH_TRANSLEV, ah_trans_deflev,
	CTLFLAG_RW, &ip4_ah_trans_deflev,	0, "");
SYSCTL_INT(_net_inet_ipsec, IPSECCTL_DEF_AH_NETLEV, ah_net_deflev,
	CTLFLAG_RW, &ip4_ah_net_deflev,	0, "");
SYSCTL_INT(_net_inet_ipsec, IPSECCTL_INBOUND_CALL_IKE,
	inbound_call_ike, CTLFLAG_RW,	&ip4_inbound_call_ike,	0, "");
SYSCTL_INT(_net_inet_ipsec, IPSECCTL_AH_CLEARTOS,
	ah_cleartos, CTLFLAG_RW,	&ip4_ah_cleartos,	0, "");
SYSCTL_INT(_net_inet_ipsec, IPSECCTL_AH_OFFSETMASK,
	ah_offsetmask, CTLFLAG_RW,	&ip4_ah_offsetmask,	0, "");
SYSCTL_INT(_net_inet_ipsec, IPSECCTL_DFBIT,
	dfbit, CTLFLAG_RW,	&ip4_ipsec_dfbit,	0, "");
SYSCTL_INT(_net_inet_ipsec, IPSECCTL_ECN,
	ecn, CTLFLAG_RW,	&ip4_ipsec_ecn,	0, "");
#endif /* __FreeBSD__ */

#ifdef INET6
struct ipsecstat ipsec6stat;
int ip6_inbound_call_ike = 0;
int ip6_esp_trans_deflev = IPSEC_LEVEL_USE;
int ip6_esp_net_deflev = IPSEC_LEVEL_USE;
int ip6_ah_trans_deflev = IPSEC_LEVEL_USE;
int ip6_ah_net_deflev = IPSEC_LEVEL_USE;
struct secpolicy ip6_def_policy;
int ip6_ipsec_ecn = 0;		/* ECN ignore(-1)/forbidden(0)/allowed(1) */

#ifdef __FreeBSD__
/* net.inet6.ipsec6 */
SYSCTL_STRUCT(_net_inet6_ipsec6, IPSECCTL_STATS,
	stats, CTLFLAG_RD, &ipsec6stat, ipsecstat, "");
SYSCTL_INT(_net_inet6_ipsec6, IPSECCTL_DEF_POLICY,
	def_policy, CTLFLAG_RW,	&ip6_def_policy.policy,	0, "");
SYSCTL_INT(_net_inet6_ipsec6, IPSECCTL_DEF_ESP_TRANSLEV, esp_trans_deflev,
	CTLFLAG_RW, &ip6_esp_trans_deflev,	0, "");
SYSCTL_INT(_net_inet6_ipsec6, IPSECCTL_DEF_ESP_NETLEV, esp_net_deflev,
	CTLFLAG_RW, &ip6_esp_net_deflev,	0, "");
SYSCTL_INT(_net_inet6_ipsec6, IPSECCTL_DEF_AH_TRANSLEV, ah_trans_deflev,
	CTLFLAG_RW, &ip6_ah_trans_deflev,	0, "");
SYSCTL_INT(_net_inet6_ipsec6, IPSECCTL_DEF_AH_NETLEV, ah_net_deflev,
	CTLFLAG_RW, &ip6_ah_net_deflev,	0, "");
SYSCTL_INT(_net_inet6_ipsec6, IPSECCTL_INBOUND_CALL_IKE,
	inbound_call_ike, CTLFLAG_RW,	&ip6_inbound_call_ike,	0, "");
SYSCTL_INT(_net_inet6_ipsec6, IPSECCTL_ECN,
	ecn, CTLFLAG_RW,	&ip6_ipsec_ecn,	0, "");
#endif /*__FreeBSD__*/
#endif /* INET6 */

static int ipsec_setsecidx __P((struct secindex *, u_int, struct mbuf *, int));
#if 0
static void ipsec_setsecidx_pcb __P((struct secindex *, u_int, u_int,
					void *, u_int, void *, u_int));
static int ipsec_delete_policy __P((struct secpolicy *));
#endif
static void vshiftl __P((unsigned char *, int, int));
static int ipsec_in_reject __P((struct secpolicy *, struct mbuf *));
static size_t ipsec_hdrsiz __P((struct secpolicy *));
static struct mbuf *ipsec4_splithdr __P((struct mbuf *));
#ifdef INET6
static struct mbuf *ipsec6_splithdr __P((struct mbuf *));
#endif
static int ipsec4_encapsulate __P((struct mbuf *, struct secas *));
#ifdef INET6
static int ipsec6_encapsulate __P((struct mbuf *, struct secas *));
#endif

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
 * For OUTBOUND packet having a socket. Searching SPD for packet,
 * and return a pointer to SP.
 * OUT:	NULL:	no apropreate SP found, the following value is set to error.
 *		0	: bypass
 *		EACCES	: discard packet.
 *		ENOENT	: ipsec_acquire() in progress, maybe.
 *		others	: error occured.
 *	others:	a pointer to SP
 *
 * NOTE: IPv6 mapped adddress concern is implemented here.
 */
struct secpolicy *
ipsec4_getpolicybysock(m, so, error)
	struct mbuf *m;
	struct socket *so;
	int *error;
{
	struct secpolicy *sp = NULL;		/* policy on socket */
	struct secpolicy *kernsp = NULL;	/* policy on kernel */
	int priv = 0;

	if (m == NULL || so == NULL || error == NULL)
		panic("ipsec4_getpolicybysock: NULL pointer was passed.\n");

	switch (so->so_proto->pr_domain->dom_family) {
	case AF_INET:
		sp = sotoinpcb(so)->inp_sp;
#if defined(__NetBSD__) || (defined(__FreeBSD__) && __FreeBSD__ >= 3)
		if (so->so_uid == 0)	/*XXX*/
			priv = 1;
		else
			priv = 0;
#else
		priv = sotoinpcb(so)->inp_socket->so_state & SS_PRIV;
#endif
		break;
#ifdef INET6
	case AF_INET6:
		sp = sotoin6pcb(so)->in6p_sp;
#if defined(__NetBSD__) || (defined(__FreeBSD__) && __FreeBSD__ >= 3)
		if (so->so_uid == 0)	/*XXX*/
			priv = 1;
		else
			priv = 0;
#else
		priv = sotoin6pcb(so)->in6p_socket->so_state & SS_PRIV;
#endif
		break;
#endif
	default:
		panic("ipsec4_getpolicybysock: unsupported address family\n");
	}

	/* sanity check */
	if (sp == NULL)
		panic("ipsec4_getpolicybysock: PCB's SP is NULL.\n");

	/* when privilieged socket */
	if (priv) {
		switch (sp->policy) {
		case IPSEC_POLICY_BYPASS:
			sp->refcnt++;
			*error = 0;
			return sp;

		case IPSEC_POLICY_ENTRUST:
		    {
			struct secindex idx;

			bzero(&idx, sizeof(idx));
			*error = ipsec_setsecidx(&idx, AF_INET, m, 1);
			if (*error != 0) {
				*error = ENOBUFS;
				return NULL;
			}

			kernsp = key_allocsp(&idx);
		    }

			/* SP found */
			if (kernsp != NULL) {
				KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
					printf("DP ipsec4_getpolicybysock called "
					       "to allocate SP:%p\n", kernsp));
				*error = 0;
				return kernsp;
			}

			/* no SP found */
			if (ip4_def_policy.policy != IPSEC_POLICY_DISCARD
			 && ip4_def_policy.policy != IPSEC_POLICY_NONE) {
				printf("fixed system default policy:%d->%d\n",
					ip4_def_policy.policy,
					IPSEC_POLICY_NONE);
				ip4_def_policy.policy = IPSEC_POLICY_NONE;
			}
			ip4_def_policy.refcnt++;
			*error = 0;
			return &ip4_def_policy;
			
		case IPSEC_POLICY_IPSEC:
			sp->refcnt++;
			ipsec_setsecidx(&sp->idx, AF_INET, m, 1);
			*error = 0;
			return sp;

		default:
			printf("ipsec4_getpolicybysock: "
			      "Invalid policy for PCB %d\n",
				sp->policy);
			*error = EINVAL;
			return NULL;
		}
		/* NOTREACHED */
	}

	/* when non-privilieged socket */
    {
	struct secindex idx;

	/* make a index to look for a policy */
	bzero(&idx, sizeof(idx));
	if ((*error = ipsec_setsecidx(&idx, AF_INET, m, 1)) != 0)
		return NULL;

	kernsp = key_allocsp(&idx);
    }

	/* SP found */
	if (kernsp != NULL) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP ipsec4_getpolicybysock called "
			       "to allocate SP:%p\n", kernsp));
		*error = 0;
		return kernsp;
	}

	/* no SP found */
	switch (sp->policy) {
	case IPSEC_POLICY_BYPASS:
		printf("ipsec4_getpolicybysock: "
		       "Illegal policy for non-priviliged defined %d\n",
			sp->policy);
		*error = EINVAL;
		return NULL;

	case IPSEC_POLICY_ENTRUST:
		if (ip4_def_policy.policy != IPSEC_POLICY_DISCARD
		 && ip4_def_policy.policy != IPSEC_POLICY_NONE) {
			printf("fixed system default policy:%d->%d\n",
				ip4_def_policy.policy,
				IPSEC_POLICY_NONE);
			ip4_def_policy.policy = IPSEC_POLICY_NONE;
		}
		ip4_def_policy.refcnt++;
		*error = 0;
		return &ip4_def_policy;

	case IPSEC_POLICY_IPSEC:
		sp->refcnt++;
		ipsec_setsecidx(&sp->idx, AF_INET, m, 1);
		*error = 0;
		return sp;

	default:
		printf("ipsec4_policybysock: "
		      "Invalid policy for PCB %d\n",
			sp->policy);
		*error = EINVAL;
		return NULL;
	}
	/* NOTREACHED */
}

/*
 * For FORWADING packet or OUTBOUND without a socket. Searching SPD for packet,
 * and return a pointer to SP.
 * OUT:	positive: a pointer to the entry for security policy leaf matched.
 *	NULL:	no apropreate SP found, the following value is set to error.
 *		0	: bypass
 *		EACCES	: discard packet.
 *		ENOENT	: ipsec_acquire() in progress, maybe.
 *		others	: error occured.
 */
struct secpolicy *
ipsec4_getpolicybyaddr(m, flag, error)
	struct mbuf *m;
	int flag;
	int *error;
{
	struct secpolicy *sp = NULL;

	/* sanity check */
	if (m == NULL || error == NULL)
		panic("ipsec4_getpolicybyaddr: NULL pointer was passed.\n");

    {
	struct secindex idx;

	bzero(&idx, sizeof(idx));

	/* make a index to look for a policy */
	if ((flag & IP_FORWARDING) == IP_FORWARDING)
		*error = ipsec_setsecidx(&idx, AF_INET, m, 2);
	else
		*error = ipsec_setsecidx(&idx, AF_INET, m, 1);

	if (*error != 0)
		return NULL;

	sp = key_allocsp(&idx);
    }

	/* SP found */
	if (sp != NULL) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP ipsec4_getpolicybyaddr called "
			       "to allocate SP:%p\n", sp));
		*error = 0;
		return sp;
	}

	/* no SP found */
	if (ip4_def_policy.policy != IPSEC_POLICY_DISCARD
	 && ip4_def_policy.policy != IPSEC_POLICY_NONE) {
		printf("fixed system default policy:%d->%d\n",
			ip4_def_policy.policy,
			IPSEC_POLICY_NONE);
		ip4_def_policy.policy = IPSEC_POLICY_NONE;
	}
	ip4_def_policy.refcnt++;
	*error = 0;
	return &ip4_def_policy;
}

#ifdef INET6
/*
 * For OUTBOUND packet having a socket. Searching SPD for packet,
 * and return a pointer to SP.
 * OUT:	NULL:	no apropreate SP found, the following value is set to error.
 *		0	: bypass
 *		EACCES	: discard packet.
 *		ENOENT	: ipsec_acquire() in progress, maybe.
 *		others	: error occured.
 *	others:	a pointer to SP
 */
struct secpolicy *
ipsec6_getpolicybysock(m, so, error)
	struct mbuf *m;
	struct socket *so;
	int *error;
{
	struct in6pcb *in6p = sotoin6pcb(so);
	struct secpolicy *sp = NULL;
	int priv = 0;

	/* sanity check */
	if (m == NULL || so == NULL || error == NULL)
		panic("ipsec6_getpolicybysock: NULL pointer was passed.\n");
	if (in6p == NULL)
		panic("ipsec6_getpolicybysock: no PCB found.\n");
	if (in6p->in6p_sp == NULL)
		panic("ipsec6_getpolicybysock: PCB's SP is NULL.\n");
	if (in6p->in6p_socket == NULL)
		panic("ipsec6_getpolicybysock: no socket found.\n");

	/* when privilieged socket */
#if defined(__NetBSD__) || (defined(__FreeBSD__) && __FreeBSD__ >= 3)
	if (so->so_uid == 0)	/*XXX*/
		priv = 1;
	else
		priv = 0;
#else
	priv = (in6p->in6p_socket->so_state & SS_PRIV);
#endif
	if (priv) {
		switch (in6p->in6p_sp->policy) {
		case IPSEC_POLICY_BYPASS:
			in6p->in6p_sp->refcnt++;
			*error = 0;
			return in6p->in6p_sp;

		case IPSEC_POLICY_ENTRUST:
		    {
			struct secindex idx;

			/* make a index to look for a policy */
			bzero(&idx, sizeof(idx));
			*error = ipsec_setsecidx(&idx, AF_INET6, m, 1);
			if (*error != 0)
				return NULL;

			sp = key_allocsp(&idx);
		    }

			/* SP found */
			if (sp != NULL) {
				KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
					printf("DP ipsec6_getpolicybysock called "
					       "to allocate SP:%p\n", sp));
				*error = 0;
				return sp;
			}

			/* no SP found */
			if (ip6_def_policy.policy != IPSEC_POLICY_DISCARD
			 && ip6_def_policy.policy != IPSEC_POLICY_NONE) {
				printf("fixed system default policy:%d->%d\n",
					ip6_def_policy.policy,
					IPSEC_POLICY_NONE);
				ip6_def_policy.policy = IPSEC_POLICY_NONE;
			}
			ip6_def_policy.refcnt++;
			*error = 0;
			return &ip6_def_policy;
			
		case IPSEC_POLICY_IPSEC:
			in6p->in6p_sp->refcnt++;
			ipsec_setsecidx(&in6p->in6p_sp->idx, AF_INET6, m, 1);
			*error = 0;
			return in6p->in6p_sp;

		default:
			printf("ipsec6_getpolicybysock: "
			      "Invalid policy for PCB %d\n",
				in6p->in6p_sp->policy);
			*error = EINVAL;
			return NULL;
		}
		/* NOTREACHED */
	}

	/* when non-privilieged socket */
    {
	struct secindex idx;

	bzero(&idx, sizeof(idx));
	/* make a index to look for a policy */
	if ((*error = ipsec_setsecidx(&idx, AF_INET6, m, 1)) != 0)
		return NULL;

	sp = key_allocsp(&idx);
    }

	/* SP found */
	if (sp != NULL) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP ipsec6_getpolicybysock called "
			       "to allocate SP:%p\n", sp));
		*error = 0;
		return sp;
	}

	/* no SP found */
	switch (in6p->in6p_sp->policy) {
	case IPSEC_POLICY_BYPASS:
		printf("ipsec6_getpolicybysock: "
		       "Illegal policy for non-priviliged defined %d\n",
			in6p->in6p_sp->policy);
		*error = EINVAL;
		return NULL;

	case IPSEC_POLICY_ENTRUST:
		if (ip6_def_policy.policy != IPSEC_POLICY_DISCARD
		 && ip6_def_policy.policy != IPSEC_POLICY_NONE) {
			printf("fixed system default policy:%d->%d\n",
				ip6_def_policy.policy,
				IPSEC_POLICY_NONE);
			ip6_def_policy.policy = IPSEC_POLICY_NONE;
		}
		ip6_def_policy.refcnt++;
		*error = 0;
		return &ip6_def_policy;

	case IPSEC_POLICY_IPSEC:
		in6p->in6p_sp->refcnt++;
		ipsec_setsecidx(&in6p->in6p_sp->idx, AF_INET6, m, 1);
		*error = 0;
		return in6p->in6p_sp;

	default:
		printf("ipsec6_policybysock: "
		      "Invalid policy for PCB %d\n",
			in6p->in6p_sp->policy);
		*error = EINVAL;
		return NULL;
	}
	/* NOTREACHED */
}

/*
 * For FORWADING packet or OUTBOUND without a socket. Searching SPD for packet,
 * and return a pointer to SP.
 * `flag' means that packet is to be forwarded whether or not.
 *	flag = 1: forwad
 * OUT:	positive: a pointer to the entry for security policy leaf matched.
 *	NULL:	no apropreate SP found, the following value is set to error.
 *		0	: bypass
 *		EACCES	: discard packet.
 *		ENOENT	: ipsec_acquire() in progress, maybe.
 *		others	: error occured.
 */
#ifndef IP_FORWARDING
#define IP_FORWARDING 1
#endif

struct secpolicy *
ipsec6_getpolicybyaddr(m, flag, error)
	struct mbuf *m;
	int flag;
	int *error;
{
	struct secpolicy *sp = NULL;

	/* sanity check */
	if (m == NULL || error == NULL)
		panic("ipsec6_getpolicybyaddr: NULL pointer was passed.\n");

    {
	struct secindex idx;

	bzero(&idx, sizeof(idx));

	/* make a index to look for a policy */
	if ((flag & IP_FORWARDING) == IP_FORWARDING)
		*error = ipsec_setsecidx(&idx, AF_INET6, m, 2);
	else
		*error = ipsec_setsecidx(&idx, AF_INET6, m, 1);

	if (*error != 0)
		return NULL;

	sp = key_allocsp(&idx);
    }

	/* SP found */
	if (sp != NULL) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP ipsec6_getpolicybyaddr called "
			       "to allocate SP:%p\n", sp));
		*error = 0;
		return sp;
	}

	/* no SP found */
	if (ip6_def_policy.policy != IPSEC_POLICY_DISCARD
	 && ip6_def_policy.policy != IPSEC_POLICY_NONE) {
		printf("fixed system default policy:%d->%d\n",
			ip6_def_policy.policy,
			IPSEC_POLICY_NONE);
		ip6_def_policy.policy = IPSEC_POLICY_NONE;
	}
	ip6_def_policy.refcnt++;
	*error = 0;
	return &ip6_def_policy;
}
#endif /* INET6 */

/*
 * seting the values for address from mbuf to secindex structure allocated.
 * IN:	according to flag, taking the followings from mbuf.
 *	== 1:	protocol family, src, dst, next protocol, src port, dst port.
 *		This function is called before fragment and after reassemble.
 *	== 2:	protocol family, src, dst, next protocol.
 *		For the forwarding packet.
 *	== 3:	protocol family, src, dst, security protocol, spi.
 *		XXX reserved for inbound processing. ??
 * OUT:
 *	0: success.
 *	1: failure.
 */
int
ipsec_setsecidx(idx, family, m, flag)
	struct secindex *idx;
	u_int family;
	struct mbuf *m;
	int flag;
{
	/* sanity check */
	if (idx == NULL || m == NULL)
		panic("ipsec_setsecidx: NULL pointer was passed.\n");

	KEYDEBUG(KEYDEBUG_IPSEC_DUMP,
		printf("ipsec_setsecidx: begin\n"); kdebug_mbuf(m));

	idx->family = family;
	idx->prefs = idx->prefd = _INALENBYAF(family) << 3;

    {
	/* sanity check for packet length. */
	struct mbuf *n;
	int tlen;

	tlen = 0;
	for (n = m; n; n = n->m_next)
		tlen += n->m_len;
	if (m->m_pkthdr.len != tlen) {
		KEYDEBUG(KEYDEBUG_IPSEC_DUMP,
			printf("ipsec_setsecidx: "
			       "total of m_len(%d) != pkthdr.len(%d), "
			       "ignored.\n",
				tlen, m->m_pkthdr.len));
		goto bad;
	}
    }

	switch (family) {
	case AF_INET:
	    {
		struct ip *ip;
		struct ip ipbuf;

		/* sanity check 1 for minimum ip header length */
		if (m->m_pkthdr.len < sizeof(struct ip)) {
			KEYDEBUG(KEYDEBUG_IPSEC_DUMP,
				printf("ipsec_setsecidx: "
				       "pkthdr.len(%d) < sizeof(struct ip), "
				       "ignored.\n",
					m->m_pkthdr.len));
			goto bad;
		}

		/*
		 * get IPv4 header packet.  usually the mbuf is contiguous
		 * and we need no copies.
		 */
		if (m->m_len >= sizeof(*ip))
			ip = mtod(m, struct ip *);
		else {
			m_copydata(m, 0, sizeof(ipbuf), (caddr_t)&ipbuf);
			ip = &ipbuf;
		}

		/* some more checks on IPv4 header. */
#if 0
		/*
		 * Since {udp,tcp}_input overwrites ip_v field by struct ipovly,
		 * this check is useless.
		 */
#ifdef _IP_VHL
		if (IP_VHL_V(ip->ip_vhl) != IPVERSION)
#else
		if (ip->ip_v != IPVERSION)
#endif
		{
			KEYDEBUG(KEYDEBUG_IPSEC_DUMP,
				printf("ipsec_setsecidx: "
					"wrong ip version on packet "
					"(expected IPv4), ignored.\n"));
			goto bad;
		}
#endif

		idx->proto = ip->ip_p;
		bcopy(&ip->ip_src, &idx->src, sizeof(ip->ip_src));
		bcopy(&ip->ip_dst, &idx->dst, sizeof(ip->ip_dst));

		switch (flag) {
		case 1:
			/* get port numbers from next protocol header */
			switch (ip->ip_p) {
			case IPPROTO_TCP:
			case IPPROTO_UDP:
			    {
				int len;
				u_short ports, portd;

				/* sanity check */
				/* set ip header length */
				if (ip->ip_hl == 0) {
					/*
					 * In upper layer stack,
					 * it is set zero to
					 * some values in ipovly.
					 */
					len = sizeof(struct ip);
				} else {
#ifdef _IP_VHL
					len = _IP_VHL_HL(ip->ip_vhl) << 2;
#else
					len = ip->ip_hl << 2;
#endif
				}

				if (m->m_pkthdr.len < len + 4) {
					KEYDEBUG(KEYDEBUG_IPSEC_DUMP,
					    printf("ipsec_setsecidx: "
					       "pkthdr.len(%d) too short to "
					       "get port number information, "
					       "ignored.\n",
						m->m_pkthdr.len));
					goto bad;
				}

				m_copydata(m, len, 2, (caddr_t)&ports);
				m_copydata(m, len + 2, 2, (caddr_t)&portd);
#if 0
				idx->ports = ntohs(ports);
				idx->portd = ntohs(portd);
#else
				idx->ports = ports;
				idx->portd = portd;
#endif
			    }
				break;

			case IPPROTO_ICMP:
			default:
				idx->ports = 0;
				idx->portd = 0;
			}
			break;
		case 2:
			break;
		case 3:
			break;
		}
	    }
		break;
#ifdef INET6
	case AF_INET6:
	    {
		struct ip6_hdr *ip6_hdr;
		struct ip6_hdr ip6buf;

		/* sanity check 1 for minimum ip header length */
		if (m->m_pkthdr.len < sizeof(struct ip6_hdr)) {
			KEYDEBUG(KEYDEBUG_IPSEC_DUMP,
				printf("ipsec_setsecidx: "
				       "pkthdr.len(%d) < sizeof(struct ip6_hdr), "
				       "ignored.\n",
					m->m_pkthdr.len));
			goto bad;
		}

		/*
		 * get IPv6 header packet.  usually the mbuf is contiguous
		 * and we need no copies.
		 */
		if (m->m_len >= sizeof(*ip6_hdr))
			ip6_hdr = mtod(m, struct ip6_hdr *);
		else {
			m_copydata(m, 0, sizeof(ip6buf), (caddr_t)&ip6buf);
			ip6_hdr = &ip6buf;
		}

		/* some more checks on IPv4 header. */
		if ((ip6_hdr->ip6_vfc & IPV6_VERSION_MASK) != IPV6_VERSION) {
			KEYDEBUG(KEYDEBUG_IPSEC_DUMP,
				printf("ipsec_setsecidx: "
					"wrong ip version on packet "
					"(expected IPv6), ignored.\n"));
			goto bad;
		}

		idx->proto = 0; /* XXX */
		bcopy(&ip6_hdr->ip6_src, &idx->src, sizeof(ip6_hdr->ip6_src));
		bcopy(&ip6_hdr->ip6_dst, &idx->dst, sizeof(ip6_hdr->ip6_dst));

#if 0 /* XXX Do it ! */
		switch (flag) {
		case 1:
			/* get port numbers from next protocol header */
			switch (ip6->ip6_nxt) {
			/* fragmented case ... */
			case IPPROTO_TCP:
			case IPPROTO_UDP:
				break;
			case IPPROTO_ICMPV6:
			default:
				idx->proto = 0; /* i.e. ignore */
				idx->ports = 0;
				idx->portd = 0;
			}
			break;
		case 2:
		case 3:
		}
#else
		idx->proto = 0; /* i.e. ignore */
		idx->ports = 0;
		idx->portd = 0;
#endif
	    }
		break;
#endif /* INET6 */
	default:
		panic("ipsec_secsecidx: no supported family passed.\n");
	}

	KEYDEBUG(KEYDEBUG_IPSEC_DUMP,
		printf("ipsec_setsecidx: end\n"); kdebug_secindex(idx));

	return 0;

    bad:
	/* XXX initialize */
	bzero(idx, sizeof(*idx));
	return -1;
}

#if 0
static void
ipsec_setsecidx_pcb(idx, family, ip_p, laddr, lport, faddr, fport)
	struct secindex *idx;
	u_int family, ip_p, lport, fport;
	void *laddr, *faddr;
{
	printf("family:%u ip_p:%u pref:%u\n",
		family, ip_p, _INALENBYAF(family));
	ipsec_hexdump(laddr, _INALENBYAF(family)),
	printf(" lport:%u\n", ntohs(lport));
	ipsec_hexdump(faddr, _INALENBYAF(family)),
	printf(" fport:%u\n", ntohs(fport));

	idx->family = family;
	idx->prefs = _INALENBYAF(family) << 8;
	idx->prefd = _INALENBYAF(family) << 8;
	idx->proto = ip_p;
	idx->ports = lport;
	idx->portd = fport;
	idx->src = (caddr_t)laddr;
	idx->dst = (caddr_t)faddr;

	return;
}
#endif

/* initialize policy in PCB */
int
ipsec_init_policy(pcb_sp)
	struct secpolicy **pcb_sp;
{
	struct secpolicy *newsp;

	/* sanity check. */
	if (pcb_sp == NULL)
		panic("ipsec_init_policy: NULL pointer was passed.\n");

	if ((newsp = key_newsp()) == NULL)
		return ENOBUFS;

	newsp->state = IPSEC_SPSTATE_ALIVE;
	newsp->policy = IPSEC_POLICY_ENTRUST;

	*pcb_sp = newsp;	

	return 0;
}

/* deep-copy a policy in PCB */
struct secpolicy *
ipsec_copy_policy(src)
	struct secpolicy *src;
{
	struct ipsecrequest *newchain = NULL;
	struct ipsecrequest *p;
	struct ipsecrequest **q;
	struct ipsecrequest *r;
	struct secpolicy *dst;

	dst = key_newsp();
	if (src == NULL || dst == NULL)
		return NULL;

	/*
	 * deep-copy IPsec request chain.  This is required since struct
	 * ipsecrequest is not reference counted.
	 */
	q = &newchain;
	for (p = src->req; p; p = p->next) {
		KMALLOC(*q, struct ipsecrequest *, sizeof(struct ipsecrequest));
		if (*q == NULL)
			goto fail;
		bzero(*q, sizeof(**q));
		(*q)->next = NULL;
		(*q)->proto = p->proto;
		(*q)->mode = p->mode;
		(*q)->level = p->level;
		(*q)->proxy = NULL;
		if (p->proxy) {
			KMALLOC((*q)->proxy, struct sockaddr *,
				p->proxy->sa_len);
			if ((*q)->proxy == NULL)
				goto fail;
			bcopy(p->proxy, (*q)->proxy, p->proxy->sa_len);
		}
		(*q)->sa = NULL;
		(*q)->sp = dst;

		q = &((*q)->next);
	}

	dst->req = newchain;
	dst->state = src->state;
	dst->policy = src->policy;
	dst->state = src->state;
	/* do not touch the refcnt fields */

	return dst;

fail:
	for (p = newchain; p; p = r) {
		r = p->next;
		if (p->proxy) {
			KFREE(p->proxy);
			p->proxy = NULL;
		}
		KFREE(p);
		p = NULL;
	}
	return NULL;
}

/* set policy and ipsec request if present. */
int
ipsec_set_policy(pcb_sp, optname, request, reqlen, priv)
	struct secpolicy **pcb_sp;
	int optname, reqlen;
	caddr_t request;
	int priv;
{
	struct sadb_x_policy *xpl = (struct sadb_x_policy *)request;
	struct secpolicy *newsp = NULL;

	/* sanity check. */
	if (pcb_sp == NULL || *pcb_sp == NULL || xpl == NULL)
		return EINVAL;

	KEYDEBUG(KEYDEBUG_IPSEC_DUMP,
		printf("ipsec_set_policy:\n");
		kdebug_sadb_x_policy((struct sadb_ext *)xpl));

	/* check policy type */
	/* ipsec_set_policy() accepts IPSEC, ENTRUST and BYPASS. */
	if (xpl->sadb_x_policy_type == IPSEC_POLICY_DISCARD
	 || xpl->sadb_x_policy_type == IPSEC_POLICY_NONE)
		return EINVAL;

	/* check privileged socket */
	if (priv == 0 && xpl->sadb_x_policy_type == IPSEC_POLICY_BYPASS)
		return EACCES;

	/* allocation new SP entry */
	if ((newsp = key_msg2sp(xpl)) == NULL)
		return EINVAL;	/* maybe ENOBUFS */

	newsp->state = IPSEC_SPSTATE_ALIVE;

	/* clear old SP and set new SP */
	key_freesp(*pcb_sp);
	*pcb_sp = newsp;
	KEYDEBUG(KEYDEBUG_IPSEC_DUMP,
		printf("ipsec_set_policy:\n");
		kdebug_secpolicy(newsp));

	return 0;
}

int
ipsec_get_policy(pcb_sp, mp)
	struct secpolicy *pcb_sp;
	struct mbuf **mp;
{
	struct sadb_x_policy *xpl;

	/* sanity check. */
	if (pcb_sp == NULL || mp == NULL)
		return EINVAL;

	if ((xpl = key_sp2msg(pcb_sp)) == NULL) {
		printf("ipsec_get_policy: No more memory.\n");
		return ENOBUFS;
	}

#if defined(__FreeBSD__) && __FreeBSD__ >= 3
	*mp = m_get(M_WAIT, MT_DATA);
#else
	*mp = m_get(M_WAIT, MT_SOOPTS);
#endif
	(*mp)->m_len = PFKEY_EXTLEN(xpl);
	m_copyback((*mp), 0, PFKEY_EXTLEN(xpl), (caddr_t)xpl);
	KEYDEBUG(KEYDEBUG_IPSEC_DUMP,
		printf("ipsec_get_policy:\n");
		kdebug_mbuf(*mp));

	KFREE(xpl);

	return 0;
}

#if 0
/* delete policy */
static int
ipsec_delete_policy(sp)
	struct secpolicy *sp;
{
	/* sanity check. */
	if (sp == NULL)
		panic("ipsec_delete_policy: NULL pointer was passed.\n");

	key_freesp(sp);

	return 0;
}
#endif

/* delete policy in PCB */
int
ipsec4_delete_pcbpolicy(inp)
	struct inpcb *inp;
{
	/* sanity check. */
	if (inp == NULL)
		panic("ipsec6_delete_pcbpolicy: NULL pointer was passed.\n");

	if (inp->inp_sp == NULL)
		return 0;

	key_freesp(inp->inp_sp);
	inp->inp_sp = NULL;

	return 0;
}

#ifdef INET6
int
ipsec6_delete_pcbpolicy(in6p)
	struct in6pcb *in6p;
{
	/* sanity check. */
	if (in6p == NULL)
		panic("ipsec6_delete_pcbpolicy: NULL pointer was passed.\n");

	if (in6p->in6p_sp == NULL)
		return 0;

	key_freesp(in6p->in6p_sp);
	in6p->in6p_sp = NULL;

	return 0;
}
#endif

/*
 * return current level.
 * IPSEC_LEVEL_USE or IPSEC_LEVEL_REQUIRE are always returned.
 */
u_int
ipsec_get_reqlevel(isr)
	struct ipsecrequest *isr;
{
	u_int level = 0;
	u_int esp_trans_deflev, esp_net_deflev, ah_trans_deflev, ah_net_deflev;

	/* sanity check */
	if (isr == NULL || isr->sp == NULL)
		panic("ipsec_get_reqlevel: NULL pointer is passed.\n");

#define IPSEC_CHECK_DEFAULT(lev) \
        (((lev) != IPSEC_LEVEL_USE && (lev) != IPSEC_LEVEL_REQUIRE) \
                ? (printf("fixed system default level " #lev ":%d->%d\n", \
			(lev), IPSEC_LEVEL_USE), \
			(lev) = IPSEC_LEVEL_USE) : (lev))

	/* set default level */
	switch (isr->sp->idx.family) {
#ifdef INET
	case AF_INET:
		esp_trans_deflev = IPSEC_CHECK_DEFAULT(ip4_esp_trans_deflev);
		esp_net_deflev = IPSEC_CHECK_DEFAULT(ip4_esp_net_deflev);
		ah_trans_deflev = IPSEC_CHECK_DEFAULT(ip4_ah_trans_deflev);
		ah_net_deflev = IPSEC_CHECK_DEFAULT(ip4_ah_net_deflev);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		esp_trans_deflev = IPSEC_CHECK_DEFAULT(ip6_esp_trans_deflev);
		esp_net_deflev = IPSEC_CHECK_DEFAULT(ip6_esp_net_deflev);
		ah_trans_deflev = IPSEC_CHECK_DEFAULT(ip6_ah_trans_deflev);
		ah_net_deflev = IPSEC_CHECK_DEFAULT(ip6_ah_net_deflev);
		break;
#endif /* INET6 */
	default:
		panic("key_get_reqlevel: Unknown family. %d\n",
			isr->sp->idx.family);
	}

#undef IPSEC_CHECK_DEFAULT(lev)

	/* set level */
	switch (isr->level) {
	case IPSEC_LEVEL_DEFAULT:
		switch (isr->proto) {
		case IPPROTO_ESP:
			if (isr->mode == IPSEC_MODE_TUNNEL)
				level = esp_net_deflev;
			else
				level = esp_trans_deflev;
			break;
		case IPPROTO_AH:
			if (isr->mode == IPSEC_MODE_TUNNEL)
				level = ah_net_deflev;
			else
				level = ah_trans_deflev;
		default:
			panic("ipsec_get_reqlevel: "
				"Illegal protocol defined %u\n",
				isr->proto);
		}
		break;

	case IPSEC_LEVEL_USE:
	case IPSEC_LEVEL_REQUIRE:
		level = isr->level;
		break;

	default:
		panic("ipsec_get_reqlevel: Illegal IPsec level %u\n",
			isr->level);
	}

	return level;
}

/*
 * Check AH/ESP integrity.
 * OUT:
 *	0: valid
 *	1: invalid
 */
static int
ipsec_in_reject(sp, m)
	struct secpolicy *sp;
	struct mbuf *m;
{
	struct ipsecrequest *isr;
	u_int level;
	int need_auth, need_conf, need_icv;

	KEYDEBUG(KEYDEBUG_IPSEC_DATA,
		printf("ipsec_in_reject: using SP\n");
		kdebug_secpolicy(sp));

	/* check policy */
	switch (sp->policy) {
	case IPSEC_POLICY_DISCARD:
		return 1;
	case IPSEC_POLICY_BYPASS:
	case IPSEC_POLICY_NONE:
		return 0;
	
	case IPSEC_POLICY_IPSEC:
		break;

	case IPSEC_POLICY_ENTRUST:
	default:
		panic("ipsec_hdrsiz: Invalid policy found. %d\n", sp->policy);
	}

	need_auth = 0;
	need_conf = 0;
	need_icv = 0;

	for (isr = sp->req; isr != NULL; isr = isr->next) {

		/* get current level */
		level = ipsec_get_reqlevel(isr);

		switch (isr->proto) {
		case IPPROTO_ESP:
			if (level == IPSEC_LEVEL_REQUIRE) {
				need_conf++;

				if (isr->sa != NULL
				 && isr->sa->flags == SADB_X_EXT_NONE
				 && isr->sa->alg_auth != SADB_AALG_NONE)
					need_icv++;
			}
			break;
		case IPPROTO_AH:
			if (level == IPSEC_LEVEL_REQUIRE) {
				need_auth++;
				need_icv++;
			}
			break;
		}
	}

	KEYDEBUG(KEYDEBUG_IPSEC_DUMP,
		printf("ipsec_in_reject: auth:%d conf:%d icv:%d m_flags:%x\n",
			need_auth, need_conf, need_icv, m->m_flags));

	if ((need_conf && !(m->m_flags & M_DECRYPTED))
	 || (!need_auth && need_icv && !(m->m_flags & M_AUTHIPDGM))
	 || (need_auth && !(m->m_flags & M_AUTHIPHDR)))
		return 1;

	return 0;
}

/*
 * Check AH/ESP integrity.
 * This function is called from tcp_input(), udp_input(),
 * and {ah,esp}4_input for tunnel mode
 */
int
ipsec4_in_reject_so(m, so)
	struct mbuf *m;
	struct socket *so;
{
	struct secpolicy *sp = NULL;
	int error;
	int result;

	/* sanity check */
	if (m == NULL)
		return 0;	/* XXX should be panic ? */

	/* get SP for this packet.
	 * When we are called from ip_forward(), we call
	 * ipsec4_getpolicybyaddr() with IP_FORWARDING flag.
	 */
	if (so == NULL)
		sp = ipsec4_getpolicybyaddr(m, IP_FORWARDING, &error);
	else
		sp = ipsec4_getpolicybysock(m, so, &error);

	if (sp == NULL)
		return 0;	/* XXX should be panic ?
				 * -> No, there may be error. */

	result = ipsec_in_reject(sp, m);
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP ipsec4_in_reject_so call free SP:%p\n", sp));
	key_freesp(sp);

	return result;
}

int
ipsec4_in_reject(m, inp)
	struct mbuf *m;
	struct inpcb *inp;
{
	if (inp == NULL)
		return ipsec4_in_reject_so(m, NULL);
	else {
		if (inp->inp_socket)
			return ipsec4_in_reject_so(m, inp->inp_socket);
		else
			panic("ipsec4_in_reject: invalid inpcb/socket");
	}
}

#ifdef INET6
/*
 * Check AH/ESP integrity.
 * This function is called from tcp6_input(), udp6_input(),
 * and {ah,esp}6_input for tunnel mode
 */
int
ipsec6_in_reject_so(m, so)
	struct mbuf *m;
	struct socket *so;
{
	struct secpolicy *sp = NULL;
	int error;
	int result;

	/* sanity check */
	if (m == NULL)
		return 0;	/* XXX should be panic ? */

	/* get SP for this packet.
	 * When we are called from ip_forward(), we call
	 * ipsec6_getpolicybyaddr() with IP_FORWARDING flag.
	 */
	if (so == NULL)
		sp = ipsec6_getpolicybyaddr(m, IP_FORWARDING, &error);
	else
		sp = ipsec6_getpolicybysock(m, so, &error);

	if (sp == NULL)
		return 0;	/* XXX should be panic ? */

	result = ipsec_in_reject(sp, m);
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP ipsec6_in_reject_so call free SP:%p\n", sp));
	key_freesp(sp);

	return result;
}

int
ipsec6_in_reject(m, in6p)
	struct mbuf *m;
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
	struct inpcb *in6p;
#else
	struct in6pcb *in6p;
#endif
{
	if (in6p == NULL)
		return ipsec6_in_reject_so(m, NULL);
	else {
		if (in6p->in6p_socket)
			return ipsec6_in_reject_so(m, in6p->in6p_socket);
		else
			panic("ipsec6_in_reject: invalid in6p/socket");
	}
}
#endif

/*
 * compute the byte size to be occupied by IPsec header.
 * in case it is tunneled, it includes the size of outer IP header.
 * NOTE: SP passed is free in this function.
 */
static size_t
ipsec_hdrsiz(sp)
	struct secpolicy *sp;
{
	struct ipsecrequest *isr;
	size_t siz, clen;

	KEYDEBUG(KEYDEBUG_IPSEC_DATA,
		printf("ipsec_in_reject: using SP\n");
		kdebug_secpolicy(sp));

	/* check policy */
	switch (sp->policy) {
	case IPSEC_POLICY_DISCARD:
	case IPSEC_POLICY_BYPASS:
	case IPSEC_POLICY_NONE:
		return 0;
	
	case IPSEC_POLICY_IPSEC:
		break;

	case IPSEC_POLICY_ENTRUST:
	default:
		panic("ipsec_hdrsiz: Invalid policy found. %d\n", sp->policy);
	}

	siz = 0;

	for (isr = sp->req; isr != NULL; isr = isr->next) {

		clen = 0;

		switch (isr->proto) {
		case IPPROTO_ESP:
#ifdef IPSEC_ESP
			clen = esp_hdrsiz(isr);
#else
			clen = 0;	/*XXX*/
#endif
			break;
		case IPPROTO_AH:
			clen = ah_hdrsiz(isr);
			break;
		case IPPROTO_IPCOMP:
			clen = sizeof(struct ipcomp);
			break;
		}

		if (isr->mode == IPSEC_MODE_TUNNEL
#if 0 /* XXX why ? There may be tunnel mode without IPsec. */
		 && clen != 0
#endif
		) {
			/* sanity check */
			if (isr->proxy == NULL)
				panic("ipsec_hdrsiz: proxy is NULL "
				      "but tunnel mode.\n");

			switch (isr->proxy->sa_family) {
			case AF_INET:
				clen += sizeof(struct ip);
				break;
#ifdef INET6
			case AF_INET6:
				clen += sizeof(struct ip6_hdr);
				break;
#endif
			default:
				printf("ipsec_hdrsiz: unknown AF %d "
					"in IPsec tunnel SA\n",
					isr->proxy->sa_family);
				break;
			}
		}
		siz += clen;
	}

	return siz;
}

/* This function is called from ip_forward() and ipsec4_hdrsize_tcp(). */
size_t
ipsec4_hdrsiz(m, inp)
	struct mbuf *m;
	struct inpcb *inp;
{
	struct secpolicy *sp = NULL;
	int error;
	size_t size;

	/* sanity check */
	if (m == NULL)
		return 0;	/* XXX should be panic ? */
	if (inp != NULL && inp->inp_socket == NULL)
		panic("ipsec4_hdrsize: why is socket NULL but there is PCB.");

	/* get SP for this packet.
	 * When we are called from ip_forward(), we call
	 * ipsec4_getpolicybyaddr() with IP_FORWARDING flag.
	 */
	if (inp == NULL)
		sp = ipsec4_getpolicybyaddr(m, IP_FORWARDING, &error);
	else
		sp = ipsec4_getpolicybysock(m, inp->inp_socket, &error);

	if (sp == NULL)
		return 0;	/* XXX should be panic ? */

	size = ipsec_hdrsiz(sp);
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP ipsec4_hdrsiz call free SP:%p\n", sp));
	KEYDEBUG(KEYDEBUG_IPSEC_DATA,
		printf("ipsec4_hdrsiz: size:%lu.\n", (unsigned long)size));
	key_freesp(sp);

	return size;
}

#ifdef INET6
/* This function is called from ipsec6_hdrsize_tcp(),
 * and maybe from ip6_forward.()
 */
size_t
ipsec6_hdrsiz(m, in6p)
	struct mbuf *m;
	struct in6pcb *in6p;
{
	struct secpolicy *sp = NULL;
	int error;
	size_t size;

	/* sanity check */
	if (m == NULL)
		return 0;	/* XXX shoud be panic ? */
	if (in6p != NULL && in6p->in6p_socket == NULL)
		panic("ipsec6_hdrsize: why is socket NULL but there is PCB.");

	/* get SP for this packet */
	/* XXX Is it right to call with IP_FORWARDING. */
	if (in6p == NULL)
		sp = ipsec6_getpolicybyaddr(m, IP_FORWARDING, &error);
	else
		sp = ipsec6_getpolicybysock(m, in6p->in6p_socket, &error);

	if (sp == NULL)
		return 0;
	size = ipsec_hdrsiz(sp);
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP ipsec6_hdrsiz call free SP:%p\n", sp));
	KEYDEBUG(KEYDEBUG_IPSEC_DATA,
		printf("ipsec6_hdrsiz: size:%lu.\n", (unsigned long)size));
	key_freesp(sp);

	return size;
}
#endif /*INET6*/

#ifdef INET
/*
 * encapsulate for ipsec tunnel.
 * ip->ip_src must be fixed later on.
 */
static int
ipsec4_encapsulate(m, sa)
	struct mbuf *m;
	struct secas *sa;
{
	struct ip *oip;
	struct ip *ip;
	size_t hlen;
	size_t plen;

	/* can't tunnel between different AFs */
	if (sa->proxy->sa_family != AF_INET) {
		m_freem(m);
		return EINVAL;
	}
#if 0
	/* XXX if the proxy is myself, perform nothing. */
	if (sa->saidx && key_ismyaddr(AF_INET, sa->saidx->idx.dst)) {
		m_freem(m);
		return EINVAL;
	}
#endif

	ip = mtod(m, struct ip *);
#ifdef _IP_VHL
	hlen = _IP_VHL_HL(ip->ip_vhl) << 2;
#else
	hlen = ip->ip_hl << 2;
#endif

	/* generate header checksum */
	ip->ip_sum = 0;
#ifdef _IP_VHL
	if (ip->ip_vhl == IP_VHL_BORING)
		ip->ip_sum = in_cksum_hdr(ip);
	else
		ip->ip_sum = in_cksum(m, hlen);
#else
	ip->ip_sum = in_cksum(m, hlen);
#endif

	plen = m->m_pkthdr.len;

	/*
	 * grow the mbuf to accomodate the new IPv4 header.
	 * NOTE: IPv4 options will never be copied.
	 */
	if (m->m_len != hlen)
		panic("ipsec4_encapsulate: assumption failed (first mbuf length)");
	if (M_LEADINGSPACE(m->m_next) < hlen) {
		struct mbuf *n;
		MGET(n, M_DONTWAIT, MT_DATA);
		if (!n) {
			m_freem(m);
			return ENOBUFS;
		}
		n->m_len = hlen;
		n->m_next = m->m_next;
		m->m_next = n;
		m->m_pkthdr.len += hlen;
		oip = mtod(n, struct ip *);
	} else {
		m->m_next->m_len += hlen;
		m->m_next->m_data -= hlen;
		m->m_pkthdr.len += hlen;
		oip = mtod(m->m_next, struct ip *);
	}
	ip = mtod(m, struct ip *);
	ovbcopy((caddr_t)ip, (caddr_t)oip, hlen);
	m->m_len = sizeof(struct ip);
	m->m_pkthdr.len -= (hlen - sizeof(struct ip));

	/* construct new IPv4 header. see RFC 2401 5.1.2.1 */
	/* ECN consideration. */
	ip_ecn_ingress(ip4_ipsec_ecn, &ip->ip_tos, &oip->ip_tos);
#ifdef _IP_VHL
	ip->ip_vhl = IP_MAKE_VHL(IPVERSION, sizeof(struct ip) >> 2);
#else
	ip->ip_hl = sizeof(struct ip) >> 2;
#endif
	ip->ip_off &= htons(~IP_OFFMASK);
	ip->ip_off &= htons(~IP_MF);
	switch (ip4_ipsec_dfbit) {
	case 0:	/*clear DF bit*/
		ip->ip_off &= htons(~IP_DF);
		break;
	case 1:	/*set DF bit*/
		ip->ip_off |= htons(IP_DF);
		break;
	default:	/*copy DF bit*/
		break;
	}
	ip->ip_p = IPPROTO_IPIP;
	if (plen + sizeof(struct ip) < IP_MAXPACKET)
		ip->ip_len = htons(plen + sizeof(struct ip));
	else {
		printf("IPv4 ipsec: size exceeds limit: "
			"leave ip_len as is (invalid packet)\n");
	}
	ip->ip_id = htons(ip_id++);
	bcopy(&((struct sockaddr_in *)sa->proxy)->sin_addr,
		&ip->ip_dst, sizeof(ip->ip_dst));
	/* XXX ip_src must be updated later */

	return 0;
}
#endif /*INET*/

#ifdef INET6
static int
ipsec6_encapsulate(m, sa)
	struct mbuf *m;
	struct secas *sa;
{
	struct ip6_hdr *oip6;
	struct ip6_hdr *ip6;
	size_t plen;

	/* can't tunnel between different AFs */
	if (sa->proxy->sa_family != AF_INET6) {
		m_freem(m);
		return EINVAL;
	}
#if 0
	/* XXX if the proxy is myself, perform nothing. */
	if (sa->saidx && key_ismyaddr(AF_INET6, sa->saidx->dst)) {
		m_freem(m);
		return EINVAL;
	}
#endif

	plen = m->m_pkthdr.len;

	/*
	 * grow the mbuf to accomodate the new IPv6 header.
	 */
	if (m->m_len != sizeof(struct ip6_hdr))
		panic("ipsec6_encapsulate: assumption failed (first mbuf length)");
	if (M_LEADINGSPACE(m->m_next) < sizeof(struct ip6_hdr)) {
		struct mbuf *n;
		MGET(n, M_DONTWAIT, MT_DATA);
		if (!n) {
			m_freem(m);
			return ENOBUFS;
		}
		n->m_len = sizeof(struct ip6_hdr);
		n->m_next = m->m_next;
		m->m_next = n;
		m->m_pkthdr.len += sizeof(struct ip6_hdr);
		oip6 = mtod(n, struct ip6_hdr *);
	} else {
		m->m_next->m_len += sizeof(struct ip6_hdr);
		m->m_next->m_data -= sizeof(struct ip6_hdr);
		m->m_pkthdr.len += sizeof(struct ip6_hdr);
		oip6 = mtod(m->m_next, struct ip6_hdr *);
	}
	ip6 = mtod(m, struct ip6_hdr *);
	ovbcopy((caddr_t)ip6, (caddr_t)oip6, sizeof(struct ip6_hdr));

	/* Fake link-local scope-class addresses */
	if (IN6_IS_SCOPE_LINKLOCAL(&oip6->ip6_src))
		oip6->ip6_src.s6_addr16[1] = 0;
	if (IN6_IS_SCOPE_LINKLOCAL(&oip6->ip6_dst))
		oip6->ip6_dst.s6_addr16[1] = 0;

	/* construct new IPv6 header. see RFC 2401 5.1.2.2 */
	/* ECN consideration. */
	ip6_ecn_ingress(ip6_ipsec_ecn, &ip6->ip6_flow, &oip6->ip6_flow);
	if (plen < IPV6_MAXPACKET - sizeof(struct ip6_hdr))
		ip6->ip6_plen = htons(plen);
	else {
		/* ip6->ip6_plen will be updated in ip6_output() */
	}
	ip6->ip6_nxt = IPPROTO_IPV6;
	bcopy(&((struct sockaddr_in6 *)sa->proxy)->sin6_addr,
		&ip6->ip6_dst, sizeof(ip6->ip6_dst));
	/* XXX ip6_src must be updated later */

	return 0;
}
#endif /*INET6*/

/*
 * Check the variable replay window.
 * ipsec_chkreplay() performs replay check before ICV verification.
 * ipsec_updatereplay() updates replay bitmap.  This must be called after
 * ICV verification (it also performs replay check, which is usually done
 * beforehand).
 * 0 (zero) is returned if packet disallowed, 1 if packet permitted.
 *
 * based on RFC 2401.
 */
int
ipsec_chkreplay(seq, sa)
	u_int32_t seq;
	struct secas *sa;
{
	const struct secreplay *replay;
	u_int32_t diff;
	int fr;
	u_int32_t wsizeb;	/* constant: bits of window size */
	int frlast;		/* constant: last frame */

	/* sanity check */
	if (sa == NULL)
		printf("ipsec_chkreplay: NULL pointer was passed.\n");

	replay = sa->replay;

	if (replay->wsize == 0)
		return 1;	/* no need to check replay. */

	/* constant */
	frlast = replay->wsize - 1;
	wsizeb = replay->wsize << 3;

	/* sequence number of 0 is invalid */
	if (seq == 0)
		return 0;

	/* first time is always okay */
	if (replay->count == 0)
		return 1;

	if (seq > replay->lastseq) {
		/* larger sequences are okay */
		return 1;
	} else {
		/* seq is equal or less than lastseq. */
		diff = replay->lastseq - seq;

		/* over range to check, i.e. too old or wrapped */
		if (diff >= wsizeb)
			return 0;

		fr = frlast - diff / 8;

		/* this packet already seen ? */
		if ((replay->bitmap)[fr] & (1 << (diff % 8)))
			return 0;

		/* out of order but good */
		return 1;
	}
}

int
ipsec_updatereplay(seq, sa)
	u_int32_t seq;
	struct secas *sa;
{
	struct secreplay *replay;
	u_int32_t diff;
	int fr;
	u_int32_t wsizeb;	/* constant: bits of window size */
	int frlast;		/* constant: last frame */

	/* sanity check */
	if (sa == NULL)
		printf("ipsec_chkreplay: NULL pointer was passed.\n");

	replay = sa->replay;

	if (replay->wsize == 0)
		goto ok;	/* no need to check replay. */

	/* constant */
	frlast = replay->wsize - 1;
	wsizeb = replay->wsize << 3;

	/* sequence number of 0 is invalid */
	if (seq == 0)
		return 0;

	/* first time */
	if (replay->count == 0) {
		replay->lastseq = seq;
		bzero(replay->bitmap, replay->wsize);
		(replay->bitmap)[frlast] = 1;
		goto ok;
	}

	if (seq > replay->lastseq) {
		/* seq is larger than lastseq. */
		diff = seq - replay->lastseq;

		/* new larger sequence number */
		if (diff < wsizeb) {
			/* In window */
			/* set bit for this packet */
			vshiftl(replay->bitmap, diff, replay->wsize);
			(replay->bitmap)[frlast] |= 1;
		} else {
			/* this packet has a "way larger" */
			bzero(replay->bitmap, replay->wsize);
			(replay->bitmap)[frlast] = 1;
		}
		replay->lastseq = seq;

		/* larger is good */
	} else {
		/* seq is equal or less than lastseq. */
		diff = replay->lastseq - seq;

		/* over range to check, i.e. too old or wrapped */
		if (diff >= wsizeb)
			return 0;

		fr = frlast - diff / 8;

		/* this packet already seen ? */
		if ((replay->bitmap)[fr] & (1 << (diff % 8)))
			return 0;

		/* mark as seen */
		(replay->bitmap)[fr] |= (1 << (diff % 8));

		/* out of order but good */
	}

ok:
	if (replay->count == ~0
	 && (sa->flags & SADB_X_EXT_CYCSEQ) == 0) {
	 	return 1;	/* don't increment, no more packets accepted */
	}

	replay->count++;

	return 1;
}

/*
 * shift variable length bunffer to left.
 * IN:	bitmap: pointer to the buffer
 * 	nbit:	the number of to shift.
 *	wsize:	buffer size (bytes).
 */
static void
vshiftl(bitmap, nbit, wsize)
	unsigned char *bitmap;
	int nbit, wsize;
{
	int s, j, i;
	unsigned char over;

	for (j = 0; j < nbit; j += 8) {
		s = (nbit - j < 8) ? (nbit - j): 8;
		bitmap[0] <<= s;
		for (i = 1; i < wsize; i++) {
			over = (bitmap[i] >> (8 - s));
			bitmap[i] <<= s;
			bitmap[i-1] |= over;
		}
	}

	return;
}

const char *
ipsec4_logpacketstr(ip, spi)
	struct ip *ip;
	u_int32_t spi;
{
	static char buf[256];
	char *p;
	u_int8_t *s, *d;

	s = (u_int8_t *)(&ip->ip_src);
	d = (u_int8_t *)(&ip->ip_dst);

	snprintf(buf, sizeof(buf), "packet(SPI=%u ", (u_int32_t)ntohl(spi));
	for (p = buf; p && *p; p++)
		;
	snprintf(p, sizeof(buf) - (p - buf), "src=%d.%d.%d.%d",
		s[0], s[1], s[2], s[3]);
	for (/*nothing*/; p && *p; p++)
		;
	snprintf(p, sizeof(buf) - (p - buf), " dst=%d.%d.%d.%d",
		d[0], d[1], d[2], d[3]);
	for (/*nothing*/; p && *p; p++)
		;
	snprintf(p, sizeof(buf) - (p - buf), ")");

	return buf;
}

#ifdef INET6
const char *
ipsec6_logpacketstr(ip6, spi)
	struct ip6_hdr *ip6;
	u_int32_t spi;
{
	static char buf[256];
	char *p;

	snprintf(buf, sizeof(buf), "packet(SPI=%u ", (u_int32_t)ntohl(spi));
	for (p = buf; p && *p; p++)
		;
	snprintf(p, sizeof(buf) - (p - buf), "src=%s",
		ip6_sprintf(&ip6->ip6_src));
	for (/*nothing*/; p && *p; p++)
		;
	snprintf(p, sizeof(buf) - (p - buf), " dst=%s",
		ip6_sprintf(&ip6->ip6_dst));
	for (/*nothing*/; p && *p; p++)
		;
	snprintf(p, sizeof(buf) - (p - buf), ")");

	return buf;
}
#endif /*INET6*/

const char *
ipsec_logsastr(sa)
	struct secas *sa;
{
	static char buf[256];
	char *p;
	struct secindex *idx = &sa->saidx->idx;

	snprintf(buf, sizeof(buf), "SA(SPI=%u ", (u_int32_t)ntohl(sa->spi));
	for (p = buf; p && *p; p++)
		;
	if (idx->family == AF_INET) {
		u_int8_t *s, *d;
		s = (u_int8_t *)(&idx->src);
		d = (u_int8_t *)(&idx->dst);
		snprintf(p, sizeof(buf) - (p - buf),
			"src=%d.%d.%d.%d dst=%d.%d.%d.%d",
			s[0], s[1], s[2], s[3], d[0], d[1], d[2], d[3]);
	}
#ifdef INET6
	else if (idx->family == AF_INET6) {
		snprintf(p, sizeof(buf) - (p - buf),
			"src=%s", ip6_sprintf((struct in6_addr *)(&idx->src)));
		for (/*nothing*/; p && *p; p++)
			;
		snprintf(p, sizeof(buf) - (p - buf),
			" dst=%s", ip6_sprintf((struct in6_addr *)(&idx->dst)));
	}
#endif
	for (/*nothing*/; p && *p; p++)
		;
	snprintf(p, sizeof(buf) - (p - buf), ")");

	return buf;
}

void
ipsec_dumpmbuf(m)
	struct mbuf *m;
{
	int totlen;
	int i;
	u_char *p;

	totlen = 0;
	printf("---\n");
	while (m) {
		p = mtod(m, u_char *);
		for (i = 0; i < m->m_len; i++) {
			printf("%02x ", p[i]);
			totlen++;
			if (totlen % 16 == 0)
				printf("\n");
		}
		m = m->m_next;
	}
	if (totlen % 16 != 0)
		printf("\n");
	printf("---\n");
}

/*
 * IPsec output logic for IPv4.
 */
int
ipsec4_output(state, sp, flags)
	struct ipsec_output_state *state;
	struct secpolicy *sp;
	int flags;
{
	struct ip *ip = NULL;
	struct ipsecrequest *isr = NULL;
	int s;
	int error;
#ifdef IPSEC_SRCSEL
	struct in_ifaddr *ia;
#endif
	struct sockaddr_in *dst4;

	if (!state)
		panic("state == NULL in ipsec4_output");
	if (!state->m)
		panic("state->m == NULL in ipsec4_output");
	if (!state->ro)
		panic("state->ro == NULL in ipsec4_output");
	if (!state->dst)
		panic("state->dst == NULL in ipsec4_output");

	ip = mtod(state->m, struct ip *);

	KEYDEBUG(KEYDEBUG_IPSEC_DATA,
		printf("ipsec4_output: applyed SP\n");
		kdebug_secpolicy(sp));

	for (isr = sp->req; isr != NULL; isr = isr->next) {

#if 0	/* give up to check restriction of transport mode */
	/* XXX but should be checked somewhere */
		/*
		 * some of the IPsec operation must be performed only in
		 * originating case.
		 */
		if (isr->mode == IPSEC_MODE_TRANSPORT
		 && (flags & IP_FORWARDING))
			continue;
#endif

		if ((error = key_checkrequest(isr)) != 0) {
			/*
			 * IPsec processing is required, but no SA found.
			 * I assume that key_acquire() had been called
			 * to get/establish the SA. Here I discard
			 * this packet because it is responsibility for
			 * upper layer to retransmit the packet.
			 */
			ipsecstat.out_nosa++;
			goto bad;
		}

		/* validity check */
		if (isr->sa == NULL) {
			switch (isr->level) {
			case IPSEC_LEVEL_USE:
				continue;
			case IPSEC_LEVEL_REQUIRE:
				/* must be not reached here. */
				panic("ipsec4_output: no SA found, but required.");
			}
		}

		if (isr->sa->state != SADB_SASTATE_MATURE
		 && isr->sa->state != SADB_SASTATE_DYING) {
			/* If there is no valid SA, we give up to process. */
			ipsecstat.out_nosa++;
			error = EINVAL;
			goto bad;
		}

		/*
		 * There may be the case that SA status will be changed when
		 * we are refering to one. So calling splsoftnet().
		 */
#ifdef __NetBSD__
		s = splsoftnet();
#else
		s = splnet();
#endif

		if (isr->mode == IPSEC_MODE_TUNNEL && isr->proxy) {
			/*
			 * build IPsec tunnel.
			 */
			/* XXX should be processed with other familiy */
			if (isr->proxy->sa_family != AF_INET) {
				printf("ipsec4_output: "
					"wrong proxy specified for spi=%u\n",
					(u_int32_t)ntohl(isr->sa->spi));
				splx(s);
				error = EAFNOSUPPORT;
				goto bad;
			}

			/* validity check */
			if (isr->proxy->sa_family != isr->sa->proxy->sa_family
			 || bcmp(_INADDRBYSA(isr->proxy),
			         _INADDRBYSA(isr->sa->proxy),
			         _INALENBYAF(isr->proxy->sa_family)) != 0) {
				printf("ipsec4_output: proxy address mismatch.\n");
#if defined(IPSEC_DEBUG)
				printf("  SP: ");
				ipsec_hexdump((caddr_t)isr->proxy,
				        _SALENBYAF(isr->proxy->sa_family));
				printf("\n");
				printf("  SA : ");
				ipsec_hexdump((caddr_t)isr->sa->proxy,
				        _SALENBYAF(isr->sa->proxy->sa_family));
				printf("\n");
#endif /* defined(IPSEC_DEBUG) */
				printf("ipsec4_output: applyed SP's proxy.\n");
			}

			ip = mtod(state->m, struct ip *);
#if 0 /* XXX */
			if (!key_checktunnelsanity(isr->sa, AF_INET,
					(caddr_t)&ip->ip_src,
					(caddr_t)&ip->ip_dst)) {
				printf("ipsec4_output: internal error: "
					"ipsec packet does not match SAD/SPD: "
					"%x->%x, SPI=%u\n",
					(u_int32_t)ntohl(ip->ip_src.s_addr),
					(u_int32_t)ntohl(ip->ip_dst.s_addr),
					(u_int32_t)ntohl(isr->sa->spi));
				splx(s);
				error = EINVAL;
				goto bad;
			}
#endif

			state->m = ipsec4_splithdr(state->m);
			if (!state->m) {
				splx(s);
				error = ENOMEM;
				goto bad;
			}
			error = ipsec4_encapsulate(state->m, isr->sa);
			splx(s);
			if (error) {
				state->m = NULL;
				goto bad;
			}
			ip = mtod(state->m, struct ip *);

			state->ro = &isr->sa->saidx->sa_route;
			state->dst = (struct sockaddr *)&state->ro->ro_dst;
			dst4 = (struct sockaddr_in *)state->dst;
			if (state->ro->ro_rt
			 && ((state->ro->ro_rt->rt_flags & RTF_UP) == 0
			  || dst4->sin_addr.s_addr != ip->ip_dst.s_addr)) {
				RTFREE(state->ro->ro_rt);
				bzero((caddr_t)state->ro, sizeof (*state->ro));
			}
			if (state->ro->ro_rt == 0) {
				dst4->sin_family = AF_INET;
				dst4->sin_len = sizeof(*dst4);
				dst4->sin_addr = ip->ip_dst;
				rtalloc(state->ro);
			}
			if (state->ro->ro_rt == 0) {
				ipstat.ips_noroute++;
				error = EHOSTUNREACH;
				goto bad;
			}

#ifdef IPSEC_SRCSEL
			/*
			 * Which address in SA or in routing table should I
			 * select from ?  But I had set from SA at
			 * ipsec4_encapsulate().
			 */
			ia = (struct in_ifaddr *)(state->ro->ro_rt->rt_ifa);
			if (state->ro->ro_rt->rt_flags & RTF_GATEWAY) {
				state->dst = (struct sockaddr *)state->ro->ro_rt->rt_gateway;
				dst4 = (struct sockaddr_in *)state->dst;
			}
			ip->ip_src = IA_SIN(ia)->sin_addr;
#endif
		} else
			splx(s);

		state->m = ipsec4_splithdr(state->m);
		if (!state->m) {
			error = ENOMEM;
			goto bad;
		}
		switch (isr->proto) {
		case IPPROTO_ESP:
#ifdef IPSEC_ESP
			if ((error = esp4_output(state->m, isr)) != 0) {
				state->m = NULL;
				goto bad;
			}
			break;
#else
			m_freem(state->m);
			state->m = NULL;
			error = EINVAL;
			goto bad;
#endif
		case IPPROTO_AH:
			if ((error = ah4_output(state->m, isr)) != 0) {
				state->m = NULL;
				goto bad;
			}
			break;
		case IPPROTO_IPCOMP:
			if ((error = ipcomp4_output(state->m, isr)) != 0) {
				state->m = NULL;
				goto bad;
			}
			break;
		default:
			printf("ipsec4_output: unknown ipsec protocol %d\n", isr->proto);
			m_freem(state->m);
			state->m = NULL;
			error = EINVAL;
			goto bad;
		}

		if (state->m == 0) {
			error = ENOMEM;
			goto bad;
		}
		ip = mtod(state->m, struct ip *);
	}

	return 0;

bad:
	m_freem(state->m);
	state->m = NULL;
	return error;
}

#ifdef INET6
/*
 * IPsec output logic for IPv6, transport mode.
 */
int
ipsec6_output_trans(state, nexthdrp, mprev, sp, flags, tun)
	struct ipsec_output_state *state;
	u_char *nexthdrp;
	struct mbuf *mprev;
	struct secpolicy *sp;
	int flags;
	int *tun;
{
	struct ip6_hdr *ip6;
	struct ipsecrequest *isr = NULL;
	int error = 0;
	int plen;

	if (!state)
		panic("state == NULL in ipsec6_output");
	if (!state->m)
		panic("state->m == NULL in ipsec6_output");
	if (!nexthdrp)
		panic("nexthdrp == NULL in ipsec6_output");
	if (!mprev)
		panic("mprev == NULL in ipsec6_output");
	if (!sp)
		panic("sp == NULL in ipsec6_output");
	if (!tun)
		panic("tun == NULL in ipsec6_output");

	KEYDEBUG(KEYDEBUG_IPSEC_DATA,
		printf("ipsec6_output: applyed SP\n");
		kdebug_secpolicy(sp));

	*tun = 0;
	for (isr = sp->req; isr; isr = isr->next) {
		if (isr->mode == IPSEC_MODE_TUNNEL) {
			/* the rest will be handled by ipsec6_output_tunnel() */
			break;
		}

		if (key_checkrequest(isr) == ENOENT) {
			/*
			 * IPsec processing is required, but no SA found.
			 * I assume that key_acquire() had been called
			 * to get/establish the SA. Here I discard
			 * this packet because it is responsibility for
			 * upper layer to retransmit the packet.
			 */
			ipsec6stat.out_nosa++;
			error = ENOENT;
			goto bad;
		}

		/* validity check */
		if (isr->sa == NULL) {
			switch (isr->level) {
			case IPSEC_LEVEL_USE:
				continue;
			case IPSEC_LEVEL_REQUIRE:
				/* must be not reached here. */
				panic("ipsec6_output: no SA found, but required.");
			}
		}

		if (isr->sa->state != SADB_SASTATE_MATURE
		 && isr->sa->state != SADB_SASTATE_DYING) {
			/* If there is no valid SA, we give up to process. */
			ipsec6stat.out_nosa++;
			error = EINVAL;
			goto bad;
		}

		switch (isr->proto) {
		case IPPROTO_ESP:
#ifdef IPSEC_ESP
			error = esp6_output(state->m, nexthdrp, mprev->m_next, isr);
#else
			m_freem(state->m);
			error = EINVAL;
#endif
			break;
		case IPPROTO_AH:
			error = ah6_output(state->m, nexthdrp, mprev->m_next, isr);
			break;
		case IPPROTO_IPCOMP:
			error = ipcomp6_output(state->m, nexthdrp, mprev->m_next, isr);
			break;
		default:
			printf("ipsec6_output_trans: unknown ipsec protocol %d\n", isr->proto);
			m_freem(state->m);
			error = EINVAL;
			break;
		}
		if (error) {
			state->m = NULL;
			goto bad;
		}
		plen = state->m->m_pkthdr.len - sizeof(struct ip6_hdr);
		if (plen > IPV6_MAXPACKET) {
			printf("ipsec6_output: IPsec with IPv6 jumbogram is not supported\n");
			ipsec6stat.out_inval++;
			error = EINVAL;	/*XXX*/
			goto bad;
		}
		ip6 = mtod(state->m, struct ip6_hdr *);
		ip6->ip6_plen = htons(plen);
	}

	/* if we have more to go, we need a tunnel mode processing */
	if (isr != NULL)
		*tun = 1;

	return 0;

bad:
	m_freem(state->m);
	state->m = NULL;
	return error;
}

/*
 * IPsec output logic for IPv6, tunnel mode.
 */
int
ipsec6_output_tunnel(state, sp, flags)
	struct ipsec_output_state *state;
	struct secpolicy *sp;
	int flags;
{
	struct ip6_hdr *ip6;
	struct ipsecrequest *isr = NULL;
	int error = 0;
	int plen;
#ifdef IPSEC_SRCSEL
	struct in6_addr *ia6;
#endif
	struct sockaddr_in6* dst6;
	int s;

	if (!state)
		panic("state == NULL in ipsec6_output");
	if (!state->m)
		panic("state->m == NULL in ipsec6_output");
	if (!sp)
		panic("sp == NULL in ipsec6_output");

	KEYDEBUG(KEYDEBUG_IPSEC_DATA,
		printf("ipsec6_output_tunnel: applyed SP\n");
		kdebug_secpolicy(sp));

	/*
	 * transport mode ipsec (before the 1st tunnel mode) is already
	 * processed by ipsec6_output_trans().
	 */
	for (isr = sp->req; isr; isr = isr->next) {
		if (isr->mode == IPSEC_MODE_TUNNEL)
			break;
	}

	for (/*already initialized*/; isr; isr = isr->next) {
		if (key_checkrequest(isr) == ENOENT) {
			/*
			 * IPsec processing is required, but no SA found.
			 * I assume that key_acquire() had been called
			 * to get/establish the SA. Here I discard
			 * this packet because it is responsibility for
			 * upper layer to retransmit the packet.
			 */
			ipsec6stat.out_nosa++;
			error = ENOENT;
			goto bad;
		}

		/* validity check */
		if (isr->sa == NULL) {
			switch (isr->level) {
			case IPSEC_LEVEL_USE:
				continue;
			case IPSEC_LEVEL_REQUIRE:
				/* must be not reached here. */
				panic("ipsec6_output_tunnel: no SA found, but required.");
			}
		}

		if (isr->sa->state != SADB_SASTATE_MATURE
		 && isr->sa->state != SADB_SASTATE_DYING) {
			/* If there is no valid SA, we give up to process. */
			ipsec6stat.out_nosa++;
			error = EINVAL;
			goto bad;
		}

		/*
		 * There may be the case that SA status will be changed when
		 * we are refering to one. So calling splsoftnet().
		 */
#ifdef __NetBSD__
		s = splsoftnet();
#else
		s = splnet();
#endif

		if (isr->mode == IPSEC_MODE_TUNNEL && isr->proxy) {
			/*
			 * build IPsec tunnel.
			 */
			/* XXX should be processed with other familiy */
			if (isr->proxy->sa_family != AF_INET6) {
				printf("ipsec6_output_tunnel: "
					"wrong proxy specified for spi=%u\n",
					(u_int32_t)ntohl(isr->sa->spi));
				splx(s);
				error = EAFNOSUPPORT;
				goto bad;
			}

			/* validity check */
			if (isr->proxy->sa_family != isr->sa->proxy->sa_family
			 || bcmp(_INADDRBYSA(isr->proxy),
			         _INADDRBYSA(isr->sa->proxy),
			         _INALENBYAF(isr->proxy->sa_family)) != 0) {
				printf("ipsec6_output_tunnel: proxy address mismatch.\n");
#if defined(IPSEC_DEBUG)
				printf("  SP: ");
				ipsec_hexdump((caddr_t)isr->proxy,
				        _SALENBYAF(isr->proxy->sa_family));
				printf("\n");
				printf("  SA : ");
				ipsec_hexdump((caddr_t)isr->sa->proxy,
				        _SALENBYAF(isr->sa->proxy->sa_family));
				printf("\n");
#endif /* defined(IPSEC_DEBUG) */
				printf("ipsec6_output_tunnel: applyed SP's proxy.\n");
			}

			ip6 = mtod(state->m, struct ip6_hdr *);
#if 0 /* XXX */
			if (!key_checktunnelsanity(isr->sa, AF_INET6,
					(caddr_t)&ip6->ip6_src,
					(caddr_t)&ip6->ip6_dst)) {
				printf("ipsec6_output_tunnel: internal error: "
					"ipsec packet does not match SAD/SPD: "
					"SPI=%u\n",
					(u_int32_t)ntohl(isr->sa->spi));
				splx(s);
				error = EINVAL;
				goto bad;
			}
#endif

			state->m = ipsec6_splithdr(state->m);
			if (!state->m) {
				splx(s);
				error = ENOMEM;
				goto bad;
			}
			error = ipsec6_encapsulate(state->m, isr->sa);
			splx(s);
			if (error) {
				state->m = 0;
				goto bad;
			}
			ip6 = mtod(state->m, struct ip6_hdr *);

			state->ro = &isr->sa->saidx->sa_route;
			state->dst = (struct sockaddr *)&state->ro->ro_dst;
			dst6 = (struct sockaddr_in6 *)state->dst;
			if (state->ro->ro_rt
			 && ((state->ro->ro_rt->rt_flags & RTF_UP) == 0
			  || !IN6_ARE_ADDR_EQUAL(&dst6->sin6_addr, &ip6->ip6_dst))) {
				RTFREE(state->ro->ro_rt);
				bzero((caddr_t)state->ro, sizeof (*state->ro));
			}
			if (state->ro->ro_rt == 0) {
				bzero(dst6, sizeof(*dst6));
				dst6->sin6_family = AF_INET6;
				dst6->sin6_len = sizeof(*dst6);
				dst6->sin6_addr = ip6->ip6_dst;
				rtalloc(state->ro);
			}
			if (state->ro->ro_rt == 0) {
				ip6stat.ip6s_noroute++;
				error = EHOSTUNREACH;
				goto bad;
			}
#if 0	/* XXX Is the following need ? */
			if (state->ro->ro_rt->rt_flags & RTF_GATEWAY) {
				state->dst = (struct sockaddr *)state->ro->ro_rt->rt_gateway;
				dst6 = (struct sockaddr_in6 *)state->dst;
			}
#endif
#ifdef IPSEC_SELSRC
			ia6 = in6_selectsrc(dst6, NULL, NULL,
					    (struct route_in6 *)state->ro,
					    NULL, &error);
			if (ia6 == NULL)
				goto bad;
			ip6->ip6_src = *ia6;
#endif
		} else
			splx(s);

		state->m = ipsec6_splithdr(state->m);
		if (!state->m) {
			error = ENOMEM;
			goto bad;
		}
		ip6 = mtod(state->m, struct ip6_hdr *);
		switch (isr->proto) {
		case IPPROTO_ESP:
#ifdef IPSEC_ESP
			error = esp6_output(state->m, &ip6->ip6_nxt, state->m->m_next, isr);
#else
			m_freem(state->m);
			error = EINVAL;
#endif
			break;
		case IPPROTO_AH:
			error = ah6_output(state->m, &ip6->ip6_nxt, state->m->m_next, isr);
			break;
		default:
			printf("ipsec6_output_tunnel: unknown ipsec protocol %d\n", isr->proto);
			m_freem(state->m);
			error = EINVAL;
			break;
		}
		if (error) {
			state->m = NULL;
			goto bad;
		}
		plen = state->m->m_pkthdr.len - sizeof(struct ip6_hdr);
		if (plen > IPV6_MAXPACKET) {
			printf("ipsec6_output_tunnel: IPsec with IPv6 jumbogram is not supported\n");
			ipsec6stat.out_inval++;
			error = EINVAL;	/*XXX*/
			goto bad;
		}
		ip6 = mtod(state->m, struct ip6_hdr *);
		ip6->ip6_plen = htons(plen);
	}

	return 0;

bad:
	m_freem(state->m);
	state->m = NULL;
	return error;
}
#endif /*INET6*/

/*
 * Chop IP header and option off from the payload.
 */
static struct mbuf *
ipsec4_splithdr(m)
	struct mbuf *m;
{
	struct mbuf *mh;
	struct ip *ip;
	int hlen;

	if (m->m_len < sizeof(struct ip))
		panic("ipsec4_splithdr: first mbuf too short");
	ip = mtod(m, struct ip *);
#ifdef _IP_VHL
	hlen = _IP_VHL_HL(ip->ip_vhl) << 2;
#else
	hlen = ip->ip_hl << 2;
#endif
	if (m->m_len > hlen) {
		MGETHDR(mh, M_DONTWAIT, MT_HEADER);
		if (!mh) {
			m_freem(m);
			return NULL;
		}
		M_COPY_PKTHDR(mh, m);
		MH_ALIGN(mh, hlen);
		m->m_flags &= ~M_PKTHDR;
		m->m_len -= hlen;
		m->m_data += hlen;
		mh->m_next = m;
		m = mh;
		m->m_len = hlen;
		bcopy((caddr_t)ip, mtod(m, caddr_t), hlen);
	} else if (m->m_len < hlen) {
		m = m_pullup(m, hlen);
		if (!m)
			return NULL;
	}
	return m;
}

#ifdef INET6
static struct mbuf *
ipsec6_splithdr(m)
	struct mbuf *m;
{
	struct mbuf *mh;
	struct ip6_hdr *ip6;
	int hlen;

	if (m->m_len < sizeof(struct ip6_hdr))
		panic("ipsec6_splithdr: first mbuf too short");
	ip6 = mtod(m, struct ip6_hdr *);
	hlen = sizeof(struct ip6_hdr);
	if (m->m_len > hlen) {
		MGETHDR(mh, M_DONTWAIT, MT_HEADER);
		if (!mh) {
			m_freem(m);
			return NULL;
		}
		M_COPY_PKTHDR(mh, m);
		MH_ALIGN(mh, hlen);
		m->m_flags &= ~M_PKTHDR;
		m->m_len -= hlen;
		m->m_data += hlen;
		mh->m_next = m;
		m = mh;
		m->m_len = hlen;
		bcopy((caddr_t)ip6, mtod(m, caddr_t), hlen);
	} else if (m->m_len < hlen) {
		m = m_pullup(m, hlen);
		if (!m)
			return NULL;
	}
	return m;
}
#endif

/* validate inbound IPsec tunnel packet. */
int
ipsec4_tunnel_validate(ip, nxt0, sa)
	struct ip *ip;
	u_int nxt0;
	struct secas *sa;
{
	u_int8_t nxt = nxt0 & 0xff;
	struct sockaddr_in *sin;
	int hlen;

	if (nxt != IPPROTO_IPV4)
		return 0;
#ifdef _IP_VHL
	hlen = _IP_VHL_HL(ip->ip_vhl) << 2;
#else
	hlen = ip->ip_hl << 2;
#endif
	if (hlen != sizeof(struct ip))
		return 0;
	if (sa->proxy == NULL)
		return 0;
	switch (sa->proxy->sa_family) {
	case AF_INET:
		sin = (struct sockaddr_in *)sa->proxy;
		if (bcmp(&ip->ip_dst, &sin->sin_addr, sizeof(ip->ip_dst)) != 0)
			return 0;
		break;
#ifdef INET6
	case AF_INET6:
		/* should be supported, but at this moment we don't. */
		/*FALLTHROUGH*/
#endif
	default:
		return 0;
	}

	return 1;
}

#ifdef INET6
/* validate inbound IPsec tunnel packet. */
int
ipsec6_tunnel_validate(ip6, nxt0, sa)
	struct ip6_hdr *ip6;
	u_int nxt0;
	struct secas *sa;
{
	u_int8_t nxt = nxt0 & 0xff;
	struct sockaddr_in6 *sin6;

	if (nxt != IPPROTO_IPV6)
		return 0;
	if (sa->proxy == NULL)
		return 0;
	switch (sa->proxy->sa_family) {
	case AF_INET6:
		sin6 = (struct sockaddr_in6 *)sa->proxy;
		if (!IN6_ARE_ADDR_EQUAL(&ip6->ip6_dst, &sin6->sin6_addr))
			return 0;
		break;
	case AF_INET:
		/* should be supported, but at this moment we don't. */
		/*FALLTHROUGH*/
	default:
		return 0;
	}

	return 1;
}
#endif

/*
 * Make a mbuf chain for encryption.
 * If the original mbuf chain contains a mbuf with a cluster,
 * allocate a new cluster and copy the data to the new cluster.
 * XXX: this hack is inefficient, but is necessary to handle cases
 * of TCP retransmission...
 */
struct mbuf *
ipsec_copypkt(m)
	struct mbuf *m;
{
	struct mbuf *n, **mpp, *mnew;

	for (n = m, mpp = &m; n; n = n->m_next) {
		if (n->m_flags & M_EXT) {
			/*
			 * Make a copy only if there are more than one references
			 * to the cluster.
			 * XXX: is this approach effective?
			 */
			if (
#ifdef __bsdi__
				n->m_ext.ext_func ||
#else
				n->m_ext.ext_free ||
#endif 
#ifdef __NetBSD__
				MCLISREFERENCED(n)
#else
				mclrefcnt[mtocl(n->m_ext.ext_buf)] > 1
#endif
			    )
			{
				int remain, copied;
				struct mbuf *mm;

				if (n->m_flags & M_PKTHDR) {
					MGETHDR(mnew, M_DONTWAIT, MT_HEADER);
					if (mnew == NULL)
						goto fail;
					mnew->m_pkthdr = n->m_pkthdr;
					mnew->m_flags = n->m_flags & M_COPYFLAGS;
				}
				else {
					MGET(mnew, M_DONTWAIT, MT_DATA);
					if (mnew == NULL)
						goto fail;
				}
				mnew->m_len = 0;
				mm = mnew;

				/*
				 * Copy data. If we don't have enough space to
				 * store the whole data, allocate a cluster
				 * or additional mbufs.
				 * XXX: we don't use m_copyback(), since the
				 * function does not use clusters and thus is
				 * inefficient.
				 */
				remain = n->m_len;
				copied = 0;
				while(1) {
					int len;
					struct mbuf *mn;

					if (remain <= (mm->m_flags & M_PKTHDR ? MHLEN : MLEN))
						len = remain;
					else { /* allocate a cluster */
						MCLGET(mm, M_DONTWAIT);
						if (!(mm->m_flags & M_EXT)) {
							m_free(mm);
							goto fail;
						}
						len = remain < MCLBYTES ?
							remain : MCLBYTES;
					}

					bcopy(n->m_data + copied, mm->m_data,
					      len);

					copied += len;
					remain -= len;
					mm->m_len = len;

					if (remain <= 0) /* completed? */
						break;

					/* need another mbuf */
					MGETHDR(mn, M_DONTWAIT, MT_HEADER);
					if (mn == NULL)
						goto fail;
					mm->m_next = mn;
					mm = mn;
				}

				/* adjust chain */
				mm->m_next = m_free(n);
				n = mm;
				*mpp = mnew;
				mpp = &n->m_next;

				continue;
			}
		}
		*mpp = n;
		mpp = &n->m_next;
	}

	return(m);
  fail:
	m_freem(m);
	return(NULL);
}

#ifdef __bsdi__
/*
 * System control for IP
 */
u_char	ipsecctlerrmap[PRC_NCMDS] = {
	0,		0,		0,		0,
	0,		EMSGSIZE,	EHOSTDOWN,	EHOSTUNREACH,
	EHOSTUNREACH,	EHOSTUNREACH,	ECONNREFUSED,	ECONNREFUSED,
	EMSGSIZE,	EHOSTUNREACH,	0,		0,
	0,		0,		0,		0,
	ENOPROTOOPT
};

int *ipsec_sysvars[] = IPSECCTL_VARS;

int
ipsec_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int	*name;
	u_int	namelen;
	void	*oldp;
	size_t	*oldlenp;
	void	*newp;
	size_t	newlen;
{
	if (name[0] >= IPSECCTL_MAXID)
		return (EOPNOTSUPP);

	switch (name[0]) {
	case IPSECCTL_STATS:
		return sysctl_rdtrunc(oldp, oldlenp, newp, &ipsecstat,
		    sizeof(ipsecstat));
	case IPSECCTL_DEF_POLICY:
		if (newp != NULL && newlen == sizeof(int)) {
			switch (*(int *)newp) {
			case IPSEC_POLICY_DISCARD:
			case IPSEC_POLICY_NONE:
				break;
			default:
				return EINVAL;
			}
		}
		return (sysctl_int_arr(ipsec_sysvars, name, namelen,
		    oldp, oldlenp, newp, newlen));
	case IPSECCTL_DEF_ESP_TRANSLEV:
	case IPSECCTL_DEF_ESP_NETLEV:
	case IPSECCTL_DEF_AH_TRANSLEV:	
	case IPSECCTL_DEF_AH_NETLEV:
		if (newp != NULL && newlen == sizeof(int)) {
			switch (*(int *)newp) {
			case IPSEC_LEVEL_USE:
			case IPSEC_LEVEL_REQUIRE:
				break;
			default:
				return EINVAL;
			}
		}
		return (sysctl_int_arr(ipsec_sysvars, name, namelen,
		    oldp, oldlenp, newp, newlen));
	default:
		return (sysctl_int_arr(ipsec_sysvars, name, namelen,
		    oldp, oldlenp, newp, newlen));
	}
}

#ifdef INET6
/*
 * System control for IP6
 */
u_char	ipsec6ctlerrmap[PRC_NCMDS] = {
	0,		0,		0,		0,
	0,		EMSGSIZE,	EHOSTDOWN,	EHOSTUNREACH,
	EHOSTUNREACH,	EHOSTUNREACH,	ECONNREFUSED,	ECONNREFUSED,
	EMSGSIZE,	EHOSTUNREACH,	0,		0,
	0,		0,		0,		0,
	ENOPROTOOPT
};

int *ipsec6_sysvars[] = IPSEC6CTL_VARS;

int
ipsec6_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int	*name;
	u_int	namelen;
	void	*oldp;
	size_t	*oldlenp;
	void	*newp;
	size_t	newlen;
{
	if (name[0] >= IPSECCTL_MAXID)	/* xxx no 6 in this definition */
		return (EOPNOTSUPP);

	switch (name[0]) {
	case IPSECCTL_STATS:	/* xxx no 6 in this definition */
		return sysctl_rdtrunc(oldp, oldlenp, newp, &ipsec6stat,
		    sizeof(ipsec6stat));
	case IPSECCTL_DEF_POLICY:
		if (newp != NULL && newlen == sizeof(int)) {
			switch (*(int *)newp) {
			case IPSEC_POLICY_DISCARD:
			case IPSEC_POLICY_NONE:
				break;
			default:
				return EINVAL;
			}
		}
		return (sysctl_int_arr(ipsec6_sysvars, name, namelen,
		    oldp, oldlenp, newp, newlen));
	case IPSECCTL_DEF_ESP_TRANSLEV:
	case IPSECCTL_DEF_ESP_NETLEV:
	case IPSECCTL_DEF_AH_TRANSLEV:	
	case IPSECCTL_DEF_AH_NETLEV:
		if (newp != NULL && newlen == sizeof(int)) {
			switch (*(int *)newp) {
			case IPSEC_LEVEL_USE:
			case IPSEC_LEVEL_REQUIRE:
				break;
			default:
				return EINVAL;
			}
		}
		return (sysctl_int_arr(ipsec6_sysvars, name, namelen,
		    oldp, oldlenp, newp, newlen));
	default:
		return (sysctl_int_arr(ipsec6_sysvars, name, namelen,
		    oldp, oldlenp, newp, newlen));
	}
}
#endif /*INET6*/
#endif /*__bsdi__*/


#ifdef __NetBSD__
/*
 * System control for IPSEC
 */
u_char	ipsecctlermap[PRC_NCMDS] = {
	0,		0,		0,		0,
	0,		EMSGSIZE,	EHOSTDOWN,	EHOSTUNREACH,
	EHOSTUNREACH,	EHOSTUNREACH,	ECONNREFUSED,	ECONNREFUSED,
	EMSGSIZE,	EHOSTUNREACH,	0,		0,
	0,		0,		0,		0,
	ENOPROTOOPT
};

int *ipsec_sysvars[] = IPSECCTL_VARS;

int
ipsec_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
		return ENOTDIR;

	/* common sanity checks */
	switch (name[0]) {
	case IPSECCTL_DEF_ESP_TRANSLEV:
	case IPSECCTL_DEF_ESP_NETLEV:
	case IPSECCTL_DEF_AH_TRANSLEV:
	case IPSECCTL_DEF_AH_NETLEV:
		if (newp != NULL && newlen == sizeof(int)) {
			switch (*(int *)newp) {
			case IPSEC_LEVEL_USE:
			case IPSEC_LEVEL_REQUIRE:
				break;
			default:
				return EINVAL;
			}
		}
	}

	switch (name[0]) {

	case IPSECCTL_STATS:
		return sysctl_struct(oldp, oldlenp, newp, newlen,
				     &ipsecstat, sizeof(ipsecstat));
	case IPSECCTL_DEF_POLICY:
		if (newp != NULL && newlen == sizeof(int)) {
			switch (*(int *)newp) {
			case IPSEC_POLICY_DISCARD:
			case IPSEC_POLICY_NONE:
				break;
			default:
				return EINVAL;
			}
		}
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip4_def_policy.policy);
	case IPSECCTL_DEF_ESP_TRANSLEV:
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip4_esp_trans_deflev);
	case IPSECCTL_DEF_ESP_NETLEV:
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip4_esp_net_deflev);
	case IPSECCTL_DEF_AH_TRANSLEV:
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip4_ah_trans_deflev);
	case IPSECCTL_DEF_AH_NETLEV:
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip4_ah_net_deflev);
	case IPSECCTL_INBOUND_CALL_IKE:
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip4_inbound_call_ike);
	case IPSECCTL_AH_CLEARTOS:
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip4_ah_cleartos);
	case IPSECCTL_AH_OFFSETMASK:
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip4_ah_offsetmask);
	case IPSECCTL_DFBIT:
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip4_ipsec_dfbit);
	case IPSECCTL_ECN:
		return sysctl_int(oldp, oldlenp, newp, newlen, &ip4_ipsec_ecn);
	default:
		return EOPNOTSUPP;
	}
	/* NOTREACHED */
}

#ifdef INET6
/*
 * System control for IPSEC6
 */
u_char	ipsec6ctlermap[PRC_NCMDS] = {
	0,		0,		0,		0,
	0,		EMSGSIZE,	EHOSTDOWN,	EHOSTUNREACH,
	EHOSTUNREACH,	EHOSTUNREACH,	ECONNREFUSED,	ECONNREFUSED,
	EMSGSIZE,	EHOSTUNREACH,	0,		0,
	0,		0,		0,		0,
	ENOPROTOOPT
};

int *ipsec6_sysvars[] = IPSEC6CTL_VARS;

int
ipsec6_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
		return ENOTDIR;

	/* common sanity checks */
	switch (name[0]) {
	case IPSECCTL_DEF_ESP_TRANSLEV:
	case IPSECCTL_DEF_ESP_NETLEV:
	case IPSECCTL_DEF_AH_TRANSLEV:
	case IPSECCTL_DEF_AH_NETLEV:
		if (newp != NULL && newlen == sizeof(int)) {
			switch (*(int *)newp) {
			case IPSEC_LEVEL_USE:
			case IPSEC_LEVEL_REQUIRE:
				break;
			default:
				return EINVAL;
			}
		}
	}

	switch (name[0]) {

	case IPSECCTL_STATS:
		return sysctl_struct(oldp, oldlenp, newp, newlen,
				     &ipsec6stat, sizeof(ipsec6stat));
	case IPSECCTL_DEF_POLICY:
		if (newp != NULL && newlen == sizeof(int)) {
			switch (*(int *)newp) {
			case IPSEC_POLICY_DISCARD:
			case IPSEC_POLICY_NONE:
				break;
			default:
				return EINVAL;
			}
		}
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip6_def_policy.policy);
	case IPSECCTL_DEF_ESP_TRANSLEV:
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip6_esp_trans_deflev);
	case IPSECCTL_DEF_ESP_NETLEV:
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip6_esp_net_deflev);
	case IPSECCTL_DEF_AH_TRANSLEV:
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip6_ah_trans_deflev);
	case IPSECCTL_DEF_AH_NETLEV:
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip6_ah_net_deflev);
	case IPSECCTL_INBOUND_CALL_IKE:
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &ip6_inbound_call_ike);
	case IPSECCTL_ECN:
		return sysctl_int(oldp, oldlenp, newp, newlen, &ip6_ipsec_ecn);
	default:
		return EOPNOTSUPP;
	}
	/* NOTREACHED */
}
#endif /*INET6*/

#endif /* __NetBSD__ */
