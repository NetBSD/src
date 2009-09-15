/*	$NetBSD: mac_parse.h,v 1.1.1.1.2.2 2009/09/15 06:04:00 snj Exp $	*/

#ifndef _MAC_PARSE_H_INCLUDED_
#define _MAC_PARSE_H_INCLUDED_

/*++
/* NAME
/*	mac_parse 3h
/* SUMMARY
/*	locate macro references in string
/* SYNOPSIS
/*	#include <mac_parse.h>
 DESCRIPTION
 .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
#define MAC_PARSE_LITERAL	1
#define MAC_PARSE_EXPR		2
#define MAC_PARSE_VARNAME	MAC_PARSE_EXPR	/* 2.1 compatibility */

#define MAC_PARSE_OK		0
#define MAC_PARSE_ERROR		(1<<0)
#define MAC_PARSE_UNDEF		(1<<1)
#define MAC_PARSE_USER		2	/* start user definitions */

typedef int (*MAC_PARSE_FN)(int, VSTRING *, char *);

extern int mac_parse(const char *, MAC_PARSE_FN, char *);

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
