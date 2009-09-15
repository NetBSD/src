/*	$NetBSD: dict_tcp.h,v 1.1.1.1.2.2 2009/09/15 06:03:57 snj Exp $	*/

#ifndef _DICT_TCP_H_INCLUDED_
#define _DICT_TCP_H_INCLUDED_

/*++
/* NAME
/*	dict_tcp 3h
/* SUMMARY
/*	dictionary manager interface to tcp-based lookup tables
/* SYNOPSIS
/*	#include <dict_tcp.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_TCP	"tcp"

extern DICT *dict_tcp_open(const char *, int, int);

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
