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
 * From: if_sppp.h,v 1.14 1999/03/30 13:28:26 phk Exp
 *
 * $Id: i4b_isppp.h,v 1.3.106.1 2015/09/22 12:06:12 skrll Exp $
 */

#ifndef _NETISDN_I4B_ISPPP_H_
#define _NETISDN_I4B_ISPPP_H_

#define SPPP_VJ			/* use VJ compression */

#include <sys/ioccom.h>


#ifdef SPPP_VJ
#if !(defined (KERNEL) || defined (_KERNEL))
#ifdef __FreeBSD__
#if 0
/*
 * this is needed on FreeBSD to make /usr/src/usr.bin/kdump and
 * /usr/src/usr.bin/truss compile.
 */
#include <sys/mbuf.h>
#endif
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <net/slcompress.h>
#endif
#endif
#endif

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
};

#define IDX_IPCP 1		/* idx into state table */

struct sipcp {
	u_long	opts;		/* IPCP options to send (bitfield) */
	u_int	flags;
#define IPCP_HISADDR_SEEN 1	/* have seen his address already */
#define IPCP_MYADDR_DYN   2	/* my address is dynamically assigned */
#define IPCP_MYADDR_SEEN  4	/* have seen his address already */
#define IPCP_VJ		  8	/* We can use VJ compression */
	int	max_state;	/* Max-Slot-Id */
	int	compress_cid;	/* Comp-Slot-Id */
};

#define AUTHNAMELEN	32
#define AUTHKEYLEN	16

struct sauth {
	u_short	proto;			/* authentication protocol to use */
	u_short	flags;
#define AUTHFLAG_NOCALLOUT	1	/* do not require authentication on */
					/* callouts */
#define AUTHFLAG_NORECHALLENGE	2	/* do not re-challenge CHAP */
	u_char	name[AUTHNAMELEN];	/* system identification name */
	u_char	secret[AUTHKEYLEN];	/* secret password */
	u_char	challenge[AUTHKEYLEN];	/* random challenge */
};

#define IDX_PAP		2
#define IDX_CHAP	3

#define IDX_COUNT (IDX_CHAP + 1) /* bump this when adding cp's! */

/*
 * Don't change the order of this.  Ordering the phases this way allows
 * for a comparision of ``pp_phase >= PHASE_AUTHENTICATE'' in order to
 * know whether LCP is up.
 */
enum ppp_phase {
	PHASE_DEAD, PHASE_ESTABLISH, PHASE_TERMINATE,
	PHASE_AUTHENTICATE, PHASE_NETWORK
};

struct sppp {
	/* NB: pp_if _must_ be first */
	struct  ifnet pp_if;    /* network interface data */
	struct  ifqueue pp_fastq; /* fast output queue */
	struct	ifqueue pp_cpq;	/* PPP control protocol queue */
	struct  sppp *pp_next;  /* next interface in keepalive list */
	u_int   pp_mode;        /* major protocol modes (cisco/ppp/...) */
	u_int   pp_flags;       /* sub modes */
	u_short pp_alivecnt;    /* keepalive packets counter */
	u_short pp_loopcnt;     /* loopback detection counter */
	u_long  pp_seq;         /* local sequence number */
	u_long  pp_rseq;        /* remote sequence number */
	time_t  pp_last_sent;
	time_t  pp_last_recv;
	enum ppp_phase pp_phase;	/* phase we're currently in */
	int	state[IDX_COUNT];	/* state machine */
	u_char  confid[IDX_COUNT];	/* id of last configuration request */
	int	rst_counter[IDX_COUNT];	/* restart counter */
	int	fail_counter[IDX_COUNT]; /* negotiation failure counter */
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
	struct callout_handle ch[IDX_COUNT]; /* per-proto and if callouts */
	struct callout_handle pap_my_to_ch; /* PAP needs one more... */
#endif
	struct slcp lcp;		/* LCP params */
	struct sipcp ipcp;		/* IPCP params */
	struct sauth myauth;		/* auth params, i'm peer */
	struct sauth hisauth;		/* auth params, i'm authenticator */
#ifdef SPPP_VJ
	int enable_vj;			/* enable VJ negotiation */
	struct slcompress pp_comp;	/* for VJ compression */
#endif
	/*
	 * These functions are filled in by sppp_attach(), and are
	 * expected to be used by the lower layer (hardware) drivers
	 * in order to communicate the (un)availability of the
	 * communication link.  Lower layer drivers that are always
	 * ready to communicate (like hardware HDLC) can shortcut
	 * pp_up from pp_tls, and pp_down from pp_tlf.
	 */
	void	(*pp_up)(struct sppp *sp);
	void	(*pp_down)(struct sppp *sp);
	/*
	 * These functions need to be filled in by the lower layer
	 * (hardware) drivers if they request notification from the
	 * PPP layer whether the link is actually required.  They
	 * correspond to the tls and tlf actions.
	 */
	void	(*pp_tls)(struct sppp *sp);
	void	(*pp_tlf)(struct sppp *sp);
	/*
	 * These (optional) functions may be filled by the hardware
	 * driver if any notification of established connections
	 * (currently: IPCP up) is desired (pp_con) or any internal
	 * state change of the interface state machine should be
	 * signaled for monitoring purposes (pp_chg).
	 */
	void	(*pp_con)(struct sppp *sp);
	void	(*pp_chg)(struct sppp *sp, int new_state);
	/* These two fields are for use by the lower layer */
	void    *pp_lowerp;
	int     pp_loweri;
};

