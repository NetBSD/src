/*	$NetBSD: if_spppsubr.c,v 1.262 2022/03/07 04:06:20 knakahara Exp $	 */

/*
 * Synchronous PPP/Cisco link level subroutines.
 * Keepalive protocol implemented in both Cisco and PPP modes.
 *
 * Copyright (C) 1994-1996 Cronyx Engineering Ltd.
 * Author: Serge Vakulenko, <vak@cronyx.ru>
 *
 * Heavily revamped to conform to RFC 1661.
 * Copyright (C) 1997, Joerg Wunsch.
 *
 * RFC2472 IPv6CP support.
 * Copyright (C) 2000, Jun-ichiro itojun Hagino <itojun@iijlab.net>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE FREEBSD PROJECT ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE FREEBSD PROJECT OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * From: Version 2.4, Thu Apr 30 17:17:21 MSD 1997
 *
 * From: if_spppsubr.c,v 1.39 1998/04/04 13:26:03 phk Exp
 *
 * From: Id: if_spppsubr.c,v 1.23 1999/02/23 14:47:50 hm Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_spppsubr.c,v 1.262 2022/03/07 04:06:20 knakahara Exp $");

#if defined(_KERNEL_OPT)
#include "opt_inet.h"
#include "opt_modular.h"
#include "opt_compat_netbsd.h"
#include "opt_net_mpsafe.h"
#include "opt_sppp.h"
#endif

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/sockio.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/callout.h>
#include <sys/md5.h>
#include <sys/inttypes.h>
#include <sys/kauth.h>
#include <sys/cprng.h>
#include <sys/module.h>
#include <sys/workqueue.h>
#include <sys/atomic.h>
#include <sys/compat_stub.h>
#include <sys/cpu.h>

#include <net/if.h>
#include <net/netisr.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/ppp_defs.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#ifdef INET
#include <netinet/ip.h>
#include <netinet/tcp.h>
#endif
#include <net/ethertypes.h>

#ifdef INET6
#include <netinet6/scope6_var.h>
#endif

#include <net/if_sppp.h>
#include <net/if_spppvar.h>

#ifdef NET_MPSAFE
#define SPPPSUBR_MPSAFE	1
#endif

#define DEFAULT_KEEPALIVE_INTERVAL	10	/* seconds between checks */
#define DEFAULT_ALIVE_INTERVAL		1	/* count of sppp_keepalive */
#define LOOPALIVECNT     		3	/* loopback detection tries */
#define DEFAULT_MAXALIVECNT    		3	/* max. missed alive packets */
#define	DEFAULT_NORECV_TIME		15	/* before we get worried */
#define DEFAULT_MAX_AUTH_FAILURES	5	/* max. auth. failures */

#ifndef SPPP_KEEPALIVE_INTERVAL
#define SPPP_KEEPALIVE_INTERVAL		DEFAULT_KEEPALIVE_INTERVAL
#endif

#ifndef SPPP_NORECV_TIME
#define SPPP_NORECV_TIME	DEFAULT_NORECV_TIME
#endif

#ifndef SPPP_ALIVE_INTERVAL
#define SPPP_ALIVE_INTERVAL		DEFAULT_ALIVE_INTERVAL
#endif

#define SPPP_CPTYPE_NAMELEN	5	/* buf size of cp type name */
#define SPPP_AUTHTYPE_NAMELEN	32	/* buf size of auth type name */
#define SPPP_LCPOPT_NAMELEN	5	/* buf size of lcp option name  */
#define SPPP_IPCPOPT_NAMELEN	5	/* buf size of ipcp option name */
#define SPPP_IPV6CPOPT_NAMELEN	5	/* buf size of ipv6cp option name */
#define SPPP_PROTO_NAMELEN	7	/* buf size of protocol name */
#define SPPP_DOTQUAD_BUFLEN	16	/* length of "aa.bb.cc.dd" */

/*
 * Interface flags that can be set in an ifconfig command.
 *
 * Setting link0 will make the link passive, i.e. it will be marked
 * as being administrative openable, but won't be opened to begin
 * with.  Incoming calls will be answered, or subsequent calls with
 * -link1 will cause the administrative open of the LCP layer.
 *
 * Setting link1 will cause the link to auto-dial only as packets
 * arrive to be sent.
 *
 * Setting IFF_DEBUG will syslog the option negotiation and state
 * transitions at level kern.debug.  Note: all logs consistently look
 * like
 *
 *   <if-name><unit>: <proto-name> <additional info...>
 *
 * with <if-name><unit> being something like "bppp0", and <proto-name>
 * being one of "lcp", "ipcp", "cisco", "chap", "pap", etc.
 */

#define IFF_PASSIVE	IFF_LINK0	/* wait passively for connection */
#define IFF_AUTO	IFF_LINK1	/* auto-dial on output */

#define CONF_REQ	1		/* PPP configure request */
#define CONF_ACK	2		/* PPP configure acknowledge */
#define CONF_NAK	3		/* PPP configure negative ack */
#define CONF_REJ	4		/* PPP configure reject */
#define TERM_REQ	5		/* PPP terminate request */
#define TERM_ACK	6		/* PPP terminate acknowledge */
#define CODE_REJ	7		/* PPP code reject */
#define PROTO_REJ	8		/* PPP protocol reject */
#define ECHO_REQ	9		/* PPP echo request */
#define ECHO_REPLY	10		/* PPP echo reply */
#define DISC_REQ	11		/* PPP discard request */

#define LCP_OPT_MRU		1	/* maximum receive unit */
#define LCP_OPT_ASYNC_MAP	2	/* async control character map */
#define LCP_OPT_AUTH_PROTO	3	/* authentication protocol */
#define LCP_OPT_QUAL_PROTO	4	/* quality protocol */
#define LCP_OPT_MAGIC		5	/* magic number */
#define LCP_OPT_RESERVED	6	/* reserved */
#define LCP_OPT_PROTO_COMP	7	/* protocol field compression */
#define LCP_OPT_ADDR_COMP	8	/* address/control field compression */
#define LCP_OPT_FCS_ALTS	9	/* FCS alternatives */
#define LCP_OPT_SELF_DESC_PAD	10	/* self-describing padding */
#define LCP_OPT_CALL_BACK	13	/* callback */
#define LCP_OPT_COMPOUND_FRMS	15	/* compound frames */
#define LCP_OPT_MP_MRRU		17	/* multilink MRRU */
#define LCP_OPT_MP_SSNHF	18	/* multilink short seq. numbers */
#define LCP_OPT_MP_EID		19	/* multilink endpoint discriminator */

#define IPCP_OPT_ADDRESSES	1	/* both IP addresses; deprecated */
#define IPCP_OPT_COMPRESSION	2	/* IP compression protocol */
#define IPCP_OPT_ADDRESS	3	/* local IP address */
#define	IPCP_OPT_PRIMDNS	129	/* primary remote dns address */
#define	IPCP_OPT_SECDNS		131	/* secondary remote dns address */

#define IPCP_UPDATE_LIMIT	8	/* limit of pending IP updating job */
#define IPCP_SET_ADDRS		1	/* marker for IP address setting job */
#define IPCP_CLEAR_ADDRS	2	/* marker for IP address clearing job */

#define IPV6CP_OPT_IFID		1	/* interface identifier */
#define IPV6CP_OPT_COMPRESSION	2	/* IPv6 compression protocol */

#define PAP_REQ			1	/* PAP name/password request */
#define PAP_ACK			2	/* PAP acknowledge */
#define PAP_NAK			3	/* PAP fail */

#define CHAP_CHALLENGE		1	/* CHAP challenge request */
#define CHAP_RESPONSE		2	/* CHAP challenge response */
#define CHAP_SUCCESS		3	/* CHAP response ok */
#define CHAP_FAILURE		4	/* CHAP response failed */

#define CHAP_MD5		5	/* hash algorithm - MD5 */

#define CISCO_MULTICAST		0x8f	/* Cisco multicast address */
#define CISCO_UNICAST		0x0f	/* Cisco unicast address */
#define CISCO_KEEPALIVE		0x8035	/* Cisco keepalive protocol */
#define CISCO_ADDR_REQ		0	/* Cisco address request */
#define CISCO_ADDR_REPLY	1	/* Cisco address reply */
#define CISCO_KEEPALIVE_REQ	2	/* Cisco keepalive request */

#define PPP_NOPROTO		0	/* no authentication protocol */

enum {
	STATE_INITIAL = SPPP_STATE_INITIAL,
	STATE_STARTING = SPPP_STATE_STARTING,
	STATE_CLOSED = SPPP_STATE_CLOSED,
	STATE_STOPPED = SPPP_STATE_STOPPED,
	STATE_CLOSING = SPPP_STATE_CLOSING,
	STATE_STOPPING = SPPP_STATE_STOPPING,
	STATE_REQ_SENT = SPPP_STATE_REQ_SENT,
	STATE_ACK_RCVD = SPPP_STATE_ACK_RCVD,
	STATE_ACK_SENT = SPPP_STATE_ACK_SENT,
	STATE_OPENED = SPPP_STATE_OPENED,
};

enum cp_rcr_type {
	CP_RCR_NONE = 0,	/* initial value */
	CP_RCR_ACK,	/* RCR+ */
	CP_RCR_NAK,	/* RCR- */
	CP_RCR_REJ,	/* RCR- */
	CP_RCR_DROP,	/* DROP message */
	CP_RCR_ERR,	/* internal error */
};

struct ppp_header {
	uint8_t address;
	uint8_t control;
	uint16_t protocol;
} __packed;
#define PPP_HEADER_LEN          sizeof (struct ppp_header)

struct lcp_header {
	uint8_t type;
	uint8_t ident;
	uint16_t len;
} __packed;
#define LCP_HEADER_LEN          sizeof (struct lcp_header)

struct cisco_packet {
	uint32_t type;
	uint32_t par1;
	uint32_t par2;
	uint16_t rel;
	uint16_t time0;
	uint16_t time1;
} __packed;
#define CISCO_PACKET_LEN 18

/*
 * We follow the spelling and capitalization of RFC 1661 here, to make
 * it easier comparing with the standard.  Please refer to this RFC in
 * case you can't make sense out of these abbreviation; it will also
 * explain the semantics related to the various events and actions.
 */
struct cp {
	u_short	proto;		/* PPP control protocol number */
	u_char protoidx;	/* index into state table in struct sppp */
	u_char flags;
#define CP_LCP		0x01	/* this is the LCP */
#define CP_AUTH		0x02	/* this is an authentication protocol */
#define CP_NCP		0x04	/* this is a NCP */
#define CP_QUAL		0x08	/* this is a quality reporting protocol */
	const char *name;	/* name of this control protocol */
	/* event handlers */
	void	(*Up)(struct sppp *, void *);
	void	(*Down)(struct sppp *, void *);
	void	(*Open)(struct sppp *, void *);
	void	(*Close)(struct sppp *, void *);
	void	(*TO)(struct sppp *, void *);
	/* actions */
	void	(*tlu)(struct sppp *);
	void	(*tld)(struct sppp *);
	void	(*tls)(const struct cp *, struct sppp *);
	void	(*tlf)(const struct cp *, struct sppp *);
	void	(*scr)(struct sppp *);
	void	(*screply)(const struct cp *, struct sppp *, u_char,
		    uint8_t, size_t, void *);

	/* message parser */
	enum cp_rcr_type
		(*parse_confreq)(struct sppp *, struct lcp_header *, int,
			    uint8_t **, size_t *, size_t *);
	void	(*parse_confrej)(struct sppp *, struct lcp_header *, int);
	void	(*parse_confnak)(struct sppp *, struct lcp_header *, int);
};

enum auth_role {
	SPPP_AUTH_NOROLE = 0,
	SPPP_AUTH_SERV = __BIT(0),
	SPPP_AUTH_PEER = __BIT(1),
};

static struct sppp *spppq;
static kmutex_t *spppq_lock = NULL;
static callout_t keepalive_ch;
static unsigned int sppp_keepalive_cnt = 0;

pktq_rps_hash_func_t sppp_pktq_rps_hash_p;

#define SPPPQ_LOCK()	if (spppq_lock) \
				mutex_enter(spppq_lock);
#define SPPPQ_UNLOCK()	if (spppq_lock) \
				mutex_exit(spppq_lock);

#define SPPP_LOCK(_sp, _op)	rw_enter(&(_sp)->pp_lock, (_op))
#define SPPP_UNLOCK(_sp)	rw_exit(&(_sp)->pp_lock)
#define SPPP_WLOCKED(_sp)	rw_write_held(&(_sp)->pp_lock)
#define SPPP_UPGRADE(_sp)	do{	\
	SPPP_UNLOCK(_sp);		\
	SPPP_LOCK(_sp, RW_WRITER);	\
}while (0)
#define SPPP_DOWNGRADE(_sp)	rw_downgrade(&(_sp)->pp_lock)
#define SPPP_WQ_SET(_wk, _func, _arg)	\
	sppp_wq_set((_wk), (_func), __UNCONST((_arg)))
#define SPPP_LOG(_sp, _lvl, _fmt, _args...)	do {		\
	if (__predict_true((_sp) != NULL)) {			\
		log((_lvl), "%s: ", (_sp)->pp_if.if_xname);	\
	}							\
	addlog((_fmt), ##_args);				\
} while (0)
#define SPPP_DLOG(_sp, _fmt, _args...)	do {	\
	if (!sppp_debug_enabled(_sp))			\
		break;					\
	SPPP_LOG(_sp, LOG_DEBUG, _fmt, ##_args);	\
} while (0)

#ifdef INET
#ifndef SPPPSUBR_MPSAFE
/*
 * The following disgusting hack gets around the problem that IP TOS
 * can't be set yet.  We want to put "interactive" traffic on a high
 * priority queue.  To decide if traffic is interactive, we check that
 * a) it is TCP and b) one of its ports is telnet, rlogin or ftp control.
 *
 * XXX is this really still necessary?  - joerg -
 */
static u_short interactive_ports[8] = {
	0,	513,	0,	0,
	0,	21,	0,	23,
};
#define INTERACTIVE(p)	(interactive_ports[(p) & 7] == (p))
#endif /* SPPPSUBR_MPSAFE */
#endif

/* almost every function needs these */

static bool sppp_debug_enabled(struct sppp *sp);
static int sppp_output(struct ifnet *, struct mbuf *,
		       const struct sockaddr *, const struct rtentry *);

static void sppp_cp_init(const struct cp *, struct sppp *);
static void sppp_cp_fini(const struct cp *, struct sppp *);
static void sppp_cp_input(const struct cp *, struct sppp *,
			  struct mbuf *);
static void sppp_cp_input(const struct cp *, struct sppp *,
			  struct mbuf *);
static void sppp_cp_send(struct sppp *, u_short, u_char,
			 u_char, u_short, void *);
/* static void sppp_cp_timeout(void *arg); */
static void sppp_cp_change_state(const struct cp *, struct sppp *, int);
static struct workqueue *
    sppp_wq_create(struct sppp *, const char *, pri_t, int, int);
static void sppp_wq_destroy(struct sppp *, struct workqueue *);
static void sppp_wq_set(struct sppp_work *,
    void (*)(struct sppp *, void *), void *);
static void sppp_wq_add(struct workqueue *, struct sppp_work *);
static void sppp_wq_wait(struct workqueue *, struct sppp_work *);
static void sppp_cp_to_lcp(void *);
static void sppp_cp_to_ipcp(void *);
static void sppp_cp_to_ipv6cp(void *);
static void sppp_auth_send(const struct cp *, struct sppp *,
			    unsigned int, unsigned int, ...);
static int sppp_auth_role(const struct cp *, struct sppp *);
static void sppp_auth_to_event(struct sppp *, void *);
static void sppp_auth_screply(const struct cp *, struct sppp *,
			    u_char, uint8_t, size_t, void *);
static void sppp_up_event(struct sppp *, void *);
static void sppp_down_event(struct sppp *, void *);
static void sppp_open_event(struct sppp *, void *);
static void sppp_close_event(struct sppp *, void *);
static void sppp_to_event(struct sppp *, void *);
static void sppp_rcr_event(struct sppp *, void *);
static void sppp_rca_event(struct sppp *, void *);
static void sppp_rcn_event(struct sppp *, void *);
static void sppp_rtr_event(struct sppp *, void *);
static void sppp_rta_event(struct sppp *, void *);
static void sppp_rxj_event(struct sppp *, void *);

static void sppp_null(struct sppp *);
static void sppp_tls(const struct cp *, struct sppp *);
static void sppp_tlf(const struct cp *, struct sppp *);
static void sppp_screply(const struct cp *, struct sppp *,
		    u_char, uint8_t, size_t, void *);
static void sppp_ifdown(struct sppp *, void *);

static void sppp_lcp_init(struct sppp *);
static void sppp_lcp_up(struct sppp *, void *);
static void sppp_lcp_down(struct sppp *, void *);
static void sppp_lcp_open(struct sppp *, void *);
static enum cp_rcr_type
	    sppp_lcp_confreq(struct sppp *, struct lcp_header *, int,
		    uint8_t **, size_t *, size_t *);
static void sppp_lcp_confrej(struct sppp *, struct lcp_header *, int);
static void sppp_lcp_confnak(struct sppp *, struct lcp_header *, int);
static void sppp_lcp_tlu(struct sppp *);
static void sppp_lcp_tld(struct sppp *);
static void sppp_lcp_tls(const struct cp *, struct sppp *);
static void sppp_lcp_tlf(const struct cp *, struct sppp *);
static void sppp_lcp_scr(struct sppp *);
static void sppp_lcp_check_and_close(struct sppp *);
static int sppp_cp_check(struct sppp *, u_char);

static void sppp_ipcp_init(struct sppp *);
static void sppp_ipcp_open(struct sppp *, void *);
static void sppp_ipcp_close(struct sppp *, void *);
static enum cp_rcr_type
	    sppp_ipcp_confreq(struct sppp *, struct lcp_header *, int,
		    uint8_t **, size_t *, size_t *);
static void sppp_ipcp_confrej(struct sppp *, struct lcp_header *, int);
static void sppp_ipcp_confnak(struct sppp *, struct lcp_header *, int);
static void sppp_ipcp_tlu(struct sppp *);
static void sppp_ipcp_tld(struct sppp *);
static void sppp_ipcp_scr(struct sppp *);

static void sppp_ipv6cp_init(struct sppp *);
static void sppp_ipv6cp_open(struct sppp *, void *);
static enum cp_rcr_type
	    sppp_ipv6cp_confreq(struct sppp *, struct lcp_header *, int,
		    uint8_t **, size_t *, size_t *);
static void sppp_ipv6cp_confrej(struct sppp *, struct lcp_header *, int);
static void sppp_ipv6cp_confnak(struct sppp *, struct lcp_header *, int);
static void sppp_ipv6cp_tlu(struct sppp *);
static void sppp_ipv6cp_tld(struct sppp *);
static void sppp_ipv6cp_scr(struct sppp *);

static void sppp_pap_input(struct sppp *, struct mbuf *);
static void sppp_pap_init(struct sppp *);
static void sppp_pap_tlu(struct sppp *);
static void sppp_pap_scr(struct sppp *);
static void sppp_pap_scr(struct sppp *);

static void sppp_chap_input(struct sppp *, struct mbuf *);
static void sppp_chap_init(struct sppp *);
static void sppp_chap_open(struct sppp *, void *);
static void sppp_chap_tlu(struct sppp *);
static void sppp_chap_scr(struct sppp *);
static void sppp_chap_rcv_challenge_event(struct sppp *, void *);

static const char *sppp_auth_type_name(char *, size_t, u_short, u_char);
static const char *sppp_cp_type_name(char *, size_t, u_char);
static const char *sppp_dotted_quad(char *, size_t, uint32_t);
static const char *sppp_ipcp_opt_name(char *, size_t, u_char);
#ifdef INET6
static const char *sppp_ipv6cp_opt_name(char *, size_t, u_char);
#endif
static const char *sppp_lcp_opt_name(char *, size_t, u_char);
static const char *sppp_phase_name(int);
static const char *sppp_proto_name(char *, size_t, u_short);
static const char *sppp_state_name(int);
static int sppp_params(struct sppp *, u_long, void *);
#ifdef INET
static void sppp_get_ip_addrs(struct sppp *, uint32_t *, uint32_t *, uint32_t *);
static void sppp_set_ip_addrs(struct sppp *);
static void sppp_clear_ip_addrs(struct sppp *);
#endif
static void sppp_keepalive(void *);
static void sppp_phase_network(struct sppp *);
static void sppp_print_bytes(const u_char *, u_short);
static void sppp_print_string(const char *, u_short);
#ifdef INET6
static void sppp_get_ip6_addrs(struct sppp *, struct in6_addr *,
				struct in6_addr *, struct in6_addr *);
#ifdef IPV6CP_MYIFID_DYN
static void sppp_set_ip6_addr(struct sppp *, const struct in6_addr *);
static void sppp_gen_ip6_addr(struct sppp *, const struct in6_addr *);
#endif
static void sppp_suggest_ip6_addr(struct sppp *, struct in6_addr *);
#endif

static void sppp_notify_up(struct sppp *);
static void sppp_notify_down(struct sppp *);
static void sppp_notify_tls_wlocked(struct sppp *);
static void sppp_notify_tlf_wlocked(struct sppp *);
#ifdef INET6
static void sppp_notify_con_wlocked(struct sppp *);
#endif
static void sppp_notify_con(struct sppp *);

static void sppp_notify_chg_wlocked(struct sppp *);

/* our control protocol descriptors */
static const struct cp lcp = {
	PPP_LCP, IDX_LCP, CP_LCP, "lcp",
	sppp_lcp_up, sppp_lcp_down, sppp_lcp_open,
	sppp_close_event, sppp_to_event,
	sppp_lcp_tlu, sppp_lcp_tld, sppp_lcp_tls,
	sppp_lcp_tlf, sppp_lcp_scr, sppp_screply,
	sppp_lcp_confreq, sppp_lcp_confrej, sppp_lcp_confnak
};

static const struct cp ipcp = {
	PPP_IPCP, IDX_IPCP,
#ifdef INET
	CP_NCP,	/*don't run IPCP if there's no IPv4 support*/
#else
	0,
#endif
	"ipcp",
	sppp_up_event, sppp_down_event, sppp_ipcp_open,
	sppp_ipcp_close, sppp_to_event,
	sppp_ipcp_tlu, sppp_ipcp_tld, sppp_tls,
	sppp_tlf, sppp_ipcp_scr, sppp_screply,
	sppp_ipcp_confreq, sppp_ipcp_confrej, sppp_ipcp_confnak,
};

static const struct cp ipv6cp = {
	PPP_IPV6CP, IDX_IPV6CP,
#ifdef INET6	/*don't run IPv6CP if there's no IPv6 support*/
	CP_NCP,
#else
	0,
#endif
	"ipv6cp",
	sppp_up_event, sppp_down_event, sppp_ipv6cp_open,
	sppp_close_event, sppp_to_event,
	sppp_ipv6cp_tlu, sppp_ipv6cp_tld, sppp_tls,
	sppp_tlf, sppp_ipv6cp_scr, sppp_screply,
	sppp_ipv6cp_confreq, sppp_ipv6cp_confrej, sppp_ipv6cp_confnak,
};

static const struct cp pap = {
	PPP_PAP, IDX_PAP, CP_AUTH, "pap",
	sppp_up_event, sppp_down_event, sppp_open_event,
	sppp_close_event, sppp_auth_to_event,
	sppp_pap_tlu, sppp_null, sppp_tls, sppp_tlf,
	sppp_pap_scr, sppp_auth_screply,
	NULL, NULL, NULL
};

static const struct cp chap = {
	PPP_CHAP, IDX_CHAP, CP_AUTH, "chap",
	sppp_up_event, sppp_down_event, sppp_chap_open,
	sppp_close_event, sppp_auth_to_event,
	sppp_chap_tlu, sppp_null, sppp_tls, sppp_tlf,
	sppp_chap_scr, sppp_auth_screply,
	NULL, NULL, NULL
};

static const struct cp *cps[IDX_COUNT] = {
	&lcp,			/* IDX_LCP */
	&ipcp,			/* IDX_IPCP */
	&ipv6cp,		/* IDX_IPV6CP */
	&pap,			/* IDX_PAP */
	&chap,			/* IDX_CHAP */
};

static inline u_int
sppp_proto2authproto(u_short proto)
{

	switch (proto) {
	case PPP_PAP:
		return SPPP_AUTHPROTO_PAP;
	case PPP_CHAP:
		return SPPP_AUTHPROTO_CHAP;
	}

	return SPPP_AUTHPROTO_NONE;
}

static inline u_short
sppp_authproto2proto(u_int authproto)
{

	switch (authproto) {
	case SPPP_AUTHPROTO_PAP:
		return PPP_PAP;
	case SPPP_AUTHPROTO_CHAP:
		return PPP_CHAP;
	}

	return PPP_NOPROTO;
}

static inline bool
sppp_debug_enabled(struct sppp *sp)
{

	if (__predict_false(sp == NULL))
		return false;

	if ((sp->pp_if.if_flags & IFF_DEBUG) == 0)
		return false;

	return true;
}

static void
sppp_change_phase(struct sppp *sp, int phase)
{
	struct ifnet *ifp;

	KASSERT(SPPP_WLOCKED(sp));

	ifp = &sp->pp_if;

	if (sp->pp_phase == phase)
		return;

	sp->pp_phase = phase;

	if (phase == SPPP_PHASE_NETWORK)
		if_link_state_change(ifp, LINK_STATE_UP);
	else
		if_link_state_change(ifp, LINK_STATE_DOWN);

	SPPP_DLOG(sp, "phase %s\n",
	    sppp_phase_name(sp->pp_phase));
}

/*
 * Exported functions, comprising our interface to the lower layer.
 */

/*
 * Process the received packet.
 */
