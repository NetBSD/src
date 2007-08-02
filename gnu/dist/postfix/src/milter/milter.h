/*	$NetBSD: milter.h,v 1.1.1.3.12.2 2007/08/02 08:05:19 heas Exp $	*/

#ifndef _MILTER_H_INCLUDED_
#define _MILTER_H_INCLUDED_

/*++
/* NAME
/*	milter 3h
/* SUMMARY
/*	smtp server
/* SYNOPSIS
/*	Postfix MTA-side Milter implementation
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>
#include <vstream.h>
#include <argv.h>

 /*
  * Each Milter handle is an element of a null-terminated linked list. The
  * functions are virtual so that we can support multiple MTA-side Milter
  * implementations. The Sendmail 8 and Sendmail X Milter-side APIs are too
  * different to implement the MTA side as a single hybrid.
  */
typedef struct MILTER {
    char   *name;			/* full name including transport */
    struct MILTER *next;		/* linkage */
    struct MILTERS *parent;		/* parent information */
    const char *(*conn_event) (struct MILTER *, const char *, const char *, const char *, unsigned, ARGV *);
    const char *(*helo_event) (struct MILTER *, const char *, int, ARGV *);
    const char *(*mail_event) (struct MILTER *, const char **, ARGV *);
    const char *(*rcpt_event) (struct MILTER *, const char **, ARGV *);
    const char *(*data_event) (struct MILTER *, ARGV *);
    const char *(*message) (struct MILTER *, VSTREAM *, off_t, ARGV *);
    const char *(*unknown_event) (struct MILTER *, const char *, ARGV *);
    const char *(*other_event) (struct MILTER *);
    void    (*abort) (struct MILTER *);
    void    (*disc_event) (struct MILTER *);
    int     (*active) (struct MILTER *);
    int     (*send) (struct MILTER *, VSTREAM *);
    void    (*free) (struct MILTER *);
} MILTER;

extern MILTER *milter8_create(const char *, int, int, int, const char *, const char *, struct MILTERS *);
extern MILTER *milter8_receive(VSTREAM *, struct MILTERS *);

 /*
  * A bunch of Milters.
  */
typedef struct MILTERS {
    MILTER *milter_list;		/* linked list of Milters */
    const char *(*mac_lookup) (const char *, void *);
    void   *mac_context;		/* macro lookup context */
    char   *conn_macros;		/* macros for connect event */
    char   *helo_macros;		/* macros for HELO/EHLO command */
    char   *mail_macros;		/* macros for MAIL FROM command */
    char   *rcpt_macros;		/* macros for RCPT TO command */
    char   *data_macros;		/* macros for DATA command */
    char   *eod_macros;			/* macros for END-OF-DATA command */
    char   *unk_macros;			/* macros for unknown command */
    void   *chg_context;		/* context for queue file changes */
    const char *(*add_header) (void *, char *, char *);
    const char *(*upd_header) (void *, ssize_t, char *, char *);
    const char *(*del_header) (void *, ssize_t, char *);
    const char *(*ins_header) (void *, ssize_t, char *, char *);
    const char *(*add_rcpt) (void *, char *);
    const char *(*del_rcpt) (void *, char *);
    const char *(*repl_body) (void *, int, VSTRING *);
} MILTERS;

typedef const char *(*MILTER_MAC_LOOKUP_FN) (const char *, void *);
typedef const char *(*MILTER_ADD_HEADER_FN) (void *, char *, char *);
typedef const char *(*MILTER_EDIT_HEADER_FN) (void *, ssize_t, char *, char *);
typedef const char *(*MILTER_DEL_HEADER_FN) (void *, ssize_t, char *);
typedef const char *(*MILTER_EDIT_RCPT_FN) (void *, char *);
typedef const char *(*MILTER_EDIT_BODY_FN) (void *, int, VSTRING *);

extern MILTERS *milter_create(const char *, int, int, int,
			              const char *, const char *,
			              const char *, const char *,
			              const char *, const char *,
			              const char *, const char *,
			              const char *);
extern void milter_macro_callback(MILTERS *, MILTER_MAC_LOOKUP_FN, void *);
extern void milter_edit_callback(MILTERS *milters, MILTER_ADD_HEADER_FN,
		               MILTER_EDIT_HEADER_FN, MILTER_EDIT_HEADER_FN,
			          MILTER_DEL_HEADER_FN, MILTER_EDIT_RCPT_FN,
			           MILTER_EDIT_RCPT_FN, MILTER_EDIT_BODY_FN,
				         void *);
extern const char *milter_conn_event(MILTERS *, const char *, const char *, const char *, unsigned);
extern const char *milter_helo_event(MILTERS *, const char *, int);
extern const char *milter_mail_event(MILTERS *, const char **);
extern const char *milter_rcpt_event(MILTERS *, const char **);
extern const char *milter_data_event(MILTERS *);
extern const char *milter_message(MILTERS *, VSTREAM *, off_t);
extern const char *milter_unknown_event(MILTERS *, const char *);
extern const char *milter_other_event(MILTERS *);
extern void milter_abort(MILTERS *);
extern void milter_disc_event(MILTERS *);
extern int milter_dummy(MILTERS *, VSTREAM *);
extern int milter_send(MILTERS *, VSTREAM *);
extern MILTERS *milter_receive(VSTREAM *, int);
extern void milter_free(MILTERS *);

 /*
  * Milter body edit commands.
  */
#define MILTER_BODY_START	1	/* start message body */
#define MILTER_BODY_LINE	2	/* message body line */
#define MILTER_BODY_END		3	/* end message body */

 /*
  * Sendmail 8 macro names. We support forms with and without the {}.
  */
#define S8_MAC__		"{_}"	/* sender resolve */
#define S8_MAC_J		"{j}"	/* myhostname */
#define S8_MAC_V		"{v}"	/* mail_name + mail_version */

#define S8_MAC_DAEMON_NAME	"{daemon_name}"
#define S8_MAC_IF_NAME		"{if_name}"
#define S8_MAC_IF_ADDR		"{if_addr}"

#define S8_MAC_CLIENT_ADDR	"{client_addr}"
#define S8_MAC_CLIENT_CONN	"{client_connections}"
#define S8_MAC_CLIENT_NAME	"{client_name}"
#define S8_MAC_CLIENT_PTR	"{client_ptr}"
#define S8_MAC_CLIENT_RES	"{client_resolve}"

#define S8_MAC_TLS_VERSION	"{tls_version}"
#define S8_MAC_CIPHER		"{cipher}"
#define S8_MAC_CIPHER_BITS	"{cipher_bits}"
#define S8_MAC_CERT_SUBJECT	"{cert_subject}"
#define S8_MAC_CERT_ISSUER	"{cert_issuer}"

#define S8_MAC_I		"{i}"	/* queue ID */
#define S8_MAC_AUTH_TYPE	"{auth_type}"	/* SASL method */
#define S8_MAC_AUTH_AUTHEN	"{auth_authen}"	/* SASL username */
#define S8_MAC_AUTH_AUTHOR	"{auth_author}"	/* SASL sender */

#define S8_MAC_MAIL_MAILER	"{mail_mailer}"	/* sender transport */
#define S8_MAC_MAIL_HOST	"{mail_host}"	/* sender nexthop */
#define S8_MAC_MAIL_ADDR	"{mail_addr}"	/* sender address */

#define S8_MAC_RCPT_MAILER	"{rcpt_mailer}"	/* recip transport */
#define S8_MAC_RCPT_HOST	"{rcpt_host}"	/* recip nexthop */
#define S8_MAC_RCPT_ADDR	"{rcpt_addr}"	/* recip address */

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

#endif
