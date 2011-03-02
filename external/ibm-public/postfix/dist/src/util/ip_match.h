/*	$NetBSD: ip_match.h,v 1.1.1.1 2011/03/02 19:32:44 tron Exp $	*/

#ifndef _IP_MATCH_H_INCLUDED_
#define _IP_MATCH_H_INCLUDED_

/*++
/* NAME
/*	ip_match 3h
/* SUMMARY
/*	IP address pattern matching
/* SYNOPSIS
/*	#include <ip_match.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern char *ip_match_parse(VSTRING *, char *);
extern char *ip_match_save(const VSTRING *);
extern char *ip_match_dump(VSTRING *, const char *);
extern int ip_match_execute(const char *, const char *);

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
