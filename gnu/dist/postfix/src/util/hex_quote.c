/*++
/* NAME
/*	hex_quote 3
/* SUMMARY
/*	quote/unquote text, HTTP style.
/* SYNOPSIS
/*	#include <hex_quote.h>
/*
/*	VSTRING	*hex_quote(hex, raw)
/*	VSTRING	*hex;
/*	const char *raw;
/*
/*	VSTRING	*hex_unquote(raw, hex)
/*	VSTRING	*raw;
/*	const char *hex;
/* DESCRIPTION
/*	hex_quote() takes a null-terminated string and replaces non-printable
/*	characters and % by %XX, XX being the two-digit hexadecimal equivalent.
/*	The hexadecimal codes are produced as upper-case characters. The result
/*	value is the hex argument.
/*
/*	hex_unquote() performs the opposite transformation. This function
/*	understands lowercase, uppercase, and mixed case %XX sequences. The
/*	result value is the raw argument in case of success, a null pointer
/*	otherwise.
/* BUGS
/*	hex_quote() cannot process null characters in data.
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

/* Utility library. */

#include "msg.h"
#include "vstring.h"
#include "hex_quote.h"

/* Application-specific. */

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

/* hex_quote - raw data to quoted */

VSTRING *hex_quote(VSTRING *hex, const char *raw)
{
    const char *cp;
    int     ch;

    VSTRING_RESET(hex);
    for (cp = raw; (ch = *(unsigned const char *) cp) != 0; cp++) {
	if (ch != '%' && ISPRINT(ch)) {
	    VSTRING_ADDCH(hex, ch);
	} else {
	    vstring_sprintf_append(hex, "%%%02X", ch);
	}
    }
    VSTRING_TERMINATE(hex);
    return (hex);
}

/* hex_unquote - quoted data to raw */

VSTRING *hex_unquote(VSTRING *raw, const char *hex)
{
    const char *cp;
    int     ch;

    VSTRING_RESET(raw);
    for (cp = hex; (ch = *cp) != 0; cp++) {
	if (ch == '%') {
	    if (ISDIGIT(cp[1]))
		ch = (cp[1] - '0') << 4;
	    else if (cp[1] >= 'a' && cp[1] <= 'f')
		ch = (cp[1] - 'a' + 10) << 4;
	    else if (cp[1] >= 'A' && cp[1] <= 'F')
		ch = (cp[1] - 'A' + 10) << 4;
	    else
		return (0);
	    if (ISDIGIT(cp[2]))
		ch |= (cp[2] - '0');
	    else if (cp[2] >= 'a' && cp[2] <= 'f')
		ch |= (cp[2] - 'a' + 10);
	    else if (cp[2] >= 'A' && cp[2] <= 'F')
		ch |= (cp[2] - 'A' + 10);
	    else
		return (0);
	    cp += 2;
	}
	VSTRING_ADDCH(raw, ch);
    }
    VSTRING_TERMINATE(raw);
    return (raw);
}

#ifdef TEST

 /*
  * Proof-of-concept test program: convert to hex and back.
  */
#include <vstream.h>

#define BUFLEN 1024

static int read_buf(VSTREAM *fp, VSTRING *buf)
{
    int     len;

    VSTRING_RESET(buf);
    len = vstream_fread(fp, STR(buf), vstring_avail(buf));
    VSTRING_AT_OFFSET(buf, len);		/* XXX */
    VSTRING_TERMINATE(buf);
    return (len);
}

main(int unused_argc, char **unused_argv)
{
    VSTRING *raw = vstring_alloc(BUFLEN);
    VSTRING *hex = vstring_alloc(100);
    int     len;

    while ((len = read_buf(VSTREAM_IN, raw)) > 0) {
	hex_quote(hex, STR(raw));
	if (hex_unquote(raw, STR(hex)) == 0)
	    msg_fatal("bad input: %.100s", STR(hex));
	if (LEN(raw) != len)
	    msg_fatal("len %d != raw len %d", len, LEN(raw));
	if (vstream_fwrite(VSTREAM_OUT, STR(raw), LEN(raw)) != LEN(raw))
	    msg_fatal("write error: %m");
    }
    vstream_fflush(VSTREAM_OUT);
    vstring_free(raw);
    vstring_free(hex);
    return (0);
}

#endif
