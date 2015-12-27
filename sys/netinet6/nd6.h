/*	$NetBSD: nd6.h,v 1.60.2.3 2015/12/27 12:10:07 skrll Exp $	*/
/*	$KAME: nd6.h,v 1.95 2002/06/08 11:31:06 itojun Exp $	*/

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

#ifndef _NETINET6_ND6_H_
#define _NETINET6_ND6_H_

#include <sys/queue.h>
#include <sys/callout.h>

#define ND6_LLINFO_PURGE	-3
#define ND6_LLINFO_NOSTATE	-2
/*
 * We don't need the WAITDELETE state any more, but we keep the definition
 * in a comment line instead of removing it. This is necessary to avoid
 * unintentionally reusing the value for another purpose, which might
 * affect backward compatibility with old applications.
 * (20000711 jinmei@kame.net)
 */
/* #define ND6_LLINFO_WAITDELETE	-1 */
#define ND6_LLINFO_INCOMPLETE	0
#define ND6_LLINFO_REACHABLE	1
#define ND6_LLINFO_STALE	2
#define ND6_LLINFO_DELAY	3
#define ND6_LLINFO_PROBE	4

#define ND6_IS_LLINFO_PROBREACH(n) ((n)->ln_state > ND6_LLINFO_INCOMPLETE)
#define ND6_LLINFO_PERMANENT(n)	(((n)->ln_expire == 0) && ((n)->ln_state > ND6_LLINFO_INCOMPLETE))

struct nd_ifinfo {
	u_int32_t linkmtu;		/* LinkMTU */
	u_int32_t maxmtu;		/* Upper bound of LinkMTU */
	u_int32_t basereachable;	/* BaseReachableTime */
	u_int32_t reachable;		/* Reachable Time */
	u_int32_t retrans;		/* Retrans Timer */
	u_int32_t flags;		/* Flags */
	int recalctm;			/* BaseReacable re-calculation timer */
	u_int8_t chlim;			/* CurHopLimit */
	u_int8_t initialized; /* Flag to see the entry is initialized */
	/* the following 3 members are for privacy extension for addrconf */
	u_int8_t randomseed0[8]; /* upper 64 bits of MD5 digest */
	u_int8_t randomseed1[8]; /* lower 64 bits (usually the EUI64 IFID) */
	u_int8_t randomid[8];	/* current random ID */
};

#define ND6_IFF_PERFORMNUD	0x01
#define ND6_IFF_ACCEPT_RTADV	0x02	/* See "RTADV Key", below. */
#define ND6_IFF_PREFER_SOURCE	0x04	/* XXX: not related to ND. */
#define ND6_IFF_IFDISABLED	0x08	/* IPv6 operation is disabled due to
					 * DAD failure.  (XXX: not ND-specific)
					 */
#define	ND6_IFF_OVERRIDE_RTADV	0x10	/* See "RTADV Key", below. */
#define	ND6_IFF_AUTO_LINKLOCAL	0x20

/*
 * RTADV Key
 *
 * The flags ND6_IFF_ACCEPT_RTADV and ND6_IFF_OVERRIDE_RTADV form a
 * tri-state variable.  (There are actually four different states, but
 * two of the states are functionally identical.)
 *
 * ND6_IFF_OVERRIDE_RTADV or 0:	This interface does not accept
 *				Router Advertisements.
 *
 * ND6_IFF_OVERRIDE_RTADV|
 * ND6_IFF_ACCEPT_RTADV:	This interface accepts Router
 *				Advertisements regardless of the
 *				global setting, ip6_accept_rtadv.
 *
 * ND6_IFF_ACCEPT_RTADV:	This interface follows the global setting,
 *				ip6_accept_rtadv.  If ip6_accept_rtadv == 0,
 *				this interface does not accept Router
 *				Advertisements.  If ip6_accept_rtadv != 0,
 *				this interface does accept them.
 */

#ifdef _KERNEL
#define ND_IFINFO(ifp) \
	(((struct in6_ifextra *)(ifp)->if_afdata[AF_INET6])->nd_ifinfo)
#define IN6_LINKMTU(ifp) \
	((ND_IFINFO(ifp)->linkmtu && ND_IFINFO(ifp)->linkmtu < (ifp)->if_mtu) \
	    ? ND_IFINFO(ifp)->linkmtu \
	    : ((ND_IFINFO(ifp)->maxmtu && ND_IFINFO(ifp)->maxmtu < (ifp)->if_mtu) \
		? ND_IFINFO(ifp)->maxmtu : (ifp)->if_mtu))
#endif

