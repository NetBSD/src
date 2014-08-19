/*	$NetBSD: smtp.h,v 1.1.1.3.10.1 2014/08/19 23:59:44 tls Exp $	*/

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
#include <dict.h>

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
  * Global iterator support. This is updated by the connection-management
  * loop, and contains dynamic context that appears in lookup keys for SASL
  * passwords, TLS policy, cached SMTP connections, and cached TLS session
  * keys.
  * 
  * For consistency and maintainability, context that is used in more than one
  * lookup key is formatted with smtp_key_format().
  */
typedef struct SMTP_ITERATOR {
    /* Public members. */
    VSTRING *request_nexthop;		/* request nexhop or empty */
    VSTRING *dest;			/* current nexthop */
    VSTRING *host;			/* hostname or empty */
    VSTRING *addr;			/* printable address or empty */
    unsigned port;			/* network byte order or null */
    struct DNS_RR *rr;			/* DNS resource record or null */
    struct DNS_RR *mx;			/* DNS resource record or null */
    /* Private members. */
    VSTRING *saved_dest;		/* saved current nexthop */
    struct SMTP_STATE *parent;		/* parent linkage */
} SMTP_ITERATOR;

#define SMTP_ITER_INIT(iter, _dest, _host, _addr, _port, state) do { \
	vstring_strcpy((iter)->dest, (_dest)); \
	vstring_strcpy((iter)->host, (_host)); \
	vstring_strcpy((iter)->addr, (_addr)); \
	(iter)->port = (_port); \
	(iter)->mx = (iter)->rr = 0; \
	vstring_strcpy((iter)->saved_dest, ""); \
	(iter)->parent = (state); \
    } while (0)

#define SMTP_ITER_CLOBBER(iter, _dest, _host, _addr) do { \
	vstring_strcpy((iter)->dest, (_dest)); \
	vstring_strcpy((iter)->host, (_host)); \
	vstring_strcpy((iter)->addr, (_addr)); \
    } while (0)

#define SMTP_ITER_SAVE_DEST(iter) do { \
	vstring_strcpy((iter)->saved_dest, STR((iter)->dest)); \
    } while (0)

#define SMTP_ITER_RESTORE_DEST(iter) do { \
	vstring_strcpy((iter)->dest, STR((iter)->saved_dest)); \
    } while (0)

 /*
  * TLS Policy support.
  */
#ifdef USE_TLS

typedef struct SMTP_TLS_POLICY {
    int     level;			/* TLS enforcement level */
    char   *protocols;			/* Acceptable SSL protocols */
    char   *grade;			/* Cipher grade: "export", ... */
    VSTRING *exclusions;		/* Excluded SSL ciphers */
    ARGV   *matchargv;			/* Cert match patterns */
    DSN_BUF *why;			/* Lookup error status */
    TLS_DANE *dane;			/* DANE TLSA digests */
} SMTP_TLS_POLICY;

 /*
  * smtp_tls_policy.c
  */
extern void smtp_tls_list_init(void);
extern int smtp_tls_policy_cache_query(DSN_BUF *, SMTP_TLS_POLICY *, SMTP_ITERATOR *);
extern void smtp_tls_policy_cache_flush(void);

 /*
  * Macros must use distinct names for local temporary variables, otherwise
  * there will be bugs due to shadowing. This happened when an earlier
  * version of smtp_tls_policy_dummy() invoked smtp_tls_policy_init(), but it
  * could also happen without macro nesting.
  * 
  * General principle: use all or part of the macro name in each temporary
  * variable name. Then, append suffixes to the names if needed.
  */
#define smtp_tls_policy_dummy(t) do { \
	SMTP_TLS_POLICY *_tls_policy_dummy_tmp = (t); \
	smtp_tls_policy_init(_tls_policy_dummy_tmp, (DSN_BUF *) 0); \
	_tls_policy_dummy_tmp->level = TLS_LEV_NONE; \
    } while (0)

 /* This macro is not part of the module external interface. */
#define smtp_tls_policy_init(t, w) do { \
	SMTP_TLS_POLICY *_tls_policy_init_tmp = (t); \
	_tls_policy_init_tmp->protocols = 0; \
	_tls_policy_init_tmp->grade = 0; \
	_tls_policy_init_tmp->exclusions = 0; \
	_tls_policy_init_tmp->matchargv = 0; \
	_tls_policy_init_tmp->why = (w); \
	_tls_policy_init_tmp->dane = 0; \
    } while (0)

