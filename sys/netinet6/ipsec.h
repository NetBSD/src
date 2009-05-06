/*	$NetBSD: ipsec.h,v 1.51 2009/05/06 21:41:59 elad Exp $	*/
/*	$KAME: ipsec.h,v 1.51 2001/08/05 04:52:58 itojun Exp $	*/

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

#if defined(_KERNEL_OPT)
#include "opt_inet.h"
#endif

#include <net/pfkeyv2.h>
#include <netkey/keydb.h>

#ifdef _KERNEL

/*
 * Security Policy Index
 * NOTE: Ensure to be same address family and upper layer protocol.
 * NOTE: ul_proto, port number, uid, gid:
 *	ANY: reserved for waldcard.
 *	0 to (~0 - 1): is one of the number of each value.
 */
struct secpolicyindex {
	struct sockaddr_storage src;	/* IP src address for SP */
	struct sockaddr_storage dst;	/* IP dst address for SP */
	u_int8_t prefs;			/* prefix length in bits for src */
	u_int8_t prefd;			/* prefix length in bits for dst */
	u_int16_t ul_proto;		/* upper layer Protocol */
#ifdef notyet
	uid_t uids;
	uid_t uidd;
	gid_t gids;
	gid_t gidd;
#endif
};

/* Security Policy Data Base */
struct secpolicy {
	TAILQ_ENTRY(secpolicy) tailq;	/* all SPD entries, both pcb/table */
	LIST_ENTRY(secpolicy) chain;	/* SPD entries on table */

	u_int8_t dir;			/* direction of packet flow */
	int readonly;			/* write prohibited */
	int persist;			/* will never be removed */
	int refcnt;			/* reference count */
	struct secpolicyindex *spidx;	/* selector - NULL if not valid */
	u_int16_t tag;			/* PF tag */
	u_int32_t id;			/* it identifies a policy in the SPD. */
#define IPSEC_MANUAL_POLICYID_MAX	0x3fff
				/*
				 * 1 - 0x3fff are reserved for user operation.
				 * 0 are reserved.  Others are for kernel use.
				 */
	struct socket *so;		/* backpointer to per-socket policy */
	u_int state;			/* 0: dead, others: alive */
#define IPSEC_SPSTATE_DEAD	0
#define IPSEC_SPSTATE_ALIVE	1

	int policy;		/* DISCARD, NONE or IPSEC, see below */
	struct ipsecrequest *req;
				/* pointer to the ipsec request tree, */
				/* if policy == IPSEC else this value == NULL.*/

	/*
	 * lifetime handler.
	 * the policy can be used without limitiation if both lifetime and
	 * validtime are zero.
	 * "lifetime" is passed by sadb_lifetime.sadb_lifetime_addtime.
	 * "validtime" is passed by sadb_lifetime.sadb_lifetime_usetime.
	 */
	time_t created;		/* time created the policy */
	time_t lastused;	/* updated every when kernel sends a packet */
	long lifetime;		/* duration of the lifetime of this policy */
	long validtime;		/* duration this policy is valid without use */
};

/* Request for IPsec */
struct ipsecrequest {
	struct ipsecrequest *next;
				/* pointer to next structure */
				/* If NULL, it means the end of chain. */
	struct secasindex saidx;/* hint for search proper SA */
				/* if __ss_len == 0 then no address specified.*/
	u_int level;		/* IPsec level defined below. */

	struct secasvar *sav;	/* place holder of SA for use */
	struct secpolicy *sp;	/* back pointer to SP */
};

/* security policy in PCB */
struct inpcbpolicy {
	struct secpolicy *sp_in;
	struct secpolicy *sp_out;
	int priv;			/* privileged socket ? */

	/* cached policy */
	struct {
		struct secpolicy *cachesp;
		struct secpolicyindex cacheidx;
		int cachehint;		/* processing requirement hint: */
#define	IPSEC_PCBHINT_MAYBE	0	/* IPsec processing maybe required */
#define	IPSEC_PCBHINT_YES	1	/* IPsec processing is required */
#define	IPSEC_PCBHINT_NO	2	/* IPsec processing not required */
		u_int cachegen;		/* spdgen when cache filled */
	} sp_cache[3];			/* XXX 3 == IPSEC_DIR_MAX */
	int sp_cacheflags;
#define IPSEC_PCBSP_CONNECTED	1
};

