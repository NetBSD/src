/*	$NetBSD: rec2stream.c,v 1.1.1.1.2.2 2009/09/15 06:02:52 snj Exp $	*/

/*++
/* NAME
/*	rec2stream 1
/* SUMMARY
/*	convert record stream to stream-lf format
/* SYNOPSIS
/*	rec2stream
/* DESCRIPTION
/*	rec2stream reads a record stream from standard input and
/*	writes the content to standard output in stream-lf format.
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream.
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

#include <vstring.h>
#include <vstream.h>

/* Global library. */

#include <record.h>
#include <rec_streamlf.h>

int     main(void)
{
    VSTRING *buf = vstring_alloc(100);
    int     type;

    while ((type = rec_get(VSTREAM_IN, buf, 0)) > 0)
	REC_STREAMLF_PUT_BUF(VSTREAM_OUT, type, buf);
    vstream_fflush(VSTREAM_OUT);
    return (0);
}
