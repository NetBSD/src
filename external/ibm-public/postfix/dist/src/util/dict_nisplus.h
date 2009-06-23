/*	$NetBSD: dict_nisplus.h,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

#ifndef _DICT_NISPLUS_H_INCLUDED_
#define _DICT_NISPLUS_H_INCLUDED_

/*++
/* NAME
/*	dict_nisplus 3h
/* SUMMARY
/*	dictionary manager interface to NIS+ maps
/* SYNOPSIS
/*	#include <dict_nisplus.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_NISPLUS	"nisplus"

extern DICT *dict_nisplus_open(const char *, int, int);

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
