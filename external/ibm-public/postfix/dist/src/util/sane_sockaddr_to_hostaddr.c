/*	$NetBSD: sane_sockaddr_to_hostaddr.c,v 1.2 2025/02/25 19:15:52 christos Exp $	*/

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
/*	sane_sockaddr_to_hostaddr() wraps sockaddr_to_hostaddr() and
/*	converts an IPv4 in IPv6 address to IPv4 form, but only if IPv4
/*	support is available.
/* HISTORY
/* .ad
/* .fi
/*	This implementation was taken from postscreen, and consolidates
/*	multiple instances of similar code across the Postfix code base.
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
#include <inet_proto.h>

static const INET_PROTO_INFO *proto_info;

/* sane_sockaddr_to_hostaddr - sanitize IPV4 in IPV6 address */

int     sane_sockaddr_to_hostaddr(const struct sockaddr *addr_storage,
				          SOCKADDR_SIZE addr_storage_len,
				          MAI_HOSTADDR_STR *addr_buf,
				          MAI_SERVPORT_STR *port_buf,
				          int socktype)
{
    int     aierr;

    if (proto_info == 0)
	proto_info = inet_proto_info();

    if ((aierr = sockaddr_to_hostaddr(addr_storage, addr_storage_len,
				      addr_buf, port_buf, socktype)) == 0
	&& strncasecmp("::ffff:", addr_buf->buf, 7) == 0
	&& strchr((char *) proto_info->sa_family_list, AF_INET) != 0)
	memmove(addr_buf->buf, addr_buf->buf + 7,
		sizeof(addr_buf->buf) - 7);
    return (aierr);
}
