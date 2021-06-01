/*	$NetBSD: if_spppvar.h,v 1.42 2021/06/01 04:59:50 yamaguchi Exp $	*/

#ifndef _NET_IF_SPPPVAR_H_
#define _NET_IF_SPPPVAR_H_

/*
 * Defines for synchronous PPP/Cisco link level subroutines.
 *
 * Copyright (C) 1994 Cronyx Ltd.
 * Author: Serge Vakulenko, <vak@cronyx.ru>
 *
 * Heavily revamped to conform to RFC 1661.
 * Copyright (C) 1997, Joerg Wunsch.
 *
 * This software is distributed with NO WARRANTIES, not even the implied
 * warranties for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Authors grant any other persons or organizations permission to use
 * or modify this software as long as this message is kept with the software,
 * all derivative works or modified versions.
 *
 * From: Version 2.0, Fri Oct  6 20:39:21 MSK 1995
 *
 * From: if_sppp.h,v 1.8 1997/10/11 11:25:20 joerg Exp
 *
 * From: Id: if_sppp.h,v 1.7 1998/12/01 20:20:19 hm Exp
 */

#include <sys/workqueue.h>
#include <sys/pcq.h>
struct sppp;

struct sppp_work {
	struct work	 work;
	void		*arg;
	void		(*func)(struct sppp *, void *);
	unsigned int	 state;
#define SPPP_WK_FREE	0
#define SPPP_WK_BUSY	1
#define SPPP_WK_UNAVAIL	2
};

#define IDX_LCP 0		/* idx into state table */

struct slcp {
	u_long	opts;		/* LCP options to send (bitfield) */
	u_long  magic;          /* local magic number */
	u_long	mru;		/* our max receive unit */
	u_long	their_mru;	/* their max receive unit */
	u_long	protos;		/* bitmask of protos that are started */
	u_char  echoid;         /* id of last keepalive echo request */
	/* restart max values, see RFC 1661 */
	int	timeout;
	int	max_terminate;
	int	max_configure;
	int	max_failure;
	/* multilink variables */
	u_long	mrru;		/* our   max received reconstructed unit */
	u_long	their_mrru;	/* their max receive dreconstructed unit */
	bool	reestablish;	/* reestablish after the next down event */
	bool	tlf_sent;	/* call lower layer's tlf before a down event */
};

#define IDX_IPCP 1		/* idx into state table */
#define IDX_IPV6CP 2		/* idx into state table */

struct sipcp {
	u_long	opts;		/* IPCP options to send (bitfield) */
	u_int	flags;
#define IPCP_HISADDR_SEEN 1	/* have seen his address already */
#define IPCP_MYADDR_SEEN  2	/* have a local address assigned already */
#define IPCP_MYADDR_DYN   4	/* my address is dynamically assigned */
#define	IPCP_HISADDR_DYN  8	/* his address is dynamically assigned */
#ifdef notdef
#define IPV6CP_MYIFID_DYN   2	/* my ifid is dynamically assigned */
#endif
#define IPV6CP_MYIFID_SEEN  4	/* have seen his ifid already */
	uint32_t saved_hisaddr;/* if hisaddr (IPv4) is dynamic, save original one here, in network byte order */
	uint32_t req_hisaddr;	/* remote address requested */
	uint32_t req_myaddr;	/* local address requested */

	uint8_t my_ifid[8];	/* IPv6CP my ifid*/
	uint8_t his_ifid[8];	/* IPv6CP his ifid*/
};

struct sauth {
	u_short	proto;			/* authentication protocol to use */
	u_short	flags;
	char	*name;			/* system identification name */
	char	*secret;		/* secret password */
	u_char	name_len;		/* no need to have a bigger size */
	u_char	secret_len;		/* because proto gives size in a byte */
};

struct schap {
	char	 challenge[16];		/* random challenge
					   [don't change size! it's really hardcoded!] */
	char	 digest[16];
	u_char	 digest_len;
	bool	 rechallenging;		/* sent challenge after open */
	bool	 response_rcvd;		/* receive response, stop sending challenge */

	struct sppp_work	 work_challenge_rcvd;
};

#define IDX_PAP		3
#define IDX_CHAP	4

#define IDX_COUNT (IDX_CHAP + 1) /* bump this when adding cp's! */

struct sppp_cp {
	u_long		 seq;		/* local sequence number */
	u_long		 rseq;		/* remote sequence number */
	int		 state;		/* state machine */
	u_char		 confid;	/* local id of last configuration request */
	u_char		 rconfid;	/* remote id of last configuration request */
	int		 rst_counter;	/* restart counter */
	int		 fail_counter;	/* negotiation failure counter */
	struct callout	 ch;		/* per-proto and if callouts */
	u_char		 rcr_type;	/* parsing result of conf-req */
	struct mbuf	*mbuf_confreq;	/* received conf-req */
	struct mbuf	*mbuf_confnak;	/* received conf-nak or conf-rej */

	struct sppp_work	 work_up;
	struct sppp_work	 work_down;
	struct sppp_work	 work_open;
	struct sppp_work	 work_close;
	struct sppp_work	 work_to;
	struct sppp_work	 work_rcr;
	struct sppp_work	 work_rca;
	struct sppp_work	 work_rcn;
	struct sppp_work	 work_rtr;
	struct sppp_work	 work_rta;
	struct sppp_work	 work_rxj;
};

