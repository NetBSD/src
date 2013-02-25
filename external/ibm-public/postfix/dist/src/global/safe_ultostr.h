/*	$NetBSD: safe_ultostr.h,v 1.1.1.1.6.2 2013/02/25 00:27:19 tls Exp $	*/

#ifndef _SAFE_ULTOSTR_H_INCLUDED_
#define _SAFE_ULTOSTR_H_INCLUDED_

/*++
/* NAME
/*	safe_ultostr 3h
/* SUMMARY
/*	convert unsigned long to safe string
/* SYNOPSIS
/*	#include <safe_ultostr.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern char *safe_ultostr(VSTRING *, unsigned long, int, int, int);
extern unsigned long safe_strtoul(const char *, char **, int);

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
