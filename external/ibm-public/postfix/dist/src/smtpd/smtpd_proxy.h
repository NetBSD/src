/*	$NetBSD: smtpd_proxy.h,v 1.1.1.2.12.1 2014/08/19 23:59:44 tls Exp $	*/

/*++
/* NAME
/*	smtpd_proxy 3h
/* SUMMARY
/*	SMTP server pass-through proxy client
/* SYNOPSIS
/*	#include <smtpd.h>
/*	#include <smtpd_proxy.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>
#include <vstring.h>

 /*
  * Application-specific.
  */
typedef int PRINTFPTRLIKE(3, 4) (*SMTPD_PROXY_CMD_FN) (SMTPD_STATE *, int, const char *,...);
typedef int PRINTFPTRLIKE(3, 4) (*SMTPD_PROXY_REC_FPRINTF_FN) (VSTREAM *, int, const char *,...);
typedef int (*SMTPD_PROXY_REC_PUT_FN) (VSTREAM *, int, const char *, ssize_t);

typedef struct SMTPD_PROXY {
    /* Public. */
    VSTREAM *stream;
    VSTRING *request;			/* proxy request buffer */
    VSTRING *reply;			/* proxy reply buffer */
    SMTPD_PROXY_CMD_FN cmd;
    SMTPD_PROXY_REC_FPRINTF_FN rec_fprintf;
    SMTPD_PROXY_REC_PUT_FN rec_put;
    /* Private. */
    int     flags;
    VSTREAM *service_stream;
    const char *service_name;
    int     timeout;
    const char *ehlo_name;
    const char *mail_from;
} SMTPD_PROXY;

#define SMTPD_PROXY_FLAG_SPEED_ADJUST	(1<<0)

#define SMTPD_PROXY_NAME_SPEED_ADJUST	"speed_adjust"

#define SMTPD_PROX_WANT_BAD	0xff	/* Do not use */
#define SMTPD_PROX_WANT_NONE	'\0'	/* Do not receive reply */
#define SMTPD_PROX_WANT_ANY	'0'	/* Expect any reply */
#define SMTPD_PROX_WANT_OK	'2'	/* Expect 2XX reply */
#define SMTPD_PROX_WANT_MORE	'3'	/* Expect 3XX reply */

extern int smtpd_proxy_create(SMTPD_STATE *, int, const char *, int, const char *, const char *);
extern void smtpd_proxy_close(SMTPD_STATE *);
extern void smtpd_proxy_free(SMTPD_STATE *);
extern int smtpd_proxy_parse_opts(const char *, const char *);

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
