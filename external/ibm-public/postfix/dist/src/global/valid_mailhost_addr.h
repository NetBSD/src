/*	$NetBSD: valid_mailhost_addr.h,v 1.1.1.1 2009/06/23 10:08:48 tron Exp $	*/

#ifndef _VALID_MAILHOST_ADDR_H_INCLUDED_
#define _VALID_MAILHOST_ADDR_H_INCLUDED_

/*++
/* NAME
/*	valid_mailhost_addr 3h
/* SUMMARY
/*	mailhost address syntax validation
/* SYNOPSIS
/*	#include <valid_mailhost_addr.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <valid_hostname.h>

 /*
  * External interface
  */
#define IPV6_COL		"IPv6:"	/* RFC 2821 */

extern const char *valid_mailhost_addr(const char *, int);
extern int valid_mailhost_literal(const char *, int);

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