#endif

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
     * Global iterator.
     */
    SMTP_ITERATOR iterator[1];		/* Usage: state->iterator->member */

    /*
     * Global iterator.
     */
#ifdef USE_TLS
    SMTP_TLS_POLICY tls[1];		/* Usage: state->tls->member */
#endif

    /*
     * Connection cache support.
     */
    HTABLE *cache_used;			/* cached addresses that were used */
    VSTRING *dest_label;		/* cached logical/physical binding */
    VSTRING *dest_prop;			/* binding properties, passivated */
    VSTRING *endp_label;		/* cached session physical endpoint */
    VSTRING *endp_prop;			/* endpoint properties, passivated */

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

 /*
  * TODO: use the new SMTP_ITER name space.
  */
#define SET_NEXTHOP_STATE(state, nexthop) { \
	vstring_strcpy((state)->iterator->request_nexthop, nexthop); \
    }

#define FREE_NEXTHOP_STATE(state) { \
	STR((state)->iterator->request_nexthop)[0] = 0; \
    }

#define HAVE_NEXTHOP_STATE(state) (STR((state)->iterator->request_nexthop)[0] != 0)


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
#define SMTP_FEATURE_EARLY_TLS_MAIL_REPLY (1<<19)	/* CVE-2009-3555 */
#define SMTP_FEATURE_XFORWARD_IDENT	(1<<20)

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
#define SMTP_MISC_FLAG_FIRST_NEXTHOP	(1<<2)
#define SMTP_MISC_FLAG_FINAL_NEXTHOP	(1<<3)
#define SMTP_MISC_FLAG_FINAL_SERVER	(1<<4)
#define SMTP_MISC_FLAG_CONN_LOAD	(1<<5)
#define SMTP_MISC_FLAG_CONN_STORE	(1<<6)
#define SMTP_MISC_FLAG_COMPLETE_SESSION	(1<<7)
#define SMTP_MISC_FLAG_PREF_IPV6	(1<<8)
#define SMTP_MISC_FLAG_PREF_IPV4	(1<<9)

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

extern int smtp_dns_support;		/* dns support level */

#define SMTP_DNS_INVALID	(-1)	/* smtp_dns_support_level = <bogus> */
#define SMTP_DNS_DISABLED	0	/* smtp_dns_support_level = disabled */
#define SMTP_DNS_ENABLED	1	/* smtp_dns_support_level = enabled */
#define SMTP_DNS_DNSSEC		2	/* smtp_dns_support_level = dnssec */

extern SCACHE *smtp_scache;		/* connection cache instance */
extern STRING_LIST *smtp_cache_dest;	/* cached destinations */

extern MAPS *smtp_ehlo_dis_maps;	/* ehlo keyword filter */

extern MAPS *smtp_pix_bug_maps;		/* PIX workarounds */

extern MAPS *smtp_generic_maps;		/* make internal address valid */
extern int smtp_ext_prop_mask;		/* address externsion propagation */
extern unsigned smtp_dns_res_opt;	/* DNS query flags */

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
    SMTP_ITERATOR *iterator;		/* dest, host, addr, port */
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
    TLS_SESS_STATE *tls_context;	/* TLS library session state */
    char   *tls_nexthop;		/* Nexthop domain for cert checks */
    int     tls_retry_plain;		/* Try plain when TLS handshake fails */
    SMTP_TLS_POLICY *tls;		/* TEMPORARY */
#endif

    SMTP_STATE *state;			/* back link */
} SMTP_SESSION;

extern SMTP_SESSION *smtp_session_alloc(VSTREAM *, SMTP_ITERATOR *, time_t, int);
extern void smtp_session_new_stream(SMTP_SESSION *, VSTREAM *, time_t, int);
extern int smtp_sess_plaintext_ok(SMTP_ITERATOR *, int);
extern void smtp_session_free(SMTP_SESSION *);
extern int smtp_session_passivate(SMTP_SESSION *, VSTRING *, VSTRING *);
extern SMTP_SESSION *smtp_session_activate(int, SMTP_ITERATOR *, VSTRING *, VSTRING *);

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
	    && (session->expire_time < vstream_ftime(session->stream) \
		|| (var_smtp_reuse_count > 0 \
		    && session->reuse_count >= var_smtp_reuse_count)))

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

