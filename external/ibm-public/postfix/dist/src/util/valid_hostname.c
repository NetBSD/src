/*	$NetBSD: valid_hostname.c,v 1.1.1.1 2009/06/23 10:09:01 tron Exp $	*/

/*++
/* NAME
/*	valid_hostname 3
/* SUMMARY
/*	network name validation
/* SYNOPSIS
/*	#include <valid_hostname.h>
/*
/*	int	valid_hostname(name, gripe)
/*	const char *name;
/*	int	gripe;
/*
/*	int	valid_hostaddr(addr, gripe)
/*	const char *addr;
/*	int	gripe;
/*
/*	int	valid_ipv4_hostaddr(addr, gripe)
/*	const char *addr;
/*	int	gripe;
/*
/*	int	valid_ipv6_hostaddr(addr, gripe)
/*	const char *addr;
/*	int	gripe;
/* DESCRIPTION
/*	valid_hostname() scrutinizes a hostname: the name should
/*	be no longer than VALID_HOSTNAME_LEN characters, should
/*	contain only letters, digits, dots and hyphens, no adjacent
/*	dots and hyphens, no leading or trailing dots or hyphens,
/*	no labels longer than VALID_LABEL_LEN characters, and it
/*	should not be all numeric.
/*
/*	valid_hostaddr() requires that the input is a valid string
/*	representation of an IPv4 or IPv6 network address as
/*	described next.
/*
/*	valid_ipv4_hostaddr() and valid_ipv6_hostaddr() implement
/*	protocol-specific address syntax checks. A valid IPv4
/*	address is in dotted-quad decimal form. A valid IPv6 address
/*      has 16-bit hexadecimal fields separated by ":", and does not
/*      include the RFC 2821 style "IPv6:" prefix.
/*
/*	These routines operate silently unless the gripe parameter
/*	specifies a non-zero value. The macros DO_GRIPE and DONT_GRIPE
/*	provide suitable constants.
/* BUGS
/*	valid_hostmumble() does not guarantee that string lengths
/*	fit the buffer sizes defined in myaddrinfo(3h).
/* DIAGNOSTICS
/*	All functions return zero if they disagree with the input.
/* SEE ALSO
/*	RFC 952, RFC 1123, RFC 1035, RFC 2373.
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
#include <ctype.h>

/* Utility library. */

#include "msg.h"
#include "mymalloc.h"
#include "stringops.h"
#include "valid_hostname.h"

/* valid_hostname - screen out bad hostnames */

int     valid_hostname(const char *name, int gripe)
{
    const char *myname = "valid_hostname";
    const char *cp;
    int     label_length = 0;
    int     label_count = 0;
    int     non_numeric = 0;
    int     ch;

    /*
     * Trivial cases first.
     */
    if (*name == 0) {
	if (gripe)
	    msg_warn("%s: empty hostname", myname);
	return (0);
    }

    /*
     * Find bad characters or label lengths. Find adjacent delimiters.
     */
    for (cp = name; (ch = *(unsigned char *) cp) != 0; cp++) {
	if (ISALNUM(ch) || ch == '_') {		/* grr.. */
	    if (label_length == 0)
		label_count++;
	    label_length++;
	    if (label_length > VALID_LABEL_LEN) {
		if (gripe)
		    msg_warn("%s: hostname label too long: %.100s", myname, name);
		return (0);
	    }
	    if (!ISDIGIT(ch))
		non_numeric = 1;
	} else if (ch == '.') {
	    if (label_length == 0 || cp[1] == 0) {
		if (gripe)
		    msg_warn("%s: misplaced delimiter: %.100s", myname, name);
		return (0);
	    }
	    label_length = 0;
	} else if (ch == '-') {
	    label_length++;
	    if (label_length == 1 || cp[1] == 0 || cp[1] == '.') {
		if (gripe)
		    msg_warn("%s: misplaced hyphen: %.100s", myname, name);
		return (0);
	    }
	}
#ifdef SLOPPY_VALID_HOSTNAME
	else if (ch == ':' && valid_ipv6_hostaddr(name, DONT_GRIPE)) {
	    non_numeric = 0;
	    break;
	}
#endif
	else {
	    if (gripe)
		msg_warn("%s: invalid character %d(decimal): %.100s",
			 myname, ch, name);
	    return (0);
	}
    }

    if (non_numeric == 0) {
	if (gripe)
	    msg_warn("%s: numeric hostname: %.100s", myname, name);
#ifndef SLOPPY_VALID_HOSTNAME
	return (0);
#endif
    }
    if (cp - name > VALID_HOSTNAME_LEN) {
	if (gripe)
	    msg_warn("%s: bad length %d for %.100s...",
		     myname, (int) (cp - name), name);
	return (0);
    }
    return (1);
}

/* valid_hostaddr - verify numerical address syntax */

int     valid_hostaddr(const char *addr, int gripe)
{
    const char *myname = "valid_hostaddr";

    /*
     * Trivial cases first.
     */
    if (*addr == 0) {
	if (gripe)
	    msg_warn("%s: empty address", myname);
	return (0);
    }

    /*
     * Protocol-dependent processing next.
     */
    if (strchr(addr, ':') != 0)
	return (valid_ipv6_hostaddr(addr, gripe));
    else
	return (valid_ipv4_hostaddr(addr, gripe));
}

/* valid_ipv4_hostaddr - test dotted quad string for correctness */

