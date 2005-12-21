/*
 * Copyright (C) 1999-2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * INTERNET SOFTWARE CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* Id: platform.h.in,v 1.24.2.1 2001/11/02 00:20:07 marka Exp */

#ifndef ISC_PLATFORM_H
#define ISC_PLATFORM_H 1

/*****
 ***** Platform-dependent defines.
 *****/

/***
 *** Network.
 ***/

/*
 * Define if this system needs the <netinet/in6.h> header file included
 * for full IPv6 support (pretty much only UnixWare).
 */
#undef ISC_PLATFORM_NEEDNETINETIN6H

/*
 * Define if this system needs the <netinet6/in6.h> header file included
 * to support in6_pkinfo (pretty much only BSD/OS).
 */
#undef ISC_PLATFORM_NEEDNETINET6IN6H

/*
 * If sockaddrs on this system have an sa_len field, ISC_PLATFORM_HAVESALEN
 * will be defined.
 */
#define ISC_PLATFORM_HAVESALEN 1

/*
 * If this system has the IPv6 structure definitions, ISC_PLATFORM_HAVEIPV6
 * will be defined.
 */
#define ISC_PLATFORM_HAVEIPV6 1

/*
 * If this system is missing in6addr_any, ISC_PLATFORM_NEEDIN6ADDRANY will
 * be defined.
 */
#undef ISC_PLATFORM_NEEDIN6ADDRANY

/*
 * If this system is missing in6addr_loopback, ISC_PLATFORM_NEEDIN6ADDRLOOPBACK
 * will be defined.
 */
#undef ISC_PLATFORM_NEEDIN6ADDRLOOPBACK

/*
 * If this system has in6_pktinfo, ISC_PLATFORM_HAVEIN6PKTINFO will be
 * defined.
 */
#define ISC_PLATFORM_HAVEIN6PKTINFO 1

/*
 * If this system has in_addr6, rather than in6_addr, ISC_PLATFORM_HAVEINADDR6
 * will be defined.
 */
#undef ISC_PLATFORM_HAVEINADDR6

/*
 * If this system needs inet_ntop(), ISC_PLATFORM_NEEDNTOP will be defined.
 */
#undef ISC_PLATFORM_NEEDNTOP

/*
 * If this system needs inet_pton(), ISC_PLATFORM_NEEDPTON will be defined.
 */
#undef ISC_PLATFORM_NEEDPTON

/*
 * If this system needs inet_aton(), ISC_PLATFORM_NEEDATON will be defined.
 */
#undef ISC_PLATFORM_NEEDATON

/*
 * If this system needs in_port_t, ISC_PLATFORM_NEEDPORTT will be defined.
 */
#undef ISC_PLATFORM_NEEDPORTT

/*
 * If the system needs strsep(), ISC_PLATFORM_NEEDSTRSEP will be defined.
 */
#undef ISC_PLATFORM_NEEDSTRSEP

/*
 * Define either ISC_PLATFORM_BSD44MSGHDR or ISC_PLATFORM_BSD43MSGHDR.
 */
#define ISC_NET_BSD44MSGHDR 1

/*
 * Define if PTHREAD_ONCE_INIT should be surrounded by braces to
 * prevent compiler warnings (such as with gcc on Solaris 2.8).
 */
#undef ISC_PLATFORM_BRACEPTHREADONCEINIT

/*
 * Define on some UnixWare systems to fix erroneous definitions of various
 * IN6_IS_ADDR_* macros.
 */
#undef ISC_PLATFORM_FIXIN6ISADDR

/***
 *** Printing.
 ***/

/*
 * If this system needs vsnprintf() and snprintf(), ISC_PLATFORM_NEEDVSNPRINTF
 * will be defined.
 */
#undef ISC_PLATFORM_NEEDVSNPRINTF

/*
 * The printf format string modifier to use with isc_uint64_t values.
 */
#define ISC_PLATFORM_QUADFORMAT "ll"

/*
 * Defined if we are using threads.
 */
#undef ISC_PLATFORM_USETHREADS

/*
 * Defined if unistd.h does not cause fd_set to be delared.
 */
#undef ISC_PLATFORM_NEEDSYSSELECTH

/*
 * Type used for resource limits.
 */
#define ISC_PLATFORM_RLIMITTYPE rlim_t

/*
 * Define if your compiler supports "long long int".
 */
#define ISC_PLATFORM_HAVELONGLONG 1

/*
 * Used to control how extern data is linked; needed for Win32 platforms.
 */
#undef ISC_PLATFORM_USEDECLSPEC

#ifndef ISC_PLATFORM_USEDECLSPEC
#define LIBISC_EXTERNAL_DATA
#define LIBDNS_EXTERNAL_DATA
#define LIBISCCC_EXTERNAL_DATA
#define LIBISCCFG_EXTERNAL_DATA
#else /* ISC_PLATFORM_USEDECLSPEC */
#ifdef LIBISC_EXPORTS
#define LIBISC_EXTERNAL_DATA __declspec(dllexport)
#else
#define LIBISC_EXTERNAL_DATA __declspec(dllimport)
#endif
#ifdef LIBDNS_EXPORTS
#define LIBDNS_EXTERNAL_DATA __declspec(dllexport)
#else
#define LIBDNS_EXTERNAL_DATA __declspec(dllimport)
#endif
#ifdef LIBISCCC_EXPORTS
#define LIBISCCC_EXTERNAL_DATA __declspec(dllexport)
#else
#define LIBISCCC_EXTERNAL_DATA __declspec(dllimport)
#endif
#ifdef LIBISCCFG_EXPORTS
#define LIBISCCFG_EXTERNAL_DATA __declspec(dllexport)
#else
#define LIBISCCFG_EXTERNAL_DATA __declspec(dllimport)
#endif
#endif /* ISC_PLATFORM_USEDECLSPEC */

/*
 * Tell emacs to use C mode for this file.
 *
 * Local Variables:
 * mode: c
 * End:
 */

#endif /* ISC_PLATFORM_H */
