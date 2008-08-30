/*++
/* NAME
/*	smtp 3h
/* SUMMARY
/*	smtp client program
/* SYNOPSIS
/*	#include "smtp.h"
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <string.h>

 /*
  * Utility library.
  */
#include <vstream.h>
#include <vstring.h>
#include <argv.h>
#include <htable.h>

 /*
  * Global library.
  */
#include <deliver_request.h>
#include <scache.h>
#include <string_list.h>
#include <maps.h>
#include <tok822.h>
#include <dsn_buf.h>
#include <header_body_checks.h>

 /*
  * Postfix TLS library.
  */
#include <tls.h>

 /*
  * State information associated with each SMTP delivery request.
  * Session-specific state is stored separately.
  */
typedef struct SMTP_STATE {
    int     misc_flags;			/* processing flags, see below */
    VSTREAM *src;			/* queue file stream */
    const char *service;		/* transport name */
    DELIVER_REQUEST *request;		/* envelope info, offsets */
    struct SMTP_SESSION *session;	/* network connection */
    int     status;			/* delivery status */
    ssize_t space_left;			/* output length control */

    /*
     * Connection cache support. The (nexthop_lookup_mx, nexthop_domain,
     * nexthop_port) triple is a parsed next-hop specification, and should be
     * a data type by itself. The (service, nexthop_mumble) members specify
     * the name under which the first good connection should be cached. The
     * nexthop_mumble members are initialized by the connection management
     * module. nexthop_domain is reset to null after one connection is saved
     * under the (service, nexthop_mumble) label, or upon exit from the
     * connection management module.
     */
    HTABLE *cache_used;			/* cached addresses that were used */
    VSTRING *dest_label;		/* cached logical/physical binding */
    VSTRING *dest_prop;			/* binding properties, passivated */
    VSTRING *endp_label;		/* cached session physical endpoint */
    VSTRING *endp_prop;			/* endpoint properties, passivated */
    int     nexthop_lookup_mx;		/* do/don't MX expand nexthop_domain */
    char   *nexthop_domain;		/* next-hop name or bare address */
    unsigned nexthop_port;		/* next-hop TCP port, network order */

    /*
     * Flags and counters to control the handling of mail delivery errors.
     * There is some redundancy for sanity checking. At the end of an SMTP
     * session all recipients should be marked one way or the other.
     */
    int     rcpt_left;			/* recipients left over */
    int     rcpt_drop;			/* recipients marked as drop */
    int     rcpt_keep;			/* recipients marked as keep */

    /*
     * DSN Support introduced major bloat in error processing.
     */
    DSN_BUF *why;			/* on-the-fly formatting buffer */
} SMTP_STATE;

#define SET_NEXTHOP_STATE(state, lookup_mx, domain, port) { \
	(state)->nexthop_lookup_mx = lookup_mx; \
	(state)->nexthop_domain = mystrdup(domain); \
	(state)->nexthop_port = port; \
    }

#define FREE_NEXTHOP_STATE(state) { \
	myfree((state)->nexthop_domain); \
	(state)->nexthop_domain = 0; \
    }

#define HAVE_NEXTHOP_STATE(state) ((state)->nexthop_domain != 0)


 /*
  * Server features.
  */
#define SMTP_FEATURE_ESMTP		(1<<0)
#define SMTP_FEATURE_8BITMIME		(1<<1)
#define SMTP_FEATURE_PIPELINING		(1<<2)
#define SMTP_FEATURE_SIZE		(1<<3)
#define SMTP_FEATURE_STARTTLS		(1<<4)
#define SMTP_FEATURE_AUTH		(1<<5)
#define SMTP_FEATURE_XFORWARD_NAME	(1<<7)
#define SMTP_FEATURE_XFORWARD_ADDR	(1<<8)
#define SMTP_FEATURE_XFORWARD_PROTO	(1<<9)
#define SMTP_FEATURE_XFORWARD_HELO	(1<<10)
#define SMTP_FEATURE_XFORWARD_DOMAIN	(1<<11)
#define SMTP_FEATURE_BEST_MX		(1<<12)	/* for next-hop or fall-back */
#define SMTP_FEATURE_RSET_REJECTED	(1<<13)	/* RSET probe rejected */
#define SMTP_FEATURE_FROM_CACHE		(1<<14)	/* cached connection */
#define SMTP_FEATURE_DSN		(1<<15)	/* DSN supported */
#define SMTP_FEATURE_PIX_NO_ESMTP	(1<<16)	/* PIX smtp fixup mode */
#define SMTP_FEATURE_PIX_DELAY_DOTCRLF	(1<<17)	/* PIX smtp fixup mode */
#define SMTP_FEATURE_XFORWARD_PORT	(1<<18)

 /*
  * Features that passivate under the endpoint.
  */
