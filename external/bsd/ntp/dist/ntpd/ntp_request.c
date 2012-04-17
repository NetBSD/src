/*	$NetBSD: ntp_request.c,v 1.5.2.1 2012/04/17 00:03:47 yamt Exp $	*/

/*
 * ntp_request.c - respond to information requests
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ntpd.h"
#include "ntp_io.h"
#include "ntp_request.h"
#include "ntp_control.h"
#include "ntp_refclock.h"
#include "ntp_if.h"
#include "ntp_stdlib.h"
#include "ntp_assert.h"

#include <stdio.h>
#include <stddef.h>
#include <signal.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#include <arpa/inet.h>

#include "recvbuff.h"

#ifdef KERNEL_PLL
#include "ntp_syscall.h"
#endif /* KERNEL_PLL */

/*
 * Structure to hold request procedure information
 */
#define	NOAUTH	0
#define	AUTH	1

#define	NO_REQUEST	(-1)
/*
 * Because we now have v6 addresses in the messages, we need to compensate
 * for the larger size.  Therefore, we introduce the alternate size to 
 * keep us friendly with older implementations.  A little ugly.
 */
static int client_v6_capable = 0;   /* the client can handle longer messages */

#define v6sizeof(type)	(client_v6_capable ? sizeof(type) : v4sizeof(type))

struct req_proc {
	short request_code;	/* defined request code */
	short needs_auth;	/* true when authentication needed */
	short sizeofitem;	/* size of request data item (older size)*/
	short v6_sizeofitem;	/* size of request data item (new size)*/
	void (*handler) (sockaddr_u *, struct interface *,
			   struct req_pkt *);	/* routine to handle request */
};

/*
 * Universal request codes
 */
static	struct req_proc univ_codes[] = {
	{ NO_REQUEST,		NOAUTH,	 0,	0, NULL }
};

static	void	req_ack	(sockaddr_u *, struct interface *, struct req_pkt *, int);
static	char *	prepare_pkt	(sockaddr_u *, struct interface *,
				 struct req_pkt *, size_t);
