/*	$NetBSD: dict_ht.h,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

#ifndef _DICT_HT_H_INCLUDED_
#define _DICT_HT_H_INCLUDED_

/*++
/* NAME
/*	dict_ht 3h
/* SUMMARY
/*	dictionary manager interface to hash tables
/* SYNOPSIS
/*	#include <dict_ht.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <htable.h>
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_HT	"internal"

extern DICT *dict_ht_open(const char *, HTABLE *, void (*) (char *));

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
