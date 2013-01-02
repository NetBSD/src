/*	$NetBSD: memcache_proto.c,v 1.1.1.1 2013/01/02 18:58:59 tron Exp $	*/

/*++
/* NAME
/*	memcache_proto 3
/* SUMMARY
/*	memcache low-level protocol
/* SYNOPSIS
/*	#include <memcache_proto.h>
/*
/*	int	memcache_get(fp, buf, len)
/*	VSTREAM	*fp;
/*	VSTRING	*buf;
/*	ssize_t	len;
/*
/*	int	memcache_printf(fp, format, ...)
/*	VSTREAM	*fp;
/*	const char *format;
/*
/*	int	memcache_vprintf(fp, format, ap)
/*	VSTREAM	*fp;
/*	const char *format;
/*	va_list	ap;
/*
/*	int	memcache_fread(fp, buf, len)
/*	VSTREAM	*fp;
/*	VSTRING	*buf;
/*	ssize_t	len;
/*
/*	int	memcache_fwrite(fp, buf, len)
/*	VSTREAM	*fp;
/*	const char *buf;
/*	ssize_t	len;
/* DESCRIPTION
/*	This module implements the low-level memcache protocol.
/*	All functions return -1 on error and 0 on succcess.
/* SEE ALSO
/*	smtp_proto(3) SMTP low-level protocol.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#include <sys_defs.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <vstring_vstream.h>

/* Application-specific. */

#include <memcache_proto.h>

#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)

/* memcache_get - read one line from peer */

int     memcache_get(VSTREAM *stream, VSTRING *vp, ssize_t bound)
{
    int     last_char;
    int     next_char;

    last_char = (bound == 0 ? vstring_get(vp, stream) :
		 vstring_get_bound(vp, stream, bound));

    switch (last_char) {

	/*
	 * Do some repair in the rare case that we stopped reading in the
	 * middle of the CRLF record terminator.
	 */
    case '\r':
	if ((next_char = VSTREAM_GETC(stream)) == '\n') {
	    VSTRING_ADDCH(vp, '\n');
	    /* FALLTRHOUGH */
	} else {
	    if (next_char != VSTREAM_EOF)
		vstream_ungetc(stream, next_char);

	    /*
	     * Input too long, or EOF
	     */
    default:
	    if (msg_verbose)
		msg_info("%s got %s", VSTREAM_PATH(stream),
			 LEN(vp) < bound ? "EOF" : "input too long");
	    return (-1);
	}

	/*
	 * Strip off the record terminator: either CRLF or just bare LF.
	 */
    case '\n':
	vstring_truncate(vp, VSTRING_LEN(vp) - 1);
	if (VSTRING_LEN(vp) > 0 && vstring_end(vp)[-1] == '\r')
	    vstring_truncate(vp, VSTRING_LEN(vp) - 1);
	VSTRING_TERMINATE(vp);
	if (msg_verbose)
	    msg_info("%s got: %s", VSTREAM_PATH(stream), STR(vp));
	return (0);
    }
}

/* memcache_fwrite - write one blob to peer */

int     memcache_fwrite(VSTREAM *stream, const char *cp, ssize_t todo)
{

    /*
     * Sanity check.
     */
    if (todo < 0)
	msg_panic("memcache_fwrite: negative todo %ld", (long) todo);

    /*
     * Do the I/O.
     */
    if (msg_verbose)
	msg_info("%s write: %.*s", VSTREAM_PATH(stream), (int) todo, cp);
    if (vstream_fwrite(stream, cp, todo) != todo
	|| vstream_fputs("\r\n", stream) == VSTREAM_EOF)
	return (-1);
    else
	return (0);
}

/* memcache_fread - read one blob from peer */

int     memcache_fread(VSTREAM *stream, VSTRING *buf, ssize_t todo)
{

    /*
     * Sanity check.
     */
    if (todo < 0)
	msg_panic("memcache_fread: negative todo %ld", (long) todo);

    /*
     * Do the I/O.
     */
    VSTRING_SPACE(buf, todo);
    VSTRING_AT_OFFSET(buf, todo);
    if (vstream_fread(stream, STR(buf), todo) != todo
	|| VSTREAM_GETC(stream) != '\r'
	|| VSTREAM_GETC(stream) != '\n') {
	if (msg_verbose)
	    msg_info("%s read: error", VSTREAM_PATH(stream));
	return (-1);
    } else {
	vstring_truncate(buf, todo);
	VSTRING_TERMINATE(buf);
	if (msg_verbose)
	    msg_info("%s read: %s", VSTREAM_PATH(stream), STR(buf));
	return (0);
    }
}

/* memcache_vprintf - write one line to peer */

int     memcache_vprintf(VSTREAM *stream, const char *fmt, va_list ap)
{

    /*
     * Do the I/O.
     */
    vstream_vfprintf(stream, fmt, ap);
    vstream_fputs("\r\n", stream);
    if (vstream_ferror(stream))
	return (-1);
    else
	return (0);
}

/* memcache_printf - write one line to peer */

int     memcache_printf(VSTREAM *stream, const char *fmt,...)
{
    va_list ap;
    int     ret;

    if (msg_verbose) {
	VSTRING *buf = vstring_alloc(100);

	va_start(ap, fmt);
	vstring_vsprintf(buf, fmt, ap);
	va_end(ap);
	msg_info("%s write: %s", VSTREAM_PATH(stream), STR(buf));
	vstring_free(buf);
    }

    /*
     * Do the I/O.
     */
    va_start(ap, fmt);
    ret = memcache_vprintf(stream, fmt, ap);
    va_end(ap);
    return (ret);
}