void
sppp_input(struct ifnet *ifp, struct mbuf *m)
{
	struct ppp_header *h = NULL;
	pktqueue_t *pktq = NULL;
	struct ifqueue *inq = NULL;
	uint16_t protocol;
	struct sppp *sp = (struct sppp *)ifp;
	int isr = 0;

	SPPP_LOCK(sp, RW_READER);

	if (ifp->if_flags & IFF_UP) {
		/* Count received bytes, add hardware framing */
		if_statadd(ifp, if_ibytes, m->m_pkthdr.len + sp->pp_framebytes);
		/* Note time of last receive */
		sp->pp_last_receive = time_uptime;
	}

	if (m->m_pkthdr.len <= PPP_HEADER_LEN) {
		/* Too small packet, drop it. */
		SPPP_DLOG(sp, "input packet is too small, "
		    "%d bytes\n", m->m_pkthdr.len);
		goto drop;
	}

	if (sp->pp_flags & PP_NOFRAMING) {
		memcpy(&protocol, mtod(m, void *), 2);
		protocol = ntohs(protocol);
		m_adj(m, 2);
	} else {

		/* Get PPP header. */
		h = mtod(m, struct ppp_header *);
		m_adj(m, PPP_HEADER_LEN);

		switch (h->address) {
		case PPP_ALLSTATIONS:
			if (h->control != PPP_UI)
				goto invalid;
			break;
		case CISCO_MULTICAST:
		case CISCO_UNICAST:
			/* Don't check the control field here (RFC 1547). */
			SPPP_DLOG(sp, "Cisco packet in PPP mode "
			    "<addr=0x%x ctrl=0x%x proto=0x%x>\n",
			    h->address, h->control, ntohs(h->protocol));
			goto drop;
		default:        /* Invalid PPP packet. */
		  invalid:
			SPPP_DLOG(sp, "invalid input packet "
			    "<addr=0x%x ctrl=0x%x proto=0x%x>\n",
			    h->address, h->control, ntohs(h->protocol));
			goto drop;
		}
		protocol = ntohs(h->protocol);
	}

	switch (protocol) {
	reject_protocol:
		if (sp->scp[IDX_LCP].state == STATE_OPENED) {
			uint16_t prot = htons(protocol);

			SPPP_UPGRADE(sp);
			sppp_cp_send(sp, PPP_LCP, PROTO_REJ,
			    ++sp->scp[IDX_LCP].seq, m->m_pkthdr.len + 2,
			    &prot);
			SPPP_DOWNGRADE(sp);
		}
		if_statinc(ifp, if_noproto);
		goto drop;
	default:
		SPPP_DLOG(sp, "invalid input protocol "
		    "<proto=0x%x>\n", protocol);
		goto reject_protocol;
	case PPP_LCP:
		SPPP_UNLOCK(sp);
		sppp_cp_input(&lcp, sp, m);
		/* already m_freem(m) */
		return;
	case PPP_PAP:
		SPPP_UNLOCK(sp);
		if (sp->pp_phase >= SPPP_PHASE_AUTHENTICATE) {
			sppp_pap_input(sp, m);
		}
		m_freem(m);
		return;
	case PPP_CHAP:
		SPPP_UNLOCK(sp);
		if (sp->pp_phase >= SPPP_PHASE_AUTHENTICATE) {
			sppp_chap_input(sp, m);
		}
		m_freem(m);
		return;
#ifdef INET
	case PPP_IPCP:
		if (!ISSET(sp->pp_ncpflags, SPPP_NCP_IPCP)) {
			SPPP_LOG(sp, LOG_INFO, "reject IPCP packet "
			    "because IPCP is disabled\n");
			goto reject_protocol;
		}
		SPPP_UNLOCK(sp);
		if (sp->pp_phase == SPPP_PHASE_NETWORK) {
			sppp_cp_input(&ipcp, sp, m);
			/* already m_freem(m) */
		} else {
			m_freem(m);
		}
		return;
	case PPP_IP:
		if (sp->scp[IDX_IPCP].state == STATE_OPENED) {
			sp->pp_last_activity = time_uptime;
			pktq = ip_pktq;
		}
		break;
#endif
#ifdef INET6
	case PPP_IPV6CP:
		if (!ISSET(sp->pp_ncpflags, SPPP_NCP_IPV6CP)) {
			SPPP_LOG(sp, LOG_INFO, "reject IPv6CP packet "
			    "because IPv6CP is disabled\n");
			goto reject_protocol;
		}
		SPPP_UNLOCK(sp);
		if (sp->pp_phase == SPPP_PHASE_NETWORK) {
			sppp_cp_input(&ipv6cp, sp, m);
			/* already m_freem(m) */
		} else {
			m_freem(m);
		}
		return;

	case PPP_IPV6:
		if (sp->scp[IDX_IPV6CP].state == STATE_OPENED) {
			sp->pp_last_activity = time_uptime;
			pktq = ip6_pktq;
		}
		break;
#endif
	}

	if ((ifp->if_flags & IFF_UP) == 0 || (!inq && !pktq)) {
		goto drop;
	}

	/* Check queue. */
	if (__predict_true(pktq)) {
		uint32_t hash = pktq_rps_hash(&sppp_pktq_rps_hash_p, m);

		if (__predict_false(!pktq_enqueue(pktq, m, hash))) {
			goto drop;
		}
		SPPP_UNLOCK(sp);
		return;
	}

	SPPP_UNLOCK(sp);

	IFQ_LOCK(inq);
	if (IF_QFULL(inq)) {
		/* Queue overflow. */
		IF_DROP(inq);
		IFQ_UNLOCK(inq);
		SPPP_DLOG(sp,"protocol queue overflow\n");
		SPPP_LOCK(sp, RW_READER);
		goto drop;
	}
	IF_ENQUEUE(inq, m);
	IFQ_UNLOCK(inq);
	schednetisr(isr);
	return;

drop:
	if_statadd2(ifp, if_ierrors, 1, if_iqdrops, 1);
	m_freem(m);
	SPPP_UNLOCK(sp);
	return;
}

/*
 * Enqueue transmit packet.
 */
static int
sppp_output(struct ifnet *ifp, struct mbuf *m,
    const struct sockaddr *dst, const struct rtentry *rt)
{
	struct sppp *sp = (struct sppp *) ifp;
	struct ppp_header *h = NULL;
#ifndef SPPPSUBR_MPSAFE
	struct ifqueue *ifq = NULL;		/* XXX */
#endif
	int s, error = 0;
	uint16_t protocol;
	size_t pktlen;

	s = splnet();
	SPPP_LOCK(sp, RW_READER);

	sp->pp_last_activity = time_uptime;

	if ((ifp->if_flags & IFF_UP) == 0 ||
	    (ifp->if_flags & (IFF_RUNNING | IFF_AUTO)) == 0) {
		SPPP_UNLOCK(sp);
		splx(s);

		m_freem(m);
		if_statinc(ifp, if_oerrors);
		return (ENETDOWN);
	}

	if ((ifp->if_flags & (IFF_RUNNING | IFF_AUTO)) == IFF_AUTO) {
		/* ignore packets that have no enabled NCP */
		if ((dst->sa_family == AF_INET &&
		    !ISSET(sp->pp_ncpflags, SPPP_NCP_IPCP)) ||
		    (dst->sa_family == AF_INET6 &&
		    !ISSET(sp->pp_ncpflags, SPPP_NCP_IPV6CP))) {
			SPPP_UNLOCK(sp);
			splx(s);

			m_freem(m);
			if_statinc(ifp, if_oerrors);
			return (ENETDOWN);
		}
		/*
		 * Interface is not yet running, but auto-dial.  Need
		 * to start LCP for it.
		 */
		ifp->if_flags |= IFF_RUNNING;
		sppp_wq_add(sp->wq_cp, &sp->scp[IDX_LCP].work_open);
	}

	/*
	 * If the queueing discipline needs packet classification,
	 * do it before prepending link headers.
	 */
	IFQ_CLASSIFY(&ifp->if_snd, m, dst->sa_family);

#ifdef INET
	if (dst->sa_family == AF_INET) {
		struct ip *ip = NULL;
#ifndef SPPPSUBR_MPSAFE
		struct tcphdr *th = NULL;
#endif

		if (m->m_len >= sizeof(struct ip)) {
			ip = mtod(m, struct ip *);
#ifndef SPPPSUBR_MPSAFE
			if (ip->ip_p == IPPROTO_TCP &&
			    m->m_len >= sizeof(struct ip) + (ip->ip_hl << 2) +
			    sizeof(struct tcphdr)) {
				th = (struct tcphdr *)
				    ((char *)ip + (ip->ip_hl << 2));
			}
#endif
		} else
			ip = NULL;

		/*
		 * When using dynamic local IP address assignment by using
		 * 0.0.0.0 as a local address, the first TCP session will
		 * not connect because the local TCP checksum is computed
		 * using 0.0.0.0 which will later become our real IP address
		 * so the TCP checksum computed at the remote end will
		 * become invalid. So we
		 * - don't let packets with src ip addr 0 thru
		 * - we flag TCP packets with src ip 0 as an error
		 */
		if (ip && ip->ip_src.s_addr == INADDR_ANY) {
			uint8_t proto = ip->ip_p;

			SPPP_UNLOCK(sp);
			splx(s);

			m_freem(m);
			if (proto == IPPROTO_TCP)
				return (EADDRNOTAVAIL);
			else
				return (0);
		}

#ifndef SPPPSUBR_MPSAFE
		/*
		 * Put low delay, telnet, rlogin and ftp control packets
		 * in front of the queue.
		 */
		if (!IF_QFULL(&sp->pp_fastq) &&
		    ((ip && (ip->ip_tos & IPTOS_LOWDELAY)) ||
		     (th && (INTERACTIVE(ntohs(th->th_sport)) ||
		      INTERACTIVE(ntohs(th->th_dport))))))
			ifq = &sp->pp_fastq;
#endif /* !SPPPSUBR_MPSAFE */
	}
#endif

#ifdef INET6
	if (dst->sa_family == AF_INET6) {
		/* XXX do something tricky here? */
	}
#endif

	if ((sp->pp_flags & PP_NOFRAMING) == 0) {
		/*
		 * Prepend general data packet PPP header. For now, IP only.
		 */
		M_PREPEND(m, PPP_HEADER_LEN, M_DONTWAIT);
		if (! m) {
			SPPP_DLOG(sp, "no memory for transmit header\n");
			if_statinc(ifp, if_oerrors);
			SPPP_UNLOCK(sp);
			splx(s);
			return (ENOBUFS);
		}
		/*
		 * May want to check size of packet
		 * (albeit due to the implementation it's always enough)
		 */
		h = mtod(m, struct ppp_header *);
		h->address = PPP_ALLSTATIONS;        /* broadcast address */
		h->control = PPP_UI;                 /* Unnumbered Info */
	}

	switch (dst->sa_family) {
#ifdef INET
	case AF_INET:   /* Internet Protocol */
		/*
		 * Don't choke with an ENETDOWN early.  It's
		 * possible that we just started dialing out,
		 * so don't drop the packet immediately.  If
		 * we notice that we run out of buffer space
		 * below, we will however remember that we are
		 * not ready to carry IP packets, and return
		 * ENETDOWN, as opposed to ENOBUFS.
		 */
		protocol = htons(PPP_IP);
		if (sp->scp[IDX_IPCP].state != STATE_OPENED) {
			if (ifp->if_flags & IFF_AUTO) {
				error = ENETDOWN;
			} else {
				IF_DROP(&ifp->if_snd);
				SPPP_UNLOCK(sp);
				splx(s);

				m_freem(m);
				if_statinc(ifp, if_oerrors);
				return (ENETDOWN);
			}
		}
		break;
#endif
#ifdef INET6
	case AF_INET6:   /* Internet Protocol version 6 */
		/*
		 * Don't choke with an ENETDOWN early.  It's
		 * possible that we just started dialing out,
		 * so don't drop the packet immediately.  If
		 * we notice that we run out of buffer space
		 * below, we will however remember that we are
		 * not ready to carry IP packets, and return
		 * ENETDOWN, as opposed to ENOBUFS.
		 */
		protocol = htons(PPP_IPV6);
		if (sp->scp[IDX_IPV6CP].state != STATE_OPENED) {
			if (ifp->if_flags & IFF_AUTO) {
				error = ENETDOWN;
			} else {
				IF_DROP(&ifp->if_snd);
				SPPP_UNLOCK(sp);
				splx(s);

				m_freem(m);
				if_statinc(ifp, if_oerrors);
				return (ENETDOWN);
			}
		}
		break;
#endif
	default:
		m_freem(m);
		if_statinc(ifp, if_oerrors);
		SPPP_UNLOCK(sp);
		splx(s);
		return (EAFNOSUPPORT);
	}

	if (sp->pp_flags & PP_NOFRAMING) {
		M_PREPEND(m, 2, M_DONTWAIT);
		if (m == NULL) {
			SPPP_DLOG(sp, "no memory for transmit header\n");
			if_statinc(ifp, if_oerrors);
			SPPP_UNLOCK(sp);
			splx(s);
			return (ENOBUFS);
		}
		*mtod(m, uint16_t *) = protocol;
	} else {
		h->protocol = protocol;
	}

	pktlen = m->m_pkthdr.len;
#ifdef SPPPSUBR_MPSAFE
	SPPP_UNLOCK(sp);
	error = if_transmit_lock(ifp, m);
	SPPP_LOCK(sp, RW_READER);
	if (error == 0)
		if_statadd(ifp, if_obytes, pktlen + sp->pp_framebytes);
#else /* !SPPPSUBR_MPSAFE */
	error = ifq_enqueue2(ifp, ifq, m);

	if (error == 0) {
		/*
		 * Count output packets and bytes.
		 * The packet length includes header + additional hardware
		 * framing according to RFC 1333.
		 */
		if (!(ifp->if_flags & IFF_OACTIVE)) {
			SPPP_UNLOCK(sp);
			if_start_lock(ifp);
			SPPP_LOCK(sp, RW_READER);
		}
		if_statadd(ifp, if_obytes, pktlen + sp->pp_framebytes);
	}
#endif /* !SPPPSUBR_MPSAFE */
	SPPP_UNLOCK(sp);
	splx(s);
	return error;
}

void
sppp_attach(struct ifnet *ifp)
{
	struct sppp *sp = (struct sppp *) ifp;
	char xnamebuf[MAXCOMLEN];

	/* Initialize keepalive handler. */
	if (! spppq) {
		callout_init(&keepalive_ch, CALLOUT_MPSAFE);
		callout_reset(&keepalive_ch, hz * SPPP_KEEPALIVE_INTERVAL, sppp_keepalive, NULL);
	}

	if (! spppq_lock)
		spppq_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_SOFTNET);

	sp->pp_if.if_type = IFT_PPP;
	sp->pp_if.if_output = sppp_output;
	sp->pp_fastq.ifq_maxlen = 32;
	sp->pp_cpq.ifq_maxlen = 20;
	sp->pp_loopcnt = 0;
	sp->pp_alivecnt = 0;
	sp->pp_alive_interval = SPPP_ALIVE_INTERVAL;
	sp->pp_last_activity = 0;
	sp->pp_last_receive = 0;
	sp->pp_maxalive = DEFAULT_MAXALIVECNT;
	sp->pp_max_noreceive = SPPP_NORECV_TIME;
	sp->pp_idle_timeout = 0;
	sp->pp_max_auth_fail = DEFAULT_MAX_AUTH_FAILURES;
	sp->pp_phase = SPPP_PHASE_DEAD;
	sp->pp_up = sppp_notify_up;
	sp->pp_down = sppp_notify_down;
	sp->pp_ncpflags = SPPP_NCP_IPCP | SPPP_NCP_IPV6CP;
#ifdef SPPP_IFDOWN_RECONNECT
	sp->pp_flags |= PP_LOOPBACK_IFDOWN | PP_KEEPALIVE_IFDOWN;
#endif
	sppp_wq_set(&sp->work_ifdown, sppp_ifdown, NULL);
	memset(sp->scp, 0, sizeof(sp->scp));
	rw_init(&sp->pp_lock);

	if_alloc_sadl(ifp);

	/* Lets not beat about the bush, we know we're down. */
	if_link_state_change(ifp, LINK_STATE_DOWN);

	snprintf(xnamebuf, sizeof(xnamebuf), "%s.wq_cp", ifp->if_xname);
	sp->wq_cp = sppp_wq_create(sp, xnamebuf,
	    PRI_SOFTNET, IPL_SOFTNET, WQ_MPSAFE);

	memset(&sp->myauth, 0, sizeof sp->myauth);
	memset(&sp->hisauth, 0, sizeof sp->hisauth);
	SPPP_LOCK(sp, RW_WRITER);
	sppp_lcp_init(sp);
	sppp_ipcp_init(sp);
	sppp_ipv6cp_init(sp);
	sppp_pap_init(sp);
	sppp_chap_init(sp);
	SPPP_UNLOCK(sp);

	SPPPQ_LOCK();
	/* Insert new entry into the keepalive list. */
	sp->pp_next = spppq;
	spppq = sp;
	SPPPQ_UNLOCK();
}

void
sppp_detach(struct ifnet *ifp)
{
	struct sppp **q, *p, *sp = (struct sppp *) ifp;

	/* Remove the entry from the keepalive list. */
	SPPPQ_LOCK();
	for (q = &spppq; (p = *q); q = &p->pp_next)
		if (p == sp) {
			*q = p->pp_next;
			break;
		}
	SPPPQ_UNLOCK();

	if (! spppq) {
		/* Stop keepalive handler. */
		callout_stop(&keepalive_ch);
		mutex_obj_free(spppq_lock);
		spppq_lock = NULL;
	}

	sppp_cp_fini(&lcp, sp);
	sppp_cp_fini(&ipcp, sp);
	sppp_cp_fini(&pap, sp);
	sppp_cp_fini(&chap, sp);
#ifdef INET6
	sppp_cp_fini(&ipv6cp, sp);
#endif
	sppp_wq_destroy(sp, sp->wq_cp);

	/* free authentication info */
	if (sp->myauth.name) free(sp->myauth.name, M_DEVBUF);
	if (sp->myauth.secret) free(sp->myauth.secret, M_DEVBUF);
	if (sp->hisauth.name) free(sp->hisauth.name, M_DEVBUF);
	if (sp->hisauth.secret) free(sp->hisauth.secret, M_DEVBUF);
	rw_destroy(&sp->pp_lock);
}

/*
 * Flush the interface output queue.
 */
void
sppp_flush(struct ifnet *ifp)
{
	struct sppp *sp = (struct sppp *) ifp;

	SPPP_LOCK(sp, RW_WRITER);
	IFQ_PURGE(&sp->pp_if.if_snd);
	IF_PURGE(&sp->pp_fastq);
	IF_PURGE(&sp->pp_cpq);
	SPPP_UNLOCK(sp);
}

/*
 * Check if the output queue is empty.
 */
int
sppp_isempty(struct ifnet *ifp)
{
	struct sppp *sp = (struct sppp *) ifp;
	int empty, s;

	s = splnet();
	SPPP_LOCK(sp, RW_READER);
	empty = IF_IS_EMPTY(&sp->pp_fastq) && IF_IS_EMPTY(&sp->pp_cpq) &&
		IFQ_IS_EMPTY(&sp->pp_if.if_snd);
	SPPP_UNLOCK(sp);
	splx(s);
	return (empty);
}

/*
 * Get next packet to send.
 */
struct mbuf *
sppp_dequeue(struct ifnet *ifp)
{
	struct sppp *sp = (struct sppp *) ifp;
	struct mbuf *m;
	int s;

	s = splnet();
	SPPP_LOCK(sp, RW_WRITER);
	/*
	 * Process only the control protocol queue until we have at
	 * least one NCP open.
	 *
	 * Do always serve all three queues in Cisco mode.
	 */
	IF_DEQUEUE(&sp->pp_cpq, m);
	if (m == NULL && sppp_cp_check(sp, CP_NCP)) {
		IF_DEQUEUE(&sp->pp_fastq, m);
		if (m == NULL)
			IFQ_DEQUEUE(&sp->pp_if.if_snd, m);
	}
	SPPP_UNLOCK(sp);
	splx(s);
	return m;
}

/*
 * Process an ioctl request.  Called on low priority level.
 */
int
sppp_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct lwp *l = curlwp;	/* XXX */
	struct ifreq *ifr = (struct ifreq *) data;
	struct ifaddr *ifa = (struct ifaddr *) data;
	struct sppp *sp = (struct sppp *) ifp;
	int s, error=0, going_up, going_down;
	u_short newmode;
	u_long lcp_mru;

	s = splnet();
	switch (cmd) {
	case SIOCINITIFADDR:
		ifa->ifa_rtrequest = p2p_rtrequest;
		break;

	case SIOCSIFFLAGS:
		if ((error = ifioctl_common(ifp, cmd, data)) != 0)
			break;

		SPPP_LOCK(sp, RW_WRITER);
		going_up = ifp->if_flags & IFF_UP &&
			(ifp->if_flags & IFF_RUNNING) == 0;
		going_down = (ifp->if_flags & IFF_UP) == 0 &&
			ifp->if_flags & IFF_RUNNING;
		newmode = ifp->if_flags & (IFF_AUTO | IFF_PASSIVE);
		if (newmode == (IFF_AUTO | IFF_PASSIVE)) {
			/* sanity */
			newmode = IFF_PASSIVE;
			ifp->if_flags &= ~IFF_AUTO;
		}

		if (ifp->if_flags & IFF_UP) {
			sp->pp_flags |= PP_ADMIN_UP;
		} else {
			sp->pp_flags &= ~PP_ADMIN_UP;
		}

		if (going_up || going_down) {
			sp->lcp.reestablish = false;
			sppp_wq_add(sp->wq_cp, &sp->scp[IDX_LCP].work_close);
		}
		if (going_up && newmode == 0) {
			/* neither auto-dial nor passive */
			ifp->if_flags |= IFF_RUNNING;
			sppp_wq_add(sp->wq_cp,
			    &sp->scp[IDX_LCP].work_open);
		} else if (going_down) {
			SPPP_UNLOCK(sp);
			sppp_flush(ifp);
			SPPP_LOCK(sp, RW_WRITER);

			ifp->if_flags &= ~IFF_RUNNING;
		}
		SPPP_UNLOCK(sp);
		break;

	case SIOCSIFMTU:
		if (ifr->ifr_mtu < PPP_MINMRU ||
		    ifr->ifr_mtu > PP_MTU) {
			error = EINVAL;
			break;
		}

		error = ifioctl_common(ifp, cmd, data);
		if (error == ENETRESET)
			error = 0;

		SPPP_LOCK(sp, RW_WRITER);
		lcp_mru = sp->lcp.mru;
		if (ifp->if_mtu < PP_MTU) {
			sp->lcp.mru = ifp->if_mtu;
		} else {
			sp->lcp.mru = PP_MTU;
		}
		if (lcp_mru != sp->lcp.mru)
			SET(sp->lcp.opts, SPPP_LCP_OPT_MRU);

		if (sp->scp[IDX_LCP].state == STATE_OPENED &&
		    ifp->if_mtu > sp->lcp.their_mru) {
			sp->pp_saved_mtu = ifp->if_mtu;
			ifp->if_mtu = sp->lcp.their_mru;

			SPPP_DLOG(sp, "setting MTU "
			    "from %"PRIu64" bytes to %"PRIu64" bytes\n",
			    sp->pp_saved_mtu, ifp->if_mtu);
		}
		SPPP_UNLOCK(sp);
		break;

	case SIOCGIFMTU:
		if ((error = ifioctl_common(ifp, cmd, data)) == ENETRESET)
			error = 0;
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		break;

	case SPPPSETAUTHCFG:
	case SPPPSETLCPCFG:
	case SPPPSETNCPCFG:
	case SPPPSETIDLETO:
	case SPPPSETAUTHFAILURE:
	case SPPPSETDNSOPTS:
	case SPPPSETKEEPALIVE:
#if defined(COMPAT_50) || defined(MODULAR)
	case __SPPPSETIDLETO50:
	case __SPPPSETKEEPALIVE50:
#endif /* COMPAT_50 || MODULAR */
		error = kauth_authorize_network(l->l_cred,
		    KAUTH_NETWORK_INTERFACE,
		    KAUTH_REQ_NETWORK_INTERFACE_SETPRIV, ifp, (void *)cmd,
		    NULL);
		if (error)
			break;
		error = sppp_params(sp, cmd, data);
		break;

	case SPPPGETAUTHCFG:
	case SPPPGETLCPCFG:
	case SPPPGETNCPCFG:
	case SPPPGETAUTHFAILURES:
		error = kauth_authorize_network(l->l_cred,
		    KAUTH_NETWORK_INTERFACE,
		    KAUTH_REQ_NETWORK_INTERFACE_GETPRIV, ifp, (void *)cmd,
		    NULL);
		if (error)
			break;
		error = sppp_params(sp, cmd, data);
		break;

	case SPPPGETSTATUS:
	case SPPPGETSTATUSNCP:
	case SPPPGETIDLETO:
	case SPPPGETDNSOPTS:
	case SPPPGETDNSADDRS:
	case SPPPGETKEEPALIVE:
#if defined(COMPAT_50) || defined(MODULAR)
	case __SPPPGETIDLETO50:
	case __SPPPGETKEEPALIVE50:
#endif /* COMPAT_50 || MODULAR */
	case SPPPGETLCPSTATUS:
	case SPPPGETIPCPSTATUS:
	case SPPPGETIPV6CPSTATUS:
		error = sppp_params(sp, cmd, data);
		break;

	default:
		error = ifioctl_common(ifp, cmd, data);
		break;
	}
	splx(s);
	return (error);
}

/*
 * PPP protocol implementation.
 */

/*
 * Send PPP control protocol packet.
 */
static void
sppp_cp_send(struct sppp *sp, u_short proto, u_char type,
	     u_char ident, u_short len, void *data)
{
	struct ifnet *ifp;
	struct lcp_header *lh;
	struct mbuf *m;
	size_t pkthdrlen;

	KASSERT(SPPP_WLOCKED(sp));

	ifp = &sp->pp_if;
	pkthdrlen = (sp->pp_flags & PP_NOFRAMING) ? 2 : PPP_HEADER_LEN;

	if (len > MHLEN - pkthdrlen - LCP_HEADER_LEN)
		len = MHLEN - pkthdrlen - LCP_HEADER_LEN;
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (! m) {
		return;
	}
	m->m_pkthdr.len = m->m_len = pkthdrlen + LCP_HEADER_LEN + len;
	m_reset_rcvif(m);

	if (sp->pp_flags & PP_NOFRAMING) {
		*mtod(m, uint16_t *) = htons(proto);
		lh = (struct lcp_header *)(mtod(m, uint8_t *) + 2);
	} else {
		struct ppp_header *h;
		h = mtod(m, struct ppp_header *);
		h->address = PPP_ALLSTATIONS;        /* broadcast address */
		h->control = PPP_UI;                 /* Unnumbered Info */
		h->protocol = htons(proto);         /* Link Control Protocol */
		lh = (struct lcp_header *)(h + 1);
	}
	lh->type = type;
	lh->ident = ident;
	lh->len = htons(LCP_HEADER_LEN + len);
	if (len)
		memcpy(lh + 1, data, len);

	if (sppp_debug_enabled(sp)) {
		char pbuf[SPPP_PROTO_NAMELEN];
		char tbuf[SPPP_CPTYPE_NAMELEN];
		const char *pname, *cpname;

		pname = sppp_proto_name(pbuf, sizeof(pbuf), proto);
		cpname = sppp_cp_type_name(tbuf, sizeof(tbuf), lh->type);
		SPPP_LOG(sp, LOG_DEBUG, "%s output <%s id=0x%x len=%d",
		    pname, cpname, lh->ident, ntohs(lh->len));
		if (len)
			sppp_print_bytes((u_char *)(lh + 1), len);
		addlog(">\n");
	}
	if (IF_QFULL(&sp->pp_cpq)) {
		IF_DROP(&sp->pp_fastq);
		IF_DROP(&ifp->if_snd);
		m_freem(m);
		if_statinc(ifp, if_oerrors);
		return;
	}

	if_statadd(ifp, if_obytes, m->m_pkthdr.len + sp->pp_framebytes);
	IF_ENQUEUE(&sp->pp_cpq, m);

	if (! (ifp->if_flags & IFF_OACTIVE)) {
		SPPP_UNLOCK(sp);
		if_start_lock(ifp);
		SPPP_LOCK(sp, RW_WRITER);
	}
}

static void
sppp_cp_to_lcp(void *xsp)
{
	struct sppp *sp = xsp;

	sppp_wq_add(sp->wq_cp, &sp->scp[IDX_LCP].work_to);
}

static void
sppp_cp_to_ipcp(void *xsp)
{
	struct sppp *sp = xsp;

	sppp_wq_add(sp->wq_cp, &sp->scp[IDX_IPCP].work_to);
}

static void
sppp_cp_to_ipv6cp(void *xsp)
{
	struct sppp *sp = xsp;

	sppp_wq_add(sp->wq_cp, &sp->scp[IDX_IPV6CP].work_to);
}

static void
sppp_cp_to_pap(void *xsp)
{
	struct sppp *sp = xsp;

	sppp_wq_add(sp->wq_cp, &sp->scp[IDX_PAP].work_to);
}

static void
sppp_cp_to_chap(void *xsp)
{
	struct sppp *sp = xsp;

	sppp_wq_add(sp->wq_cp, &sp->scp[IDX_CHAP].work_to);
}

