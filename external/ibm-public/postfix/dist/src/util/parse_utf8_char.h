/*	$NetBSD: parse_utf8_char.h,v 1.2 2025/02/25 19:15:52 christos Exp $	*/

/*++
/* NAME
/*	parse_utf8_char 3h
/* SUMMARY
/*	parse one UTF-8 multibyte character
/* SYNOPSIS
/*	#include <parse_utf8_char.h>
/*
/*	char	*parse_utf8_char(str, end)
/*	const char *str;
/*	const char *end;
/* DESCRIPTION
/*	parse_utf8_char() determines if the byte sequence starting
/*	at \fBstr\fR begins with a complete UTF-8 character as
/*	defined in RFC 3629. That is, a proper encoding of code
/*	points U+0000..U+10FFFF, excluding over-long encodings and
/*	excluding U+D800..U+DFFF surrogates.
/*
/*	When the byte sequence starting at \fBstr\fR begins with a
/*	complete UTF-8 character, this function returns a pointer
/*	to the last byte in that character. Otherwise, it returns
/*	a null pointer.
/*
/*	The \fBend\fR argument is either null (the byte sequence
/*	starting at \fBstr\fR must be null terminated), or \fBend
/*	- str\fR specifies the length of the byte sequence.
/* BUGS
/*	Code points in the range U+FDD0..U+FDEF and ending in FFFE
/*	or FFFF are non-characters in UNICODE. This function does
/*	not reject these.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	porcupine.org
/*	Amawalk, NY 10501, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>

#ifdef NO_INLINE
#define inline				/* */
#endif

/* parse_utf8_char - parse and validate one UTF8 multibyte sequence */

static inline char *parse_utf8_char(const char *str, const char *end)
{
    const unsigned char *cp = (const unsigned char *) str;
    const unsigned char *ep = (const unsigned char *) end;
    unsigned char c0, ch;

    /*
     * Optimized for correct input, time, space, and for CPUs that have a
     * decent number of registers. Other implementation considerations:
     * 
     * - In the UTF-8 encoding, a non-leading byte is never null. Therefore,
     * this function will correctly reject a partial UTF-8 character at the
     * end of a null-terminated string.
     * 
     * - If the "end" argument is a null constant, and if this function is
     * inlined, then an optimizing compiler should propagate the constant
     * through the "ep" variable, and eliminate any code branches that
     * require ep != 0.
     */
    /* Single-byte encodings. */
    if (EXPECTED((c0 = *cp) <= 0x7f) /* we know that c0 >= 0x0 */ ) {
	return ((char *) cp);
    }
    /* Two-byte encodings. */
    else if (EXPECTED(c0 <= 0xdf) /* we know that c0 >= 0x80 */ ) {
	/* Exclude over-long encodings. */
	if (UNEXPECTED(c0 < 0xc2)
	    || UNEXPECTED(ep && cp + 1 >= ep)
	/* Require UTF-8 tail byte. */
	    || UNEXPECTED(((ch = *++cp) & 0xc0) != 0x80))
	    return (0);
	return ((char *) cp);
    }
    /* Three-byte encodings. */
    else if (EXPECTED(c0 <= 0xef) /* we know that c0 >= 0xe0 */ ) {
	if (UNEXPECTED(ep && cp + 2 >= ep)
	/* Exclude over-long encodings. */
	    || UNEXPECTED((ch = *++cp) < (c0 == 0xe0 ? 0xa0 : 0x80))
	/* Exclude U+D800..U+DFFF. */
	    || UNEXPECTED(ch > (c0 == 0xed ? 0x9f : 0xbf))
	/* Require UTF-8 tail byte. */
	    || UNEXPECTED(((ch = *++cp) & 0xc0) != 0x80))
	    return (0);
	return ((char *) cp);
    }
    /* Four-byte encodings. */
    else if (EXPECTED(c0 <= 0xf4) /* we know that c0 >= 0xf0 */ ) {
	if (UNEXPECTED(ep && cp + 3 >= ep)
	/* Exclude over-long encodings. */
	    || UNEXPECTED((ch = *++cp) < (c0 == 0xf0 ? 0x90 : 0x80))
	/* Exclude code points above U+10FFFF. */
	    || UNEXPECTED(ch > (c0 == 0xf4 ? 0x8f : 0xbf))
	/* Require UTF-8 tail byte. */
	    || UNEXPECTED(((ch = *++cp) & 0xc0) != 0x80)
	/* Require UTF-8 tail byte. */
	    || UNEXPECTED(((ch = *++cp) & 0xc0) != 0x80))
	    return (0);
	return ((char *) cp);
    }
    /* Invalid: c0 >= 0xf5 */
    else {
	return (0);
    }
}

#undef inline
