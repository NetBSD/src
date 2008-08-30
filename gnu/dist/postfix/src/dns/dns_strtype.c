/*++
/* NAME
/*	dns_strtype 3
/* SUMMARY
/*	name service lookup type codes and printable forms
/* SYNOPSIS
/*	#include <dns.h>
/*
/*	const char *dns_strtype(code)
/*	int	code;
/*
/*	int	dns_type(strval)
/*	const char *strval;
/* DESCRIPTION
/*	dns_strtype() maps a name service lookup type to printable string.
/*	The result is for read-only purposes, and unknown codes share a
/*	common string buffer.
/*
/*	dns_type() converts a name service lookup string value to a numeric
/*	code. A null result means the code was not found. The input can be
/*	in lower case, upper case or mixed case.
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

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <vstring.h>

/* DNS library. */

#include "dns.h"

 /*
  * Mapping from type code to printable string. Some names are possibly not
  * defined on every platform, so I have #ifdef-ed them all just to be safe.
  */
struct dns_type_map {
    unsigned type;
    const char *text;
};

static struct dns_type_map dns_type_map[] = {
#ifdef T_A
    T_A, "A",
#endif
#ifdef T_AAAA
    T_AAAA, "AAAA",
#endif
#ifdef T_NS
    T_NS, "NS",
#endif
#ifdef T_MD
    T_MD, "MD",
#endif
#ifdef T_MF
    T_MF, "MF",
#endif
#ifdef T_CNAME
    T_CNAME, "CNAME",
#endif
#ifdef T_SOA
    T_SOA, "SOA",
#endif
#ifdef T_MB
    T_MB, "MB",
#endif
#ifdef T_MG
    T_MG, "MG",
#endif
#ifdef T_MR
    T_MR, "MR",
#endif
#ifdef T_NULL
    T_NULL, "NULL",
#endif
#ifdef T_WKS
    T_WKS, "WKS",
#endif
#ifdef T_PTR
    T_PTR, "PTR",
#endif
#ifdef T_HINFO
    T_HINFO, "HINFO",
#endif
#ifdef T_MINFO
    T_MINFO, "MINFO",
#endif
#ifdef T_MX
    T_MX, "MX",
#endif
#ifdef T_TXT
    T_TXT, "TXT",
#endif
#ifdef T_RP
    T_RP, "RP",
#endif
#ifdef T_AFSDB
    T_AFSDB, "AFSDB",
#endif
#ifdef T_X25
    T_X25, "X25",
#endif
#ifdef T_ISDN
    T_ISDN, "ISDN",
#endif
#ifdef T_RT
    T_RT, "RT",
#endif
#ifdef T_NSAP
    T_NSAP, "NSAP",
#endif
#ifdef T_NSAP_PTR
    T_NSAP_PTR, "NSAP_PTR",
#endif
#ifdef T_SIG
    T_SIG, "SIG",
#endif
#ifdef T_KEY
    T_KEY, "KEY",
#endif
#ifdef T_PX
    T_PX, "PX",
#endif
#ifdef T_GPOS
    T_GPOS, "GPOS",
#endif
#ifdef T_AAAA
    T_AAAA, "AAAA",
#endif
#ifdef T_LOC
    T_LOC, "LOC",
#endif
#ifdef T_UINFO
    T_UINFO, "UINFO",
#endif
#ifdef T_UID
    T_UID, "UID",
#endif
#ifdef T_GID
    T_GID, "GID",
#endif
#ifdef T_UNSPEC
    T_UNSPEC, "UNSPEC",
#endif
#ifdef T_AXFR
    T_AXFR, "AXFR",
#endif
#ifdef T_MAILB
    T_MAILB, "MAILB",
#endif
#ifdef T_MAILA
    T_MAILA, "MAILA",
#endif
#ifdef T_ANY
    T_ANY, "ANY",
#endif
};

/* dns_strtype - translate DNS query type to string */

const char *dns_strtype(unsigned type)
{
    static VSTRING *unknown = 0;
    unsigned i;

    for (i = 0; i < sizeof(dns_type_map) / sizeof(dns_type_map[0]); i++)
	if (dns_type_map[i].type == type)
	    return (dns_type_map[i].text);
    if (unknown == 0)
	unknown = vstring_alloc(sizeof("Unknown type XXXXXX"));
    vstring_sprintf(unknown, "Unknown type %u", type);
    return (vstring_str(unknown));
}

/* dns_type - translate string to DNS query type */

unsigned dns_type(const char *text)
{
    unsigned i;

    for (i = 0; i < sizeof(dns_type_map) / sizeof(dns_type_map[0]); i++)
	if (strcasecmp(dns_type_map[i].text, text) == 0)
	    return (dns_type_map[i].type);
    return (0);
}
