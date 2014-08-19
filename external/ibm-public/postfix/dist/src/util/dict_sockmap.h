/*	$NetBSD: dict_sockmap.h,v 1.1.1.1.8.2 2014/08/19 23:59:45 tls Exp $	*/

#ifndef _DICT_SOCKMAP_H_INCLUDED_
#define _DICT_SOCKMAP_H_INCLUDED_

/*++
/* NAME
/*	dict_sockmap 3h
/* SUMMARY
/*	dictionary manager interface to Sendmail-stye socketmap.
/* SYNOPSIS
/*	#include <dict_sockmap.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_SOCKMAP	"socketmap"

extern DICT *dict_sockmap_open(const char *, int, int);

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
