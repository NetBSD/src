/*	$NetBSD: match_list.h,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

#ifndef _MATCH_LIST_H_INCLUDED_
#define _MATCH_LIST_H_INCLUDED_

/*++
/* NAME
/*	match_list 3h
/* SUMMARY
/*	generic list-based pattern matching
/* SYNOPSIS
/*	#include <match_list.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
typedef struct MATCH_LIST MATCH_LIST;
typedef int (*MATCH_LIST_FN) (int, const char *, const char *);

extern MATCH_LIST *match_list_init(int, const char *, int,...);
extern int match_list_match(MATCH_LIST *,...);
extern void match_list_free(MATCH_LIST *);

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