#define SMTP_FEATURE_ENDPOINT_MASK \
	(~(SMTP_FEATURE_BEST_MX | SMTP_FEATURE_RSET_REJECTED \
	| SMTP_FEATURE_FROM_CACHE))

 /*
  * Features that passivate under the logical destination.
  */
#define SMTP_FEATURE_DESTINATION_MASK (SMTP_FEATURE_BEST_MX)

 /*
  * Misc flags.
  */
#define SMTP_MISC_FLAG_LOOP_DETECT	(1<<0)
#define	SMTP_MISC_FLAG_IN_STARTTLS	(1<<1)
#define SMTP_MISC_FLAG_USE_LMTP		(1<<2)
#define SMTP_MISC_FLAG_FIRST_NEXTHOP	(1<<3)
#define SMTP_MISC_FLAG_FINAL_NEXTHOP	(1<<4)
#define SMTP_MISC_FLAG_FINAL_SERVER	(1<<5)
#define SMTP_MISC_FLAG_CONN_LOAD	(1<<6)
#define SMTP_MISC_FLAG_CONN_STORE	(1<<7)
#define SMTP_MISC_FLAG_COMPLETE_SESSION	(1<<8)

#define SMTP_MISC_FLAG_CONN_CACHE_MASK \
	(SMTP_MISC_FLAG_CONN_LOAD | SMTP_MISC_FLAG_CONN_STORE)

 /*
  * smtp.c
  */
#define SMTP_HAS_DSN(why)	(STR((why)->status)[0] != 0)
#define SMTP_HAS_SOFT_DSN(why)	(STR((why)->status)[0] == '4')
#define SMTP_HAS_HARD_DSN(why)	(STR((why)->status)[0] == '5')
#define SMTP_HAS_LOOP_DSN(why) \
    (SMTP_HAS_DSN(why) && strcmp(STR((why)->status) + 1, ".4.6") == 0)

#define SMTP_SET_SOFT_DSN(why)	(STR((why)->status)[0] = '4')
#define SMTP_SET_HARD_DSN(why)	(STR((why)->status)[0] = '5')

extern int smtp_host_lookup_mask;	/* host lookup methods to use */

#define SMTP_HOST_FLAG_DNS	(1<<0)
#define SMTP_HOST_FLAG_NATIVE	(1<<1)

extern SCACHE *smtp_scache;		/* connection cache instance */
extern STRING_LIST *smtp_cache_dest;	/* cached destinations */

extern MAPS *smtp_ehlo_dis_maps;	/* ehlo keyword filter */

extern MAPS *smtp_pix_bug_maps;		/* PIX workarounds */

extern MAPS *smtp_generic_maps;		/* make internal address valid */
extern int smtp_ext_prop_mask;		/* address externsion propagation */

#ifdef USE_TLS

extern TLS_APPL_STATE *smtp_tls_ctx;	/* client-side TLS engine */

#endif

extern HBC_CHECKS *smtp_header_checks;	/* limited header checks */
extern HBC_CHECKS *smtp_body_checks;	/* limited body checks */

 /*
  * smtp_session.c
  */
typedef struct SMTP_SESSION {
    VSTREAM *stream;			/* network connection */
    char   *dest;			/* nexthop or fallback */
    char   *host;			/* mail exchanger */
    char   *addr;			/* mail exchanger */
    char   *namaddr;			/* mail exchanger */
    char   *helo;			/* helo response */
    unsigned port;			/* network byte order */
    char   *namaddrport;		/* mail exchanger, incl. port */

    VSTRING *buffer;			/* I/O buffer */
    VSTRING *scratch;			/* scratch buffer */
    VSTRING *scratch2;			/* scratch buffer */

    int     features;			/* server features */
    off_t   size_limit;			/* server limit or unknown */

    ARGV   *history;			/* transaction log */
    int     error_mask;			/* error classes */
    struct MIME_STATE *mime_state;	/* mime state machine */

    int     sndbufsize;			/* PIPELINING buffer size */
    int     send_proto_helo;		/* XFORWARD support */

    time_t  expire_time;		/* session reuse expiration time */
    int     reuse_count;		/* # of times reused (for logging) */
    int     dead;			/* No further I/O allowed */

#ifdef USE_SASL_AUTH
    char   *sasl_mechanism_list;	/* server mechanism list */
    char   *sasl_username;		/* client username */
    char   *sasl_passwd;		/* client password */
    struct XSASL_CLIENT *sasl_client;	/* SASL internal state */
    VSTRING *sasl_reply;		/* client response */
#endif

    /*
     * TLS related state, don't forget to initialize in session_tls_init()!
     */
#ifdef USE_TLS
    TLS_SESS_STATE *tls_context;	/* TLS session state */
    char   *tls_nexthop;		/* Nexthop domain for cert checks */
    int     tls_level;			/* TLS enforcement level */
    int     tls_retry_plain;		/* Try plain when TLS handshake fails */
    char   *tls_protocols;		/* Acceptable SSL protocols */
    char   *tls_grade;			/* Cipher grade: "export", ... */
    VSTRING *tls_exclusions;		/* Excluded SSL ciphers */
    ARGV   *tls_matchargv;		/* Cert match patterns */
#endif

    SMTP_STATE *state;			/* back link */
} SMTP_SESSION;

