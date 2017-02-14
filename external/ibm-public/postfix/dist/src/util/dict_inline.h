/*	$NetBSD: dict_inline.h,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

#ifndef _DICT_INLINE_H_INCLUDED_
#define _DICT_INLINE_H_INCLUDED_

/*++
/* NAME
/*	dict_inline 3h
/* SUMMARY
/*	dictionary manager interface for inline tables
/* SYNOPSIS
/*	#include <dict_inline.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_INLINE	"inline"

extern DICT *dict_inline_open(const char *, int, int);

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
