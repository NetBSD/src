/*	$NetBSD: mail_addr_crunch.h,v 1.1.1.1.2.2 2009/09/15 06:02:44 snj Exp $	*/

#ifndef _MAIL_ADDR_CRUNCH_H_INCLUDED_
#define _MAIL_ADDR_CRUNCH_H_INCLUDED_

/*++
/* NAME
/*	mail_addr_crunch 3h
/* SUMMARY
/*	parse and canonicalize addresses, apply address extension
/* SYNOPSIS
/*	#include <mail_addr_crunch.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <argv.h>

 /*
  * External interface.
  */
extern ARGV *mail_addr_crunch(const char *, const char *);

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
