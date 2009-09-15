/*	$NetBSD: mail_addr.h,v 1.1.1.1.2.2 2009/09/15 06:02:44 snj Exp $	*/

#ifndef _MAIL_ADDR_H_INCLUDED_
#define _MAIL_ADDR_H_INCLUDED_

/*++
/* NAME
/*	mail_addr 3h
/* SUMMARY
/*	pre-defined mail addresses
/* SYNOPSIS
/*	#include <mail_addr.h>
/* DESCRIPTION
/* .nf

 /*
  * Pre-defined addresses.
  */
#define MAIL_ADDR_POSTMASTER	"postmaster"
#define MAIL_ADDR_MAIL_DAEMON	"MAILER-DAEMON"
#define MAIL_ADDR_EMPTY		""

extern const char *mail_addr_double_bounce(void);
extern const char *mail_addr_postmaster(void);
extern const char *mail_addr_mail_daemon(void);

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
