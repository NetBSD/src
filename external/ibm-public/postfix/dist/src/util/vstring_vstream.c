/*	$NetBSD: vstring_vstream.c,v 1.2 2020/03/18 19:05:22 christos Exp $	*/

/*++
/* NAME
/*	vstring_vstream 3
/* SUMMARY
/*	auto-resizing string library, standard I/O interface
/* SYNOPSIS
/*	#include <vstring_vstream.h>
/*
/*	int	vstring_get_flags(vp, fp, flags)
/*	VSTRING	*vp;
/*	VSTREAM	*fp;
/*	int	flags
/*
/*	int	vstring_get_flags_nonl(vp, fp, flags)
/*	VSTRING	*vp;
/*	VSTREAM	*fp;
/*	int	flags
/*
/*	int	vstring_get_flags_null(vp, fp, flags)
/*	VSTRING	*vp;
/*	VSTREAM	*fp;
/*	int	flags
/*
/*	int	vstring_get_flags_bound(vp, fp, flags, bound)
/*	VSTRING	*vp;
/*	VSTREAM	*fp;
/*	ssize_t	bound;
/*	int	flags
/*
/*	int	vstring_get_flags_nonl_bound(vp, fp, flags, bound)
/*	VSTRING	*vp;
/*	VSTREAM	*fp;
/*	ssize_t	bound;
/*	int	flags
/*
/*	int	vstring_get_flags_null_bound(vp, fp, flags, bound)
/*	VSTRING	*vp;
/*	VSTREAM	*fp;
/*	ssize_t	bound;
/*	int	flags
/* CONVENIENCE API
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
/*	The flags argument is VSTRING_GET_FLAG_NONE (default) or
/*	VSTRING_GET_FLAG_APPEND (append instead of overwrite).
/*
/*	vstring_get_flags() reads one line from the named stream, including the
/*	terminating newline character if present.
/*
/*	vstring_get_flags_nonl() reads a line from the named stream and strips
/*	the trailing newline character.
/*
/*	vstring_get_flags_null() reads a null-terminated string from the named
/*	stream.
/*
/*	the vstring_get_flags<whatever>_bound() routines read no more
/*	than \fIbound\fR characters.  Otherwise they behave like the
/*	unbounded versions documented above.
/*
/*	The functions without _flags in their name accept the same
/*	arguments except flags. These functions use the default
/*	flags value.
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
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
#define VSTRING_GET_RESULT(vp, baselen) \
    (VSTRING_LEN(vp) > (base_len) ? vstring_end(vp)[-1] : VSTREAM_EOF)

/* vstring_get_flags - read line from file, keep newline */

int     vstring_get_flags(VSTRING *vp, VSTREAM *fp, int flags)
{
    int     c;
    ssize_t base_len;

    if ((flags & VSTRING_GET_FLAG_APPEND) == 0)
	VSTRING_RESET(vp);
    base_len = VSTRING_LEN(vp);
    while ((c = VSTREAM_GETC(fp)) != VSTREAM_EOF) {
	VSTRING_ADDCH(vp, c);
	if (c == '\n')
	    break;
    }
    VSTRING_TERMINATE(vp);
    return (VSTRING_GET_RESULT(vp, baselen));
}

/* vstring_get_flags_nonl - read line from file, strip newline */

int     vstring_get_flags_nonl(VSTRING *vp, VSTREAM *fp, int flags)
{
    int     c;
    ssize_t base_len;

    if ((flags & VSTRING_GET_FLAG_APPEND) == 0)
	VSTRING_RESET(vp);
    base_len = VSTRING_LEN(vp);
    while ((c = VSTREAM_GETC(fp)) != VSTREAM_EOF && c != '\n')
	VSTRING_ADDCH(vp, c);
    VSTRING_TERMINATE(vp);
    return (c == '\n' ? c : VSTRING_GET_RESULT(vp, baselen));
}

/* vstring_get_flags_null - read null-terminated string from file */

int     vstring_get_flags_null(VSTRING *vp, VSTREAM *fp, int flags)
{
    int     c;
    ssize_t base_len;

    if ((flags & VSTRING_GET_FLAG_APPEND) == 0)
	VSTRING_RESET(vp);
    base_len = VSTRING_LEN(vp);
    while ((c = VSTREAM_GETC(fp)) != VSTREAM_EOF && c != 0)
	VSTRING_ADDCH(vp, c);
    VSTRING_TERMINATE(vp);
    return (c == 0 ? c : VSTRING_GET_RESULT(vp, baselen));
}

/* vstring_get_flags_bound - read line from file, keep newline, up to bound */

int     vstring_get_flags_bound(VSTRING *vp, VSTREAM *fp, int flags,
				        ssize_t bound)
{
    int     c;
    ssize_t base_len;

    if (bound <= 0)
	msg_panic("vstring_get_bound: invalid bound %ld", (long) bound);

    if ((flags & VSTRING_GET_FLAG_APPEND) == 0)
	VSTRING_RESET(vp);
    base_len = VSTRING_LEN(vp);
    while (bound-- > 0 && (c = VSTREAM_GETC(fp)) != VSTREAM_EOF) {
	VSTRING_ADDCH(vp, c);
	if (c == '\n')
	    break;
    }
    VSTRING_TERMINATE(vp);
    return (VSTRING_GET_RESULT(vp, baselen));
}

/* vstring_get_flags_nonl_bound - read line from file, strip newline, up to bound */

int     vstring_get_flags_nonl_bound(VSTRING *vp, VSTREAM *fp, int flags,
				             ssize_t bound)
{
    int     c;
    ssize_t base_len;

    if (bound <= 0)
	msg_panic("vstring_get_nonl_bound: invalid bound %ld", (long) bound);

    if ((flags & VSTRING_GET_FLAG_APPEND) == 0)
	VSTRING_RESET(vp);
    base_len = VSTRING_LEN(vp);
    while (bound-- > 0 && (c = VSTREAM_GETC(fp)) != VSTREAM_EOF && c != '\n')
	VSTRING_ADDCH(vp, c);
    VSTRING_TERMINATE(vp);
    return (c == '\n' ? c : VSTRING_GET_RESULT(vp, baselen));
}

/* vstring_get_flags_null_bound - read null-terminated string from file */

int     vstring_get_flags_null_bound(VSTRING *vp, VSTREAM *fp, int flags,
				             ssize_t bound)
{
    int     c;
    ssize_t base_len;

    if (bound <= 0)
	msg_panic("vstring_get_null_bound: invalid bound %ld", (long) bound);

    if ((flags & VSTRING_GET_FLAG_APPEND) == 0)
	VSTRING_RESET(vp);
    base_len = VSTRING_LEN(vp);
    while (bound-- > 0 && (c = VSTREAM_GETC(fp)) != VSTREAM_EOF && c != 0)
	VSTRING_ADDCH(vp, c);
    VSTRING_TERMINATE(vp);
    return (c == 0 ? c : VSTRING_GET_RESULT(vp, baselen));
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
