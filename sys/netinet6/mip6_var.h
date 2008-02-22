/*	$NetBSD: mip6_var.h,v 1.1.2.1 2008/02/22 02:53:34 keiichi Exp $	*/
/*	$KAME: mip6_var.h,v 1.128 2005/12/30 08:57:24 t-momose Exp $	*/

/*
 * Copyright (C) 2004 WIDE Project.  All rights reserved.
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

#ifndef _NETINET6_MIP6_VAR_H_
#define _NETINET6_MIP6_VAR_H_

/* 
 * The Binding Update List entry used in the kernel. 
 * This entry is stored in in6_ifaddr (in6_var.h) 
 */
struct mip6_bul_internal {
	LIST_ENTRY(mip6_bul_internal) mbul_entry;
	struct in6_addr     mbul_peeraddr;   /* peer address of this BUL */
	struct in6_addr     mbul_hoa;        /* home address */
	struct in6_addr     mbul_coa;        /* care-of address */
	u_int16_t           mbul_flags;      /* flags: Ack, LL, Key, Home */
	struct mip_softc    *mbul_mip;       /* back pointer to mip */
	const struct encaptab *mbul_encap;
	u_int8_t            mbul_state;      /* internal state */
#ifdef MIP6_MCOA
	u_int16_t           mbul_bid;        /* Binding Unique Identifier */
#endif /* MIP6_MCOA */
};
#define MIP6_BUL_STATE_NOT_SUPPORTED 0x01
#define MIP6_BUL_STATE_NEEDTUNNEL (MIP6_BUL_STATE_NOT_SUPPORTED)

/* the binding cache entry used in the kernel */
struct mip6_bc_internal {
	LIST_ENTRY(mip6_bc_internal) mbc_entry;
	struct in6_addr mbc_cnaddr;	/* own address */
	struct in6_addr mbc_hoa;	/* home address */
	struct in6_addr mbc_coa;	/* care-of address */
	struct ifaddr	*mbc_ifaddr;    /* pointer to ifaddr of own address */
	u_int16_t       mbc_flags;      /* flags: Ack, LL, Key, Home */
	int 		mbc_hash_cache; /* hash ID cache for performance optimization */
#ifdef MIP6_MCOA
	u_int16_t       mbc_bid;        /* Binding Unique Identifier */
#endif /* MIP6_MCOA */
};
LIST_HEAD(mip6_bc_list, mip6_bc_internal);

/*
 * Mobile IPv6 related statistics.
 */
struct mip6stat {
	u_quad_t mip6s_mh;		/* Mobility Header recieved */
	u_quad_t mip6s_omh;		/* Mobility Header sent */
	u_quad_t mip6s_hoti;		/* HoTI recieved */
	u_quad_t mip6s_ohoti;		/* HoTI sent */
	u_quad_t mip6s_coti;		/* CoTI received */
	u_quad_t mip6s_ocoti;		/* CoTI sent */
	u_quad_t mip6s_hot;		/* HoT received */
	u_quad_t mip6s_ohot;		/* HoT sent */
	u_quad_t mip6s_cot;		/* CoT received */
	u_quad_t mip6s_ocot;		/* CoT sent */
	u_quad_t mip6s_bu;		/* BU received */
	u_quad_t mip6s_obu;		/* BU sent */
	u_quad_t mip6s_ba;		/* BA received */
	u_quad_t mip6s_ba_hist[256];	/* BA status input histgram */
	u_quad_t mip6s_oba;		/* BA sent */
	u_quad_t mip6s_oba_hist[256];	/* BA status output histgram */
	u_quad_t mip6s_br;		/* BR received */
	u_quad_t mip6s_obr;		/* BR sent */
	u_quad_t mip6s_be;		/* BE received */
	u_quad_t mip6s_be_hist[256];	/* BE status input histogram */
	u_quad_t mip6s_obe;		/* BE sent */
	u_quad_t mip6s_obe_hist[256];	/* BE status output histogram */
	u_quad_t mip6s_hao;		/* HAO received */
	u_quad_t mip6s_unverifiedhao;	/* unverified HAO received */
	u_quad_t mip6s_ohao;		/* HAO sent */
	u_quad_t mip6s_rthdr2;		/* RTHDR2 received */
	u_quad_t mip6s_orthdr2;		/* RTHDR2 sent */
	u_quad_t mip6s_revtunnel;	/* reverse tunnel input */
	u_quad_t mip6s_orevtunnel;	/* reverse tunnel output */
	u_quad_t mip6s_checksum;	/* bad checksum */
	u_quad_t mip6s_payloadproto;	/* payload proto != no nxt header */
	u_quad_t mip6s_unknowntype;	/* unknown MH type value */
	u_quad_t mip6s_nohif;		/* not my home address */
	u_quad_t mip6s_nobue;		/* no related BUE */
	u_quad_t mip6s_hinitcookie;	/* home init cookie mismatch */
	u_quad_t mip6s_cinitcookie;	/* careof init cookie mismatch */
	u_quad_t mip6s_unprotected;	/* not IPseced signaling */
	u_quad_t mip6s_haopolicy;	/* BU is discarded due to bad HAO */
	u_quad_t mip6s_rrauthfail;	/* RR authentication failed */
	u_quad_t mip6s_seqno;		/* seqno mismatch */
	u_quad_t mip6s_paramprobhao;	/* ICMP paramprob for HAO received */
	u_quad_t mip6s_paramprobmh;	/* ICMP paramprob for MH received */
	u_quad_t mip6s_invalidcoa;	/* Invalid Care-of address */
	u_quad_t mip6s_invalidopt;	/* Invalid mobility options */
	u_quad_t mip6s_circularrefered;	/* Circular reference */
};

