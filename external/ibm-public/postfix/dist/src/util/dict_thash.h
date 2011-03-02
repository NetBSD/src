/*	$NetBSD: dict_thash.h,v 1.1.1.1 2011/03/02 19:32:42 tron Exp $	*/

#ifndef _DICT_THASH_H_INCLUDED_
#define _DICT_THASH_H_INCLUDED_

/*++
/* NAME
/*	dict_thash 3h
/* SUMMARY
/*	dictionary manager interface to flat text files
/* SYNOPSIS
/*	#include <dict_thash.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_THASH	"texthash"

extern DICT *dict_thash_open(const char *, int, int);

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
