/*++
/* NAME
/*	readlline 3
/* SUMMARY
/*	read logical line
/* SYNOPSIS
/*	#include <readlline.h>
/*
/*	VSTRING	*readlline(buf, fp, lineno, stripnl)
/*	VSTRING	*buf;
/*	VSTREAM	*fp;
/*	int	*lineno;
/*	int	stripnl;
/* DESCRIPTION
/*	readlline() reads one logical line from the named stream.
/*	A line that starts with whitespace is a continuation of
/*	the previous line. When the stripnl argument is non-zero,
/*	the newline between continued lines
/*	is deleted from the input. The result value is the input
/*	buffer argument or a null pointer when no input is found.
/*
/*	Arguments:
/* .IP buf
/*	A variable-length buffer for input.
/* .IP fp
/*	Handle to an open stream.
/* .IP lineno
/*	A null pointer, or a pointer to an integer that is incremented
/*	after reading a newline.
/* .IP stripnl
/*	Non-zero to strip newlines. readlline.h provides the symbolic
/*	constants READLL_STRIPNL and READLL_KEEPNL for convenience.
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

/* Utility library. */

#include "vstream.h"
#include "vstring.h"
#include "readlline.h"

/* readlline - read one logical line */

VSTRING *readlline(VSTRING *buf, VSTREAM *fp, int *lineno, int stripnl)
{
    int     ch;
    int     next;

    /*
     * Lines that start with whitespace continue the preceding line.
     */
    VSTRING_RESET(buf);
    while ((ch = VSTREAM_GETC(fp)) != VSTREAM_EOF) {
	if (ch == '\n') {
	    if (stripnl == 0)
		VSTRING_ADDCH(buf, ch);
	    if (lineno)
		*lineno += 1;
	    if ((next = VSTREAM_GETC(fp)) == ' ' || next == '\t') {
		ch = next;
	    } else {
		if (next != VSTREAM_EOF)
		    vstream_ungetc(fp, next);
		break;
	    }
	}
	VSTRING_ADDCH(buf, ch);
    }
    VSTRING_TERMINATE(buf);
    return (VSTRING_LEN(buf) || ch == '\n' ? buf : 0);
}
