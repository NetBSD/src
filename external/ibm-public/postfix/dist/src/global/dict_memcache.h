/*	$NetBSD: dict_memcache.h,v 1.1.1.1.2.2 2013/01/23 00:05:02 yamt Exp $	*/

#ifndef _DICT_MEMCACHE_INCLUDED_
#define _DICT_MEMCACHE_INCLUDED_

/*++
/* NAME
/*	dict_memcache 3h
/* SUMMARY
/*	dictionary interface to memcache databases
/* SYNOPSIS
/*	#include <dict_memcache.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_MEMCACHE "memcache"

extern DICT *dict_memcache_open(const char *name, int unused_flags,
				        int dict_flags);

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