struct sppp {
	/* NB: pp_if _must_ be first */
	struct  ifnet pp_if;    /* network interface data */
	struct  ifqueue pp_fastq; /* fast output queue */
	struct	ifqueue pp_cpq;	/* PPP control protocol queue */
	struct  sppp *pp_next;  /* next interface in keepalive list */
	u_int   pp_flags;       /* use Cisco protocol instead of PPP */
	u_int	pp_ncpflags;	/* enable or disable each NCP */
	u_int	pp_framebytes;	/* number of bytes added by (hardware) framing */
	u_int   pp_alivecnt;    /* keepalive packets counter */
	u_int	pp_alive_interval;	/* keepalive interval */
	u_int   pp_loopcnt;     /* loopback detection counter */
	u_int	pp_maxalive;	/* number or echo req. w/o reply */
	uint64_t	pp_saved_mtu;	/* saved MTU value */
	time_t	pp_last_receive;	/* peer's last "sign of life" */
	time_t	pp_max_noreceive;	/* seconds since last receive before
					   we start to worry and send echo
					   requests */
	time_t	pp_last_activity;	/* second of last payload data s/r */
	time_t	pp_idle_timeout;	/* idle seconds before auto-disconnect,
					 * 0 = disabled */
	int	pp_auth_failures;	/* authorization failures */
	int	pp_max_auth_fail;	/* max. allowed authorization failures */
	int	pp_phase;	/* phase we're currently in */
	krwlock_t	pp_lock;	/* lock for sppp structure */
	int	query_dns;	/* 1 if we want to know the dns addresses */
	uint32_t	dns_addrs[2];
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
	struct callout_handle ch[IDX_COUNT]; /* per-proto and if callouts */
	struct callout_handle pap_my_to_ch; /* PAP needs one more... */
#endif
	struct workqueue *wq_cp;
	struct sppp_work work_ifdown;
	struct sppp_cp scp[IDX_COUNT];
	struct slcp lcp;		/* LCP params */
	struct sipcp ipcp;		/* IPCP params */
	struct sipcp ipv6cp;		/* IPv6CP params */
	struct sauth myauth;		/* auth params, i'm peer */
	struct sauth hisauth;		/* auth params, i'm authenticator */
	struct schap chap;		/* CHAP params */
	/*
	 * These functions are filled in by sppp_attach(), and are
	 * expected to be used by the lower layer (hardware) drivers
	 * in order to communicate the (un)availability of the
	 * communication link.  Lower layer drivers that are always
	 * ready to communicate (like hardware HDLC) can shortcut
	 * pp_up from pp_tls, and pp_down from pp_tlf.
	 */
	void	(*pp_up)(struct sppp *);
	void	(*pp_down)(struct sppp *);
	/*
	 * These functions need to be filled in by the lower layer
	 * (hardware) drivers if they request notification from the
	 * PPP layer whether the link is actually required.  They
	 * correspond to the tls and tlf actions.
	 */
	void	(*pp_tls)(struct sppp *);
	void	(*pp_tlf)(struct sppp *);
	/*
	 * These (optional) functions may be filled by the hardware
	 * driver if any notification of established connections
	 * (currently: IPCP up) is desired (pp_con) or any internal
	 * state change of the interface state machine should be
	 * signaled for monitoring purposes (pp_chg).
	 */
	void	(*pp_con)(struct sppp *);
	void	(*pp_chg)(struct sppp *, int);
};

#define PP_KEEPALIVE		0x01	/* use keepalive protocol */
					/* 0x02 was PP_CISCO */
					/* 0x04 was PP_TIMO */
#define PP_CALLIN		0x08	/* we are being called */
#define PP_NEEDAUTH		0x10	/* remote requested authentication */
#define PP_NOFRAMING		0x20	/* do not add/expect encapsulation
					   around PPP frames (i.e. the serial
					   HDLC like encapsulation, RFC1662) */
#define PP_LOOPBACK		0x40	/* in line loopback mode */
#define PP_LOOPBACK_IFDOWN	0x80	/* if_down() when loopback detected */
#define PP_KEEPALIVE_IFDOWN	0x100	/* if_down() when no ECHO_REPLY received */
#define PP_ADMIN_UP		0x200	/* the interface is up */


#define PP_MTU          1500    /* default/minimal MRU */
#define PP_MAX_MRU	2048	/* maximal MRU we want to negotiate */

#ifdef _KERNEL
void sppp_attach (struct ifnet *);
void sppp_detach (struct ifnet *);
void sppp_input (struct ifnet *, struct mbuf *);
int sppp_ioctl(struct ifnet *, u_long, void *);
struct mbuf *sppp_dequeue (struct ifnet *);
int sppp_isempty (struct ifnet *);
void sppp_flush (struct ifnet *);
#endif

/*
 * Locking notes:
 * + spppq is protected by spppq_lock (an adaptive mutex)
 *     spppq is a list of all struct sppps, and it is used for
 *     sending keepalive packets.
 * + struct sppp is protected by sppp->pp_lock (an rwlock)
 *     sppp holds configuration parameters for line,
 *     authentication and addresses. It also has pointers
 *     of functions to notify events to lower layer.
 *     When notify events, sppp->pp_lock must be released.
 *     Because the event handler implemented in a lower
 *     layer often call functions implemented in
 *     if_spppsubr.c.
 *
 * Locking order:
 *    - IFNET_LOCK => spppq_lock => struct sppp->pp_lock
 *
 * NOTICE
 * - Lower layers must not acquire sppp->pp_lock
 */
#endif /* !_NET_IF_SPPPVAR_H_ */
