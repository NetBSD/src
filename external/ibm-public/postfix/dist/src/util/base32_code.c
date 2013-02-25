/*	$NetBSD: base32_code.c,v 1.1.1.1.6.2 2013/02/25 00:27:31 tls Exp $	*/

/*++
/* NAME
/*	base32_code 3
/* SUMMARY
/*	encode/decode data, base 32 style
/* SYNOPSIS
/*	#include <base32_code.h>
/*
/*	VSTRING	*base32_encode(result, in, len)
/*	VSTRING	*result;
/*	const char *in;
/*	ssize_t	len;
/*
/*	VSTRING	*base32_decode(result, in, len)
/*	VSTRING	*result;
/*	const char *in;
/*	ssize_t	len;
/* DESCRIPTION
/*	base32_encode() takes a block of len bytes and encodes it as one
/*	null-terminated string.  The result value is the result argument.
/*
/*	base32_decode() performs the opposite transformation. The result
/*	value is the result argument. The result is null terminated, whether
/*	or not that makes sense.
/* DIAGNOSTICS
/*	base32_decode() returns a null pointer when the input contains
/*	characters not in the base 32 alphabet.
/* SEE ALSO
/*	RFC 4648; padding is strictly enforced
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

#include "sys_defs.h"
#include <ctype.h>
#include <string.h>
#include <limits.h>

#ifndef UCHAR_MAX
#define UCHAR_MAX 0xff
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <base32_code.h>

/* Application-specific. */

static unsigned char to_b32[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

#define UNSIG_CHAR_PTR(x) ((unsigned char *)(x))

/* base32_encode - raw data to encoded */

VSTRING *base32_encode(VSTRING *result, const char *in, ssize_t len)
{
    const unsigned char *cp;
    ssize_t count;
    static int pad_count[] = {0, 6, 4, 3, 1};

    /*
     * Encode 5 -> 8.
     */
    VSTRING_RESET(result);
    for (cp = UNSIG_CHAR_PTR(in), count = len; count > 0; count -= 5, cp += 5) {
	VSTRING_ADDCH(result, to_b32[cp[0] >> 3]);
	if (count < 2) {
	    VSTRING_ADDCH(result, to_b32[(cp[0] & 0x7) << 2]);
	    break;
	}
	VSTRING_ADDCH(result, to_b32[(cp[0] & 0x7) << 2 | cp[1] >> 6]);
	VSTRING_ADDCH(result, to_b32[(cp[1] & 0x3f) >> 1]);
	if (count < 3) {
	    VSTRING_ADDCH(result, to_b32[(cp[1] & 0x1) << 4]);
	    break;
	}
	VSTRING_ADDCH(result, to_b32[(cp[1] & 0x1) << 4 | cp[2] >> 4]);
	if (count < 4) {
	    VSTRING_ADDCH(result, to_b32[(cp[2] & 0xf) << 1]);
	    break;
	}
	VSTRING_ADDCH(result, to_b32[(cp[2] & 0xf) << 1 | cp[3] >> 7]);
	VSTRING_ADDCH(result, to_b32[(cp[3] & 0x7f) >> 2]);
	if (count < 5) {
	    VSTRING_ADDCH(result, to_b32[(cp[3] & 0x3) << 3]);
	    break;
	}
	VSTRING_ADDCH(result, to_b32[(cp[3] & 0x3) << 3 | cp[4] >> 5]);
	VSTRING_ADDCH(result, to_b32[cp[4] & 0x1f]);
    }
    if (count > 0)
	vstring_strncat(result, "======", pad_count[count]);
    VSTRING_TERMINATE(result);
    return (result);
}

/* base32_decode - encoded data to raw */

VSTRING *base32_decode(VSTRING *result, const char *in, ssize_t len)
{
    static unsigned char *un_b32 = 0;
    const unsigned char *cp;
    ssize_t count;
    unsigned int ch0;
    unsigned int ch1;
    unsigned int ch2;
    unsigned int ch3;
    unsigned int ch4;
    unsigned int ch5;
    unsigned int ch6;
    unsigned int ch7;

#define CHARS_PER_BYTE		(UCHAR_MAX + 1)
#define INVALID			0xff
#if 1
#define ENFORCE_LENGTH(x)	(x)
#define ENFORCE_PADDING(x)	(x)
#define ENFORCE_NULL_BITS(x)	(x)
#else
#define ENFORCE_LENGTH(x)	(1)
#define ENFORCE_PADDING(x)	(1)
#define ENFORCE_NULL_BITS(x)	(1)
#endif

    /*
     * Sanity check.
     */
    if (ENFORCE_LENGTH(len % 8))
	return (0);

    /*
     * Once: initialize the decoding lookup table on the fly.
     */
    if (un_b32 == 0) {
	un_b32 = (unsigned char *) mymalloc(CHARS_PER_BYTE);
	memset(un_b32, INVALID, CHARS_PER_BYTE);
	for (cp = to_b32; cp < to_b32 + sizeof(to_b32); cp++)
	    un_b32[*cp] = cp - to_b32;
    }

    /*
     * Decode 8 -> 5.
     */
    VSTRING_RESET(result);
    for (cp = UNSIG_CHAR_PTR(in), count = 0; count < len; count += 8) {
	if ((ch0 = un_b32[*cp++]) == INVALID
	    || (ch1 = un_b32[*cp++]) == INVALID)
	    return (0);
	VSTRING_ADDCH(result, ch0 << 3 | ch1 >> 2);
	if ((ch2 = *cp++) == '='
	    && ENFORCE_PADDING(strcmp((char *) cp, "=====") == 0)
	    && ENFORCE_NULL_BITS((ch1 & 0x3) == 0))
	    break;
	if ((ch2 = un_b32[ch2]) == INVALID)
	    return (0);
	if ((ch3 = un_b32[*cp++]) == INVALID)
	    return (0);
	VSTRING_ADDCH(result, ch1 << 6 | ch2 << 1 | ch3 >> 4);
	if ((ch4 = *cp++) == '='
	    && ENFORCE_PADDING(strcmp((char *) cp, "===") == 0)
	    && ENFORCE_NULL_BITS((ch3 & 0xf) == 0))
	    break;
	if ((ch4 = un_b32[ch4]) == INVALID)
	    return (0);
	VSTRING_ADDCH(result, ch3 << 4 | ch4 >> 1);
	if ((ch5 = *cp++) == '='
	    && ENFORCE_PADDING(strcmp((char *) cp, "==") == 0)
	    && ENFORCE_NULL_BITS((ch4 & 0x1) == 0))
	    break;
	if ((ch5 = un_b32[ch5]) == INVALID)
	    return (0);
	if ((ch6 = un_b32[*cp++]) == INVALID)
	    return (0);
	VSTRING_ADDCH(result, ch4 << 7 | ch5 << 2 | ch6 >> 3);
	if ((ch7 = *cp++) == '='
	    && ENFORCE_NULL_BITS((ch6 & 0x7) == 0))
	    break;
	if ((ch7 = un_b32[ch7]) == INVALID)
	    return (0);
	VSTRING_ADDCH(result, ch6 << 5 | ch7);
    }
    VSTRING_TERMINATE(result);
    return (result);
}

#ifdef TEST

 /*
  * Proof-of-concept test program: convert to base 32 and back.
  */

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *b1 = vstring_alloc(1);
    VSTRING *b2 = vstring_alloc(1);
    VSTRING *test = vstring_alloc(1);
    int     i, j;

    /*
     * Test all byte values (except null) on all byte positions.
     */
    for (j = 0; j < 256; j++)
	for (i = 1; i < 256; i++)
	    VSTRING_ADDCH(test, i);
    VSTRING_TERMINATE(test);

#define DECODE(b,x,l) { \
	if (base32_decode((b),(x),(l)) == 0) \
	    msg_panic("bad base32: %s", (x)); \
    }