static void
sppp_cp_init(const struct cp *cp, struct sppp *sp)
{
	struct sppp_cp *scp;
	typedef void (*sppp_co_cb_t)(void *);
	static const sppp_co_cb_t to_cb[IDX_COUNT] = {
		[IDX_LCP] = sppp_cp_to_lcp,
		[IDX_IPCP] = sppp_cp_to_ipcp,
		[IDX_IPV6CP] = sppp_cp_to_ipv6cp,
		[IDX_PAP] = sppp_cp_to_pap,
		[IDX_CHAP] = sppp_cp_to_chap,
	};

	scp = &sp->scp[cp->protoidx];
	scp->state = STATE_INITIAL;
	scp->fail_counter = 0;
	scp->seq = 0;
	scp->rseq = 0;

	SPPP_WQ_SET(&scp->work_up, cp->Up, cp);
	SPPP_WQ_SET(&scp->work_down, cp->Down,  cp);
	SPPP_WQ_SET(&scp->work_open, cp->Open, cp);
	SPPP_WQ_SET(&scp->work_close, cp->Close, cp);
	SPPP_WQ_SET(&scp->work_to, cp->TO, cp);
	SPPP_WQ_SET(&scp->work_rcr, sppp_rcr_event, cp);
	SPPP_WQ_SET(&scp->work_rca, sppp_rca_event, cp);
	SPPP_WQ_SET(&scp->work_rcn, sppp_rcn_event, cp);
	SPPP_WQ_SET(&scp->work_rtr, sppp_rtr_event, cp);
	SPPP_WQ_SET(&scp->work_rta, sppp_rta_event, cp);
	SPPP_WQ_SET(&scp->work_rxj, sppp_rxj_event, cp);

	callout_init(&scp->ch, CALLOUT_MPSAFE);
	callout_setfunc(&scp->ch, to_cb[cp->protoidx], sp);
}

static void
sppp_cp_fini(const struct cp *cp, struct sppp *sp)
{
	struct sppp_cp *scp;
	scp = &sp->scp[cp->protoidx];

	sppp_wq_wait(sp->wq_cp, &scp->work_up);
	sppp_wq_wait(sp->wq_cp, &scp->work_down);
	sppp_wq_wait(sp->wq_cp, &scp->work_open);
	sppp_wq_wait(sp->wq_cp, &scp->work_close);
	sppp_wq_wait(sp->wq_cp, &scp->work_to);
	sppp_wq_wait(sp->wq_cp, &scp->work_rcr);
	sppp_wq_wait(sp->wq_cp, &scp->work_rca);
	sppp_wq_wait(sp->wq_cp, &scp->work_rcn);
	sppp_wq_wait(sp->wq_cp, &scp->work_rtr);
	sppp_wq_wait(sp->wq_cp, &scp->work_rta);
	sppp_wq_wait(sp->wq_cp, &scp->work_rxj);

	callout_halt(&scp->ch, NULL);
	callout_destroy(&scp->ch);

	if (scp->mbuf_confreq != NULL) {
		m_freem(scp->mbuf_confreq);
		scp->mbuf_confreq = NULL;
	}
	if (scp->mbuf_confnak != NULL) {
		m_freem(scp->mbuf_confnak);
		scp->mbuf_confnak = NULL;
	}
}

/*
 * Handle incoming PPP control protocol packets.
 */
static void
sppp_cp_input(const struct cp *cp, struct sppp *sp, struct mbuf *m)
{
	struct ifnet *ifp;
	struct lcp_header *h;
	struct sppp_cp *scp;
	int printlen, len = m->m_pkthdr.len;
	u_char *p;
	uint32_t u32;
	char tbuf[SPPP_CPTYPE_NAMELEN];
	const char *cpname;
	bool debug;

	SPPP_LOCK(sp, RW_WRITER);

	ifp = &sp->pp_if;
	debug = sppp_debug_enabled(sp);
	scp = &sp->scp[cp->protoidx];

	if (len < 4) {
		SPPP_DLOG(sp, "%s invalid packet length: %d bytes\n",
		    cp->name, len);
		goto out;
	}
	h = mtod(m, struct lcp_header *);
	if (debug) {
		printlen = ntohs(h->len);
		cpname = sppp_cp_type_name(tbuf, sizeof(tbuf), h->type);
		SPPP_LOG(sp, LOG_DEBUG, "%s input(%s): <%s id=0x%x len=%d",
		    cp->name, sppp_state_name(scp->state),
		    cpname, h->ident, printlen);
		if (len < printlen)
			printlen = len;
		if (printlen > 4)
			sppp_print_bytes((u_char *)(h + 1), printlen - 4);
		addlog(">\n");
	}
	if (len > ntohs(h->len))
		len = ntohs(h->len);
	p = (u_char *)(h + 1);
	switch (h->type) {
	case CONF_REQ:
		if (len < 4) {
			SPPP_DLOG(sp,"%s invalid conf-req length %d\n",
			    cp->name, len);
			if_statinc(ifp, if_ierrors);
			break;
		}

		scp->rcr_type = CP_RCR_NONE;
		scp->rconfid = h->ident;
		if (scp->mbuf_confreq != NULL) {
			m_freem(scp->mbuf_confreq);
		}
		scp->mbuf_confreq = m;
		m = NULL;
		sppp_wq_add(sp->wq_cp, &scp->work_rcr);
		break;
	case CONF_ACK:
		if (h->ident != scp->confid) {
			SPPP_DLOG(sp, "%s id mismatch 0x%x != 0x%x\n",
			    cp->name, h->ident, scp->confid);
			if_statinc(ifp, if_ierrors);
			break;
		}
		sppp_wq_add(sp->wq_cp, &scp->work_rca);
		break;
	case CONF_NAK:
	case CONF_REJ:
		if (h->ident != scp->confid) {
			SPPP_DLOG(sp, "%s id mismatch 0x%x != 0x%x\n",
			    cp->name, h->ident, scp->confid);
			if_statinc(ifp, if_ierrors);
			break;
		}

		if (scp->mbuf_confnak) {
			m_freem(scp->mbuf_confnak);
		}
		scp->mbuf_confnak = m;
		m = NULL;
		sppp_wq_add(sp->wq_cp, &scp->work_rcn);
		break;
	case TERM_REQ:
		scp->rseq = h->ident;
		sppp_wq_add(sp->wq_cp, &scp->work_rtr);
		break;
	case TERM_ACK:
		if (h->ident != scp->confid &&
		    h->ident != scp->seq) {
			SPPP_DLOG(sp, "%s id mismatch "
			    "0x%x != 0x%x and 0x%x != %0lx\n",
			    cp->name, h->ident, scp->confid,
			    h->ident, scp->seq);
			if_statinc(ifp, if_ierrors);
			break;
		}

		sppp_wq_add(sp->wq_cp, &scp->work_rta);
		break;
	case CODE_REJ:
		/* XXX catastrophic rejects (RXJ-) aren't handled yet. */
		cpname = sppp_cp_type_name(tbuf, sizeof(tbuf), h->type);
		SPPP_LOG(sp, LOG_INFO, "%s: ignoring RXJ (%s) for code ?, "
		    "danger will robinson\n", cp->name, cpname);
		sppp_wq_add(sp->wq_cp, &scp->work_rxj);
		break;
	case PROTO_REJ:
	    {
		int catastrophic;
		const struct cp *upper;
		int i;
		uint16_t proto;

		catastrophic = 0;
		upper = NULL;
		proto = p[0] << 8 | p[1];
		for (i = 0; i < IDX_COUNT; i++) {
			if (cps[i]->proto == proto) {
				upper = cps[i];
				break;
			}
		}
		if (upper == NULL)
			catastrophic++;

		if (debug) {
			cpname = sppp_cp_type_name(tbuf, sizeof(tbuf), h->type);
			SPPP_LOG(sp, LOG_INFO,
			    "%s: RXJ%c (%s) for proto 0x%x (%s/%s)\n",
			    cp->name, catastrophic ? '-' : '+',
			    cpname, proto, upper ? upper->name : "unknown",
			    upper ? sppp_state_name(sp->scp[upper->protoidx].state) : "?");
		}

		/*
		 * if we got RXJ+ against conf-req, the peer does not implement
		 * this particular protocol type.  terminate the protocol.
		 */
		if (upper && !catastrophic) {
			if (sp->scp[upper->protoidx].state == STATE_REQ_SENT) {
				sppp_wq_add(sp->wq_cp,
				    &sp->scp[upper->protoidx].work_close);
				break;
			}
		}
		sppp_wq_add(sp->wq_cp, &scp->work_rxj);
		break;
	    }
	case DISC_REQ:
		if (cp->proto != PPP_LCP)
			goto illegal;
		/* Discard the packet. */
		break;
	case ECHO_REQ:
		if (cp->proto != PPP_LCP)
			goto illegal;
		if (scp->state != STATE_OPENED) {
			SPPP_DLOG(sp, "lcp echo req but lcp closed\n");
			if_statinc(ifp, if_ierrors);
			break;
		}
		if (len < 8) {
			SPPP_DLOG(sp, "invalid lcp echo request "
			       "packet length: %d bytes\n", len);
			break;
		}
		memcpy(&u32, h + 1, sizeof u32);
		if (ntohl(u32) == sp->lcp.magic) {
			/* Line loopback mode detected. */
			SPPP_DLOG(sp, "loopback\n");

			if (sp->pp_flags & PP_LOOPBACK_IFDOWN) {
				sp->pp_flags |= PP_LOOPBACK;
				sppp_wq_add(sp->wq_cp,
				    &sp->work_ifdown);
			}

			/* Shut down the PPP link. */
			sppp_wq_add(sp->wq_cp,
			    &sp->scp[IDX_LCP].work_close);
			sppp_wq_add(sp->wq_cp,
			    &sp->scp[IDX_LCP].work_open);
			break;
		}
		u32 = htonl(sp->lcp.magic);
		memcpy(h + 1, &u32, sizeof u32);
		SPPP_DLOG(sp, "got lcp echo req, sending echo rep\n");
		sppp_cp_send(sp, PPP_LCP, ECHO_REPLY, h->ident, len - 4,
		    h + 1);
		break;
	case ECHO_REPLY:
		if (cp->proto != PPP_LCP)
			goto illegal;
		if (h->ident != sp->lcp.echoid) {
			if_statinc(ifp, if_ierrors);
			break;
		}
		if (len < 8) {
			SPPP_DLOG(sp, "lcp invalid echo reply "
			    "packet length: %d bytes\n", len);
			break;
		}
		SPPP_DLOG(sp, "lcp got echo rep\n");
		memcpy(&u32, h + 1, sizeof u32);
		if (ntohl(u32) != sp->lcp.magic)
			sp->pp_alivecnt = 0;
		break;
	default:
		/* Unknown packet type -- send Code-Reject packet. */
	  illegal:
		SPPP_DLOG(sp, "%s send code-rej for 0x%x\n",
		    cp->name, h->type);
		sppp_cp_send(sp, cp->proto, CODE_REJ,
		    ++scp->seq, m->m_pkthdr.len, h);
		if_statinc(ifp, if_ierrors);
	}

out:
	SPPP_UNLOCK(sp);
	if (m != NULL)
		m_freem(m);
}

/*
 * The generic part of all Up/Down/Open/Close/TO event handlers.
 * Basically, the state transition handling in the automaton.
 */
static void
sppp_up_event(struct sppp *sp, void *xcp)
{
	const struct cp *cp = xcp;

	KASSERT(SPPP_WLOCKED(sp));
	KASSERT(!cpu_softintr_p());

	if ((cp->flags & CP_AUTH) != 0 &&
	    sppp_auth_role(cp, sp) == SPPP_AUTH_NOROLE)
		return;

	SPPP_DLOG(sp, "%s up(%s)\n", cp->name,
	    sppp_state_name(sp->scp[cp->protoidx].state));

	switch (sp->scp[cp->protoidx].state) {
	case STATE_INITIAL:
		sppp_cp_change_state(cp, sp, STATE_CLOSED);
		break;
	case STATE_STARTING:
		sp->scp[cp->protoidx].rst_counter = sp->lcp.max_configure;
		(cp->scr)(sp);
		sppp_cp_change_state(cp, sp, STATE_REQ_SENT);
		break;
	default:
		SPPP_LOG(sp, LOG_DEBUG,
		    "%s illegal up in state %s\n", cp->name,
		    sppp_state_name(sp->scp[cp->protoidx].state));
	}
}

static void
sppp_down_event(struct sppp *sp, void *xcp)
{
	const struct cp *cp = xcp;

	KASSERT(SPPP_WLOCKED(sp));
	KASSERT(!cpu_softintr_p());

	if ((cp->flags & CP_AUTH) != 0 &&
	    sppp_auth_role(cp, sp) == SPPP_AUTH_NOROLE)
		return;

	SPPP_DLOG(sp, "%s down(%s)\n", cp->name,
	    sppp_state_name(sp->scp[cp->protoidx].state));

	switch (sp->scp[cp->protoidx].state) {
	case STATE_CLOSED:
	case STATE_CLOSING:
		sppp_cp_change_state(cp, sp, STATE_INITIAL);
		break;
	case STATE_STOPPED:
		(cp->tls)(cp, sp);
		/* fall through */
	case STATE_STOPPING:
	case STATE_REQ_SENT:
	case STATE_ACK_RCVD:
	case STATE_ACK_SENT:
		sppp_cp_change_state(cp, sp, STATE_STARTING);
		break;
	case STATE_OPENED:
		(cp->tld)(sp);
		sppp_cp_change_state(cp, sp, STATE_STARTING);
		break;
	default:
		/*
		 * a down event may be caused regardless
		 * of state just in LCP case.
		 */
		if (cp->proto == PPP_LCP)
			break;

		SPPP_LOG(sp, LOG_DEBUG,
		    "%s illegal down in state %s\n", cp->name,
		    sppp_state_name(sp->scp[cp->protoidx].state));
	}
}

static void
sppp_open_event(struct sppp *sp, void *xcp)
{
	const struct cp *cp = xcp;

	KASSERT(SPPP_WLOCKED(sp));
	KASSERT(!cpu_softintr_p());

	if ((cp->flags & CP_AUTH) != 0 &&
	    sppp_auth_role(cp, sp) == SPPP_AUTH_NOROLE)
		return;

	SPPP_DLOG(sp, "%s open(%s)\n", cp->name,
	    sppp_state_name(sp->scp[cp->protoidx].state));

	switch (sp->scp[cp->protoidx].state) {
	case STATE_INITIAL:
		sppp_cp_change_state(cp, sp, STATE_STARTING);
		(cp->tls)(cp, sp);
		break;
	case STATE_STARTING:
		break;
	case STATE_CLOSED:
		sp->scp[cp->protoidx].rst_counter = sp->lcp.max_configure;
		sp->lcp.protos |= (1 << cp->protoidx);
		(cp->scr)(sp);
		sppp_cp_change_state(cp, sp, STATE_REQ_SENT);
		break;
	case STATE_STOPPED:
	case STATE_STOPPING:
	case STATE_REQ_SENT:
	case STATE_ACK_RCVD:
	case STATE_ACK_SENT:
	case STATE_OPENED:
		break;
	case STATE_CLOSING:
		sppp_cp_change_state(cp, sp, STATE_STOPPING);
		break;
	}
}

static void
sppp_close_event(struct sppp *sp, void *xcp)
{
	const struct cp *cp = xcp;

	KASSERT(SPPP_WLOCKED(sp));
	KASSERT(!cpu_softintr_p());

	if ((cp->flags & CP_AUTH) != 0 &&
	    sppp_auth_role(cp, sp) == SPPP_AUTH_NOROLE)
		return;

	SPPP_DLOG(sp, "%s close(%s)\n", cp->name,
	    sppp_state_name(sp->scp[cp->protoidx].state));

	switch (sp->scp[cp->protoidx].state) {
	case STATE_INITIAL:
	case STATE_CLOSED:
	case STATE_CLOSING:
		break;
	case STATE_STARTING:
		sppp_cp_change_state(cp, sp, STATE_INITIAL);
		(cp->tlf)(cp, sp);
		break;
	case STATE_STOPPED:
		sppp_cp_change_state(cp, sp, STATE_CLOSED);
		break;
	case STATE_STOPPING:
		sppp_cp_change_state(cp, sp, STATE_CLOSING);
		break;
	case STATE_OPENED:
		(cp->tld)(sp);
		/* fall through */
	case STATE_REQ_SENT:
	case STATE_ACK_RCVD:
	case STATE_ACK_SENT:
		sp->scp[cp->protoidx].rst_counter = sp->lcp.max_terminate;
		if ((cp->flags & CP_AUTH) == 0) {
			sppp_cp_send(sp, cp->proto, TERM_REQ,
			    ++sp->scp[cp->protoidx].seq, 0, 0);
		}
		sppp_cp_change_state(cp, sp, STATE_CLOSING);
		break;
	}
}

static void
sppp_to_event(struct sppp *sp, void *xcp)
{
	const struct cp *cp = xcp;
	int s;

	KASSERT(SPPP_WLOCKED(sp));
	KASSERT(!cpu_softintr_p());

	s = splnet();

	SPPP_DLOG(sp, "%s TO(%s) rst_counter = %d\n", cp->name,
	    sppp_state_name(sp->scp[cp->protoidx].state),
	    sp->scp[cp->protoidx].rst_counter);

	if (--sp->scp[cp->protoidx].rst_counter < 0)
		/* TO- event */
		switch (sp->scp[cp->protoidx].state) {
		case STATE_CLOSING:
			sppp_cp_change_state(cp, sp, STATE_CLOSED);
			(cp->tlf)(cp, sp);
			break;
		case STATE_STOPPING:
			sppp_cp_change_state(cp, sp, STATE_STOPPED);
			(cp->tlf)(cp, sp);
			break;
		case STATE_REQ_SENT:
		case STATE_ACK_RCVD:
		case STATE_ACK_SENT:
			sppp_cp_change_state(cp, sp, STATE_STOPPED);
			(cp->tlf)(cp, sp);
			break;
		}
	else
		/* TO+ event */
		switch (sp->scp[cp->protoidx].state) {
		case STATE_CLOSING:
		case STATE_STOPPING:
			if ((cp->flags & CP_AUTH) == 0) {
				sppp_cp_send(sp, cp->proto, TERM_REQ,
				    ++sp->scp[cp->protoidx].seq, 0, 0);
			}
			callout_schedule(&sp->scp[cp->protoidx].ch, sp->lcp.timeout);
			break;
		case STATE_REQ_SENT:
		case STATE_ACK_RCVD:
			(cp->scr)(sp);
			/* sppp_cp_change_state() will restart the timer */
			sppp_cp_change_state(cp, sp, STATE_REQ_SENT);
			break;
		case STATE_ACK_SENT:
			(cp->scr)(sp);
			callout_schedule(&sp->scp[cp->protoidx].ch, sp->lcp.timeout);
			break;
		}

	splx(s);
}
static void
sppp_rcr_update_state(const struct cp *cp, struct sppp *sp,
    enum cp_rcr_type type, uint8_t ident, size_t msglen, void *msg)
{
	struct ifnet *ifp;
	u_char ctype;

	ifp = &sp->pp_if;

	if (type == CP_RCR_ERR) {
		/* parse error, shut down */
		sppp_wq_add(sp->wq_cp, &sp->scp[IDX_LCP].work_close);
		sppp_wq_add(sp->wq_cp, &sp->scp[IDX_LCP].work_open);
	} else if (type == CP_RCR_ACK) {
		/* RCR+ event */
		ctype = CONF_ACK;
		switch (sp->scp[cp->protoidx].state) {
		case STATE_OPENED:
			sppp_cp_change_state(cp, sp, STATE_ACK_SENT);
			cp->tld(sp);
			cp->scr(sp);
			cp->screply(cp, sp, ctype, ident, msglen, msg);
			break;
		case STATE_REQ_SENT:
			sppp_cp_change_state(cp, sp, STATE_ACK_SENT);
			/* fall through */
		case STATE_ACK_SENT:
			cp->screply(cp, sp, ctype, ident, msglen, msg);
			break;
		case STATE_STOPPED:
			sppp_cp_change_state(cp, sp, STATE_ACK_SENT);
			cp->scr(sp);
			cp->screply(cp, sp, ctype, ident, msglen, msg);
			break;
		case STATE_ACK_RCVD:
			sppp_cp_change_state(cp, sp, STATE_OPENED);
			SPPP_DLOG(sp, "%s tlu\n", cp->name);
			cp->tlu(sp);
			cp->screply(cp, sp, ctype, ident, msglen, msg);
			break;
		case STATE_CLOSING:
		case STATE_STOPPING:
			break;
		case STATE_CLOSED:
			if ((cp->flags & CP_AUTH) == 0) {
				sppp_cp_send(sp, cp->proto, TERM_ACK,
				    ident, 0, 0);
			}
			break;
		default:
			SPPP_LOG(sp, LOG_DEBUG,
			    "%s illegal RCR+ in state %s\n", cp->name,
			    sppp_state_name(sp->scp[cp->protoidx].state));
			if_statinc(ifp, if_ierrors);
		}
	} else if (type == CP_RCR_NAK || type == CP_RCR_REJ) {
		ctype = type == CP_RCR_NAK ? CONF_NAK : CONF_REJ;
		/* RCR- event */
		switch (sp->scp[cp->protoidx].state) {
		case STATE_OPENED:
			sppp_cp_change_state(cp, sp, STATE_REQ_SENT);
			cp->tld(sp);
			cp->scr(sp);
			cp->screply(cp, sp, ctype, ident, msglen, msg);
			break;
		case STATE_ACK_SENT:
			sppp_cp_change_state(cp, sp, STATE_REQ_SENT);
			/* fall through */
		case STATE_REQ_SENT:
			cp->screply(cp, sp, ctype, ident, msglen, msg);
			break;
		case STATE_STOPPED:
			sppp_cp_change_state(cp, sp, STATE_REQ_SENT);
			cp->scr(sp);
			cp->screply(cp, sp, ctype, ident, msglen, msg);
			break;
		case STATE_ACK_RCVD:
			sppp_cp_change_state(cp, sp, STATE_ACK_RCVD);
			cp->screply(cp, sp, ctype, ident, msglen, msg);
			break;
		case STATE_CLOSING:
		case STATE_STOPPING:
			break;
		case STATE_CLOSED:
			sppp_cp_change_state(cp, sp, STATE_CLOSED);
			if ((cp->flags & CP_AUTH) == 0) {
				sppp_cp_send(sp, cp->proto, TERM_ACK,
				    ident, 0, 0);
			}
			break;
		default:
			SPPP_LOG(sp, LOG_DEBUG,
			    "%s illegal RCR- in state %s\n", cp->name,
			    sppp_state_name(sp->scp[cp->protoidx].state));
			if_statinc(ifp, if_ierrors);
		}
	}
}

static void
sppp_rcr_event(struct sppp *sp, void *xcp)
{
	const struct cp *cp = xcp;
	struct sppp_cp *scp;
	struct lcp_header *h;
	struct mbuf *m;
	enum cp_rcr_type type;
	size_t len;
	uint8_t *buf;
	size_t blen, rlen;
	uint8_t ident;

	KASSERT(!cpu_softintr_p());

	scp = &sp->scp[cp->protoidx];

	if (cp->parse_confreq != NULL) {
		m = scp->mbuf_confreq;
		if (m == NULL)
			return;
		scp->mbuf_confreq = NULL;

		h = mtod(m, struct lcp_header *);
		if (h->type != CONF_REQ) {
			m_freem(m);
			return;
		}

		ident = h->ident;
		len = MIN(m->m_pkthdr.len, ntohs(h->len));

		type = (cp->parse_confreq)(sp, h, len,
		    &buf, &blen, &rlen);
		m_freem(m);
	} else {
		/* mbuf_cofreq is already parsed and freed */
		type = scp->rcr_type;
		ident = scp->rconfid;
		buf = NULL;
		blen = rlen = 0;
	}

	sppp_rcr_update_state(cp, sp, type, ident, rlen, (void *)buf);

	if (buf != NULL)
		kmem_free(buf, blen);
}

static void
sppp_rca_event(struct sppp *sp, void *xcp)
{
	struct ifnet *ifp;
	const struct cp *cp = xcp;

	KASSERT(!cpu_softintr_p());

	ifp = &sp->pp_if;

	switch (sp->scp[cp->protoidx].state) {
	case STATE_CLOSED:
	case STATE_STOPPED:
		if ((cp->flags & CP_AUTH) == 0) {
			sppp_cp_send(sp, cp->proto, TERM_ACK,
			    sp->scp[cp->protoidx].rconfid, 0, 0);
		}
		break;
	case STATE_CLOSING:
	case STATE_STOPPING:
		break;
	case STATE_REQ_SENT:
		sp->scp[cp->protoidx].rst_counter = sp->lcp.max_configure;
		sppp_cp_change_state(cp, sp, STATE_ACK_RCVD);
		break;
	case STATE_OPENED:
		(cp->tld)(sp);
		/* fall through */
	case STATE_ACK_RCVD:
		(cp->scr)(sp);
		sppp_cp_change_state(cp, sp, STATE_REQ_SENT);
		break;
	case STATE_ACK_SENT:
		sppp_cp_change_state(cp, sp, STATE_OPENED);
		sp->scp[cp->protoidx].rst_counter = sp->lcp.max_configure;
		SPPP_DLOG(sp, "%s tlu\n", cp->name);
		(cp->tlu)(sp);
		break;
	default:
		SPPP_LOG(sp, LOG_DEBUG,
		    "%s illegal RCA in state %s\n", cp->name,
		    sppp_state_name(sp->scp[cp->protoidx].state));
		if_statinc(ifp, if_ierrors);
	}
}

static void
sppp_rcn_event(struct sppp *sp, void *xcp)
{
	const struct cp *cp = xcp;
	struct sppp_cp *scp;
	struct lcp_header *h;
	struct mbuf *m;
	struct ifnet *ifp = &sp->pp_if;
	size_t len;

	KASSERT(!cpu_softintr_p());

	scp = &sp->scp[cp->protoidx];
	m = scp->mbuf_confnak;
	if (m == NULL)
		return;
	scp->mbuf_confnak = NULL;

	h = mtod(m, struct lcp_header *);
	len = MIN(m->m_pkthdr.len, ntohs(h->len));

	switch (h->type) {
	case CONF_NAK:
		(cp->parse_confnak)(sp, h, len);
		break;
	case CONF_REJ:
		(cp->parse_confrej)(sp, h, len);
		break;
	default:
		m_freem(m);
		return;
	}

	m_freem(m);

	switch (scp->state) {
	case STATE_CLOSED:
	case STATE_STOPPED:
		if ((cp->flags & CP_AUTH) == 0) {
			sppp_cp_send(sp, cp->proto, TERM_ACK,
			    scp->rconfid, 0, 0);
		}
		break;
	case STATE_REQ_SENT:
	case STATE_ACK_SENT:
		scp->rst_counter = sp->lcp.max_configure;
		(cp->scr)(sp);
		break;
	case STATE_OPENED:
		(cp->tld)(sp);
		/* fall through */
	case STATE_ACK_RCVD:
		sppp_cp_change_state(cp, sp, STATE_ACK_SENT);
		(cp->scr)(sp);
		break;
	case STATE_CLOSING:
	case STATE_STOPPING:
		break;
	default:
		SPPP_LOG(sp, LOG_DEBUG, "%s illegal RCN in state %s\n",
		    cp->name, sppp_state_name(scp->state));
		if_statinc(ifp, if_ierrors);
	}
}

static void
sppp_rtr_event(struct sppp *sp, void *xcp)
{
	struct ifnet *ifp;
	const struct cp *cp = xcp;

	KASSERT(!cpu_softintr_p());

	ifp = &sp->pp_if;

	switch (sp->scp[cp->protoidx].state) {
	case STATE_ACK_RCVD:
	case STATE_ACK_SENT:
		sppp_cp_change_state(cp, sp, STATE_REQ_SENT);
		break;
	case STATE_CLOSED:
	case STATE_STOPPED:
	case STATE_CLOSING:
	case STATE_STOPPING:
	case STATE_REQ_SENT:
		break;
	case STATE_OPENED:
		(cp->tld)(sp);
		sp->scp[cp->protoidx].rst_counter = 0;
		sppp_cp_change_state(cp, sp, STATE_STOPPING);
		break;
	default:
		SPPP_LOG(sp, LOG_DEBUG, "%s illegal RTR in state %s\n",
		    cp->name,
		    sppp_state_name(sp->scp[cp->protoidx].state));
		if_statinc(ifp, if_ierrors);
		return;
	}

	/* Send Terminate-Ack packet. */
	SPPP_DLOG(sp, "%s send terminate-ack\n", cp->name);
	if ((cp->flags & CP_AUTH) == 0) {
		sppp_cp_send(sp, cp->proto, TERM_ACK,
		    sp->scp[cp->protoidx].rseq, 0, 0);
	}
}

