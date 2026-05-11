/*	$NetBSD: normalize_v4mapped_addr.c,v 1.2.2.2 2026/05/11 17:14:04 martin Exp $	*/

/*++
/* NAME
/*	normalize_v4mapped_addr 3
/* SUMMARY
/*	normalize v4mapped IPv6 address
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	int     normalize_v4mapped_sockaddr(
/*	struct sockaddr *sa,
/*	SOCKADDR_SIZE *sa_len)
/*
/*	int     normalize_v4mapped_hostaddr(
/*	MAI_HOSTADDR_STR *addr)
/* DESCRIPTION
/*	The functions in this module convert IPV6 addresses containing a
/*	mapped IPv4 address to the IPv4 form, but only if IPv4 support
/*	is enabled.
/*
/*	normalize_v4mapped_sockaddr converts a V4mapped IPv6 sockaddr
/*	structure (struct sockaddr_in6) to the IPv4 form (struct
/*	sockaddr_in) and reduces *sa_len to (sizeof(struct sockaddr_in)).
/*
/*	normalize_v4mapped_hostaddr converts a V4mapped IPv6 printable
/*	address (:ffff:d.d.d.d) to the IPv4 form (d.d.d.d).
/*
/*	Both functions return 0 (false) when no input was converted,
/*	and return 1 (true) when input was converted.
/* BUGS
/*	normalize_v4mapped_hostaddr() converts only canonical address
/*	forms, not all equivalent forms.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <inet_proto.h>
#include <sock_addr.h>
#include <normalize_v4mapped_addr.h>

/* normalize_v4mapped_sockaddr - normalize V4mapped IPv6 sockaddr */

int     normalize_v4mapped_sockaddr(struct sockaddr *sa, SOCKADDR_SIZE *sa_len)
{
#ifdef HAS_IPV6
    struct sockaddr_in sin;

    if (sa->sa_family == AF_INET6
	&& IN6_IS_ADDR_V4MAPPED(&SOCK_ADDR_IN6_ADDR(sa))
      && strchr((char *) inet_proto_info()->sa_family_list, AF_INET) != 0) {
	memset((void *) &sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = SOCK_ADDR_IN6_PORT(sa);
	memcpy((void *) &sin.sin_addr, ((void *) &SOCK_ADDR_IN6_ADDR(sa)) + 12,
	       sizeof(sin.sin_addr));
	*(struct sockaddr_in *) sa = sin;
	*sa_len = sizeof(sin);
	return (1);
    }
#endif
    return (0);
}

/* normalize_v4mapped_hostaddr - normalize V4mapped IPv6 printable address */

int     normalize_v4mapped_hostaddr(MAI_HOSTADDR_STR *addr)
{
#ifdef HAS_IPV6
    if (addr->buf[0] == ':'
	&& strncasecmp("::ffff:", addr->buf, 7) == 0
      && strchr((char *) inet_proto_info()->sa_family_list, AF_INET) != 0) {
	memmove(addr->buf, addr->buf + 7, strlen(addr->buf) + 1 - 7);
	return (1);
    }
#endif
    return (0);
}
