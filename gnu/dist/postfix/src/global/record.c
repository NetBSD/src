/*++
/* NAME
/*	record 3
/* SUMMARY
/*	simple typed record I/O
/* SYNOPSIS
/*	#include <record.h>
/*
/*	int	rec_get(stream, buf, maxsize)
/*	VSTREAM	*stream;
/*	VSTRING	*buf;
/*	int	maxsize;
/*
/*	int	rec_put(stream, type, data, len)
/*	VSTREAM	*stream;
/*	int	type;
/*	const char *data;
/*	int	len;
/* AUXILIARY FUNCTIONS
/*	int	rec_put_type(stream, type, offset)
/*	VSTREAM	*stream;
/*	int	type;
/*	long	offset;
/*
/*	int	rec_fprintf(stream, type, format, ...)
/*	VSTREAM	*stream;
/*	int	type;
/*	const char *format;
/*
/*	int	rec_fputs(stream, type, str)
/*	VSTREAM	*stream;
/*	int	type;
/*	const char *str;
/*
/*	int	REC_PUT_BUF(stream, type, buf)
/*	VSTREAM	*stream;
/*	int	type;
/*	VSTRING	*buf;
/*
/*	int	rec_vfprintf(stream, type, format, ap)
/*	VSTREAM	*stream;
/*	int	type;
/*	const char *format;
/*	va_list	ap;
/* DESCRIPTION
/*	This module reads and writes typed variable-length records.
/*	Each record contains a 1-byte type code (0..255), a length
/*	(1 or more bytes) and as much data as the length specifies.
/*
/*	rec_get() retrieves a record from the named record stream
/*	and returns the record type. The \fImaxsize\fR argument is
/*	zero, or specifies a maximal acceptable record length.
/*	The result is REC_TYPE_EOF when the end of the file was reached,
/*	and REC_TYPE_ERROR in case of a bad record. The result buffer is
/*	null-terminated for convenience. Records may contain embedded
/*	null characters.
/*
/*	rec_put() stores the specified record and returns the record
/*	type, or REC_TYPE_ERROR in case of problems.
/*
/*	rec_put_type() updates the type field of the record at the
/*	specified file offset.  The result is the new record type,
/*	or REC_TYPE_ERROR in case of trouble.
/*
/*	rec_fprintf() and rec_vfprintf() format their arguments and
/*	write the result to the named stream. The result is the same
/*	as with rec_put().
/*
/*	rec_fputs() writes a record with as contents a copy of the
/*	specified string. The result is the same as with rec_put().
/*
/*	REC_PUT_BUF() is a wrapper for rec_put() that makes it
/*	easier to handle VSTRING buffers. It is an unsafe macro
/*	that evaluates some arguments more than once.
/* DIAGNOSTICS
/*	Panics: interface violations. Fatal errors: insufficient memory.
/*	Warnings: corrupted file.
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
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#ifndef NBBY
#define NBBY 8				/* XXX should be in sys_defs.h */
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstream.h>
#include <vstring.h>

/* Global library. */

#include <record.h>

/* rec_put_type - update record type field */

int     rec_put_type(VSTREAM *stream, int type, long offset)
{
    if (msg_verbose > 2)
	msg_info("rec_put_type: %d at %ld", type, offset);

    if (vstream_fseek(stream, offset, SEEK_SET) < 0
	|| VSTREAM_PUTC(type, stream) != type) {
	return (REC_TYPE_ERROR);
    } else {
	return (type);
    }
}

/* rec_put - store typed record */

