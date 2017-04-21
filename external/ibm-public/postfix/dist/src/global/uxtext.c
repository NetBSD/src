/*	$NetBSD: uxtext.c,v 1.2.4.2 2017/04/21 16:52:48 bouyer Exp $	*/

/*++
/* NAME
/*	uxtext 3
/* SUMMARY
/*	quote/unquote text, xtext style.
/* SYNOPSIS
/*	#include <uxtext.h>
/*
/*	VSTRING	*uxtext_quote(quoted, unquoted, special)
/*	VSTRING	*quoted;
/*	const char *unquoted;
/*	const char *special;
/*
/*	VSTRING	*uxtext_quote_append(unquoted, quoted, special)
/*	VSTRING	*unquoted;
/*	const char *quoted;
/*	const char *special;
/*
/*	VSTRING	*uxtext_unquote(unquoted, quoted)
/*	VSTRING	*unquoted;
/*	const char *quoted;
/*
/*	VSTRING	*uxtext_unquote_append(unquoted, quoted)
/*	VSTRING	*unquoted;
/*	const char *quoted;
/* DESCRIPTION
/*	uxtext_quote() takes a null-terminated UTF8 string and
/*	replaces characters \, <33(10) and >126(10), as well as
/*	characters specified with "special" with \x{XX}, XX being
/*	a 2-6-digit uppercase hexadecimal equivalent.
/*
/*	uxtext_quote_append() is like uxtext_quote(), but appends
/*	the conversion result to the result buffer.
/*
/*	uxtext_unquote() performs the opposite transformation. This
/*	function understands lowercase, uppercase, and mixed case
/*	\x{XX...} sequences.  The result value is the unquoted
/*	argument in case of success, a null pointer otherwise.
/*
/*	uxtext_unquote_append() is like uxtext_unquote(), but appends
/*	the conversion result to the result buffer.
/* BUGS
/*	This module cannot process null characters in data.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Arnt Gulbrandsen
/*
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
#include "uxtext.h"

/* Application-specific. */

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

/* uxtext_quote_append - append unquoted data to quoted data */

VSTRING *uxtext_quote_append(VSTRING *quoted, const char *unquoted,
			             const char *special)
{
    unsigned const char *cp;
    int     ch;

    for (cp = (unsigned const char *) unquoted; (ch = *cp) != 0; cp++) {
	/* Fix 20140709: the '\' character must always be quoted. */
	if (ch != '\\' && ch > 32 && ch < 127
	    && (*special == 0 || strchr(special, ch) == 0)) {
	    VSTRING_ADDCH(quoted, ch);
	} else {

	    /*
	     * had RFC6533 been written like 6531 and 6532, this else clause
	     * would be one line long.
	     */
	    int     unicode = 0;
	    int     pick = 0;

	    if (ch < 0x80) {
		//0000 0000 - 0000 007 F 0x xxxxxx
		    unicode = ch;
	    } else if ((ch & 0xe0) == 0xc0) {
		//0000 0080 - 0000 07 FF 110 xxxxx 10 xxxxxx
		    unicode = (ch & 0x1f);
		pick = 1;
	    } else if ((ch & 0xf0) == 0xe0) {
		//0000 0800 - 0000 FFFF 1110 xxxx 10 xxxxxx 10 xxxxxx
		    unicode = (ch & 0x0f);
		pick = 2;
	    } else if ((ch & 0xf8) == 0xf0) {
		//0001 0000 - 001 F FFFF 11110 xxx 10 xxxxxx 10 xxxxxx 10 xxxxxx
		    unicode = (ch & 0x07);
		pick = 3;
	    } else if ((ch & 0xfc) == 0xf8) {
		//0020 0000 - 03 FF FFFF 111110 xx 10 xxxxxx 10 xxxxxx...10 xxxxxx
		    unicode = (ch & 0x03);
		pick = 4;
	    } else if ((ch & 0xfe) == 0xfc) {
		//0400 0000 - 7 FFF FFFF 1111110 x 10 xxxxxx...10 xxxxxx
		    unicode = (ch & 0x01);
		pick = 5;
	    } else {
		return (0);
	    }
	    while (pick > 0) {
		ch = *++cp;
		if ((ch & 0xc0) != 0x80)
		    return (0);
		unicode = unicode << 6 | (ch & 0x3f);
		pick--;
	    }
	    vstring_sprintf_append(quoted, "\\x{%02X}", unicode);
	}
    }
    VSTRING_TERMINATE(quoted);
    return (quoted);
}

/* uxtext_quote - unquoted data to quoted */

VSTRING *uxtext_quote(VSTRING *quoted, const char *unquoted, const char *special)
{
    VSTRING_RESET(quoted);
    uxtext_quote_append(quoted, unquoted, special);
    return (quoted);
}

/* uxtext_unquote_append - quoted data to unquoted */