static void
sppp_rta_event(struct sppp *sp, void *xcp)
{
	const struct cp *cp = xcp;
	struct ifnet *ifp = &sp->pp_if;

	KASSERT(!cpu_softintr_p());

	switch (sp->scp[cp->protoidx].state) {
	case STATE_CLOSED:
	case STATE_STOPPED:
	case STATE_REQ_SENT:
	case STATE_ACK_SENT:
		break;
	case STATE_CLOSING:
		sppp_cp_change_state(cp, sp, STATE_CLOSED);
		(cp->tlf)(cp, sp);
		break;
	case STATE_STOPPING:
		sppp_cp_change_state(cp, sp, STATE_STOPPED);
		(cp->tlf)(cp, sp);
		break;
	case STATE_ACK_RCVD:
		sppp_cp_change_state(cp, sp, STATE_REQ_SENT);
		break;
	case STATE_OPENED:
		(cp->tld)(sp);
		(cp->scr)(sp);
		sppp_cp_change_state(cp, sp, STATE_ACK_RCVD);
		break;
	default:
		SPPP_LOG(sp, LOG_DEBUG, "%s illegal RTA in state %s\n",
		    cp->name,  sppp_state_name(sp->scp[cp->protoidx].state));
		if_statinc(ifp, if_ierrors);
	}
}

static void
sppp_rxj_event(struct sppp *sp, void *xcp)
{
	const struct cp *cp = xcp;
	struct ifnet *ifp = &sp->pp_if;

	KASSERT(!cpu_softintr_p());

	/* XXX catastrophic rejects (RXJ-) aren't handled yet. */
	switch (sp->scp[cp->protoidx].state) {
	case STATE_CLOSED:
	case STATE_STOPPED:
	case STATE_REQ_SENT:
	case STATE_ACK_SENT:
	case STATE_CLOSING:
	case STATE_STOPPING:
	case STATE_OPENED:
		break;
	case STATE_ACK_RCVD:
		sppp_cp_change_state(cp, sp, STATE_REQ_SENT);
		break;
	default:
		SPPP_LOG(sp, LOG_DEBUG, "%s illegal RXJ- in state %s\n",
		    cp->name,  sppp_state_name(sp->scp[cp->protoidx].state));
		if_statinc(ifp, if_ierrors);
	}
}

/*
 * Change the state of a control protocol in the state automaton.
 * Takes care of starting/stopping the restart timer.
 */
void
sppp_cp_change_state(const struct cp *cp, struct sppp *sp, int newstate)
{

	KASSERT(SPPP_WLOCKED(sp));

	sp->scp[cp->protoidx].state = newstate;
	callout_stop(&sp->scp[cp->protoidx].ch);
	switch (newstate) {
	case STATE_INITIAL:
	case STATE_STARTING:
	case STATE_CLOSED:
	case STATE_STOPPED:
	case STATE_OPENED:
		break;
	case STATE_CLOSING:
	case STATE_STOPPING:
	case STATE_REQ_SENT:
	case STATE_ACK_RCVD:
	case STATE_ACK_SENT:
		callout_schedule(&sp->scp[cp->protoidx].ch, sp->lcp.timeout);
		break;
	}
}

/*
 *--------------------------------------------------------------------------*
 *                                                                          *
 *                         The LCP implementation.                          *
 *                                                                          *
 *--------------------------------------------------------------------------*
 */
static void
sppp_lcp_init(struct sppp *sp)
{

	KASSERT(SPPP_WLOCKED(sp));

	sppp_cp_init(&lcp, sp);

	SET(sp->lcp.opts, SPPP_LCP_OPT_MAGIC);
	sp->lcp.magic = 0;
	sp->lcp.protos = 0;
	sp->lcp.max_terminate = 2;
	sp->lcp.max_configure = 10;
	sp->lcp.max_failure = 10;
	sp->lcp.tlf_sent = false;

	/*
	 * Initialize counters and timeout values.  Note that we don't
	 * use the 3 seconds suggested in RFC 1661 since we are likely
	 * running on a fast link.  XXX We should probably implement
	 * the exponential backoff option.  Note that these values are
	 * relevant for all control protocols, not just LCP only.
	 */
	sp->lcp.timeout = 1 * hz;
}

static void
sppp_lcp_up(struct sppp *sp, void *xcp)
{
	struct ifnet *ifp;
	const struct cp *cp = xcp;
	int pidx;

	KASSERT(SPPP_WLOCKED(sp));
	ifp = &sp->pp_if;

	pidx = cp->protoidx;
	/* Initialize activity timestamp: opening a connection is an activity */
	sp->pp_last_receive = sp->pp_last_activity = time_uptime;

	/*
	 * If this interface is passive or dial-on-demand, and we are
	 * still in Initial state, it means we've got an incoming
	 * call.  Activate the interface.
	 */
	if ((ifp->if_flags & (IFF_AUTO | IFF_PASSIVE)) != 0) {
		ifp->if_flags |= IFF_RUNNING;
		if (sp->scp[pidx].state == STATE_INITIAL) {
			SPPP_DLOG(sp, "Up event (incoming call)\n");
			sp->pp_flags |= PP_CALLIN;
			sppp_wq_add(sp->wq_cp, &sp->scp[pidx].work_open);
		} else {
			SPPP_DLOG(sp, "Up event\n");
		}
	}

	sppp_up_event(sp, xcp);
}

static void
sppp_lcp_down(struct sppp *sp, void *xcp)
{
	const struct cp *cp = xcp;
	struct ifnet *ifp;
	int pidx;

	KASSERT(SPPP_WLOCKED(sp));
	KASSERT(!cpu_softintr_p());

	ifp = &sp->pp_if;
	pidx = cp->protoidx;
	sppp_down_event(sp, xcp);

	/*
	 * We need to do tls to restart when a down event is caused
	 * by the last tlf.
	 */
	if (sp->scp[pidx].state == STATE_STARTING &&
	    sp->lcp.tlf_sent) {
		cp->tls(cp, sp);
		sp->lcp.tlf_sent = false;
	}

	SPPP_DLOG(sp, "Down event (carrier loss)\n");

	if ((ifp->if_flags & (IFF_AUTO | IFF_PASSIVE)) == 0) {
		if (sp->lcp.reestablish)
			sppp_wq_add(sp->wq_cp, &sp->scp[IDX_LCP].work_open);
	} else {
		sp->pp_flags &= ~PP_CALLIN;
		if (sp->scp[pidx].state != STATE_INITIAL)
			sppp_wq_add(sp->wq_cp, &sp->scp[pidx].work_close);
		ifp->if_flags &= ~IFF_RUNNING;
	}
	sp->scp[pidx].fail_counter = 0;
}

static void
sppp_lcp_open(struct sppp *sp, void *xcp)
{

	KASSERT(SPPP_WLOCKED(sp));
	KASSERT(!cpu_softintr_p());

	sp->lcp.reestablish = false;
	sp->scp[IDX_LCP].fail_counter = 0;

	/* the interface was down while waiting for reconnection */
	if ((sp->pp_flags & PP_ADMIN_UP) == 0)
		return;

	if (sp->pp_if.if_mtu < PP_MTU) {
		sp->lcp.mru = sp->pp_if.if_mtu;
		SET(sp->lcp.opts, SPPP_LCP_OPT_MRU);
	} else {
		sp->lcp.mru = PP_MTU;
	}
	sp->lcp.their_mru = PP_MTU;

	/*
	 * If we are authenticator, negotiate LCP_AUTH
	 */
	if (sp->hisauth.proto != PPP_NOPROTO)
		SET(sp->lcp.opts, SPPP_LCP_OPT_AUTH_PROTO);
	else
		CLR(sp->lcp.opts, SPPP_LCP_OPT_AUTH_PROTO);
	sp->pp_flags &= ~PP_NEEDAUTH;
	sppp_open_event(sp, xcp);
}


/*
 * Analyze a configure request.  Return true if it was agreeable, and
 * caused action sca, false if it has been rejected or nak'ed, and
 * caused action scn.  (The return value is used to make the state
 * transition decision in the state automaton.)
 */
static enum cp_rcr_type
sppp_lcp_confreq(struct sppp *sp, struct lcp_header *h, int origlen,
    uint8_t **msgbuf, size_t *buflen, size_t *msglen)
{
	u_char *buf, *r, *p, l, blen;
	enum cp_rcr_type type;
	int len, rlen;
	uint32_t nmagic;
	u_short authproto;
	char lbuf[SPPP_LCPOPT_NAMELEN];
	bool debug;

	KASSERT(SPPP_WLOCKED(sp));

	debug = sppp_debug_enabled(sp);

	if (origlen < sizeof(*h))
		return CP_RCR_DROP;

	origlen -= sizeof(*h);
	type = CP_RCR_NONE;
	type = 0;

	if (origlen <= 0)
		return CP_RCR_DROP;
	else
		blen = origlen;

	buf = kmem_intr_alloc(blen, KM_NOSLEEP);
	if (buf == NULL)
		return CP_RCR_DROP;

	if (debug)
		SPPP_LOG(sp, LOG_DEBUG, "lcp parse opts:");

	/* pass 1: check for things that need to be rejected */
	p = (void *)(h + 1);
	r = buf;
	rlen = 0;
	for (len = origlen; len > 1; len-= l, p += l) {
		l = p[1];
		if (l == 0)
			break;

		/* Sanity check option length */
		if (l > len) {
			/*
			 * Malicious option - drop immediately.
			 * XXX Maybe we should just RXJ it?
			 */
			if (debug)
				addlog("\n");

			SPPP_LOG(sp, LOG_DEBUG,
			    "received malicious LCP option 0x%02x, "
			    "length 0x%02x, (len: 0x%02x) dropping.\n",
			    p[0], l, len);
			type = CP_RCR_ERR;
			goto end;
		}
		if (debug)
			addlog(" %s", sppp_lcp_opt_name(lbuf, sizeof(lbuf), *p));
		switch (p[0]) {
		case LCP_OPT_MAGIC:
			/* Magic number. */
			/* fall through, both are same length */
		case LCP_OPT_ASYNC_MAP:
			/* Async control character map. */
			if (len >= 6 || l == 6)
				continue;
			if (debug)
				addlog(" [invalid]");
			break;
		case LCP_OPT_MP_EID:
			if (len >= l && l >= 3) {
				switch (p[2]) {
				case 0: if (l==3+ 0) continue;break;
				case 2: if (l==3+ 4) continue;break;
				case 3: if (l==3+ 6) continue;break;
				case 6: if (l==3+16) continue;break;
				case 1: /* FALLTHROUGH */
				case 4: if (l<=3+20) continue;break;
				case 5: if (l<=3+15) continue;break;
				/* XXX should it be default: continue;? */
				}
			}
			if (debug)
				addlog(" [invalid class %d len %d]", p[2], l);
			break;
		case LCP_OPT_MP_SSNHF:
			if (len >= 2 && l == 2) {
				if (debug)
					addlog(" [rej]");
				break;
			}
			if (debug)
				addlog(" [invalid]");
			break;
		case LCP_OPT_MP_MRRU:
			/* Multilink maximum received reconstructed unit */
			/* should be fall through, both are same length */
			/* FALLTHROUGH */
		case LCP_OPT_MRU:
			/* Maximum receive unit. */
			if (len >= 4 && l == 4)
				continue;
			if (debug)
				addlog(" [invalid]");
			break;
		case LCP_OPT_AUTH_PROTO:
			if (len < 4) {
				if (debug)
					addlog(" [invalid]");
				break;
			}
			authproto = (p[2] << 8) + p[3];
			if (authproto == PPP_CHAP && l != 5) {
				if (debug)
					addlog(" [invalid chap len]");
				break;
			}
			if (ISSET(sp->myauth.flags, SPPP_AUTHFLAG_PASSIVEAUTHPROTO)) {
				if (authproto == PPP_PAP || authproto == PPP_CHAP)
					sp->myauth.proto = authproto;
			}
			if (sp->myauth.proto == PPP_NOPROTO) {
				/* we are not configured to do auth */
				if (debug)
					addlog(" [not configured]");
				break;
			}
			/*
			 * Remote want us to authenticate, remember this,
			 * so we stay in SPPP_PHASE_AUTHENTICATE after LCP got
			 * up.
			 */
			sp->pp_flags |= PP_NEEDAUTH;
			continue;
		default:
			/* Others not supported. */
			if (debug)
				addlog(" [rej]");
			break;
		}
		if (rlen + l > blen) {
			if (debug)
				addlog(" [overflow]");
			continue;
		}
		/* Add the option to rejected list. */
		memcpy(r, p, l);
		r += l;
		rlen += l;
	}

	if (rlen > 0) {
		type = CP_RCR_REJ;
		goto end;
	}

	if (debug)
		addlog("\n");

	/*
	 * pass 2: check for option values that are unacceptable and
	 * thus require to be nak'ed.
	 */
	if (debug)
		SPPP_LOG(sp, LOG_DEBUG, "lcp parse opt values:");

	p = (void *)(h + 1);
	r = buf;
	rlen = 0;
	for (len = origlen; len > 0; len -= l, p += l) {
		l = p[1];
		if (l == 0)
			break;

		if (debug)
			addlog(" %s", sppp_lcp_opt_name(lbuf, sizeof(lbuf), *p));
		switch (p[0]) {
		case LCP_OPT_MAGIC:
			/* Magic number -- extract. */
			nmagic = (uint32_t)p[2] << 24 |
				(uint32_t)p[3] << 16 | p[4] << 8 | p[5];
			if (nmagic != sp->lcp.magic) {
				if (debug)
					addlog(" 0x%x", nmagic);
				continue;
			}
			/*
			 * Local and remote magics equal -- loopback?
			 */
			if (sp->pp_loopcnt >= LOOPALIVECNT*5) {
				SPPP_DLOG(sp, "loopback\n");
				sp->pp_loopcnt = 0;

				if (sp->pp_flags & PP_LOOPBACK_IFDOWN) {
					sp->pp_flags |= PP_LOOPBACK;
					sppp_wq_add(sp->wq_cp,
					    &sp->work_ifdown);
				}

				sppp_wq_add(sp->wq_cp,
				    &sp->scp[IDX_LCP].work_close);
				sppp_wq_add(sp->wq_cp,
				    &sp->scp[IDX_LCP].work_open);
			} else {
				if (debug)
					addlog(" [glitch]");
				++sp->pp_loopcnt;
			}
			/*
			 * We negate our magic here, and NAK it.  If
			 * we see it later in an NAK packet, we
			 * suggest a new one.
			 */
			nmagic = ~sp->lcp.magic;
			/* Gonna NAK it. */
			p[2] = nmagic >> 24;
			p[3] = nmagic >> 16;
			p[4] = nmagic >> 8;
			p[5] = nmagic;
			break;

		case LCP_OPT_ASYNC_MAP:
			/*
			 * Async control character map -- just ignore it.
			 *
			 * Quote from RFC 1662, chapter 6:
			 * To enable this functionality, synchronous PPP
			 * implementations MUST always respond to the
			 * Async-Control-Character-Map Configuration
			 * Option with the LCP Configure-Ack.  However,
			 * acceptance of the Configuration Option does
			 * not imply that the synchronous implementation
			 * will do any ACCM mapping.  Instead, all such
			 * octet mapping will be performed by the
			 * asynchronous-to-synchronous converter.
			 */
			continue;

		case LCP_OPT_MRU:
			/*
			 * Maximum receive unit.  Always agreeable,
			 * but ignored by now.
			 */
			sp->lcp.their_mru = p[2] * 256 + p[3];
			if (debug)
				addlog(" %ld", sp->lcp.their_mru);
			continue;

		case LCP_OPT_AUTH_PROTO:
			authproto = (p[2] << 8) + p[3];
			if (ISSET(sp->myauth.flags, SPPP_AUTHFLAG_PASSIVEAUTHPROTO)) {
				if (authproto == PPP_PAP || authproto == PPP_CHAP)
					sp->myauth.proto = authproto;
			}
			if (sp->myauth.proto == authproto) {
				if (authproto != PPP_CHAP || p[4] == CHAP_MD5) {
					continue;
				}
				if (debug)
					addlog(" [chap without MD5]");
			} else {
				if (debug) {
					char pbuf1[SPPP_PROTO_NAMELEN];
					char pbuf2[SPPP_PROTO_NAMELEN];
					const char *pname1, *pname2;

					pname1 = sppp_proto_name(pbuf1,
					    sizeof(pbuf1), sp->myauth.proto);
					pname2 = sppp_proto_name(pbuf2,
					    sizeof(pbuf2), authproto);
					addlog(" [mine %s != his %s]",
					       pname1, pname2);
				}
			}
			/* not agreed, nak */
			if (sp->myauth.proto == PPP_CHAP) {
				l = 5;
			} else {
				l = 4;
			}

			if (rlen + l > blen) {
				if (debug)
					addlog(" [overflow]");
				continue;
			}

			r[0] = LCP_OPT_AUTH_PROTO;
			r[1] = l;
			r[2] = sp->myauth.proto >> 8;
			r[3] = sp->myauth.proto & 0xff;
			if (sp->myauth.proto == PPP_CHAP)
				r[4] = CHAP_MD5;
			rlen += l;
			r += l;
			continue;
		case LCP_OPT_MP_EID:
			/*
			 * Endpoint identification.
			 * Always agreeable,
			 * but ignored by now.
			 */
			if (debug) {
				addlog(" type %d", p[2]);
				sppp_print_bytes(p+3, p[1]-3);
			}
			continue;
		case LCP_OPT_MP_MRRU:
			/*
			 * Maximum received reconstructed unit. 
			 * Always agreeable,
			 * but ignored by now.
			 */
			sp->lcp.their_mrru = p[2] * 256 + p[3];
			if (debug)
				addlog(" %ld", sp->lcp.their_mrru);
			continue;
		}
		if (rlen + l > blen) {
			if (debug)
				addlog(" [overflow]");
			continue;
		}
		/* Add the option to nak'ed list. */
		memcpy(r, p, l);
		r += l;
		rlen += l;
	}

	if (rlen > 0) {
		if (++sp->scp[IDX_LCP].fail_counter >= sp->lcp.max_failure) {
			if (debug)
				addlog(" max_failure (%d) exceeded, ",
				    sp->lcp.max_failure);
			type = CP_RCR_REJ;
		} else {
			type = CP_RCR_NAK;
		}
	} else {
		type = CP_RCR_ACK;
		rlen = origlen;
		memcpy(r, h + 1, rlen);
		sp->scp[IDX_LCP].fail_counter = 0;
		sp->pp_loopcnt = 0;
	}

end:
	if (debug)
		addlog("\n");

	if (type == CP_RCR_ERR || type == CP_RCR_DROP) {
		if (buf != NULL)
			kmem_intr_free(buf, blen);
	} else {
		*msgbuf = buf;
		*buflen = blen;
		*msglen = rlen;
	}

	return type;
}

/*
 * Analyze the LCP Configure-Reject option list, and adjust our
 * negotiation.
 */
static void
sppp_lcp_confrej(struct sppp *sp, struct lcp_header *h, int len)
{
	u_char *p, l;
	bool debug;

	KASSERT(SPPP_WLOCKED(sp));

	debug = sppp_debug_enabled(sp);

	if (len <= sizeof(*h))
		return;

	len -= sizeof(*h);

	if (debug)
		SPPP_LOG(sp, LOG_DEBUG, "lcp rej opts:");

	p = (void *)(h + 1);
	for (; len > 1 && (l = p[1]) != 0; len -= l, p += l) {
		/* Sanity check option length */
		if (l > len) {
			/*
			 * Malicious option - drop immediately.
			 * XXX Maybe we should just RXJ it?
			 */
			if (debug)
				addlog("\n");

			SPPP_LOG(sp, LOG_DEBUG,
			    "received malicious LCP option, dropping.\n");
			goto end;
		}
		if (debug) {
			char lbuf[SPPP_LCPOPT_NAMELEN];
			addlog(" %s", sppp_lcp_opt_name(lbuf, sizeof(lbuf), *p));
		}
		switch (p[0]) {
		case LCP_OPT_MAGIC:
			/* Magic number -- can't use it, use 0 */
			CLR(sp->lcp.opts, SPPP_LCP_OPT_MAGIC);
			sp->lcp.magic = 0;
			break;
		case LCP_OPT_MRU:
			/*
			 * We try to negotiate a lower MRU if the underlying
			 * link's MTU is less than PP_MTU (e.g. PPPoE). If the
			 * peer rejects this lower rate, fallback to the
			 * default.
			 */
			if (!debug) {
				SPPP_LOG(sp, LOG_INFO,
				    "peer rejected our MRU of "
				    "%ld bytes. Defaulting to %d bytes\n",
				    sp->lcp.mru, PP_MTU);
			}
			CLR(sp->lcp.opts, SPPP_LCP_OPT_MRU);
			sp->lcp.mru = PP_MTU;
			break;
		case LCP_OPT_AUTH_PROTO:
			/*
			 * Peer doesn't want to authenticate himself,
			 * deny unless this is a dialout call, and
			 * SPPP_AUTHFLAG_NOCALLOUT is set.
			 */
			if ((sp->pp_flags & PP_CALLIN) == 0 &&
			    (sp->hisauth.flags & SPPP_AUTHFLAG_NOCALLOUT) != 0) {
				if (debug) {
					addlog(" [don't insist on auth "
					       "for callout]");
				}
				CLR(sp->lcp.opts, SPPP_LCP_OPT_AUTH_PROTO);
				break;
			}
			if (debug)
				addlog("[access denied]\n");
			sppp_wq_add(sp->wq_cp, &sp->scp[IDX_LCP].work_close);
			break;
		}
	}
	if (debug)
		addlog("\n");
end:
	return;
}

/*
 * Analyze the LCP Configure-NAK option list, and adjust our
 * negotiation.
 */
static void
sppp_lcp_confnak(struct sppp *sp, struct lcp_header *h, int len)
{
	u_char *p, l;
	uint32_t magic;
	bool debug;

	KASSERT(SPPP_WLOCKED(sp));

	if (len <= sizeof(*h))
		return;

	debug = sppp_debug_enabled(sp);
	len -= sizeof(*h);

	if (debug)
		SPPP_LOG(sp, LOG_DEBUG, "lcp nak opts:");

	p = (void *)(h + 1);
	for (; len > 1 && (l = p[1]) != 0; len -= l, p += l) {
		/* Sanity check option length */
		if (l > len) {
			/*
			 * Malicious option - drop immediately.
			 * XXX Maybe we should just RXJ it?
			 */
			if (debug)
				addlog("\n");

			SPPP_LOG(sp, LOG_DEBUG,
			    "received malicious LCP option, dropping.\n");
			goto end;
		}
		if (debug) {
			char lbuf[SPPP_LCPOPT_NAMELEN];
			addlog(" %s", sppp_lcp_opt_name(lbuf, sizeof(lbuf),*p));
		}
		switch (p[0]) {
		case LCP_OPT_MAGIC:
			/* Magic number -- renegotiate */
			if (ISSET(sp->lcp.opts, SPPP_LCP_OPT_MAGIC) &&
			    len >= 6 && l == 6) {
				magic = (uint32_t)p[2] << 24 |
					(uint32_t)p[3] << 16 | p[4] << 8 | p[5];
				/*
				 * If the remote magic is our negated one,
				 * this looks like a loopback problem.
				 * Suggest a new magic to make sure.
				 */
				if (magic == ~sp->lcp.magic) {
					if (debug)
						addlog(" magic glitch");
					sp->lcp.magic = cprng_fast32();
				} else {
					sp->lcp.magic = magic;
					if (debug)
						addlog(" %d", magic);
				}
			}
			break;
		case LCP_OPT_MRU:
			/*
			 * Peer wants to advise us to negotiate an MRU.
			 * Agree on it if it's reasonable, or use
			 * default otherwise.
			 */
			if (len >= 4 && l == 4) {
				u_int mru = p[2] * 256 + p[3];
				if (debug)
					addlog(" %d", mru);
				if (mru < PPP_MINMRU || mru > sp->pp_if.if_mtu)
					mru = sp->pp_if.if_mtu;
				sp->lcp.mru = mru;
				SET(sp->lcp.opts, SPPP_LCP_OPT_MRU);
			}
			break;
		case LCP_OPT_AUTH_PROTO:
			/*
			 * Peer doesn't like our authentication method,
			 * deny.
			 */
			if (debug)
				addlog("[access denied]\n");
			sppp_wq_add(sp->wq_cp, &sp->scp[IDX_LCP].work_close);
			break;
		}
	}
	if (debug)
		addlog("\n");
end:
	return;
}

static void
sppp_lcp_tlu(struct sppp *sp)
{
	struct ifnet *ifp;
	struct sppp_cp *scp;
	int i;
	bool going_up;

	KASSERT(SPPP_WLOCKED(sp));

	ifp = &sp->pp_if;

	/* unlock for IFNET_LOCK and if_up() */
	SPPP_UNLOCK(sp);

	if (! (ifp->if_flags & IFF_UP) &&
	    (ifp->if_flags & IFF_RUNNING)) {
		/* Coming out of loopback mode. */
		going_up = true;
		if_up(ifp);
	} else {
		going_up = false;
	}

	IFNET_LOCK(ifp);
	SPPP_LOCK(sp, RW_WRITER);

	if (going_up) {
		if ((sp->pp_flags & PP_LOOPBACK) == 0) {
			SPPP_LOG(sp, LOG_DEBUG,
			    "interface is going up, "
			    "but no loopback packet is deteted\n");
		}
		sp->pp_flags &= ~PP_LOOPBACK;
	}

	if (ifp->if_mtu > sp->lcp.their_mru) {
		sp->pp_saved_mtu = ifp->if_mtu;
		ifp->if_mtu = sp->lcp.their_mru;
		SPPP_DLOG(sp, "setting MTU "
		    "from %"PRIu64" bytes to %"PRIu64" bytes\n",
		    sp->pp_saved_mtu, ifp->if_mtu);
	}
	IFNET_UNLOCK(ifp);

	if (ISSET(sp->lcp.opts, SPPP_LCP_OPT_AUTH_PROTO) ||
	    (sp->pp_flags & PP_NEEDAUTH) != 0)
		sppp_change_phase(sp, SPPP_PHASE_AUTHENTICATE);
	else
		sppp_change_phase(sp, SPPP_PHASE_NETWORK);


	for (i = 0; i < IDX_COUNT; i++) {
		scp = &sp->scp[(cps[i])->protoidx];

		if (((cps[i])->flags & CP_LCP) == 0)
			sppp_wq_add(sp->wq_cp, &scp->work_up);

		/*
		 * Open all authentication protocols.  This is even required
		 * if we already proceeded to network phase, since it might be
		 * that remote wants us to authenticate, so we might have to
		 * send a PAP request.  Undesired authentication protocols
		 * don't do anything when they get an Open event.
		 */
		if ((cps[i])->flags & CP_AUTH)
			sppp_wq_add(sp->wq_cp, &scp->work_open);

		/* Open all NCPs. */
		if (sp->pp_phase == SPPP_PHASE_NETWORK &&
		    ((cps[i])->flags & CP_NCP) != 0) {
			sppp_wq_add(sp->wq_cp, &scp->work_open);
		}
	}

	/* notify low-level driver of state change */
	sppp_notify_chg_wlocked(sp);
}