struct in6_nbrinfo {
	char ifname[IFNAMSIZ];	/* if name, e.g. "en0" */
	struct in6_addr addr;	/* IPv6 address of the neighbor */
	long	asked;		/* number of queries already sent for this addr */
	int	isrouter;	/* if it acts as a router */
	int	state;		/* reachability state */
	int	expire;		/* lifetime for NDP state transition */
};

#define DRLSTSIZ 10
#define PRLSTSIZ 10
struct	in6_drlist {
	char ifname[IFNAMSIZ];
	struct {
		struct	in6_addr rtaddr;
		u_char	flags;
		u_short	rtlifetime;
		u_long	expire;
		u_short if_index;
	} defrouter[DRLSTSIZ];
};

struct	in6_defrouter {
	struct	sockaddr_in6 rtaddr;
	u_char	flags;
	u_short	rtlifetime;
	u_long	expire;
	u_short if_index;
};

#ifdef _KERNEL
struct	in6_oprlist {
	char ifname[IFNAMSIZ];
	struct {
		struct	in6_addr prefix;
		struct prf_ra raflags;
		u_char	prefixlen;
		u_char	origin;
		u_long vltime;
		u_long pltime;
		u_long expire;
		u_short if_index;
		u_short advrtrs; /* number of advertisement routers */
		struct	in6_addr advrtr[DRLSTSIZ]; /* XXX: explicit limit */
	} prefix[PRLSTSIZ];
};
#endif

struct	in6_prlist {
	char ifname[IFNAMSIZ];
	struct {
		struct	in6_addr prefix;
		struct prf_ra raflags;
		u_char	prefixlen;
		u_char	origin;
		u_int32_t vltime;
		u_int32_t pltime;
		time_t expire;
		u_short if_index;
		u_short advrtrs; /* number of advertisement routers */
		struct	in6_addr advrtr[DRLSTSIZ]; /* XXX: explicit limit */
	} prefix[PRLSTSIZ];
};

struct in6_prefix {
	struct	sockaddr_in6 prefix;
	struct prf_ra raflags;
	u_char	prefixlen;
	u_char	origin;
	u_int32_t vltime;
	u_int32_t pltime;
	time_t expire;
	u_int32_t flags;
	int refcnt;
	u_short if_index;
	u_short advrtrs; /* number of advertisement routers */
	/* struct sockaddr_in6 advrtr[] */
};

#ifdef _KERNEL
struct	in6_ondireq {
	char ifname[IFNAMSIZ];
	struct {
		u_int32_t linkmtu;	/* LinkMTU */
		u_int32_t maxmtu;	/* Upper bound of LinkMTU */
		u_int32_t basereachable; /* BaseReachableTime */
		u_int32_t reachable;	/* Reachable Time */
		u_int32_t retrans;	/* Retrans Timer */
		u_int32_t flags;	/* Flags */
		int recalctm;		/* BaseReacable re-calculation timer */
		u_int8_t chlim;		/* CurHopLimit */
		u_int8_t receivedra;
	} ndi;
};
#endif

struct	in6_ndireq {
	char ifname[IFNAMSIZ];
	struct nd_ifinfo ndi;
};

struct	in6_ndifreq {
	char ifname[IFNAMSIZ];
	u_long ifindex;
};

/* Prefix status */
#define NDPRF_ONLINK		0x1
#define NDPRF_DETACHED		0x2
#define NDPRF_HOME		0x4

/* protocol constants */
#define MAX_RTR_SOLICITATION_DELAY	1	/* 1sec */
#define RTR_SOLICITATION_INTERVAL	4	/* 4sec */
#define MAX_RTR_SOLICITATIONS		3

#define ND6_INFINITE_LIFETIME		((u_int32_t)~0)

#ifdef _KERNEL
/* node constants */
#define MAX_REACHABLE_TIME		3600000	/* msec */
#define REACHABLE_TIME			30000	/* msec */
#define RETRANS_TIMER			1000	/* msec */
#define MIN_RANDOM_FACTOR		512	/* 1024 * 0.5 */
#define MAX_RANDOM_FACTOR		1536	/* 1024 * 1.5 */
#define DEF_TEMP_VALID_LIFETIME		604800	/* 1 week */
#define DEF_TEMP_PREFERRED_LIFETIME	86400	/* 1 day */
#define TEMPADDR_REGEN_ADVANCE		5	/* sec */
#define MAX_TEMP_DESYNC_FACTOR		600	/* 10 min */
#define ND_COMPUTE_RTIME(x) \
		(((MIN_RANDOM_FACTOR * (x >> 10)) + (cprng_fast32() & \
		((MAX_RANDOM_FACTOR - MIN_RANDOM_FACTOR) * (x >> 10)))) /1000)

