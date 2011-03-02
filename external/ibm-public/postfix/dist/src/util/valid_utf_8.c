/*	$NetBSD: valid_utf_8.c,v 1.1.1.1 2011/03/02 19:32:46 tron Exp $	*/

/*++
/* NAME
/*	valid_utf_8 3
/* SUMMARY
/*	predicate if string is valid UTF-8
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	int	valid_utf_8(str, len)
/*	const char *str;
/*	ssize_t	len;
/* DESCRIPTION
/*	valid_utf_8() determines if a string satisfies the UTF-8
/*	definition in RFC 3629. That is, it contains proper encodings
/*	of code points U+0000..U+10FFFF, excluding over-long encodings
/*	and excluding U+D800..U+DFFF surrogates.
/*
/*	A zero-length string is considered valid.
/* DIAGNOSTICS
/*	The result value is zero when the caller specifies a negative
/*	length, or a string that violates RFC 3629, for example a
/*	string that is truncated in the middle of a multi-byte
/*	sequence.
/* BUGS
/*	But wait, there is more. Code points in the range U+FDD0..U+FDEF
/*	and ending in FFFE or FFFF are non-characters in UNICODE. This
/*	function does not block these.
/* SEE ALSO
/*	RFC 3629
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

/* Utility library. */

#include <stringops.h>

/* valid_utf_8 - validate string according to RFC 3629 */

int     valid_utf_8(const char *str, ssize_t len)
{
    const unsigned char *end = (const unsigned char *) str + len;
    const unsigned char *cp;
    unsigned char c0, ch;

    if (len < 0)
	return (0);
    if (len <= 0)
	return (1);

    /*
     * Optimized for correct input, time, space, and for CPUs that have a
     * decent number of registers.
     */
    for (cp = (const unsigned char *) str; cp < end; cp++) {
	/* Single-byte encodings. */
	if (EXPECTED((c0 = *cp) <= 0x7f) /* we know that c0 >= 0x0 */ ) {
	     /* void */ ;
	}
	/* Two-byte encodings. */
	else if (EXPECTED(c0 <= 0xdf) /* we know that c0 >= 0x80 */ ) {
	    /* Exclude over-long encodings. */
	    if (UNEXPECTED(c0 < 0xc2)
		|| UNEXPECTED(cp + 1 >= end)
	    /* Require UTF-8 tail byte. */
		|| UNEXPECTED((ch = *++cp) < 0x80) || UNEXPECTED(ch > 0xbf))
		return (0);
	}
	/* Three-byte encodings. */
	else if (EXPECTED(c0 <= 0xef) /* we know that c0 >= 0xe0 */ ) {
	    if (UNEXPECTED(cp + 2 >= end)
	    /* Exclude over-long encodings. */
		|| UNEXPECTED((ch = *++cp) < (c0 == 0xe0 ? 0xa0 : 0x80))
	    /* Exclude U+D800..U+DFFF. */
		|| UNEXPECTED(ch > (c0 == 0xed ? 0x9f : 0xbf))
	    /* Require UTF-8 tail byte. */
		|| UNEXPECTED((ch = *++cp) < 0x80) || UNEXPECTED(ch > 0xbf))
		return (0);
	}
	/* Four-byte encodings. */
	else if (EXPECTED(c0 <= 0xf4) /* we know that c0 >= 0xf0 */ ) {
	    if (UNEXPECTED(cp + 3 >= end)
	    /* Exclude over-long encodings. */
		|| UNEXPECTED((ch = *++cp) < (c0 == 0xf0 ? 0x90 : 0x80))
	    /* Exclude code points above U+10FFFF. */
		|| UNEXPECTED(ch > (c0 == 0xf4 ? 0x8f : 0xbf))
	    /* Require UTF-8 tail byte. */
		|| UNEXPECTED((ch = *++cp) < 0x80) || UNEXPECTED(ch > 0xbf)
	    /* Require UTF-8 tail byte. */
		|| UNEXPECTED((ch = *++cp) < 0x80) || UNEXPECTED(ch > 0xbf))
		return (0);
	}
	/* Invalid: c0 >= 0xf5 */
	else {
	    return (0);
	}
    }
    return (1);
}

 /*
  * Stand-alone test program. Each string is a line without line terminator.
  */
#ifdef TEST
#include <stdlib.h>
#include <vstream.h>
#include <vstring.h>
#include <vstring_vstream.h>

#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)

int     main(void)
{
    VSTRING *buf = vstring_alloc(1);

    while (vstring_get_nonl(buf, VSTREAM_IN) != VSTREAM_EOF) {
	vstream_printf("%c", (LEN(buf) && !valid_utf_8(STR(buf), LEN(buf))) ?
		       '!' : ' ');
	vstream_fwrite(VSTREAM_OUT, STR(buf), LEN(buf));
	vstream_printf("\n");
    }
    vstream_fflush(VSTREAM_OUT);
    vstring_free(buf);
    exit(0);
}

#endif