static void
sppp_lcp_tld(struct sppp *sp)
{
	struct ifnet *ifp;
	struct sppp_cp *scp;
	int i, phase;

	KASSERT(SPPP_WLOCKED(sp));

	phase = sp->pp_phase;

	sppp_change_phase(sp, SPPP_PHASE_TERMINATE);

	if (sp->pp_saved_mtu > 0) {
		ifp = &sp->pp_if;

		SPPP_UNLOCK(sp);
		IFNET_LOCK(ifp);
		SPPP_LOCK(sp, RW_WRITER);

		SPPP_DLOG(sp, "setting MTU "
		    "from %"PRIu64" bytes to %"PRIu64" bytes\n",
		    ifp->if_mtu, sp->pp_saved_mtu);

		ifp->if_mtu = sp->pp_saved_mtu;
		sp->pp_saved_mtu = 0;
		IFNET_UNLOCK(ifp);
	}

	/*
	 * Take upper layers down.  We send the Down event first and
	 * the Close second to prevent the upper layers from sending
	 * ``a flurry of terminate-request packets'', as the RFC
	 * describes it.
	 */
	for (i = 0; i < IDX_COUNT; i++) {
		scp = &sp->scp[(cps[i])->protoidx];

		if (((cps[i])->flags & CP_LCP) == 0)
			sppp_wq_add(sp->wq_cp, &scp->work_down);

		if ((cps[i])->flags & CP_AUTH) {
			sppp_wq_add(sp->wq_cp, &scp->work_close);
		}

		/* Close all NCPs. */
		if (phase == SPPP_PHASE_NETWORK &&
		    ((cps[i])->flags & CP_NCP) != 0) {
			sppp_wq_add(sp->wq_cp, &scp->work_close);
		}
	}
}

static void
sppp_lcp_tls(const struct cp *cp __unused, struct sppp *sp)
{

	KASSERT(SPPP_WLOCKED(sp));

	sppp_change_phase(sp, SPPP_PHASE_ESTABLISH);

	/* Notify lower layer if desired. */
	sppp_notify_tls_wlocked(sp);
	sp->lcp.tlf_sent = false;
}

static void
sppp_lcp_tlf(const struct cp *cp __unused, struct sppp *sp)
{

	KASSERT(SPPP_WLOCKED(sp));

	sppp_change_phase(sp, SPPP_PHASE_DEAD);

	/* Notify lower layer if desired. */
	sppp_notify_tlf_wlocked(sp);

	switch (sp->scp[IDX_LCP].state) {
	case STATE_CLOSED:
	case STATE_STOPPED:
		sp->lcp.tlf_sent = true;
		break;
	case STATE_INITIAL:
	default:
		/* just in case */
		sp->lcp.tlf_sent = false;
	}
}

static void
sppp_lcp_scr(struct sppp *sp)
{
	char opt[6 /* magicnum */ + 4 /* mru */ + 5 /* chap */];
	int i = 0;
	u_short authproto;

	KASSERT(SPPP_WLOCKED(sp));

	if (ISSET(sp->lcp.opts, SPPP_LCP_OPT_MAGIC)) {
		if (! sp->lcp.magic)
			sp->lcp.magic = cprng_fast32();
		opt[i++] = LCP_OPT_MAGIC;
		opt[i++] = 6;
		opt[i++] = sp->lcp.magic >> 24;
		opt[i++] = sp->lcp.magic >> 16;
		opt[i++] = sp->lcp.magic >> 8;
		opt[i++] = sp->lcp.magic;
	}

	if (ISSET(sp->lcp.opts,SPPP_LCP_OPT_MRU)) {
		opt[i++] = LCP_OPT_MRU;
		opt[i++] = 4;
		opt[i++] = sp->lcp.mru >> 8;
		opt[i++] = sp->lcp.mru;
	}

	if (ISSET(sp->lcp.opts, SPPP_LCP_OPT_AUTH_PROTO)) {
		authproto = sp->hisauth.proto;
		opt[i++] = LCP_OPT_AUTH_PROTO;
		opt[i++] = authproto == PPP_CHAP? 5: 4;
		opt[i++] = authproto >> 8;
		opt[i++] = authproto;
		if (authproto == PPP_CHAP)
			opt[i++] = CHAP_MD5;
	}

	sp->scp[IDX_LCP].confid = ++sp->scp[IDX_LCP].seq;
	sppp_cp_send(sp, PPP_LCP, CONF_REQ, sp->scp[IDX_LCP].confid, i, &opt);
}

/*
 * Check the open NCPs, return true if at least one NCP is open.
 */

static int
sppp_cp_check(struct sppp *sp, u_char cp_flags)
{
	int i, mask;

	for (i = 0, mask = 1; i < IDX_COUNT; i++, mask <<= 1)
		if ((sp->lcp.protos & mask) && (cps[i])->flags & cp_flags)
			return 1;
	return 0;
}

/*
 * Re-check the open NCPs and see if we should terminate the link.
 * Called by the NCPs during their tlf action handling.
 */
static void
sppp_lcp_check_and_close(struct sppp *sp)
{

	KASSERT(SPPP_WLOCKED(sp));

	if (sp->pp_phase < SPPP_PHASE_AUTHENTICATE) {
		/* don't bother, we are already going down */
		return;
	}

	if (sp->pp_phase == SPPP_PHASE_AUTHENTICATE &&
	    sppp_cp_check(sp, CP_AUTH))
		return;

	if (sp->pp_phase >= SPPP_PHASE_NETWORK &&
	    sppp_cp_check(sp, CP_NCP))
		return;

	sppp_wq_add(sp->wq_cp, &sp->scp[IDX_LCP].work_close);

	if (sp->pp_max_auth_fail != 0 &&
	    sp->pp_auth_failures >= sp->pp_max_auth_fail) {
		SPPP_LOG(sp, LOG_INFO, "authentication failed %d times, "
		    "not retrying again\n", sp->pp_auth_failures);

		sppp_wq_add(sp->wq_cp, &sp->work_ifdown);
		sp->pp_if.if_flags &= ~IFF_RUNNING;
	} else {
		sppp_wq_add(sp->wq_cp, &sp->scp[IDX_LCP].work_open);
	}
}

/*
 *--------------------------------------------------------------------------*
 *                                                                          *
 *                        The IPCP implementation.                          *
 *                                                                          *
 *--------------------------------------------------------------------------*
 */

static void
sppp_ipcp_init(struct sppp *sp)
{

	KASSERT(SPPP_WLOCKED(sp));

	sppp_cp_init(&ipcp, sp);

	sp->ipcp.opts = 0;
	sp->ipcp.flags = 0;
}

static void
sppp_ipcp_open(struct sppp *sp, void *xcp)
{
	uint32_t myaddr, hisaddr;

	KASSERT(SPPP_WLOCKED(sp));
	KASSERT(!cpu_softintr_p());

	if (!ISSET(sp->pp_ncpflags, SPPP_NCP_IPCP))
		return;

	sp->ipcp.flags &= ~(IPCP_HISADDR_SEEN|IPCP_MYADDR_SEEN|IPCP_MYADDR_DYN|IPCP_HISADDR_DYN);
	sp->ipcp.req_myaddr = 0;
	sp->ipcp.req_hisaddr = 0;
	memset(&sp->dns_addrs, 0, sizeof sp->dns_addrs);

#ifdef INET
	sppp_get_ip_addrs(sp, &myaddr, &hisaddr, 0);
#else
	myaddr = hisaddr = 0;
#endif
	/*
	 * If we don't have his address, this probably means our
	 * interface doesn't want to talk IP at all.  (This could
	 * be the case if somebody wants to speak only IPX, for
	 * example.)  Don't open IPCP in this case.
	 */
	if (hisaddr == 0) {
		/* XXX this message should go away */
		SPPP_DLOG(sp, "ipcp_open(): no IP interface\n");
		return;
	}

	if (myaddr == 0) {
		/*
		 * I don't have an assigned address, so i need to
		 * negotiate my address.
		 */
		sp->ipcp.flags |= IPCP_MYADDR_DYN;
		SET(sp->ipcp.opts, SPPP_IPCP_OPT_ADDRESS);
	}
	if (hisaddr == 1) {
		/*
		 * XXX - remove this hack!
		 * remote has no valid address, we need to get one assigned.
		 */
		sp->ipcp.flags |= IPCP_HISADDR_DYN;
		sp->ipcp.saved_hisaddr = htonl(hisaddr);
	}

	if (sp->query_dns & 1) {
		SET(sp->ipcp.opts, SPPP_IPCP_OPT_PRIMDNS);
	} else {
		CLR(sp->ipcp.opts, SPPP_IPCP_OPT_PRIMDNS);
	}

	if (sp->query_dns & 2) {
		SET(sp->ipcp.opts, SPPP_IPCP_OPT_SECDNS);
	} else {
		CLR(sp->ipcp.opts, SPPP_IPCP_OPT_SECDNS);
	}
	sppp_open_event(sp, xcp);
}

static void
sppp_ipcp_close(struct sppp *sp, void *xcp)
{

	KASSERT(SPPP_WLOCKED(sp));
	KASSERT(!cpu_softintr_p());

	sppp_close_event(sp, xcp);

#ifdef INET
	if (sp->ipcp.flags & (IPCP_MYADDR_DYN|IPCP_HISADDR_DYN)) {
		/*
		 * Some address was dynamic, clear it again.
		 */
		sppp_clear_ip_addrs(sp);
	}
#endif
	memset(&sp->dns_addrs, 0, sizeof sp->dns_addrs);
}

/*
 * Analyze a configure request.  Return true if it was agreeable, and
 * caused action sca, false if it has been rejected or nak'ed, and
 * caused action scn.  (The return value is used to make the state
 * transition decision in the state automaton.)
 */
static enum cp_rcr_type
sppp_ipcp_confreq(struct sppp *sp, struct lcp_header *h, int origlen,
   uint8_t **msgbuf, size_t *buflen, size_t *msglen)
{
	u_char *buf, *r, *p, l, blen;
	enum cp_rcr_type type;
	int rlen, len;
	uint32_t hisaddr, desiredaddr;
	char ipbuf[SPPP_IPCPOPT_NAMELEN];
	char dqbuf[SPPP_DOTQUAD_BUFLEN];
	const char *dq;
	bool debug;

	KASSERT(SPPP_WLOCKED(sp));

	type = CP_RCR_NONE;
	origlen -= sizeof(*h);

	if (origlen < 0)
		return CP_RCR_DROP;

	debug = sppp_debug_enabled(sp);

	/*
	 * Make sure to allocate a buf that can at least hold a
	 * conf-nak with an `address' option.  We might need it below.
	 */
	blen = MAX(6, origlen);

	buf = kmem_intr_alloc(blen, KM_NOSLEEP);
	if (buf == NULL)
		return CP_RCR_DROP;

	/* pass 1: see if we can recognize them */
	if (debug)
		SPPP_LOG(sp, LOG_DEBUG, "ipcp parse opts:");
	p = (void *)(h + 1);
	r = buf;
	rlen = 0;
	for (len = origlen; len > 1; len -= l, p += l) {
		l = p[1];
		if (l == 0)
			break;

		/* Sanity check option length */
		if (l > len) {
			/* XXX should we just RXJ? */
			if (debug)
				addlog("\n");

			SPPP_LOG(sp, LOG_DEBUG,
			    " malicious IPCP option received, dropping\n");
			type = CP_RCR_ERR;
			goto end;
		}
		if (debug) {
			addlog(" %s",
			    sppp_ipcp_opt_name(ipbuf, sizeof(ipbuf), *p));
		}
		switch (p[0]) {
#ifdef notyet
		case IPCP_OPT_COMPRESSION:
			if (len >= 6 && l >= 6) {
				/* correctly formed compress option */
				continue;
			}
			if (debug)
				addlog(" [invalid]");
			break;
#endif
		case IPCP_OPT_ADDRESS:
			if (len >= 6 && l == 6) {
				/* correctly formed address option */
				continue;
			}
			if (debug)
				addlog(" [invalid]");
			break;
		default:
			/* Others not supported. */
			if (debug)
				addlog(" [rej]");
			break;
		}
		/* Add the option to rejected list. */
		if (rlen + l > blen) {
			if (debug)
				addlog(" [overflow]");
			continue;
		}
		memcpy(r, p, l);
		r += l;
		rlen += l;
	}

	if (rlen > 0) {
		type = CP_RCR_REJ;
		goto end;
	}

	if (debug)
		addlog("\n");

	/* pass 2: parse option values */
	if (sp->ipcp.flags & IPCP_HISADDR_SEEN)
		hisaddr = sp->ipcp.req_hisaddr;	/* we already aggreed on that */
	else
#ifdef INET
		sppp_get_ip_addrs(sp, 0, &hisaddr, 0);	/* user configuration */
#else
		hisaddr = 0;
#endif
	if (debug)
		SPPP_LOG(sp, LOG_DEBUG, "ipcp parse opt values:");
	p = (void *)(h + 1);
	r = buf;
	rlen = 0;
	for (len = origlen; len > 1; len -= l, p += l) {
		l = p[1];
		if (l == 0)
			break;

		if (debug) {
			addlog(" %s",
			    sppp_ipcp_opt_name(ipbuf, sizeof(ipbuf), *p));
		}
		switch (p[0]) {
#ifdef notyet
		case IPCP_OPT_COMPRESSION:
			continue;
#endif
		case IPCP_OPT_ADDRESS:
			desiredaddr = p[2] << 24 | p[3] << 16 |
				p[4] << 8 | p[5];
			if (desiredaddr == hisaddr ||
		    	   ((sp->ipcp.flags & IPCP_HISADDR_DYN) && desiredaddr != 0)) {
				/*
			 	* Peer's address is same as our value,
			 	* this is agreeable.  Gonna conf-ack
			 	* it.
			 	*/
				if (debug) {
					dq = sppp_dotted_quad(dqbuf,
					    sizeof(dqbuf), hisaddr);
					addlog(" %s [ack]", dq);
				}
				/* record that we've seen it already */
				sp->ipcp.flags |= IPCP_HISADDR_SEEN;
				sp->ipcp.req_hisaddr = desiredaddr;
				hisaddr = desiredaddr;
				continue;
			}
			/*
		 	* The address wasn't agreeable.  This is either
		 	* he sent us 0.0.0.0, asking to assign him an
		 	* address, or he send us another address not
		 	* matching our value.  Either case, we gonna
		 	* conf-nak it with our value.
		 	*/
			if (debug) {
				if (desiredaddr == 0) {
					addlog(" [addr requested]");
				} else {
					dq = sppp_dotted_quad(dqbuf,
					    sizeof(dqbuf), desiredaddr);
					addlog(" %s [not agreed]", dq);
				}
			}

			p[2] = hisaddr >> 24;
			p[3] = hisaddr >> 16;
			p[4] = hisaddr >> 8;
			p[5] = hisaddr;
			break;
		}
		if (rlen + l > blen) {
			if (debug)
				addlog(" [overflow]");
			continue;
		}
		/* Add the option to nak'ed list. */
		memcpy(r, p, l);
		r += l;
		rlen += l;
	}

	if (rlen > 0) {
		type = CP_RCR_NAK;
	} else {
		if ((sp->ipcp.flags & IPCP_HISADDR_SEEN) == 0) {
			/*
			 * If we are about to conf-ack the request, but haven't seen
			 * his address so far, gonna conf-nak it instead, with the
			 * `address' option present and our idea of his address being
			 * filled in there, to request negotiation of both addresses.
			 *
			 * XXX This can result in an endless req - nak loop if peer
			 * doesn't want to send us his address.  Q: What should we do
			 * about it?  XXX  A: implement the max-failure counter.
			 */
			buf[0] = IPCP_OPT_ADDRESS;
			buf[1] = 6;
			buf[2] = hisaddr >> 24;
			buf[3] = hisaddr >> 16;
			buf[4] = hisaddr >> 8;
			buf[5] = hisaddr;
			rlen = 6;
			if (debug)
				addlog(" still need hisaddr");
			type = CP_RCR_NAK;
		} else {
			type = CP_RCR_ACK;
			rlen = origlen;
			memcpy(r, h + 1, rlen);
		}
	}

end:
	if (debug)
		addlog("\n");

	if (type == CP_RCR_ERR || type == CP_RCR_DROP) {
		if (buf != NULL)
			kmem_intr_free(buf, blen);
	} else {
		*msgbuf = buf;
		*buflen = blen;
		*msglen = rlen;
	}

	return type;
}

/*
 * Analyze the IPCP Configure-Reject option list, and adjust our
 * negotiation.
 */
static void
sppp_ipcp_confrej(struct sppp *sp, struct lcp_header *h, int len)
{
	u_char *p, l;
	bool debug;

	KASSERT(SPPP_WLOCKED(sp));

	if (len <= sizeof(*h))
		return;

	len -= sizeof(*h);
	debug = sppp_debug_enabled(sp);

	if (debug)
		SPPP_LOG(sp, LOG_DEBUG, "ipcp rej opts:");

	p = (void *)(h + 1);
	for (; len > 1; len -= l, p += l) {
		l = p[1];
		if (l == 0)
			break;

		/* Sanity check option length */
		if (l > len) {
			/* XXX should we just RXJ? */
			if (debug)
				addlog("\n");
			SPPP_LOG(sp, LOG_DEBUG,
			    "malicious IPCP option received, dropping\n");
			goto end;
		}
		if (debug) {
			char ipbuf[SPPP_IPCPOPT_NAMELEN];
			addlog(" %s",
			    sppp_ipcp_opt_name(ipbuf, sizeof(ipbuf), *p));
		}
		switch (p[0]) {
		case IPCP_OPT_ADDRESS:
			/*
			 * Peer doesn't grok address option.  This is
			 * bad.  XXX  Should we better give up here?
			 */
			if (!debug) {
				SPPP_LOG(sp, LOG_ERR,
				    "IPCP address option rejected\n");
			}
			CLR(sp->ipcp.opts, SPPP_IPCP_OPT_ADDRESS);
			break;
#ifdef notyet
		case IPCP_OPT_COMPRESS:
			CLR(sp->ipcp.opts, SPPP_IPCP_OPT_COMPRESS);
			break;
#endif
		case IPCP_OPT_PRIMDNS:
			CLR(sp->ipcp.opts, SPPP_IPCP_OPT_PRIMDNS);
			break;

		case IPCP_OPT_SECDNS:
			CLR(sp->ipcp.opts, SPPP_IPCP_OPT_SECDNS);
			break;
		}
	}
	if (debug)
		addlog("\n");
end:
	return;
}

/*
 * Analyze the IPCP Configure-NAK option list, and adjust our
 * negotiation.
 */
static void
sppp_ipcp_confnak(struct sppp *sp, struct lcp_header *h, int len)
{
	u_char *p, l;
	struct ifnet *ifp = &sp->pp_if;
	int debug = ifp->if_flags & IFF_DEBUG;
	uint32_t wantaddr;

	KASSERT(SPPP_WLOCKED(sp));

	len -= sizeof(*h);

	debug = sppp_debug_enabled(sp);

	if (debug)
		SPPP_LOG(sp, LOG_DEBUG, "ipcp nak opts:");

	p = (void *)(h + 1);
	for (; len > 1; len -= l, p += l) {
		l = p[1];
		if (l == 0)
			break;

		/* Sanity check option length */
		if (l > len) {
			/* XXX should we just RXJ? */
			if (debug)
				addlog("\n");
			SPPP_LOG(sp, LOG_DEBUG,
			    "malicious IPCP option received, dropping\n");
			return;
		}
		if (debug) {
			char ipbuf[SPPP_IPCPOPT_NAMELEN];
			addlog(" %s",
			    sppp_ipcp_opt_name(ipbuf, sizeof(ipbuf), *p));
		}
		switch (*p) {
		case IPCP_OPT_ADDRESS:
			/*
			 * Peer doesn't like our local IP address.  See
			 * if we can do something for him.  We'll drop
			 * him our address then.
			 */
			if (len >= 6 && l == 6) {
				wantaddr = p[2] << 24 | p[3] << 16 |
					p[4] << 8 | p[5];
				SET(sp->ipcp.opts, SPPP_IPCP_OPT_ADDRESS);
				if (debug) {
					char dqbuf[SPPP_DOTQUAD_BUFLEN];
					const char *dq;

					dq = sppp_dotted_quad(dqbuf,
					    sizeof(dqbuf), wantaddr);
					addlog(" [wantaddr %s]", dq);
				}
				/*
				 * When doing dynamic address assignment,
				 * we accept his offer.  Otherwise, we
				 * ignore it and thus continue to negotiate
				 * our already existing value.
				 */
				if (sp->ipcp.flags & IPCP_MYADDR_DYN) {
					if (ntohl(wantaddr) != INADDR_ANY) {
						if (debug)
							addlog(" [agree]");
						sp->ipcp.flags |= IPCP_MYADDR_SEEN;
						sp->ipcp.req_myaddr = wantaddr;
					} else {
						if (debug)
							addlog(" [not agreed]");
					}
				}
			}
			break;

		case IPCP_OPT_PRIMDNS:
			if (ISSET(sp->ipcp.opts, SPPP_IPCP_OPT_PRIMDNS) &&
			    len >= 6 && l == 6) {
				sp->dns_addrs[0] = p[2] << 24 | p[3] << 16 |
					p[4] << 8 | p[5];
			}
			break;

		case IPCP_OPT_SECDNS:
			if (ISSET(sp->ipcp.opts, SPPP_IPCP_OPT_SECDNS) &&
			    len >= 6 && l == 6) {
				sp->dns_addrs[1] = p[2] << 24 | p[3] << 16 |
					p[4] << 8 | p[5];
			}
			break;
#ifdef notyet
		case IPCP_OPT_COMPRESS:
			/*
			 * Peer wants different compression parameters.
			 */
			break;
#endif
		}
	}
	if (debug)
		addlog("\n");
}

static void
sppp_ipcp_tlu(struct sppp *sp)
{
#ifdef INET
	struct ifnet *ifp;

	KASSERT(SPPP_WLOCKED(sp));

	SPPP_LOG(sp, LOG_INFO, "IPCP layer up\n");
	ifp = &sp->pp_if;
	if ((sp->ipcp.flags & IPCP_MYADDR_DYN) &&
	    ((sp->ipcp.flags & IPCP_MYADDR_SEEN) == 0)) {
		SPPP_LOG(sp, LOG_WARNING,
		    "no IP address, closing IPCP\n");
		sppp_wq_add(sp->wq_cp,
		    &sp->scp[IDX_IPCP].work_close);
	} else {
		/* we are up. Set addresses and notify anyone interested */
		sppp_set_ip_addrs(sp);
		rt_ifmsg(ifp);
	}
#endif
}

static void
sppp_ipcp_tld(struct sppp *sp)
{
#ifdef INET
	struct ifnet *ifp;

	KASSERT(SPPP_WLOCKED(sp));

	SPPP_LOG(sp, LOG_INFO, "IPCP layer down\n");
	ifp = &sp->pp_if;
	rt_ifmsg(ifp);
#endif
}

static void
sppp_ipcp_scr(struct sppp *sp)
{
	uint8_t opt[6 /* compression */ + 6 /* address */ + 12 /* dns addresses */];
#ifdef INET
	uint32_t ouraddr;
#endif
	int i = 0;

	KASSERT(SPPP_WLOCKED(sp));

#ifdef notyet
	if (ISSET(sp->ipcp.opts,SPPP_IPCP_OPT_COMPRESSION)) {
		opt[i++] = IPCP_OPT_COMPRESSION;
		opt[i++] = 6;
		opt[i++] = 0;	/* VJ header compression */
		opt[i++] = 0x2d; /* VJ header compression */
		opt[i++] = max_slot_id;
		opt[i++] = comp_slot_id;
	}
#endif

#ifdef INET
	if (ISSET(sp->ipcp.opts, SPPP_IPCP_OPT_ADDRESS)) {
		if (sp->ipcp.flags & IPCP_MYADDR_SEEN) {
			ouraddr = sp->ipcp.req_myaddr;	/* not sure if this can ever happen */
		} else {
			sppp_get_ip_addrs(sp, &ouraddr, 0, 0);
		}
		opt[i++] = IPCP_OPT_ADDRESS;
		opt[i++] = 6;
		opt[i++] = ouraddr >> 24;
		opt[i++] = ouraddr >> 16;
		opt[i++] = ouraddr >> 8;
		opt[i++] = ouraddr;
	}
#endif

	if (ISSET(sp->ipcp.opts, SPPP_IPCP_OPT_PRIMDNS)) {
		opt[i++] = IPCP_OPT_PRIMDNS;
		opt[i++] = 6;
		opt[i++] = sp->dns_addrs[0] >> 24;
		opt[i++] = sp->dns_addrs[0] >> 16;
		opt[i++] = sp->dns_addrs[0] >> 8;
		opt[i++] = sp->dns_addrs[0];
	}
	if (ISSET(sp->ipcp.opts, SPPP_IPCP_OPT_SECDNS)) {
		opt[i++] = IPCP_OPT_SECDNS;
		opt[i++] = 6;
		opt[i++] = sp->dns_addrs[1] >> 24;
		opt[i++] = sp->dns_addrs[1] >> 16;
		opt[i++] = sp->dns_addrs[1] >> 8;
		opt[i++] = sp->dns_addrs[1];
	}

	sp->scp[IDX_IPCP].confid = ++sp->scp[IDX_IPCP].seq;
	sppp_cp_send(sp, PPP_IPCP, CONF_REQ, sp->scp[IDX_IPCP].confid, i, &opt);
}

/*
 *--------------------------------------------------------------------------*
 *                                                                          *
 *                      The IPv6CP implementation.                          *
 *                                                                          *
 *--------------------------------------------------------------------------*
 */

#ifdef INET6
static void
sppp_ipv6cp_init(struct sppp *sp)
{

	KASSERT(SPPP_WLOCKED(sp));

	sppp_cp_init(&ipv6cp, sp);

	sp->ipv6cp.opts = 0;
	sp->ipv6cp.flags = 0;
}

static void
sppp_ipv6cp_open(struct sppp *sp, void *xcp)
{
	struct in6_addr myaddr, hisaddr;

	KASSERT(SPPP_WLOCKED(sp));
	KASSERT(!cpu_softintr_p());

	if (!ISSET(sp->pp_ncpflags, SPPP_NCP_IPV6CP))
		return;

#ifdef IPV6CP_MYIFID_DYN
	sp->ipv6cp.flags &= ~(IPV6CP_MYIFID_SEEN|IPV6CP_MYIFID_DYN);
#else
	sp->ipv6cp.flags &= ~IPV6CP_MYIFID_SEEN;
#endif

	sppp_get_ip6_addrs(sp, &myaddr, &hisaddr, 0);
	/*
	 * If we don't have our address, this probably means our
	 * interface doesn't want to talk IPv6 at all.  (This could
	 * be the case if somebody wants to speak only IPX, for
	 * example.)  Don't open IPv6CP in this case.
	 */
	if (IN6_IS_ADDR_UNSPECIFIED(&myaddr)) {
		/* XXX this message should go away */
		SPPP_DLOG(sp, "ipv6cp_open(): no IPv6 interface\n");
		return;
	}

	sp->ipv6cp.flags |= IPV6CP_MYIFID_SEEN;
	SET(sp->ipv6cp.opts, SPPP_IPV6CP_OPT_IFID);
	sppp_open_event(sp, xcp);
}

/*
 * Analyze a configure request.  Return true if it was agreeable, and
 * caused action sca, false if it has been rejected or nak'ed, and
 * caused action scn.  (The return value is used to make the state
 * transition decision in the state automaton.)
 */
