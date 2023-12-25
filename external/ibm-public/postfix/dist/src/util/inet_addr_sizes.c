/*	$NetBSD: inet_addr_sizes.c,v 1.2.2.2 2023/12/25 12:43:37 martin Exp $	*/

/*++
/* NAME
/*	inet_addr_sizes 3
/* SUMMARY
/*      get network address size metrics
/* SYNOPSIS
/*	#include <inet_addr_sizes.h>
/*
/*	typedef struct {
/* .in +4
/*	    int     af;                  /* network address family (binary) */
/*	    char   *ipproto_str;         /* IP protocol version (string) */
/*	    int     addr_bitcount;       /* bits per address */
/*	    int     addr_bytecount;      /* bytes per address */
/*	    int     addr_strlen;         /* address string length */
/*	    int     addr_bitcount_strlen;/* addr_bitcount string length */
/* .in -4
/*	} INET_ADDR_SIZES;
/*
/*	const INET_ADDR_SIZES *inet_addr_sizes(int family)
/* DESCRIPTION
/*	inet_addr_sizes() returns address size metrics for the
/*	specified network address family, AF_INET or AF_INET6.
/* DIAGNOSTICS
/*	inet_addr_sizes() returns a null pointer when the argument
/*	specifies an unexpected address family.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <inet_addr_sizes.h>
#include <msg.h>
#include <myaddrinfo.h>

 /*
  * Stringify a numeric constant and use sizeof() to determine the resulting
  * string length at compile time. Note that sizeof() includes a null
  * terminator; the -1 corrects for that.
  */
#define _STRINGIFY(x) #x
#define _STRLEN(x) (sizeof(_STRINGIFY(x)) - 1)

static const INET_ADDR_SIZES table[] = {
    {AF_INET, "IPv4", MAI_V4ADDR_BITS, MAI_V4ADDR_BYTES, INET_ADDRSTRLEN,
    _STRLEN(MAI_V4ADDR_BITS)},
#ifdef HAS_IPV6
    {AF_INET6, "IPv6", MAI_V6ADDR_BITS, MAI_V6ADDR_BYTES, INET6_ADDRSTRLEN,
    _STRLEN(MAI_V6ADDR_BITS)},
#endif
};

/* inet_addr_sizes - get address size metrics for address family */

const   INET_ADDR_SIZES *inet_addr_sizes(int af)
{
    const   INET_ADDR_SIZES *sp;

    for (sp = table; /* see below */ ; sp++) {
	if (sp >= table + sizeof(table) / sizeof(*table))
	    return (0);
	if (sp->af == af)
	    return (sp);
    }
}
