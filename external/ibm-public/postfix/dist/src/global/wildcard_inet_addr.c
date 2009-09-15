/*	$NetBSD: wildcard_inet_addr.c,v 1.1.1.1.2.2 2009/09/15 06:02:56 snj Exp $	*/

/*++
/* NAME
/*	wildcard_inet_addr 3
/* SUMMARY
/*	expand wild-card address
/* SYNOPSIS
/*	#include <wildcard_inet_addr.h>
/*
/*	INET_ADDR_LIST *wildcard_inet_addr(void)
/* DESCRIPTION
/*	wildcard_inet_addr() determines all wild-card addresses
/*	for all supported address families.
/* DIAGNOSTICS
/*	Fatal errors: out of memory.
/* SEE ALSO
/*	inet_addr_list(3) address list management
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
/*	Dean C. Strik
/*	Department ICT
/*	Eindhoven University of Technology
/*	P.O. Box 513
/*	5600 MB  Eindhoven, Netherlands
/*	E-mail: <dean@ipnet6.org>
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <msg.h>
#include <inet_addr_list.h>
#include <inet_addr_host.h>

/* Global library. */

#include <wildcard_inet_addr.h>

/* Application-specific. */

static INET_ADDR_LIST wild_addr_list;

static void wildcard_inet_addr_init(INET_ADDR_LIST *addr_list)
{
    inet_addr_list_init(addr_list);
    if (inet_addr_host(addr_list, "") == 0)
	msg_fatal("could not get list of wildcard addresses");
}

/* wildcard_inet_addr_list - return list of addresses */

INET_ADDR_LIST *wildcard_inet_addr_list(void)
{
    if (wild_addr_list.used == 0)
	wildcard_inet_addr_init(&wild_addr_list);

    return (&wild_addr_list);
}
