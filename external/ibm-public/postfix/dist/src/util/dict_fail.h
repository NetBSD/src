/*	$NetBSD: dict_fail.h,v 1.1.1.1.2.2 2013/01/23 00:05:16 yamt Exp $	*/

#ifndef _DICT_FAIL_H_INCLUDED_
#define _DICT_FAIL_H_INCLUDED_

/*++
/* NAME
/*	dict_fail 3h
/* SUMMARY
/*	dictionary manager interface to 'always fail' table
/* SYNOPSIS
/*	#include <dict_fail.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_FAIL	"fail"

extern DICT *dict_fail_open(const char *, int, int);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	jeffm
/*	ghostgun.com
/*--*/

#endif
