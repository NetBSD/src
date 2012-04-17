/*	$NetBSD: ntp_control.h,v 1.1.1.1.6.1 2012/04/17 00:03:44 yamt Exp $	*/

/*
 * ntp_control.h - definitions related to NTP mode 6 control messages
 */

#include "ntp_types.h"

struct ntp_control {
	u_char li_vn_mode;		/* leap, version, mode */
	u_char r_m_e_op;		/* response, more, error, opcode */
	u_short sequence;		/* sequence number of request */
	u_short status;			/* status word for association */
	associd_t associd;		/* association ID */
	u_short offset;			/* offset of this batch of data */
	u_short count;			/* count of data in this packet */
	u_char data[(480 + MAX_MAC_LEN)]; /* data + auth */
};

/*
 * Length of the control header, in octets
 */
#define	CTL_HEADER_LEN		(offsetof(struct ntp_control, data))
#define	CTL_MAX_DATA_LEN	468


/*
 * Limits and things
 */
#define	CTL_MAXTRAPS	3		/* maximum number of traps we allow */
#define	CTL_TRAPTIME	(60*60)		/* time out traps in 1 hour */
#define	CTL_MAXAUTHSIZE	64		/* maximum size of an authen'ed req */

/*
 * Decoding for the r_m_e_op field
 */
#define	CTL_RESPONSE	0x80
#define	CTL_ERROR	0x40
#define	CTL_MORE	0x20
#define	CTL_OP_MASK	0x1f

#define	CTL_ISRESPONSE(r_m_e_op)	(((r_m_e_op) & 0x80) != 0)
#define	CTL_ISMORE(r_m_e_op)	(((r_m_e_op) & 0x20) != 0)
#define	CTL_ISERROR(r_m_e_op)	(((r_m_e_op) & 0x40) != 0)
#define	CTL_OP(r_m_e_op)	((r_m_e_op) & CTL_OP_MASK)

/*
 * Opcodes
 */
#define	CTL_OP_UNSPEC		0	/* unspeciffied */
#define	CTL_OP_READSTAT		1	/* read status */
#define	CTL_OP_READVAR		2	/* read variables */
#define	CTL_OP_WRITEVAR		3	/* write variables */
#define	CTL_OP_READCLOCK	4	/* read clock variables */
#define	CTL_OP_WRITECLOCK	5	/* write clock variables */
#define	CTL_OP_SETTRAP		6	/* set trap address */
#define	CTL_OP_ASYNCMSG		7	/* asynchronous message */
#define CTL_OP_CONFIGURE	8	/* runtime configuration */
#define CTL_OP_SAVECONFIG	9	/* save config to file */
#define	CTL_OP_UNSETTRAP	31	/* unset trap */

/*
 * {En,De}coding of the system status word
 */
#define	CTL_SST_TS_UNSPEC	0	/* unspec */
#define	CTL_SST_TS_ATOM		1	/* pps */
#define	CTL_SST_TS_LF		2	/* lf radio */
#define	CTL_SST_TS_HF		3	/* hf radio */
#define	CTL_SST_TS_UHF		4	/* uhf radio */
#define	CTL_SST_TS_LOCAL	5	/* local */
#define	CTL_SST_TS_NTP		6	/* ntp */
#define	CTL_SST_TS_UDPTIME	7	/* other */
#define	CTL_SST_TS_WRSTWTCH	8	/* wristwatch */
#define	CTL_SST_TS_TELEPHONE	9	/* telephone */

#define	CTL_SYS_MAXEVENTS	15

#define	CTL_SYS_STATUS(li, source, nevnt, evnt) \
		(((((unsigned short)(li))<< 14)&0xc000) | \
		(((source)<<8)&0x3f00) | \
		(((nevnt)<<4)&0x00f0) | \
		((evnt)&0x000f))

#define	CTL_SYS_LI(status)	(((status)>>14) & 0x3)
#define	CTL_SYS_SOURCE(status)	(((status)>>8) & 0x3f)
#define	CTL_SYS_NEVNT(status)	(((status)>>4) & 0xf)
#define	CTL_SYS_EVENT(status)	((status) & 0xf)

/*
 * {En,De}coding of the peer status word
 */
#define	CTL_PST_CONFIG		0x80
#define	CTL_PST_AUTHENABLE	0x40
#define	CTL_PST_AUTHENTIC	0x20
#define	CTL_PST_REACH		0x10
#define	CTL_PST_BCAST		0x08

#define	CTL_PST_SEL_REJECT	0	/*   reject */
#define	CTL_PST_SEL_SANE	1	/* x falsetick */
#define	CTL_PST_SEL_CORRECT	2	/* . excess */
#define	CTL_PST_SEL_SELCAND	3	/* - outlyer */
#define	CTL_PST_SEL_SYNCCAND	4	/* + candidate */
#define	CTL_PST_SEL_EXCESS	5	/* # backup */
#define	CTL_PST_SEL_SYSPEER	6	/* * sys.peer */
#define	CTL_PST_SEL_PPS		7	/* o pps.peer */

#define	CTL_PEER_MAXEVENTS	15

#define	CTL_PEER_STATUS(status, nevnt, evnt) \
		((((status)<<8) & 0xff00) | \
		(((nevnt)<<4) & 0x00f0) | \
		((evnt) & 0x000f))

#define	CTL_PEER_STATVAL(status)(((status)>>8) & 0xff)
#define	CTL_PEER_NEVNT(status)	(((status)>>4) & 0xf)
#define	CTL_PEER_EVENT(status)	((status) & 0xf)

/*
 * {En,De}coding of the clock status word
 */
