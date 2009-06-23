/*	$NetBSD: myaddrinfo.h,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

#ifndef _MYADDRINFO_H_INCLUDED_
#define _MYADDRINFO_H_INCLUDED_

/*++
/* NAME
/*	myaddrinfo 3h
/* SUMMARY
/*	addrinfo encapsulation and emulation
/* SYNOPSIS
/*	#include <myaddrinfo.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>			/* MAI_STRERROR() */
#include <limits.h>			/* CHAR_BIT */

 /*
  * Backwards compatibility support for IPV4 systems without addrinfo API.
  */
#ifdef EMULATE_IPV4_ADDRINFO

 /*
  * Avoid clashes with global symbols, just in case some third-party library
  * provides its own addrinfo() implementation. This also allows us to test
  * the IPV4 emulation code on an IPV6 enabled system.
  */
#undef  freeaddrinfo
#define freeaddrinfo	mai_freeaddrinfo
#undef  gai_strerror
#define gai_strerror	mai_strerror
#undef  addrinfo
#define addrinfo	mai_addrinfo
#undef  sockaddr_storage
#define sockaddr_storage mai_sockaddr_storage

 /*
  * Modern systems define this in <netdb.h>.
  */
struct addrinfo {
    int     ai_flags;			/* AI_PASSIVE|CANONNAME|NUMERICHOST */
    int     ai_family;			/* PF_xxx */
    int     ai_socktype;		/* SOCK_xxx */
    int     ai_protocol;		/* 0 or IPPROTO_xxx */
    size_t  ai_addrlen;			/* length of ai_addr */
    char   *ai_canonname;		/* canonical name for nodename */
    struct sockaddr *ai_addr;		/* binary address */
    struct addrinfo *ai_next;		/* next structure in linked list */
};

 /*
  * Modern systems define this in <sys/socket.h>.
  */
struct sockaddr_storage {
    struct sockaddr_in dummy;		/* alignment!! */
};

 /*
  * Result codes. See gai_strerror() for text. Undefine already imported
  * definitions so that we can test the IPv4-only emulation on a modern
  * system without getting a ton of compiler warnings.
  */
#undef  EAI_ADDRFAMILY
#define EAI_ADDRFAMILY   1
#undef  EAI_AGAIN
#define EAI_AGAIN        2
#undef  EAI_BADFLAGS
#define EAI_BADFLAGS     3
#undef  EAI_FAIL
#define EAI_FAIL         4
#undef  EAI_FAMILY
#define EAI_FAMILY       5
#undef  EAI_MEMORY
#define EAI_MEMORY       6
#undef  EAI_NODATA
#define EAI_NODATA       7
#undef  EAI_NONAME
#define EAI_NONAME       8
#undef  EAI_SERVICE
#define EAI_SERVICE      9
#undef  EAI_SOCKTYPE
#define EAI_SOCKTYPE    10
#undef  EAI_SYSTEM
#define EAI_SYSTEM      11
#undef  EAI_BADHINTS
#define EAI_BADHINTS    12
#undef  EAI_PROTOCOL
#define EAI_PROTOCOL    13
#undef  EAI_RESNULL
#define EAI_RESNULL     14
#undef  EAI_MAX
#define EAI_MAX         15

extern void freeaddrinfo(struct addrinfo *);
extern char *gai_strerror(int);

#endif

 /*
  * Bounds grow in leaps. These macros attempt to keep non-library code free
  * from IPV6 #ifdef pollution. Avoid macro names that end in STRLEN because
  * they suggest that space for the null terminator is not included.
  */
#ifdef HAS_IPV6
# define MAI_HOSTADDR_STRSIZE	INET6_ADDRSTRLEN
#else
# ifndef INET_ADDRSTRLEN
#  define INET_ADDRSTRLEN	16
# endif
# define MAI_HOSTADDR_STRSIZE	INET_ADDRSTRLEN
#endif

