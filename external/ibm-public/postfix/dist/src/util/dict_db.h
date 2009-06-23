/*	$NetBSD: dict_db.h,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

#ifndef _DICT_DB_H_INCLUDED_
#define _DICT_DB_H_INCLUDED_

/*++
/* NAME
/*	dict_db 3h
/* SUMMARY
/*	dictionary manager interface to DB files
/* SYNOPSIS
/*	#include <dict_db.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_HASH	"hash"
#define DICT_TYPE_BTREE	"btree"

extern DICT *dict_hash_open(const char *, int, int);
extern DICT *dict_btree_open(const char *, int, int);

 /*
  * XXX Should be part of the DICT interface.
  */
extern int dict_db_cache_size;

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
