/*	$NetBSD: smtp.h,v 1.1.1.6 2005/08/18 21:08:50 rpaulo Exp $	*/

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
  * SASL library.
  */
#ifdef USE_SASL_AUTH
#include <sasl.h>
#include <saslutil.h>
#endif

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

 /*
  * Postfix TLS library.
  */
#ifdef USE_TLS
#include <tls.h>
#endif

 /*
  * State information associated with each SMTP delivery request.
  * Session-specific state is stored separately.
  */
typedef struct SMTP_STATE {
    VSTREAM *src;			/* queue file stream */
    const char *service;		/* transport name */
    DELIVER_REQUEST *request;		/* envelope info, offsets */
    struct SMTP_SESSION *session;	/* network connection */
    int     status;			/* delivery status */
    int     space_left;			/* output length control */

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
    int     final_server;		/* final mail server */
    int     rcpt_left;			/* recipients left over */
    int     rcpt_drop;			/* recipients marked as drop */
    int     rcpt_keep;			/* recipients marked as keep */
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
#define SMTP_FEATURE_MAYBEPIX		(1<<6)	/* PIX smtp fixup mode */
#define SMTP_FEATURE_XFORWARD_NAME	(1<<7)
#define SMTP_FEATURE_XFORWARD_ADDR	(1<<8)
#define SMTP_FEATURE_XFORWARD_PROTO	(1<<9)
#define SMTP_FEATURE_XFORWARD_HELO	(1<<10)
#define SMTP_FEATURE_XFORWARD_DOMAIN	(1<<11)
#define SMTP_FEATURE_BEST_MX		(1<<12)	/* for next-hop or fall-back */
#define SMTP_FEATURE_RSET_REJECTED	(1<<13)	/* RSET probe rejected */
#define SMTP_FEATURE_FROM_CACHE		(1<<14)	/* cached connection */

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

#define SMTP_MISC_FLAG_DEFAULT		SMTP_MISC_FLAG_LOOP_DETECT

 /*
  * smtp.c
  */
extern int smtp_errno;			/* XXX can we get rid of this? */

#define SMTP_ERR_NONE	0		/* no error */
#define SMTP_ERR_FAIL	1		/* permanent error */
#define SMTP_ERR_RETRY	2		/* temporary error */
#define SMTP_ERR_LOOP	3		/* mailer loop */

extern int smtp_host_lookup_mask;	/* host lookup methods to use */

#define SMTP_HOST_FLAG_DNS	(1<<0)
#define SMTP_HOST_FLAG_NATIVE	(1<<1)

extern SCACHE *smtp_scache;		/* connection cache instance */
extern STRING_LIST *smtp_cache_dest;	/* cached destinations */

extern MAPS *smtp_ehlo_dis_maps;	/* ehlo keyword filter */

extern MAPS *smtp_generic_maps;		/* make internal address valid */
extern int smtp_ext_prop_mask;		/* address externsion propagation */

#ifdef USE_TLS

extern SSL_CTX *smtp_tls_ctx;		/* client-side TLS engine */

#endif

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

    int     reuse_count;		/* how many uses left */

#ifdef USE_SASL_AUTH
    char   *sasl_mechanism_list;	/* server mechanism list */
    char   *sasl_username;		/* client username */
    char   *sasl_passwd;		/* client password */
    sasl_conn_t *sasl_conn;		/* SASL internal state */
    VSTRING *sasl_encoded;		/* encoding buffer */
    VSTRING *sasl_decoded;		/* decoding buffer */
    sasl_callback_t *sasl_callbacks;	/* stateful callbacks */
#endif

    /*
     * TLS related state.
     */
#ifdef USE_TLS
    int     tls_use_tls;		/* can do TLS */
    int     tls_enforce_tls;		/* must do TLS */
    int     tls_enforce_peername;	/* cert must match */
    TLScontext_t *tls_context;		/* TLS session state */
    tls_info_t tls_info;		/* legacy */
#endif

} SMTP_SESSION;

extern SMTP_SESSION *smtp_session_alloc(VSTREAM *, const char *,
			         const char *, const char *, unsigned, int);
extern void smtp_session_free(SMTP_SESSION *);
extern int smtp_session_passivate(SMTP_SESSION *, VSTRING *, VSTRING *);
extern SMTP_SESSION *smtp_session_activate(int, VSTRING *, VSTRING *);

#define SMTP_SESS_FLAG_NONE	0	/* no options */
#define SMTP_SESS_FLAG_CACHE	(1<<0)	/* enable session caching */

#ifdef USE_TLS
extern void smtp_tls_list_init(void);

#endif

 /*
  * smtp_connect.c
  */
extern int smtp_connect(SMTP_STATE *);

 /*
  * smtp_proto.c
  */
extern int smtp_helo(SMTP_STATE *, int);
extern int smtp_xfer(SMTP_STATE *);
extern int smtp_rset(SMTP_STATE *);
extern int smtp_quit(SMTP_STATE *);

 /*
  * smtp_chat.c
  */
typedef struct SMTP_RESP {		/* server response */
    int     code;			/* status */
    char   *str;			/* text */
    VSTRING *buf;			/* origin of text */
} SMTP_RESP;

extern void PRINTFLIKE(2, 3) smtp_chat_cmd(SMTP_SESSION *, char *,...);
extern SMTP_RESP *smtp_chat_resp(SMTP_SESSION *);
extern void smtp_chat_init(SMTP_SESSION *);
extern void smtp_chat_reset(SMTP_SESSION *);
extern void smtp_chat_notify(SMTP_SESSION *);

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
	    (rcpt)->status = SMTP_RCPT_STATE_DROP; (state)->rcpt_drop++; \
	} while (0)

#define SMTP_RCPT_KEEP(state, rcpt) do { \
	    (rcpt)->status = SMTP_RCPT_STATE_KEEP; (state)->rcpt_keep++; \
	} while (0)

#define SMTP_RCPT_ISMARKED(rcpt) ((rcpt)->status != 0)

#define SMTP_RCPT_LEFT(state) (state)->rcpt_left

extern void smtp_rcpt_cleanup(SMTP_STATE *);
extern void smtp_rcpt_done(SMTP_STATE *, const char *, RECIPIENT *);

 /*
  * smtp_trouble.c
  */
extern int PRINTFLIKE(3, 4) smtp_conn_fail(SMTP_STATE *, int, char *,...);
extern int PRINTFLIKE(3, 4) smtp_site_fail(SMTP_STATE *, int, char *,...);
extern int PRINTFLIKE(3, 4) smtp_mesg_fail(SMTP_STATE *, int, char *,...);
extern void PRINTFLIKE(4, 5) smtp_rcpt_fail(SMTP_STATE *, int, RECIPIENT *, char *,...);
extern int smtp_stream_except(SMTP_STATE *, int, char *);

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
