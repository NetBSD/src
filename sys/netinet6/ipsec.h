/*	$NetBSD: ipsec.h,v 1.3 1999/07/03 21:30:19 thorpej Exp $	*/

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

#ifndef _NETINET6_IPSEC_H_
#define _NETINET6_IPSEC_H_

#include <netkey/keyv2.h>
#include <netkey/keydb.h>

#ifdef KERNEL

/* Security Policy Data Base */
struct secpolicy {
	struct secpolicy *next;
	struct secpolicy *prev;
	struct keytree *spt;	/* back pointer to the top of SPD */

	struct secindex idx;	/* index */

	int refcnt;		/* reference count */
	u_int state;		/* 0: dead, others: alive */
#define IPSEC_SPSTATE_DEAD	0
#define IPSEC_SPSTATE_ALIVE	1

	u_int policy;		/* DISCARD, NONE or IPSEC, see keyv2.h */
	struct ipsecrequest *req;
				/* pointer to the ipsec request tree, */
				/* if policy == IPSEC else this value == NULL.*/
};

/* Request for IPsec */
struct ipsecrequest {
	struct ipsecrequest *next;
				/* pointer to next structure */
				/* If 0, it means end of chain. */

	u_int proto;		/* IPPROTO_ESP or IPPROTO_AH */
	u_int mode;		/* mode for security protocol, see below. */
	u_int level;		/* IPsec level defined below. */
	struct sockaddr *proxy;	/* Destination address in Outer IP header. */
				/* If mode == TRANSPORT, it must be set NULL. */
	struct secas *sa;	/* place holder for the security association */

	struct secpolicy *sp;	/* back pointer to SP */
};

#endif /*KERNEL*/

#define	IPSEC_MODE_TRANSPORT	0
#define	IPSEC_MODE_TUNNEL	1

/* Policy level */
/*
 * IPSEC, ENTRUST and BYPASS are allowd for setsockopt() in PCB,
 * DISCARD, IPSEC and NONE are allowd for setkey() in SPD.
 * DISCARD and NONE are allowd for system default.
 */
#define IPSEC_POLICY_DISCARD	0	/* discarding packet */
#define IPSEC_POLICY_NONE	1	/* through IPsec engine */
#define IPSEC_POLICY_IPSEC	2	/* do IPsec */
#define IPSEC_POLICY_ENTRUST	3	/* consulting SPD if present. */
#define IPSEC_POLICY_BYPASS	4	/* only for privileged socket. */

/* Security protocol level */
#define	IPSEC_LEVEL_DEFAULT	0	/* reference to system default */
#define	IPSEC_LEVEL_USE		1	/* use SA if present. */
#define	IPSEC_LEVEL_REQUIRE	2	/* require SA. */

#define IPSEC_REPLAYWSIZE  32

/* statistics for ipsec processing */
struct ipsecstat {
	u_long in_success;  /* succeeded inbound process */
	u_long in_polvio;   /* security policy violation for inbound process */
	u_long in_nosa;     /* inbound SA is unavailable */
	u_long in_inval;    /* inbound processing failed due to EINVAL */
	u_long in_badspi;   /* failed getting a SPI */
	u_long in_ahreplay; /* AH replay check failed */
	u_long in_espreplay; /* ESP replay check failed */
	u_long in_ahauthsucc; /* AH authentication success */
	u_long in_ahauthfail; /* AH authentication failure */
	u_long in_espauthsucc; /* ESP authentication success */
	u_long in_espauthfail; /* ESP authentication failure */
	u_long in_esphist[SADB_EALG_MAX];
	u_long in_ahhist[SADB_AALG_MAX];
	u_long out_success; /* succeeded outbound process */
	u_long out_polvio;  /* security policy violation for outbound process */
	u_long out_nosa;    /* outbound SA is unavailable */
	u_long out_inval;   /* outbound process failed due to EINVAL */
	u_long out_noroute; /* there is no route */
	u_long out_esphist[SADB_EALG_MAX];
	u_long out_ahhist[SADB_AALG_MAX];
};

/*
 * Definitions for IPsec & Key sysctl operations.
 */
/*
 * Names for IPsec & Key sysctl objects
 */
