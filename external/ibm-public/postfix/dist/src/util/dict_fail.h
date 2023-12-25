/*	$NetBSD: dict_fail.h,v 1.1.1.1.44.1 2023/12/25 12:43:36 martin Exp $	*/

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
#include <mkmap.h>

 /*
  * External interface.
  */
#define DICT_TYPE_FAIL	"fail"

extern DICT *dict_fail_open(const char *, int, int);
extern MKMAP *mkmap_fail_open(const char *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	jeffm
/*	ghostgun.com
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