extern SMTP_SESSION *smtp_session_alloc(VSTREAM *, const char *, const char *,
			               const char *, unsigned, time_t, int);
extern void smtp_session_free(SMTP_SESSION *);
extern int smtp_session_passivate(SMTP_SESSION *, VSTRING *, VSTRING *);
extern SMTP_SESSION *smtp_session_activate(int, VSTRING *, VSTRING *);

#ifdef USE_TLS
extern void smtp_tls_list_init(void);

#endif

 /*
  * What's in a name?
  */
#define SMTP_HNAME(rr) (var_smtp_cname_overr ? (rr)->rname : (rr)->qname)

 /*
  * smtp_connect.c
  */
extern int smtp_connect(SMTP_STATE *);

 /*
  * smtp_proto.c
  */
extern int smtp_helo(SMTP_STATE *);
extern int smtp_xfer(SMTP_STATE *);
extern int smtp_rset(SMTP_STATE *);
extern int smtp_quit(SMTP_STATE *);

extern HBC_CALL_BACKS smtp_hbc_callbacks[];

 /*
  * A connection is re-usable if session->expire_time is > 0 and the
  * expiration time has not been reached.  This is subtle because the timer
  * can expire between sending a command and receiving the reply for that
  * command.
  * 
  * But wait, there is more! When SMTP command pipelining is enabled, there are
  * two protocol loops that execute at very different times: one loop that
  * generates commands, and one loop that receives replies to those commands.
  * These will be called "sender loop" and "receiver loop", respectively. At
  * well-defined protocol synchronization points, the sender loop pauses to
  * let the receiver loop catch up.
  * 
  * When we choose to reuse a connection, both the sender and receiver protocol
  * loops end with "." (mail delivery) or "RSET" (address probe). When we
  * choose not to reuse, both the sender and receiver protocol loops end with
  * "QUIT". The problem is that we must make the same protocol choices in
  * both the sender and receiver loops, even though those loops may execute
  * at completely different times.
  * 
  * We "freeze" the choice in the sender loop, just before we generate "." or
  * "RSET". The reader loop leaves the connection cachable even if the timer
  * expires by the time the response arrives. The connection cleanup code
  * will call smtp_quit() for connections with an expired cache expiration
  * timer.
  * 
  * We could have made the programmer's life a lot simpler by not making a
  * choice at all, and always leaving it up to the connection cleanup code to
  * call smtp_quit() for connections with an expired cache expiration timer.
  * 
  * As a general principle, neither the sender loop nor the receiver loop must
  * modify the connection caching state, if that can affect the receiver
  * state machine for not-yet processed replies to already-generated
  * commands. This restriction does not apply when we have to exit the
  * protocol loops prematurely due to e.g., timeout or connection loss, so
  * that those pending replies will never be received.
  * 
  * But wait, there is even more! Only the first good connection for a specific
  * destination may be cached under both the next-hop destination name and
  * the server address; connections to alternate servers must be cached under
  * the server address alone. This means we must distinguish between bad
  * connections and other reasons why connections cannot be cached.
  */
#define THIS_SESSION_IS_CACHED \
	(!THIS_SESSION_IS_DEAD && session->expire_time > 0)

#define THIS_SESSION_IS_EXPIRED \
	(THIS_SESSION_IS_CACHED \
	    && session->expire_time < vstream_ftime(session->stream))

#define THIS_SESSION_IS_BAD \
	(!THIS_SESSION_IS_DEAD && session->expire_time < 0)

#define THIS_SESSION_IS_DEAD \
	(session->dead != 0)

 /* Bring the bad news. */

#define DONT_CACHE_THIS_SESSION \
	(session->expire_time = 0)

#define DONT_CACHE_BAD_SESSION \
	(session->expire_time = -1)

#define DONT_USE_DEAD_SESSION \
	(session->dead = 1)

 /* Initialization. */

#define USE_NEWBORN_SESSION \
	(session->dead = 0)

#define CACHE_THIS_SESSION_UNTIL(when) \
	(session->expire_time = (when))

 /*
  * Encapsulate the following so that we don't expose details of of
  * connection management and error handling to the SMTP protocol engine.
  */
