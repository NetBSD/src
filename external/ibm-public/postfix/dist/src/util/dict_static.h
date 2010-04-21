/*	$NetBSD: dict_static.h,v 1.1.1.1.4.2 2010/04/21 05:24:17 matt Exp $	*/

#ifndef _DICT_STATIC_H_INCLUDED_
#define _DICT_STATIC_H_INCLUDED_

/*++
/* NAME
/*	dict_static 3h
/* SUMMARY
/*	dictionary manager interface to static settings
/* SYNOPSIS
/*	#include <dict_static.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_STATIC	"static"

extern DICT *dict_static_open(const char *, int, int);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	jeffm
/*	ghostgun.com
/*--*/

#endif