VSTRING *uxtext_unquote_append(VSTRING *unquoted, const char *quoted)
{
    const unsigned char *cp;
    int     ch;

    for (cp = (const unsigned char *) quoted; (ch = *cp) != 0; cp++) {
	if (ch == '\\' && cp[1] == 'x' && cp[2] == '{') {
	    int     unicode = 0;

	    cp += 2;
	    while ((ch = *++cp) != '}') {
		if (ISDIGIT(ch))
		    unicode = (unicode << 4) + (ch - '0');
		else if (ch >= 'a' && ch <= 'f')
		    unicode = (unicode << 4) + (ch - 'a' + 10);
		else if (ch >= 'A' && ch <= 'F')
		    unicode = (unicode << 4) + (ch - 'A' + 10);
		else
		    return (0);			/* also covers the null
						 * terminator */
		if (unicode > 0x10ffff)
		    return (0);
	    }

	    /*
	     * the following block is from
	     * https://github.com/aox/aox/blob/master/encodings/utf.cpp, with
	     * permission by the authors.
	     */
	    if (unicode < 0x80) {
		VSTRING_ADDCH(unquoted, (char) unicode);
	    } else if (unicode < 0x800) {
		VSTRING_ADDCH(unquoted, 0xc0 | ((char) (unicode >> 6)));
		VSTRING_ADDCH(unquoted, 0x80 | ((char) (unicode & 0x3f)));
	    } else if (unicode < 0x10000) {
		VSTRING_ADDCH(unquoted, 0xe0 | ((char) (unicode >> 12)));
		VSTRING_ADDCH(unquoted, 0x80 | ((char) (unicode >> 6) & 0x3f));
		VSTRING_ADDCH(unquoted, 0x80 | ((char) (unicode & 0x3f)));
	    } else if (unicode < 0x200000) {
		VSTRING_ADDCH(unquoted, 0xf0 | ((char) (unicode >> 18)));
		VSTRING_ADDCH(unquoted, 0x80 | ((char) (unicode >> 12) & 0x3f));
		VSTRING_ADDCH(unquoted, 0x80 | ((char) (unicode >> 6) & 0x3f));
		VSTRING_ADDCH(unquoted, 0x80 | ((char) (unicode & 0x3f)));
	    } else if (unicode < 0x4000000) {
		VSTRING_ADDCH(unquoted, 0xf8 | ((char) (unicode >> 24)));
		VSTRING_ADDCH(unquoted, 0x80 | ((char) (unicode >> 18) & 0x3f));
		VSTRING_ADDCH(unquoted, 0x80 | ((char) (unicode >> 12) & 0x3f));
		VSTRING_ADDCH(unquoted, 0x80 | ((char) (unicode >> 6) & 0x3f));
		VSTRING_ADDCH(unquoted, 0x80 | ((char) (unicode & 0x3f)));
	    } else {
		VSTRING_ADDCH(unquoted, 0xfc | ((char) (unicode >> 30)));
		VSTRING_ADDCH(unquoted, 0x80 | ((char) (unicode >> 24) & 0x3f));
		VSTRING_ADDCH(unquoted, 0x80 | ((char) (unicode >> 18) & 0x3f));
		VSTRING_ADDCH(unquoted, 0x80 | ((char) (unicode >> 12) & 0x3f));
		VSTRING_ADDCH(unquoted, 0x80 | ((char) (unicode >> 6) & 0x3f));
		VSTRING_ADDCH(unquoted, 0x80 | ((char) (unicode & 0x3f)));
	    }
	} else {
	    VSTRING_ADDCH(unquoted, ch);
	}
    }
    VSTRING_TERMINATE(unquoted);
    return (unquoted);
}

/* uxtext_unquote - quoted data to unquoted */

VSTRING *uxtext_unquote(VSTRING *unquoted, const char *quoted)
{
    VSTRING_RESET(unquoted);
    return (uxtext_unquote_append(unquoted, quoted) ? unquoted : 0);
}

#ifdef TEST

 /*
  * Proof-of-concept test program: convert to quoted and back.
  */
#include <vstream.h>

#define BUFLEN 1024

static ssize_t read_buf(VSTREAM *fp, VSTRING *buf)
{
    ssize_t len;

    VSTRING_RESET(buf);
    len = vstream_fread(fp, STR(buf), vstring_avail(buf));
    VSTRING_AT_OFFSET(buf, len);		/* XXX */
    VSTRING_TERMINATE(buf);
    return (len);
}

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *unquoted = vstring_alloc(BUFLEN);
    VSTRING *quoted = vstring_alloc(100);
    ssize_t len;

    /*
     * Negative tests.
     */
    if (uxtext_unquote(unquoted, "\\x{x1}") != 0)
	msg_warn("undetected error pattern 1");
    if (uxtext_unquote(unquoted, "\\x{2x}") != 0)
	msg_warn("undetected error pattern 2");
    if (uxtext_unquote(unquoted, "\\x{33") != 0)
	msg_warn("undetected error pattern 3");

    /*
     * Positive tests.
     */
    while ((len = read_buf(VSTREAM_IN, unquoted)) > 0) {
	uxtext_quote(quoted, STR(unquoted), "+=");
	if (uxtext_unquote(unquoted, STR(quoted)) == 0)
	    msg_fatal("bad input: %.100s", STR(quoted));
	if (LEN(unquoted) != len)
	    msg_fatal("len %ld != unquoted len %ld",
		      (long) len, (long) LEN(unquoted));
	if (vstream_fwrite(VSTREAM_OUT, STR(unquoted), LEN(unquoted)) != LEN(unquoted))
	    msg_fatal("write error: %m");
    }
    vstream_fflush(VSTREAM_OUT);
    vstring_free(unquoted);
    vstring_free(quoted);
    return (0);
}

#endif
