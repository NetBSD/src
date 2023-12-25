/*	$NetBSD: hash_fnv.h,v 1.2.2.1 2023/12/25 12:43:37 martin Exp $	*/

#ifndef _HASH_FNV_H_INCLUDED_
#define _HASH_FNV_H_INCLUDED_

/*++
/* NAME
/*	hash_fnv 3h
/* SUMMARY
/*	Fowler/Noll/Vo hash function
/* SYNOPSIS
/*	#include <hash_fnv.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
#ifndef HASH_FNV_T
#include <stdint.h>
#ifdef USE_FNV_32BIT
#define HASH_FNV_T	uint32_t
#else					/* USE_FNV_32BIT */
#define HASH_FNV_T	uint64_t
#endif					/* USE_FNV_32BIT */
#endif					/* HASH_FNV_T */

extern HASH_FNV_T hash_fnv(const void *, size_t);
extern HASH_FNV_T hash_fnvz(const char *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
