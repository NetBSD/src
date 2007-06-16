/*	$NetBSD: record.c,v 1.1.1.6.2.1 2007/06/16 17:00:13 snj Exp $	*/

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
/*	ssize_t	maxsize;
/*
/*	int	rec_get_raw(stream, buf, maxsize, flags)
/*	VSTREAM	*stream;
/*	VSTRING	*buf;
/*	ssize_t	maxsize;
/*	int	flags;
/*
/*	int	rec_put(stream, type, data, len)
/*	VSTREAM	*stream;
/*	int	type;
/*	const char *data;
/*	ssize_t	len;
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
/*
/*	int	rec_goto(stream, where)
/*	VSTREAM	*stream;
/*	const char *where;
/*
/*	int	rec_pad(stream, type, len)
/*	VSTREAM *stream;
/*	int	type;
/*	int	len;
/*
/*	REC_SPACE_NEED(buflen, reclen)
/*	ssize_t	buflen;
/*	ssize_t	reclen;
/* DESCRIPTION
/*	This module reads and writes typed variable-length records.
/*	Each record contains a 1-byte type code (0..255), a length
/*	(1 or more bytes) and as much data as the length specifies.
/*
/*	rec_get_raw() retrieves a record from the named record stream
/*	and returns the record type. The \fImaxsize\fR argument is
/*	zero, or specifies a maximal acceptable record length.
/*	The result is REC_TYPE_EOF when the end of the file was reached,
/*	and REC_TYPE_ERROR in case of a bad record. The result buffer is
/*	null-terminated for convenience. Records may contain embedded
/*	null characters. The \fIflags\fR argument specifies zero or
/*	more of the following:
/* .IP REC_FLAG_FOLLOW_PTR
/*	Follow PTR records, instead of exposing them to the application.
/* .IP REC_FLAG_SKIP_DTXT
/*	Skip "deleted text" records, instead of exposing them to
/*	the application.
/* .IP REC_FLAG_SEEK_END
/*	Seek to the end-of-file upon reading a REC_TYPE_END record.
/* .PP
/*	Specify REC_FLAG_NONE to request no special processing,
/*	and REC_FLAG_DEFAULT for normal use.
/*
/*	rec_get() is a wrapper around rec_get_raw() that always
/*	enables the REC_FLAG_FOLLOW_PTR, REC_FLAG_SKIP_DTXT
/*	and REC_FLAG_SEEK_END features.
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
/*
/*	rec_goto() takes the argument of a pointer record and moves
/*	the file pointer to the specified location. A zero position
/*	means do nothing. The result is REC_TYPE_ERROR in case of
/*	failure.
/*
/*	rec_pad() writes a record that occupies the larger of (the
/*	specified amount) or (an implementation-defined minimum).
/*
/*	REC_SPACE_NEED(buflen, reclen) converts the specified buffer
/*	length into a record length. This macro modifies its second
/*	argument.
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
#include <stringops.h>

/* Global library. */

#include <off_cvt.h>
#include <rec_type.h>
#include <record.h>

/* rec_put_type - update record type field */

int     rec_put_type(VSTREAM *stream, int type, off_t offset)
{
    if (type < 0 || type > 255)
	msg_panic("rec_put_type: bad record type %d", type);

    if (msg_verbose > 2)
	msg_info("rec_put_type: %d at %ld", type, (long) offset);

    if (vstream_fseek(stream, offset, SEEK_SET) < 0
	|| VSTREAM_PUTC(type, stream) != type) {
	return (REC_TYPE_ERROR);
    } else {
	return (type);
    }
}

/* rec_put - store typed record */

