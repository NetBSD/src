/*	$NetBSD: vstring_vstream.c,v 1.1.1.1.2.2 2009/09/15 06:04:05 snj Exp $	*/

/*++
/* NAME
/*	vstring_vstream 3
/* SUMMARY
/*	auto-resizing string library, standard I/O interface
/* SYNOPSIS
/*	#include <vstring_vstream.h>
/*
/*	int	vstring_get(vp, fp)
/*	VSTRING	*vp;
/*	VSTREAM	*fp;
/*
/*	int	vstring_get_nonl(vp, fp)
/*	VSTRING	*vp;
/*	VSTREAM	*fp;
/*
/*	int	vstring_get_null(vp, fp)
/*	VSTRING	*vp;
/*	VSTREAM	*fp;
/*
/*	int	vstring_get_bound(vp, fp, bound)
/*	VSTRING	*vp;
/*	VSTREAM	*fp;
/*	ssize_t	bound;
/*
/*	int	vstring_get_nonl_bound(vp, fp, bound)
/*	VSTRING	*vp;
/*	VSTREAM	*fp;
/*	ssize_t	bound;
/*
/*	int	vstring_get_null_bound(vp, fp, bound)
/*	VSTRING	*vp;
/*	VSTREAM	*fp;
/*	ssize_t	bound;
/* DESCRIPTION
/*	The routines in this module each read one newline or null-terminated
/*	string from an input stream. In all cases the result is either the
/*	last character read, typically the record terminator, or VSTREAM_EOF.
/*
/*	vstring_get() reads one line from the named stream, including the
/*	terminating newline character if present.
/*
/*	vstring_get_nonl() reads a line from the named stream and strips
/*	the trailing newline character.
/*
/*	vstring_get_null() reads a null-terminated string from the named
/*	stream.
/*
/*	the vstring_get<whatever>_bound() routines read no more
/*	than \fIbound\fR characters.  Otherwise they behave like the
/*	unbounded versions documented above.
/* DIAGNOSTICS
/*	Fatal errors: memory allocation failure.
/*	Panic: improper string bound.
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
#include <stdio.h>
#include <string.h>

/* Application-specific. */

#include "msg.h"
#include "vstring.h"
#include "vstream.h"
#include "vstring_vstream.h"

 /*
  * Macro to return the last character added to a VSTRING, for consistency.
  */
#define VSTRING_GET_RESULT(vp) \
    (VSTRING_LEN(vp) > 0 ? vstring_end(vp)[-1] : VSTREAM_EOF)

/* vstring_get - read line from file, keep newline */

int     vstring_get(VSTRING *vp, VSTREAM *fp)
{
    int     c;

    VSTRING_RESET(vp);
    while ((c = VSTREAM_GETC(fp)) != VSTREAM_EOF) {
	VSTRING_ADDCH(vp, c);
	if (c == '\n')
	    break;
    }
    VSTRING_TERMINATE(vp);
    return (VSTRING_GET_RESULT(vp));
}

/* vstring_get_nonl - read line from file, strip newline */

int     vstring_get_nonl(VSTRING *vp, VSTREAM *fp)
{
    int     c;

    VSTRING_RESET(vp);
    while ((c = VSTREAM_GETC(fp)) != VSTREAM_EOF && c != '\n')
	VSTRING_ADDCH(vp, c);
    VSTRING_TERMINATE(vp);
    return (c == '\n' ? c : VSTRING_GET_RESULT(vp));
}

/* vstring_get_null - read null-terminated string from file */

int     vstring_get_null(VSTRING *vp, VSTREAM *fp)
{
    int     c;

    VSTRING_RESET(vp);
    while ((c = VSTREAM_GETC(fp)) != VSTREAM_EOF && c != 0)
	VSTRING_ADDCH(vp, c);
    VSTRING_TERMINATE(vp);
    return (c == 0 ? c : VSTRING_GET_RESULT(vp));
}

/* vstring_get_bound - read line from file, keep newline, up to bound */

int     vstring_get_bound(VSTRING *vp, VSTREAM *fp, ssize_t bound)
{
    int     c;

    if (bound <= 0)
	msg_panic("vstring_get_bound: invalid bound %ld", (long) bound);

    VSTRING_RESET(vp);
    while (bound-- > 0 && (c = VSTREAM_GETC(fp)) != VSTREAM_EOF) {
	VSTRING_ADDCH(vp, c);
	if (c == '\n')
	    break;
    }
    VSTRING_TERMINATE(vp);
    return (VSTRING_GET_RESULT(vp));
}

/* vstring_get_nonl_bound - read line from file, strip newline, up to bound */

int     vstring_get_nonl_bound(VSTRING *vp, VSTREAM *fp, ssize_t bound)
{
    int     c;

    if (bound <= 0)
	msg_panic("vstring_get_nonl_bound: invalid bound %ld", (long) bound);

    VSTRING_RESET(vp);
    while (bound-- > 0 && (c = VSTREAM_GETC(fp)) != VSTREAM_EOF && c != '\n')
	VSTRING_ADDCH(vp, c);
    VSTRING_TERMINATE(vp);
    return (c == '\n' ? c : VSTRING_GET_RESULT(vp));
}

/* vstring_get_null_bound - read null-terminated string from file */

int     vstring_get_null_bound(VSTRING *vp, VSTREAM *fp, ssize_t bound)
{
    int     c;

    if (bound <= 0)
	msg_panic("vstring_get_nonl_bound: invalid bound %ld", (long) bound);

    VSTRING_RESET(vp);
    while (bound-- > 0 && (c = VSTREAM_GETC(fp)) != VSTREAM_EOF && c != 0)
	VSTRING_ADDCH(vp, c);
    VSTRING_TERMINATE(vp);
    return (c == 0 ? c : VSTRING_GET_RESULT(vp));
}

#ifdef TEST

 /*
  * Proof-of-concept test program: copy the source to this module to stdout.
  */
#include <fcntl.h>

#define TEXT_VSTREAM    "vstring_vstream.c"

int     main(void)
{
    VSTRING *vp = vstring_alloc(1);
    VSTREAM *fp;

    if ((fp = vstream_fopen(TEXT_VSTREAM, O_RDONLY, 0)) == 0)
	msg_fatal("open %s: %m", TEXT_VSTREAM);
    while (vstring_fgets(vp, fp))
	vstream_fprintf(VSTREAM_OUT, "%s", vstring_str(vp));
    vstream_fclose(fp);
    vstream_fflush(VSTREAM_OUT);
    vstring_free(vp);
    return (0);
}

#endif
