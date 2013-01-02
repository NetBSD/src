/*	$NetBSD: safe_ultostr.c,v 1.1.1.1 2013/01/02 18:59:00 tron Exp $	*/

/*++
/* NAME
/*	safe_ultostr 3
/* SUMMARY
/*	convert unsigned long to safe string
/* SYNOPSIS
/*	#include <safe_ultostr.h>
/*
/*	char	*safe_ultostr(result, ulval, base, padlen, padchar)
/*	VSTRING	*result;
/*	unsigned long ulval;
/*	int	base;
/*	int	padlen;
/*	int	padchar;
/*
/*	unsigned long safe_strtoul(start, end, base)
/*	const char *start;
/*	char **end;
/*	int	base;
/* DESCRIPTION
/*	The functions in this module perform conversions between
/*	unsigned long values and "safe" alphanumerical strings
/*	(strings with digits, uppercase letters and lowercase
/*	letters, but without the vowels AEIOUaeiou). Specifically,
/*	the characters B-Z represent the numbers 10-30, and b-z
/*	represent 31-51.
/*
/*	safe_ultostr() converts an unsigned long value to a safe
/*	alphanumerical string. This is the reverse of safe_strtoul().
/*
/*	safe_strtoul() implements similar functionality as strtoul()
/*	except that it uses a safe alphanumerical string as input,
/*	and that it supports no signs or 0/0x prefixes.
/*
/*	Arguments:
/* .IP result
/*	Buffer for storage of the result of conversion to string.
/* .IP ulval
/*	Unsigned long value.
/* .IP base
/*	Value between 2 and 52 inclusive.
/* .IP padlen
/* .IP padchar
/*	Left-pad a short result with padchar characters to the
/*	specified length.  Specify padlen=0 to disable padding.
/* .IP start
/*	Pointer to the first character of the string to be converted.
/* .IP end
/*	On return, pointer to the first character not in the input
/*	alphabet, or to the string terminator.
/* DIAGNOSTICS
/*	Fatal: out of memory.
/*
/*	safe_strtoul() returns (0, EINVAL) when no conversion could
/*	be performed, and (ULONG_MAX, ERANGE) in case of overflow.
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
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <mymalloc.h>

/* Global library. */

#include <safe_ultostr.h>

/* Application-specific. */

#define STR	vstring_str
#define END	vstring_end
#define SWAP(type, a, b) { type temp; temp = a; a = b; b = temp; }

static unsigned char safe_chars[] =
"0123456789BCDFGHJKLMNPQRSTVWXYZbcdfghjklmnpqrstvwxyz";

#define SAFE_MAX_BASE	(sizeof(safe_chars) - 1)
#define SAFE_MIN_BASE	(2)

/* safe_ultostr - convert unsigned long to safe alphanumerical string */

char   *safe_ultostr(VSTRING *buf, unsigned long ulval, int base,
		               int padlen, int padchar)
{
    const char *myname = "safe_ultostr";
    char   *start;
    char   *last;
    int     i;

    /*
     * Sanity check.
     */
    if (base < SAFE_MIN_BASE || base > SAFE_MAX_BASE)
	msg_panic("%s: bad base: %d", myname, base);

    /*
     * First accumulate the result, backwards.
     */
    VSTRING_RESET(buf);
    while (ulval != 0) {
	VSTRING_ADDCH(buf, safe_chars[ulval % base]);
	ulval /= base;
    }
    while (VSTRING_LEN(buf) < padlen)
	VSTRING_ADDCH(buf, padchar);
    VSTRING_TERMINATE(buf);

    /*
     * Then, reverse the result.
     */
    start = STR(buf);
    last = END(buf) - 1;
    for (i = 0; i < VSTRING_LEN(buf) / 2; i++)
	SWAP(int, start[i], last[-i]);
    return (STR(buf));
}

/* safe_strtoul - convert safe alphanumerical string to unsigned long */

unsigned long safe_strtoul(const char *start, char **end, int base)
{
    const char *myname = "safe_strtoul";
    static unsigned char *char_map = 0;
    unsigned char *cp;
    unsigned long sum;
    unsigned long div_limit;
    unsigned long mod_limit;
    int     char_val;

    /*
     * Sanity check.
     */
    if (base < SAFE_MIN_BASE || base > SAFE_MAX_BASE)
	msg_panic("%s: bad base: %d", myname, base);

    /*
     * One-time initialization. Assume 8-bit bytes.
     */
    if (char_map == 0) {
	char_map = (unsigned char *) mymalloc(256);
	for (char_val = 0; char_val < 256; char_val++)
	    char_map[char_val] = SAFE_MAX_BASE;
	for (char_val = 0; char_val < SAFE_MAX_BASE; char_val++)
	    char_map[safe_chars[char_val]] = char_val;
    }

    /*
     * Per-call initialization.
     */
    sum = 0;
    div_limit = ULONG_MAX / base;
    mod_limit = ULONG_MAX % base;

    /*
     * Skip leading whitespace. We don't implement sign/base prefixes.
     */
    while (ISSPACE(*start))
	++start;

    /*
     * Start the conversion.
     */
    errno = 0;
    for (cp = (unsigned char *) start; *cp; cp++) {
	/* Return (0, EINVAL) if no conversion was made. */
	if ((char_val = char_map[*cp]) >= base) {
	    if (cp == (unsigned char *) start)
		errno = EINVAL;
	    break;
	}
	/* Return (ULONG_MAX, ERANGE) if the result is too large. */
	if (sum > div_limit
	    || (sum == div_limit && char_val > mod_limit)) {
	    sum = ULONG_MAX;
	    errno = ERANGE;
	    /* Skip "valid" characters, per the strtoul() spec. */
	    while (char_map[*++cp] < base)
		 /* void */ ;
	    break;
	}
	sum = sum * base + char_val;
    }
    if (end)
	*end = (char *) cp;
    return (sum);
}

#ifdef TEST

 /*
  * Proof-of-concept test program. Read a number from stdin, convert to
  * string, and print the result.
  */
#include <stdio.h>			/* sscanf */
#include <vstream.h>
#include <vstring_vstream.h>

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *buf = vstring_alloc(100);
    char   *junk;
    unsigned long ulval;
    int     base;
    char    ch;
    unsigned long ulval2;

#ifdef MISSING_STRTOUL
#define strtoul strtol
#endif

    while (vstring_get_nonl(buf, VSTREAM_IN) != VSTREAM_EOF) {
	ch = 0;
	if (sscanf(STR(buf), "%lu %d%c", &ulval, &base, &ch) != 2 || ch) {
	    msg_warn("bad input %s", STR(buf));
	} else {
	    (void) safe_ultostr(buf, ulval, base, 5, '0');
	    vstream_printf("%lu = %s\n", ulval, STR(buf));
	    ulval2 = safe_strtoul(STR(buf), &junk, base);
	    if (*junk || (ulval2 == ULONG_MAX && errno == ERANGE))
		msg_warn("%s: %m", STR(buf));
	    if (ulval2 != ulval)
		msg_warn("%lu != %lu", ulval2, ulval);
	}
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(buf);
    return (0);
}

#endif
