/*++
/* NAME
/*	stream2rec 1
/* SUMMARY
/*	convert stream-lf data to record format
/* SYNOPSIS
/*	stream2rec
/* DESCRIPTION
/*	stream2rec reads lines from standard input and writes
/*	them to standard output in record form.
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

#include <vstream.h>
#include <vstring.h>

/* Global library. */

#include <record.h>
#include <rec_streamlf.h>

int     main(void)
{
    VSTRING *buf = vstring_alloc(150);
    int     type;

    while ((type = rec_streamlf_get(VSTREAM_IN, buf, 150)) > 0)
	REC_PUT_BUF(VSTREAM_OUT, type, buf);
    vstream_fflush(VSTREAM_OUT);
    return (0);
}