#define VERIFY(b,t,l) { \
	if (memcmp((b), (t), (l)) != 0) \
	    msg_panic("bad test: %s", (b)); \
    }

    /*
     * Test all padding variants.
     */
    for (i = 1; i <= 8; i++) {
	base32_encode(b1, STR(test), LEN(test));
	DECODE(b2, STR(b1), LEN(b1));
	VERIFY(STR(b2), STR(test), LEN(test));

	base32_encode(b1, STR(test), LEN(test));
	base32_encode(b2, STR(b1), LEN(b1));
	base32_encode(b1, STR(b2), LEN(b2));
	DECODE(b2, STR(b1), LEN(b1));
	DECODE(b1, STR(b2), LEN(b2));
	DECODE(b2, STR(b1), LEN(b1));
	VERIFY(STR(b2), STR(test), LEN(test));

	base32_encode(b1, STR(test), LEN(test));
	base32_encode(b2, STR(b1), LEN(b1));
	base32_encode(b1, STR(b2), LEN(b2));
	base32_encode(b2, STR(b1), LEN(b1));
	base32_encode(b1, STR(b2), LEN(b2));
	DECODE(b2, STR(b1), LEN(b1));
	DECODE(b1, STR(b2), LEN(b2));
	DECODE(b2, STR(b1), LEN(b1));
	DECODE(b1, STR(b2), LEN(b2));
	DECODE(b2, STR(b1), LEN(b1));
	VERIFY(STR(b2), STR(test), LEN(test));
	vstring_truncate(test, LEN(test) - 1);
    }
    vstring_free(test);
    vstring_free(b1);
    vstring_free(b2);
    return (0);
}

#endif