static enum cp_rcr_type
sppp_ipv6cp_confreq(struct sppp *sp, struct lcp_header *h, int origlen,
    uint8_t **msgbuf, size_t *buflen, size_t *msglen)
{
	u_char *buf, *r, *p, l, blen;
	int rlen, len;
	struct in6_addr myaddr, desiredaddr, suggestaddr;
	enum cp_rcr_type type;
	int ifidcount;
	int collision, nohisaddr;
	char ip6buf[INET6_ADDRSTRLEN];
	char tbuf[SPPP_CPTYPE_NAMELEN];
	char ipv6buf[SPPP_IPV6CPOPT_NAMELEN];
	const char *cpname;
	bool debug;

	KASSERT(SPPP_WLOCKED(sp));

	debug = sppp_debug_enabled(sp);
	type = CP_RCR_NONE;
	origlen -= sizeof(*h);

	if (origlen < 0)
		return CP_RCR_DROP;

	/*
	 * Make sure to allocate a buf that can at least hold a
	 * conf-nak with an `address' option.  We might need it below.
	 */
	blen = MAX(6, origlen);

	buf = kmem_intr_alloc(blen, KM_NOSLEEP);
	if (buf == NULL)
		return CP_RCR_DROP;

	/* pass 1: see if we can recognize them */
	if (debug)
		SPPP_LOG(sp, LOG_DEBUG, "ipv6cp parse opts:");
	p = (void *)(h + 1);
	r = buf;
	rlen = 0;
	ifidcount = 0;
	for (len = origlen; len > 1; len -= l, p += l) {
		l = p[1];
		if (l == 0)
			break;

		/* Sanity check option length */
		if (l > len) {
			/* XXX just RXJ? */
			if (debug)
				addlog("\n");
			SPPP_LOG(sp, LOG_DEBUG,
			    "received malicious IPCPv6 option, "
			    "dropping\n");
			type = CP_RCR_ERR;
			goto end;
		}
		if (debug) {
			addlog(" %s", sppp_ipv6cp_opt_name(ipv6buf,
			    sizeof(ipv6buf),*p));
		}
		switch (p[0]) {
		case IPV6CP_OPT_IFID:
			if (len >= 10 && l == 10 && ifidcount == 0) {
				/* correctly formed address option */
				ifidcount++;
				continue;
			}
			if (debug)
				addlog(" [invalid]");
			break;
#ifdef notyet
		case IPV6CP_OPT_COMPRESSION:
			if (len >= 4 && l >= 4) {
				/* correctly formed compress option */
				continue;
			}
			if (debug)
				addlog(" [invalid]");
			break;
#endif
		default:
			/* Others not supported. */
			if (debug)
				addlog(" [rej]");
			break;
		}
		if (rlen + l > blen) {
			if (debug)
				addlog(" [overflow]");
			continue;
		}
		/* Add the option to rejected list. */
		memcpy(r, p, l);
		r += l;
		rlen += l;
	}

	if (rlen > 0) {
		type = CP_RCR_REJ;
		goto end;
	}

	if (debug)
		addlog("\n");

	/* pass 2: parse option values */
	sppp_get_ip6_addrs(sp, &myaddr, 0, 0);
	if (debug)
		SPPP_LOG(sp, LOG_DEBUG, "ipv6cp parse opt values:");
	p = (void *)(h + 1);
	r = buf;
	rlen = 0;
	type = CP_RCR_ACK;
	for (len = origlen; len > 1; len -= l, p += l) {
		l = p[1];
		if (l == 0)
			break;

		if (debug) {
			addlog(" %s", sppp_ipv6cp_opt_name(ipv6buf,
			    sizeof(ipv6buf), *p));
		}
		switch (p[0]) {
#ifdef notyet
		case IPV6CP_OPT_COMPRESSION:
			continue;
#endif
		case IPV6CP_OPT_IFID:
			memset(&desiredaddr, 0, sizeof(desiredaddr));
			memcpy(&desiredaddr.s6_addr[8], &p[2], 8);
			collision = (memcmp(&desiredaddr.s6_addr[8],
					&myaddr.s6_addr[8], 8) == 0);
			nohisaddr = IN6_IS_ADDR_UNSPECIFIED(&desiredaddr);

			desiredaddr.s6_addr16[0] = htons(0xfe80);
			(void)in6_setscope(&desiredaddr, &sp->pp_if, NULL);

			if (!collision && !nohisaddr) {
				/* no collision, hisaddr known - Conf-Ack */
				type = CP_RCR_ACK;
				memcpy(sp->ipv6cp.my_ifid, &myaddr.s6_addr[8],
				    sizeof(sp->ipv6cp.my_ifid));
				memcpy(sp->ipv6cp.his_ifid,
				    &desiredaddr.s6_addr[8],
				    sizeof(sp->ipv6cp.my_ifid));

				if (debug) {
					cpname = sppp_cp_type_name(tbuf,
					    sizeof(tbuf), CONF_ACK);
					addlog(" %s [%s]",
					    IN6_PRINT(ip6buf, &desiredaddr),
					    cpname);
				}
				continue;
			}

			memset(&suggestaddr, 0, sizeof(suggestaddr));
			if (collision && nohisaddr) {
				/* collision, hisaddr unknown - Conf-Rej */
				type = CP_RCR_REJ;
				memset(&p[2], 0, 8);
			} else {
				/*
				 * - no collision, hisaddr unknown, or
				 * - collision, hisaddr known
				 * Conf-Nak, suggest hisaddr
				 */
				type = CP_RCR_NAK;
				sppp_suggest_ip6_addr(sp, &suggestaddr);
				memcpy(&p[2], &suggestaddr.s6_addr[8], 8);
			}
			if (debug) {
				int ctype = type == CP_RCR_REJ ? CONF_REJ : CONF_NAK;

				cpname = sppp_cp_type_name(tbuf, sizeof(tbuf), ctype);
				addlog(" %s [%s]", IN6_PRINT(ip6buf, &desiredaddr),
				   cpname);
			}
			break;
		}
		if (rlen + l > blen) {
			if (debug)
				addlog(" [overflow]");
			continue;
		}
		/* Add the option to nak'ed list. */
		memcpy(r, p, l);
		r += l;
		rlen += l;
	}

	if (rlen > 0) {
		if (type != CP_RCR_ACK) {
			if (debug) {
				int ctype ;
				ctype = type == CP_RCR_REJ ?
				    CONF_REJ : CONF_NAK;
				cpname =  sppp_cp_type_name(tbuf, sizeof(tbuf), ctype);
				addlog(" send %s suggest %s\n",
				    cpname, IN6_PRINT(ip6buf, &suggestaddr));
			}
		}
#ifdef notdef
		if (type == CP_RCR_ACK)
			panic("IPv6CP RCR: CONF_ACK with non-zero rlen");
#endif
	} else {
		if (type == CP_RCR_ACK) {
			rlen = origlen;
			memcpy(r, h + 1, rlen);
		}
	}
end:
	if (debug)
		addlog("\n");

	if (type == CP_RCR_ERR || type == CP_RCR_DROP) {
		if (buf != NULL)
			kmem_intr_free(buf, blen);
	} else {
		*msgbuf = buf;
		*buflen = blen;
		*msglen = rlen;
	}

	return type;
}

/*
 * Analyze the IPv6CP Configure-Reject option list, and adjust our
 * negotiation.
 */
static void
sppp_ipv6cp_confrej(struct sppp *sp, struct lcp_header *h, int len)
{
	u_char *p, l;
	bool debug;

	KASSERT(SPPP_WLOCKED(sp));

	if (len <= sizeof(*h))
		return;

	len -= sizeof(*h);
	debug = sppp_debug_enabled(sp);

	if (debug)
		SPPP_LOG(sp, LOG_DEBUG, "ipv6cp rej opts:");

	p = (void *)(h + 1);
	for (; len > 1; len -= l, p += l) {
		l = p[1];
		if (l == 0)
			break;

		if (l > len) {
			/* XXX just RXJ? */
			if (debug)
				addlog("\n");
			SPPP_LOG(sp, LOG_DEBUG,
			    "received malicious IPCPv6 option, "
			    "dropping\n");
			goto end;
		}
		if (debug) {
			char ipv6buf[SPPP_IPV6CPOPT_NAMELEN];
			addlog(" %s", sppp_ipv6cp_opt_name(ipv6buf,
			    sizeof(ipv6buf), *p));
		}
		switch (p[0]) {
		case IPV6CP_OPT_IFID:
			/*
			 * Peer doesn't grok address option.  This is
			 * bad.  XXX  Should we better give up here?
			 */
			CLR(sp->ipv6cp.opts, SPPP_IPV6CP_OPT_IFID);
			break;
#ifdef notyet
		case IPV6CP_OPT_COMPRESS:
			CLR(sp->ipv6cp.opts, SPPP_IPV6CP_OPT_COMPRESS);
			break;
#endif
		}
	}
	if (debug)
		addlog("\n");
end:
	return;
}

/*
 * Analyze the IPv6CP Configure-NAK option list, and adjust our
 * negotiation.
 */
static void
sppp_ipv6cp_confnak(struct sppp *sp, struct lcp_header *h, int len)
{
	u_char *p, l;
	struct in6_addr suggestaddr;
	char ip6buf[INET6_ADDRSTRLEN];
	bool debug;

	KASSERT(SPPP_WLOCKED(sp));

	if (len <= sizeof(*h))
		return;

	len -= sizeof(*h);
	debug = sppp_debug_enabled(sp);

	if (debug)
		SPPP_LOG(sp, LOG_DEBUG, "ipv6cp nak opts:");

	p = (void *)(h + 1);
	for (; len > 1; len -= l, p += l) {
		l = p[1];
		if (l == 0)
			break;

		if (l > len) {
			/* XXX just RXJ? */
			if (debug)
				addlog("\n");
			SPPP_LOG(sp, LOG_DEBUG,
			    "received malicious IPCPv6 option, "
			    "dropping\n");
			goto end;
		}
		if (debug) {
			char ipv6buf[SPPP_IPV6CPOPT_NAMELEN];
			addlog(" %s", sppp_ipv6cp_opt_name(ipv6buf,
			    sizeof(ipv6buf), *p));
		}
		switch (p[0]) {
		case IPV6CP_OPT_IFID:
			/*
			 * Peer doesn't like our local ifid.  See
			 * if we can do something for him.  We'll drop
			 * him our address then.
			 */
			if (len < 10 || l != 10)
				break;
			memset(&suggestaddr, 0, sizeof(suggestaddr));
			suggestaddr.s6_addr16[0] = htons(0xfe80);
			(void)in6_setscope(&suggestaddr, &sp->pp_if, NULL);
			memcpy(&suggestaddr.s6_addr[8], &p[2], 8);

			SET(sp->ipv6cp.opts, SPPP_IPV6CP_OPT_IFID);
			if (debug)
				addlog(" [suggestaddr %s]",
				       IN6_PRINT(ip6buf, &suggestaddr));
#ifdef IPV6CP_MYIFID_DYN
			/*
			 * When doing dynamic address assignment,
			 * we accept his offer.
			 */
			if (sp->ipv6cp.flags & IPV6CP_MYIFID_DYN) {
				struct in6_addr lastsuggest;
				/*
				 * If <suggested myaddr from peer> equals to
				 * <hisaddr we have suggested last time>,
				 * we have a collision.  generate new random
				 * ifid.
				 */
				sppp_suggest_ip6_addr(&lastsuggest);
				if (IN6_ARE_ADDR_EQUAL(&suggestaddr,
						 lastsuggest)) {
					if (debug)
						addlog(" [random]");
					sppp_gen_ip6_addr(sp, &suggestaddr);
				}
				sppp_set_ip6_addr(sp, &suggestaddr, 0);
				if (debug)
					addlog(" [agree]");
				sp->ipv6cp.flags |= IPV6CP_MYIFID_SEEN;
			}
#else
			/*
			 * Since we do not do dynamic address assignment,
			 * we ignore it and thus continue to negotiate
			 * our already existing value.  This can possibly
			 * go into infinite request-reject loop.
			 *
			 * This is not likely because we normally use
			 * ifid based on MAC-address.
			 * If you have no ethernet card on the node, too bad.
			 * XXX should we use fail_counter?
			 */
#endif
			break;
#ifdef notyet
		case IPV6CP_OPT_COMPRESS:
			/*
			 * Peer wants different compression parameters.
			 */
			break;
#endif
		}
	}
	if (debug)
		addlog("\n");
end:
	return;
}

static void
sppp_ipv6cp_tlu(struct sppp *sp)
{
	struct ifnet *ifp;

	KASSERT(SPPP_WLOCKED(sp));

	SPPP_LOG(sp, LOG_INFO, "IPv6CP layer up\n");
	ifp = &sp->pp_if;
	/* we are up - notify isdn daemon */
	sppp_notify_con_wlocked(sp);
	rt_ifmsg(ifp);
}

static void
sppp_ipv6cp_tld(struct sppp *sp)
{
	struct ifnet *ifp;

	KASSERT(SPPP_WLOCKED(sp));

	SPPP_LOG(sp, LOG_INFO, "IPv6CP layer down\n");
	ifp = &sp->pp_if;
	rt_ifmsg(ifp);
}

static void
sppp_ipv6cp_scr(struct sppp *sp)
{
	char opt[10 /* ifid */ + 4 /* compression, minimum */];
	struct in6_addr ouraddr;
	int i = 0;

	KASSERT(SPPP_WLOCKED(sp));

	if (ISSET(sp->ipv6cp.opts, SPPP_IPV6CP_OPT_IFID)) {
		sppp_get_ip6_addrs(sp, &ouraddr, 0, 0);

		opt[i++] = IPV6CP_OPT_IFID;
		opt[i++] = 10;
		memcpy(&opt[i], &ouraddr.s6_addr[8], 8);
		i += 8;
	}

#ifdef notyet
	if (ISSET(sp->ipv6cp.opts, SPPP_IPV6CP_OPT_COMPRESSION)) {
		opt[i++] = IPV6CP_OPT_COMPRESSION;
		opt[i++] = 4;
		opt[i++] = 0;	/* TBD */
		opt[i++] = 0;	/* TBD */
		/* variable length data may follow */
	}
#endif

	sp->scp[IDX_IPV6CP].confid = ++sp->scp[IDX_IPV6CP].seq;
	sppp_cp_send(sp, PPP_IPV6CP, CONF_REQ, sp->scp[IDX_IPV6CP].confid, i, &opt);
}
#else /*INET6*/
static void
sppp_ipv6cp_init(struct sppp *sp)
{

	KASSERT(SPPP_WLOCKED(sp));
}

static void
sppp_ipv6cp_open(struct sppp *sp, void *xcp)
{

	KASSERT(SPPP_WLOCKED(sp));
}

static enum cp_rcr_type
sppp_ipv6cp_confreq(struct sppp *sp, struct lcp_header *h,
    int len, uint8_t **msgbuf, size_t *buflen, size_t *msglen)
{

	KASSERT(SPPP_WLOCKED(sp));
	return 0;
}

static void
sppp_ipv6cp_confrej(struct sppp *sp, struct lcp_header *h,
		    int len)
{

	KASSERT(SPPP_WLOCKED(sp));
}

static void
sppp_ipv6cp_confnak(struct sppp *sp, struct lcp_header *h,
		    int len)
{

	KASSERT(SPPP_WLOCKED(sp));
}

static void
sppp_ipv6cp_tlu(struct sppp *sp)
{

	KASSERT(SPPP_WLOCKED(sp));
}

static void
sppp_ipv6cp_tld(struct sppp *sp)
{

	KASSERT(SPPP_WLOCKED(sp));
}

static void
sppp_ipv6cp_scr(struct sppp *sp)
{

	KASSERT(SPPP_WLOCKED(sp));
}
#endif /*INET6*/

/*
 *--------------------------------------------------------------------------*
 *                                                                          *
 *                        The CHAP implementation.                          *
 *                                                                          *
 *--------------------------------------------------------------------------*
 */
/*
 * The authentication protocols is implemented on the state machine for
 * control protocols. And it uses following actions and events.
 *
 * Actions:
 *    - scr: send CHAP_CHALLENGE and CHAP_RESPONSE
 *    - sca: send CHAP_SUCCESS
 *    - scn: send CHAP_FAILURE and shutdown lcp
 * Events:
 *    - RCR+: receive CHAP_RESPONSE containing correct digest
 *    - RCR-: receive CHAP_RESPONSE containing wrong digest
 *    - RCA: receive CHAP_SUCCESS
 *    - RCN: (this event is unused)
 *    - TO+: re-send CHAP_CHALLENGE and CHAP_RESPONSE
 *    - TO-: this layer finish
 */

/*
 * Handle incoming CHAP packets.
 */
void
sppp_chap_input(struct sppp *sp, struct mbuf *m)
{
	struct ifnet *ifp;
	struct lcp_header *h;
	int len, x;
	u_char *value, *name, digest[sizeof(sp->chap.challenge)];
	int value_len, name_len;
	MD5_CTX ctx;
	char abuf[SPPP_AUTHTYPE_NAMELEN];
	const char *authname;
	bool debug;

	ifp = &sp->pp_if;
	debug = sppp_debug_enabled(sp);
	len = m->m_pkthdr.len;
	if (len < 4) {
		SPPP_DLOG(sp, "chap invalid packet length: "
		    "%d bytes\n", len);
		return;
	}
	h = mtod(m, struct lcp_header *);
	if (len > ntohs(h->len))
		len = ntohs(h->len);

	SPPP_LOCK(sp, RW_WRITER);

	switch (h->type) {
	/* challenge, failure and success are his authproto */
	case CHAP_CHALLENGE:
		if (sp->myauth.secret == NULL || sp->myauth.name == NULL) {
			/* can't do anything useful */
			sp->pp_auth_failures++;
			SPPP_DLOG(sp, "chap input "
			    "without my name and my secret being set\n");
			break;
		}
		value = 1 + (u_char *)(h + 1);
		value_len = value[-1];
		name = value + value_len;
		name_len = len - value_len - 5;
		if (name_len < 0) {
			if (debug) {
				authname = sppp_auth_type_name(abuf,
				    sizeof(abuf), PPP_CHAP, h->type);
				SPPP_LOG(sp, LOG_DEBUG,
				    "chap corrupted challenge "
				    "<%s id=0x%x len=%d",
				    authname, h->ident, ntohs(h->len));
				if (len > 4)
					sppp_print_bytes((u_char *)(h + 1),
					    len - 4);
				addlog(">\n");
			}
			break;
		}

		if (debug) {
			authname = sppp_auth_type_name(abuf,
			    sizeof(abuf), PPP_CHAP, h->type);
			SPPP_LOG(sp, LOG_DEBUG,
			    "chap input <%s id=0x%x len=%d name=",
			    authname, h->ident, ntohs(h->len));
			sppp_print_string((char *) name, name_len);
			addlog(" value-size=%d value=", value_len);
			sppp_print_bytes(value, value_len);
			addlog(">\n");
		}

		/* Compute reply value. */
		MD5Init(&ctx);
		MD5Update(&ctx, &h->ident, 1);
		MD5Update(&ctx, sp->myauth.secret, sp->myauth.secret_len);
		MD5Update(&ctx, value, value_len);
		MD5Final(sp->chap.digest, &ctx);
		sp->chap.digest_len = sizeof(sp->chap.digest);
		sp->scp[IDX_CHAP].rconfid = h->ident;

		sppp_wq_add(sp->wq_cp, &sp->chap.work_challenge_rcvd);
		break;

	case CHAP_SUCCESS:
		if (debug) {
			SPPP_LOG(sp, LOG_DEBUG, "chap success");
			if (len > 4) {
				addlog(": ");
				sppp_print_string((char *)(h + 1), len - 4);
			}
			addlog("\n");
		}

		if (h->ident != sp->scp[IDX_CHAP].rconfid) {
			SPPP_DLOG(sp, "%s id mismatch 0x%x != 0x%x\n",
			    chap.name, h->ident,
			    sp->scp[IDX_CHAP].rconfid);
			if_statinc(ifp, if_ierrors);
			break;
		}

		if (sp->chap.digest_len == 0) {
			SPPP_DLOG(sp, "receive CHAP success"
			    " without challenge\n");
			if_statinc(ifp, if_ierrors);
			break;
		}

		x = splnet();
		sp->pp_auth_failures = 0;
		sp->pp_flags &= ~PP_NEEDAUTH;
		splx(x);
		memset(sp->chap.digest, 0, sizeof(sp->chap.digest));
		sp->chap.digest_len = 0;

		if (!ISSET(sppp_auth_role(&chap, sp), SPPP_AUTH_SERV)) {
			/*
			 * we are not authenticator for CHAP,
			 * generate a dummy RCR+ event without CHAP_RESPONSE
			 */
			sp->scp[IDX_CHAP].rcr_type = CP_RCR_ACK;
			sppp_wq_add(sp->wq_cp, &sp->scp[IDX_CHAP].work_rcr);
		}
		sppp_wq_add(sp->wq_cp, &sp->scp[IDX_CHAP].work_rca);
		break;

	case CHAP_FAILURE:
		if (h->ident != sp->scp[IDX_CHAP].rconfid) {
			SPPP_DLOG(sp, "%s id mismatch 0x%x != 0x%x\n",
			    chap.name, h->ident, sp->scp[IDX_CHAP].rconfid);
			if_statinc(ifp, if_ierrors);
			break;
		}

		if (sp->chap.digest_len == 0) {
			SPPP_DLOG(sp, "receive CHAP failure "
			    "without challenge\n");
			if_statinc(ifp, if_ierrors);
			break;
		}

		x = splnet();
		sp->pp_auth_failures++;
		splx(x);
		SPPP_LOG(sp, LOG_INFO, "chap failure");
		if (debug) {
			if (len > 4) {
				addlog(": ");
				sppp_print_string((char *)(h + 1), len - 4);
			}
		}
		addlog("\n");

		memset(sp->chap.digest, 0, sizeof(sp->chap.digest));
		sp->chap.digest_len = 0;
		/*
		 * await LCP shutdown by authenticator,
		 * so we don't have to enqueue sc->scp[IDX_CHAP].work_rcn
		 */
		break;

	/* response is my authproto */
	case CHAP_RESPONSE:
		if (sp->hisauth.name == NULL || sp->hisauth.secret == NULL) {
			/* can't do anything useful */
			SPPP_DLOG(sp, "chap response "
			    "without his name and his secret being set\n");
			break;
		}
		value = 1 + (u_char *)(h + 1);
		value_len = value[-1];
		name = value + value_len;
		name_len = len - value_len - 5;
		if (name_len < 0) {
			if (debug) {
				authname = sppp_auth_type_name(abuf,
				    sizeof(abuf), PPP_CHAP, h->type);
				SPPP_LOG(sp, LOG_DEBUG,
				    "chap corrupted response "
				    "<%s id=0x%x len=%d",
				    authname, h->ident, ntohs(h->len));
				if (len > 4)
					sppp_print_bytes((u_char *)(h + 1),
					    len - 4);
				addlog(">\n");
			}
			break;
		}
		if (h->ident != sp->scp[IDX_CHAP].confid) {
			SPPP_DLOG(sp, "chap dropping response for old ID "
			    "(got %d, expected %d)\n",
			    h->ident, sp->scp[IDX_CHAP].confid);
			break;
		} else {
			sp->scp[IDX_CHAP].rconfid = h->ident;
		}

		if (sp->hisauth.name != NULL &&
		    (name_len != sp->hisauth.name_len
		    || memcmp(name, sp->hisauth.name, name_len) != 0)) {
			SPPP_LOG(sp, LOG_INFO,
			    "chap response, his name ");
			sppp_print_string(name, name_len);
			addlog(" != expected ");
			sppp_print_string(sp->hisauth.name,
					  sp->hisauth.name_len);
			addlog("\n");

			/* generate RCR- event */
			sp->scp[IDX_CHAP].rcr_type = CP_RCR_NAK;
			sppp_wq_add(sp->wq_cp, &sp->scp[IDX_CHAP].work_rcr);
			break;
		}

		if (debug) {
			authname = sppp_auth_type_name(abuf,
			    sizeof(abuf), PPP_CHAP, h->type);
			SPPP_LOG(sp, LOG_DEBUG, "chap input(%s) "
			    "<%s id=0x%x len=%d name=",
			    sppp_state_name(sp->scp[IDX_CHAP].state),
			    authname, h->ident, ntohs(h->len));
			sppp_print_string((char *)name, name_len);
			addlog(" value-size=%d value=", value_len);
			sppp_print_bytes(value, value_len);
			addlog(">\n");
		}

		if (value_len == sizeof(sp->chap.challenge) &&
		    value_len == sizeof(sp->chap.digest)) {
			MD5Init(&ctx);
			MD5Update(&ctx, &h->ident, 1);
			MD5Update(&ctx, sp->hisauth.secret, sp->hisauth.secret_len);
			MD5Update(&ctx, sp->chap.challenge, sizeof(sp->chap.challenge));
			MD5Final(digest, &ctx);

			if (memcmp(digest, value, value_len) == 0) {
				sp->scp[IDX_CHAP].rcr_type = CP_RCR_ACK;
			} else {
				sp->scp[IDX_CHAP].rcr_type = CP_RCR_NAK;
			}
		} else {
			if (debug) {
				SPPP_LOG(sp, LOG_DEBUG,
				    "chap bad hash value length: "
				    "%d bytes, should be %zu\n",
				    value_len, sizeof(sp->chap.challenge));
			}

			sp->scp[IDX_CHAP].rcr_type = CP_RCR_NAK;
		}

		sppp_wq_add(sp->wq_cp, &sp->scp[IDX_CHAP].work_rcr);

		/* generate a dummy RCA event */
		if (sp->scp[IDX_CHAP].rcr_type == CP_RCR_ACK &&
		    (!ISSET(sppp_auth_role(&chap, sp), SPPP_AUTH_PEER) ||
		    sp->chap.rechallenging)) {
			sppp_wq_add(sp->wq_cp, &sp->scp[IDX_CHAP].work_rca);
		}
		break;

	default:
		/* Unknown CHAP packet type -- ignore. */
		if (debug) {
			SPPP_LOG(sp, LOG_DEBUG, "chap unknown input(%s) "
			    "<0x%x id=0x%xh len=%d",
			    sppp_state_name(sp->scp[IDX_CHAP].state),
			    h->type, h->ident, ntohs(h->len));
			if (len > 4)
				sppp_print_bytes((u_char *)(h + 1), len - 4);
			addlog(">\n");
		}
		break;

	}

	SPPP_UNLOCK(sp);
}

static void
sppp_chap_init(struct sppp *sp)
{

	KASSERT(SPPP_WLOCKED(sp));

	sppp_cp_init(&chap, sp);

	SPPP_WQ_SET(&sp->chap.work_challenge_rcvd,
	    sppp_chap_rcv_challenge_event, &chap);
}

static void
sppp_chap_open(struct sppp *sp, void *xcp)
{

	KASSERT(SPPP_WLOCKED(sp));

	memset(sp->chap.digest, 0, sizeof(sp->chap.digest));
	sp->chap.digest_len = 0;
	sp->chap.rechallenging = false;
	sp->chap.response_rcvd = false;
	sppp_open_event(sp, xcp);
}

static void
sppp_chap_tlu(struct sppp *sp)
{
	int i, x;

	KASSERT(SPPP_WLOCKED(sp));

	i = 0;
	sp->scp[IDX_CHAP].rst_counter = sp->lcp.max_configure;
	x = splnet();
	sp->pp_auth_failures = 0;
	splx(x);

	SPPP_LOG(sp, LOG_DEBUG, "chap %s",
	    sp->pp_phase == SPPP_PHASE_NETWORK ? "reconfirmed" : "tlu");

	/*
	 * Some broken CHAP implementations (Conware CoNet, firmware
	 * 4.0.?) don't want to re-authenticate their CHAP once the
	 * initial challenge-response exchange has taken place.
	 * Provide for an option to avoid rechallenges.
	 */
	if (ISSET(sppp_auth_role(&chap, sp), SPPP_AUTH_SERV) &&
	    (sp->hisauth.flags & SPPP_AUTHFLAG_NORECHALLENGE) == 0) {
		/*
		 * Compute the re-challenge timeout.  This will yield
		 * a number between 300 and 810 seconds.
		 */
		i = 300 + ((unsigned)(cprng_fast32() & 0xff00) >> 7);
		callout_schedule(&sp->scp[IDX_CHAP].ch, i * hz);

		if (sppp_debug_enabled(sp)) {
			addlog(", next rechallenge in %d seconds", i);
		}
	}

	addlog("\n");

	/*
	 * If we are already in phase network, we are done here.  This
	 * is the case if this is a dummy tlu event after a re-challenge.
	 */
	if (sp->pp_phase != SPPP_PHASE_NETWORK)
		sppp_phase_network(sp);
}