#define	IPSEC_PCB_SKIP_IPSEC(inpp, dir)					\
	((inpp)->sp_cache[(dir)].cachehint == IPSEC_PCBHINT_NO &&	\
	 (inpp)->sp_cache[(dir)].cachegen == ipsec_spdgen)

/* SP acquiring list table. */
struct secspacq {
	LIST_ENTRY(secspacq) chain;

	struct secpolicyindex spidx;

	long created;		/* for lifetime */
	int count;		/* for lifetime */
	/* XXX: here is mbuf place holder to be sent ? */
};

struct ipsecaux {
	struct socket *so;
	int hdrs;	/* # of ipsec headers */

	struct secpolicy *sp;
	struct ipsecrequest *req;
};
#endif /* _KERNEL */

/* according to IANA assignment, port 0x0000 and proto 0xff are reserved. */
#define IPSEC_PORT_ANY		0
#define IPSEC_ULPROTO_ANY	255
#define IPSEC_PROTO_ANY		255

/* mode of security protocol */
/* NOTE: DON'T use IPSEC_MODE_ANY at SPD.  It's only use in SAD */
#define	IPSEC_MODE_ANY		0	/* i.e. wildcard. */
#define	IPSEC_MODE_TRANSPORT	1
#define	IPSEC_MODE_TUNNEL	2

/*
 * Direction of security policy.
 * NOTE: Since INVALID is used just as flag.
 * The other are used for loop counter too.
 */
#define IPSEC_DIR_ANY		0
#define IPSEC_DIR_INBOUND	1
#define IPSEC_DIR_OUTBOUND	2
#define IPSEC_DIR_MAX		3
#define IPSEC_DIR_INVALID	4

/* Policy level */
/*
 * IPSEC, ENTRUST and BYPASS are allowed for setsockopt() in PCB,
 * DISCARD, IPSEC and NONE are allowed for setkey() in SPD.
 * DISCARD and NONE are allowed for system default.
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
#define	IPSEC_LEVEL_UNIQUE	3	/* unique SA. */

#define IPSEC_MANUAL_REQID_MAX	0x3fff
				/*
				 * if security policy level == unique, this id
				 * indicate to a relative SA for use, else is
				 * zero.
				 * 1 - 0x3fff are reserved for manual keying.
				 * 0 are reserved for above reason.  Others is
				 * for kernel use.
				 * Note that this id doesn't identify SA
				 * by only itself.
				 */
#define IPSEC_REPLAYWSIZE  32

/*
 * statistics for ipsec processing.
 * Each counter is an unsigned 64-bit value.
 */
#define	IPSEC_STAT_IN_SUCCESS	0	/* succeeded inbound process */
#define	IPSEC_STAT_IN_POLVIO	1	/* security policy violation for
					   inbound process */
#define	IPSEC_STAT_IN_NOSA	2	/* inbound SA is unavailable */
#define	IPSEC_STAT_IN_INVAL	3	/* inbound processing failed EINVAL */
#define	IPSEC_STAT_IN_NOMEM	4	/* inbound processing failed ENOBUFS */
#define	IPSEC_STAT_IN_BADSPI	5	/* failed getting an SPI */
#define	IPSEC_STAT_IN_AHREPLAY	6	/* AH replay check failed */
#define	IPSEC_STAT_IN_ESPREPLAY	7	/* ESP replay check failed */
#define	IPSEC_STAT_IN_AHAUTHSUCC 8	/* AH authentication success */
#define	IPSEC_STAT_IN_AHAUTHFAIL 9	/* AH authentication failure */
#define	IPSEC_STAT_IN_ESPAUTHSUCC 10	/* ESP authentication success */
#define	IPSEC_STAT_IN_ESPAUTHFAIL 11	/* ESP authentication failure */
#define	IPSEC_STAT_IN_ESPHIST	12
		/* space for 256 counters */
#define	IPSEC_STAT_IN_AHHIST	268
		/* space for 256 counters */
#define	IPSEC_STAT_IN_COMPHIST	524
		/* space for 256 counters */
