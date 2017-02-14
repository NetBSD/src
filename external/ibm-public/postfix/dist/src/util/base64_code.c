/*	$NetBSD: base64_code.c,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

/*++
/* NAME
/*	base64_code 3
/* SUMMARY
/*	encode/decode data, base 64 style
/* SYNOPSIS
/*	#include <base64_code.h>
/*
/*	VSTRING	*base64_encode(result, in, len)
/*	VSTRING	*result;
/*	const char *in;
/*	ssize_t	len;
/*
/*	VSTRING	*base64_decode(result, in, len)
/*	VSTRING	*result;
/*	const char *in;
/*	ssize_t	len;
/*
/*	VSTRING	*base64_encode_opt(result, in, len, flags)
/*	VSTRING	*result;
/*	const char *in;
/*	ssize_t	len;
/*	int	flags;
/*
/*	VSTRING	*base64_decode_opt(result, in, len, flags)
/*	VSTRING	*result;
/*	const char *in;
/*	ssize_t	len;
/*	int	flags;
/* DESCRIPTION
/*	base64_encode() takes a block of len bytes and encodes it as one
/*	null-terminated string.  The result value is the result argument.
/*
/*	base64_decode() performs the opposite transformation. The result
/*	value is the result argument. The result is null terminated, whether
/*	or not that makes sense.
/*
/*	base64_encode_opt() and base64_decode_opt() provide extended
/*	interfaces.  In both cases the flags arguments is the bit-wise
/*	OR of zero or more the following:
/* .IP BASE64_FLAG_APPEND
/*	Append the result, instead of overwriting the result buffer.
/* .PP
/*	For convenience, BASE64_FLAG_NONE specifies none of the above.
/* DIAGNOSTICS
/*	base64_decode () returns a null pointer when the input contains
/*	characters not in the base 64 alphabet.
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
#include <base64_code.h>

/* Application-specific. */

static unsigned char to_b64[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define UNSIG_CHAR_PTR(x) ((unsigned char *)(x))

/* base64_encode - raw data to encoded */

#undef base64_encode

extern VSTRING *base64_encode(VSTRING *, const char *, ssize_t);

VSTRING *base64_encode(VSTRING *result, const char *in, ssize_t len)
{
    return (base64_encode_opt(result, in, len, BASE64_FLAG_NONE));
}

VSTRING *base64_encode_opt(VSTRING *result, const char *in, ssize_t len,
			           int flags)
{
    const unsigned char *cp;
    ssize_t count;

    /*
     * Encode 3 -> 4.
     */
    if ((flags & BASE64_FLAG_APPEND) == 0)
	VSTRING_RESET(result);
    for (cp = UNSIG_CHAR_PTR(in), count = len; count > 0; count -= 3, cp += 3) {
	VSTRING_ADDCH(result, to_b64[cp[0] >> 2]);
	if (count > 1) {
	    VSTRING_ADDCH(result, to_b64[(cp[0] & 0x3) << 4 | cp[1] >> 4]);
	    if (count > 2) {
		VSTRING_ADDCH(result, to_b64[(cp[1] & 0xf) << 2 | cp[2] >> 6]);
		VSTRING_ADDCH(result, to_b64[cp[2] & 0x3f]);
	    } else {
		VSTRING_ADDCH(result, to_b64[(cp[1] & 0xf) << 2]);
		VSTRING_ADDCH(result, '=');
		break;
	    }
	} else {
	    VSTRING_ADDCH(result, to_b64[(cp[0] & 0x3) << 4]);
	    VSTRING_ADDCH(result, '=');
	    VSTRING_ADDCH(result, '=');
	    break;
	}
    }
    VSTRING_TERMINATE(result);
    return (result);
}

/* base64_decode - encoded data to raw */

#undef base64_decode

extern VSTRING *base64_decode(VSTRING *, const char *, ssize_t);

VSTRING *base64_decode(VSTRING *result, const char *in, ssize_t len)
{
    return (base64_decode_opt(result, in, len, BASE64_FLAG_NONE));
}

VSTRING *base64_decode_opt(VSTRING *result, const char *in, ssize_t len,
			           int flags)
{
    static unsigned char *un_b64 = 0;
    const unsigned char *cp;
    ssize_t count;
    unsigned int ch0;
    unsigned int ch1;
    unsigned int ch2;
    unsigned int ch3;

#define CHARS_PER_BYTE	(UCHAR_MAX + 1)
#define INVALID		0xff

    /*
     * Sanity check.
     */
    if (len % 4)
	return (0);

    /*
     * Once: initialize the decoding lookup table on the fly.
     */
    if (un_b64 == 0) {
	un_b64 = (unsigned char *) mymalloc(CHARS_PER_BYTE);
	memset(un_b64, INVALID, CHARS_PER_BYTE);
	for (cp = to_b64; cp < to_b64 + sizeof(to_b64) - 1; cp++)
	    un_b64[*cp] = cp - to_b64;
    }

    /*
     * Decode 4 -> 3.
     */
    if ((flags & BASE64_FLAG_APPEND) == 0)
	VSTRING_RESET(result);
    for (cp = UNSIG_CHAR_PTR(in), count = 0; count < len; count += 4) {
	if ((ch0 = un_b64[*cp++]) == INVALID
	    || (ch1 = un_b64[*cp++]) == INVALID)
	    return (0);
	VSTRING_ADDCH(result, ch0 << 2 | ch1 >> 4);
	if ((ch2 = *cp++) == '=')
	    break;
	if ((ch2 = un_b64[ch2]) == INVALID)
	    return (0);
	VSTRING_ADDCH(result, ch1 << 4 | ch2 >> 2);
	if ((ch3 = *cp++) == '=')
	    break;
	if ((ch3 = un_b64[ch3]) == INVALID)
	    return (0);
	VSTRING_ADDCH(result, ch2 << 6 | ch3);
    }
    VSTRING_TERMINATE(result);
    return (result);
}

#ifdef TEST

 /*
  * Proof-of-concept test program: convert to base 64 and back.
  */

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *b1 = vstring_alloc(1);
    VSTRING *b2 = vstring_alloc(1);
    char    test[256];
    int     n;

    for (n = 0; n < sizeof(test); n++)
	test[n] = n;
    base64_encode(b1, test, sizeof(test));
    if (base64_decode(b2, STR(b1), LEN(b1)) == 0)
	msg_panic("bad base64: %s", STR(b1));
    if (LEN(b2) != sizeof(test))
	msg_panic("bad decode length: %ld != %ld",
		  (long) LEN(b2), (long) sizeof(test));
    for (n = 0; n < sizeof(test); n++)
	if (STR(b2)[n] != test[n])
	    msg_panic("bad decode value %d != %d",
		      (unsigned char) STR(b2)[n], (unsigned char) test[n]);
    vstring_free(b1);
    vstring_free(b2);
    return (0);
}

#endif
