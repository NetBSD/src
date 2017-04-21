/*	$NetBSD: mystrtok.c,v 1.1.1.1.36.1 2017/04/21 16:52:53 bouyer Exp $	*/

/*++
/* NAME
/*	mystrtok 3
/* SUMMARY
/*	safe tokenizer
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	char	*mystrtok(bufp, delimiters)
/*	char	**bufp;
/*	const char *delimiters;
/*
/*	char	*mystrtokq(bufp, delimiters, parens)
/*	char	**bufp;
/*	const char *delimiters;
/*	const char *parens;
/* DESCRIPTION
/*	mystrtok() splits a buffer on the specified \fIdelimiters\fR.
/*	Tokens are delimited by runs of delimiters, so this routine
/*	cannot return zero-length tokens.
/*
/*	mystrtokq() is like mystrtok() but will not split text
/*	between balanced parentheses.  \fIparens\fR specifies the
/*	opening and closing parenthesis (one of each).  The set of
/*	\fIparens\fR must be distinct from the set of \fIdelimiters\fR.
/*
/*	The \fIbufp\fR argument specifies the start of the search; it
/*	is updated with each call. The input is destroyed.
/*
/*	The result value is the next token, or a null pointer when the
/*	end of the buffer was reached.
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
#include <string.h>

/* Utility library. */

#include "stringops.h"

/* mystrtok - safe tokenizer */

char   *mystrtok(char **src, const char *sep)
{
    char   *start = *src;
    char   *end;

    /*
     * Skip over leading delimiters.
     */
    start += strspn(start, sep);
    if (*start == 0) {
	*src = start;
	return (0);
    }

    /*
     * Separate off one token.
     */
    end = start + strcspn(start, sep);
    if (*end != 0)
	*end++ = 0;
    *src = end;
    return (start);
}

/* mystrtokq - safe tokenizer with quoting support */

char   *mystrtokq(char **src, const char *sep, const char *parens)
{
    char   *start = *src;
    static char   *cp;
    int     ch;
    int     level;

    /*
     * Skip over leading delimiters.
     */
    start += strspn(start, sep);
    if (*start == 0) {
	*src = start;
	return (0);
    }

    /*
     * Parse out the next token.
     */
    for (level = 0, cp = start; (ch = *(unsigned char *) cp) != 0; cp++) {
	if (ch == parens[0]) {
	    level++;
        } else if (level > 0 && ch == parens[1]) {
	    level--;
	} else if (level == 0 && strchr(sep, ch) != 0) {
	    *cp++ = 0;
	    break;
	}
    }
    *src = cp;
    return (start);
}

#ifdef TEST

 /*
  * Test program: read lines from stdin, split on whitespace.
  */
#include "vstring.h"
#include "vstream.h"
#include "vstring_vstream.h"

int     main(void)
{
    VSTRING *vp = vstring_alloc(100);
    char   *start;
    char   *str;

    while (vstring_fgets(vp, VSTREAM_IN) && VSTRING_LEN(vp) > 0) {
	start = vstring_str(vp);
	if (strchr(start, CHARS_BRACE[0]) == 0) {
	    while ((str = mystrtok(&start, CHARS_SPACE)) != 0)
		vstream_printf(">%s<\n", str);
	} else {
	    while ((str = mystrtokq(&start, CHARS_SPACE, CHARS_BRACE)) != 0)
		vstream_printf(">%s<\n", str);
	}
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(vp);
    return (0);
}

#endif
