/*++
/* NAME
/*	dns_strerror 3
/* SUMMARY
/*	name service lookup error code to string
/* SYNOPSIS
/*	#include <dhs.h>
/*
/*	const char *dns_strerror(code)
/*	int	code;
/* DESCRIPTION
/*	dns_strerror() maps a name service lookup error to printable string.
/*	The result is for read-only purposes, and unknown codes share a
/*	common string buffer.
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
#include <netdb.h>

/* Utility library. */

#include <vstring.h>

/* DNS library. */

#include "dns.h"

 /*
  * Mapping from error code to printable string. The herror() routine does
  * something similar, but has output only to the stderr stream.
  */
struct dns_error_map {
    unsigned error;
    const char *text;
};

static struct dns_error_map dns_error_map[] = {
    HOST_NOT_FOUND, "Host not found",
    TRY_AGAIN, "Host not found, try again",
    NO_RECOVERY, "Non-recoverable error",
    NO_DATA, "Host found but no data record of requested type",
};

/* dns_strerror - map resolver error code to printable string */

const char *dns_strerror(unsigned error)
{
    static VSTRING *unknown = 0;
    unsigned i;

    for (i = 0; i < sizeof(dns_error_map) / sizeof(dns_error_map[0]); i++)
	if (dns_error_map[i].error == error)
	    return (dns_error_map[i].text);
    if (unknown == 0)
	unknown = vstring_alloc(sizeof("Unknown error XXXXXX"));
    vstring_sprintf(unknown, "Unknown error %u", error);
    return (vstring_str(unknown));
}
