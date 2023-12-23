/*	$NetBSD: inet_prefix_top.h,v 1.1.1.1 2023/12/23 20:24:59 christos Exp $	*/

#ifndef _INET_MASK_TOP_H_INCLUDED_
#define _INET_MASK_TOP_H_INCLUDED_

/*++
/* NAME
/*	inet_prefix_top 3h
/* SUMMARY
/*	convert net/mask to printable string
/* SYNOPSIS
/*	#include <inet_prefix_top.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern char *inet_prefix_top(int, const void *, int);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*--*/

#endif