TAILQ_HEAD(nd_drhead, nd_defrouter);
struct	nd_defrouter {
	TAILQ_ENTRY(nd_defrouter) dr_entry;
	struct	in6_addr rtaddr;
	u_char	flags;		/* flags on RA message */
	u_short	rtlifetime;
	u_long	expire;
	struct  ifnet *ifp;
	int	installed;	/* is installed into kernel routing table */
};

struct nd_prefixctl {
	struct ifnet *ndprc_ifp;

	/* prefix */
	struct sockaddr_in6 ndprc_prefix;
	u_char	ndprc_plen;

	u_int32_t ndprc_vltime;	/* advertised valid lifetime */
	u_int32_t ndprc_pltime;	/* advertised preferred lifetime */

	struct prf_ra ndprc_flags;
};

#define ndprc_raf		ndprc_flags
#define ndprc_raf_onlink	ndprc_flags.onlink
#define ndprc_raf_auto		ndprc_flags.autonomous
#define ndprc_raf_router	ndprc_flags.router

struct nd_prefix {
	struct ifnet *ndpr_ifp;
	LIST_ENTRY(nd_prefix) ndpr_entry;
	struct sockaddr_in6 ndpr_prefix;	/* prefix */
	struct in6_addr ndpr_mask; /* netmask derived from the prefix */

	u_int32_t ndpr_vltime;	/* advertised valid lifetime */
	u_int32_t ndpr_pltime;	/* advertised preferred lifetime */

	time_t ndpr_expire;	/* expiration time of the prefix */
	time_t ndpr_preferred;	/* preferred time of the prefix */
	time_t ndpr_lastupdate; /* reception time of last advertisement */

	struct prf_ra ndpr_flags;
	u_int32_t ndpr_stateflags; /* actual state flags */
	/* list of routers that advertise the prefix: */
	LIST_HEAD(pr_rtrhead, nd_pfxrouter) ndpr_advrtrs;
	u_char	ndpr_plen;
	int	ndpr_refcnt;	/* reference couter from addresses */
};

#define ndpr_next		ndpr_entry.le_next

#define ndpr_raf		ndpr_flags
#define ndpr_raf_onlink		ndpr_flags.onlink
#define ndpr_raf_auto		ndpr_flags.autonomous
#define ndpr_raf_router		ndpr_flags.router

/*
 * Message format for use in obtaining information about prefixes
 * from inet6 sysctl function
 */
struct inet6_ndpr_msghdr {
	u_short	inpm_msglen;	/* to skip over non-understood messages */
	u_char	inpm_version;	/* future binary compatibility */
	u_char	inpm_type;	/* message type */
	struct in6_addr inpm_prefix;
	u_long	prm_vltim;
	u_long	prm_pltime;
	u_long	prm_expire;
	u_long	prm_preferred;
	struct in6_prflags prm_flags;
	u_short	prm_index;	/* index for associated ifp */
	u_char	prm_plen;	/* length of prefix in bits */
};

#define prm_raf_onlink		prm_flags.prf_ra.onlink
#define prm_raf_auto		prm_flags.prf_ra.autonomous

#define prm_statef_onlink	prm_flags.prf_state.onlink

#define prm_rrf_decrvalid	prm_flags.prf_rr.decrvalid
#define prm_rrf_decrprefd	prm_flags.prf_rr.decrprefd

struct nd_pfxrouter {
	LIST_ENTRY(nd_pfxrouter) pfr_entry;
	struct nd_defrouter *router;
};

LIST_HEAD(nd_prhead, nd_prefix);

#include <sys/mallocvar.h>
MALLOC_DECLARE(M_IP6NDP);

/* nd6.c */
extern int nd6_prune;
extern int nd6_delay;
extern int nd6_umaxtries;
extern int nd6_mmaxtries;
extern int nd6_useloopback;
extern int nd6_maxnudhint;
extern int nd6_gctimer;
extern struct nd_drhead nd_defrouter;
extern struct nd_prhead nd_prefix;
extern int nd6_debug;

#define nd6log(x)	do { if (nd6_debug) log x; } while (/*CONSTCOND*/ 0)

extern struct callout nd6_timer_ch;

/* nd6_rtr.c */
extern int nd6_defifindex;
extern int ip6_desync_factor;	/* seconds */
extern u_int32_t ip6_temp_preferred_lifetime; /* seconds */
extern u_int32_t ip6_temp_valid_lifetime; /* seconds */
extern int ip6_temp_regen_advance; /* seconds */
extern int nd6_numroutes;

