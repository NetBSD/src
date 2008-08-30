/*++
/* NAME
/*	rec_streamlf 3
/* SUMMARY
/*	record interface to stream-lf files
/* SYNOPSIS
/*	#include <rec_streamlf.h>
/*
/*	int	rec_streamlf_get(stream, buf, maxlen)
/*	VSTREAM	*stream;
/*	VSTRING	*buf;
/*	int	maxlen;
/*
/*	int	rec_streamlf_put(stream, type, data, len)
/*	VSTREAM	*stream;
/*	int	type;
/*	const char *data;
/*	int	len;
/* AUXILIARY FUNCTIONS
/*	int	REC_STREAMLF_PUT_BUF(stream, type, buf)
/*	VSTREAM	*stream;
/*	int	type;
/*	VSTRING	*buf;
/* DESCRIPTION
/*	This module implements record I/O on top of stream-lf files.
/*
/*	rec_streamlf_get() reads one record from the specified stream.
/*	The result is null-terminated and may contain embedded null
/*	characters.
/*	The \fImaxlen\fR argument specifies an upper bound to the amount
/*	of data read. The result is REC_TYPE_NORM when the record was
/*	terminated by newline, REC_TYPE_CONT when no terminating newline
/*	was found (the record was larger than \fImaxlen\fR characters or
/*	EOF was reached), and REC_TYPE_EOF when no data could be read or
/*	when an I/O error was detected.
/*
/*	rec_streamlf_put() writes one record to the named stream.
/*	When the record type is REC_TYPE_NORM, a newline character is
/*	appended to the output. The result is the record type, or
/*	REC_TYPE_EOF in case of problems.
/*
/*	REC_STREAMLF_PUT_BUF() is a wrapper for rec_streamlf_put() that
/*	makes it more convenient to output VSTRING buffers.
/*	REC_STREAMLF_PUT_BUF() is an unsafe macro that evaluates some
/*	arguments more than once.
/* SEE ALSO
/*	record(3) typed records
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
#include <rec_type.h>
#include <rec_streamlf.h>

/* rec_streamlf_get - read record from stream-lf file */

int     rec_streamlf_get(VSTREAM *stream, VSTRING *buf, int maxlen)
{
    int     n = maxlen;
    int     ch;

    /*
     * If this one character ar a time code proves to be a performance
     * bottleneck, switch to block search (memchr()) and to block move
     * (memcpy()) operations.
     */
    VSTRING_RESET(buf);
    while (n-- > 0) {
	if ((ch = VSTREAM_GETC(stream)) == VSTREAM_EOF)
	    return (VSTRING_LEN(buf) > 0 ? REC_TYPE_CONT : REC_TYPE_EOF);
	if (ch == '\n') {
	    VSTRING_TERMINATE(buf);
	    return (REC_TYPE_NORM);
	}
	VSTRING_ADDCH(buf, ch);
    }
    VSTRING_TERMINATE(buf);
    return (REC_TYPE_CONT);
}

/* rec_streamlf_put - write record to stream-lf file */

int     rec_streamlf_put(VSTREAM *stream, int type, const char *data, int len)
{
    if (len > 0)
	(void) vstream_fwrite(stream, data, len);
    if (type == REC_TYPE_NORM)
	(void) VSTREAM_PUTC('\n', stream);
    return (vstream_ferror(stream) ? REC_TYPE_EOF : type);
}
