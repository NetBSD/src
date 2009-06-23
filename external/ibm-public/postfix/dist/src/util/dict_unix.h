/*	$NetBSD: dict_unix.h,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

#ifndef _DICT_UNIX_H_INCLUDED_
#define _DICT_UNIX_H_INCLUDED_

/*++
/* NAME
/*	dict_unix 3h
/* SUMMARY
/*	dictionary manager interface to UNIX maps
/* SYNOPSIS
/*	#include <dict_unix.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_UNIX	"unix"

extern DICT *dict_unix_open(const char *, int, int);

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