union nd_opts {
	struct nd_opt_hdr *nd_opt_array[8];
	struct {
		struct nd_opt_hdr *zero;
		struct nd_opt_hdr *src_lladdr;
		struct nd_opt_hdr *tgt_lladdr;
		struct nd_opt_prefix_info *pi_beg; /* multiple opts, start */
		struct nd_opt_rd_hdr *rh;
		struct nd_opt_mtu *mtu;
		struct nd_opt_hdr *search;	/* multiple opts */
		struct nd_opt_hdr *last;	/* multiple opts */
		int done;
		struct nd_opt_prefix_info *pi_end;/* multiple opts, end */
	} nd_opt_each;
};
#define nd_opts_src_lladdr	nd_opt_each.src_lladdr
#define nd_opts_tgt_lladdr	nd_opt_each.tgt_lladdr
#define nd_opts_pi		nd_opt_each.pi_beg
#define nd_opts_pi_end		nd_opt_each.pi_end
#define nd_opts_rh		nd_opt_each.rh
#define nd_opts_mtu		nd_opt_each.mtu
#define nd_opts_search		nd_opt_each.search
#define nd_opts_last		nd_opt_each.last
#define nd_opts_done		nd_opt_each.done

#include <net/if_llatbl.h>

/* XXX: need nd6_var.h?? */
/* nd6.c */
void nd6_init(void);
struct nd_ifinfo *nd6_ifattach(struct ifnet *);
void nd6_ifdetach(struct ifnet *, struct in6_ifextra *);
int nd6_is_addr_neighbor(const struct sockaddr_in6 *, struct ifnet *);
void nd6_option_init(void *, int, union nd_opts *);
struct nd_opt_hdr *nd6_option(union nd_opts *);
int nd6_options(union nd_opts *);
struct	rtentry *nd6_lookup(const struct in6_addr *, int, struct ifnet *);
void nd6_setmtu(struct ifnet *);
void nd6_llinfo_settimer(struct llentry *, time_t);
void nd6_llinfo_settimer_locked(struct llentry *, time_t);
void nd6_timer(void *);
void nd6_purge(struct ifnet *, struct in6_ifextra *);
void nd6_nud_hint(struct rtentry *);
int nd6_resolve(struct ifnet *, struct rtentry *,
	struct mbuf *, struct sockaddr *, u_char *);
void nd6_rtrequest(int, struct rtentry *, const struct rt_addrinfo *);
int nd6_ioctl(u_long, void *, struct ifnet *);
void nd6_cache_lladdr(struct ifnet *, struct in6_addr *,
	char *, int, int, int);
int nd6_output(struct ifnet *, struct ifnet *, struct mbuf *,
	const struct sockaddr_in6 *, struct rtentry *);
int nd6_storelladdr(const struct ifnet *, const struct rtentry *, struct mbuf *,
	const struct sockaddr *, uint8_t *, size_t);
int nd6_sysctl(int, void *, size_t *, void *, size_t);
int nd6_need_cache(struct ifnet *);
void nd6_llinfo_release_pkts(struct llentry *, struct ifnet *,
    struct rtentry *);

/* nd6_nbr.c */
void nd6_na_input(struct mbuf *, int, int);
void nd6_na_output(struct ifnet *, const struct in6_addr *,
	const struct in6_addr *, u_long, int, const struct sockaddr *);
void nd6_ns_input(struct mbuf *, int, int);
void nd6_ns_output(struct ifnet *, const struct in6_addr *,
	const struct in6_addr *, struct in6_addr *, int);
const void *nd6_ifptomac(const struct ifnet *);
void nd6_dad_start(struct ifaddr *, int);
void nd6_dad_stop(struct ifaddr *);
void nd6_dad_duplicated(struct ifaddr *);

/* nd6_rtr.c */
void nd6_rs_input(struct mbuf *, int, int);
void nd6_ra_input(struct mbuf *, int, int);
void prelist_del(struct nd_prefix *);
void defrouter_addreq(struct nd_defrouter *);
void defrouter_reset(void);
void defrouter_select(void);
void defrtrlist_del(struct nd_defrouter *, struct in6_ifextra *);
void prelist_remove(struct nd_prefix *);
int nd6_prelist_add(struct nd_prefixctl *, struct nd_defrouter *,
	struct nd_prefix **);
int nd6_prefix_onlink(struct nd_prefix *);
int nd6_prefix_offlink(struct nd_prefix *);
void pfxlist_onlink_check(void);
struct nd_defrouter *defrouter_lookup(const struct in6_addr *, struct ifnet *);
struct nd_prefix *nd6_prefix_lookup(struct nd_prefixctl *);
int in6_ifdel(struct ifnet *, struct in6_addr *);
void rt6_flush(struct in6_addr *, struct ifnet *);
int nd6_setdefaultiface(int);
int in6_tmpifadd(const struct in6_ifaddr *, int, int);
bool nd6_accepts_rtadv(const struct nd_ifinfo *);

#endif /* _KERNEL */

#endif /* !_NETINET6_ND6_H_ */
