/*	$NetBSD: dns_str_resflags.c,v 1.2.8.1 2023/12/25 12:43:31 martin Exp $	*/

/*++
/* NAME
/*	dns_str_resflags 3
/* SUMMARY
/*	convert resolver flags to printable form
/* SYNOPSIS
/*	#include <dns.h>
/*
/*	const char *dns_str_resflags(mask)
/*	unsigned long mask;
/* DESCRIPTION
/*	dns_str_resflags() converts RES_* resolver(5) flags from internal
/*	form to printable string. Individual flag names are separated
/*	with '|'.  The result is overwritten with each call.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*
/*	Viktor Dukhovni
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

 /*
  * Utility library.
  */
#include <name_mask.h>

 /*
  * DNS library.
  */
#include <dns.h>

 /*
  * Application-specific.
  */

 /*
  * This list overlaps with dns_res_opt_masks[] in smtp.c, but there we
  * permit only a small subset of all possible flags.
  */
static const LONG_NAME_MASK resflag_table[] = {
    "RES_INIT", RES_INIT,
    "RES_DEBUG", RES_DEBUG,
#if defined(RES_AAONLY) && !HAVE_GLIBC_API_VERSION_SUPPORT(2, 24)
    "RES_AAONLY", RES_AAONLY,
#endif
    "RES_USEVC", RES_USEVC,
#if defined(RES_PRIMARY) && !HAVE_GLIBC_API_VERSION_SUPPORT(2, 24)
    "RES_PRIMARY", RES_PRIMARY,
#endif
    "RES_IGNTC", RES_IGNTC,
    "RES_RECURSE", RES_RECURSE,
    "RES_DEFNAMES", RES_DEFNAMES,
    "RES_STAYOPEN", RES_STAYOPEN,
    "RES_DNSRCH", RES_DNSRCH,
#ifdef RES_INSECURE1
    "RES_INSECURE1", RES_INSECURE1,
#endif
#ifdef RES_INSECURE2
    "RES_INSECURE2", RES_INSECURE2,
#endif
    "RES_NOALIASES", RES_NOALIASES,
#ifdef RES_USE_INET6
    "RES_USE_INET6", RES_USE_INET6,
#endif
#ifdef RES_ROTATE
    "RES_ROTATE", RES_ROTATE,
#endif
#if defined(RES_NOCHECKNAME) && !HAVE_GLIBC_API_VERSION_SUPPORT(2, 24)
    "RES_NOCHECKNAME", RES_NOCHECKNAME,
#endif
    "RES_USE_EDNS0", RES_USE_EDNS0,
    "RES_USE_DNSSEC", RES_USE_DNSSEC,
#if defined(RES_KEEPTSIG) && !HAVE_GLIBC_API_VERSION_SUPPORT(2, 24)
    "RES_KEEPTSIG", RES_KEEPTSIG,
#endif
#if defined(RES_BLAST) && !HAVE_GLIBC_API_VERSION_SUPPORT(2, 24)
    "RES_BLAST", RES_BLAST,
#endif
#ifdef RES_USEBSTRING
    "RES_USEBSTRING", RES_USEBSTRING,
#endif
#ifdef RES_NSID
    "RES_NSID", RES_NSID,
#endif
#ifdef RES_NOIP6DOTINT
    "RES_NOIP6DOTINT", RES_NOIP6DOTINT,
#endif
#ifdef RES_USE_DNAME
    "RES_USE_DNAME", RES_USE_DNAME,
#endif
#ifdef RES_NO_NIBBLE2
    "RES_NO_NIBBLE2", RES_NO_NIBBLE2,
#endif
#ifdef RES_SNGLKUP
    "RES_SNGLKUP", RES_SNGLKUP,
#endif
#ifdef RES_SNGLKUPREOP
    "RES_SNGLKUPREOP", RES_SNGLKUPREOP,
#endif
#ifdef RES_NOTLDQUERY
    "RES_NOTLDQUERY", RES_NOTLDQUERY,
#endif
    0,
};

/* dns_str_resflags - convert RES_* resolver flags to printable form */

const char *dns_str_resflags(unsigned long mask)
{
    static VSTRING *buf;

    if (buf == 0)
	buf = vstring_alloc(20);
    return (str_long_name_mask_opt(buf, "dsns_str_resflags", resflag_table,
				   mask, NAME_MASK_NUMBER | NAME_MASK_PIPE));
}