#define MAI_HOSTNAME_STRSIZE	1025
#define MAI_SERVNAME_STRSIZE	32
#define MAI_SERVPORT_STRSIZE	sizeof("65535")

#define MAI_V4ADDR_BITS		32
#define MAI_V6ADDR_BITS		128
#define MAI_V4ADDR_BYTES	((MAI_V4ADDR_BITS + (CHAR_BIT - 1))/CHAR_BIT)
#define MAI_V6ADDR_BYTES	((MAI_V6ADDR_BITS + (CHAR_BIT - 1))/CHAR_BIT)

 /*
  * Routines and data structures to hide some of the complexity of the
  * addrinfo API. They still don't hide that we may get results for address
  * families that we aren't interested in.
  * 
  * Note: the getnameinfo() and inet_ntop() system library functions use unsafe
  * APIs with separate pointer and length arguments. To avoid buffer overflow
  * problems with these functions, Postfix uses pointers to structures
  * internally. This way the compiler can enforce that callers provide
  * buffers with the appropriate length, instead of having to trust that
  * callers will never mess up some length calculation.
  */
typedef struct {
    char    buf[MAI_HOSTNAME_STRSIZE];
} MAI_HOSTNAME_STR;

typedef struct {
    char    buf[MAI_HOSTADDR_STRSIZE];
} MAI_HOSTADDR_STR;

typedef struct {
    char    buf[MAI_SERVNAME_STRSIZE];
} MAI_SERVNAME_STR;

typedef struct {
    char    buf[MAI_SERVPORT_STRSIZE];
} MAI_SERVPORT_STR;

extern int hostname_to_sockaddr(const char *, const char *, int,
				        struct addrinfo **);
extern int hostaddr_to_sockaddr(const char *, const char *, int,
				        struct addrinfo **);
extern int sockaddr_to_hostaddr(const struct sockaddr *, SOCKADDR_SIZE,
		               MAI_HOSTADDR_STR *, MAI_SERVPORT_STR *, int);
extern int sockaddr_to_hostname(const struct sockaddr *, SOCKADDR_SIZE,
		               MAI_HOSTNAME_STR *, MAI_SERVNAME_STR *, int);
extern void myaddrinfo_control(int,...);

#define MAI_CTL_END	0		/* list terminator */

#define MAI_STRERROR(e) ((e) == EAI_SYSTEM ? strerror(errno) : gai_strerror(e))

 /*
  * Macros for the case where we really don't want to be bothered with things
  * that may fail.
  */
#define HOSTNAME_TO_SOCKADDR(host, serv, sock, res) \
    do { \
	int _aierr; \
	_aierr = hostname_to_sockaddr((host), (serv), (sock), (res)); \
	if (_aierr) \
	    msg_fatal("hostname_to_sockaddr: %s", MAI_STRERROR(_aierr)); \
    } while (0)

#define HOSTADDR_TO_SOCKADDR(host, serv, sock, res) \
    do { \
	int _aierr; \
	_aierr = hostaddr_to_sockaddr((host), (serv), (sock), (res)); \
	if (_aierr) \
	    msg_fatal("hostaddr_to_sockaddr: %s", MAI_STRERROR(_aierr)); \
    } while (0)

#define SOCKADDR_TO_HOSTADDR(sa, salen, host, port, sock) \
    do { \
	int _aierr; \
	_aierr = sockaddr_to_hostaddr((sa), (salen), (host), (port), (sock)); \
	if (_aierr) \
	    msg_fatal("sockaddr_to_hostaddr: %s", MAI_STRERROR(_aierr)); \
    } while (0)

#define SOCKADDR_TO_HOSTNAME(sa, salen, host, service, sock) \
    do { \
	int _aierr; \
	_aierr = sockaddr_to_hostname((sa), (salen), (host), (service), (sock)); \
	if (_aierr) \
	    msg_fatal("sockaddr_to_hostname: %s", MAI_STRERROR(_aierr)); \
    } while (0)

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#endif
