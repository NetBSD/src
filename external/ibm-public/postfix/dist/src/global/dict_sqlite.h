/*	$NetBSD: dict_sqlite.h,v 1.1.1.1 2011/03/02 19:32:14 tron Exp $	*/

#ifndef _DICT_SQLITE_H_INCLUDED_
#define _DICT_SQLITE_H_INCLUDED_

/*++
/* NAME
/*	dict_sqlite 3h
/* SUMMARY
/*	dictionary manager interface to sqlite databases
/* SYNOPSIS
/*	#include <dict_sqlite.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_SQLITE "sqlite"

extern DICT *dict_sqlite_open(const char *, int, int);


/* AUTHOR(S)
/*	Axel Steiner
/*	ast@treibsand.com
/*--*/

#endif
