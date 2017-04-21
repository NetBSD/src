/*	$NetBSD: dict_union.h,v 1.2.4.2 2017/04/21 16:52:53 bouyer Exp $	*/

#ifndef _DICT_UNION_H_INCLUDED_
#define _DICT_UNION_H_INCLUDED_

/*++
/* NAME
/*	dict_union 3h
/* SUMMARY
/*	dictionary manager interface for union of tables
/* SYNOPSIS
/*	#include <dict_union.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_UNION	"unionmap"

extern DICT *dict_union_open(const char *, int, int);

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
