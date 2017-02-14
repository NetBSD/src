/*	$NetBSD: dict_random.h,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

#ifndef _DICT_RANDOM_H_INCLUDED_
#define _DICT_RANDOM_H_INCLUDED_

/*++
/* NAME
/*	dict_random 3h
/* SUMMARY
/*	dictionary manager interface for ramdomized tables
/* SYNOPSIS
/*	#include <dict_random.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_RANDOM	"randmap"

extern DICT *dict_random_open(const char *, int, int);

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
