/*	$NetBSD: netstring.c,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

/*++
/* NAME
/*	netstring 3
/* SUMMARY
/*	netstring stream I/O support
/* SYNOPSIS
/*	#include <netstring.h>
/*
/*	void	netstring_setup(stream, timeout)
/*	VSTREAM *stream;
/*	int	timeout;
/*
/*	void	netstring_except(stream, exception)
/*	VSTREAM	*stream;
/*	int	exception;
/*
/*	VSTRING	*netstring_get(stream, buf, limit)
/*	VSTREAM	*stream;
/*	VSTRING	*buf;
/*	ssize_t	limit;
/*
/*	void	netstring_put(stream, data, len)
/*	VSTREAM *stream;
/*	const char *data;
/*	ssize_t	len;
/*
/*	void	netstring_put_multi(stream, data, len, data, len, ..., 0)
/*	VSTREAM *stream;
/*	const char *data;
/*	ssize_t	len;
/*
/*	void	NETSTRING_PUT_BUF(stream, buf)
/*	VSTREAM *stream;
/*	VSTRING	*buf;
/*
/*	void	netstring_fflush(stream)
/*	VSTREAM *stream;
/*
/*	VSTRING	*netstring_memcpy(buf, data, len)
/*	VSTRING	*buf;
/*	const char *data;
/*	ssize_t	len;
/*
/*	VSTRING	*netstring_memcat(buf, data, len)
/*	VSTRING	*buf;
/*	const char *src;
/*	ssize_t len;
/* AUXILIARY ROUTINES
/*	ssize_t	netstring_get_length(stream)
/*	VSTREAM *stream;
/*
/*	VSTRING	*netstring_get_data(stream, buf, len)
/*	VSTREAM *stream;
/*	VSTRING	*buf;
/*	ssize_t	len;
/*
/*	void	netstring_get_terminator(stream)
/*	VSTREAM *stream;
/* DESCRIPTION
/*	This module reads and writes netstrings with error detection:
/*	timeouts, unexpected end-of-file, or format errors. Netstring
/*	is a data format designed by Daniel Bernstein.
/*
/*	netstring_setup() arranges for a time limit on the netstring
/*	read and write operations described below.
/*	This routine alters the behavior of streams as follows:
/* .IP \(bu
/*	The read/write timeout is set to the specified value.
/* .IP \(bu
/*	The stream is configured to enable exception handling.
/* .PP
/*	netstring_except() raises the specified exception on the
/*	named stream. See the DIAGNOSTICS section below.
/*
/*	netstring_get() reads a netstring from the specified stream
/*	and extracts its content. The limit specifies a maximal size.
/*	Specify zero to disable the size limit. The result is not null
/*	terminated.  The result value is the buf argument.
/*
/*	netstring_put() encapsulates the specified string as a netstring
/*	and sends the result to the specified stream.
/*	The stream output buffer is not flushed.
/*
/*	netstring_put_multi() encapsulates the content of multiple strings
/*	as one netstring and sends the result to the specified stream. The
/*	argument list must be terminated with a null data pointer.
/*	The stream output buffer is not flushed.
/*
/*	NETSTRING_PUT_BUF() is a macro that provides a VSTRING-based
/*	wrapper for the netstring_put() routine.
/*
/*	netstring_fflush() flushes the output buffer of the specified
/*	stream and handles any errors.
/*
/*	netstring_memcpy() encapsulates the specified data as a netstring
/*	and copies the result over the specified buffer. The result
/*	value is the buffer.
/*
/*	netstring_memcat() encapsulates the specified data as a netstring
/*	and appends the result to the specified buffer. The result
/*	value is the buffer.
/*
/*	The following routines provide low-level access to a netstring
/*	stream.
/*
/*	netstring_get_length() reads a length field from the specified
/*	stream, and absorbs the netstring length field terminator.
/*
/*	netstring_get_data() reads the specified number of bytes from the
/*	specified stream into the specified buffer, and absorbs the
/*	netstring terminator.  The result value is the buf argument.
/*
/*	netstring_get_terminator() reads the netstring terminator from
/*	the specified stream.
/* DIAGNOSTICS
/* .fi
/* .ad
/*	In case of error, a vstream_longjmp() call is performed to the
/*	context specified with vstream_setjmp().
/*	Error codes passed along with vstream_longjmp() are:
/* .IP NETSTRING_ERR_EOF
/*	An I/O error happened, or the peer has disconnected unexpectedly.
/* .IP NETSTRING_ERR_TIME
/*	The time limit specified to netstring_setup() was exceeded.
/* .IP NETSTRING_ERR_FORMAT
/*	The input contains an unexpected character value.
/* .IP NETSTRING_ERR_SIZE
/*	The input is larger than acceptable.
/* BUGS
/*	The timeout deadline affects all I/O on the named stream, not
/*	just the I/O done on behalf of this module.
/*
/*	The timeout deadline overwrites any previously set up state on
/*	the named stream.
/*
/*	netstrings are not null terminated, which makes printing them
/*	a bit awkward.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* SEE ALSO
/*	http://cr.yp.to/proto/netstrings.txt, netstring definition
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdarg.h>
#include <ctype.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <netstring.h>

/* Application-specific. */

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

/* netstring_setup - initialize netstring stream */

void    netstring_setup(VSTREAM *stream, int timeout)
{
    vstream_control(stream,
		    VSTREAM_CTL_TIMEOUT, timeout,
		    VSTREAM_CTL_EXCEPT,
		    VSTREAM_CTL_END);
}

/* netstring_except - process netstring stream exception */

