/*	$NetBSD: nvtable.h,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

#ifndef _NVTABLE_H_INCLUDED_
#define _NVTABLE_H_INCLUDED_

/*++
/* NAME
/*	nvtable 3h
/* SUMMARY
/*	attribute list manager
/* SYNOPSIS
/*	#include <nvtable.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <htable.h>
#include <mymalloc.h>

typedef struct HTABLE NVTABLE;
typedef struct HTABLE_INFO NVTABLE_INFO;

#define nvtable_create(size)		htable_create(size)
#define nvtable_locate(table, key)	htable_locate((table), (key))
#define nvtable_walk(table, action, ptr) htable_walk((table), (action), (ptr))
#define nvtable_list(table)		htable_list(table)
#define nvtable_find(table, key)	htable_find((table), (key))
#define nvtable_delete(table, key)	htable_delete((table), (key), myfree)
#define nvtable_free(table)		htable_free((table), myfree)

extern NVTABLE_INFO *nvtable_update(NVTABLE *, const char *, const char *);

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
