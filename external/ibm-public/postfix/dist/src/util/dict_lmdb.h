/*	$NetBSD: dict_lmdb.h,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

#ifndef _DICT_LMDB_H_INCLUDED_
#define _DICT_LMDB_H_INCLUDED_

/*++
/* NAME
/*	dict_lmdb 3h
/* SUMMARY
/*	dictionary manager interface to OpenLDAP LMDB files
/* SYNOPSIS
/*	#include <dict_lmdb.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_LMDB	"lmdb"

extern DICT *dict_lmdb_open(const char *, int, int);

 /*
  * XXX Should be part of the DICT interface.
  */
extern size_t dict_lmdb_map_size;

 /* Minimum size without SIGSEGV. */
#define DEFINE_DICT_LMDB_MAP_SIZE size_t dict_lmdb_map_size = 8192

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Howard Chu
/*	Symas Corporation
/*--*/

#endif
