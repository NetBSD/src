/*	$NetBSD: valid_uri_scheme.h,v 1.1.1.1 2025/02/25 19:11:47 christos Exp $	*/

#ifndef _VALID_SCHEME_H_INCLUDED_
#define _VALID_SCHEME_H_INCLUDED_

/*++
/* NAME
/*	valid_uri_scheme 3h
/* SUMMARY
/*	validate scheme:// prefix
/* SYNOPSIS
/*	#include <valid_uri_scheme.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern ssize_t valid_uri_scheme(const char *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
