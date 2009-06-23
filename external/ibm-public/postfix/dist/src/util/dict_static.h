/*	$NetBSD: dict_static.h,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

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
