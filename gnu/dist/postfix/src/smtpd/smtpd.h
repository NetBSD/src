/*++
/* NAME
/*	smtpd 3h
/* SUMMARY
/*	smtp server
/* SYNOPSIS
/*	include "smtpd.h"
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <unistd.h>

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

 /*
  * Global library.
  */
#include <mail_stream.h>

 /*
  * Variables that keep track of conversation state. There is only one SMTP
  * conversation at a time, so the state variables can be made global. And
  * some of this has to be global anyway, so that the run-time error handler
  * can clean up in case of a fatal error deep down in some library routine.
  */
typedef struct SMTPD_DEFER {
    int     active;			/* is this active */
    VSTRING *reason;			/* reason for deferral */
    int     class;			/* error notification class */
} SMTPD_DEFER;

typedef struct {
    int     flags;			/* XFORWARD server state */
    char   *name;			/* name for access control */
    char   *addr;			/* address for access control */
    char   *namaddr;			/* name[address] */
    char   *protocol;			/* email protocol */
    char   *helo_name;			/* helo/ehlo parameter */
    char   *ident;			/* message identifier */
} SMTPD_XFORWARD_ATTR;

typedef struct SMTPD_STATE {
    int     err;			/* cleanup server/queue file errors */
    VSTREAM *client;			/* SMTP client handle */
    VSTRING *buffer;			/* SMTP client buffer */
    time_t  time;			/* start of MAIL FROM transaction */
    char   *name;			/* client hostname */
    char   *addr;			/* client host address string */
    char   *namaddr;			/* combined name and address */
    int     peer_code;			/* 2=ok, 4=soft, 5=hard */
    int     error_count;		/* reset after DOT */
    int     error_mask;			/* client errors */
    int     notify_mask;		/* what to report to postmaster */
    char   *helo_name;			/* client HELO/EHLO argument */
    char   *queue_id;			/* from cleanup server/queue file */
    VSTREAM *cleanup;			/* cleanup server/queue file handle */
    MAIL_STREAM *dest;			/* another server/file handle */
    int     rcpt_count;			/* number of accepted recipients */
    char   *access_denied;		/* fixme */
    ARGV   *history;			/* protocol transcript */
    char   *reason;			/* cause of connection loss */
    char   *sender;			/* sender address */
    char   *encoding;			/* owned by mail_cmd() */
    char   *verp_delims;		/* owned by mail_cmd() */
    char   *recipient;			/* recipient address */
    char   *etrn_name;			/* client ETRN argument */
    char   *protocol;			/* SMTP or ESMTP */
    char   *where;			/* protocol stage */
    int     recursion;			/* Kellerspeicherpegelanzeiger */
    off_t   msg_size;			/* MAIL FROM message size */
    int     junk_cmds;			/* counter */
    int     rcpt_overshoot;		/* counter */

    /*
     * SASL specific.
     */
#ifdef USE_SASL_AUTH
#if SASL_VERSION_MAJOR >= 2
    const char *sasl_mechanism_list;
#else
    char   *sasl_mechanism_list;
#endif
    char   *sasl_method;
    char   *sasl_username;
    char   *sasl_sender;
    sasl_conn_t *sasl_conn;
    VSTRING *sasl_encoded;
    VSTRING *sasl_decoded;
#endif

    /*
     * Specific to smtpd access checks.
     */
    int     sender_rcptmap_checked;	/* sender validated against maps */
    int     recipient_rcptmap_checked;	/* recipient validated against maps */
    int     warn_if_reject;		/* force reject into warning */
    SMTPD_DEFER defer_if_reject;	/* force reject into deferral */
    SMTPD_DEFER defer_if_permit;	/* force permit into deferral */
    int     defer_if_permit_client;	/* force permit into warning */
    int     defer_if_permit_helo;	/* force permit into warning */
    int     defer_if_permit_sender;	/* force permit into warning */
    int     discard;			/* discard message */
    char   *saved_filter;		/* postponed filter action */
    char   *saved_redirect;		/* postponed redirect action */
    int     saved_flags;		/* postponed hold/discard */
    VSTRING *expand_buf;		/* scratch space for $name expansion */
    ARGV   *prepend;			/* prepended headers */
    VSTRING *instance;			/* policy query correlation */
    int     seqno;			/* policy query correlation */

    /*
     * Pass-through proxy client.
     */
    VSTREAM *proxy;			/* proxy handle */
    VSTRING *proxy_buffer;		/* proxy query/reply buffer */
    char   *proxy_mail;			/* owned by mail_cmd() */
    int     proxy_xforward_features;	/* XFORWARD proxy state */

    /*
     * XFORWARD server state.
     */
    SMTPD_XFORWARD_ATTR xforward;	/* up-stream logging info */
} SMTPD_STATE;

#define SMTPD_STATE_XFORWARD_INIT  (1<<0)	/* xforward preset done */
#define SMTPD_STATE_XFORWARD_NAME  (1<<1)	/* client name received */
#define SMTPD_STATE_XFORWARD_ADDR  (1<<2)	/* client address received */
#define SMTPD_STATE_XFORWARD_PROTO (1<<3)	/* protocol received */
#define SMTPD_STATE_XFORWARD_HELO  (1<<4)	/* client helo received */
#define SMTPD_STATE_XFORWARD_IDENT (1<<5)	/* message identifier */

