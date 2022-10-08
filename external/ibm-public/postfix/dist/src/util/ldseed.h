/*	$NetBSD: ldseed.h,v 1.1.1.1 2022/10/08 16:09:11 christos Exp $	*/

#ifndef _LDSEED_H_INCLUDED_
#define _LDSEED_H_INCLUDED_

/*++
/* NAME
/*	ldseed 3h
/* SUMMARY
/*	seed for non-cryptographic applications
/* SYNOPSIS
/*	#include <ldseed.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern void ldseed(void *, size_t);

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
