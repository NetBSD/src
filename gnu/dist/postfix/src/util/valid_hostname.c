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
/* DESCRIPTION
/*	valid_hostname() scrutinizes a hostname: the name should be no
/*	longer than VALID_HOSTNAME_LEN characters, should contain only
/*	letters, digits, dots and hyphens, no adjacent dots and hyphens,
/*	no leading or trailing dots or hyphens, no labels longer than
/*	VALID_LABEL_LEN characters, and no numeric top-level domain.
/*
/*	valid_hostaddr() requirs that the input is a valid string
/*	representation of an internet network address.
/*
/*	These routines operate silently unless the gripe parameter
/*	specifies a non-zero value. The macros DO_GRIPE and DONT_GRIPE
/*	provide suitable constants.
/* DIAGNOSTICS
/*	Both functions return zero if they disagree with the input.
/* SEE ALSO
/*	RFC 952, 1123, RFC 1035
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
    char   *myname = "valid_hostname";
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
	} else {
	    if (gripe)
		msg_warn("%s: invalid character %d(decimal): %.100s",
			 myname, ch, name);
	    return (0);
	}
    }

    if (non_numeric == 0) {
	if (gripe)
	    msg_warn("%s: numeric hostname: %.100s", myname, name);
	/* NOT: return (0); this confuses users of the DNS client */
    }
    if (cp - name > VALID_HOSTNAME_LEN) {
	if (gripe)
	    msg_warn("%s: bad length %d for %.100s...",
		     myname, (int) (cp - name), name);
	return (0);
    }
    return (1);
}

/* valid_hostaddr - test dotted quad string for correctness */

int     valid_hostaddr(const char *addr, int gripe)
{
    const char *cp;
    char   *myname = "valid_hostaddr";
    int     in_byte = 0;
    int     byte_count = 0;
    int     byte_val = 0;
    int     ch;

#define BYTES_NEEDED	4

    /*
     * Trivial cases first.
     */
    if (*addr == 0) {
	if (gripe)
	    msg_warn("%s: empty address", myname);
	return (0);
    }

    /*
     * Scary code to avoid sscanf() overflow nasties.
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
	    if ((byte_count == 1 && byte_val == 0)) {
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
