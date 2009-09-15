/*	$NetBSD: smtpd.h,v 1.1.1.1.2.2 2009/09/15 06:03:33 snj Exp $	*/

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
#include <sys/time.h>
#include <unistd.h>

 /*
  * Utility library.
  */
#include <vstream.h>
#include <vstring.h>
#include <argv.h>
#include <myaddrinfo.h>

 /*
  * Global library.
  */
#include <mail_stream.h>

 /*
  * Postfix TLS library.
  */
#include <tls.h>

 /*
  * Milter library.
  */
#include <milter.h>

 /*
  * Variables that keep track of conversation state. There is only one SMTP
  * conversation at a time, so the state variables can be made global. And
  * some of this has to be global anyway, so that the run-time error handler
  * can clean up in case of a fatal error deep down in some library routine.
  */
typedef struct SMTPD_DEFER {
    int     active;			/* is this active */
    VSTRING *reason;			/* reason for deferral */
    VSTRING *dsn;			/* DSN detail */
    int     code;			/* SMTP reply code */
    int     class;			/* error notification class */
} SMTPD_DEFER;

typedef struct {
    int     flags;			/* XFORWARD server state */
    char   *name;			/* name for access control */
    char   *addr;			/* address for access control */
    char   *port;			/* port for logging */
    char   *namaddr;			/* name[address]:port */
    char   *rfc_addr;			/* address for RFC 2821 */
    char   *protocol;			/* email protocol */
    char   *helo_name;			/* helo/ehlo parameter */
    char   *ident;			/* message identifier */
    char   *domain;			/* rewrite context */
} SMTPD_XFORWARD_ATTR;

typedef struct SMTPD_STATE {
    int     flags;			/* see below */
    int     err;			/* cleanup server/queue file errors */
    VSTREAM *client;			/* SMTP client handle */
    VSTRING *buffer;			/* SMTP client buffer */
    VSTRING *addr_buf;			/* internalized address buffer */
    char   *service;			/* for event rate control */
    struct timeval arrival_time;	/* start of MAIL FROM transaction */
    char   *name;			/* verified client hostname */
    char   *reverse_name;		/* unverified client hostname */
    char   *addr;			/* client host address string */
    char   *port;			/* port for logging */
    char   *namaddr;			/* name[address]:port */
    char   *rfc_addr;			/* address for RFC 2821 */
    int     addr_family;		/* address family */
    struct sockaddr_storage sockaddr;	/* binary client endpoint */
    int     name_status;		/* 2=ok 4=soft 5=hard 6=forged */
    int     reverse_name_status;	/* 2=ok 4=soft 5=hard */
    int     conn_count;			/* connections from this client */
    int     conn_rate;			/* connection rate for this client */
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
    off_t   act_size;			/* END-OF-DATA message size */
    int     junk_cmds;			/* counter */
    int     rcpt_overshoot;		/* counter */
    char   *rewrite_context;		/* address rewriting context */

    /*
     * SASL specific.
     */
#ifdef USE_SASL_AUTH
    struct XSASL_SERVER *sasl_server;
    VSTRING *sasl_reply;
    char   *sasl_mechanism_list;
    char   *sasl_method;
    char   *sasl_username;
    char   *sasl_sender;
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
    char   *saved_bcc;			/* postponed bcc action */
    int     saved_flags;		/* postponed hold/discard */
#ifdef DELAY_ACTION
    int     saved_delay;		/* postponed deferred delay */
#endif
    VSTRING *expand_buf;		/* scratch space for $name expansion */
    ARGV   *prepend;			/* prepended headers */
    VSTRING *instance;			/* policy query correlation */
    int     seqno;			/* policy query correlation */
    int     ehlo_discard_mask;		/* suppressed EHLO features */
    char   *dsn_envid;			/* temporary MAIL FROM state */
    int     dsn_ret;			/* temporary MAIL FROM state */
    VSTRING *dsn_buf;			/* scratch space for xtext expansion */
    VSTRING *dsn_orcpt_buf;		/* scratch space for ORCPT parsing */

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

    /*
     * TLS related state.
     */
#ifdef USE_TLS
    int     tls_use_tls;		/* can use TLS */
    int     tls_enforce_tls;		/* must use TLS */
    int     tls_auth_only;		/* use SASL over TLS only */
    TLS_SESS_STATE *tls_context;	/* TLS session state */
#endif

    /*
     * Milter support.
     */
    const char **milter_argv;		/* SMTP command vector */
    ssize_t milter_argc;		/* SMTP command vector */
    const char *milter_reject_text;	/* input to call-back from Milter */
} SMTPD_STATE;