#define IPSECCTL_STATS			1	/* stats */
#define IPSECCTL_DEF_POLICY		2
#define IPSECCTL_DEF_ESP_TRANSLEV	3	/* int; ESP transport mode */
#define IPSECCTL_DEF_ESP_NETLEV		4	/* int; ESP tunnel mode */
#define IPSECCTL_DEF_AH_TRANSLEV	5	/* int; AH transport mode */
#define IPSECCTL_DEF_AH_NETLEV		6	/* int; AH tunnel mode */
#define IPSECCTL_INBOUND_CALL_IKE	7
#define	IPSECCTL_AH_CLEARTOS		8
#define	IPSECCTL_AH_OFFSETMASK		9
#define	IPSECCTL_DFBIT			10
#define	IPSECCTL_ECN			11
#define IPSECCTL_MAXID			12

#define IPSECCTL_NAMES { \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "def_policy", CTLTYPE_INT }, \
	{ "esp_trans_deflev", CTLTYPE_INT }, \
	{ "esp_net_deflev", CTLTYPE_INT }, \
	{ "ah_trans_deflev", CTLTYPE_INT }, \
	{ "ah_net_deflev", CTLTYPE_INT }, \
	{ "inbound_call_ike", CTLTYPE_INT }, \
	{ "ah_cleartos", CTLTYPE_INT }, \
	{ "ah_offsetmask", CTLTYPE_INT }, \
	{ "dfbit", CTLTYPE_INT }, \
	{ "ecn", CTLTYPE_INT }, \
}

#define IPSEC6CTL_NAMES { \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "def_policy", CTLTYPE_INT }, \
	{ "esp_trans_deflev", CTLTYPE_INT }, \
	{ "esp_net_deflev", CTLTYPE_INT }, \
	{ "ah_trans_deflev", CTLTYPE_INT }, \
	{ "ah_net_deflev", CTLTYPE_INT }, \
	{ "inbound_call_ike", CTLTYPE_INT }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "ecn", CTLTYPE_INT }, \
}

#define IPSECCTL_VARS { \
	0, \
	0, \
	&ip4_def_policy.policy, \
	&ip4_esp_trans_deflev, \
	&ip4_esp_net_deflev, \
	&ip4_ah_trans_deflev, \
	&ip4_ah_net_deflev, \
	&ip4_inbound_call_ike, \
	&ip4_ah_cleartos, \
	&ip4_ah_offsetmask, \
	&ip4_ipsec_dfbit, \
	&ip4_ipsec_ecn, \
}

#define IPSEC6CTL_VARS { \
	0, \
	0, \
	&ip6_def_policy.policy, \
	&ip6_esp_trans_deflev, \
	&ip6_esp_net_deflev, \
	&ip6_ah_trans_deflev, \
	&ip6_ah_net_deflev, \
	&ip6_inbound_call_ike, \
	0, \
	0, \
	0, \
	&ip6_ipsec_ecn, \
}

#ifdef KERNEL
struct ipsec_output_state {
	struct mbuf *m;
	struct route *ro;
	struct sockaddr *dst;
};

extern struct ipsecstat ipsecstat;
extern struct secpolicy ip4_def_policy;
extern int ip4_esp_trans_deflev;
extern int ip4_esp_net_deflev;
extern int ip4_ah_trans_deflev;
extern int ip4_ah_net_deflev;
extern int ip4_inbound_call_ike;
extern int ip4_ah_cleartos;
extern int ip4_ah_offsetmask;
extern int ip4_ipsec_dfbit;
extern int ip4_ipsec_ecn;

#ifdef INET6
extern struct ipsecstat ipsec6stat;
extern struct secpolicy ip6_def_policy;
extern int ip6_esp_trans_deflev;
extern int ip6_esp_net_deflev;
extern int ip6_ah_trans_deflev;
extern int ip6_ah_net_deflev;
extern int ip6_inbound_call_ike;
extern int ip6_ipsec_ecn;
#endif

extern struct secpolicy *ipsec4_getpolicybysock
	__P((struct mbuf *, struct socket *, int *));
extern struct secpolicy *ipsec4_getpolicybyaddr
	__P((struct mbuf *, int, int *));

