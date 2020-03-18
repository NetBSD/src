/*	$NetBSD: normalize_mailhost_addr.h,v 1.2 2020/03/18 19:05:16 christos Exp $	*/

#ifndef _NORMALIZE_MAILHOST_ADDR_H_INCLUDED_
#define _NORMALIZE_MAILHOST_ADDR_H_INCLUDED_

/*++
/* NAME
/*	normalize_mailhost_addr 3h
/* SUMMARY
/*	normalize mailhost address string representation
/* SYNOPSIS
/*	#include <normalize_mailhost_addr.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern int normalize_mailhost_addr(const char *, char **, char **, int *);

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
