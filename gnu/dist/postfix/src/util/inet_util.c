/*	$NetBSD: inet_util.c,v 1.1.1.5 2006/01/05 02:17:36 rpaulo Exp $	*/

/*++
/* NAME
/*	inet_util 3
/* SUMMARY
/*	INET-domain utilities
/* SYNOPSIS
/*	#include <inet_util.h>
/*
/*	char	*inet_parse(addr, hostp, portp)
/*	const char *addr;
/*	char	**hostp;
/*	char	**portp;
/* DESCRIPTION
/*	This module implements various support routines for
/*	dealing with AF_INET connections, addresses etc.
/*
/*	inet_parse() takes an address of the form host:port and
/*	breaks it up into its constituent parts. The resulting
/*	host information is an empty string when the address
/*	contains no host part or no host: part. inet_parse()
/*	returns a pointer to memory that it has allocated for
/*	string storage. The caller should pass the host to the
/*	myfree() function when the storage is no longer needed.
/* DIAGNOSTICS
/*	Fatal errors: invalid address or host forms.
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

/* System libraries. */

#include <sys_defs.h>

/* Utility library. */

#include "mymalloc.h"
#include "split_at.h"
#include "inet_util.h"

/* inet_parse - parse host:port address spec */

char   *inet_parse(const char *addr, char **hostp, char **portp)
{
    char   *buf;

    buf = mystrdup(addr);
    if ((*portp = split_at_right(buf, ':')) != 0) {
	*hostp = buf;
    } else {
	*portp = buf;
	*hostp = "";
    }
    return (buf);
}
