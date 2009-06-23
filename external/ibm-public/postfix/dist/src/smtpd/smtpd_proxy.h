/*	$NetBSD: smtpd_proxy.h,v 1.1.1.1 2009/06/23 10:08:56 tron Exp $	*/

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
#define SMTPD_PROX_WANT_NONE	'\0'	/* Do not receive reply */
#define SMTPD_PROX_WANT_ANY	'0'	/* Expect any reply */
#define SMTPD_PROX_WANT_OK	'2'	/* Expect 2XX reply */
#define SMTPD_PROX_WANT_MORE	'3'	/* Expect 3XX reply */

extern int smtpd_proxy_open(SMTPD_STATE *, const char *, int, const char *, const char *);
extern int PRINTFLIKE(3, 4) smtpd_proxy_cmd(SMTPD_STATE *, int, const char *,...);
extern int smtpd_proxy_rec_put(VSTREAM *, int, const char *, ssize_t);
extern int PRINTFLIKE(3, 4) smtpd_proxy_rec_fprintf(VSTREAM *, int, const char *,...);
extern void smtpd_proxy_close(SMTPD_STATE *);

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
