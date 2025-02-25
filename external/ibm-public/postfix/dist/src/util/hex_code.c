/*	$NetBSD: hex_code.c,v 1.4 2025/02/25 19:15:52 christos Exp $	*/

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
/* .IP HEX_ENCODE_FLAG_APPEND
/*	Append output to the buffer.
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
/*
/*	Wietse Venema
/*	porcupine.org
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

    if ((flags & HEX_ENCODE_FLAG_APPEND) == 0)
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

 /*
  * Proof-of-concept test program: convert to hexadecimal and back.
  */
#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

typedef struct TEST_CASE {
    const char *label;			/* identifies test case */
    VSTRING *(*func) (VSTRING *, const char *, ssize_t, int);
    const char *input;			/* input string */
    ssize_t inlen;			/* input size */
    int     flags;			/* flags */
    const char *exp_output;		/* expected output or null */
    ssize_t exp_outlen;			/* expected size */
} TEST_CASE;

/*
  * The test cases.
  */
#define OUTPUT_INIT	"thrash:"	/* output buffer initial content */
#define OUTPUT_INIT_SZ	(sizeof(OUTPUT_INIT) - 1)

static const TEST_CASE test_cases[] = {
    {"hex_encode_no_options", hex_encode_opt,
	"this is a test",
	sizeof("this is a test") - 1,
	HEX_ENCODE_FLAG_NONE,
	"7468697320697320612074657374",
	sizeof("7468697320697320612074657374") - 1,
    },
    {"hex_decode_no_options", hex_decode_opt,
	"7468697320697320612074657374",
	sizeof("7468697320697320612074657374") - 1,
	HEX_DECODE_FLAG_NONE,
	"this is a test",
	sizeof("this is a test") - 1,
    },
    {"hex_decode_no_colon_allow_colon", hex_decode_opt,
	"7468697320697320612074657374",
	sizeof("7468697320697320612074657374") - 1,
	HEX_DECODE_FLAG_ALLOW_COLON,
	"this is a test",
	sizeof("this is a test") - 1,
    },
    {"hex_encode_appends", hex_encode_opt,
	"this is a test",
	sizeof("this is a test") - 1,
	HEX_ENCODE_FLAG_APPEND,
	OUTPUT_INIT "7468697320697320612074657374",
	sizeof(OUTPUT_INIT "7468697320697320612074657374") - 1,
    },
    {"hex_encode_with_colon", hex_encode_opt,
	"this is a test",
	sizeof("this is a test") - 1,
	HEX_ENCODE_FLAG_USE_COLON,
	"74:68:69:73:20:69:73:20:61:20:74:65:73:74",
	sizeof("74:68:69:73:20:69:73:20:61:20:74:65:73:74") - 1,
    },
    {"hex_encode_with_colon_and_append", hex_encode_opt,
	"this is a test",
	sizeof("this is a test") - 1,
	HEX_ENCODE_FLAG_USE_COLON | HEX_ENCODE_FLAG_APPEND,
	OUTPUT_INIT "74:68:69:73:20:69:73:20:61:20:74:65:73:74",
	sizeof(OUTPUT_INIT "74:68:69:73:20:69:73:20:61:20:74:65:73:74") - 1,
    },
    {"hex_decode_error", hex_decode_opt,
	"this is a test",
	sizeof("this is a test") - 1,
	HEX_DECODE_FLAG_ALLOW_COLON,
	0,
	0,
    },
    {0},
};

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *buf = vstring_alloc(1);
    int     pass = 0;
    int     fail = 0;
    const TEST_CASE *tp;

    for (tp = test_cases; tp->label != 0; tp++) {
	VSTRING *out;
	int     ok = 0;

	msg_info("RUN  %s", tp->label);
	vstring_memcpy(buf, OUTPUT_INIT, OUTPUT_INIT_SZ);
	out = tp->func(buf, tp->input, tp->inlen, tp->flags);
	if (out == 0 && tp->exp_output == 0) {
	    ok = 1;
	} else if (out != buf) {
	    msg_warn("got result '%p', want: '%p'",
		     (void *) out, (void *) buf);
	} else if (LEN(out) != tp->exp_outlen) {
	    msg_warn("got result length '%ld', want: '%ld'",
		     (long) LEN(out), (long) tp->exp_outlen);
	} else if (memcmp(STR(out), tp->exp_output, tp->exp_outlen) != 0) {
	    msg_warn("got result '%*s', want: '%*s'",
		     (int) LEN(out), STR(out),
		     (int) tp->exp_outlen, tp->exp_output);
	} else {
	    ok = 1;
	}
	if (ok) {
	    msg_info("PASS %s", tp->label);
	    pass++;
	} else {
	    msg_info("FAIL %s", tp->label);
	    fail++;
	}
    }
    vstring_free(buf);
    msg_info("PASS=%d FAIL=%d", pass, fail);
    return (fail > 0);
}

#endif