#define SMTPD_STATE_XFORWARD_CLIENT_MASK \
	(SMTPD_STATE_XFORWARD_NAME | SMTPD_STATE_XFORWARD_ADDR \
	| SMTPD_STATE_XFORWARD_PROTO | SMTPD_STATE_XFORWARD_HELO)

extern void smtpd_state_init(SMTPD_STATE *, VSTREAM *);
extern void smtpd_state_reset(SMTPD_STATE *);

 /*
  * Conversation stages.  This is used for "lost connection after XXX"
  * diagnostics.
  */
#define SMTPD_AFTER_CONNECT	"CONNECT"
#define SMTPD_AFTER_DOT		"END-OF-MESSAGE"

 /*
  * Representation of unknown client information within smtpd processes. This
  * is not the representation that Postfix uses in queue files, in queue
  * manager delivery requests, or in XCLIENT/XFORWARD commands!
  */
#define CLIENT_ATTR_UNKNOWN	"unknown"

#define CLIENT_NAME_UNKNOWN	CLIENT_ATTR_UNKNOWN
#define CLIENT_ADDR_UNKNOWN	CLIENT_ATTR_UNKNOWN
#define CLIENT_NAMADDR_UNKNOWN	CLIENT_ATTR_UNKNOWN
#define CLIENT_HELO_UNKNOWN	0
#define CLIENT_PROTO_UNKNOWN	CLIENT_ATTR_UNKNOWN
#define CLIENT_IDENT_UNKNOWN	0

#define IS_AVAIL_CLIENT_ATTR(v)	((v) && strcmp((v), CLIENT_ATTR_UNKNOWN))

#define IS_AVAIL_CLIENT_NAME(v)	IS_AVAIL_CLIENT_ATTR(v)
#define IS_AVAIL_CLIENT_ADDR(v)	IS_AVAIL_CLIENT_ATTR(v)
#define IS_AVAIL_CLIENT_NAMADDR(v) IS_AVAIL_CLIENT_ATTR(v)
#define IS_AVAIL_CLIENT_HELO(v)	((v) != 0)
#define IS_AVAIL_CLIENT_PROTO(v) IS_AVAIL_CLIENT_ATTR(v)
#define IS_AVAIL_CLIENT_IDENT(v) ((v) != 0)

 /*
  * If running in stand-alone mode, do not try to talk to Postfix daemons but
  * write to queue file instead.
  */
#define SMTPD_STAND_ALONE(state) \
	(state->client == VSTREAM_IN && getuid() != var_owner_uid)

 /*
  * If running as proxy front-end, disable actions that require communication
  * with the cleanup server.
  */
#define USE_SMTPD_PROXY(state) \
	(SMTPD_STAND_ALONE(state) == 0 && *var_smtpd_proxy_filt)

 /*
  * SMTPD peer information lookup.
  */
extern void smtpd_peer_init(SMTPD_STATE *state);
extern void smtpd_peer_reset(SMTPD_STATE *state);

#define	SMTPD_PEER_CODE_OK	2
#define SMTPD_PEER_CODE_TEMP	4
#define SMTPD_PEER_CODE_PERM	5

 /*
  * Choose between normal or forwarded attributes.
  * 
  * Note 1: inside the SMTP server, forwarded attributes must have the exact
  * same representation as normal attributes: unknown string values are
  * "unknown", except for HELO which defaults to null. This is better than
  * having to change every piece of code that accesses a possibly forwarded
  * attribute.
  * 
  * Note 2: outside the SMTP server, the representation of unknown/known
  * attribute values is different in queue files, in queue manager delivery
  * requests, and in over-the-network XFORWARD commands.
  * 
  * Note 3: if forwarding client information, don't mix information from the
  * current SMTP session with forwarded information from an up-stream
  * session.
  */
#define FORWARD_CLIENT_ATTR(s, a) \
	(((s)->xforward.flags & SMTPD_STATE_XFORWARD_CLIENT_MASK) ? \
	    (s)->xforward.a : (s)->a)

#define FORWARD_IDENT_ATTR(s) \
	(((s)->xforward.flags & SMTPD_STATE_XFORWARD_IDENT) ? \
	    (s)->queue_id : (s)->ident)

#define FORWARD_ADDR(s)		FORWARD_CLIENT_ATTR((s), addr)
#define FORWARD_NAME(s)		FORWARD_CLIENT_ATTR((s), name)
#define FORWARD_NAMADDR(s)	FORWARD_CLIENT_ATTR((s), namaddr)
#define FORWARD_PROTO(s)	FORWARD_CLIENT_ATTR((s), protocol)
#define FORWARD_HELO(s)		FORWARD_CLIENT_ATTR((s), helo_name)
#define FORWARD_IDENT(s)	FORWARD_IDENT_ATTR(s)

extern void smtpd_xforward_init(SMTPD_STATE *);
extern void smtpd_xforward_preset(SMTPD_STATE *);
extern void smtpd_xforward_reset(SMTPD_STATE *);

 /*
  * Transparency: before mail is queued, do we check for unknown recipients,
  * do we allow address mapping, automatic bcc, header/body checks?
  */
extern int smtpd_input_transp_mask;

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/