int     valid_ipv4_hostaddr(const char *addr, int gripe)
{
    const char *cp;
    const char *myname = "valid_ipv4_hostaddr";
    int     in_byte = 0;
    int     byte_count = 0;
    int     byte_val = 0;
    int     ch;

#define BYTES_NEEDED	4

    /*
     * Scary code to avoid sscanf() overflow nasties.
     * 
     * This routine is called by valid_ipv6_hostaddr(). It must not call that
     * routine, to avoid deadly recursion.
     */
    for (cp = addr; (ch = *(unsigned const char *) cp) != 0; cp++) {
	if (ISDIGIT(ch)) {
	    if (in_byte == 0) {
		in_byte = 1;
		byte_val = 0;
		byte_count++;
	    }
	    byte_val *= 10;
	    byte_val += ch - '0';
	    if (byte_val > 255) {
		if (gripe)
		    msg_warn("%s: invalid octet value: %.100s", myname, addr);
		return (0);
	    }
	} else if (ch == '.') {
	    if (in_byte == 0 || cp[1] == 0) {
		if (gripe)
		    msg_warn("%s: misplaced dot: %.100s", myname, addr);
		return (0);
	    }
	    /* XXX Allow 0.0.0.0 but not 0.1.2.3 */
	    if (byte_count == 1 && byte_val == 0 && addr[strspn(addr, "0.")]) {
		if (gripe)
		    msg_warn("%s: bad initial octet value: %.100s", myname, addr);
		return (0);
	    }
	    in_byte = 0;
	} else {
	    if (gripe)
		msg_warn("%s: invalid character %d(decimal): %.100s",
			 myname, ch, addr);
	    return (0);
	}
    }

    if (byte_count != BYTES_NEEDED) {
	if (gripe)
	    msg_warn("%s: invalid octet count: %.100s", myname, addr);
	return (0);
    }
    return (1);
}

/* valid_ipv6_hostaddr - validate IPv6 address syntax */

int     valid_ipv6_hostaddr(const char *addr, int gripe)
{
    const char *myname = "valid_ipv6_hostaddr";
    int     null_field = 0;
    int     field = 0;
    unsigned char *cp = (unsigned char *) addr;
    int     len = 0;

    /*
     * FIX 200501 The IPv6 patch validated syntax with getaddrinfo(), but I
     * am not confident that everyone's system library routines are robust
     * enough, like buffer overflow free. Remember, the valid_hostmumble()
     * routines are meant to protect Postfix against malformed information in
     * data received from the network.
     * 
     * We require eight-field hex addresses of the form 0:1:2:3:4:5:6:7,
     * 0:1:2:3:4:5:6a.6b.7c.7d, or some :: compressed version of the same.
     * 
     * Note: the character position is advanced inside the loop. I have added
     * comments to show why we can't get stuck.
     */
    for (;;) {
	switch (*cp) {
	case 0:
	    /* Terminate the loop. */
	    if (field < 2) {
		if (gripe)
		    msg_warn("%s: too few `:' in IPv6 address: %.100s",
			     myname, addr);
		return (0);
	    } else if (len == 0 && null_field != field - 1) {
		if (gripe)
		    msg_warn("%s: bad null last field in IPv6 address: %.100s",
			     myname, addr);
		return (0);
	    } else
		return (1);
	case '.':
	    /* Terminate the loop. */
	    if (field < 2 || field > 6) {
		if (gripe)
		    msg_warn("%s: malformed IPv4-in-IPv6 address: %.100s",
			     myname, addr);
		return (0);
	    } else
		/* NOT: valid_hostaddr(). Avoid recursion. */
		return (valid_ipv4_hostaddr((char *) cp - len, gripe));
	case ':':
	    /* Advance by exactly 1 character position or terminate. */
	    if (field == 0 && len == 0 && ISALNUM(cp[1])) {
		if (gripe)
		    msg_warn("%s: bad null first field in IPv6 address: %.100s",
			     myname, addr);
		return (0);
	    }
	    field++;
	    if (field > 7) {
		if (gripe)
		    msg_warn("%s: too many `:' in IPv6 address: %.100s",
			     myname, addr);
		return (0);
	    }
	    cp++;
	    len = 0;
	    if (*cp == ':') {
		if (null_field > 0) {
		    if (gripe)
			msg_warn("%s: too many `::' in IPv6 address: %.100s",
				 myname, addr);
		    return (0);
		}
		null_field = field;
	    }
	    break;
	default:
	    /* Advance by at least 1 character position or terminate. */
	    len = strspn((char *) cp, "0123456789abcdefABCDEF");
	    if (len /* - strspn((char *) cp, "0") */ > 4) {
		if (gripe)
		    msg_warn("%s: malformed IPv6 address: %.100s",
			     myname, addr);
		return (0);
	    }
	    if (len <= 0) {
		if (gripe)
		    msg_warn("%s: invalid character %d(decimal) in IPv6 address: %.100s",
			     myname, *cp, addr);
		return (0);
	    }
	    cp += len;
	    break;
	}
    }
}

#ifdef TEST

 /*
  * Test program - reads hostnames from stdin, reports invalid hostnames to
  * stderr.
  */
#include <stdlib.h>

#include "vstring.h"
#include "vstream.h"
#include "vstring_vstream.h"
#include "msg_vstream.h"

int     main(int unused_argc, char **argv)
{
    VSTRING *buffer = vstring_alloc(1);

    msg_vstream_init(argv[0], VSTREAM_ERR);
    msg_verbose = 1;

    while (vstring_fgets_nonl(buffer, VSTREAM_IN)) {
	msg_info("testing: \"%s\"", vstring_str(buffer));
	valid_hostname(vstring_str(buffer), DO_GRIPE);
	valid_hostaddr(vstring_str(buffer), DO_GRIPE);
    }
    exit(0);
}

#endif
