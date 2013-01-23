/*	$NetBSD: mac_expand.h,v 1.1.1.2.4.1 2013/01/23 00:05:16 yamt Exp $	*/

#ifndef _MAC_EXPAND_H_INCLUDED_
#define _MAC_EXPAND_H_INCLUDED_

/*++
/* NAME
/*	mac_expand 3h
/* SUMMARY
/*	expand macro references in string
/* SYNOPSIS
/*	#include <mac_expand.h>
 DESCRIPTION
 .nf

 /*
  * Utility library.
  */
#include <vstring.h>
#include <mac_parse.h>

 /*
  * Features.
  */
#define MAC_EXP_FLAG_NONE	(0)
#define MAC_EXP_FLAG_RECURSE	(1<<0)
#define MAC_EXP_FLAG_APPEND	(1<<1)
#define MAC_EXP_FLAG_SCAN	(1<<2)

 /*
  * Real lookup or just a test?
  */
#define MAC_EXP_MODE_TEST	(0)
#define MAC_EXP_MODE_USE	(1)

typedef const char *(*MAC_EXP_LOOKUP_FN)(const char *, int, char *);

extern int mac_expand(VSTRING *, const char *, int, const char *, MAC_EXP_LOOKUP_FN, char *);

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