static void
sppp_chap_scr(struct sppp *sp)
{
	uint32_t *ch;
	u_char clen, dsize;
	int role;

	KASSERT(SPPP_WLOCKED(sp));

	role = sppp_auth_role(&chap, sp);

	if (ISSET(role, SPPP_AUTH_SERV) &&
	    !sp->chap.response_rcvd) {
		/* we are authenticator for CHAP, send challenge */
		ch = (uint32_t *)sp->chap.challenge;
		clen = sizeof(sp->chap.challenge);
		/* Compute random challenge. */
		cprng_strong(kern_cprng, ch, clen, 0);

		sp->scp[IDX_CHAP].confid = ++sp->scp[IDX_CHAP].seq;
		sppp_auth_send(&chap, sp, CHAP_CHALLENGE, sp->scp[IDX_CHAP].confid,
		    sizeof(clen), (const char *)&clen,
		    sizeof(sp->chap.challenge), sp->chap.challenge,
		    0);
	}

	if (ISSET(role, SPPP_AUTH_PEER) &&
	    sp->chap.digest_len > 0) {
		/* we are peer for CHAP, send response */
		dsize = sp->chap.digest_len;

		sppp_auth_send(&chap, sp, CHAP_RESPONSE, sp->scp[IDX_CHAP].rconfid,
		    sizeof(dsize), (const char *)&dsize,
		    sp->chap.digest_len, sp->chap.digest,
		    sp->myauth.name_len, sp->myauth.name, 0);
	}
}

static void
sppp_chap_rcv_challenge_event(struct sppp *sp, void *xcp)
{
	const struct cp *cp = xcp;

	KASSERT(!cpu_softintr_p());

	sp->chap.rechallenging = false;

	switch (sp->scp[IDX_CHAP].state) {
	case STATE_REQ_SENT:
		sppp_cp_change_state(cp, sp, STATE_REQ_SENT);
		cp->scr(sp);
		break;
	case STATE_OPENED:
		sppp_cp_change_state(cp, sp, STATE_ACK_SENT);
		cp->scr(sp);
		break;
	}
}

/*
 *--------------------------------------------------------------------------*
 *                                                                          *
 *                        The PAP implementation.                           *
 *                                                                          *
 *--------------------------------------------------------------------------*
 */
/*
 * PAP uses following actions and events.
 * Actions:
 *    - scr: send PAP_REQ
 *    - sca: send PAP_ACK
 *    - scn: send PAP_NAK
 * Events:
 *    - RCR+: receive PAP_REQ containing correct username and password
 *    - RCR-: receive PAP_REQ containing wrong username and password
 *    - RCA: receive PAP_ACK
 *    - RCN: (this event is unused)
 *    - TO+: re-send PAP_REQ
 *    - TO-: this layer finish
 */

/*
 * Handle incoming PAP packets.  */
static void
sppp_pap_input(struct sppp *sp, struct mbuf *m)
{
	struct ifnet *ifp;
	struct lcp_header *h;
	int len, x;
	char *name, *secret;
	int name_len, secret_len;
	char abuf[SPPP_AUTHTYPE_NAMELEN];
	const char *authname;
	bool debug;

	ifp = &sp->pp_if;
	debug = sppp_debug_enabled(sp);

	/*
	 * Malicious input might leave this uninitialized, so
	 * init to an impossible value.
	 */
	secret_len = -1;

	len = m->m_pkthdr.len;
	if (len < 5) {
		SPPP_DLOG(sp, "pap invalid packet length: "
		    "%d bytes\n", len);
		return;
	}
	h = mtod(m, struct lcp_header *);
	if (len > ntohs(h->len))
		len = ntohs(h->len);

	SPPP_LOCK(sp, RW_WRITER);

	switch (h->type) {
	/* PAP request is my authproto */
	case PAP_REQ:
		if (sp->hisauth.name == NULL || sp->hisauth.secret == NULL) {
			/* can't do anything useful */
			SPPP_DLOG(sp, "pap request"
			    " without his name and his secret being set\n");
			break;
		}
		name = 1 + (u_char *)(h + 1);
		name_len = name[-1];
		secret = name + name_len + 1;
		if (name_len > len - 6 ||
		    (secret_len = secret[-1]) > len - 6 - name_len) {
			if (debug) {
				authname = sppp_auth_type_name(abuf,
				    sizeof(abuf), PPP_PAP, h->type);
				SPPP_LOG(sp, LOG_DEBUG, "pap corrupted input "
				    "<%s id=0x%x len=%d",
				    authname, h->ident, ntohs(h->len));
				if (len > 4)
					sppp_print_bytes((u_char *)(h + 1),
					    len - 4);
				addlog(">\n");
			}
			break;
		}
		if (debug) {
			authname = sppp_auth_type_name(abuf,
			    sizeof(abuf), PPP_PAP, h->type);
			SPPP_LOG(sp, LOG_DEBUG, "pap input(%s) "
			    "<%s id=0x%x len=%d name=",
			    sppp_state_name(sp->scp[IDX_PAP].state),
			    authname, h->ident, ntohs(h->len));
			sppp_print_string((char *)name, name_len);
			addlog(" secret=");
			sppp_print_string((char *)secret, secret_len);
			addlog(">\n");
		}

		sp->scp[IDX_PAP].rconfid = h->ident;

		if (name_len == sp->hisauth.name_len &&
		    memcmp(name, sp->hisauth.name, name_len) == 0 &&
		    secret_len == sp->hisauth.secret_len &&
		    memcmp(secret, sp->hisauth.secret, secret_len) == 0) {
			sp->scp[IDX_PAP].rcr_type = CP_RCR_ACK;
		} else {
			sp->scp[IDX_PAP].rcr_type = CP_RCR_NAK;
		}

		sppp_wq_add(sp->wq_cp, &sp->scp[IDX_PAP].work_rcr);

		/* generate a dummy RCA event */
		if (sp->scp[IDX_PAP].rcr_type == CP_RCR_ACK &&
		    !ISSET(sppp_auth_role(&pap, sp), SPPP_AUTH_PEER)) {
			sppp_wq_add(sp->wq_cp, &sp->scp[IDX_PAP].work_rca);
		}
		break;

	/* ack and nak are his authproto */
	case PAP_ACK:
		if (debug) {
			SPPP_LOG(sp, LOG_DEBUG, "pap success");
			name = 1 + (u_char *)(h + 1);
			name_len = name[-1];
			if (len > 5 && name_len < len+4) {
				addlog(": ");
				sppp_print_string(name, name_len);
			}
			addlog("\n");
		}

		if (h->ident != sp->scp[IDX_PAP].confid) {
			SPPP_DLOG(sp, "%s id mismatch 0x%x != 0x%x\n",
			    pap.name, h->ident, sp->scp[IDX_PAP].rconfid);
			if_statinc(ifp, if_ierrors);
			break;
		}

		x = splnet();
		sp->pp_auth_failures = 0;
		sp->pp_flags &= ~PP_NEEDAUTH;
		splx(x);

		/* we are not authenticator, generate a dummy RCR+ event */
		if (!ISSET(sppp_auth_role(&pap, sp), SPPP_AUTH_SERV)) {
			sp->scp[IDX_PAP].rcr_type = CP_RCR_ACK;
			sppp_wq_add(sp->wq_cp, &sp->scp[IDX_PAP].work_rcr);
		}

		sppp_wq_add(sp->wq_cp, &sp->scp[IDX_PAP].work_rca);
		break;

	case PAP_NAK:
		if (debug) {
			SPPP_LOG(sp, LOG_INFO, "pap failure");
			name = 1 + (u_char *)(h + 1);
			name_len = name[-1];
			if (len > 5 && name_len < len+4) {
				addlog(": ");
				sppp_print_string(name, name_len);
			}
			addlog("\n");
		} else {
			SPPP_LOG(sp, LOG_INFO, "pap failure\n");
		}

		if (h->ident != sp->scp[IDX_PAP].confid) {
			SPPP_DLOG(sp, "%s id mismatch 0x%x != 0x%x\n",
			    pap.name, h->ident, sp->scp[IDX_PAP].rconfid);
			if_statinc(ifp, if_ierrors);
			break;
		}

		sp->pp_auth_failures++;
		/*
		 * await LCP shutdown by authenticator,
		 * so we don't have to enqueue sc->scp[IDX_PAP].work_rcn
		 */
		break;

	default:
		/* Unknown PAP packet type -- ignore. */
		if (debug) {
			SPPP_LOG(sp, LOG_DEBUG, "pap corrupted input "
			    "<0x%x id=0x%x len=%d",
			    h->type, h->ident, ntohs(h->len));
			if (len > 4)
				sppp_print_bytes((u_char *)(h + 1), len - 4);
			addlog(">\n");
		}
		break;
	}

	SPPP_UNLOCK(sp);
}

static void
sppp_pap_init(struct sppp *sp)
{

	KASSERT(SPPP_WLOCKED(sp));
	sppp_cp_init(&pap, sp);
}

static void
sppp_pap_tlu(struct sppp *sp)
{
	int x;

	SPPP_DLOG(sp, "%s tlu\n", pap.name);

	sp->scp[IDX_PAP].rst_counter = sp->lcp.max_configure;
	x = splnet();
	sp->pp_auth_failures = 0;
	splx(x);

	if (sp->pp_phase < SPPP_PHASE_NETWORK)
		sppp_phase_network(sp);
}

static void
sppp_pap_scr(struct sppp *sp)
{
	u_char idlen, pwdlen;

	KASSERT(SPPP_WLOCKED(sp));

	if (ISSET(sppp_auth_role(&pap, sp), SPPP_AUTH_PEER) &&
	    sp->scp[IDX_PAP].state != STATE_ACK_RCVD) {
		if (sp->myauth.secret == NULL ||
		    sp->myauth.name == NULL) {
			SPPP_LOG(sp, LOG_DEBUG,
			    "couldn't send PAP_REQ "
			    "because of no name or no secret\n");
		} else {
			sp->scp[IDX_PAP].confid = ++sp->scp[IDX_PAP].seq;
			pwdlen = sp->myauth.secret_len;
			idlen = sp->myauth.name_len;

			sppp_auth_send(&pap, sp, PAP_REQ, sp->scp[IDX_PAP].confid,
			    sizeof idlen, (const char *)&idlen,
			    idlen, sp->myauth.name,
			    sizeof pwdlen, (const char *)&pwdlen,
			    pwdlen, sp->myauth.secret,
			    0);
		}
	}
}

/*
 * Random miscellaneous functions.
 */

/*
 * Send a PAP or CHAP proto packet.
 *
 * Varadic function, each of the elements for the ellipsis is of type
 * ``size_t mlen, const u_char *msg''.  Processing will stop iff
 * mlen == 0.
 * NOTE: never declare variadic functions with types subject to type
 * promotion (i.e. u_char). This is asking for big trouble depending
 * on the architecture you are on...
 */

static void
sppp_auth_send(const struct cp *cp, struct sppp *sp,
               unsigned int type, unsigned int id,
	       ...)
{
	struct ifnet *ifp;
	struct lcp_header *lh;
	struct mbuf *m;
	u_char *p;
	int len;
	size_t pkthdrlen;
	unsigned int mlen;
	const char *msg;
	va_list ap;

	KASSERT(SPPP_WLOCKED(sp));

	ifp = &sp->pp_if;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (! m)
		return;
	m_reset_rcvif(m);

	if (sp->pp_flags & PP_NOFRAMING) {
		*mtod(m, uint16_t *) = htons(cp->proto);
		pkthdrlen = 2;
		lh = (struct lcp_header *)(mtod(m, uint8_t *)+2);
	} else {
		struct ppp_header *h;
		h = mtod(m, struct ppp_header *);
		h->address = PPP_ALLSTATIONS;		/* broadcast address */
		h->control = PPP_UI;			/* Unnumbered Info */
		h->protocol = htons(cp->proto);
		pkthdrlen = PPP_HEADER_LEN;

		lh = (struct lcp_header *)(h + 1);
	}

	lh->type = type;
	lh->ident = id;
	p = (u_char *)(lh + 1);

	va_start(ap, id);
	len = 0;

	while ((mlen = (unsigned int)va_arg(ap, size_t)) != 0) {
		msg = va_arg(ap, const char *);
		len += mlen;
		if (len > MHLEN - pkthdrlen - LCP_HEADER_LEN) {
			va_end(ap);
			m_freem(m);
			return;
		}

		memcpy(p, msg, mlen);
		p += mlen;
	}
	va_end(ap);

	m->m_pkthdr.len = m->m_len = pkthdrlen + LCP_HEADER_LEN + len;
	lh->len = htons(LCP_HEADER_LEN + len);

	if (sppp_debug_enabled(sp)) {
		char abuf[SPPP_AUTHTYPE_NAMELEN];
		const char *authname;

		authname = sppp_auth_type_name(abuf,
		    sizeof(abuf), cp->proto, lh->type);
		SPPP_LOG(sp, LOG_DEBUG, "%s output <%s id=0x%x len=%d",
		    cp->name, authname,
		    lh->ident, ntohs(lh->len));
		if (len)
			sppp_print_bytes((u_char *)(lh + 1), len);
		addlog(">\n");
	}
	if (IF_QFULL(&sp->pp_cpq)) {
		IF_DROP(&sp->pp_fastq);
		IF_DROP(&ifp->if_snd);
		m_freem(m);
		if_statinc(ifp, if_oerrors);
		return;
	}

	if_statadd(ifp, if_obytes, m->m_pkthdr.len + sp->pp_framebytes);
	IF_ENQUEUE(&sp->pp_cpq, m);

	if (! (ifp->if_flags & IFF_OACTIVE)) {
		SPPP_UNLOCK(sp);
		if_start_lock(ifp);
		SPPP_LOCK(sp, RW_WRITER);
	}
}

static int
sppp_auth_role(const struct cp *cp, struct sppp *sp)
{
	int role;

	role = SPPP_AUTH_NOROLE;

	if (sp->hisauth.proto == cp->proto &&
	    ISSET(sp->lcp.opts, SPPP_LCP_OPT_AUTH_PROTO))
		SET(role, SPPP_AUTH_SERV);

	if (sp->myauth.proto == cp->proto)
		SET(role, SPPP_AUTH_PEER);

	return role;
}

static void
sppp_auth_to_event(struct sppp *sp, void *xcp)
{
	const struct cp *cp = xcp;
	bool override;
	int state;

	KASSERT(SPPP_WLOCKED(sp));
	KASSERT(!cpu_softintr_p());

	override = false;
	state = sp->scp[cp->protoidx].state;

	if (sp->scp[cp->protoidx].rst_counter > 0) {
		/* override TO+ event */
		switch (state) {
		case STATE_OPENED:
			if ((sp->hisauth.flags & SPPP_AUTHFLAG_NORECHALLENGE) == 0) {
				override = true;
				sp->chap.rechallenging = true;
				sp->chap.response_rcvd = false;
				sppp_cp_change_state(cp, sp, STATE_REQ_SENT);
				cp->scr(sp);
			}
			break;

		case STATE_ACK_RCVD:
			override = true;
			cp->scr(sp);
			callout_schedule(&sp->scp[cp->protoidx].ch, sp->lcp.timeout);
			break;
		}
	}

	if (override) {
		SPPP_DLOG(sp, "%s TO(%s) rst_counter = %d\n",
		    cp->name, sppp_state_name(state),
		    sp->scp[cp->protoidx].rst_counter);
		sp->scp[cp->protoidx].rst_counter--;
	} else {
		sppp_to_event(sp, xcp);
	}
}

static void
sppp_auth_screply(const struct cp *cp, struct sppp *sp, u_char ctype,
    uint8_t ident, size_t _mlen __unused, void *_msg __unused)
{
	static const char *succmsg = "Welcome!";
	static const char *failmsg = "Failed...";
	const char *msg;
	u_char type, mlen;

	KASSERT(SPPP_WLOCKED(sp));

	if (!ISSET(sppp_auth_role(cp, sp), SPPP_AUTH_SERV))
		return;

	if (ctype == CONF_ACK) {
		type = cp->proto == PPP_CHAP ? CHAP_SUCCESS : PAP_ACK;
		msg = succmsg;
		mlen = sizeof(succmsg) - 1;

		sp->pp_auth_failures = 0;
	} else {
		type = cp->proto == PPP_CHAP ? CHAP_FAILURE : PAP_NAK;
		msg = failmsg;
		mlen = sizeof(failmsg) - 1;

		/* shutdown LCP if auth failed */
		sppp_wq_add(sp->wq_cp, &sp->scp[IDX_LCP].work_close);
		sp->pp_auth_failures++;
	}

	sppp_auth_send(cp, sp, type, ident, mlen, (const u_char *)msg, 0);
}

/*
 * Send keepalive packets, every 10 seconds.
 */
static void
sppp_keepalive(void *dummy)
{
	struct sppp *sp;
	int s;
	time_t now;

	SPPPQ_LOCK();

	s = splnet();
	now = time_uptime;
	for (sp=spppq; sp; sp=sp->pp_next) {
		struct ifnet *ifp = NULL;

		SPPP_LOCK(sp, RW_WRITER);
		ifp = &sp->pp_if;

		/* check idle timeout */
		if ((sp->pp_idle_timeout != 0) && (ifp->if_flags & IFF_RUNNING)
		    && (sp->pp_phase == SPPP_PHASE_NETWORK)) {
		    /* idle timeout is enabled for this interface */
		    if ((now-sp->pp_last_activity) >= sp->pp_idle_timeout) {
			SPPP_DLOG(sp, "no activity for %lu seconds\n",
				(unsigned long)(now-sp->pp_last_activity));
			sppp_wq_add(sp->wq_cp, &sp->scp[IDX_LCP].work_close);
			SPPP_UNLOCK(sp);
			continue;
		    }
		}

		/* Keepalive mode disabled or channel down? */
		if (! (sp->pp_flags & PP_KEEPALIVE) ||
		    ! (ifp->if_flags & IFF_RUNNING)) {
			SPPP_UNLOCK(sp);
			continue;
		}

		/* No keepalive in PPP mode if LCP not opened yet. */
		if (sp->pp_phase < SPPP_PHASE_AUTHENTICATE) {
			SPPP_UNLOCK(sp);
			continue;
		}

		/* No echo reply, but maybe user data passed through? */
		if (sp->pp_max_noreceive != 0 &&
		    (now - sp->pp_last_receive) < sp->pp_max_noreceive) {
			sp->pp_alivecnt = 0;
			SPPP_UNLOCK(sp);
			continue;
		}

		/* No echo request */
		if (sp->pp_alive_interval == 0) {
			SPPP_UNLOCK(sp);
			continue;
		}

		/* send a ECHO_REQ once in sp->pp_alive_interval times */
		if ((sppp_keepalive_cnt % sp->pp_alive_interval) != 0) {
			SPPP_UNLOCK(sp);
			continue;
		}

		if (sp->pp_alivecnt >= sp->pp_maxalive) {
			/* No keepalive packets got.  Stop the interface. */
			if (sp->pp_flags & PP_KEEPALIVE_IFDOWN)
				sppp_wq_add(sp->wq_cp, &sp->work_ifdown);

			SPPP_LOG(sp, LOG_INFO,"LCP keepalive timed out, "
			    "going to restart the connection\n");
			sp->pp_alivecnt = 0;

			/* we are down, close all open protocols */
			sppp_wq_add(sp->wq_cp, &sp->scp[IDX_LCP].work_close);

			/* And now prepare LCP to reestablish the link, if configured to do so. */
			sp->lcp.reestablish = true;

			SPPP_UNLOCK(sp);
			continue;
		}
		if (sp->pp_alivecnt < sp->pp_maxalive)
			++sp->pp_alivecnt;
		if (sp->pp_phase >= SPPP_PHASE_AUTHENTICATE) {
			int32_t nmagic = htonl(sp->lcp.magic);
			sp->lcp.echoid = ++sp->scp[IDX_LCP].seq;
			sppp_cp_send(sp, PPP_LCP, ECHO_REQ,
				sp->lcp.echoid, 4, &nmagic);
		}

		SPPP_UNLOCK(sp);
	}
	splx(s);
	sppp_keepalive_cnt++;
	callout_reset(&keepalive_ch, hz * SPPP_KEEPALIVE_INTERVAL, sppp_keepalive, NULL);

	SPPPQ_UNLOCK();
}

#ifdef INET
/*
 * Get both IP addresses.
 */
static void
sppp_get_ip_addrs(struct sppp *sp, uint32_t *src, uint32_t *dst, uint32_t *srcmask)
{
	struct ifnet *ifp = &sp->pp_if;
	struct ifaddr *ifa;
	struct sockaddr_in *si, *sm;
	uint32_t ssrc, ddst;
	int bound, s;
	struct psref psref;

	sm = NULL;
	ssrc = ddst = 0;
	/*
	 * Pick the first AF_INET address from the list,
	 * aliases don't make any sense on a p2p link anyway.
	 */
	si = 0;
	bound = curlwp_bind();
	s = pserialize_read_enter();
	IFADDR_READER_FOREACH(ifa, ifp) {
		if (ifa->ifa_addr->sa_family == AF_INET) {
			si = (struct sockaddr_in *)ifa->ifa_addr;
			sm = (struct sockaddr_in *)ifa->ifa_netmask;
			if (si) {
				ifa_acquire(ifa, &psref);
				break;
			}
		}
	}
	pserialize_read_exit(s);
	if (ifa) {
		if (si && si->sin_addr.s_addr) {
			ssrc = si->sin_addr.s_addr;
			if (srcmask)
				*srcmask = ntohl(sm->sin_addr.s_addr);
		}

		si = (struct sockaddr_in *)ifa->ifa_dstaddr;
		if (si && si->sin_addr.s_addr)
			ddst = si->sin_addr.s_addr;
		ifa_release(ifa, &psref);
	}
	curlwp_bindx(bound);

	if (dst) *dst = ntohl(ddst);
	if (src) *src = ntohl(ssrc);
}

/*
 * Set IP addresses.  Must be called at splnet.
 * If an address is 0, leave it the way it is.
 */
static void
sppp_set_ip_addrs(struct sppp *sp)
{
	struct ifnet *ifp;
	struct ifaddr *ifa;
	struct sockaddr_in *si, *dest;
	uint32_t myaddr = 0, hisaddr = 0;
	int s;

	KASSERT(SPPP_WLOCKED(sp));

	ifp = &sp->pp_if;

	SPPP_UNLOCK(sp);
	IFNET_LOCK(ifp);
	SPPP_LOCK(sp, RW_WRITER);

	/*
	 * Pick the first AF_INET address from the list,
	 * aliases don't make any sense on a p2p link anyway.
	 */
	si = dest = NULL;
	s = pserialize_read_enter();
	IFADDR_READER_FOREACH(ifa, ifp) {
		if (ifa->ifa_addr->sa_family == AF_INET) {
			si = (struct sockaddr_in *)ifa->ifa_addr;
			dest = (struct sockaddr_in *)ifa->ifa_dstaddr;
			break;
		}
	}
	pserialize_read_exit(s);

	if ((sp->ipcp.flags & IPCP_MYADDR_DYN) && (sp->ipcp.flags & IPCP_MYADDR_SEEN))
		myaddr = sp->ipcp.req_myaddr;
	else if (si != NULL)
		myaddr = ntohl(si->sin_addr.s_addr);

	if ((sp->ipcp.flags & IPCP_HISADDR_DYN) && (sp->ipcp.flags & IPCP_HISADDR_SEEN))
		hisaddr = sp->ipcp.req_hisaddr;
	else if (dest != NULL)
		hisaddr = ntohl(dest->sin_addr.s_addr);

	if (si != NULL && dest != NULL) {
		int error;
		struct sockaddr_in new_sin = *si;
		struct sockaddr_in new_dst = *dest;

		if (myaddr != 0)
			new_sin.sin_addr.s_addr = htonl(myaddr);
		if (hisaddr != 0) {
			new_dst.sin_addr.s_addr = htonl(hisaddr);
			if (new_dst.sin_addr.s_addr != dest->sin_addr.s_addr)
				sp->ipcp.saved_hisaddr = dest->sin_addr.s_addr;
		}

		in_addrhash_remove(ifatoia(ifa));

		error = in_ifinit(ifp, ifatoia(ifa), &new_sin, &new_dst, 0);

		in_addrhash_insert(ifatoia(ifa));

		if (error) {
			SPPP_DLOG(sp, "%s: in_ifinit failed, error=%d\n",
			    __func__, error);
		} else {
			pfil_run_addrhooks(if_pfil, SIOCAIFADDR, ifa);
		}
	}

	IFNET_UNLOCK(ifp);

	sppp_notify_con(sp);
}

/*
 * Clear IP addresses.  Must be called at splnet.
 */
static void
sppp_clear_ip_addrs(struct sppp *sp)
{
	struct ifnet *ifp;
	struct ifaddr *ifa;
	struct sockaddr_in *si, *dest;
	int s;

	KASSERT(SPPP_WLOCKED(sp));

	ifp = &sp->pp_if;

	SPPP_UNLOCK(sp);
	IFNET_LOCK(ifp);
	SPPP_LOCK(sp, RW_WRITER);

	/*
	 * Pick the first AF_INET address from the list,
	 * aliases don't make any sense on a p2p link anyway.
	 */
	si = dest = NULL;
	s = pserialize_read_enter();
	IFADDR_READER_FOREACH(ifa, ifp) {
		if (ifa->ifa_addr->sa_family == AF_INET) {
			si = (struct sockaddr_in *)ifa->ifa_addr;
			dest = (struct sockaddr_in *)ifa->ifa_dstaddr;
			break;
		}
	}
	pserialize_read_exit(s);

	if (si != NULL) {
		struct sockaddr_in new_sin = *si;
		struct sockaddr_in new_dst = *dest;
		int error;

		if (sp->ipcp.flags & IPCP_MYADDR_DYN)
			new_sin.sin_addr.s_addr = 0;
		if (sp->ipcp.flags & IPCP_HISADDR_DYN &&
		    ntohl(sp->ipcp.saved_hisaddr) != 0)
			new_dst.sin_addr.s_addr = sp->ipcp.saved_hisaddr;

		in_addrhash_remove(ifatoia(ifa));

		error = in_ifinit(ifp, ifatoia(ifa), &new_sin, &new_dst, 0);

		in_addrhash_insert(ifatoia(ifa));

		if (error) {
			SPPP_DLOG(sp, "%s: in_ifinit failed, error=%d\n",
			    __func__, error);
		} else {
			pfil_run_addrhooks(if_pfil, SIOCAIFADDR, ifa);
		}
	}

	IFNET_UNLOCK(ifp);
}
#endif

#ifdef INET6
/*
 * Get both IPv6 addresses.
 */
static void
sppp_get_ip6_addrs(struct sppp *sp, struct in6_addr *src, struct in6_addr *dst,
		   struct in6_addr *srcmask)
{
	struct ifnet *ifp = &sp->pp_if;
	struct ifaddr *ifa;
	struct sockaddr_in6 *si, *sm;
	struct in6_addr ssrc, ddst;
	int bound, s;
	struct psref psref;

	sm = NULL;
	memset(&ssrc, 0, sizeof(ssrc));
	memset(&ddst, 0, sizeof(ddst));
	/*
	 * Pick the first link-local AF_INET6 address from the list,
	 * aliases don't make any sense on a p2p link anyway.
	 */
	si = 0;
	bound = curlwp_bind();
	s = pserialize_read_enter();
	IFADDR_READER_FOREACH(ifa, ifp) {
		if (ifa->ifa_addr->sa_family == AF_INET6) {
			si = (struct sockaddr_in6 *)ifa->ifa_addr;
			sm = (struct sockaddr_in6 *)ifa->ifa_netmask;
			if (si && IN6_IS_ADDR_LINKLOCAL(&si->sin6_addr)) {
				ifa_acquire(ifa, &psref);
				break;
			}
		}
	}
	pserialize_read_exit(s);

	if (ifa) {
		if (si && !IN6_IS_ADDR_UNSPECIFIED(&si->sin6_addr)) {
			memcpy(&ssrc, &si->sin6_addr, sizeof(ssrc));
			if (srcmask) {
				memcpy(srcmask, &sm->sin6_addr,
				    sizeof(*srcmask));
			}
		}

		si = (struct sockaddr_in6 *)ifa->ifa_dstaddr;
		if (si && !IN6_IS_ADDR_UNSPECIFIED(&si->sin6_addr))
			memcpy(&ddst, &si->sin6_addr, sizeof(ddst));
		ifa_release(ifa, &psref);
	}
	curlwp_bindx(bound);