#ifdef INET6
extern struct secpolicy *ipsec6_getpolicybysock
	__P((struct mbuf *, struct socket *, int *));
extern struct secpolicy *ipsec6_getpolicybyaddr
	__P((struct mbuf *, int, int *));
#endif /*INET6*/

struct inpcb;
#ifdef INET6
struct in6pcb;
#endif
extern int ipsec_init_policy __P((struct secpolicy **));
extern struct secpolicy *ipsec_copy_policy __P((struct secpolicy *));
extern int ipsec_set_policy __P((struct secpolicy **, int, caddr_t, int, int));
extern int ipsec_get_policy __P((struct secpolicy *, struct mbuf **));
extern int ipsec4_delete_pcbpolicy __P((struct inpcb *));
#ifdef INET6
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
extern int ipsec6_delete_pcbpolicy __P((struct inpcb *));
#else
extern int ipsec6_delete_pcbpolicy __P((struct in6pcb *));
#endif
#endif
extern u_int ipsec_get_reqlevel __P((struct ipsecrequest *));

extern int ipsec4_in_reject_so __P((struct mbuf *, struct socket *));
extern int ipsec4_in_reject __P((struct mbuf *, struct inpcb *));

#ifdef INET6
extern int ipsec6_in_reject_so __P((struct mbuf *, struct socket *));
extern int ipsec6_in_reject __P((struct mbuf *, struct in6pcb *));
#endif /*INET6*/

struct secas;
struct tcpcb;
struct tcp6cb;
extern int ipsec_chkreplay __P((u_int32_t, struct secas *));
extern int ipsec_updatereplay __P((u_int32_t, struct secas *));

extern size_t ipsec4_hdrsiz __P((struct mbuf *, struct inpcb *));
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
extern size_t ipsec_hdrsiz_tcp __P((struct tcpcb *, int));
#else
extern size_t ipsec4_hdrsiz_tcp __P((struct tcpcb *));
#endif
#ifdef INET6
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
extern size_t ipsec6_hdrsiz __P((struct mbuf *, struct inpcb *));
#else
extern size_t ipsec6_hdrsiz __P((struct mbuf *, struct in6pcb *));
#if !defined(__NetBSD__) || defined(TCP6)
extern size_t ipsec6_hdrsiz_tcp __P((struct tcp6cb *));
#else
extern size_t ipsec6_hdrsiz_tcp __P((struct tcpcb *));
#endif
#endif
#endif

struct ip;
#ifdef INET6
struct ip6_hdr;
#endif
extern const char *ipsec4_logpacketstr __P((struct ip *, u_int32_t));
#ifdef INET6
extern const char *ipsec6_logpacketstr __P((struct ip6_hdr *, u_int32_t));
#endif
extern const char *ipsec_logsastr __P((struct secas *));

extern void ipsec_dumpmbuf __P((struct mbuf *));

extern int ipsec4_output __P((struct ipsec_output_state *, struct secpolicy *,
	int));
#ifdef INET6
extern int ipsec6_output_trans __P((struct ipsec_output_state *, u_char *,
	struct mbuf *, struct secpolicy *, int, int *));
extern int ipsec6_output_tunnel __P((struct ipsec_output_state *,
	struct secpolicy *, int));
#endif
extern int ipsec4_tunnel_validate __P((struct ip *, u_int, struct secas *));
#ifdef INET6
extern int ipsec6_tunnel_validate __P((struct ip6_hdr *, u_int,
	struct secas *));
#endif
extern struct mbuf *ipsec_copypkt __P((struct mbuf *));

#if defined(__bsdi__) || defined(__NetBSD__)
extern int ipsec_sysctl __P((int *, u_int, void *, size_t *, void *, size_t));
extern int ipsec6_sysctl __P((int *, u_int, void *, size_t *, void *, size_t));
#endif /* __bsdi__ || __NetBSD__ */

#endif /*KERNEL*/

#ifndef KERNEL
extern int ipsec_get_policylen(char *reqstr);
extern int ipsec_set_policy(char *buf, int len, char *reqstr);
extern char *ipsec_dump_policy(char *buf, char *delimiter);

extern char *ipsec_strerror(void);
#endif /*!KERNEL*/

#endif /*_NETINET6_IPSEC_H_*/