int     rec_put(VSTREAM *stream, int type, const char *data, int len)
{
    int     len_rest;
    int     len_byte;

    if (msg_verbose > 2)
	msg_info("rec_put: type %c len %d data %.10s", type, len, data);

    /*
     * Write the record type, one byte.
     */
    if (VSTREAM_PUTC(type, stream) == VSTREAM_EOF)
	return (REC_TYPE_ERROR);

    /*
     * Write the record data length in 7-bit portions, using the 8th bit to
     * indicate that there is more. Use as many length bytes as needed.
     */
    len_rest = len;
    do {
	len_byte = len_rest & 0177;
	if (len_rest >>= 7)
	    len_byte |= 0200;
	if (VSTREAM_PUTC(len_byte, stream) == VSTREAM_EOF) {
	    return (REC_TYPE_ERROR);
	}
    } while (len_rest != 0);

    /*
     * Write the record data portion. Use as many length bytes as needed.
     */
    if (len && vstream_fwrite(stream, data, len) != len)
	return (REC_TYPE_ERROR);
    return (type);
}

/* rec_get - retrieve typed record */

int     rec_get(VSTREAM *stream, VSTRING *buf, int maxsize)
{
    char   *myname = "rec_get";
    int     type;
    int     len;
    int     len_byte;
    int     shift;

    /*
     * Sanity check.
     */
    if (maxsize < 0)
	msg_panic("%s: bad record size limit: %d", myname, maxsize);

    /*
     * Extract the record type.
     */
    if ((type = VSTREAM_GETC(stream)) == VSTREAM_EOF)
	return (REC_TYPE_EOF);

    /*
     * Find out the record data length. Return an error result when the
     * record data length is malformed or when it exceeds the acceptable
     * limit.
     */
    for (len = 0, shift = 0; /* void */ ; shift += 7) {
	if (shift >= (int) (NBBY * sizeof(int))) {
	    msg_warn("%s: too many length bits, record type %d",
		     VSTREAM_PATH(stream), type);
	    return (REC_TYPE_ERROR);
	}
	if ((len_byte = VSTREAM_GETC(stream)) == VSTREAM_EOF) {
	    msg_warn("%s: unexpected EOF reading length, record type %d",
		     VSTREAM_PATH(stream), type);
	    return (REC_TYPE_ERROR);
	}
	len |= (len_byte & 0177) << shift;
	if ((len_byte & 0200) == 0)
	    break;
    }
    if (len < 0 || (maxsize > 0 && len > maxsize)) {
	msg_warn("%s: illegal length %d, record type %d",
		 VSTREAM_PATH(stream), len, type);
	while (len-- > 0 && VSTREAM_GETC(stream) != VSTREAM_EOF)
	     /* void */ ;
	return (REC_TYPE_ERROR);
    }

    /*
     * Reserve buffer space for the result, and read the record data into the
     * buffer.
     */
    VSTRING_RESET(buf);
    VSTRING_SPACE(buf, len);
    if (vstream_fread(stream, vstring_str(buf), len) != len) {
	msg_warn("%s: unexpected EOF in data, record type %d length %d",
		 VSTREAM_PATH(stream), type, len);
	return (REC_TYPE_ERROR);
    }
    VSTRING_AT_OFFSET(buf, len);
    VSTRING_TERMINATE(buf);
    if (msg_verbose > 2)
	msg_info("%s: type %c len %d data %.10s", myname,
		 type, len, vstring_str(buf));
    return (type);
}

/* rec_vfprintf - write formatted string to record */

int     rec_vfprintf(VSTREAM *stream, int type, const char *format, va_list ap)
{
    static VSTRING *vp;

    if (vp == 0)
	vp = vstring_alloc(100);

    /*
     * Writing a formatted string involves an extra copy, because we must
     * know the record length before we can write it.
     */
    vstring_vsprintf(vp, format, ap);
    return (REC_PUT_BUF(stream, type, vp));
}

/* rec_fprintf - write formatted string to record */

int     rec_fprintf(VSTREAM *stream, int type, const char *format,...)
{
    int     result;
    va_list ap;

    va_start(ap, format);
    result = rec_vfprintf(stream, type, format, ap);
    va_end(ap);
    return (result);
}

/* rec_fputs - write string to record */

int     rec_fputs(VSTREAM *stream, int type, const char *str)
{
    return (rec_put(stream, type, str, str ? strlen(str) : 0));
}