	if (dst)
		memcpy(dst, &ddst, sizeof(*dst));
	if (src)
		memcpy(src, &ssrc, sizeof(*src));
}

#ifdef IPV6CP_MYIFID_DYN
/*
 * Generate random ifid.
 */
static void
sppp_gen_ip6_addr(struct sppp *sp, struct in6_addr *addr)
{
	/* TBD */
}

/*
 * Set my IPv6 address.  Must be called at splnet.
 */
static void
sppp_set_ip6_addr(struct sppp *sp, const struct in6_addr *src)
{
	struct ifnet *ifp;
	struct ifaddr *ifa;
	struct sockaddr_in6 *sin6;
	int s;
	struct psref psref;

	KASSERT(SPPP_WLOCKED(sp));

	ifp = &sp->pp_if;

	SPPP_UNLOCK(sp);
	IFNET_LOCK(ifp);
	SPPP_LOCK(sp, RW_WRITER);

	/*
	 * Pick the first link-local AF_INET6 address from the list,
	 * aliases don't make any sense on a p2p link anyway.
	 */

	sin6 = NULL;
	s = pserialize_read_enter();
	IFADDR_READER_FOREACH(ifa, ifp)
	{
		if (ifa->ifa_addr->sa_family == AF_INET6)
		{
			sin6 = (struct sockaddr_in6 *)ifa->ifa_addr;
			if (sin6 && IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
				ifa_acquire(ifa, &psref);
				break;
			}
		}
	}
	pserialize_read_exit(s);

	if (ifa && sin6)
	{
		int error;
		struct sockaddr_in6 new_sin6 = *sin6;

		memcpy(&new_sin6.sin6_addr, src, sizeof(new_sin6.sin6_addr));
		error = in6_ifinit(ifp, ifatoia6(ifa), &new_sin6, 1);
		if (error) {
			SPPP_DLOG(sp, "%s: in6_ifinit failed, error=%d\n",
			    __func__, error);
		} else {
			pfil_run_addrhooks(if_pfil, SIOCAIFADDR_IN6, ifa);
		}
		ifa_release(ifa, &psref);
	}

	IFNET_UNLOCK(ifp);
}
#endif

/*
 * Suggest a candidate address to be used by peer.
 */
static void
sppp_suggest_ip6_addr(struct sppp *sp, struct in6_addr *suggest)
{
	struct in6_addr myaddr;
	struct timeval tv;

	sppp_get_ip6_addrs(sp, &myaddr, 0, 0);

	myaddr.s6_addr[8] &= ~0x02;	/* u bit to "local" */
	microtime(&tv);
	if ((tv.tv_usec & 0xff) == 0 && (tv.tv_sec & 0xff) == 0) {
		myaddr.s6_addr[14] ^= 0xff;
		myaddr.s6_addr[15] ^= 0xff;
	} else {
		myaddr.s6_addr[14] ^= (tv.tv_usec & 0xff);
		myaddr.s6_addr[15] ^= (tv.tv_sec & 0xff);
	}
	if (suggest)
		memcpy(suggest, &myaddr, sizeof(myaddr));
}
#endif /*INET6*/

/*
 * Process ioctl requests specific to the PPP interface.
 * Permissions have already been checked.
 */
static int
sppp_params(struct sppp *sp, u_long cmd, void *data)
{
	switch (cmd) {
	case SPPPGETAUTHCFG:
	    {
		struct spppauthcfg *cfg = (struct spppauthcfg *)data;
		int error;
		size_t len;

		SPPP_LOCK(sp, RW_READER);

		cfg->myauthflags = sp->myauth.flags;
		cfg->hisauthflags = sp->hisauth.flags;
		strlcpy(cfg->ifname, sp->pp_if.if_xname, sizeof(cfg->ifname));
		cfg->hisauth = sppp_proto2authproto(sp->hisauth.proto);
		cfg->myauth = sppp_proto2authproto(sp->myauth.proto);
		if (cfg->myname_length == 0) {
		    if (sp->myauth.name != NULL)
			cfg->myname_length = sp->myauth.name_len + 1;
		} else {
		    if (sp->myauth.name == NULL) {
			cfg->myname_length = 0;
		    } else {
			len = sp->myauth.name_len + 1;

			if (cfg->myname_length < len) {
				SPPP_UNLOCK(sp);
				return (ENAMETOOLONG);
			}
			error = copyout(sp->myauth.name, cfg->myname, len);
			if (error) {
				SPPP_UNLOCK(sp);
				return error;
			}
		    }
		}
		if (cfg->hisname_length == 0) {
		    if (sp->hisauth.name != NULL)
			cfg->hisname_length = sp->hisauth.name_len + 1;
		} else {
		    if (sp->hisauth.name == NULL) {
		    	cfg->hisname_length = 0;
		    } else {
			len = sp->hisauth.name_len + 1;

			if (cfg->hisname_length < len) {
				SPPP_UNLOCK(sp);
				return (ENAMETOOLONG);
			}
			error = copyout(sp->hisauth.name, cfg->hisname, len);
			if (error) {
				SPPP_UNLOCK(sp);
				return error;
			}
		    }
		}
		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPSETAUTHCFG:
	    {
		struct spppauthcfg *cfg = (struct spppauthcfg *)data;
		int error;

		SPPP_LOCK(sp, RW_WRITER);

		if (sp->myauth.name) {
			free(sp->myauth.name, M_DEVBUF);
			sp->myauth.name = NULL;
		}
		if (sp->myauth.secret) {
			free(sp->myauth.secret, M_DEVBUF);
			sp->myauth.secret = NULL;
		}
		if (sp->hisauth.name) {
			free(sp->hisauth.name, M_DEVBUF);
			sp->hisauth.name = NULL;
		}
		if (sp->hisauth.secret) {
			free(sp->hisauth.secret, M_DEVBUF);
			sp->hisauth.secret = NULL;
		}

		if (cfg->hisname != NULL && cfg->hisname_length > 0) {
			if (cfg->hisname_length >= MCLBYTES) {
				SPPP_UNLOCK(sp);
				return (ENAMETOOLONG);
			}
			sp->hisauth.name = malloc(cfg->hisname_length, M_DEVBUF, M_WAITOK);
			error = copyin(cfg->hisname, sp->hisauth.name, cfg->hisname_length);
			if (error) {
				free(sp->hisauth.name, M_DEVBUF);
				sp->hisauth.name = NULL;
				SPPP_UNLOCK(sp);
				return error;
			}
			sp->hisauth.name_len = cfg->hisname_length - 1;
			sp->hisauth.name[sp->hisauth.name_len] = 0;
		}
		if (cfg->hissecret != NULL && cfg->hissecret_length > 0) {
			if (cfg->hissecret_length >= MCLBYTES) {
				SPPP_UNLOCK(sp);
				return (ENAMETOOLONG);
			}
			sp->hisauth.secret = malloc(cfg->hissecret_length,
			    M_DEVBUF, M_WAITOK);
			error = copyin(cfg->hissecret, sp->hisauth.secret,
			    cfg->hissecret_length);
			if (error) {
				free(sp->hisauth.secret, M_DEVBUF);
				sp->hisauth.secret = NULL;
				SPPP_UNLOCK(sp);
				return error;
			}
			sp->hisauth.secret_len = cfg->hissecret_length - 1;
			sp->hisauth.secret[sp->hisauth.secret_len] = 0;
		}
		if (cfg->myname != NULL && cfg->myname_length > 0) {
			if (cfg->myname_length >= MCLBYTES) {
				SPPP_UNLOCK(sp);
				return (ENAMETOOLONG);
			}
			sp->myauth.name = malloc(cfg->myname_length, M_DEVBUF, M_WAITOK);
			error = copyin(cfg->myname, sp->myauth.name, cfg->myname_length);
			if (error) {
				free(sp->myauth.name, M_DEVBUF);
				sp->myauth.name = NULL;
				SPPP_UNLOCK(sp);
				return error;
			}
			sp->myauth.name_len = cfg->myname_length - 1;
			sp->myauth.name[sp->myauth.name_len] = 0;
		}
		if (cfg->mysecret != NULL && cfg->mysecret_length > 0) {
			if (cfg->mysecret_length >= MCLBYTES) {
				SPPP_UNLOCK(sp);
				return (ENAMETOOLONG);
			}
			sp->myauth.secret = malloc(cfg->mysecret_length,
			    M_DEVBUF, M_WAITOK);
			error = copyin(cfg->mysecret, sp->myauth.secret,
			    cfg->mysecret_length);
			if (error) {
				free(sp->myauth.secret, M_DEVBUF);
				sp->myauth.secret = NULL;
				SPPP_UNLOCK(sp);
				return error;
			}
			sp->myauth.secret_len = cfg->mysecret_length - 1;
			sp->myauth.secret[sp->myauth.secret_len] = 0;
		}
		sp->myauth.flags = cfg->myauthflags;
		if (cfg->myauth != SPPP_AUTHPROTO_NOCHG) {
			sp->myauth.proto = sppp_authproto2proto(cfg->myauth);
		}
		sp->hisauth.flags = cfg->hisauthflags;
		if (cfg->hisauth != SPPP_AUTHPROTO_NOCHG) {
			sp->hisauth.proto = sppp_authproto2proto(cfg->hisauth);
		}
		sp->pp_auth_failures = 0;
		if (sp->hisauth.proto != PPP_NOPROTO)
			SET(sp->lcp.opts, SPPP_LCP_OPT_AUTH_PROTO);
		else
			CLR(sp->lcp.opts, SPPP_LCP_OPT_AUTH_PROTO);

		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPGETLCPCFG:
	    {
		struct sppplcpcfg *lcpp = (struct sppplcpcfg *)data;

		SPPP_LOCK(sp, RW_READER);
		lcpp->lcp_timeout = sp->lcp.timeout;
		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPSETLCPCFG:
	    {
		struct sppplcpcfg *lcpp = (struct sppplcpcfg *)data;

		SPPP_LOCK(sp, RW_WRITER);
		sp->lcp.timeout = lcpp->lcp_timeout;
		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPGETNCPCFG:
	    {
		struct spppncpcfg *ncpp = (struct spppncpcfg *) data;

		SPPP_LOCK(sp, RW_READER);
		ncpp->ncp_flags = sp->pp_ncpflags;
		SPPP_UNLOCK(sp);
	    }
		break;
	case SPPPSETNCPCFG:
	    {
		struct spppncpcfg *ncpp = (struct spppncpcfg *) data;

		SPPP_LOCK(sp, RW_WRITER);
		sp->pp_ncpflags = ncpp->ncp_flags;
		SPPP_UNLOCK(sp);
	    }
		break;
	case SPPPGETSTATUS:
	    {
		struct spppstatus *status = (struct spppstatus *)data;

		SPPP_LOCK(sp, RW_READER);
		status->phase = sp->pp_phase;
		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPGETSTATUSNCP:
	    {
		struct spppstatusncp *status = (struct spppstatusncp *)data;

		SPPP_LOCK(sp, RW_READER);
		status->phase = sp->pp_phase;
		status->ncpup = sppp_cp_check(sp, CP_NCP);
		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPGETIDLETO:
	    {
	    	struct spppidletimeout *to = (struct spppidletimeout *)data;

		SPPP_LOCK(sp, RW_READER);
		to->idle_seconds = sp->pp_idle_timeout;
		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPSETIDLETO:
	    {
	    	struct spppidletimeout *to = (struct spppidletimeout *)data;

		SPPP_LOCK(sp, RW_WRITER);
		sp->pp_idle_timeout = to->idle_seconds;
		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPSETAUTHFAILURE:
	    {
	    	struct spppauthfailuresettings *afsettings =
		    (struct spppauthfailuresettings *)data;

		SPPP_LOCK(sp, RW_WRITER);
		sp->pp_max_auth_fail = afsettings->max_failures;
		sp->pp_auth_failures = 0;
		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPGETAUTHFAILURES:
	    {
	    	struct spppauthfailurestats *stats = (struct spppauthfailurestats *)data;

		SPPP_LOCK(sp, RW_READER);
		stats->auth_failures = sp->pp_auth_failures;
		stats->max_failures = sp->pp_max_auth_fail;
		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPSETDNSOPTS:
	    {
		struct spppdnssettings *req = (struct spppdnssettings *)data;

		SPPP_LOCK(sp, RW_WRITER);
		sp->query_dns = req->query_dns & 3;
		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPGETDNSOPTS:
	    {
		struct spppdnssettings *req = (struct spppdnssettings *)data;

		SPPP_LOCK(sp, RW_READER);
		req->query_dns = sp->query_dns;
		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPGETDNSADDRS:
	    {
		struct spppdnsaddrs *addrs = (struct spppdnsaddrs *)data;

		SPPP_LOCK(sp, RW_READER);
		memcpy(&addrs->dns, &sp->dns_addrs, sizeof addrs->dns);
		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPGETKEEPALIVE:
	    {
	    	struct spppkeepalivesettings *settings =
		     (struct spppkeepalivesettings*)data;

		SPPP_LOCK(sp, RW_READER);
		settings->maxalive = sp->pp_maxalive;
		settings->max_noreceive = sp->pp_max_noreceive;
		settings->alive_interval = sp->pp_alive_interval;
		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPSETKEEPALIVE:
	    {
	    	struct spppkeepalivesettings *settings =
		     (struct spppkeepalivesettings*)data;

		SPPP_LOCK(sp, RW_WRITER);
		sp->pp_maxalive = settings->maxalive;
		sp->pp_max_noreceive = settings->max_noreceive;
		sp->pp_alive_interval = settings->alive_interval;
		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPGETLCPSTATUS:
	    {
		struct sppplcpstatus *status =
		    (struct sppplcpstatus *)data;

		SPPP_LOCK(sp, RW_READER);
		status->state = sp->scp[IDX_LCP].state;
		status->opts = sp->lcp.opts;
		status->magic = sp->lcp.magic;
		status->mru = sp->lcp.mru;
		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPGETIPCPSTATUS:
	    {
		struct spppipcpstatus *status =
		    (struct spppipcpstatus *)data;
		u_int32_t myaddr;

		SPPP_LOCK(sp, RW_READER);
		status->state = sp->scp[IDX_IPCP].state;
		status->opts = sp->ipcp.opts;
#ifdef INET
		sppp_get_ip_addrs(sp, &myaddr, 0, 0);
#else
		myaddr = 0;
#endif
		status->myaddr = ntohl(myaddr);
		SPPP_UNLOCK(sp);
	    }
	    break;
	case SPPPGETIPV6CPSTATUS:
	    {
		struct spppipv6cpstatus *status =
		    (struct spppipv6cpstatus *)data;

		SPPP_LOCK(sp, RW_READER);
		status->state = sp->scp[IDX_IPV6CP].state;
		memcpy(status->my_ifid, sp->ipv6cp.my_ifid,
		    sizeof(status->my_ifid));
		memcpy(status->his_ifid, sp->ipv6cp.his_ifid,
		    sizeof(status->his_ifid));
		SPPP_UNLOCK(sp);
	    }
	    break;
	default:
	    {
		int ret;

		MODULE_HOOK_CALL(sppp_params_50_hook, (sp, cmd, data),
		    enosys(), ret);
		if (ret != ENOSYS)
			return ret;
		return (EINVAL);
	    }
	}
	return (0);
}

static void
sppp_phase_network(struct sppp *sp)
{
	int i;

	KASSERT(SPPP_WLOCKED(sp));

	sppp_change_phase(sp, SPPP_PHASE_NETWORK);

	/* Notify NCPs now. */
	for (i = 0; i < IDX_COUNT; i++)
		if ((cps[i])->flags & CP_NCP)
			sppp_wq_add(sp->wq_cp, &sp->scp[i].work_open);
}

static const char *
sppp_cp_type_name(char *buf, size_t buflen, u_char type)
{

	switch (type) {
	case CONF_REQ:   return "conf-req";
	case CONF_ACK:   return "conf-ack";
	case CONF_NAK:   return "conf-nak";
	case CONF_REJ:   return "conf-rej";
	case TERM_REQ:   return "term-req";
	case TERM_ACK:   return "term-ack";
	case CODE_REJ:   return "code-rej";
	case PROTO_REJ:  return "proto-rej";
	case ECHO_REQ:   return "echo-req";
	case ECHO_REPLY: return "echo-reply";
	case DISC_REQ:   return "discard-req";
	}
	if (buf != NULL)
		snprintf(buf, buflen, "0x%02x", type);
	return buf;
}

static const char *
sppp_auth_type_name(char *buf, size_t buflen, u_short proto, u_char type)
{
	const char *name;

	switch (proto) {
	case PPP_CHAP:
		switch (type) {
		case CHAP_CHALLENGE:	return "challenge";
		case CHAP_RESPONSE:	return "response";
		case CHAP_SUCCESS:	return "success";
		case CHAP_FAILURE:	return "failure";
		default:		name = "chap"; break;
		}
		break;

	case PPP_PAP:
		switch (type) {
		case PAP_REQ:		return "req";
		case PAP_ACK:		return "ack";
		case PAP_NAK:		return "nak";
		default:		name = "pap";	break;
		}
		break;

	default:
		name = "bad";
		break;
	}

	if (buf != NULL)
		snprintf(buf, buflen, "%s(%#x) 0x%02x", name, proto, type);
	return buf;
}

static const char *
sppp_lcp_opt_name(char *buf, size_t buflen, u_char opt)
{

	switch (opt) {
	case LCP_OPT_MRU:		return "mru";
	case LCP_OPT_ASYNC_MAP:		return "async-map";
	case LCP_OPT_AUTH_PROTO:	return "auth-proto";
	case LCP_OPT_QUAL_PROTO:	return "qual-proto";
	case LCP_OPT_MAGIC:		return "magic";
	case LCP_OPT_PROTO_COMP:	return "proto-comp";
	case LCP_OPT_ADDR_COMP:		return "addr-comp";
	case LCP_OPT_SELF_DESC_PAD:	return "sdpad";
	case LCP_OPT_CALL_BACK:		return "callback";
	case LCP_OPT_COMPOUND_FRMS:	return "cmpd-frms";
	case LCP_OPT_MP_MRRU:		return "mrru";
	case LCP_OPT_MP_SSNHF:		return "mp-ssnhf";
	case LCP_OPT_MP_EID:		return "mp-eid";
	}
	if (buf != NULL)
		snprintf(buf, buflen, "0x%02x", opt);
	return buf;
}

static const char *
sppp_ipcp_opt_name(char *buf, size_t buflen, u_char opt)
{

	switch (opt) {
	case IPCP_OPT_ADDRESSES:	return "addresses";
	case IPCP_OPT_COMPRESSION:	return "compression";
	case IPCP_OPT_ADDRESS:		return "address";
	case IPCP_OPT_PRIMDNS:		return "primdns";
	case IPCP_OPT_SECDNS:		return "secdns";
	}
	if (buf != NULL)
		snprintf(buf, buflen, "0x%02x", opt);
	return buf;
}

#ifdef INET6
static const char *
sppp_ipv6cp_opt_name(char *buf, size_t buflen, u_char opt)
{

	switch (opt) {
	case IPV6CP_OPT_IFID:		return "ifid";
	case IPV6CP_OPT_COMPRESSION:	return "compression";
	}
	if (buf != NULL)
		snprintf(buf, buflen, "0x%02x", opt);
	return buf;
}
#endif

static const char *
sppp_state_name(int state)
{

	switch (state) {
	case STATE_INITIAL:	return "initial";
	case STATE_STARTING:	return "starting";
	case STATE_CLOSED:	return "closed";
	case STATE_STOPPED:	return "stopped";
	case STATE_CLOSING:	return "closing";
	case STATE_STOPPING:	return "stopping";
	case STATE_REQ_SENT:	return "req-sent";
	case STATE_ACK_RCVD:	return "ack-rcvd";
	case STATE_ACK_SENT:	return "ack-sent";
	case STATE_OPENED:	return "opened";
	}
	return "illegal";
}

static const char *
sppp_phase_name(int phase)
{

	switch (phase) {
	case SPPP_PHASE_DEAD:		return "dead";
	case SPPP_PHASE_ESTABLISH:	return "establish";
	case SPPP_PHASE_TERMINATE:	return "terminate";
	case SPPP_PHASE_AUTHENTICATE: 	return "authenticate";
	case SPPP_PHASE_NETWORK:	return "network";
	}
	return "illegal";
}

static const char *
sppp_proto_name(char *buf, size_t buflen, u_short proto)
{

	switch (proto) {
	case PPP_LCP:	return "lcp";
	case PPP_IPCP:	return "ipcp";
	case PPP_PAP:	return "pap";
	case PPP_CHAP:	return "chap";
	case PPP_IPV6CP: return "ipv6cp";
	}
	if (buf != NULL) {
		snprintf(buf, sizeof(buf), "0x%04x",
		    (unsigned)proto);
	}
	return buf;
}

static void
sppp_print_bytes(const u_char *p, u_short len)
{
	addlog(" %02x", *p++);
	while (--len > 0)
		addlog("-%02x", *p++);
}

static void
sppp_print_string(const char *p, u_short len)
{
	u_char c;

	while (len-- > 0) {
		c = *p++;
		/*
		 * Print only ASCII chars directly.  RFC 1994 recommends
		 * using only them, but we don't rely on it.  */
		if (c < ' ' || c > '~')
			addlog("\\x%x", c);
		else
			addlog("%c", c);
	}
}

static const char *
sppp_dotted_quad(char *buf, size_t buflen, uint32_t addr)
{

	if (buf != NULL) {
		snprintf(buf, buflen, "%u.%u.%u.%u",
			(unsigned int)((addr >> 24) & 0xff),
			(unsigned int)((addr >> 16) & 0xff),
			(unsigned int)((addr >> 8) & 0xff),
			(unsigned int)(addr & 0xff));
	}
	return buf;
}

/* a dummy, used to drop uninteresting events */
static void
sppp_null(struct sppp *unused)
{
	/* do just nothing */
}

static void
sppp_tls(const struct cp *cp, struct sppp *sp)
{

	SPPP_DLOG(sp, "%s tls\n", cp->name);

	/* notify lcp that is lower layer */
	sp->lcp.protos |= (1 << cp->protoidx);
}

static void
sppp_tlf(const struct cp *cp, struct sppp *sp)
{

	SPPP_DLOG(sp, "%s tlf\n", cp->name);

	/* notify lcp that is lower layer */
	sp->lcp.protos &= ~(1 << cp->protoidx);

	/* cleanup */
	if (sp->scp[cp->protoidx].mbuf_confreq != NULL) {
		m_freem(sp->scp[cp->protoidx].mbuf_confreq);
		sp->scp[cp->protoidx].mbuf_confreq = NULL;
	}
	if (sp->scp[cp->protoidx].mbuf_confnak != NULL) {
		m_freem(sp->scp[cp->protoidx].mbuf_confnak);
		sp->scp[cp->protoidx].mbuf_confnak = NULL;
	}

	sppp_lcp_check_and_close(sp);
}

static void
sppp_screply(const struct cp *cp, struct sppp *sp, u_char type,
    uint8_t ident, size_t msglen, void *msg)
{

	if (msglen == 0)
		return;

	switch (type) {
	case CONF_ACK:
	case CONF_NAK:
	case CONF_REJ:
		break;
	default:
		return;
	}

	if (sppp_debug_enabled(sp)) {
		char tbuf[SPPP_CPTYPE_NAMELEN];
		const char *cpname;

		cpname = sppp_cp_type_name(tbuf, sizeof(tbuf), type);
		SPPP_LOG(sp, LOG_DEBUG, "send %s\n", cpname);
	}

	sppp_cp_send(sp, cp->proto, type, ident, msglen, msg);
}

static void
sppp_ifdown(struct sppp *sp, void *xcp __unused)
{

	SPPP_UNLOCK(sp);
	if_down(&sp->pp_if);
	IF_PURGE(&sp->pp_cpq);
	SPPP_LOCK(sp, RW_WRITER);
}

static void
sppp_notify_up(struct sppp *sp)
{

	sppp_wq_add(sp->wq_cp, &sp->scp[IDX_LCP].work_up);
}

static void
sppp_notify_down(struct sppp *sp)
{

	sppp_wq_add(sp->wq_cp, &sp->scp[IDX_LCP].work_down);
}

static void
sppp_notify_tls_wlocked(struct sppp *sp)
{

	KASSERT(SPPP_WLOCKED(sp));

	if (!sp->pp_tls)
		return;

	SPPP_UNLOCK(sp);
	sp->pp_tls(sp);
	SPPP_LOCK(sp, RW_WRITER);
}

static void
sppp_notify_tlf_wlocked(struct sppp *sp)
{

	KASSERT(SPPP_WLOCKED(sp));

	if (!sp->pp_tlf)
		return;

	SPPP_UNLOCK(sp);
	sp->pp_tlf(sp);
	SPPP_LOCK(sp, RW_WRITER);
}

static void
sppp_notify_con(struct sppp *sp)
{

	if (!sp->pp_con)
		return;

	sp->pp_con(sp);
}

#ifdef INET6
static void
sppp_notify_con_wlocked(struct sppp *sp)
{

	KASSERT(SPPP_WLOCKED(sp));

	SPPP_UNLOCK(sp);
	sppp_notify_con(sp);
	SPPP_LOCK(sp, RW_WRITER);

}
#endif

static void
sppp_notify_chg_wlocked(struct sppp *sp)
{

	KASSERT(SPPP_WLOCKED(sp));

	if (!sp->pp_chg)
		return;

	SPPP_UNLOCK(sp);
	sp->pp_chg(sp, sp->pp_phase);
	SPPP_LOCK(sp, RW_WRITER);
}

static void
sppp_wq_work(struct work *wk, void *xsp)
{
	struct sppp *sp;
	struct sppp_work *work;

	sp = xsp;
	work = container_of(wk, struct sppp_work, work);
	atomic_cas_uint(&work->state, SPPP_WK_BUSY, SPPP_WK_FREE);

	SPPP_LOCK(sp, RW_WRITER);
	work->func(sp, work->arg);
	SPPP_UNLOCK(sp);
}

static struct workqueue *
sppp_wq_create(struct sppp *sp, const char *xnamebuf, pri_t prio, int ipl, int flags)
{
	struct workqueue *wq;
	int error;

	error = workqueue_create(&wq, xnamebuf, sppp_wq_work,
	    (void *)sp, prio, ipl, flags);
	if (error) {
		panic("%s: workqueue_create failed [%s, %d]\n",
		    sp->pp_if.if_xname, xnamebuf, error);
	}

	return wq;
}

static void
sppp_wq_destroy(struct sppp *sp __unused, struct workqueue *wq)
{

	workqueue_destroy(wq);
}

static void
sppp_wq_set(struct sppp_work *work,
    void (*func)(struct sppp *, void *), void *arg)
{

	work->func = func;
	work->arg = arg;
}

static void
sppp_wq_add(struct workqueue *wq, struct sppp_work *work)
{

	if (atomic_cas_uint(&work->state, SPPP_WK_FREE, SPPP_WK_BUSY)
	    != SPPP_WK_FREE)
		return;

	KASSERT(work->func != NULL);
	kpreempt_disable();
	workqueue_enqueue(wq, &work->work, NULL);
	kpreempt_enable();
}
static void
sppp_wq_wait(struct workqueue *wq, struct sppp_work *work)
{

	atomic_swap_uint(&work->state, SPPP_WK_UNAVAIL);
	workqueue_wait(wq, &work->work);
}

/*
 * This file is large.  Tell emacs to highlight it nevertheless.
 *
 * Local Variables:
 * hilit-auto-highlight-maxout: 120000
 * End:
 */

/*
 * Module glue
 */
MODULE(MODULE_CLASS_MISC, sppp_subr, NULL);

static int
sppp_subr_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
	case MODULE_CMD_FINI:
		return 0;
	case MODULE_CMD_STAT:
	case MODULE_CMD_AUTOUNLOAD:
	default:
		return ENOTTY;
	}
}