#define	IPSEC_STAT_OUT_SUCCESS	780	/* succeeded outbound process */
#define	IPSEC_STAT_OUT_POLVIO	781	/* security policy violation for
					   outbound process */
#define	IPSEC_STAT_OUT_NOSA	782	/* outbound SA is unavailable */
#define	IPSEC_STAT_OUT_INVAL	783	/* outbound processing failed EINVAL */
#define	IPSEC_STAT_OUT_NOMEM	784	/* outbound processing failed ENOBUFS */
#define	IPSEC_STAT_OUT_NOROUTE	785	/* no route */
#define	IPSEC_STAT_OUT_ESPHIST	786
		/* space for 256 counters */
#define	IPSEC_STAT_OUT_AHHIST	1042
		/* space for 256 counters */
#define	IPSEC_STAT_OUT_COMPHIST	1298
		/* space for 256 counters */
#define	IPSEC_STAT_SPDCACHELOOKUP 1554
#define	IPSEC_STAT_SPDCACHEMISS	1555

#define	IPSEC_NSTATS		1556

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
#if 0	/* obsolete, do not reuse */
#define IPSECCTL_INBOUND_CALL_IKE	7
#endif
#define	IPSECCTL_AH_CLEARTOS		8
#define	IPSECCTL_AH_OFFSETMASK		9
#define	IPSECCTL_DFBIT			10
#define	IPSECCTL_ECN			11
#define	IPSECCTL_DEBUG			12
#define IPSECCTL_MAXID			13

#define IPSECCTL_NAMES { \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "def_policy", CTLTYPE_INT }, \
	{ "esp_trans_deflev", CTLTYPE_INT }, \
	{ "esp_net_deflev", CTLTYPE_INT }, \
	{ "ah_trans_deflev", CTLTYPE_INT }, \
	{ "ah_net_deflev", CTLTYPE_INT }, \
	{ 0, 0 }, \
	{ "ah_cleartos", CTLTYPE_INT }, \
	{ "ah_offsetmask", CTLTYPE_INT }, \
	{ "dfbit", CTLTYPE_INT }, \
	{ "ecn", CTLTYPE_INT }, \
	{ "debug", CTLTYPE_INT }, \
}

#define IPSEC6CTL_NAMES { \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "def_policy", CTLTYPE_INT }, \
	{ "esp_trans_deflev", CTLTYPE_INT }, \
	{ "esp_net_deflev", CTLTYPE_INT }, \
	{ "ah_trans_deflev", CTLTYPE_INT }, \
	{ "ah_net_deflev", CTLTYPE_INT }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "ecn", CTLTYPE_INT }, \
	{ "debug", CTLTYPE_INT }, \
}

#ifdef _KERNEL
struct ipsec_output_state {
	struct mbuf *m;
	struct route *ro;
	const struct sockaddr *dst;
	int encap;
};

struct ipsec_history {
	int ih_proto;
	u_int32_t ih_spi;
};

extern int ipsec_debug;

#ifdef INET
extern struct secpolicy *ip4_def_policy;
extern int ip4_esp_trans_deflev;
extern int ip4_esp_net_deflev;
extern int ip4_ah_trans_deflev;
extern int ip4_ah_net_deflev;
extern int ip4_ah_cleartos;
extern int ip4_ah_offsetmask;
extern int ip4_ipsec_dfbit;
extern int ip4_ipsec_ecn;
#endif

#ifdef INET6
extern struct secpolicy *ip6_def_policy;
extern int ip6_esp_trans_deflev;
extern int ip6_esp_net_deflev;
extern int ip6_ah_trans_deflev;
extern int ip6_ah_net_deflev;
extern int ip6_ipsec_ecn;
#endif

#define ipseclog(x)	do { if (ipsec_debug) log x; } while (/*CONSTCOND*/ 0)

extern int ipsec_pcbconn(struct inpcbpolicy *);
extern int ipsec_pcbdisconn(struct inpcbpolicy *);
extern void ipsec_invalpcbcacheall(void);

extern u_int ipsec_spdgen;

extern struct secpolicy *ipsec4_getpolicybysock
(struct mbuf *, u_int, struct socket *, int *);
extern struct secpolicy *ipsec4_getpolicybyaddr
(struct mbuf *, u_int, int, int *);

