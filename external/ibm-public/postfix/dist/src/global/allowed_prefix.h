/*	$NetBSD: allowed_prefix.h,v 1.2 2026/05/09 18:49:15 christos Exp $	*/

#ifndef _ALLOWED_PARENT_H_INCLUDED_
#define _ALLOWED_PARENT_H_INCLUDED_

/*++
/* NAME
/*	allowed_prefix 3h
/* SUMMARY
/*	Match a pathname against a list of allowed parent directories
/* SYNOPSIS
/*	#include <allowed_prefix.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <argv.h>

 /*
  * External interface.
  */
typedef ARGV ALLOWED_PREFIX;
extern ALLOWED_PREFIX *allowed_prefix_create(const char *);
extern bool allowed_prefix_match(ALLOWED_PREFIX *, const char *);
extern void allowed_prefix_free(ALLOWED_PREFIX *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
