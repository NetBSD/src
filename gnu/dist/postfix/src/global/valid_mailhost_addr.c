/*++
/* NAME
/*	valid_mailhost_addr 3
/* SUMMARY
/*	mailhost address syntax validation
/* SYNOPSIS
/*	#include <valid_mailhost_addr.h>
/*
/*	const char *valid_mailhost_addr(name, gripe)
/*	const char *name;
/*	int	gripe;
/*
/*	int	valid_mailhost_literal(addr, gripe)
/*	const char *addr;
/*	int	gripe;
/* DESCRIPTION
/*	valid_mailhost_addr() requires that the input is a valid
/*	RFC 2821 string representation of an IPv4 or IPv6 network
/*	address.  A valid IPv4 address is in dotted quad decimal
/*	form.  A valid IPv6 address includes the "IPV6:" prefix as
/*	required by RFC 2821, and is in valid hexadecimal form or
/*	in valid IPv4-in-IPv6 form.  The result value is the bare
/*	address in the input argument (i.e. text after "IPV6:"
/*	prefix, if any) in case of success, a null pointer in case
/*	of failure.
/*
/*	valid_mailhost_literal() requires an address enclosed in
/*	[].  The result is non-zero in case of success, zero in
/*	case of failure.
/*
/*	These routines operate silently unless the gripe parameter
/*	specifies a non-zero value. The macros DO_GRIPE and DONT_GRIPE
/*	provide suitable constants.
/*
/*	The IPV6_COL macro defines the "IPv6:" prefix.
/* DIAGNOSTICS
/*	Warnings are logged with msg_warn().
/* SEE ALSO
/*	valid_hostname(3)
/*	RFC 952, RFC 1123, RFC 1035, RFC 2821
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

#include <msg.h>
#include <myaddrinfo.h>

/* Global library. */

#include <valid_mailhost_addr.h>

/* Application-specific. */

#define IPV6_COL_LEN       (sizeof(IPV6_COL) - 1)
#define HAS_IPV6_COL(str)  (strncasecmp((str), IPV6_COL, IPV6_COL_LEN) == 0)
#define SKIP_IPV6_COL(str) (HAS_IPV6_COL(str) ? (str) + IPV6_COL_LEN : (str))

/* valid_mailhost_addr - validate RFC 2821 numerical address form */

const char *valid_mailhost_addr(const char *addr, int gripe)
{
    const char *bare_addr;

    bare_addr = SKIP_IPV6_COL(addr);
    return ((bare_addr != addr ? valid_ipv6_hostaddr : valid_ipv4_hostaddr)
	    (bare_addr, gripe) ? bare_addr : 0);
}

/* valid_mailhost_literal - validate [RFC 2821 numerical address] form */

int     valid_mailhost_literal(const char *addr, int gripe)
{
    const char *myname = "valid_mailhost_literal";
    MAI_HOSTADDR_STR hostaddr;
    const char *last;
    size_t address_bytes;

    if (*addr != '[') {
	if (gripe)
	    msg_warn("%s: '[' expected at start: %.100s", myname, addr);
	return (0);
    }
    if ((last = strchr(addr, ']')) == 0) {
	if (gripe)
	    msg_warn("%s: ']' expected at end: %.100s", myname, addr);
	return (0);
    }
    if (last[1]) {
	if (gripe)
	    msg_warn("%s: unexpected text after ']': %.100s", myname, addr);
	return (0);
    }
    if ((address_bytes = last - addr - 1) >= sizeof(hostaddr.buf)) {
	if (gripe)
	    msg_warn("%s: too much text: %.100s", myname, addr);
	return (0);
    }
    strncpy(hostaddr.buf, addr + 1, address_bytes);
    hostaddr.buf[address_bytes] = 0;
    return (valid_mailhost_addr(hostaddr.buf, gripe) != 0);
}

#ifdef TEST

 /*
  * Test program - reads hostnames from stdin, reports invalid hostnames to
  * stderr.
  */
#include <stdlib.h>

#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <msg_vstream.h>

int     main(int unused_argc, char **argv)
{
    VSTRING *buffer = vstring_alloc(1);

    msg_vstream_init(argv[0], VSTREAM_ERR);
    msg_verbose = 1;

    while (vstring_fgets_nonl(buffer, VSTREAM_IN)) {
	msg_info("testing: \"%s\"", vstring_str(buffer));
	if (vstring_str(buffer)[0] == '[')
	    valid_mailhost_literal(vstring_str(buffer), DO_GRIPE);
	else
	    valid_mailhost_addr(vstring_str(buffer), DO_GRIPE);
    }
    exit(0);
}

#endif