int     rec_put(VSTREAM *stream, int type, const char *data, ssize_t len)
{
    ssize_t len_rest;
    int     len_byte;

    if (type < 0 || type > 255)
	msg_panic("rec_put: bad record type %d", type);

    if (msg_verbose > 2)
	msg_info("rec_put: type %c len %ld data %.10s",
		 type, (long) len, data);

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
	if (len_rest >>= 7U)
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

/* rec_get_raw - retrieve typed record */

int     rec_get_raw(VSTREAM *stream, VSTRING *buf, ssize_t maxsize, int flags)
{
    const char *myname = "rec_get";
    int     type;
    ssize_t len;
    int     len_byte;
    unsigned shift;

    /*
     * Sanity check.
     */
    if (maxsize < 0)
	msg_panic("%s: bad record size limit: %ld", myname, (long) maxsize);

    for (;;) {

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
	    msg_warn("%s: illegal length %ld, record type %d",
		     VSTREAM_PATH(stream), (long) len, type);
	    while (len-- > 0 && VSTREAM_GETC(stream) != VSTREAM_EOF)
		 /* void */ ;
	    return (REC_TYPE_ERROR);
	}

	/*
	 * Reserve buffer space for the result, and read the record data into
	 * the buffer.
	 */
	VSTRING_RESET(buf);
	VSTRING_SPACE(buf, len);
	if (vstream_fread(stream, vstring_str(buf), len) != len) {
	    msg_warn("%s: unexpected EOF in data, record type %d length %ld",
		     VSTREAM_PATH(stream), type, (long) len);
	    return (REC_TYPE_ERROR);
	}
	VSTRING_AT_OFFSET(buf, len);
	VSTRING_TERMINATE(buf);
	if (msg_verbose > 2)
	    msg_info("%s: type %c len %ld data %.10s", myname,
		     type, (long) len, vstring_str(buf));

	/*
	 * Transparency options.
	 */
	if (flags == 0)
	    break;
	if (type == REC_TYPE_PTR && (flags & REC_FLAG_FOLLOW_PTR) != 0
	    && (type = rec_goto(stream, vstring_str(buf))) != REC_TYPE_ERROR)
	    continue;
	if (type == REC_TYPE_DTXT && (flags & REC_FLAG_SKIP_DTXT) != 0)
	    continue;
	if (type == REC_TYPE_END && (flags & REC_FLAG_SEEK_END) != 0)
	    (void) vstream_fseek(stream, (off_t) 0, SEEK_END);
	break;
    }
    return (type);
}

/* rec_goto - follow PTR record */

int     rec_goto(VSTREAM *stream, const char *buf)
{
    off_t   offset;
    static const char *saved_path;
    static off_t saved_offset;
    static int reverse_count;

    /*
     * Crude workaround for queue file loops. VSTREAMs currently have no
     * option to attach application-specific data, so we use global state and
     * simple logic to detect if an application switches streams. We trigger
     * on reverse jumps only. There's one reverse jump for every inserted
     * header, but only one reverse jump for all appended recipients. No-one
     * is likely to insert 10000 message headers, but someone might append
     * 10000 recipients.
     */
#define STREQ(x,y) ((x) == (y) && strcmp((x), (y)) == 0)
#define REVERSE_JUMP_LIMIT	10000

    if (!STREQ(saved_path, VSTREAM_PATH(stream))) {
	saved_path = VSTREAM_PATH(stream);
	reverse_count = 0;
	saved_offset = 0;
    }
    while (ISSPACE(*buf))
	buf++;
    if ((offset = off_cvt_string(buf)) < 0) {
	msg_warn("%s: malformed pointer record value: %s",
		 VSTREAM_PATH(stream), buf);
	return (REC_TYPE_ERROR);
    } else if (offset == 0) {
	/* Dummy record. */
	return (0);
    } else if (offset <= saved_offset && ++reverse_count > REVERSE_JUMP_LIMIT) {
	msg_warn("%s: too many reverse jump records", VSTREAM_PATH(stream));
	return (REC_TYPE_ERROR);
    } else if (vstream_fseek(stream, offset, SEEK_SET) < 0) {
	msg_warn("%s: seek error after pointer record: %m",
		 VSTREAM_PATH(stream));
	return (REC_TYPE_ERROR);
    } else {
	saved_offset = offset;
	return (0);
    }
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

/* rec_pad - write padding record */

int     rec_pad(VSTREAM *stream, int type, int len)
{
    int     width = len - 2;		/* type + length */

    return (rec_fprintf(stream, type, "%*s",
			width < 1 ? 1 : width, "0"));
}
