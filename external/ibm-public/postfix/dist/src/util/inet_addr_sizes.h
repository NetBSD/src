/*	$NetBSD: inet_addr_sizes.h,v 1.2.2.2 2023/12/25 12:43:37 martin Exp $	*/

#ifndef _INET_ADDR_SIZES_H_INCLUDED_
#define _INET_ADDR_SIZES_H_INCLUDED_

/*++
/* NAME
/*      inet_addr_sizes 3h
/* SUMMARY
/*      get network address size metrics
/* SYNOPSIS
/*      #include <inet_addr_sizes.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
typedef struct {
    int     af;				/* network address family (binary) */
    char   *ipproto_str;		/* IP protocol version (string) */
    int     addr_bitcount;		/* bits per address */
    int     addr_bytecount;		/* bytes per address */
    int     addr_strlen;		/* address string length */
    int     addr_bitcount_strlen;	/* addr_bitcount string length */
} INET_ADDR_SIZES;

extern const INET_ADDR_SIZES *inet_addr_sizes(int);

/* LICENSE
/* .ad
/* .fi
/*      The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*      Wietse Venema
/*--*/

#endif