extern void PRINTFLIKE(2, 3) smtp_chat_cmd(SMTP_SESSION *, const char *,...);
extern DICT *smtp_chat_resp_filter;
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
  * smtp_key.c
  */
char   *smtp_key_prefix(VSTRING *, const char *, SMTP_ITERATOR *, int);

#define SMTP_KEY_FLAG_SERVICE		(1<<0)	/* service name */
#define SMTP_KEY_FLAG_SENDER		(1<<1)	/* sender address */
#define SMTP_KEY_FLAG_REQ_NEXTHOP	(1<<2)	/* request nexthop */
#define SMTP_KEY_FLAG_NEXTHOP		(1<<3)	/* current nexthop */
#define SMTP_KEY_FLAG_HOSTNAME		(1<<4)	/* remote host name */
#define SMTP_KEY_FLAG_ADDR		(1<<5)	/* remote address */
#define SMTP_KEY_FLAG_PORT		(1<<6)	/* remote port */

#define SMTP_KEY_MASK_ALL \
	(SMTP_KEY_FLAG_SERVICE | SMTP_KEY_FLAG_SENDER | \
	SMTP_KEY_FLAG_REQ_NEXTHOP | \
	SMTP_KEY_FLAG_NEXTHOP | SMTP_KEY_FLAG_HOSTNAME | \
	SMTP_KEY_FLAG_ADDR | SMTP_KEY_FLAG_PORT)

 /*
  * Conditional lookup-key flags for cached connections that may be
  * SASL-authenticated with a per-{sender, nexthop, or hostname} credential.
  * Each bit corresponds to one type of smtp_sasl_password_file lookup key,
  * and is turned on only when the corresponding main.cf parameter is turned
  * on.
  */
#define COND_SASL_SMTP_KEY_FLAG_SENDER \
	((var_smtp_sender_auth && *var_smtp_sasl_passwd) ? \
	    SMTP_KEY_FLAG_SENDER : 0)

#define COND_SASL_SMTP_KEY_FLAG_NEXTHOP \
	(*var_smtp_sasl_passwd ? SMTP_KEY_FLAG_NEXTHOP : 0)

#define COND_SASL_SMTP_KEY_FLAG_HOSTNAME \
	(*var_smtp_sasl_passwd ? SMTP_KEY_FLAG_HOSTNAME : 0)

 /*
  * Connection-cache destination lookup key. The SENDER attribute is a proxy
  * for sender-dependent SASL credentials (or absence thereof), and prevents
  * false connection sharing when different SASL credentials may be required
  * for different deliveries to the same domain and port. The SERVICE
  * attribute is a proxy for all request-independent configuration details.
  */
#define SMTP_KEY_MASK_SCACHE_DEST_LABEL \
	(SMTP_KEY_FLAG_SERVICE | COND_SASL_SMTP_KEY_FLAG_SENDER \
	| SMTP_KEY_FLAG_REQ_NEXTHOP)

 /*
  * Connection-cache endpoint lookup key. The SENDER, NEXTHOP, and HOSTNAME
  * attributes are proxies for SASL credentials (or absence thereof), and
  * prevent false connection sharing when different SASL credentials may be
  * required for different deliveries to the same IP address and port.
  */
#define SMTP_KEY_MASK_SCACHE_ENDP_LABEL \
	(SMTP_KEY_FLAG_SERVICE | COND_SASL_SMTP_KEY_FLAG_SENDER \
	| COND_SASL_SMTP_KEY_FLAG_NEXTHOP | COND_SASL_SMTP_KEY_FLAG_HOSTNAME \
	| SMTP_KEY_FLAG_ADDR | SMTP_KEY_FLAG_PORT)

 /*
  * Silly little macros.
  */
#define STR(s) vstring_str(s)
#define LEN(s) VSTRING_LEN(s)

extern int smtp_mode;

#define SMTP_X(x) (smtp_mode ? VAR_SMTP_##x : VAR_LMTP_##x)
#define X_SMTP(x) (smtp_mode ? x##_SMTP : x##_LMTP)

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
/*
/*	Victor Duchovni
/*	Morgan Stanley
/*--*/
