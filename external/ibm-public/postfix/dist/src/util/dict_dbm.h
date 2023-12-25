/*	$NetBSD: dict_dbm.h,v 1.1.1.2.54.1 2023/12/25 12:43:36 martin Exp $	*/

#ifndef _DICT_DBM_H_INCLUDED_
#define _DICT_DBM_H_INCLUDED_

/*++
/* NAME
/*	dict_dbm 3h
/* SUMMARY
/*	dictionary manager interface to DBM files
/* SYNOPSIS
/*	#include <dict_dbm.h>
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
#define DICT_TYPE_DBM	"dbm"

extern DICT *dict_dbm_open(const char *, int, int);
extern MKMAP *mkmap_dbm_open(const char *);

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

#endif
