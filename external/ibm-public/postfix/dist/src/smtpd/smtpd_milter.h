/*	$NetBSD: smtpd_milter.h,v 1.1.1.1.4.2 2010/04/21 05:24:12 matt Exp $	*/

/*++
/* NAME
/*	smtpd_milter 3h
/* SUMMARY
/*	SMTP server milter glue
/* SYNOPSIS
/*	#include <smtpd.h>
/*	#include <smtpd_milter.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern const char *smtpd_milter_eval(const char *, void *);

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

