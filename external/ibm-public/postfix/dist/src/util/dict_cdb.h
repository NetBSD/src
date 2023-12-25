/*	$NetBSD: dict_cdb.h,v 1.1.1.1.60.1 2023/12/25 12:43:36 martin Exp $	*/

#ifndef _DICT_CDB_H_INCLUDED_
#define _DICT_CDB_H_INCLUDED_

/*++
/* NAME
/*	dict_cdb 3h
/* SUMMARY
/*	dictionary manager interface to CDB files
/* SYNOPSIS
/*	#include <dict_cdb.h>
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
#define DICT_TYPE_CDB "cdb"

extern DICT *dict_cdb_open(const char *, int, int);
extern MKMAP *mkmap_cdb_open(const char *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif /* _DICT_CDB_H_INCLUDED_ */
