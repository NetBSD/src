/*++
/* NAME
/*	vbuf 3
/* SUMMARY
/*	generic buffer package
/* SYNOPSIS
/*	#include <vbuf.h>
/*
/*	int	VBUF_GET(bp)
/*	VBUF	*bp;
/*
/*	int	VBUF_PUT(bp, ch)
/*	VBUF	*bp;
/*	int	ch;
/*
/*	int	VBUF_SPACE(bp, len)
/*	VBUF	*bp;
/*	ssize_t	len;
/*
/*	int	vbuf_unget(bp, ch)
/*	VBUF	*bp;
/*	int	ch;
/*
/*	ssize_t	vbuf_read(bp, buf, len)
/*	VBUF	*bp;
/*	char	*buf;
/*	ssize_t	len;
/*
/*	ssize_t	vbuf_write(bp, buf, len)
/*	VBUF	*bp;
/*	const char *buf;
/*	ssize_t	len;
/*
/*	int	vbuf_err(bp)
/*	VBUF	*bp;
/*
/*	int	vbuf_eof(bp)
/*	VBUF	*bp;
/*
/*	int	vbuf_timeout(bp)
/*	VBUF	*bp;
/*
/*	int	vbuf_clearerr(bp)
/*	VBUF	*bp;
/* DESCRIPTION
/*	This module implements a buffer with read/write primitives that
/*	automatically handle buffer-empty or buffer-full conditions.
/*	The application is expected to provide callback routines that run
/*	when the read-write primitives detect a buffer-empty/full condition.
/*
/*	VBUF buffers provide primitives to store and retrieve characters,
/*	and to look up buffer status information.
/*	By design, VBUF buffers provide no explicit primitives for buffer
/*	memory management. This is left to the application to avoid any bias
/*	toward specific management models. The application is free to use
/*	whatever strategy suits best: memory-resident buffer, memory mapped
/*	file, or stdio-like window to an open file.
/*
/*	VBUF_GET() returns the next character from the specified buffer,
/*	or VBUF_EOF when none is available. VBUF_GET() is an unsafe macro
/*	that evaluates its argument more than once.
/*
/*	VBUF_PUT() stores one character into the specified buffer. The result
/*	is the stored character, or VBUF_EOF in case of problems. VBUF_PUT()
/*	is an unsafe macro that evaluates its arguments more than once.
/*
/*	VBUF_SPACE() requests that the requested amount of buffer space be
/*	made available, so that it can be accessed without using VBUF_PUT().
/*	The result value is 0 for success, VBUF_EOF for problems.
/*	VBUF_SPACE() is an unsafe macro that evaluates its arguments more
/*	than once. VBUF_SPACE() does not support read-only streams.
/*
/*	vbuf_unget() provides at least one character of pushback, and returns
/*	the pushed back character, or VBUF_EOF in case of problems. It is
/*	an error to call vbuf_unget() on a buffer before reading any data
/*	from it. vbuf_unget() clears the buffer's end-of-file indicator upon
/*	success, and sets the buffer's error indicator when an attempt is
/*	made to push back a non-character value.
/*
/*	vbuf_read() and vbuf_write() do bulk I/O. The result value is the
/*	number of bytes transferred. A short count is returned in case of
/*	an error.
/*
/*	vbuf_timeout() is a macro that returns non-zero if a timeout error
/*	condition was detected while reading or writing the buffer. The
/*	error status can be reset by calling vbuf_clearerr().
/*
/*	vbuf_err() is a macro that returns non-zero if a non-EOF error
/*	(including timeout) condition was detected while reading or writing
/*	the buffer. The error status can be reset by calling vbuf_clearerr().
/*
/*	vbuf_eof() is a macro that returns non-zero if an end-of-file
/*	condition was detected while reading or writing the buffer. The error
/*	status can be reset by calling vbuf_clearerr().
/* APPLICATION CALLBACK SYNOPSIS
/*	int	get_ready(bp)
/*	VBUF	*bp;
/*
/*	int	put_ready(bp)
/*	VBUF	*bp;
/*
/*	int	space(bp, len)
/*	VBUF	*bp;
/*	ssize_t	len;
/* APPLICATION CALLBACK DESCRIPTION
/* .ad
/* .fi
/*	get_ready() is called when VBUF_GET() detects a buffer-empty condition.
/*	The result is zero when more data could be read, VBUF_EOF otherwise.
/*
/*	put_ready() is called when VBUF_PUT() detects a buffer-full condition.
/*	The result is zero when the buffer could be flushed, VBUF_EOF otherwise.
/*
/*	space() performs whatever magic necessary to make at least \fIlen\fR
/*	bytes available for access without using VBUF_PUT(). The result is 0
/*	in case of success, VBUF_EOF otherwise.
/* SEE ALSO
/*	vbuf(3h) layout of the VBUF data structure.
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
#include <string.h>

/* Utility library. */

#include "vbuf.h"

/* vbuf_unget - implement at least one character pushback */

int     vbuf_unget(VBUF *bp, int ch)
{
    if ((ch & 0xff) != ch || -bp->cnt >= bp->len) {
	bp->flags |= VBUF_FLAG_ERR;
	return (VBUF_EOF);
    } else {
	bp->cnt--;
	bp->flags &= ~VBUF_FLAG_EOF;
	return (*--bp->ptr = ch);
    }
}

/* vbuf_get - handle read buffer empty condition */

int     vbuf_get(VBUF *bp)
{
    return (bp->get_ready(bp) ? VBUF_EOF : VBUF_GET(bp));
}

/* vbuf_put - handle write buffer full condition */

int     vbuf_put(VBUF *bp, int ch)
{
    return (bp->put_ready(bp) ? VBUF_EOF : VBUF_PUT(bp, ch));
}

/* vbuf_read - bulk read from buffer */

ssize_t vbuf_read(VBUF *bp, char *buf, ssize_t len)
{
    ssize_t count;
    char   *cp;
    ssize_t n;

#if 0
    for (count = 0; count < len; count++)
	if ((buf[count] = VBUF_GET(bp)) < 0)
	    break;
    return (count);
#else
    for (cp = buf, count = len; count > 0; cp += n, count -= n) {
	if (bp->cnt >= 0 && bp->get_ready(bp))
	    break;
	n = (count < -bp->cnt ? count : -bp->cnt);
	memcpy(cp, bp->ptr, n);
	bp->ptr += n;
	bp->cnt += n;
    }
    return (len - count);
#endif
}

/* vbuf_write - bulk write to buffer */

ssize_t vbuf_write(VBUF *bp, const char *buf, ssize_t len)
{
    ssize_t count;
    const char *cp;
    ssize_t n;

#if 0
    for (count = 0; count < len; count++)
	if (VBUF_PUT(bp, buf[count]) < 0)
	    break;
    return (count);
#else
    for (cp = buf, count = len; count > 0; cp += n, count -= n) {
	if (bp->cnt <= 0 && bp->put_ready(bp) != 0)
	    break;
	n = (count < bp->cnt ? count : bp->cnt);
	memcpy(bp->ptr, cp, n);
	bp->ptr += n;
	bp->cnt -= n;
    }
    return (len - count);
#endif
}
