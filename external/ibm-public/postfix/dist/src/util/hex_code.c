/*	$NetBSD: hex_code.c,v 1.3 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	hex_code 3
/* SUMMARY
/*	encode/decode data, hexadecimal style
/* SYNOPSIS
/*	#include <hex_code.h>
/*
/*	VSTRING	*hex_encode(result, in, len)
/*	VSTRING	*result;
/*	const char *in;
/*	ssize_t	len;
/*
/*	VSTRING	*hex_decode(result, in, len)
/*	VSTRING	*result;
/*	const char *in;
/*	ssize_t	len;
/*
/*	VSTRING	*hex_encode_opt(result, in, len, flags)
/*	VSTRING	*result;
/*	const char *in;
/*	ssize_t	len;
/*	int	flags;
/*
/*	VSTRING	*hex_decode_opt(result, in, len, flags)
/*	VSTRING	*result;
/*	const char *in;
/*	ssize_t	len;
/*	int	flags;
/* DESCRIPTION
/*	hex_encode() takes a block of len bytes and encodes it as one
/*	upper-case null-terminated string.  The result value is
/*	the result argument.
/*
/*	hex_decode() performs the opposite transformation on
/*	lower-case, upper-case or mixed-case input. The result
/*	value is the result argument. The result is null terminated,
/*	whether or not that makes sense.
/*
/*	hex_encode_opt() enables extended functionality as controlled
/*	with \fIflags\fR.
/* .IP HEX_ENCODE_FLAG_NONE
/*	The default: a self-documenting flag that enables no
/*	functionality.
/* .IP HEX_ENCODE_FLAG_USE_COLON
/*	Inserts one ":" between bytes.
/* .PP
/*	hex_decode_opt() enables extended functionality as controlled
/*	with \fIflags\fR.
/* .IP HEX_DECODE_FLAG_NONE
/*	The default: a self-documenting flag that enables no
/*	functionality.
/* .IP HEX_DECODE_FLAG_ALLOW_COLON
/*	Allows, but does not require, one ":" between bytes.
/* DIAGNOSTICS
/*	hex_decode() returns a null pointer when the input contains
/*	characters not in the hexadecimal alphabet.
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
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <ctype.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <hex_code.h>

/* Application-specific. */

static const unsigned char hex_chars[] = "0123456789ABCDEF";

#define UCHAR_PTR(x) ((const unsigned char *)(x))

/* hex_encode - ABI compatibility */

#undef hex_encode

VSTRING *hex_encode(VSTRING *result, const char *in, ssize_t len)
{
    return (hex_encode_opt(result, in, len, HEX_ENCODE_FLAG_NONE));
}

/* hex_encode_opt - raw data to encoded */

VSTRING *hex_encode_opt(VSTRING *result, const char *in, ssize_t len, int flags)
{
    const unsigned char *cp;
    int     ch;
    ssize_t count;

    VSTRING_RESET(result);
    for (cp = UCHAR_PTR(in), count = len; count > 0; count--, cp++) {
	ch = *cp;
	VSTRING_ADDCH(result, hex_chars[(ch >> 4) & 0xf]);
	VSTRING_ADDCH(result, hex_chars[ch & 0xf]);
	if ((flags & HEX_ENCODE_FLAG_USE_COLON) && count > 1)
	    VSTRING_ADDCH(result, ':');
    }
    VSTRING_TERMINATE(result);
    return (result);
}

/* hex_decode - ABI compatibility wrapper */

#undef hex_decode

VSTRING *hex_decode(VSTRING *result, const char *in, ssize_t len)
{
    return (hex_decode_opt(result, in, len, HEX_DECODE_FLAG_NONE));
}

/* hex_decode_opt - encoded data to raw */

VSTRING *hex_decode_opt(VSTRING *result, const char *in, ssize_t len, int flags)
{
    const unsigned char *cp;
    ssize_t count;
    unsigned int hex;
    unsigned int bin;

    VSTRING_RESET(result);
    for (cp = UCHAR_PTR(in), count = len; count > 0; cp += 2, count -= 2) {
	if (count < 2)
	    return (0);
	hex = cp[0];
	if (hex >= '0' && hex <= '9')
	    bin = (hex - '0') << 4;
	else if (hex >= 'A' && hex <= 'F')
	    bin = (hex - 'A' + 10) << 4;
	else if (hex >= 'a' && hex <= 'f')
	    bin = (hex - 'a' + 10) << 4;
	else
	    return (0);
	hex = cp[1];
	if (hex >= '0' && hex <= '9')
	    bin |= (hex - '0');
	else if (hex >= 'A' && hex <= 'F')
	    bin |= (hex - 'A' + 10);
	else if (hex >= 'a' && hex <= 'f')
	    bin |= (hex - 'a' + 10);
	else
	    return (0);
	VSTRING_ADDCH(result, bin);

	/*
	 * Support *colon-separated* input (no leading or trailing colons).
	 * After decoding "xx", skip a possible ':' preceding "yy" in
	 * "xx:yy".
	 */
	if ((flags & HEX_DECODE_FLAG_ALLOW_COLON)
	    && count > 4 && cp[2] == ':') {
	    ++cp;
	    --count;
	}
    }
    VSTRING_TERMINATE(result);
    return (result);
}

#ifdef TEST
#include <argv.h>

 /*
  * Proof-of-concept test program: convert to hexadecimal and back.
  */

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *b1 = vstring_alloc(1);
    VSTRING *b2 = vstring_alloc(1);
    char   *test = "this is a test";
    ARGV   *argv;

#define DECODE(b,x,l) { \
	if (hex_decode((b),(x),(l)) == 0) \
	    msg_panic("bad hex: %s", (x)); \
    }
#define VERIFY(b,t) { \
	if (strcmp((b), (t)) != 0) \
	    msg_panic("bad test: %s", (b)); \
    }

    hex_encode(b1, test, strlen(test));
    DECODE(b2, STR(b1), LEN(b1));
    VERIFY(STR(b2), test);

    hex_encode(b1, test, strlen(test));
    hex_encode(b2, STR(b1), LEN(b1));
    hex_encode(b1, STR(b2), LEN(b2));
    DECODE(b2, STR(b1), LEN(b1));
    DECODE(b1, STR(b2), LEN(b2));
    DECODE(b2, STR(b1), LEN(b1));
    VERIFY(STR(b2), test);

    hex_encode(b1, test, strlen(test));
    hex_encode(b2, STR(b1), LEN(b1));
    hex_encode(b1, STR(b2), LEN(b2));
    hex_encode(b2, STR(b1), LEN(b1));
    hex_encode(b1, STR(b2), LEN(b2));
    DECODE(b2, STR(b1), LEN(b1));
    DECODE(b1, STR(b2), LEN(b2));
    DECODE(b2, STR(b1), LEN(b1));
    DECODE(b1, STR(b2), LEN(b2));
    DECODE(b2, STR(b1), LEN(b1));
    VERIFY(STR(b2), test);

    hex_encode_opt(b1, test, strlen(test), HEX_ENCODE_FLAG_USE_COLON);
    argv = argv_split(STR(b1), ":");
    if (argv->argc != strlen(test))
	msg_panic("HEX_ENCODE_FLAG_USE_COLON");
    if (hex_decode_opt(b2, STR(b1), LEN(b1), HEX_DECODE_FLAG_ALLOW_COLON) == 0)
	msg_panic("HEX_DECODE_FLAG_ALLOW_COLON");
    VERIFY(STR(b2), test);
    argv_free(argv);

    vstring_free(b1);
    vstring_free(b2);
    return (0);
}

#endif
