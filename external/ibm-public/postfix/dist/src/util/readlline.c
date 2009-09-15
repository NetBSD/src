/*	$NetBSD: readlline.c,v 1.1.1.1.2.2 2009/09/15 06:04:02 snj Exp $	*/

/*++
/* NAME
/*	readlline 3
/* SUMMARY
/*	read logical line
/* SYNOPSIS
/*	#include <readlline.h>
/*
/*	VSTRING	*readlline(buf, fp, lineno)
/*	VSTRING	*buf;
/*	VSTREAM	*fp;
/*	int	*lineno;
/* DESCRIPTION
/*	readlline() reads one logical line from the named stream.
/* .IP "blank lines and comments"
/*	Empty lines and whitespace-only lines are ignored, as
/*	are lines whose first non-whitespace character is a `#'.
/* .IP "multi-line text"
/*	A logical line starts with non-whitespace text. A line that
/*	starts with whitespace continues a logical line.
/* .PP
/*	The result value is the input buffer argument or a null pointer
/*	when no input is found.
/*
/*	Arguments:
/* .IP buf
/*	A variable-length buffer for input. The result is null terminated.
/* .IP fp
/*	Handle to an open stream.
/* .IP lineno
/*	A null pointer, or a pointer to an integer that is incremented
/*	after reading a newline character.
/* .RE
/* DIAGNOSTICS
/*	Warning: a continuation line that does not continue preceding text.
/*	The invalid input is ignored, to avoid complicating caller code.
/* SECURITY
/* .ad
/* .fi
/*	readlline() imposes no logical line length limit therefore it
/*	should be used for reading trusted information only.
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
#include <ctype.h>

/* Utility library. */

#include "msg.h"
#include "vstream.h"
#include "vstring.h"
#include "readlline.h"

#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)
#define END(x) vstring_end(x)

/* readlline - read one logical line */

VSTRING *readlline(VSTRING *buf, VSTREAM *fp, int *lineno)
{
    int     ch;
    int     next;
    ssize_t start;
    char   *cp;

    VSTRING_RESET(buf);

    /*
     * Ignore comment lines, all whitespace lines, and empty lines. Terminate
     * at EOF or at the beginning of the next logical line.
     */
    for (;;) {
	/* Read one line, possibly not newline terminated. */
	start = LEN(buf);
	while ((ch = VSTREAM_GETC(fp)) != VSTREAM_EOF && ch != '\n')
	    VSTRING_ADDCH(buf, ch);
	if (ch == '\n' && lineno != 0)
	    *lineno += 1;
	/* Ignore comment line, all whitespace line, or empty line. */
	for (cp = STR(buf) + start; cp < END(buf) && ISSPACE(*cp); cp++)
	     /* void */ ;
	if (cp == END(buf) || *cp == '#')
	    vstring_truncate(buf, start);
	/* Terminate at EOF or at the beginning of the next logical line. */
	if (ch == VSTREAM_EOF)
	    break;
	if (LEN(buf) > 0) {
	    if ((next = VSTREAM_GETC(fp)) != VSTREAM_EOF)
		vstream_ungetc(fp, next);
	    if (next != '#' && !ISSPACE(next))
		break;
	}
    }
    VSTRING_TERMINATE(buf);

    /*
     * Invalid input: continuing text without preceding text. Allowing this
     * would complicate "postconf -e", which implements its own multi-line
     * parsing routine. Do not abort, just warn, so that critical programs
     * like postmap do not leave behind a truncated table.
     */
    if (LEN(buf) > 0 && ISSPACE(*STR(buf))) {
	msg_warn("%s: logical line must not start with whitespace: \"%.30s%s\"",
		 VSTREAM_PATH(fp), STR(buf),
		 LEN(buf) > 30 ? "..." : "");
	return (readlline(buf, fp, lineno));
    }

    /*
     * Done.
     */
    return (LEN(buf) > 0 ? buf : 0);
}
