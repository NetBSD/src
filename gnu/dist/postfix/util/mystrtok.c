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
/* DESCRIPTION
/*	mystrtok() splits a buffer on the specified \fIdelimiters\fR.
/*	Tokens are delimited by runs of delimiters, so this routine
/*	cannot return zero-length tokens.
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

    while (vstring_fgets(vp, VSTREAM_IN)) {
	start = vstring_str(vp);
	while ((str = mystrtok(&start, " \t\r\n")) != 0)
	    vstream_printf(">%s<\n", str);
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(vp);
}

#endif