#define SMTPD_FLAG_HANGUP	   (1<<0)	/* 421/521 disconnect */
#define SMTPD_FLAG_ILL_PIPELINING  (1<<1)	/* inappropriate pipelining */

#define SMTPD_MASK_MAIL_KEEP		~0	/* keep all after MAIL reset */

#define SMTPD_STATE_XFORWARD_INIT  (1<<0)	/* xforward preset done */
#define SMTPD_STATE_XFORWARD_NAME  (1<<1)	/* client name received */
#define SMTPD_STATE_XFORWARD_ADDR  (1<<2)	/* client address received */
#define SMTPD_STATE_XFORWARD_PROTO (1<<3)	/* protocol received */
#define SMTPD_STATE_XFORWARD_HELO  (1<<4)	/* client helo received */
#define SMTPD_STATE_XFORWARD_IDENT (1<<5)	/* message identifier */
#define SMTPD_STATE_XFORWARD_DOMAIN (1<<6)	/* message identifier */
#define SMTPD_STATE_XFORWARD_PORT  (1<<7)	/* client port received */

#define SMTPD_STATE_XFORWARD_CLIENT_MASK \
	(SMTPD_STATE_XFORWARD_NAME | SMTPD_STATE_XFORWARD_ADDR \
	| SMTPD_STATE_XFORWARD_PROTO | SMTPD_STATE_XFORWARD_HELO \
	| SMTPD_STATE_XFORWARD_PORT)

extern void smtpd_state_init(SMTPD_STATE *, VSTREAM *, const char *);
extern void smtpd_state_reset(SMTPD_STATE *);

 /*
  * Conversation stages.  This is used for "lost connection after XXX"
  * diagnostics.
  */
#define SMTPD_AFTER_CONNECT	"CONNECT"
#define SMTPD_AFTER_DATA	"DATA content"
#define SMTPD_AFTER_DOT		"END-OF-MESSAGE"

 /*
  * Other stages. These are sometimes used to change the way information is
  * logged or what information will be available for access control.
  */
#define SMTPD_CMD_HELO		"HELO"
#define SMTPD_CMD_EHLO		"EHLO"
#define SMTPD_CMD_STARTTLS	"STARTTLS"
#define SMTPD_CMD_AUTH		"AUTH"
#define SMTPD_CMD_MAIL		"MAIL"
#define SMTPD_CMD_RCPT		"RCPT"
#define SMTPD_CMD_DATA		"DATA"
#define SMTPD_CMD_EOD		SMTPD_AFTER_DOT	/* XXX Was: END-OF-DATA */
#define SMTPD_CMD_RSET		"RSET"
#define SMTPD_CMD_NOOP		"NOOP"
#define SMTPD_CMD_VRFY		"VRFY"
#define SMTPD_CMD_ETRN		"ETRN"
#define SMTPD_CMD_QUIT		"QUIT"
#define SMTPD_CMD_XCLIENT	"XCLIENT"
#define SMTPD_CMD_XFORWARD	"XFORWARD"
#define SMTPD_CMD_UNKNOWN	"UNKNOWN"

 /*
  * Representation of unknown and non-existent client information. Throughout
  * Postfix, we use the "unknown" string value for unknown client information
  * (e.g., unknown remote client hostname), and we use the empty string, null
  * pointer or "no queue file record" for non-existent client information
  * (e.g., no HELO command, or local submission).
  * 
  * Inside the SMTP server, unknown real client attributes are represented by
  * the string "unknown", and non-existent HELO is represented as a null
  * pointer. The SMTP server uses this same representation internally for
  * forwarded client attributes; the XFORWARD syntax makes no distinction
  * between unknown (remote submission) and non-existent (local submission).
  * 
  * The SMTP client sends forwarded client attributes only when upstream client
  * attributes exist (i.e. remote submission). Thus, local submissions will
  * appear to come from an SMTP-based content filter, which is acceptable.
  * 
  * Known/unknown client attribute values use the SMTP server's internal
  * representation in queue files, in queue manager delivery requests, and in
  * delivery agent $name expansions.
  * 
  * Non-existent attribute values are never present in queue files. Non-existent
  * information is represented as empty strings in queue manager delivery
  * requests and in delivery agent $name expansions.
  */
