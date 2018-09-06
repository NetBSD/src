/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#ifndef IRS_NETDB_H
#define IRS_NETDB_H 1

#include <stddef.h>	/* Required on FreeBSD (and  others?) for size_t. */
#include <netdb.h>	/* Contractual provision. */

/*
 * Define if <netdb.h> does not declare struct addrinfo.
 */
#undef ISC_IRS_NEEDADDRINFO

#ifdef ISC_IRS_NEEDADDRINFO
struct addrinfo {
	int		ai_flags;      /* AI_PASSIVE, AI_CANONNAME */
	int		ai_family;     /* PF_xxx */
	int		ai_socktype;   /* SOCK_xxx */
	int		ai_protocol;   /* 0 or IPPROTO_xxx for IPv4 and IPv6 */
	size_t		ai_addrlen;    /* Length of ai_addr */
	char		*ai_canonname; /* Canonical name for hostname */
	struct sockaddr	*ai_addr;      /* Binary address */
	struct addrinfo	*ai_next;      /* Next structure in linked list */
};
#endif

/*
 * Undefine all #defines we are interested in as <netdb.h> may or may not have
 * defined them.
 */

/*
 * Error return codes from gethostbyname() and gethostbyaddr()
 * (left in extern int h_errno).
 */

#undef	NETDB_INTERNAL
#undef	NETDB_SUCCESS
#undef	HOST_NOT_FOUND
#undef	TRY_AGAIN
#undef	NO_RECOVERY
#undef	NO_DATA
#undef	NO_ADDRESS

#define	NETDB_INTERNAL	-1	/* see errno */
#define	NETDB_SUCCESS	0	/* no problem */
#define	HOST_NOT_FOUND	1 /* Authoritative Answer Host not found */
#define	TRY_AGAIN	2 /* Non-Authoritive Host not found, or SERVERFAIL */
#define	NO_RECOVERY	3 /* Non recoverable errors, FORMERR, REFUSED, NOTIMP */
#define	NO_DATA		4 /* Valid name, no data record of requested type */
#define	NO_ADDRESS	NO_DATA		/* no address, look for MX record */

/*
 * Error return codes from getaddrinfo().  EAI_INSECUREDATA is our own extension
 * and it's very unlikely to be already defined, but undef it just in case; it
 * at least doesn't do any harm.
 */

#undef	EAI_ADDRFAMILY
#undef	EAI_AGAIN
#undef	EAI_BADFLAGS
#undef	EAI_FAIL
#undef	EAI_FAMILY
#undef	EAI_MEMORY
#undef	EAI_NODATA
#undef	EAI_NONAME
#undef	EAI_SERVICE
#undef	EAI_SOCKTYPE
#undef	EAI_SYSTEM
#undef	EAI_BADHINTS
#undef	EAI_PROTOCOL
#undef	EAI_OVERFLOW
#undef	EAI_INSECUREDATA
#undef	EAI_MAX

#define	EAI_ADDRFAMILY	 1	/* address family for hostname not supported */
#define	EAI_AGAIN	 2	/* temporary failure in name resolution */
#define	EAI_BADFLAGS	 3	/* invalid value for ai_flags */
#define	EAI_FAIL	 4	/* non-recoverable failure in name resolution */
#define	EAI_FAMILY	 5	/* ai_family not supported */
#define	EAI_MEMORY	 6	/* memory allocation failure */
#define	EAI_NODATA	 7	/* no address associated with hostname */
#define	EAI_NONAME	 8	/* hostname nor servname provided, or not known */
#define	EAI_SERVICE	 9	/* servname not supported for ai_socktype */
#define	EAI_SOCKTYPE	10	/* ai_socktype not supported */
#define	EAI_SYSTEM	11	/* system error returned in errno */
#define EAI_BADHINTS	12
#define EAI_PROTOCOL	13
#define EAI_OVERFLOW	14
#define EAI_INSECUREDATA 15
#define EAI_MAX		16

/*
 * Flag values for getaddrinfo()
 */
#undef	AI_PASSIVE
#undef	AI_CANONNAME
#undef	AI_NUMERICHOST

#define	AI_PASSIVE	0x00000001
#define	AI_CANONNAME	0x00000002
#define AI_NUMERICHOST	0x00000004

/*
 * Flag values for getipnodebyname()
 */
#undef AI_V4MAPPED
#undef AI_ALL
#undef AI_ADDRCONFIG
#undef AI_DEFAULT

#define AI_V4MAPPED	0x00000008
#define AI_ALL		0x00000010
#define AI_ADDRCONFIG	0x00000020
#define AI_DEFAULT	(AI_V4MAPPED|AI_ADDRCONFIG)

/*
 * Constants for getnameinfo()
 */
#undef	NI_MAXHOST
#undef	NI_MAXSERV

#define	NI_MAXHOST	1025
#define	NI_MAXSERV	32

/*
 * Flag values for getnameinfo()
 */
#undef	NI_NOFQDN
#undef	NI_NUMERICHOST
#undef	NI_NAMEREQD
#undef	NI_NUMERICSERV
#undef	NI_DGRAM
#undef	NI_NUMERICSCOPE

#define	NI_NOFQDN	0x00000001
#define	NI_NUMERICHOST	0x00000002
#define	NI_NAMEREQD	0x00000004
#define	NI_NUMERICSERV	0x00000008
#define	NI_DGRAM	0x00000010

/*
 * Define to map into irs_ namespace.
 */

#ifndef __NetBSD__
#define IRS_NAMESPACE
#endif

#ifdef IRS_NAMESPACE

/*
 * Use our versions not the ones from the C library.
 */

#ifdef getnameinfo
#undef getnameinfo
#endif
#define getnameinfo irs_getnameinfo

#ifdef getaddrinfo
#undef getaddrinfo
#endif
#define getaddrinfo irs_getaddrinfo

#ifdef freeaddrinfo
#undef freeaddrinfo
#endif
#define freeaddrinfo irs_freeaddrinfo

#ifdef gai_strerror
#undef gai_strerror
#endif
#define gai_strerror irs_gai_strerror

int
getaddrinfo(const char *hostname, const char *servname,
	    const struct addrinfo *hints, struct addrinfo **res);

int
getnameinfo(const struct sockaddr *sa, IRS_GETNAMEINFO_SOCKLEN_T salen,
	    char *host, IRS_GETNAMEINFO_BUFLEN_T hostlen,
	    char *serv, IRS_GETNAMEINFO_BUFLEN_T servlen,
	    IRS_GETNAMEINFO_FLAGS_T flags);

void freeaddrinfo (struct addrinfo *ai);

IRS_GAISTRERROR_RETURN_T
gai_strerror(int ecode);

#endif

/*
 * Tell Emacs to use C mode on this file.
 * Local variables:
 * mode: c
 * End:
 */

#endif /* IRS_NETDB_H */