#ifdef _KERNEL

#define mip6log(arg) do {	\
	if (mip6ctl_debug)	\
	    log arg;		\
} while (/*CONSTCOND*/ 0)

/* macros for a nodetype check. */
#define MIP6_IS_MN ((mip6_nodetype & MIP6_NODETYPE_MOBILE_NODE) || \
	(mip6_nodetype & MIP6_NODETYPE_MOBILE_ROUTER))
#define MIP6_IS_MR ((mip6_nodetype & MIP6_NODETYPE_MOBILE_ROUTER))
#define MIP6_IS_HA ((mip6_nodetype & MIP6_NODETYPE_HOME_AGENT))
#define MIP6_IS_CN ((mip6_nodetype & MIP6_NODETYPE_CORRESPONDENT_NODE))

/* Calculation pad length to be appended */
/* xn + y; x must be 2^m */
#define MIP6_PADLEN(cur_offset, x, y)	\
	((x + y) - ((cur_offset) & (x - 1))) & (x - 1)
#define MIP6_FILL_PADDING(buf, padlen)			\
	do {						\
		bzero((buf), (padlen));			\
	 	if ((padlen) > 1) {			\
			(buf)[0] = IP6OPT_PADN;		\
			(buf)[1] = (padlen) - 2;	\
		}					\
	} while (/*CONSTCOND*/ 0)


/*
 * configuration knobs.  defined in mip6.c.
 */
extern u_int8_t mip6_nodetype;
extern int mip6ctl_debug;
extern int mip6ctl_use_ipsec;
extern int mip6ctl_use_migrate;

/* function prototypes. */
/* correspondent node functions. */
int mip6_bce_update(struct sockaddr_in6 *, struct sockaddr_in6 *,
    struct sockaddr_in6 *, u_int16_t, u_int16_t);
struct mip6_bc_internal *mip6_bce_get(struct in6_addr *, struct in6_addr *,
    struct in6_addr *, u_int16_t);
int mip6_bce_remove_addr(struct sockaddr_in6 *, struct sockaddr_in6 *,
    struct sockaddr_in6 *, u_int16_t, u_int16_t);
int mip6_bce_remove_bc(struct mip6_bc_internal *);
void mip6_bce_remove_all (void);
struct ip6_rthdr2 *mip6_create_rthdr2(struct in6_addr *);

/* home agent functions. */
int mip6_bc_proxy_control(struct in6_addr *, struct in6_addr *, int);
void mip6_do_dad(struct in6_addr *, int);
void mip6_stop_dad(struct in6_addr *, int);
void mip6_do_dad_lladdr(int);

/* mobile node functions. */
int mip6_bul_add(const struct in6_addr *, const struct in6_addr *,
    const struct in6_addr *, u_short, u_int16_t, u_int8_t, u_int16_t);
struct mip6_bul_internal *mip6_bul_get(const struct in6_addr *,
    const struct in6_addr *, u_int16_t);
void mip6_bul_remove(struct mip6_bul_internal *);
void mip6_bul_remove_all(void);
struct mip6_bul_internal *mip6_bul_get_home_agent(const struct in6_addr *);
#ifndef __APPLE__
struct nd_prefixctl;
int mip6_are_homeprefix(struct nd_prefixctl *);
#else
int mip6_are_homeprefix(struct nd_prefix *);
#endif
int mip6_ifa6_is_addr_valid_hoa(struct in6_ifaddr *);
u_int8_t *mip6_create_hoa_opt(struct in6_addr *); 
struct ip6_opt_home_address *mip6_search_hoa_in_destopt(u_int8_t *);
void mip6_probe_routers(void);
int mip6_get_ip6hdrinfo(struct mbuf *, struct in6_addr *, struct in6_addr *, 
		struct in6_addr *, struct in6_addr *, u_int8_t, int *);
#define HOA_PRESENT 	0x01
#define RTHDR_PRESENT 	0x02
void mip6_md_scan(u_int16_t);


/* used by one or more kind of nodetypes. */
struct in6_ifaddr *mip6_ifa_ifwithin6addr(const struct in6_addr *);
int mip6_encapsulate(struct mbuf **, struct in6_addr *, struct in6_addr *);
#ifdef __APPLE__
int mip6_tunnel_input(struct mbuf **, int *);
#else
int mip6_tunnel_input(struct mbuf **, int *, int);
#endif /* __APPLE__ */
void mip6_notify_rr_hint(struct in6_addr *, struct in6_addr *);

/* a sysctl entry. */
#if defined(__NetBSD__) || defined(__OpenBSD__)
int mip6_sysctl(int *, u_int, void *, size_t *, void *, size_t);
#endif /* defined(__NetBSD__) || defined(__OpenBSD__) */

#endif /* _KERNEL */
 
#endif /* !_NETINET6_MIP6_VAR_H_ */

