/*	$NetBSD: sock_addr.h,v 1.1.1.1.2.2 2009/09/15 06:04:03 snj Exp $	*/

#ifndef _SOCK_ADDR_EQ_H_INCLUDED_
#define _SOCK_ADDR_EQ_H_INCLUDED_

/*++
/* NAME
/*	sock_addr 3h
/* SUMMARY
/*	socket address utilities
/* SYNOPSIS
/*	#include <sock_addr.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

 /*
  * External interface.
  */
#define SOCK_ADDR_PTR(ptr)	((struct sockaddr *)(ptr))
#define SOCK_ADDR_FAMILY(ptr)	SOCK_ADDR_PTR(ptr)->sa_family
#ifdef HAS_SA_LEN
#define SOCK_ADDR_LEN(ptr)	SOCK_ADDR_PTR(ptr)->sa_len
#endif

#define SOCK_ADDR_IN_PTR(sa)	((struct sockaddr_in *)(sa))
#define SOCK_ADDR_IN_FAMILY(sa)	SOCK_ADDR_IN_PTR(sa)->sin_family
#define SOCK_ADDR_IN_PORT(sa)	SOCK_ADDR_IN_PTR(sa)->sin_port
#define SOCK_ADDR_IN_ADDR(sa)	SOCK_ADDR_IN_PTR(sa)->sin_addr
#define IN_ADDR(ia)		(*((struct in_addr *) (ia)))

extern int sock_addr_cmp_addr(const struct sockaddr *, const struct sockaddr *);
extern int sock_addr_cmp_port(const struct sockaddr *, const struct sockaddr *);
extern int sock_addr_in_loopback(const struct sockaddr *);

#ifdef HAS_IPV6

#ifndef HAS_SA_LEN
#define SOCK_ADDR_LEN(sa) \
    (SOCK_ADDR_PTR(sa)->sa_family == AF_INET6 ? \
     sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in))
#endif

#define SOCK_ADDR_PORT(sa) \
    (SOCK_ADDR_PTR(sa)->sa_family == AF_INET6 ? \
	SOCK_ADDR_IN6_PORT(sa) : SOCK_ADDR_IN_PORT(sa))
#define SOCK_ADDR_PORTP(sa) \
    (SOCK_ADDR_PTR(sa)->sa_family == AF_INET6 ? \
	&SOCK_ADDR_IN6_PORT(sa) : &SOCK_ADDR_IN_PORT(sa))

#define SOCK_ADDR_IN6_PTR(sa)	((struct sockaddr_in6 *)(sa))
#define SOCK_ADDR_IN6_FAMILY(sa) SOCK_ADDR_IN6_PTR(sa)->sin6_family
#define SOCK_ADDR_IN6_PORT(sa)	SOCK_ADDR_IN6_PTR(sa)->sin6_port
#define SOCK_ADDR_IN6_ADDR(sa)	SOCK_ADDR_IN6_PTR(sa)->sin6_addr
#define IN6_ADDR(ia)		(*((struct in6_addr *) (ia)))

#define SOCK_ADDR_EQ_ADDR(sa, sb) \
    ((SOCK_ADDR_FAMILY(sa) == AF_INET && SOCK_ADDR_FAMILY(sb) == AF_INET \
      && SOCK_ADDR_IN_ADDR(sa).s_addr == SOCK_ADDR_IN_ADDR(sb).s_addr) \
     || (SOCK_ADDR_FAMILY(sa) == AF_INET6 && SOCK_ADDR_FAMILY(sb) == AF_INET6 \
         && memcmp((char *) &(SOCK_ADDR_IN6_ADDR(sa)), \
                   (char *) &(SOCK_ADDR_IN6_ADDR(sb)), \
                   sizeof(SOCK_ADDR_IN6_ADDR(sa))) == 0))

#define SOCK_ADDR_EQ_PORT(sa, sb) \
    ((SOCK_ADDR_FAMILY(sa) == AF_INET && SOCK_ADDR_FAMILY(sb) == AF_INET \
      && SOCK_ADDR_IN_PORT(sa) == SOCK_ADDR_IN_PORT(sb)) \
     || (SOCK_ADDR_FAMILY(sa) == AF_INET6 && SOCK_ADDR_FAMILY(sb) == AF_INET6 \
         && SOCK_ADDR_IN6_PORT(sa) == SOCK_ADDR_IN6_PORT(sb)))

#else

#ifndef HAS_SA_LEN
#define SOCK_ADDR_LEN(sa)	sizeof(struct sockaddr_in)
#endif

#define SOCK_ADDR_PORT(sa)	SOCK_ADDR_IN_PORT(sa))
#define SOCK_ADDR_PORTP(sa)	&SOCK_ADDR_IN_PORT(sa))

#define SOCK_ADDR_EQ_ADDR(sa, sb) \
    (SOCK_ADDR_FAMILY(sa) == AF_INET && SOCK_ADDR_FAMILY(sb) == AF_INET \
     && SOCK_ADDR_IN_ADDR(sa).s_addr == SOCK_ADDR_IN_ADDR(sb).s_addr)

#define SOCK_ADDR_EQ_PORT(sa, sb) \
    (SOCK_ADDR_FAMILY(sa) == AF_INET && SOCK_ADDR_FAMILY(sb) == AF_INET \
     && SOCK_ADDR_IN_PORT(sa) == SOCK_ADDR_IN_PORT(sb))

#endif

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
