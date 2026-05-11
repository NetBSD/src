/*	$NetBSD: sane_sockaddr_to_hostaddr.c,v 1.2.2.1 2026/05/11 17:14:04 martin Exp $	*/

/*++
/* NAME
/*	sane_sockaddr_to_hostaddr 3
/* SUMMARY
/*	Sanitize IPv4 in IPv6 address
/* SYNOPSIS
/*	#include <myaddrinfo.h>
/*
/*	int	sane_sockaddr_to_hostaddr(
/*	struct sockaddr *addr_storage,
/*	SOCKADDR_SIZE addr_storage_len,
/*	MAI_HOSTADDR_STR *addr_buf,
/*	MAI_SERVPORT_STR *port_buf,
/*	int socktype)
/* DESCRIPTION
/*	sane_sockaddr_to_hostaddr() converts a V4mapped IPv6 address to
/*	IPv4 form, but only if IPv4 support is available. It then invokes
/*	sockaddr_to_hostaddr() to convert the result to human-readable
/*	form.
/*
/*	NOTE: The V4mapped IPv6 conversion to IPv4 applies to both inputs
/*	and output.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <myaddrinfo.h>
#include <normalize_v4mapped_addr.h>

/* sane_sockaddr_to_hostaddr - sanitize IPV4 in IPV6 address */

int     sane_sockaddr_to_hostaddr(struct sockaddr *addr_storage,
				          SOCKADDR_SIZE *addr_storage_len,
				          MAI_HOSTADDR_STR *addr_buf,
				          MAI_SERVPORT_STR *port_buf,
				          int socktype)
{
#ifdef AF_INET6
    if (addr_storage->sa_family == AF_INET6)
	normalize_v4mapped_sockaddr(addr_storage, addr_storage_len);
#endif
    return (sockaddr_to_hostaddr(addr_storage, *addr_storage_len,
				 addr_buf, port_buf, socktype));
}
