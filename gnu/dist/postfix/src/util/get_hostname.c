/*++
/* NAME
/*	get_hostname 3
/* SUMMARY
/*	network name lookup
/* SYNOPSIS
/*	#include <get_hostname.h>
/*
/*	const char *get_hostname()
/* DESCRIPTION
/*	get_hostname() returns the local hostname as obtained
/*	via gethostname() or its moral equivalent. This routine
/*	goes to great length to avoid dependencies on any network
/*	services.
/* DIAGNOSTICS
/*	Fatal errors: no hostname, invalid hostname.
/* SEE ALSO
/*	valid_hostname(3)
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
#include <sys/param.h>
#include <string.h>
#include <unistd.h>

#if (MAXHOSTNAMELEN < 256)
#undef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	256
#endif

/* Utility library. */

#include "mymalloc.h"
#include "msg.h"
#include "valid_hostname.h"
#include "get_hostname.h"

/* Local stuff. */

static char *my_host_name;

/* get_hostname - look up my host name */

const char *get_hostname(void)
{
    char    namebuf[MAXHOSTNAMELEN + 1];

    /*
     * The gethostname() call is not (or not yet) in ANSI or POSIX, but it is
     * part of the socket interface library. We avoid the more politically-
     * correct uname() routine because that has no portable way of dealing
     * with long (FQDN) hostnames.
     */
    if (my_host_name == 0) {
	if (gethostname(namebuf, sizeof(namebuf)) < 0)
	    msg_fatal("gethostname: %m");
	namebuf[MAXHOSTNAMELEN] = 0;
	if (valid_hostname(namebuf, DO_GRIPE) == 0)
	    msg_fatal("unable to use my own hostname");
	my_host_name = mystrdup(namebuf);
    }
    return (my_host_name);
}