#define CLIENT_ATTR_UNKNOWN	"unknown"

#define CLIENT_NAME_UNKNOWN	CLIENT_ATTR_UNKNOWN
#define CLIENT_ADDR_UNKNOWN	CLIENT_ATTR_UNKNOWN
#define CLIENT_PORT_UNKNOWN	CLIENT_ATTR_UNKNOWN
#define CLIENT_NAMADDR_UNKNOWN	CLIENT_ATTR_UNKNOWN
#define CLIENT_HELO_UNKNOWN	0
#define CLIENT_PROTO_UNKNOWN	CLIENT_ATTR_UNKNOWN
#define CLIENT_IDENT_UNKNOWN	0
#define CLIENT_DOMAIN_UNKNOWN	0

#define IS_AVAIL_CLIENT_ATTR(v)	((v) && strcmp((v), CLIENT_ATTR_UNKNOWN))

#define IS_AVAIL_CLIENT_NAME(v)	IS_AVAIL_CLIENT_ATTR(v)
#define IS_AVAIL_CLIENT_ADDR(v)	IS_AVAIL_CLIENT_ATTR(v)
#define IS_AVAIL_CLIENT_PORT(v)	IS_AVAIL_CLIENT_ATTR(v)
#define IS_AVAIL_CLIENT_NAMADDR(v) IS_AVAIL_CLIENT_ATTR(v)
#define IS_AVAIL_CLIENT_HELO(v)	((v) != 0)
#define IS_AVAIL_CLIENT_PROTO(v) IS_AVAIL_CLIENT_ATTR(v)
#define IS_AVAIL_CLIENT_IDENT(v) ((v) != 0)
#define IS_AVAIL_CLIENT_DOMAIN(v) ((v) != 0)

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
#define SMTPD_PEER_CODE_FORGED	6

 /*
  * Construct name[addr] or name[addr]:port as appropriate
  */
#define SMTPD_BUILD_NAMADDRPORT(name, addr, port) \
	concatenate((name), "[", (addr), "]", \
		    var_smtpd_client_port_log ? ":" : (char *) 0, \
		    (port), (char *) 0)

 /*
  * Don't mix information from the current SMTP session with forwarded
  * information from an up-stream session.
  */
#define FORWARD_CLIENT_ATTR(s, a) \
	(((s)->xforward.flags & SMTPD_STATE_XFORWARD_CLIENT_MASK) ? \
	    (s)->xforward.a : (s)->a)

#define FORWARD_ADDR(s)		FORWARD_CLIENT_ATTR((s), rfc_addr)
#define FORWARD_NAME(s)		FORWARD_CLIENT_ATTR((s), name)
#define FORWARD_NAMADDR(s)	FORWARD_CLIENT_ATTR((s), namaddr)
#define FORWARD_PROTO(s)	FORWARD_CLIENT_ATTR((s), protocol)
#define FORWARD_HELO(s)		FORWARD_CLIENT_ATTR((s), helo_name)
#define FORWARD_PORT(s)		FORWARD_CLIENT_ATTR((s), port)

#define FORWARD_IDENT(s) \
	(((s)->xforward.flags & SMTPD_STATE_XFORWARD_IDENT) ? \
	    (s)->queue_id : (s)->ident)

#define FORWARD_DOMAIN(s) \
	(((s)->xforward.flags & SMTPD_STATE_XFORWARD_DOMAIN) ? \
	    (s)->xforward.domain : (s)->rewrite_context)

extern void smtpd_xforward_init(SMTPD_STATE *);
extern void smtpd_xforward_preset(SMTPD_STATE *);
extern void smtpd_xforward_reset(SMTPD_STATE *);

 /*
  * Transparency: before mail is queued, do we check for unknown recipients,
  * do we allow address mapping, automatic bcc, header/body checks?
  */
extern int smtpd_input_transp_mask;

 /*
  * More Milter support.
  */
extern MILTERS *smtpd_milters;

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