#define RETRY_AS_PLAINTEXT do { \
	session->tls_retry_plain = 1; \
	state->misc_flags &= ~SMTP_MISC_FLAG_FINAL_SERVER; \
    } while (0)

 /*
  * smtp_chat.c
  */
typedef struct SMTP_RESP {		/* server response */
    int     code;			/* SMTP code */
    const char *dsn;			/* enhanced status */
    char   *str;			/* full reply */
    VSTRING *dsn_buf;			/* status buffer */
    VSTRING *str_buf;			/* reply buffer */
} SMTP_RESP;

extern void PRINTFLIKE(2, 3) smtp_chat_cmd(SMTP_SESSION *, char *,...);
extern SMTP_RESP *smtp_chat_resp(SMTP_SESSION *);
extern void smtp_chat_init(SMTP_SESSION *);
extern void smtp_chat_reset(SMTP_SESSION *);
extern void smtp_chat_notify(SMTP_SESSION *);

#define SMTP_RESP_FAKE(resp, _dsn) \
    ((resp)->code = 0, \
     (resp)->dsn = (_dsn), \
     (resp)->str = DSN_BY_LOCAL_MTA, \
     (resp))

#define DSN_BY_LOCAL_MTA	((char *) 0)	/* DSN issued by local MTA */

 /*
  * These operations implement a redundant mark-and-sweep algorithm that
  * explicitly accounts for the fate of every recipient. The interface is
  * documented in smtp_rcpt.c, which also implements the sweeping. The
  * smtp_trouble.c module does most of the marking after failure.
  * 
  * When a delivery fails or succeeds, take one of the following actions:
  * 
  * - Mark the recipient as KEEP (deliver to alternate MTA) and do not update
  * the delivery request status.
  * 
  * - Mark the recipient as DROP (remove from delivery request), log whether
  * delivery succeeded or failed, delete the recipient from the queue file
  * and/or update defer or bounce logfiles, and update the delivery request
  * status.
  * 
  * At the end of a delivery attempt, all recipients must be marked one way or
  * the other. Failure to do so will trigger a panic.
  */
#define SMTP_RCPT_STATE_KEEP	1	/* send to backup host */
#define SMTP_RCPT_STATE_DROP	2	/* remove from request */
#define SMTP_RCPT_INIT(state) do { \
	    (state)->rcpt_drop = (state)->rcpt_keep = 0; \
	    (state)->rcpt_left = state->request->rcpt_list.len; \
	} while (0)

#define SMTP_RCPT_DROP(state, rcpt) do { \
	    (rcpt)->u.status = SMTP_RCPT_STATE_DROP; (state)->rcpt_drop++; \
	} while (0)

#define SMTP_RCPT_KEEP(state, rcpt) do { \
	    (rcpt)->u.status = SMTP_RCPT_STATE_KEEP; (state)->rcpt_keep++; \
	} while (0)

#define SMTP_RCPT_ISMARKED(rcpt) ((rcpt)->u.status != 0)

#define SMTP_RCPT_LEFT(state) (state)->rcpt_left

extern void smtp_rcpt_cleanup(SMTP_STATE *);
extern void smtp_rcpt_done(SMTP_STATE *, SMTP_RESP *, RECIPIENT *);

 /*
  * smtp_trouble.c
  */
extern int smtp_sess_fail(SMTP_STATE *);
extern int PRINTFLIKE(4, 5) smtp_site_fail(SMTP_STATE *, const char *,
				             SMTP_RESP *, const char *,...);
extern int PRINTFLIKE(4, 5) smtp_mesg_fail(SMTP_STATE *, const char *,
				             SMTP_RESP *, const char *,...);
extern void PRINTFLIKE(5, 6) smtp_rcpt_fail(SMTP_STATE *, RECIPIENT *,
					          const char *, SMTP_RESP *,
					            const char *,...);
extern int smtp_stream_except(SMTP_STATE *, int, const char *);

 /*
  * smtp_unalias.c
  */
extern const char *smtp_unalias_name(const char *);
extern VSTRING *smtp_unalias_addr(VSTRING *, const char *);

 /*
  * smtp_state.c
  */
extern SMTP_STATE *smtp_state_alloc(void);
extern void smtp_state_free(SMTP_STATE *);

 /*
  * smtp_map11.c
  */
extern int smtp_map11_external(VSTRING *, MAPS *, int);
extern int smtp_map11_tree(TOK822 *, MAPS *, int);
extern int smtp_map11_internal(VSTRING *, MAPS *, int);

 /*
  * Silly little macros.
  */
#define STR(s) vstring_str(s)
#define LEN(s) VSTRING_LEN(s)

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	TLS support originally by:
/*	Lutz Jaenicke
/*	BTU Cottbus
/*	Allgemeine Elektrotechnik
/*	Universitaetsplatz 3-4
/*	D-03044 Cottbus, Germany
/*--*/
