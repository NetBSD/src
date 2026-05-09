/*	$NetBSD: dict_debug.h,v 1.1.1.1 2026/05/09 18:39:23 christos Exp $	*/

#ifndef _DICT_DEBUG_H_INCLUDED_
#define _DICT_DEBUG_H_INCLUDED_

/*++
/* NAME
/*	dict_debug 3h
/* SUMMARY
/*	dictionary manager interface for "debug" tables
/* SYNOPSIS
/*	#include <dict_debug.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_DEBUG "debug"

extern DICT *dict_debug_open(const char *, int, int);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Richard Hansen <rhansen@rhansen.org>
/*--*/

#endif
