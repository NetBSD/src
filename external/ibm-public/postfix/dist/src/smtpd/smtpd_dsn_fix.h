/*	$NetBSD: smtpd_dsn_fix.h,v 1.1.1.1.16.1 2014/08/19 23:59:44 tls Exp $	*/

/*++
/* NAME
/*	smtpd_check 3h
/* SUMMARY
/*	SMTP client request filtering
/* SYNOPSIS
/*	#include "smtpd.h"
/*	#include "smtpd_check_int.h"
/* DESCRIPTION
/* .nf

 /*
  * Internal interface.
  */
#define SMTPD_NAME_CLIENT	"Client host"
#define SMTPD_NAME_REV_CLIENT	"Unverified Client host"
#define SMTPD_NAME_CCERT	"Client certificate"
#define SMTPD_NAME_SASL_USER	"SASL login name"
#define SMTPD_NAME_HELO		"Helo command"
#define SMTPD_NAME_SENDER	"Sender address"
#define SMTPD_NAME_RECIPIENT	"Recipient address"
#define SMTPD_NAME_ETRN		"Etrn command"
#define SMTPD_NAME_DATA		"Data command"
#define SMTPD_NAME_EOD		"End-of-data"

 /*
  * Workaround for absence of "bad sender address" status code: use "bad
  * sender address syntax" instead. If we were to use "4.1.0" then we would
  * lose the critical distinction between sender and recipient problems.
  */
#define SND_DSN			"4.1.7"

extern const char *smtpd_dsn_fix(const char *, const char *);

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