#define PP_KEEPALIVE    0x01    /* use keepalive protocol */
#define PP_CALLIN	0x08	/* we are being called */
#define PP_NEEDAUTH	0x10	/* remote requested authentication */


#define PP_MTU          1500    /* default/minimal MRU */
#define PP_MAX_MRU	2048	/* maximal MRU we want to negotiate */

/*
 * Definitions to pass struct sppp data down into the kernel using the
 * SIOC[SG]IFGENERIC ioctl interface.
 *
 * In order to use this, create a struct spppreq, fill in the cmd
 * field with SPPPIOGDEFS, and put the address of this structure into
 * the ifr_data portion of a struct ifreq.  Pass this struct to a
 * SIOCGIFGENERIC ioctl.  Then replace the cmd field by SPPPIOCDEFS,
 * modify the defs field as desired, and pass the struct ifreq now
 * to a SIOCSIFGENERIC ioctl.
 */

#define SPPPIOGDEFS  ((void *)(('S' << 24) + (1 << 16) + sizeof(struct sppp)))
#define SPPPIOSDEFS  ((void *)(('S' << 24) + (2 << 16) + sizeof(struct sppp)))

struct spppreq {
	u_long	cmd;
	struct sppp defs;
};

#ifndef SIOCSIFGENERIC
#define	SIOCSIFGENERIC	 _IOW('i', 57, struct ifreq)	/* generic IF set op */
#endif

#ifndef SIOCGIFGENERIC
#define	SIOCGIFGENERIC	_IOWR('i', 58, struct ifreq)	/* generic IF get op */
#endif

#if defined(KERNEL) || defined(_KERNEL)

#ifndef USE_ISPPP

void sppp_attach (struct ifnet *ifp);
void sppp_detach (struct ifnet *ifp);
void sppp_input (struct ifnet *ifp, struct mbuf *m);

#if defined(__FreeBSD_version) && __FreeBSD_version >= 300003
int sppp_ioctl (struct ifnet *ifp, u_long cmd, void *data);
#else
#ifdef __FreeBSD__
int sppp_ioctl (struct ifnet *ifp, int cmd, void *data);
#else
int sppp_ioctl (struct ifnet *ifp, u_long cmd, void *data);
#endif
#endif

struct mbuf *sppp_dequeue (struct ifnet *ifp);
struct mbuf *sppp_pick(struct ifnet *ifp);
int sppp_isempty (struct ifnet *ifp);
void sppp_flush (struct ifnet *ifp);

#else /* USE_ISPPP */

void isppp_attach (struct ifnet *ifp);
void isppp_detach (struct ifnet *ifp);
void isppp_input (struct ifnet *ifp, struct mbuf *m);

#if defined(__FreeBSD_version) && __FreeBSD_version >= 300003
int isppp_ioctl (struct ifnet *ifp, u_long cmd, void *data);
#else
#ifdef __FreeBSD__
int isppp_ioctl (struct ifnet *ifp, int cmd, void *data);
#else
int isppp_ioctl (struct ifnet *ifp, u_long cmd, void *data);
#endif
#endif

struct mbuf *isppp_dequeue (struct ifnet *ifp);
struct mbuf *isppp_pick(struct ifnet *ifp);
int isppp_isempty (struct ifnet *ifp);
void isppp_flush (struct ifnet *ifp);
#endif /* USE_ISPPP */

#endif /* KERNEL */

#endif /* !_NETISDN_I4B_ISPPP_H_ */