void    netstring_except(VSTREAM *stream, int exception)
{
    vstream_longjmp(stream, exception);
}

/* netstring_get_length - read netstring length + terminator */

ssize_t netstring_get_length(VSTREAM *stream)
{
    const char *myname = "netstring_get_length";
    ssize_t len = 0;
    int     ch;

    for (;;) {
	switch (ch = VSTREAM_GETC(stream)) {
	case VSTREAM_EOF:
	    netstring_except(stream, vstream_ftimeout(stream) ?
			     NETSTRING_ERR_TIME : NETSTRING_ERR_EOF);
	case ':':
	    if (msg_verbose > 1)
		msg_info("%s: read netstring length %ld", myname, (long) len);
	    return (len);
	default:
	    if (!ISDIGIT(ch))
		netstring_except(stream, NETSTRING_ERR_FORMAT);
	    len = len * 10 + ch - '0';
	    /* vstream_fread() would read zero bytes. Reject input anyway. */
	    if (len < 0)
		netstring_except(stream, NETSTRING_ERR_SIZE);
	    break;
	}
    }
}

/* netstring_get_data - read netstring payload + terminator */

VSTRING *netstring_get_data(VSTREAM *stream, VSTRING *buf, ssize_t len)
{
    const char *myname = "netstring_get_data";

    /*
     * Allocate buffer space.
     */
    VSTRING_RESET(buf);
    VSTRING_SPACE(buf, len);

    /*
     * Read the payload and absorb the terminator.
     */
    if (vstream_fread(stream, STR(buf), len) != len)
	netstring_except(stream, vstream_ftimeout(stream) ?
			 NETSTRING_ERR_TIME : NETSTRING_ERR_EOF);
    if (msg_verbose > 1)
	msg_info("%s: read netstring data %.*s",
		 myname, (int) (len < 30 ? len : 30), STR(buf));
    netstring_get_terminator(stream);

    /*
     * Position the buffer.
     */
    VSTRING_AT_OFFSET(buf, len);
    return (buf);
}

/* netstring_get_terminator - absorb netstring terminator */

void    netstring_get_terminator(VSTREAM *stream)
{
    if (VSTREAM_GETC(stream) != ',')
	netstring_except(stream, NETSTRING_ERR_FORMAT);
}

/* netstring_get - read string from netstring stream */

VSTRING *netstring_get(VSTREAM *stream, VSTRING *buf, ssize_t limit)
{
    ssize_t len;

    len = netstring_get_length(stream);
    if (limit && len > limit)
	netstring_except(stream, NETSTRING_ERR_SIZE);
    netstring_get_data(stream, buf, len);
    return (buf);
}

/* netstring_put - send string as netstring */

void    netstring_put(VSTREAM *stream, const char *data, ssize_t len)
{
    const char *myname = "netstring_put";

    if (msg_verbose > 1)
	msg_info("%s: write netstring len %ld data %.*s",
		 myname, (long) len, (int) (len < 30 ? len : 30), data);
    vstream_fprintf(stream, "%ld:", (long) len);
    vstream_fwrite(stream, data, len);
    VSTREAM_PUTC(',', stream);
}

/* netstring_put_multi - send multiple strings as one netstring */

void    netstring_put_multi(VSTREAM *stream,...)
{
    const char *myname = "netstring_put_multi";
    ssize_t total;
    char   *data;
    ssize_t data_len;
    va_list ap;

    /*
     * Figure out the total result size.
     */
    va_start(ap, stream);
    for (total = 0; (data = va_arg(ap, char *)) != 0; total += data_len)
	if ((data_len = va_arg(ap, ssize_t)) < 0)
	    msg_panic("netstring_put_multi: bad data length %ld", (long) data_len);
    va_end(ap);

    /*
     * Debugging support.
     */
    if (msg_verbose > 1) {
	va_start(ap, stream);
	data = va_arg(ap, char *);
	data_len = va_arg(ap, ssize_t);
	msg_info("%s: write netstring len %ld data %.*s",
	 myname, (long) total, (int) (data_len < 30 ? data_len : 30), data);
	va_end(ap);
    }

    /*
     * Send the length, content and terminator.
     */
    vstream_fprintf(stream, "%ld:", (long) total);
    va_start(ap, stream);
    while ((data = va_arg(ap, char *)) != 0) {
	data_len = va_arg(ap, ssize_t);
	if (data_len > 0)
	    if (vstream_fwrite(stream, data, data_len) != data_len)
		netstring_except(stream, vstream_ftimeout(stream) ?
				 NETSTRING_ERR_TIME : NETSTRING_ERR_EOF);
    }
    va_end(ap);
    vstream_fwrite(stream, ",", 1);
}

/* netstring_fflush - flush netstring stream */

void    netstring_fflush(VSTREAM *stream)
{
    if (vstream_fflush(stream) == VSTREAM_EOF)
	netstring_except(stream, vstream_ftimeout(stream) ?
			 NETSTRING_ERR_TIME : NETSTRING_ERR_EOF);
}

/* netstring_memcpy - copy data as in-memory netstring */

VSTRING *netstring_memcpy(VSTRING *buf, const char *src, ssize_t len)
{
    vstring_sprintf(buf, "%ld:", (long) len);
    vstring_memcat(buf, src, len);
    VSTRING_ADDCH(buf, ',');
    return (buf);
}

/* netstring_memcat - append data as in-memory netstring */

VSTRING *netstring_memcat(VSTRING *buf, const char *src, ssize_t len)
{
    vstring_sprintf_append(buf, "%ld:", (long) len);
    vstring_memcat(buf, src, len);
    VSTRING_ADDCH(buf, ',');
    return (buf);
}