static	char *	more_pkt	(void);
static	void	flush_pkt	(void);
static	void	peer_list	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	peer_list_sum	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	peer_info	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	peer_stats	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	sys_info	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	sys_stats	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	mem_stats	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	io_stats	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	timer_stats	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	loop_info	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	do_conf		(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	do_unconf	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	set_sys_flag	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	clr_sys_flag	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	setclr_flags	(sockaddr_u *, struct interface *, struct req_pkt *, u_long);
static	void	list_restrict	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	do_resaddflags	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	do_ressubflags	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	do_unrestrict	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	do_restrict	(sockaddr_u *, struct interface *, struct req_pkt *, int);
static	void	mon_getlist_0	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	mon_getlist_1	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	reset_stats	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	reset_peer	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	do_key_reread	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	trust_key	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	untrust_key	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	do_trustkey	(sockaddr_u *, struct interface *, struct req_pkt *, u_long);
static	void	get_auth_info	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	reset_auth_stats (void);
static	void	req_get_traps	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	req_set_trap	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	req_clr_trap	(sockaddr_u *, struct interface *, struct req_pkt *);
static	void	do_setclr_trap	(sockaddr_u *, struct interface *, struct req_pkt *, int);
static	void	set_request_keyid (sockaddr_u *, struct interface *, struct req_pkt *);
static	void	set_control_keyid (sockaddr_u *, struct interface *, struct req_pkt *);
static	void	get_ctl_stats   (sockaddr_u *, struct interface *, struct req_pkt *);
static	void	get_if_stats    (sockaddr_u *, struct interface *, struct req_pkt *);
static	void	do_if_reload    (sockaddr_u *, struct interface *, struct req_pkt *);
#ifdef KERNEL_PLL
static	void	get_kernel_info (sockaddr_u *, struct interface *, struct req_pkt *);
#endif /* KERNEL_PLL */
#ifdef REFCLOCK
static	void	get_clock_info (sockaddr_u *, struct interface *, struct req_pkt *);
static	void	set_clock_fudge (sockaddr_u *, struct interface *, struct req_pkt *);
#endif	/* REFCLOCK */
#ifdef REFCLOCK
static	void	get_clkbug_info (sockaddr_u *, struct interface *, struct req_pkt *);
#endif	/* REFCLOCK */

/*
 * ntpd request codes
 */
static	struct req_proc ntp_codes[] = {
	{ REQ_PEER_LIST,	NOAUTH,	0, 0,	peer_list },
	{ REQ_PEER_LIST_SUM,	NOAUTH,	0, 0,	peer_list_sum },
	{ REQ_PEER_INFO,    NOAUTH, v4sizeof(struct info_peer_list),
				sizeof(struct info_peer_list), peer_info},
	{ REQ_PEER_STATS,   NOAUTH, v4sizeof(struct info_peer_list),
				sizeof(struct info_peer_list), peer_stats},
	{ REQ_SYS_INFO,		NOAUTH,	0, 0,	sys_info },
	{ REQ_SYS_STATS,	NOAUTH,	0, 0,	sys_stats },
	{ REQ_IO_STATS,		NOAUTH,	0, 0,	io_stats },
	{ REQ_MEM_STATS,	NOAUTH,	0, 0,	mem_stats },
	{ REQ_LOOP_INFO,	NOAUTH,	0, 0,	loop_info },
	{ REQ_TIMER_STATS,	NOAUTH,	0, 0,	timer_stats },
	{ REQ_CONFIG,	    AUTH, v4sizeof(struct conf_peer),
				sizeof(struct conf_peer), do_conf },
	{ REQ_UNCONFIG,	    AUTH, v4sizeof(struct conf_unpeer),
				sizeof(struct conf_unpeer), do_unconf },
	{ REQ_SET_SYS_FLAG, AUTH, sizeof(struct conf_sys_flags),
				sizeof(struct conf_sys_flags), set_sys_flag },
	{ REQ_CLR_SYS_FLAG, AUTH, sizeof(struct conf_sys_flags), 
				sizeof(struct conf_sys_flags),  clr_sys_flag },
	{ REQ_GET_RESTRICT,	NOAUTH,	0, 0,	list_restrict },
	{ REQ_RESADDFLAGS, AUTH, v4sizeof(struct conf_restrict),
				sizeof(struct conf_restrict), do_resaddflags },
	{ REQ_RESSUBFLAGS, AUTH, v4sizeof(struct conf_restrict),
				sizeof(struct conf_restrict), do_ressubflags },
	{ REQ_UNRESTRICT, AUTH, v4sizeof(struct conf_restrict),
				sizeof(struct conf_restrict), do_unrestrict },
	{ REQ_MON_GETLIST,	NOAUTH,	0, 0,	mon_getlist_0 },
	{ REQ_MON_GETLIST_1,	NOAUTH,	0, 0,	mon_getlist_1 },
	{ REQ_RESET_STATS, AUTH, sizeof(struct reset_flags), 0, reset_stats },
	{ REQ_RESET_PEER,  AUTH, v4sizeof(struct conf_unpeer),
				sizeof(struct conf_unpeer), reset_peer },
	{ REQ_REREAD_KEYS,	AUTH,	0, 0,	do_key_reread },
	{ REQ_TRUSTKEY,   AUTH, sizeof(u_long), sizeof(u_long), trust_key },
	{ REQ_UNTRUSTKEY, AUTH, sizeof(u_long), sizeof(u_long), untrust_key },
	{ REQ_AUTHINFO,		NOAUTH,	0, 0,	get_auth_info },
	{ REQ_TRAPS,		NOAUTH, 0, 0,	req_get_traps },
	{ REQ_ADD_TRAP,	AUTH, v4sizeof(struct conf_trap),
				sizeof(struct conf_trap), req_set_trap },
	{ REQ_CLR_TRAP,	AUTH, v4sizeof(struct conf_trap),
				sizeof(struct conf_trap), req_clr_trap },
	{ REQ_REQUEST_KEY, AUTH, sizeof(u_long), sizeof(u_long), 
				set_request_keyid },
	{ REQ_CONTROL_KEY, AUTH, sizeof(u_long), sizeof(u_long), 
				set_control_keyid },
	{ REQ_GET_CTLSTATS,	NOAUTH,	0, 0,	get_ctl_stats },
#ifdef KERNEL_PLL
	{ REQ_GET_KERNEL,	NOAUTH,	0, 0,	get_kernel_info },
#endif
#ifdef REFCLOCK
	{ REQ_GET_CLOCKINFO, NOAUTH, sizeof(u_int32), sizeof(u_int32), 
				get_clock_info },
	{ REQ_SET_CLKFUDGE, AUTH, sizeof(struct conf_fudge), 
				sizeof(struct conf_fudge), set_clock_fudge },
	{ REQ_GET_CLKBUGINFO, NOAUTH, sizeof(u_int32), sizeof(u_int32),
				get_clkbug_info },
#endif
	{ REQ_IF_STATS,		AUTH, 0, 0,	get_if_stats },
	{ REQ_IF_RELOAD,        AUTH, 0, 0,	do_if_reload },

	{ NO_REQUEST,		NOAUTH,	0, 0,	0 }
};


/*
 * Authentication keyid used to authenticate requests.  Zero means we
 * don't allow writing anything.
 */
keyid_t info_auth_keyid;

/*
 * Statistic counters to keep track of requests and responses.
 */
u_long numrequests;		/* number of requests we've received */
u_long numresppkts;		/* number of resp packets sent with data */

u_long errorcounter[INFO_ERR_AUTH+1];	/* lazy way to count errors, indexed */
/* by the error code */

/*
 * A hack.  To keep the authentication module clear of ntp-ism's, we
 * include a time reset variable for its stats here.
 */
static u_long auth_timereset;

/*
 * Response packet used by these routines.  Also some state information
 * so that we can handle packet formatting within a common set of
 * subroutines.  Note we try to enter data in place whenever possible,
 * but the need to set the more bit correctly means we occasionally
 * use the extra buffer and copy.
 */
static struct resp_pkt rpkt;
static int reqver;
static int seqno;
static int nitems;
static int itemsize;
static int databytes;
static char exbuf[RESP_DATA_SIZE];
static int usingexbuf;
static sockaddr_u *toaddr;
static struct interface *frominter;

/*
 * init_request - initialize request data
 */
void
init_request (void)
{
	size_t i;

	numrequests = 0;
	numresppkts = 0;
	auth_timereset = 0;
	info_auth_keyid = 0;	/* by default, can't do this */

	for (i = 0; i < sizeof(errorcounter)/sizeof(errorcounter[0]); i++)
	    errorcounter[i] = 0;
}


/*
 * req_ack - acknowledge request with no data
 */
static void
req_ack(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt,
	int errcode
	)
{
	/*
	 * fill in the fields
	 */
	rpkt.rm_vn_mode = RM_VN_MODE(RESP_BIT, 0, reqver);
	rpkt.auth_seq = AUTH_SEQ(0, 0);
	rpkt.implementation = inpkt->implementation;
	rpkt.request = inpkt->request;
	rpkt.err_nitems = ERR_NITEMS(errcode, 0); 
	rpkt.mbz_itemsize = MBZ_ITEMSIZE(0);

	/*
	 * send packet and bump counters
	 */
	sendpkt(srcadr, inter, -1, (struct pkt *)&rpkt, RESP_HEADER_SIZE);
	errorcounter[errcode]++;
}


/*
 * prepare_pkt - prepare response packet for transmission, return pointer
 *		 to storage for data item.
 */
static char *
prepare_pkt(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *pkt,
	size_t structsize
	)
{
	DPRINTF(4, ("request: preparing pkt\n"));

	/*
	 * Fill in the implementation, request and itemsize fields
	 * since these won't change.
	 */
	rpkt.implementation = pkt->implementation;
	rpkt.request = pkt->request;
	rpkt.mbz_itemsize = MBZ_ITEMSIZE(structsize);

	/*
	 * Compute the static data needed to carry on.
	 */
	toaddr = srcadr;
	frominter = inter;
	seqno = 0;
	nitems = 0;
	itemsize = structsize;
	databytes = 0;
	usingexbuf = 0;

	/*
	 * return the beginning of the packet buffer.
	 */
	return &rpkt.data[0];
}


/*
 * more_pkt - return a data pointer for a new item.
 */
static char *
more_pkt(void)
{
	/*
	 * If we were using the extra buffer, send the packet.
	 */
	if (usingexbuf) {
		DPRINTF(3, ("request: sending pkt\n"));
		rpkt.rm_vn_mode = RM_VN_MODE(RESP_BIT, MORE_BIT, reqver);
		rpkt.auth_seq = AUTH_SEQ(0, seqno);
		rpkt.err_nitems = htons((u_short)nitems);
		sendpkt(toaddr, frominter, -1, (struct pkt *)&rpkt,
			RESP_HEADER_SIZE + databytes);
		numresppkts++;

		/*
		 * Copy data out of exbuf into the packet.
		 */
		memcpy(&rpkt.data[0], exbuf, (unsigned)itemsize);
		seqno++;
		databytes = 0;
		nitems = 0;
		usingexbuf = 0;
	}

	databytes += itemsize;
	nitems++;
	if (databytes + itemsize <= RESP_DATA_SIZE) {
		DPRINTF(4, ("request: giving him more data\n"));
		/*
		 * More room in packet.  Give him the
		 * next address.
		 */
		return &rpkt.data[databytes];
	} else {
		/*
		 * No room in packet.  Give him the extra
		 * buffer unless this was the last in the sequence.
		 */
		DPRINTF(4, ("request: into extra buffer\n"));
		if (seqno == MAXSEQ)
			return NULL;
		else {
			usingexbuf = 1;
			return exbuf;
		}
	}
}


/*
 * flush_pkt - we're done, return remaining information.
 */
static void
flush_pkt(void)
{
	DPRINTF(3, ("request: flushing packet, %d items\n", nitems));
	/*
	 * Must send the last packet.  If nothing in here and nothing
	 * has been sent, send an error saying no data to be found.
	 */
	if (seqno == 0 && nitems == 0)
		req_ack(toaddr, frominter, (struct req_pkt *)&rpkt,
			INFO_ERR_NODATA);
	else {
		rpkt.rm_vn_mode = RM_VN_MODE(RESP_BIT, 0, reqver);
		rpkt.auth_seq = AUTH_SEQ(0, seqno);
		rpkt.err_nitems = htons((u_short)nitems);
		sendpkt(toaddr, frominter, -1, (struct pkt *)&rpkt,
			RESP_HEADER_SIZE+databytes);
		numresppkts++;
	}
}



/*
 * Given a buffer, return the packet mode
 */
int
get_packet_mode(struct recvbuf *rbufp)
{
	struct req_pkt *inpkt = (struct req_pkt *)&rbufp->recv_pkt;
	return (INFO_MODE(inpkt->rm_vn_mode));
}


/*
 * process_private - process private mode (7) packets
 */
void
process_private(
	struct recvbuf *rbufp,
	int mod_okay
	)
{
	static u_long quiet_until;
	struct req_pkt *inpkt;
	struct req_pkt_tail *tailinpkt;
	sockaddr_u *srcadr;
	struct interface *inter;
	struct req_proc *proc;
	int ec;
	short temp_size;
	l_fp ftmp;
	double dtemp;
	size_t recv_len;
	size_t noslop_len;
	size_t mac_len;

	/*
	 * Initialize pointers, for convenience
	 */
	recv_len = rbufp->recv_length;
	inpkt = (struct req_pkt *)&rbufp->recv_pkt;
	srcadr = &rbufp->recv_srcadr;
	inter = rbufp->dstadr;

	DPRINTF(3, ("process_private: impl %d req %d\n",
		    inpkt->implementation, inpkt->request));

	/*
	 * Do some sanity checks on the packet.  Return a format
	 * error if it fails.
	 */
	ec = 0;
	if (   (++ec, ISRESPONSE(inpkt->rm_vn_mode))
	    || (++ec, ISMORE(inpkt->rm_vn_mode))
	    || (++ec, INFO_VERSION(inpkt->rm_vn_mode) > NTP_VERSION)
	    || (++ec, INFO_VERSION(inpkt->rm_vn_mode) < NTP_OLDVERSION)
	    || (++ec, INFO_SEQ(inpkt->auth_seq) != 0)
	    || (++ec, INFO_ERR(inpkt->err_nitems) != 0)
	    || (++ec, INFO_MBZ(inpkt->mbz_itemsize) != 0)
	    || (++ec, rbufp->recv_length < (int)REQ_LEN_HDR)
		) {
		NLOG(NLOG_SYSEVENT)
			if (current_time >= quiet_until) {
				msyslog(LOG_ERR,
					"process_private: drop test %d"
					" failed, pkt from %s",
					ec, stoa(srcadr));
				quiet_until = current_time + 60;
			}
		return;
	}

	reqver = INFO_VERSION(inpkt->rm_vn_mode);

	/*
	 * Get the appropriate procedure list to search.
	 */
	if (inpkt->implementation == IMPL_UNIV)
		proc = univ_codes;
	else if ((inpkt->implementation == IMPL_XNTPD) ||
		 (inpkt->implementation == IMPL_XNTPD_OLD))
		proc = ntp_codes;
	else {
		req_ack(srcadr, inter, inpkt, INFO_ERR_IMPL);
		return;
	}

	/*
	 * Search the list for the request codes.  If it isn't one
	 * we know, return an error.
	 */
	while (proc->request_code != NO_REQUEST) {
		if (proc->request_code == (short) inpkt->request)
			break;
		proc++;
	}
	if (proc->request_code == NO_REQUEST) {
		req_ack(srcadr, inter, inpkt, INFO_ERR_REQ);
		return;
	}

	DPRINTF(4, ("found request in tables\n"));

	/*
	 * If we need data, check to see if we have some.  If we
	 * don't, check to see that there is none (picky, picky).
	 */	

	/* This part is a bit tricky, we want to be sure that the size
	 * returned is either the old or the new size.  We also can find
	 * out if the client can accept both types of messages this way. 
	 *
	 * Handle the exception of REQ_CONFIG. It can have two data sizes.
	 */
	temp_size = INFO_ITEMSIZE(inpkt->mbz_itemsize);
	if ((temp_size != proc->sizeofitem &&
	     temp_size != proc->v6_sizeofitem) &&
	    !(inpkt->implementation == IMPL_XNTPD &&
	      inpkt->request == REQ_CONFIG &&
	      temp_size == sizeof(struct old_conf_peer))) {
		DPRINTF(3, ("process_private: wrong item size, received %d, should be %d or %d\n",
			    temp_size, proc->sizeofitem, proc->v6_sizeofitem));
		req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
		return;
	}
	if ((proc->sizeofitem != 0) &&
	    ((size_t)(temp_size * INFO_NITEMS(inpkt->err_nitems)) >
	     (recv_len - REQ_LEN_HDR))) {
		DPRINTF(3, ("process_private: not enough data\n"));
		req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
		return;
	}

	switch (inpkt->implementation) {
	case IMPL_XNTPD:
		client_v6_capable = 1;
		break;
	case IMPL_XNTPD_OLD:
		client_v6_capable = 0;
		break;
	default:
		req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
		return;
	}

	/*
	 * If we need to authenticate, do so.  Note that an
	 * authenticatable packet must include a mac field, must
	 * have used key info_auth_keyid and must have included
	 * a time stamp in the appropriate field.  The time stamp
	 * must be within INFO_TS_MAXSKEW of the receive
	 * time stamp.
	 */
	if (proc->needs_auth && sys_authenticate) {

		if (recv_len < (REQ_LEN_HDR +
		    (INFO_ITEMSIZE(inpkt->mbz_itemsize) *
		    INFO_NITEMS(inpkt->err_nitems)) +
		    REQ_TAIL_MIN)) {
			req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
			return;
		}

		/*
		 * For 16-octet digests, regardless of itemsize and
		 * nitems, authenticated requests are a fixed size
		 * with the timestamp, key ID, and digest located
		 * at the end of the packet.  Because the key ID
		 * determining the digest size precedes the digest,
		 * for larger digests the fixed size request scheme
		 * is abandoned and the timestamp, key ID, and digest
		 * are located relative to the start of the packet,
		 * with the digest size determined by the packet size.
		 */
		noslop_len = REQ_LEN_HDR
			     + INFO_ITEMSIZE(inpkt->mbz_itemsize) *
			       INFO_NITEMS(inpkt->err_nitems)
			     + sizeof(inpkt->tstamp);
		/* 32-bit alignment */
		noslop_len = (noslop_len + 3) & ~3;
		if (recv_len > (noslop_len + MAX_MAC_LEN))
			mac_len = 20;
		else
			mac_len = recv_len - noslop_len;

		tailinpkt = (void *)((char *)inpkt + recv_len -
			    (mac_len + sizeof(inpkt->tstamp)));

		/*
		 * If this guy is restricted from doing this, don't let
		 * him.  If the wrong key was used, or packet doesn't
		 * have mac, return.
		 */
		if (!INFO_IS_AUTH(inpkt->auth_seq) || !info_auth_keyid
		    || ntohl(tailinpkt->keyid) != info_auth_keyid) {
			DPRINTF(5, ("failed auth %d info_auth_keyid %u pkt keyid %u maclen %lu\n",
				    INFO_IS_AUTH(inpkt->auth_seq),
				    info_auth_keyid,
				    ntohl(tailinpkt->keyid), (u_long)mac_len));
#ifdef DEBUG
			msyslog(LOG_DEBUG,
				"process_private: failed auth %d info_auth_keyid %u pkt keyid %u maclen %lu\n",
				INFO_IS_AUTH(inpkt->auth_seq),
				info_auth_keyid,
				ntohl(tailinpkt->keyid), (u_long)mac_len);
#endif
			req_ack(srcadr, inter, inpkt, INFO_ERR_AUTH);
			return;
		}
		if (recv_len > REQ_LEN_NOMAC + MAX_MAC_LEN) {
			DPRINTF(5, ("bad pkt length %zu\n", recv_len));
			msyslog(LOG_ERR,
				"process_private: bad pkt length %zu",
				recv_len);
			req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
			return;
		}
		if (!mod_okay || !authhavekey(info_auth_keyid)) {
			DPRINTF(5, ("failed auth mod_okay %d\n",
				    mod_okay));
#ifdef DEBUG
			msyslog(LOG_DEBUG,
				"process_private: failed auth mod_okay %d\n",
				mod_okay);
#endif
			req_ack(srcadr, inter, inpkt, INFO_ERR_AUTH);
			return;
		}

		/*
		 * calculate absolute time difference between xmit time stamp
		 * and receive time stamp.  If too large, too bad.
		 */
		NTOHL_FP(&tailinpkt->tstamp, &ftmp);
		L_SUB(&ftmp, &rbufp->recv_time);
		LFPTOD(&ftmp, dtemp);
		if (fabs(dtemp) > INFO_TS_MAXSKEW) {
			/*
			 * He's a loser.  Tell him.
			 */
			DPRINTF(5, ("xmit/rcv timestamp delta %g > INFO_TS_MAXSKEW %g\n",
				    dtemp, INFO_TS_MAXSKEW));
			req_ack(srcadr, inter, inpkt, INFO_ERR_AUTH);
			return;
		}

		/*
		 * So far so good.  See if decryption works out okay.
		 */
		if (!authdecrypt(info_auth_keyid, (u_int32 *)inpkt,
				 recv_len - mac_len, mac_len)) {
			DPRINTF(5, ("authdecrypt failed\n"));
			req_ack(srcadr, inter, inpkt, INFO_ERR_AUTH);
			return;
		}
	}

	DPRINTF(3, ("process_private: all okay, into handler\n"));
	/*
	 * Packet is okay.  Call the handler to send him data.
	 */
	(proc->handler)(srcadr, inter, inpkt);
}


/*
 * peer_list - send a list of the peers
 */
static void
peer_list(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_peer_list *ip;
	register struct peer *pp;
	register int i;
	register int skip = 0;

	ip = (struct info_peer_list *)prepare_pkt(srcadr, inter, inpkt,
	    v6sizeof(struct info_peer_list));
	for (i = 0; i < NTP_HASH_SIZE && ip != 0; i++) {
		pp = peer_hash[i];
		while (pp != 0 && ip != 0) {
			if (IS_IPV6(&pp->srcadr)) {
				if (client_v6_capable) {
					ip->addr6 = SOCK_ADDR6(&pp->srcadr);
					ip->v6_flag = 1;
					skip = 0;
				} else {
					skip = 1;
					break;
				}
			} else {
				ip->addr = NSRCADR(&pp->srcadr);
				if (client_v6_capable)
					ip->v6_flag = 0;
				skip = 0;
			}

			if(!skip) {
				ip->port = NSRCPORT(&pp->srcadr);
				ip->hmode = pp->hmode;
				ip->flags = 0;
				if (pp->flags & FLAG_CONFIG)
				    ip->flags |= INFO_FLAG_CONFIG;
				if (pp == sys_peer)
				    ip->flags |= INFO_FLAG_SYSPEER;
				if (pp->status == CTL_PST_SEL_SYNCCAND)
				    ip->flags |= INFO_FLAG_SEL_CANDIDATE;
				if (pp->status >= CTL_PST_SEL_SYSPEER)
				    ip->flags |= INFO_FLAG_SHORTLIST;
				ip = (struct info_peer_list *)more_pkt();
			}
			pp = pp->next; 
		}
	}
	flush_pkt();
}


/*
 * peer_list_sum - return extended peer list
 */
static void
peer_list_sum(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_peer_summary *ips;
	register struct peer *pp;
	register int i;
	l_fp ltmp;
	register int skip;

#ifdef DEBUG
	if (debug > 2)
	    printf("wants peer list summary\n");
#endif
	ips = (struct info_peer_summary *)prepare_pkt(srcadr, inter, inpkt,
	    v6sizeof(struct info_peer_summary));
	for (i = 0; i < NTP_HASH_SIZE && ips != 0; i++) {
		pp = peer_hash[i];
		while (pp != 0 && ips != 0) {
#ifdef DEBUG
			if (debug > 3)
			    printf("sum: got one\n");
#endif
			/*
			 * Be careful here not to return v6 peers when we
			 * want only v4.
			 */
			if (IS_IPV6(&pp->srcadr)) {
				if (client_v6_capable) {
					ips->srcadr6 = SOCK_ADDR6(&pp->srcadr);
					ips->v6_flag = 1;
					if (pp->dstadr)
						ips->dstadr6 = SOCK_ADDR6(&pp->dstadr->sin);
					else
						memset(&ips->dstadr6, 0, sizeof(ips->dstadr6));
					skip = 0;
				} else {
					skip = 1;
					break;
				}
			} else {
				ips->srcadr = NSRCADR(&pp->srcadr);
				if (client_v6_capable)
					ips->v6_flag = 0;
				
				if (pp->dstadr) {
					if (!pp->processed)
						ips->dstadr = NSRCADR(&pp->dstadr->sin);
					else {
						if (MDF_BCAST == pp->cast_flags)
							ips->dstadr = NSRCADR(&pp->dstadr->bcast);
						else if (pp->cast_flags) {
							ips->dstadr = NSRCADR(&pp->dstadr->sin);
							if (!ips->dstadr)
								ips->dstadr = NSRCADR(&pp->dstadr->bcast);
						}
					}
				} else
					ips->dstadr = 0;

				skip = 0;
			}
			
			if (!skip){ 
				ips->srcport = NSRCPORT(&pp->srcadr);
				ips->stratum = pp->stratum;
				ips->hpoll = pp->hpoll;
				ips->ppoll = pp->ppoll;
				ips->reach = pp->reach;
				ips->flags = 0;
				if (pp == sys_peer)
				    ips->flags |= INFO_FLAG_SYSPEER;
				if (pp->flags & FLAG_CONFIG)
				    ips->flags |= INFO_FLAG_CONFIG;
				if (pp->flags & FLAG_REFCLOCK)
				    ips->flags |= INFO_FLAG_REFCLOCK;
				if (pp->flags & FLAG_PREFER)
				    ips->flags |= INFO_FLAG_PREFER;
				if (pp->flags & FLAG_BURST)
				    ips->flags |= INFO_FLAG_BURST;
				if (pp->status == CTL_PST_SEL_SYNCCAND)
				    ips->flags |= INFO_FLAG_SEL_CANDIDATE;
				if (pp->status >= CTL_PST_SEL_SYSPEER)
				    ips->flags |= INFO_FLAG_SHORTLIST;
				ips->hmode = pp->hmode;
				ips->delay = HTONS_FP(DTOFP(pp->delay));
				DTOLFP(pp->offset, &ltmp);
				HTONL_FP(&ltmp, &ips->offset);
				ips->dispersion = HTONS_FP(DTOUFP(SQRT(pp->disp)));
			}	
			pp = pp->next; 
			ips = (struct info_peer_summary *)more_pkt();
		}
	}
	flush_pkt();
}


/*
 * peer_info - send information for one or more peers
 */
static void
peer_info (
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_peer_list *ipl;
	register struct peer *pp;
	register struct info_peer *ip;
	register int items;
	register int i, j;
	sockaddr_u addr;
	extern struct peer *sys_peer;
	l_fp ltmp;

	items = INFO_NITEMS(inpkt->err_nitems);
	ipl = (struct info_peer_list *) inpkt->data;

	ip = (struct info_peer *)prepare_pkt(srcadr, inter, inpkt,
	    v6sizeof(struct info_peer));
	while (items-- > 0 && ip != 0) {
		ZERO_SOCK(&addr);
		NSRCPORT(&addr) = ipl->port;
		if (client_v6_capable && ipl->v6_flag) {
			AF(&addr) = AF_INET6;
			SOCK_ADDR6(&addr) = ipl->addr6;
		} else {
			AF(&addr) = AF_INET;
			NSRCADR(&addr) = ipl->addr;
		}
#ifdef ISC_PLATFORM_HAVESALEN
		addr.sa.sa_len = SOCKLEN(&addr);
#endif
		ipl++;
		pp = findexistingpeer(&addr, NULL, -1, 0);
		if (NULL == pp)
			continue;
		if (IS_IPV6(srcadr)) {
			if (pp->dstadr)
				ip->dstadr6 =
				    (MDF_BCAST == pp->cast_flags)
					? SOCK_ADDR6(&pp->dstadr->bcast)
					: SOCK_ADDR6(&pp->dstadr->sin);
			else
				memset(&ip->dstadr6, 0, sizeof(ip->dstadr6));

			ip->srcadr6 = SOCK_ADDR6(&pp->srcadr);
			ip->v6_flag = 1;
		} else {
			if (pp->dstadr) {
				if (!pp->processed)
					ip->dstadr = NSRCADR(&pp->dstadr->sin);
				else {
					if (MDF_BCAST == pp->cast_flags)
						ip->dstadr = NSRCADR(&pp->dstadr->bcast);
					else if (pp->cast_flags) {
						ip->dstadr = NSRCADR(&pp->dstadr->sin);
						if (!ip->dstadr)
							ip->dstadr = NSRCADR(&pp->dstadr->bcast);
					}
				}
			} else
				ip->dstadr = 0;

			ip->srcadr = NSRCADR(&pp->srcadr);
			if (client_v6_capable)
				ip->v6_flag = 0;
		}
		ip->srcport = NSRCPORT(&pp->srcadr);
		ip->flags = 0;
		if (pp == sys_peer)
		    ip->flags |= INFO_FLAG_SYSPEER;
		if (pp->flags & FLAG_CONFIG)
		    ip->flags |= INFO_FLAG_CONFIG;
		if (pp->flags & FLAG_REFCLOCK)
		    ip->flags |= INFO_FLAG_REFCLOCK;
		if (pp->flags & FLAG_PREFER)
		    ip->flags |= INFO_FLAG_PREFER;
		if (pp->flags & FLAG_BURST)
		    ip->flags |= INFO_FLAG_BURST;
		if (pp->status == CTL_PST_SEL_SYNCCAND)
		    ip->flags |= INFO_FLAG_SEL_CANDIDATE;
		if (pp->status >= CTL_PST_SEL_SYSPEER)
		    ip->flags |= INFO_FLAG_SHORTLIST;
		ip->leap = pp->leap;
		ip->hmode = pp->hmode;
		ip->keyid = pp->keyid;
		ip->stratum = pp->stratum;
		ip->ppoll = pp->ppoll;
		ip->hpoll = pp->hpoll;
		ip->precision = pp->precision;
		ip->version = pp->version;
		ip->reach = pp->reach;
		ip->unreach = (u_char) pp->unreach;
		ip->flash = (u_char)pp->flash;
		ip->flash2 = (u_short) pp->flash;
		ip->estbdelay = HTONS_FP(DTOFP(pp->delay));
		ip->ttl = pp->ttl;
		ip->associd = htons(pp->associd);
		ip->rootdelay = HTONS_FP(DTOUFP(pp->rootdelay));
		ip->rootdispersion = HTONS_FP(DTOUFP(pp->rootdisp));
		ip->refid = pp->refid;
		HTONL_FP(&pp->reftime, &ip->reftime);
		HTONL_FP(&pp->aorg, &ip->org);
		HTONL_FP(&pp->rec, &ip->rec);
		HTONL_FP(&pp->xmt, &ip->xmt);
		j = pp->filter_nextpt - 1;
		for (i = 0; i < NTP_SHIFT; i++, j--) {
			if (j < 0)
			    j = NTP_SHIFT-1;
			ip->filtdelay[i] = HTONS_FP(DTOFP(pp->filter_delay[j]));
			DTOLFP(pp->filter_offset[j], &ltmp);
			HTONL_FP(&ltmp, &ip->filtoffset[i]);
			ip->order[i] = (u_char)((pp->filter_nextpt+NTP_SHIFT-1)
				- pp->filter_order[i]);
			if (ip->order[i] >= NTP_SHIFT)
			    ip->order[i] -= NTP_SHIFT;
		}
		DTOLFP(pp->offset, &ltmp);
		HTONL_FP(&ltmp, &ip->offset);
		ip->delay = HTONS_FP(DTOFP(pp->delay));
		ip->dispersion = HTONS_FP(DTOUFP(SQRT(pp->disp)));
		ip->selectdisp = HTONS_FP(DTOUFP(SQRT(pp->jitter)));
		ip = (struct info_peer *)more_pkt();
	}
	flush_pkt();
}


/*
 * peer_stats - send statistics for one or more peers
 */
static void
peer_stats (
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_peer_list *ipl;
	register struct peer *pp;
	register struct info_peer_stats *ip;
	register int items;
	sockaddr_u addr;
	extern struct peer *sys_peer;

#ifdef DEBUG
	if (debug)
	     printf("peer_stats: called\n");
#endif
	items = INFO_NITEMS(inpkt->err_nitems);
	ipl = (struct info_peer_list *) inpkt->data;
	ip = (struct info_peer_stats *)prepare_pkt(srcadr, inter, inpkt,
	    v6sizeof(struct info_peer_stats));
	while (items-- > 0 && ip != 0) {
		memset((char *)&addr, 0, sizeof(addr));
		NSRCPORT(&addr) = ipl->port;
		if (client_v6_capable && ipl->v6_flag) {
			AF(&addr) = AF_INET6;
			SOCK_ADDR6(&addr) = ipl->addr6;
		} else {
			AF(&addr) = AF_INET;
			NSRCADR(&addr) = ipl->addr;
		}	
#ifdef ISC_PLATFORM_HAVESALEN
		addr.sa.sa_len = SOCKLEN(&addr);
#endif
		DPRINTF(1, ("peer_stats: looking for %s, %d, %d\n",
			    stoa(&addr), ipl->port, NSRCPORT(&addr)));

		ipl = (struct info_peer_list *)((char *)ipl +
		    INFO_ITEMSIZE(inpkt->mbz_itemsize));

		pp = findexistingpeer(&addr, NULL, -1, 0);
		if (NULL == pp)
			continue;

		DPRINTF(1, ("peer_stats: found %s\n", stoa(&addr)));

		if (IS_IPV4(&pp->srcadr)) {
			if (pp->dstadr) {
				if (!pp->processed)
					ip->dstadr = NSRCADR(&pp->dstadr->sin);
				else {
					if (MDF_BCAST == pp->cast_flags)
						ip->dstadr = NSRCADR(&pp->dstadr->bcast);
					else if (pp->cast_flags) {
						ip->dstadr = NSRCADR(&pp->dstadr->sin);
						if (!ip->dstadr)
							ip->dstadr = NSRCADR(&pp->dstadr->bcast);
					}
				}
			} else
				ip->dstadr = 0;
			
			ip->srcadr = NSRCADR(&pp->srcadr);
			if (client_v6_capable)
				ip->v6_flag = 0;
		} else {
			if (pp->dstadr)
				ip->dstadr6 =
				    (MDF_BCAST == pp->cast_flags)
					? SOCK_ADDR6(&pp->dstadr->bcast)
					: SOCK_ADDR6(&pp->dstadr->sin);
			else
				memset(&ip->dstadr6, 0, sizeof(ip->dstadr6));

			ip->srcadr6 = SOCK_ADDR6(&pp->srcadr);
			ip->v6_flag = 1;
		}	
		ip->srcport = NSRCPORT(&pp->srcadr);
		ip->flags = 0;
		if (pp == sys_peer)
		    ip->flags |= INFO_FLAG_SYSPEER;
		if (pp->flags & FLAG_CONFIG)
		    ip->flags |= INFO_FLAG_CONFIG;
		if (pp->flags & FLAG_REFCLOCK)
		    ip->flags |= INFO_FLAG_REFCLOCK;
		if (pp->flags & FLAG_PREFER)
		    ip->flags |= INFO_FLAG_PREFER;
		if (pp->flags & FLAG_BURST)
		    ip->flags |= INFO_FLAG_BURST;
		if (pp->flags & FLAG_IBURST)
		    ip->flags |= INFO_FLAG_IBURST;
		if (pp->status == CTL_PST_SEL_SYNCCAND)
		    ip->flags |= INFO_FLAG_SEL_CANDIDATE;
		if (pp->status >= CTL_PST_SEL_SYSPEER)
		    ip->flags |= INFO_FLAG_SHORTLIST;
		ip->flags = htons(ip->flags);
		ip->timereceived = htonl((u_int32)(current_time - pp->timereceived));
		ip->timetosend = htonl(pp->nextdate - current_time);
		ip->timereachable = htonl((u_int32)(current_time - pp->timereachable));
		ip->sent = htonl((u_int32)(pp->sent));
		ip->processed = htonl((u_int32)(pp->processed));
		ip->badauth = htonl((u_int32)(pp->badauth));
		ip->bogusorg = htonl((u_int32)(pp->bogusorg));
		ip->oldpkt = htonl((u_int32)(pp->oldpkt));
		ip->seldisp = htonl((u_int32)(pp->seldisptoolarge));
		ip->selbroken = htonl((u_int32)(pp->selbroken));
		ip->candidate = pp->status;
		ip = (struct info_peer_stats *)more_pkt();
	}
	flush_pkt();
}


/*
 * sys_info - return system info
 */
static void
sys_info(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_sys *is;

	is = (struct info_sys *)prepare_pkt(srcadr, inter, inpkt,
	    v6sizeof(struct info_sys));

	if (sys_peer) {
		if (IS_IPV4(&sys_peer->srcadr)) {
			is->peer = NSRCADR(&sys_peer->srcadr);
			if (client_v6_capable)
				is->v6_flag = 0;
		} else if (client_v6_capable) {
			is->peer6 = SOCK_ADDR6(&sys_peer->srcadr);
			is->v6_flag = 1;
		}
		is->peer_mode = sys_peer->hmode;
	} else {
		is->peer = 0;
		if (client_v6_capable) {
			is->v6_flag = 0;
		}
		is->peer_mode = 0;
	}

	is->leap = sys_leap;
	is->stratum = sys_stratum;
	is->precision = sys_precision;
	is->rootdelay = htonl(DTOFP(sys_rootdelay));
	is->rootdispersion = htonl(DTOUFP(sys_rootdisp));
	is->frequency = htonl(DTOFP(sys_jitter));
	is->stability = htonl(DTOUFP(clock_stability));
	is->refid = sys_refid;
	HTONL_FP(&sys_reftime, &is->reftime);

	is->poll = sys_poll;
	
	is->flags = 0;
	if (sys_authenticate)
		is->flags |= INFO_FLAG_AUTHENTICATE;
	if (sys_bclient)
		is->flags |= INFO_FLAG_BCLIENT;
#ifdef REFCLOCK
	if (cal_enable)
		is->flags |= INFO_FLAG_CAL;
#endif /* REFCLOCK */
	if (kern_enable)
		is->flags |= INFO_FLAG_KERNEL;
	if (mon_enabled != MON_OFF)
		is->flags |= INFO_FLAG_MONITOR;
	if (ntp_enable)
		is->flags |= INFO_FLAG_NTP;
	if (pps_enable)
		is->flags |= INFO_FLAG_PPS_SYNC;
	if (stats_control)
		is->flags |= INFO_FLAG_FILEGEN;
	is->bdelay = HTONS_FP(DTOFP(sys_bdelay));
	HTONL_UF(sys_authdelay.l_f, &is->authdelay);
	(void) more_pkt();
	flush_pkt();
}


/*
 * sys_stats - return system statistics
 */
static void
sys_stats(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_sys_stats *ss;

	/*
	 * Importations from the protocol module
	 */
	ss = (struct info_sys_stats *)prepare_pkt(srcadr, inter, inpkt,
		sizeof(struct info_sys_stats));
	ss->timeup = htonl((u_int32)current_time);
	ss->timereset = htonl((u_int32)(current_time - sys_stattime));
	ss->denied = htonl((u_int32)sys_restricted);
	ss->oldversionpkt = htonl((u_int32)sys_oldversion);
	ss->newversionpkt = htonl((u_int32)sys_newversion);
	ss->unknownversion = htonl((u_int32)sys_declined);
	ss->badlength = htonl((u_int32)sys_badlength);
	ss->processed = htonl((u_int32)sys_processed);
	ss->badauth = htonl((u_int32)sys_badauth);
	ss->limitrejected = htonl((u_int32)sys_limitrejected);
	ss->received = htonl((u_int32)sys_received);
	(void) more_pkt();
	flush_pkt();
}


/*
 * mem_stats - return memory statistics
 */
static void
mem_stats(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_mem_stats *ms;
	register int i;

	/*
	 * Importations from the peer module
	 */
	extern int peer_hash_count[];
	extern int peer_free_count;
	extern u_long peer_timereset;
	extern u_long findpeer_calls;
	extern u_long peer_allocations;
	extern u_long peer_demobilizations;
	extern int total_peer_structs;

	ms = (struct info_mem_stats *)prepare_pkt(srcadr, inter, inpkt,
						  sizeof(struct info_mem_stats));

	ms->timereset = htonl((u_int32)(current_time - peer_timereset));
	ms->totalpeermem = htons((u_short)total_peer_structs);
	ms->freepeermem = htons((u_short)peer_free_count);
	ms->findpeer_calls = htonl((u_int32)findpeer_calls);
	ms->allocations = htonl((u_int32)peer_allocations);
	ms->demobilizations = htonl((u_int32)peer_demobilizations);

	for (i = 0; i < NTP_HASH_SIZE; i++) {
		if (peer_hash_count[i] > 255)
		    ms->hashcount[i] = 255;
		else
		    ms->hashcount[i] = (u_char)peer_hash_count[i];
	}

	(void) more_pkt();
	flush_pkt();
}


/*
 * io_stats - return io statistics
 */
static void
io_stats(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_io_stats *io;

	/*
	 * Importations from the io module
	 */
	extern u_long io_timereset;
	
	io = (struct info_io_stats *)prepare_pkt(srcadr, inter, inpkt,
						 sizeof(struct info_io_stats));

	io->timereset = htonl((u_int32)(current_time - io_timereset));
	io->totalrecvbufs = htons((u_short) total_recvbuffs());
	io->freerecvbufs = htons((u_short) free_recvbuffs());
	io->fullrecvbufs = htons((u_short) full_recvbuffs());
	io->lowwater = htons((u_short) lowater_additions());
	io->dropped = htonl((u_int32)packets_dropped);
	io->ignored = htonl((u_int32)packets_ignored);
	io->received = htonl((u_int32)packets_received);
	io->sent = htonl((u_int32)packets_sent);
	io->notsent = htonl((u_int32)packets_notsent);
	io->interrupts = htonl((u_int32)handler_calls);
	io->int_received = htonl((u_int32)handler_pkts);

	(void) more_pkt();
	flush_pkt();
}


/*
 * timer_stats - return timer statistics
 */
static void
timer_stats(
	sockaddr_u *		srcadr,
	struct interface *	inter,
	struct req_pkt *	inpkt
	)
{
	struct info_timer_stats *	ts;
	u_long				sincereset;

	ts = (struct info_timer_stats *)prepare_pkt(srcadr, inter,
						    inpkt, sizeof(*ts));

	sincereset = current_time - timer_timereset;
	ts->timereset = htonl((u_int32)sincereset);
	ts->alarms = ts->timereset;
	ts->overflows = htonl((u_int32)alarm_overflow);
	ts->xmtcalls = htonl((u_int32)timer_xmtcalls);

	(void) more_pkt();
	flush_pkt();
}


/*
 * loop_info - return the current state of the loop filter
 */
static void
loop_info(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_loop *li;
	l_fp ltmp;

	/*
	 * Importations from the loop filter module
	 */
	extern double last_offset;
	extern double drift_comp;
	extern int tc_counter;
	extern u_long sys_epoch;

	li = (struct info_loop *)prepare_pkt(srcadr, inter, inpkt,
	    sizeof(struct info_loop));

	DTOLFP(last_offset, &ltmp);
	HTONL_FP(&ltmp, &li->last_offset);
	DTOLFP(drift_comp * 1e6, &ltmp);
	HTONL_FP(&ltmp, &li->drift_comp);
	li->compliance = htonl((u_int32)(tc_counter));
	li->watchdog_timer = htonl((u_int32)(current_time - sys_epoch));

	(void) more_pkt();
	flush_pkt();
}


/*
 * do_conf - add a peer to the configuration list
 */
static void
do_conf(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	static u_long soonest_ifrescan_time = 0;
	int items;
	u_int fl;
	struct conf_peer *cp; 
	struct conf_peer temp_cp;
	sockaddr_u peeraddr;

	/*
	 * Do a check of everything to see that it looks
	 * okay.  If not, complain about it.  Note we are
	 * very picky here.
	 */
	items = INFO_NITEMS(inpkt->err_nitems);
	cp = (struct conf_peer *)inpkt->data;
	memset(&temp_cp, 0, sizeof(struct conf_peer));
	memcpy(&temp_cp, (char *)cp, INFO_ITEMSIZE(inpkt->mbz_itemsize));

#if 0 /* paranoid checking - these are done in newpeer() */
	fl = 0;
	while (items-- > 0 && !fl) {
		if (((temp_cp.version) > NTP_VERSION)
		    || ((temp_cp.version) < NTP_OLDVERSION))
		    fl = 1;
		if (temp_cp.hmode != MODE_ACTIVE
		    && temp_cp.hmode != MODE_CLIENT
		    && temp_cp.hmode != MODE_BROADCAST)
		    fl = 1;
		if (temp_cp.flags & ~(CONF_FLAG_PREFER | CONF_FLAG_BURST |
		    CONF_FLAG_IBURST | CONF_FLAG_SKEY))
			fl = 1;
		cp = (struct conf_peer *)
		    ((char *)cp + INFO_ITEMSIZE(inpkt->mbz_itemsize));
	}

	if (fl) {
		req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
		return;
	}
#endif /* end paranoid checking */

	/*
	 * Looks okay, try it out
	 */
	items = INFO_NITEMS(inpkt->err_nitems);
	cp = (struct conf_peer *)inpkt->data;  

	while (items-- > 0) {
		memset(&temp_cp, 0, sizeof(struct conf_peer));
		memcpy(&temp_cp, (char *)cp, INFO_ITEMSIZE(inpkt->mbz_itemsize));
		ZERO_SOCK(&peeraddr);

		fl = 0;
		if (temp_cp.flags & CONF_FLAG_PREFER)
			fl |= FLAG_PREFER;
		if (temp_cp.flags & CONF_FLAG_BURST)
		    fl |= FLAG_BURST;
		if (temp_cp.flags & CONF_FLAG_IBURST)
		    fl |= FLAG_IBURST;
#ifdef OPENSSL
		if (temp_cp.flags & CONF_FLAG_SKEY)
			fl |= FLAG_SKEY;
#endif /* OPENSSL */		
		if (client_v6_capable && temp_cp.v6_flag != 0) {
			AF(&peeraddr) = AF_INET6;
			SOCK_ADDR6(&peeraddr) = temp_cp.peeraddr6; 
		} else {
			AF(&peeraddr) = AF_INET;
			NSRCADR(&peeraddr) = temp_cp.peeraddr;
			/*
			 * Make sure the address is valid
			 */
			if (!ISREFCLOCKADR(&peeraddr) && 
			    ISBADADR(&peeraddr)) {
				req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
				return;
			}

		}
		NSRCPORT(&peeraddr) = htons(NTP_PORT);
#ifdef ISC_PLATFORM_HAVESALEN
		peeraddr.sa.sa_len = SOCKLEN(&peeraddr);
#endif

		/* XXX W2DO? minpoll/maxpoll arguments ??? */
		if (peer_config(&peeraddr, (struct interface *)0,
		    temp_cp.hmode, temp_cp.version, temp_cp.minpoll, 
		    temp_cp.maxpoll, fl, temp_cp.ttl, temp_cp.keyid,
		    NULL) == 0) {
			req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
			return;
		}

		/*
		 * ntp_intres.c uses REQ_CONFIG/doconf() to add each
		 * server after its name is resolved.  If we have been
		 * disconnected from the network, it may notice the
		 * network has returned and add the first server while
		 * the relevant interface is still disabled, awaiting
		 * the next interface rescan.  To get things moving
		 * more quickly, trigger an interface scan now, except
		 * if we have done so in the last half minute.
		 */
		if (soonest_ifrescan_time < current_time) {
			soonest_ifrescan_time = current_time + 30;
			timer_interfacetimeout(current_time);
			DPRINTF(1, ("do_conf triggering interface rescan\n"));
		}

		cp = (struct conf_peer *)
		    ((char *)cp + INFO_ITEMSIZE(inpkt->mbz_itemsize));
	}

	req_ack(srcadr, inter, inpkt, INFO_OKAY);
}

#if 0
/* XXX */
/*
 * dns_a - Snarf DNS info for an association ID
 */
static void
dns_a(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_dns_assoc *dp;
	register int items;
	struct sockaddr_in peeraddr;

	/*
	 * Do a check of everything to see that it looks
	 * okay.  If not, complain about it.  Note we are
	 * very picky here.
	 */
	items = INFO_NITEMS(inpkt->err_nitems);
	dp = (struct info_dns_assoc *)inpkt->data;

	/*
	 * Looks okay, try it out
	 */
	items = INFO_NITEMS(inpkt->err_nitems);
	dp = (struct info_dns_assoc *)inpkt->data;
	memset((char *)&peeraddr, 0, sizeof(struct sockaddr_in));
	peeraddr.sin_family = AF_INET;
	peeraddr.sin_port = htons(NTP_PORT);

	/*
	 * Make sure the address is valid
	 */
	if (!ISREFCLOCKADR(&peeraddr) && ISBADADR(&peeraddr)) {
		msyslog(LOG_ERR, "dns_a: !ISREFCLOCKADR && ISBADADR");
		req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
		return;
	}

	while (items-- > 0) {
		associd_t associd;
		size_t hnl;
		struct peer *peer;
		int bogon = 0;

		associd = dp->associd;
		peer = findpeerbyassoc(associd);
		if (peer == 0 || peer->flags & FLAG_REFCLOCK) {
			msyslog(LOG_ERR, "dns_a: %s",
				(peer == 0)
				? "peer == 0"
				: "peer->flags & FLAG_REFCLOCK");
			++bogon;
		}
		peeraddr.sin_addr.s_addr = dp->peeraddr;
		for (hnl = 0; dp->hostname[hnl] && hnl < sizeof dp->hostname; ++hnl) ;
		if (hnl >= sizeof dp->hostname) {
			msyslog(LOG_ERR, "dns_a: hnl (%ld) >= %ld",
				(long)hnl, (long)sizeof dp->hostname);
			++bogon;
		}

		msyslog(LOG_INFO, "dns_a: <%s> for %s, AssocID %d, bogon %d",
			dp->hostname,
			stoa((sockaddr_u *)&peeraddr), associd,
			bogon);

		if (bogon) {
			/* If it didn't work */
			req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
			return;
		} else {
#if 0
#ifdef PUBKEY
			crypto_public(peer, dp->hostname);
#endif /* PUBKEY */
#endif
		}

		dp++;
	}

	req_ack(srcadr, inter, inpkt, INFO_OKAY);
}
#endif /* 0 */

/*
 * do_unconf - remove a peer from the configuration list
 */
static void
do_unconf(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct conf_unpeer *cp;
	struct conf_unpeer temp_cp;
	register int items;
	register struct peer *peer;
	sockaddr_u peeraddr;
	int bad, found;

	/*
	 * This is a bit unstructured, but I like to be careful.
	 * We check to see that every peer exists and is actually
	 * configured.  If so, we remove them.  If not, we return
	 * an error.
	 */
	items = INFO_NITEMS(inpkt->err_nitems);
	cp = (struct conf_unpeer *)inpkt->data;

	bad = 0;
	while (items-- > 0 && !bad) {
		memset(&temp_cp, 0, sizeof(temp_cp));
		ZERO_SOCK(&peeraddr);
		memcpy(&temp_cp, cp, INFO_ITEMSIZE(inpkt->mbz_itemsize));
		if (client_v6_capable && temp_cp.v6_flag) {
			AF(&peeraddr) = AF_INET6;
			SOCK_ADDR6(&peeraddr) = temp_cp.peeraddr6;
		} else {
			AF(&peeraddr) = AF_INET;
			NSRCADR(&peeraddr) = temp_cp.peeraddr;
		}
		SET_PORT(&peeraddr, NTP_PORT);
#ifdef ISC_PLATFORM_HAVESALEN
		peeraddr.sa.sa_len = SOCKLEN(&peeraddr);
#endif
		found = 0;
		peer = NULL;

		DPRINTF(1, ("searching for %s\n", stoa(&peeraddr)));

		while (!found) {
			peer = findexistingpeer(&peeraddr, peer, -1, 0);
			if (!peer)
				break;
			if (peer->flags & FLAG_CONFIG)
				found = 1;
		}
		if (!found)
			bad = 1;
		cp = (struct conf_unpeer *)
			((char *)cp + INFO_ITEMSIZE(inpkt->mbz_itemsize));
	}

	if (bad) {
		req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
		return;
	}

	/*
	 * Now do it in earnest.
	 */

	items = INFO_NITEMS(inpkt->err_nitems);
	cp = (struct conf_unpeer *)inpkt->data;

	while (items-- > 0) {
		memset(&temp_cp, 0, sizeof(temp_cp));
		memset(&peeraddr, 0, sizeof(peeraddr));
		memcpy(&temp_cp, cp, INFO_ITEMSIZE(inpkt->mbz_itemsize));
		if (client_v6_capable && temp_cp.v6_flag) {
			AF(&peeraddr) = AF_INET6;
			SOCK_ADDR6(&peeraddr) = temp_cp.peeraddr6;
		} else {
			AF(&peeraddr) = AF_INET;
			NSRCADR(&peeraddr) = temp_cp.peeraddr;
		}
		SET_PORT(&peeraddr, NTP_PORT);
#ifdef ISC_PLATFORM_HAVESALEN
		peeraddr.sa.sa_len = SOCKLEN(&peeraddr);
#endif
		found = 0;
		peer = NULL;

		while (!found) {
			peer = findexistingpeer(&peeraddr, peer, -1, 0);
			if (!peer)
				break;
			if (peer->flags & FLAG_CONFIG)
				found = 1;
		}
		NTP_INSIST(found);
		NTP_INSIST(peer);

		peer_clear(peer, "GONE");
		unpeer(peer);

		cp = (struct conf_unpeer *)
			((char *)cp + INFO_ITEMSIZE(inpkt->mbz_itemsize));
	}

	req_ack(srcadr, inter, inpkt, INFO_OKAY);
}


/*
 * set_sys_flag - set system flags
 */
static void
set_sys_flag(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	setclr_flags(srcadr, inter, inpkt, 1);
}


/*
 * clr_sys_flag - clear system flags
 */
static void
clr_sys_flag(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	setclr_flags(srcadr, inter, inpkt, 0);
}


/*
 * setclr_flags - do the grunge work of flag setting/clearing
 */
static void
setclr_flags(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt,
	u_long set
	)
{
	struct conf_sys_flags *sf;
	u_int32 flags;
	int prev_kern_enable;

	prev_kern_enable = kern_enable;
	if (INFO_NITEMS(inpkt->err_nitems) > 1) {
		msyslog(LOG_ERR, "setclr_flags: err_nitems > 1");
		req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
		return;
	}

	sf = (struct conf_sys_flags *)inpkt->data;
	flags = ntohl(sf->flags);
	
	if (flags & ~(SYS_FLAG_BCLIENT | SYS_FLAG_PPS |
		      SYS_FLAG_NTP | SYS_FLAG_KERNEL | SYS_FLAG_MONITOR |
		      SYS_FLAG_FILEGEN | SYS_FLAG_AUTH | SYS_FLAG_CAL)) {
		msyslog(LOG_ERR, "setclr_flags: extra flags: %#x",
			flags & ~(SYS_FLAG_BCLIENT | SYS_FLAG_PPS |
				  SYS_FLAG_NTP | SYS_FLAG_KERNEL |
				  SYS_FLAG_MONITOR | SYS_FLAG_FILEGEN |
				  SYS_FLAG_AUTH | SYS_FLAG_CAL));
		req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
		return;
	}

	if (flags & SYS_FLAG_BCLIENT)
		proto_config(PROTO_BROADCLIENT, set, 0., NULL);
	if (flags & SYS_FLAG_PPS)
		proto_config(PROTO_PPS, set, 0., NULL);
	if (flags & SYS_FLAG_NTP)
		proto_config(PROTO_NTP, set, 0., NULL);
	if (flags & SYS_FLAG_KERNEL)
		proto_config(PROTO_KERNEL, set, 0., NULL);
	if (flags & SYS_FLAG_MONITOR)
		proto_config(PROTO_MONITOR, set, 0., NULL);
	if (flags & SYS_FLAG_FILEGEN)
		proto_config(PROTO_FILEGEN, set, 0., NULL);
	if (flags & SYS_FLAG_AUTH)
		proto_config(PROTO_AUTHENTICATE, set, 0., NULL);
	if (flags & SYS_FLAG_CAL)
		proto_config(PROTO_CAL, set, 0., NULL);
	req_ack(srcadr, inter, inpkt, INFO_OKAY);

	/* Reset the kernel ntp parameters if the kernel flag changed. */
	if (prev_kern_enable && !kern_enable)
	     	loop_config(LOOP_KERN_CLEAR, 0.0);
	if (!prev_kern_enable && kern_enable)
	     	loop_config(LOOP_DRIFTCOMP, drift_comp);
}

/*
 * list_restrict4 - recursive helper for list_restrict dumps IPv4
 *		    restriction list in reverse order.
 */
static void
list_restrict4(
	restrict_u *		res,
	struct info_restrict **	ppir
	)
{
	struct info_restrict *	pir;

	if (res->link != NULL)
		list_restrict4(res->link, ppir);

	pir = *ppir;
	pir->addr = htonl(res->u.v4.addr);
	if (client_v6_capable) 
		pir->v6_flag = 0;
	pir->mask = htonl(res->u.v4.mask);
	pir->count = htonl(res->count);
	pir->flags = htons(res->flags);
	pir->mflags = htons(res->mflags);
	*ppir = (struct info_restrict *)more_pkt();
}


/*
 * list_restrict6 - recursive helper for list_restrict dumps IPv6
 *		    restriction list in reverse order.
 */
static void
list_restrict6(
	restrict_u *		res,
	struct info_restrict **	ppir
	)
{
	struct info_restrict *	pir;

	if (res->link != NULL)
		list_restrict6(res->link, ppir);

	pir = *ppir;
	pir->addr6 = res->u.v6.addr; 
	pir->mask6 = res->u.v6.mask;
	pir->v6_flag = 1;
	pir->count = htonl(res->count);
	pir->flags = htons(res->flags);
	pir->mflags = htons(res->mflags);
	*ppir = (struct info_restrict *)more_pkt();
}


/*
 * list_restrict - return the restrict list
 */
static void
list_restrict(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	struct info_restrict *ir;

	DPRINTF(3, ("wants restrict list summary\n"));

	ir = (struct info_restrict *)prepare_pkt(srcadr, inter, inpkt,
	    v6sizeof(struct info_restrict));
	
	/*
	 * The restriction lists are kept sorted in the reverse order
	 * than they were originally.  To preserve the output semantics,
	 * dump each list in reverse order.  A recursive helper function
	 * achieves that.
	 */
	list_restrict4(restrictlist4, &ir);
	if (client_v6_capable)
		list_restrict6(restrictlist6, &ir);
	flush_pkt();
}


/*
 * do_resaddflags - add flags to a restrict entry (or create one)
 */
static void
do_resaddflags(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	do_restrict(srcadr, inter, inpkt, RESTRICT_FLAGS);
}



/*
 * do_ressubflags - remove flags from a restrict entry
 */
static void
do_ressubflags(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	do_restrict(srcadr, inter, inpkt, RESTRICT_UNFLAG);
}


/*
 * do_unrestrict - remove a restrict entry from the list
 */
static void
do_unrestrict(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	do_restrict(srcadr, inter, inpkt, RESTRICT_REMOVE);
}


/*
 * do_restrict - do the dirty stuff of dealing with restrictions
 */
static void
do_restrict(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt,
	int op
	)
{
	register struct conf_restrict *cr;
	register int items;
	sockaddr_u matchaddr;
	sockaddr_u matchmask;
	int bad;

	/*
	 * Do a check of the flags to make sure that only
	 * the NTPPORT flag is set, if any.  If not, complain
	 * about it.  Note we are very picky here.
	 */
	items = INFO_NITEMS(inpkt->err_nitems);
	cr = (struct conf_restrict *)inpkt->data;

	bad = 0;
	cr->flags = ntohs(cr->flags);
	cr->mflags = ntohs(cr->mflags);
	while (items-- > 0 && !bad) {
		if (cr->mflags & ~(RESM_NTPONLY))
		    bad |= 1;
		if (cr->flags & ~(RES_ALLFLAGS))
		    bad |= 2;
		if (cr->mask != htonl(INADDR_ANY)) {
			if (client_v6_capable && cr->v6_flag != 0) {
				if (IN6_IS_ADDR_UNSPECIFIED(&cr->addr6))
					bad |= 4;
			} else
				if (cr->addr == htonl(INADDR_ANY))
					bad |= 8;
		}
		cr = (struct conf_restrict *)((char *)cr +
		    INFO_ITEMSIZE(inpkt->mbz_itemsize));
	}

	if (bad) {
		msyslog(LOG_ERR, "do_restrict: bad = %#x", bad);
		req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
		return;
	}

	/*
	 * Looks okay, try it out
	 */
	items = INFO_NITEMS(inpkt->err_nitems);
	cr = (struct conf_restrict *)inpkt->data;
	ZERO_SOCK(&matchaddr);
	ZERO_SOCK(&matchmask);

	while (items-- > 0) {
		if (client_v6_capable && cr->v6_flag) {
			AF(&matchaddr) = AF_INET6;
			AF(&matchmask) = AF_INET6;
			SOCK_ADDR6(&matchaddr) = cr->addr6;
			SOCK_ADDR6(&matchmask) = cr->mask6;
		} else {
			AF(&matchaddr) = AF_INET;
			AF(&matchmask) = AF_INET;
			NSRCADR(&matchaddr) = cr->addr;
			NSRCADR(&matchmask) = cr->mask;
		}
		hack_restrict(op, &matchaddr, &matchmask, cr->mflags,
			 cr->flags);
		cr++;
	}

	req_ack(srcadr, inter, inpkt, INFO_OKAY);
}


/*
 * mon_getlist - return monitor data
 */
static void
mon_getlist_0(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_monitor *im;
	register struct mon_data *md;
	extern struct mon_data mon_mru_list;
	extern int mon_enabled;

#ifdef DEBUG
	if (debug > 2)
	    printf("wants monitor 0 list\n");
#endif
	if (!mon_enabled) {
		req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
		return;
	}
	im = (struct info_monitor *)prepare_pkt(srcadr, inter, inpkt,
	    v6sizeof(struct info_monitor));
	for (md = mon_mru_list.mru_next; md != &mon_mru_list && im != 0;
	     md = md->mru_next) {
		im->lasttime = htonl((u_int32)((current_time -
		    md->firsttime) / md->count));
		im->firsttime = htonl((u_int32)(current_time - md->lasttime));
		im->restr = htonl((u_int32)md->flags);
		im->count = htonl((u_int32)(md->count));
		if (IS_IPV6(&md->rmtadr)) {
			if (!client_v6_capable)
				continue;
			im->addr6 = SOCK_ADDR6(&md->rmtadr);
			im->v6_flag = 1;
		} else {
			im->addr = NSRCADR(&md->rmtadr);
			if (client_v6_capable)
				im->v6_flag = 0;
		}
		im->port = md->rmtport;
		im->mode = md->mode;
		im->version = md->version;
		im = (struct info_monitor *)more_pkt();
	}
	flush_pkt();
}

/*
 * mon_getlist - return monitor data
 */
static void
mon_getlist_1(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_monitor_1 *im;
	register struct mon_data *md;
	extern struct mon_data mon_mru_list;
	extern int mon_enabled;

	if (!mon_enabled) {
		req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
		return;
	}
	im = (struct info_monitor_1 *)prepare_pkt(srcadr, inter, inpkt,
	    v6sizeof(struct info_monitor_1));
	for (md = mon_mru_list.mru_next; md != &mon_mru_list && im != 0;
	     md = md->mru_next) {
		im->lasttime = htonl((u_int32)((current_time -
		    md->firsttime) / md->count));
		im->firsttime = htonl((u_int32)(current_time - md->lasttime));
		im->restr = htonl((u_int32)md->flags);
		im->count = htonl((u_int32)md->count);
		if (IS_IPV6(&md->rmtadr)) {
			if (!client_v6_capable)
				continue;
			im->addr6 = SOCK_ADDR6(&md->rmtadr);
			im->v6_flag = 1;
			im->daddr6 = SOCK_ADDR6(&md->interface->sin);
		} else {
			im->addr = NSRCADR(&md->rmtadr);
			if (client_v6_capable)
				im->v6_flag = 0;
			if (MDF_BCAST == md->cast_flags)
				im->daddr = NSRCADR(&md->interface->bcast);
			else if (md->cast_flags) {
				im->daddr = NSRCADR(&md->interface->sin);
				if (!im->daddr)
					im->daddr = NSRCADR(&md->interface->bcast);
			} else
				im->daddr = 4;
		}
		im->flags = htonl(md->cast_flags);
		im->port = md->rmtport;
		im->mode = md->mode;
		im->version = md->version;
		im = (struct info_monitor_1 *)more_pkt();
	}
	flush_pkt();
}

/*
 * Module entry points and the flags they correspond with
 */
struct reset_entry {
	int flag;		/* flag this corresponds to */
	void (*handler) (void); /* routine to handle request */
};

struct reset_entry reset_entries[] = {
	{ RESET_FLAG_ALLPEERS,	peer_all_reset },
	{ RESET_FLAG_IO,	io_clr_stats },
	{ RESET_FLAG_SYS,	proto_clr_stats },
	{ RESET_FLAG_MEM,	peer_clr_stats },
	{ RESET_FLAG_TIMER,	timer_clr_stats },
	{ RESET_FLAG_AUTH,	reset_auth_stats },
	{ RESET_FLAG_CTL,	ctl_clr_stats },
	{ 0,			0 }
};

/*
 * reset_stats - reset statistic counters here and there
 */
static void
reset_stats(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	struct reset_flags *rflags;
	u_long flags;
	struct reset_entry *rent;

	if (INFO_NITEMS(inpkt->err_nitems) > 1) {
		msyslog(LOG_ERR, "reset_stats: err_nitems > 1");
		req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
		return;
	}

	rflags = (struct reset_flags *)inpkt->data;
	flags = ntohl(rflags->flags);

	if (flags & ~RESET_ALLFLAGS) {
		msyslog(LOG_ERR, "reset_stats: reset leaves %#lx",
			flags & ~RESET_ALLFLAGS);
		req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
		return;
	}

	for (rent = reset_entries; rent->flag != 0; rent++) {
		if (flags & rent->flag)
			(*rent->handler)();
	}
	req_ack(srcadr, inter, inpkt, INFO_OKAY);
}


/*
 * reset_peer - clear a peer's statistics
 */
static void
reset_peer(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	struct conf_unpeer *cp;
	int items;
	struct peer *peer;
	sockaddr_u peeraddr;
	int bad;

	/*
	 * We check first to see that every peer exists.  If not,
	 * we return an error.
	 */

	items = INFO_NITEMS(inpkt->err_nitems);
	cp = (struct conf_unpeer *)inpkt->data;

	bad = 0;
	while (items-- > 0 && !bad) {
		ZERO_SOCK(&peeraddr);
		if (client_v6_capable && cp->v6_flag) {
			AF(&peeraddr) = AF_INET6;
			SOCK_ADDR6(&peeraddr) = cp->peeraddr6;
		} else {
			AF(&peeraddr) = AF_INET;
			NSRCADR(&peeraddr) = cp->peeraddr;
		}

#ifdef ISC_PLATFORM_HAVESALEN
		peeraddr.sa.sa_len = SOCKLEN(&peeraddr);
#endif
		peer = findexistingpeer(&peeraddr, NULL, -1, 0);
		if (NULL == peer)
			bad++;
		cp = (struct conf_unpeer *)((char *)cp +
		    INFO_ITEMSIZE(inpkt->mbz_itemsize));
	}

	if (bad) {
		req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
		return;
	}

	/*
	 * Now do it in earnest.
	 */

	items = INFO_NITEMS(inpkt->err_nitems);
	cp = (struct conf_unpeer *)inpkt->data;
	while (items-- > 0) {
		ZERO_SOCK(&peeraddr);
		if (client_v6_capable && cp->v6_flag) {
			AF(&peeraddr) = AF_INET6;
			SOCK_ADDR6(&peeraddr) = cp->peeraddr6;
		} else {
			AF(&peeraddr) = AF_INET;
			NSRCADR(&peeraddr) = cp->peeraddr;
		}
		SET_PORT(&peeraddr, 123);
#ifdef ISC_PLATFORM_HAVESALEN
		peeraddr.sa.sa_len = SOCKLEN(&peeraddr);
#endif
		peer = findexistingpeer(&peeraddr, NULL, -1, 0);
		while (peer != NULL) {
			peer_reset(peer);
			peer = findexistingpeer(&peeraddr, peer, -1, 0);
		}
		cp = (struct conf_unpeer *)((char *)cp +
		    INFO_ITEMSIZE(inpkt->mbz_itemsize));
	}

	req_ack(srcadr, inter, inpkt, INFO_OKAY);
}


/*
 * do_key_reread - reread the encryption key file
 */
static void
do_key_reread(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	rereadkeys();
	req_ack(srcadr, inter, inpkt, INFO_OKAY);
}


/*
 * trust_key - make one or more keys trusted
 */
static void
trust_key(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	do_trustkey(srcadr, inter, inpkt, 1);
}


/*
 * untrust_key - make one or more keys untrusted
 */
static void
untrust_key(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	do_trustkey(srcadr, inter, inpkt, 0);
}


/*
 * do_trustkey - make keys either trustable or untrustable
 */
static void
do_trustkey(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt,
	u_long trust
	)
{
	register u_long *kp;
	register int items;

	items = INFO_NITEMS(inpkt->err_nitems);
	kp = (u_long *)inpkt->data;
	while (items-- > 0) {
		authtrust(*kp, trust);
		kp++;
	}

	req_ack(srcadr, inter, inpkt, INFO_OKAY);
}


/*
 * get_auth_info - return some stats concerning the authentication module
 */
static void
get_auth_info(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_auth *ia;

	/*
	 * Importations from the authentication module
	 */
	extern u_long authnumkeys;
	extern int authnumfreekeys;
	extern u_long authkeylookups;
	extern u_long authkeynotfound;
	extern u_long authencryptions;
	extern u_long authdecryptions;
	extern u_long authkeyuncached;
	extern u_long authkeyexpired;

	ia = (struct info_auth *)prepare_pkt(srcadr, inter, inpkt,
					     sizeof(struct info_auth));

	ia->numkeys = htonl((u_int32)authnumkeys);
	ia->numfreekeys = htonl((u_int32)authnumfreekeys);
	ia->keylookups = htonl((u_int32)authkeylookups);
	ia->keynotfound = htonl((u_int32)authkeynotfound);
	ia->encryptions = htonl((u_int32)authencryptions);
	ia->decryptions = htonl((u_int32)authdecryptions);
	ia->keyuncached = htonl((u_int32)authkeyuncached);
	ia->expired = htonl((u_int32)authkeyexpired);
	ia->timereset = htonl((u_int32)(current_time - auth_timereset));
	
	(void) more_pkt();
	flush_pkt();
}



/*
 * reset_auth_stats - reset the authentication stat counters.  Done here
 *		      to keep ntp-isms out of the authentication module
 */
static void
reset_auth_stats(void)
{
	/*
	 * Importations from the authentication module
	 */
	extern u_long authkeylookups;
	extern u_long authkeynotfound;
	extern u_long authencryptions;
	extern u_long authdecryptions;
	extern u_long authkeyuncached;

	authkeylookups = 0;
	authkeynotfound = 0;
	authencryptions = 0;
	authdecryptions = 0;
	authkeyuncached = 0;
	auth_timereset = current_time;
}


/*
 * req_get_traps - return information about current trap holders
 */
static void
req_get_traps(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_trap *it;
	register struct ctl_trap *tr;
	register int i;

	/*
	 * Imported from the control module
	 */
	extern struct ctl_trap ctl_trap[];
	extern int num_ctl_traps;

	if (num_ctl_traps == 0) {
		req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
		return;
	}

	it = (struct info_trap *)prepare_pkt(srcadr, inter, inpkt,
	    v6sizeof(struct info_trap));

	for (i = 0, tr = ctl_trap; i < CTL_MAXTRAPS; i++, tr++) {
		if (tr->tr_flags & TRAP_INUSE) {
			if (IS_IPV4(&tr->tr_addr)) {
				if (tr->tr_localaddr == any_interface)
					it->local_address = 0;
				else
					it->local_address
					    = NSRCADR(&tr->tr_localaddr->sin);
				it->trap_address = NSRCADR(&tr->tr_addr);
				if (client_v6_capable)
					it->v6_flag = 0;
			} else {
				if (!client_v6_capable)
					continue;
				it->local_address6 
				    = SOCK_ADDR6(&tr->tr_localaddr->sin);
				it->trap_address6 = SOCK_ADDR6(&tr->tr_addr);
				it->v6_flag = 1;
			}
			it->trap_port = NSRCPORT(&tr->tr_addr);
			it->sequence = htons(tr->tr_sequence);
			it->settime = htonl((u_int32)(current_time - tr->tr_settime));
			it->origtime = htonl((u_int32)(current_time - tr->tr_origtime));
			it->resets = htonl((u_int32)tr->tr_resets);
			it->flags = htonl((u_int32)tr->tr_flags);
			it = (struct info_trap *)more_pkt();
		}
	}
	flush_pkt();
}


/*
 * req_set_trap - configure a trap
 */
static void
req_set_trap(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	do_setclr_trap(srcadr, inter, inpkt, 1);
}



/*
 * req_clr_trap - unconfigure a trap
 */
static void
req_clr_trap(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	do_setclr_trap(srcadr, inter, inpkt, 0);
}



/*
 * do_setclr_trap - do the grunge work of (un)configuring a trap
 */
static void
do_setclr_trap(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt,
	int set
	)
{
	register struct conf_trap *ct;
	register struct interface *linter;
	int res;
	sockaddr_u laddr;

	/*
	 * Prepare sockaddr
	 */
	ZERO_SOCK(&laddr);
	AF(&laddr) = AF(srcadr);
	SET_PORT(&laddr, NTP_PORT);

	/*
	 * Restrict ourselves to one item only.  This eliminates
	 * the error reporting problem.
	 */
	if (INFO_NITEMS(inpkt->err_nitems) > 1) {
		msyslog(LOG_ERR, "do_setclr_trap: err_nitems > 1");
		req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
		return;
	}
	ct = (struct conf_trap *)inpkt->data;

	/*
	 * Look for the local interface.  If none, use the default.
	 */
	if (ct->local_address == 0) {
		linter = any_interface;
	} else {
		if (IS_IPV4(&laddr))
			NSRCADR(&laddr) = ct->local_address;
		else
			SOCK_ADDR6(&laddr) = ct->local_address6;
		linter = findinterface(&laddr);
		if (NULL == linter) {
			req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
			return;
		}
	}

	if (IS_IPV4(&laddr))
		NSRCADR(&laddr) = ct->trap_address;
	else
		SOCK_ADDR6(&laddr) = ct->trap_address6;
	if (ct->trap_port)
		NSRCPORT(&laddr) = ct->trap_port;
	else
		SET_PORT(&laddr, TRAPPORT);

	if (set) {
		res = ctlsettrap(&laddr, linter, 0,
				 INFO_VERSION(inpkt->rm_vn_mode));
	} else {
		res = ctlclrtrap(&laddr, linter, 0);
	}

	if (!res) {
		req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
	} else {
		req_ack(srcadr, inter, inpkt, INFO_OKAY);
	}
	return;
}



/*
 * set_request_keyid - set the keyid used to authenticate requests
 */
static void
set_request_keyid(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	keyid_t *pkeyid;

	/*
	 * Restrict ourselves to one item only.
	 */
	if (INFO_NITEMS(inpkt->err_nitems) > 1) {
		msyslog(LOG_ERR, "set_request_keyid: err_nitems > 1");
		req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
		return;
	}

	pkeyid = (keyid_t *)inpkt->data;
	info_auth_keyid = ntohl(*pkeyid);
	req_ack(srcadr, inter, inpkt, INFO_OKAY);
}



/*
 * set_control_keyid - set the keyid used to authenticate requests
 */
static void
set_control_keyid(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	keyid_t *pkeyid;
	extern keyid_t ctl_auth_keyid;

	/*
	 * Restrict ourselves to one item only.
	 */
	if (INFO_NITEMS(inpkt->err_nitems) > 1) {
		msyslog(LOG_ERR, "set_control_keyid: err_nitems > 1");
		req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
		return;
	}

	pkeyid = (keyid_t *)inpkt->data;
	ctl_auth_keyid = ntohl(*pkeyid);
	req_ack(srcadr, inter, inpkt, INFO_OKAY);
}



/*
 * get_ctl_stats - return some stats concerning the control message module
 */
static void
get_ctl_stats(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_control *ic;

	/*
	 * Importations from the control module
	 */
	extern u_long ctltimereset;
	extern u_long numctlreq;
	extern u_long numctlbadpkts;
	extern u_long numctlresponses;
	extern u_long numctlfrags;
	extern u_long numctlerrors;
	extern u_long numctltooshort;
	extern u_long numctlinputresp;
	extern u_long numctlinputfrag;
	extern u_long numctlinputerr;
	extern u_long numctlbadoffset;
	extern u_long numctlbadversion;
	extern u_long numctldatatooshort;
	extern u_long numctlbadop;
	extern u_long numasyncmsgs;

	ic = (struct info_control *)prepare_pkt(srcadr, inter, inpkt,
						sizeof(struct info_control));

	ic->ctltimereset = htonl((u_int32)(current_time - ctltimereset));
	ic->numctlreq = htonl((u_int32)numctlreq);
	ic->numctlbadpkts = htonl((u_int32)numctlbadpkts);
	ic->numctlresponses = htonl((u_int32)numctlresponses);
	ic->numctlfrags = htonl((u_int32)numctlfrags);
	ic->numctlerrors = htonl((u_int32)numctlerrors);
	ic->numctltooshort = htonl((u_int32)numctltooshort);
	ic->numctlinputresp = htonl((u_int32)numctlinputresp);
	ic->numctlinputfrag = htonl((u_int32)numctlinputfrag);
	ic->numctlinputerr = htonl((u_int32)numctlinputerr);
	ic->numctlbadoffset = htonl((u_int32)numctlbadoffset);
	ic->numctlbadversion = htonl((u_int32)numctlbadversion);
	ic->numctldatatooshort = htonl((u_int32)numctldatatooshort);
	ic->numctlbadop = htonl((u_int32)numctlbadop);
	ic->numasyncmsgs = htonl((u_int32)numasyncmsgs);

	(void) more_pkt();
	flush_pkt();
}


#ifdef KERNEL_PLL
/*
 * get_kernel_info - get kernel pll/pps information
 */
static void
get_kernel_info(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_kernel *ik;
	struct timex ntx;

	if (!pll_control) {
		req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
		return;
	}

	memset((char *)&ntx, 0, sizeof(ntx));
	if (ntp_adjtime(&ntx) < 0)
		msyslog(LOG_ERR, "get_kernel_info: ntp_adjtime() failed: %m");
	ik = (struct info_kernel *)prepare_pkt(srcadr, inter, inpkt,
	    sizeof(struct info_kernel));

	/*
	 * pll variables
	 */
	ik->offset = htonl((u_int32)ntx.offset);
	ik->freq = htonl((u_int32)ntx.freq);
	ik->maxerror = htonl((u_int32)ntx.maxerror);
	ik->esterror = htonl((u_int32)ntx.esterror);
	ik->status = htons(ntx.status);
	ik->constant = htonl((u_int32)ntx.constant);
	ik->precision = htonl((u_int32)ntx.precision);
	ik->tolerance = htonl((u_int32)ntx.tolerance);

	/*
	 * pps variables
	 */
	ik->ppsfreq = htonl((u_int32)ntx.ppsfreq);
	ik->jitter = htonl((u_int32)ntx.jitter);
	ik->shift = htons(ntx.shift);
	ik->stabil = htonl((u_int32)ntx.stabil);
	ik->jitcnt = htonl((u_int32)ntx.jitcnt);
	ik->calcnt = htonl((u_int32)ntx.calcnt);
	ik->errcnt = htonl((u_int32)ntx.errcnt);
	ik->stbcnt = htonl((u_int32)ntx.stbcnt);
	
	(void) more_pkt();
	flush_pkt();
}
#endif /* KERNEL_PLL */


#ifdef REFCLOCK
/*
 * get_clock_info - get info about a clock
 */
static void
get_clock_info(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_clock *ic;
	register u_int32 *clkaddr;
	register int items;
	struct refclockstat clock_stat;
	sockaddr_u addr;
	l_fp ltmp;

	ZERO_SOCK(&addr);
	AF(&addr) = AF_INET;
#ifdef ISC_PLATFORM_HAVESALEN
	addr.sa.sa_len = SOCKLEN(&addr);
#endif
	SET_PORT(&addr, NTP_PORT);
	items = INFO_NITEMS(inpkt->err_nitems);
	clkaddr = (u_int32 *) inpkt->data;

	ic = (struct info_clock *)prepare_pkt(srcadr, inter, inpkt,
					      sizeof(struct info_clock));

	while (items-- > 0) {
		NSRCADR(&addr) = *clkaddr++;
		if (!ISREFCLOCKADR(&addr) ||
		    findexistingpeer(&addr, NULL, -1, 0) == NULL) {
			req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
			return;
		}

		clock_stat.kv_list = (struct ctl_var *)0;

		refclock_control(&addr, NULL, &clock_stat);

		ic->clockadr = NSRCADR(&addr);
		ic->type = clock_stat.type;
		ic->flags = clock_stat.flags;
		ic->lastevent = clock_stat.lastevent;
		ic->currentstatus = clock_stat.currentstatus;
		ic->polls = htonl((u_int32)clock_stat.polls);
		ic->noresponse = htonl((u_int32)clock_stat.noresponse);
		ic->badformat = htonl((u_int32)clock_stat.badformat);
		ic->baddata = htonl((u_int32)clock_stat.baddata);
		ic->timestarted = htonl((u_int32)clock_stat.timereset);
		DTOLFP(clock_stat.fudgetime1, &ltmp);
		HTONL_FP(&ltmp, &ic->fudgetime1);
		DTOLFP(clock_stat.fudgetime2, &ltmp);
		HTONL_FP(&ltmp, &ic->fudgetime2);
		ic->fudgeval1 = htonl((u_int32)clock_stat.fudgeval1);
		ic->fudgeval2 = htonl(clock_stat.fudgeval2);

		free_varlist(clock_stat.kv_list);

		ic = (struct info_clock *)more_pkt();
	}
	flush_pkt();
}



/*
 * set_clock_fudge - get a clock's fudge factors
 */
static void
set_clock_fudge(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register struct conf_fudge *cf;
	register int items;
	struct refclockstat clock_stat;
	sockaddr_u addr;
	l_fp ltmp;

	ZERO_SOCK(&addr);
	memset((char *)&clock_stat, 0, sizeof clock_stat);
	items = INFO_NITEMS(inpkt->err_nitems);
	cf = (struct conf_fudge *) inpkt->data;

	while (items-- > 0) {
		AF(&addr) = AF_INET;
		NSRCADR(&addr) = cf->clockadr;
#ifdef ISC_PLATFORM_HAVESALEN
		addr.sa.sa_len = SOCKLEN(&addr);
#endif
		SET_PORT(&addr, NTP_PORT);
		if (!ISREFCLOCKADR(&addr) ||
		    findexistingpeer(&addr, NULL, -1, 0) == 0) {
			req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
			return;
		}

		switch(ntohl(cf->which)) {
		    case FUDGE_TIME1:
			NTOHL_FP(&cf->fudgetime, &ltmp);
			LFPTOD(&ltmp, clock_stat.fudgetime1);
			clock_stat.haveflags = CLK_HAVETIME1;
			break;
		    case FUDGE_TIME2:
			NTOHL_FP(&cf->fudgetime, &ltmp);
			LFPTOD(&ltmp, clock_stat.fudgetime2);
			clock_stat.haveflags = CLK_HAVETIME2;
			break;
		    case FUDGE_VAL1:
			clock_stat.fudgeval1 = ntohl(cf->fudgeval_flags);
			clock_stat.haveflags = CLK_HAVEVAL1;
			break;
		    case FUDGE_VAL2:
			clock_stat.fudgeval2 = ntohl(cf->fudgeval_flags);
			clock_stat.haveflags = CLK_HAVEVAL2;
			break;
		    case FUDGE_FLAGS:
			clock_stat.flags = (u_char) (ntohl(cf->fudgeval_flags) & 0xf);
			clock_stat.haveflags =
				(CLK_HAVEFLAG1|CLK_HAVEFLAG2|CLK_HAVEFLAG3|CLK_HAVEFLAG4);
			break;
		    default:
			msyslog(LOG_ERR, "set_clock_fudge: default!");
			req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
			return;
		}

		refclock_control(&addr, &clock_stat, (struct refclockstat *)0);
	}

	req_ack(srcadr, inter, inpkt, INFO_OKAY);
}
#endif

#ifdef REFCLOCK
/*
 * get_clkbug_info - get debugging info about a clock
 */
static void
get_clkbug_info(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	register int i;
	register struct info_clkbug *ic;
	register u_int32 *clkaddr;
	register int items;
	struct refclockbug bug;
	sockaddr_u addr;

	ZERO_SOCK(&addr);
	AF(&addr) = AF_INET;
#ifdef ISC_PLATFORM_HAVESALEN
	addr.sa.sa_len = SOCKLEN(&addr);
#endif
	SET_PORT(&addr, NTP_PORT);
	items = INFO_NITEMS(inpkt->err_nitems);
	clkaddr = (u_int32 *) inpkt->data;

	ic = (struct info_clkbug *)prepare_pkt(srcadr, inter, inpkt,
					       sizeof(struct info_clkbug));

	while (items-- > 0) {
		NSRCADR(&addr) = *clkaddr++;
		if (!ISREFCLOCKADR(&addr) ||
		    findexistingpeer(&addr, NULL, -1, 0) == 0) {
			req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
			return;
		}

		memset((char *)&bug, 0, sizeof bug);
		refclock_buginfo(&addr, &bug);
		if (bug.nvalues == 0 && bug.ntimes == 0) {
			req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
			return;
		}

		ic->clockadr = NSRCADR(&addr);
		i = bug.nvalues;
		if (i > NUMCBUGVALUES)
		    i = NUMCBUGVALUES;
		ic->nvalues = (u_char)i;
		ic->svalues = htons((u_short) (bug.svalues & ((1<<i)-1)));
		while (--i >= 0)
		    ic->values[i] = htonl(bug.values[i]);

		i = bug.ntimes;
		if (i > NUMCBUGTIMES)
		    i = NUMCBUGTIMES;
		ic->ntimes = (u_char)i;
		ic->stimes = htonl(bug.stimes);
		while (--i >= 0) {
			HTONL_FP(&bug.times[i], &ic->times[i]);
		}

		ic = (struct info_clkbug *)more_pkt();
	}
	flush_pkt();
}
#endif

/*
 * receiver of interface structures
 */
static void
fill_info_if_stats(void *data, interface_info_t *interface_info)
{
	struct info_if_stats **ifsp = (struct info_if_stats **)data;
	struct info_if_stats *ifs = *ifsp;
	endpt *ep = interface_info->ep;
	
	memset(ifs, 0, sizeof(*ifs));
	
	if (IS_IPV6(&ep->sin)) {
		if (!client_v6_capable) {
			return;
		}
		ifs->v6_flag = 1;
		ifs->unaddr.addr6 = SOCK_ADDR6(&ep->sin);
		ifs->unbcast.addr6 = SOCK_ADDR6(&ep->bcast);
		ifs->unmask.addr6 = SOCK_ADDR6(&ep->mask);
	} else {
		ifs->v6_flag = 0;
		ifs->unaddr.addr = SOCK_ADDR4(&ep->sin);
		ifs->unbcast.addr = SOCK_ADDR4(&ep->bcast);
		ifs->unmask.addr = SOCK_ADDR4(&ep->mask);
	}
	ifs->v6_flag = htonl(ifs->v6_flag);
	strncpy(ifs->name, ep->name, sizeof(ifs->name));
	ifs->family = htons(ep->family);
	ifs->flags = htonl(ep->flags);
	ifs->last_ttl = htonl(ep->last_ttl);
	ifs->num_mcast = htonl(ep->num_mcast);
	ifs->received = htonl(ep->received);
	ifs->sent = htonl(ep->sent);
	ifs->notsent = htonl(ep->notsent);
	ifs->ifindex = htonl(ep->ifindex);
	/* scope no longer in struct interface, in in6_addr typically */
	ifs->scopeid = ifs->ifindex;
	ifs->ifnum = htonl(ep->ifnum);
	ifs->uptime = htonl(current_time - ep->starttime);
	ifs->ignore_packets = ep->ignore_packets;
	ifs->peercnt = htonl(ep->peercnt);
	ifs->action = interface_info->action;
	
	*ifsp = (struct info_if_stats *)more_pkt();
}

/*
 * get_if_stats - get interface statistics
 */
static void
get_if_stats(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	struct info_if_stats *ifs;

	DPRINTF(3, ("wants interface statistics\n"));

	ifs = (struct info_if_stats *)prepare_pkt(srcadr, inter, inpkt,
	    v6sizeof(struct info_if_stats));

	interface_enumerate(fill_info_if_stats, &ifs);
	
	flush_pkt();
}

static void
do_if_reload(
	sockaddr_u *srcadr,
	struct interface *inter,
	struct req_pkt *inpkt
	)
{
	struct info_if_stats *ifs;

	DPRINTF(3, ("wants interface reload\n"));

	ifs = (struct info_if_stats *)prepare_pkt(srcadr, inter, inpkt,
	    v6sizeof(struct info_if_stats));

	interface_update(fill_info_if_stats, &ifs);
	
	flush_pkt();
}

