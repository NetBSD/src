/*++
/* NAME
/*	xtext 3
/* SUMMARY
/*	quote/unquote text, xtext style.
/* SYNOPSIS
/*	#include <xtext.h>
/*
/*	VSTRING	*xtext_quote(quoted, unquoted, special)
/*	VSTRING	*quoted;
/*	const char *unquoted;
/*	const char *special;
/*
/*	VSTRING	*xtext_unquote_append(unquoted, quoted)
/*	VSTRING	*unquoted;
/*	const char *quoted;
/*
/*	VSTRING	*xtext_unquote(unquoted, quoted)
/*	VSTRING	*unquoted;
/*	const char *quoted;
/* DESCRIPTION
/*	xtext_quote() takes a null-terminated string and replaces characters
/*	+, <33(10) and >126(10), as well as characters specified with "special"
/*	by +XX, XX being the two-digit uppercase hexadecimal equivalent.
/*
/*	xtext_quote_append() is like xtext_quote(), but appends the conversion
/*	result to the result buffer.
/*
/*	xtext_unquote() performs the opposite transformation. This function
/*	understands lowercase, uppercase, and mixed case +XX sequences. The
/*	result value is the unquoted argument in case of success, a null pointer
/*	otherwise.
/* BUGS
/*	This module cannot process null characters in data.
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
#include "vstring.h"
#include "xtext.h"

/* Application-specific. */

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

/* xtext_quote_append - append unquoted data to quoted data */

VSTRING *xtext_quote_append(VSTRING *quoted, const char *unquoted,
			            const char *special)
{
    const char *cp;
    int     ch;

    for (cp = unquoted; (ch = *(unsigned const char *) cp) != 0; cp++) {
	if (ch != '+' && ch > 32 && ch < 127
	    && (*special == 0 || strchr(special, ch) == 0)) {
	    VSTRING_ADDCH(quoted, ch);
	} else {
	    vstring_sprintf_append(quoted, "+%02X", ch);
	}
    }
    VSTRING_TERMINATE(quoted);
    return (quoted);
}

/* xtext_quote - unquoted data to quoted */

VSTRING *xtext_quote(VSTRING *quoted, const char *unquoted, const char *special)
{
    VSTRING_RESET(quoted);
    xtext_quote_append(quoted, unquoted, special);
    return (quoted);
}

/* xtext_unquote - quoted data to unquoted */

VSTRING *xtext_unquote(VSTRING *unquoted, const char *quoted)
{
    const char *cp;
    int     ch;

    VSTRING_RESET(unquoted);
    for (cp = quoted; (ch = *cp) != 0; cp++) {
	if (ch == '+') {
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
	VSTRING_ADDCH(unquoted, ch);
    }
    VSTRING_TERMINATE(unquoted);
    return (unquoted);
}

#ifdef TEST

 /*
  * Proof-of-concept test program: convert to quoted and back.
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
    VSTRING *unquoted = vstring_alloc(BUFLEN);
    VSTRING *quoted = vstring_alloc(100);
    int     len;

    while ((len = read_buf(VSTREAM_IN, unquoted)) > 0) {
	xtext_quote(quoted, STR(unquoted), "+=");
	if (xtext_unquote(unquoted, STR(quoted)) == 0)
	    msg_fatal("bad input: %.100s", STR(quoted));
	if (LEN(unquoted) != len)
	    msg_fatal("len %d != unquoted len %d", len, LEN(unquoted));
	if (vstream_fwrite(VSTREAM_OUT, STR(unquoted), LEN(unquoted)) != LEN(unquoted))
	    msg_fatal("write error: %m");
    }
    vstream_fflush(VSTREAM_OUT);
    vstring_free(unquoted);
    vstring_free(quoted);
    return (0);
}

#endif