#ifdef INET6
extern struct secpolicy *ipsec6_getpolicybysock
(struct mbuf *, u_int, struct socket *, int *);
extern struct secpolicy *ipsec6_getpolicybyaddr
(struct mbuf *, u_int, int, int *);
#endif /* INET6 */

struct inpcb;
#ifdef INET6
struct in6pcb;
#endif
extern int ipsec_init_pcbpolicy(struct socket *, struct inpcbpolicy **);
extern int ipsec_copy_pcbpolicy
(struct inpcbpolicy *, struct inpcbpolicy *);
extern u_int ipsec_get_reqlevel(struct ipsecrequest *, int);

extern int ipsec4_set_policy(struct inpcb *, int, void *, size_t, kauth_cred_t);
extern int ipsec4_get_policy(struct inpcb *, void *, size_t,
	    struct mbuf **);
extern int ipsec4_delete_pcbpolicy(struct inpcb *);
extern int ipsec4_in_reject_so(struct mbuf *, struct socket *);
extern int ipsec4_in_reject(struct mbuf *, struct inpcb *);

#ifdef INET6
extern int ipsec6_in_reject_so(struct mbuf *, struct socket *);
extern int ipsec6_delete_pcbpolicy(struct in6pcb *);
extern int ipsec6_set_policy(struct in6pcb *, int, void *, size_t,
    kauth_cred_t);
extern int ipsec6_get_policy(struct in6pcb *, void *, size_t,
	    struct mbuf **);
extern int ipsec6_in_reject(struct mbuf *, struct in6pcb *);
#endif /* INET6 */

struct secas;
struct tcpcb;
struct tcp6cb;
extern int ipsec_chkreplay(u_int32_t, struct secasvar *);
extern int ipsec_updatereplay(u_int32_t, struct secasvar *);

extern void ipsec4_init(void);
extern size_t ipsec4_hdrsiz(struct mbuf *, u_int, struct inpcb *);
extern size_t ipsec4_hdrsiz_tcp(struct tcpcb *);
#ifdef INET6
extern void ipsec6_init(void);
extern size_t ipsec6_hdrsiz(struct mbuf *, u_int, struct in6pcb *);
extern size_t ipsec6_hdrsiz_tcp(struct tcpcb *);
#endif

struct ip;
#ifdef INET6
struct ip6_hdr;
#endif
extern const char *ipsec4_logpacketstr(struct ip *, u_int32_t);
#ifdef INET6
extern const char *ipsec6_logpacketstr(struct ip6_hdr *, u_int32_t);
#endif
extern const char *ipsec_logsastr(struct secasvar *);

extern void ipsec_dumpmbuf(struct mbuf *);

extern int ipsec4_output(struct ipsec_output_state *, struct secpolicy *,
	int);
#ifdef INET6
extern int ipsec6_output_trans(struct ipsec_output_state *, u_char *,
	struct mbuf *, struct secpolicy *, int, int *);
extern int ipsec6_output_tunnel(struct ipsec_output_state *,
	struct secpolicy *, int);
#endif
extern int ipsec4_tunnel_validate(struct ip *, u_int, struct secasvar *);
#ifdef INET6
extern int ipsec6_tunnel_validate(struct ip6_hdr *, u_int,
	struct secasvar *);
#endif
extern struct mbuf *ipsec_copypkt(struct mbuf *);
extern void ipsec_delaux(struct mbuf *);
extern int ipsec_addhist(struct mbuf *, int, u_int32_t);
extern int ipsec_getnhist(struct mbuf *);
extern struct ipsec_history *ipsec_gethist(struct mbuf *, int *);
extern void ipsec_clearhist(struct mbuf *);

extern int ipsec_sysctl(int *, u_int, void *, size_t *, void *, size_t);
extern int ipsec6_sysctl(int *, u_int, void *, size_t *, void *, size_t);

#endif /* _KERNEL */

#ifndef _KERNEL
typedef void *ipsec_policy_t;
extern ipsec_policy_t ipsec_set_policy(const char *, int);
extern int ipsec_get_policylen(ipsec_policy_t);
extern char *ipsec_dump_policy(ipsec_policy_t, const char *);

extern const char *ipsec_strerror(void);
#endif /* !_KERNEL */

#endif /* !_NETINET6_IPSEC_H_ */