#define	CTL_CLK_OKAY		0
#define	CTL_CLK_NOREPLY		1
#define	CTL_CLK_BADFORMAT	2
#define	CTL_CLK_FAULT		3
#define	CTL_CLK_PROPAGATION	4
#define	CTL_CLK_BADDATE		5
#define	CTL_CLK_BADTIME		6

#define	CTL_CLK_STATUS(status, event) \
		((((status)<<8) & 0xff00) | \
		((event) & 0x00ff))

/*
 * Error code responses returned when the E bit is set.
 */
#define	CERR_UNSPEC	0
#define	CERR_PERMISSION	1
#define	CERR_BADFMT	2
#define	CERR_BADOP	3
#define	CERR_BADASSOC	4
#define	CERR_UNKNOWNVAR	5
#define	CERR_BADVALUE	6
#define	CERR_RESTRICT	7

#define	CERR_NORESOURCE	CERR_PERMISSION	/* wish there was a different code */


/*
 * System variables we understand
 */
#define	CS_LEAP		1
#define	CS_STRATUM	2
#define	CS_PRECISION	3
#define	CS_ROOTDELAY	4
#define	CS_ROOTDISPERSION	5
#define	CS_REFID	6
#define	CS_REFTIME	7
#define	CS_POLL		8
#define	CS_PEERID	9
#define	CS_OFFSET	10
#define	CS_DRIFT	11
#define CS_JITTER	12
#define CS_ERROR	13
#define	CS_CLOCK	14
#define	CS_PROCESSOR	15
#define	CS_SYSTEM	16
#define CS_VERSION	17
#define	CS_STABIL	18
#define CS_VARLIST	19
#define CS_TAI          20
#define CS_LEAPTAB      21
#define CS_LEAPEND      22
#define	CS_RATE		23
#ifdef OPENSSL
#define CS_FLAGS	24
#define CS_HOST		25
#define CS_PUBLIC	26
#define	CS_CERTIF	27
#define	CS_SIGNATURE	28
#define	CS_REVTIME	29
#define	CS_GROUP	30
#define CS_DIGEST	31
#define	CS_MAXCODE	CS_DIGEST
#else
#define	CS_MAXCODE	CS_RATE
#endif /* OPENSSL */

/*
 * Peer variables we understand
 */
#define	CP_CONFIG	1
#define	CP_AUTHENABLE	2
#define	CP_AUTHENTIC	3
#define	CP_SRCADR	4
#define	CP_SRCPORT	5
#define	CP_DSTADR	6
#define	CP_DSTPORT	7
#define	CP_LEAP		8
#define	CP_HMODE	9
#define	CP_STRATUM	10
#define	CP_PPOLL	11
#define	CP_HPOLL	12
#define	CP_PRECISION	13
#define	CP_ROOTDELAY	14
#define	CP_ROOTDISPERSION	15
#define	CP_REFID	16
#define	CP_REFTIME	17
#define	CP_ORG		18
#define	CP_REC		19
#define	CP_XMT		20
#define	CP_REACH	21
#define	CP_UNREACH	22
#define	CP_TIMER	23
#define	CP_DELAY	24
#define	CP_OFFSET	25
#define CP_JITTER	26
#define	CP_DISPERSION	27
#define	CP_KEYID	28
#define	CP_FILTDELAY	29
#define	CP_FILTOFFSET	30
#define	CP_PMODE	31
#define	CP_RECEIVED	32
#define	CP_SENT		33
#define	CP_FILTERROR	34
#define	CP_FLASH	35
#define CP_TTL		36
#define CP_VARLIST	37
#define	CP_IN		38
#define	CP_OUT		39
#define	CP_RATE		40
#define	CP_BIAS		41
#ifdef OPENSSL
#define CP_FLAGS	42
#define CP_HOST		43
#define CP_VALID	44
#define	CP_INITSEQ	45
#define	CP_INITKEY	46
#define	CP_INITTSP	47
#define	CP_SIGNATURE	48
#define	CP_MAXCODE	CP_SIGNATURE
#else
#define	CP_MAXCODE	CP_BIAS
#endif /* OPENSSL */

/*
 * Clock variables we understand
 */
#define	CC_TYPE		1
#define	CC_TIMECODE	2
#define	CC_POLL		3
#define	CC_NOREPLY	4
#define	CC_BADFORMAT	5
#define	CC_BADDATA	6
#define	CC_FUDGETIME1	7
#define	CC_FUDGETIME2	8
#define	CC_FUDGEVAL1	9
#define	CC_FUDGEVAL2	10
#define	CC_FLAGS	11
#define	CC_DEVICE	12
#define CC_VARLIST	13
#define	CC_MAXCODE	CC_VARLIST

/*
 * Definition of the structure used internally to hold trap information.
 * ntp_request.c wants to see this.
 */
struct ctl_trap {
	sockaddr_u tr_addr;		/* address of trap recipient */
	struct interface *tr_localaddr;	/* interface to send this through */
	u_long tr_settime;		/* time trap was set */
	u_long tr_count;		/* async messages sent to this guy */
	u_long tr_origtime;		/* time trap was originally set */
	u_long tr_resets;		/* count of resets for this trap */
	u_short tr_sequence;		/* trap sequence id */
	u_char tr_flags;		/* trap flags */
	u_char tr_version;		/* version number of trapper */
};
extern struct ctl_trap ctl_trap[];

/*
 * Flag bits
 */
#define	TRAP_INUSE	0x1		/* this trap is active */
#define	TRAP_NONPRIO	0x2		/* this trap is non-priority */
#define	TRAP_CONFIGURED	0x4		/* this trap was configured */

/*
 * Types of things we may deal with
 * shared between ntpq and library
 */
#define	TYPE_SYS	1
#define	TYPE_PEER	2
#define	TYPE_CLOCK	3
