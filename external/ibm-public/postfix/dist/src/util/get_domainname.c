/*	$NetBSD: get_domainname.c,v 1.1.1.1.2.2 2009/09/15 06:03:58 snj Exp $	*/

/*++
/* NAME
/*	get_domainname 3
/* SUMMARY
/*	network domain name lookup
/* SYNOPSIS
/*	#include <get_domainname.h>
/*
/*	const char *get_domainname()
/* DESCRIPTION
/*	get_domainname() returns the local domain name as obtained
/*	by stripping the hostname component from the result from
/*	get_hostname().  The result is the hostname when get_hostname()
/*	does not return a FQDN form ("foo"), or its result has only two
/*	components ("foo.com").
/* DIAGNOSTICS
/*	Fatal errors: no hostname, invalid hostname.
/* SEE ALSO
/*	get_hostname(3)
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

/* System library. */

#include <sys_defs.h>
#include <string.h>

/* Utility library. */

#include "mymalloc.h"
#include "get_hostname.h"
#include "get_domainname.h"

/* Local stuff. */

static char *my_domain_name;

/* get_domainname - look up my domain name */

const char *get_domainname(void)
{
    const char *host;
    const char *dot;

    /*
     * Use the hostname when it is not a FQDN ("foo"), or when the hostname
     * actually is a domain name ("foo.com").
     */
    if (my_domain_name == 0) {
	host = get_hostname();
	if ((dot = strchr(host, '.')) == 0 || strchr(dot + 1, '.') == 0) {
	    my_domain_name = mystrdup(host);
	} else {
	    my_domain_name = mystrdup(dot + 1);
	}
    }
    return (my_domain_name);
}
