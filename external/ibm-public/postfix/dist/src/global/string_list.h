/*	$NetBSD: string_list.h,v 1.1.1.1.10.1 2013/01/23 00:05:04 yamt Exp $	*/

#ifndef _STRING_LIST_H_INCLUDED_
#define _STRING_LIST_H_INCLUDED_

/*++
/* NAME
/*	string_list 3h
/* SUMMARY
/*	match a string against a pattern list
/* SYNOPSIS
/*	#include <string_list.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <match_list.h>

 /*
  * External interface.
  */
#define STRING_LIST	MATCH_LIST

#define string_list_init(f, p)	match_list_init((f), (p), 1, match_string)
#define string_list_match	match_list_match
#define string_list_free	match_list_free

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
