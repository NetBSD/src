/*	$NetBSD: smtp_stream.c,v 1.4.2.1 2023/12/25 12:43:32 martin Exp $	*/

/*++
/* NAME
/*	smtp_stream 3
/* SUMMARY
/*	smtp stream I/O support
/* SYNOPSIS
/*	#include <smtp_stream.h>
/*
/*	void	smtp_stream_setup(stream, timeout, enable_deadline,
/*					min_data_rate)
/*	VSTREAM *stream;
/*	int	timeout;
/*	int	enable_deadline;
/*	int	min_data_rate;
/*
/*	void	smtp_printf(stream, format, ...)
/*	VSTREAM *stream;
/*	const char *format;
/*
/*	void	smtp_flush(stream)
/*	VSTREAM *stream;
/*
/*	int	smtp_fgetc(stream)
/*	VSTREAM *stream;
/*
/*	int	smtp_get(vp, stream, maxlen, flags)
/*	VSTRING	*vp;
/*	VSTREAM *stream;
/*	ssize_t	maxlen;
/*	int	flags;
/*
/*	void	smtp_fputs(str, len, stream)
/*	const char *str;
/*	ssize_t	len;
/*	VSTREAM *stream;
/*
/*	void	smtp_fwrite(str, len, stream)
/*	const char *str;
/*	ssize_t	len;
/*	VSTREAM *stream;
/*
/*	void	smtp_fread_buf(vp, len, stream)
/*	VSTRING	*vp;
/*	ssize_t	len;
/*	VSTREAM *stream;
/*
/*	void	smtp_fputc(ch, stream)
/*	int	ch;
/*	VSTREAM *stream;
/*
/*	void	smtp_vprintf(stream, format, ap)
/*	VSTREAM *stream;
/*	char	*format;
/*	va_list	ap;
/*
/*	int	smtp_forbid_bare_lf;
/* AUXILIARY API
/*	int	smtp_get_noexcept(vp, stream, maxlen, flags)
/*	VSTRING	*vp;
/*	VSTREAM *stream;
/*	ssize_t	maxlen;
/*	int	flags;
/* LEGACY API
/*	void	smtp_timeout_setup(stream, timeout)
/*	VSTREAM *stream;
/*	int	timeout;
/* DESCRIPTION
/*	This module reads and writes text records delimited by CR LF,
/*	with error detection: timeouts or unexpected end-of-file.
/*	A trailing CR LF is added upon writing and removed upon reading.
/*
/*	smtp_stream_setup() prepares the specified stream for SMTP read
/*	and write operations described below.
/*	This routine alters the behavior of streams as follows:
/* .IP \(bu
/*	When enable_deadline is non-zero, then the timeout argument
/*	specifies a deadline for the total amount time that may be
/*	spent in all subsequent read/write operations.
/*	Otherwise, the stream is configured to enforce
/*	a time limit for each individual read/write system call.
/* .IP \f(bu
/*	Additionally, when min_data_rate is > 0, the deadline is
/*	incremented by 1/min_data_rate seconds for every min_data_rate
/*	bytes transferred. However, the deadline will never exceed
/*	the value specified with the timeout argument.
/* .IP \f(bu
/*	The stream is configured to use double buffering.
/* .IP \f(bu
/*	The stream is configured to enable exception handling.
/* .PP
/*	smtp_printf() formats its arguments and writes the result to
/*	the named stream, followed by a CR LF pair. The stream is NOT flushed.
/*	Long lines of text are not broken.
/*
/*	smtp_flush() flushes the named stream.
/*
/*	smtp_fgetc() reads one character from the named stream.
/*
/*	smtp_get() reads the named stream up to and including
/*	the next LF character and strips the trailing CR LF. The
/*	\fImaxlen\fR argument limits the length of a line of text,
/*	and protects the program against running out of memory.
/*	Specify a zero bound to turn off bounds checking.
/*	The result is the last character read, or VSTREAM_EOF.
/*	The \fIflags\fR argument is zero or more of:
/* .RS
/* .IP SMTP_GET_FLAG_SKIP
/*	Skip over input in excess of \fImaxlen\fR). Either way, a result
/*	value of '\n' means that the input did not exceed \fImaxlen\fR.
/* .IP SMTP_GET_FLAG_APPEND
/*	Append content to the buffer instead of overwriting it.
/* .RE
/*	Specify SMTP_GET_FLAG_NONE for no special processing.
/*
/*	smtp_fputs() writes its string argument to the named stream.
/*	Long strings are not broken. Each string is followed by a
/*	CR LF pair. The stream is not flushed.
/*
/*	smtp_fwrite() writes its string argument to the named stream.
/*	Long strings are not broken. No CR LF is appended. The stream
/*	is not flushed.
/*
/*	smtp_fread_buf() invokes vstream_fread_buf() to read the
/*	specified number of unformatted bytes from the stream. The
/*	result is not null-terminated. NOTE: do not skip calling
/*	smtp_fread_buf() when len == 0. This function has side
/*	effects including resetting the buffer write position, and
/*	skipping the call would invalidate the buffer state.
/*
/*	smtp_fputc() writes one character to the named stream.
/*	The stream is not flushed.
/*
/*	smtp_vprintf() is the machine underneath smtp_printf().
/*
/*	smtp_get_noexcept() implements the subset of smtp_get()
/*	without long jumps for timeout or EOF errors. Instead,
/*	query the stream status with vstream_feof() etc.
/*	This function will make a VSTREAM long jump (error code
/*	SMTP_ERR_LF) when rejecting input with a bare newline byte.
/*
/*	smtp_timeout_setup() is a backwards-compatibility interface
/*	for programs that don't require deadline or data-rate support.
/*
/*	smtp_forbid_bare_lf controls whether smtp_get_noexcept()
/*	will reject input with a bare newline byte.
/* DIAGNOSTICS
/* .fi
/* .ad
/*	In case of error, a vstream_longjmp() call is performed to the
/*	context specified with vstream_setjmp().
/*	After write error, further writes to the socket are disabled.
/*	This eliminates the need for clumsy code to avoid unwanted
/*	I/O while shutting down a TLS engine or closing a VSTREAM.
/*	Error codes passed along with vstream_longjmp() are:
/* .IP SMTP_ERR_EOF
/*	An I/O error happened, or the peer has disconnected unexpectedly.
/* .IP SMTP_ERR_TIME
/*	The time limit specified to smtp_stream_setup() was exceeded.
/* .PP
/*	Additional error codes that may be used by applications:
/* .IP SMTP_ERR_QUIET
/*	Perform silent cleanup; the error was already reported by
/*	the application.
/*	This error is never generated by the smtp_stream(3) module, but
/*	is defined for application-specific use.
/* .IP SMTP_ERR_DATA
/*	Application data error - the program cannot proceed with this
/*	SMTP session.
/* .IP SMTP_ERR_NONE
/*	A non-error code that makes setjmp()/longjmp() convenient
/*	to use.
/* BUGS
/*	The timeout deadline affects all I/O on the named stream, not
/*	just the I/O done on behalf of this module.
/*
/*	The timeout deadline overwrites any previously set up state on
/*	the named stream.
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

#include <sys_defs.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>			/* FD_ZERO() needs bzero() prototype */
#include <errno.h>

/* Utility library. */

#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <msg.h>
#include <iostuff.h>

/* Application-specific. */

#include "smtp_stream.h"

 /*
  * Important: the time limit feature must not introduce any system calls
  * when the input is already in the buffer, or when the output still fits in
  * the buffer. Such system calls would really hurt when receiving or sending
  * body content one line at a time.
  */
int     smtp_forbid_bare_lf;

/* smtp_timeout_reset - reset per-stream error flags */

static void smtp_timeout_reset(VSTREAM *stream)
{

    /*
     * Individual smtp_stream(3) I/O functions must not recharge the deadline
     * timer, because multiline responses involve multiple smtp_stream(3)
     * calls, and we really want to limit the time to send or receive a
     * response.
     */
    vstream_clearerr(stream);
}

/* smtp_longjmp - raise an exception */

static NORETURN smtp_longjmp(VSTREAM *stream, int err, const char *context)
{

    /*
     * If we failed to write, don't bang our head against the wall another
     * time when closing the stream. In the case of SMTP over TLS, poisoning
     * the socket with shutdown() is more robust than purging the VSTREAM
     * buffer or replacing the write function pointer with dummy_write().
     */
    if (msg_verbose)
	msg_info("%s: %s", context, err == SMTP_ERR_TIME ? "timeout" : "EOF");
    if (vstream_wr_error(stream))
	/* Don't report ECONNRESET (hangup), EINVAL (already shut down), etc. */
	(void) shutdown(vstream_fileno(stream), SHUT_WR);
    vstream_longjmp(stream, err);
}

/* smtp_stream_setup - configure timeout trap */

void    smtp_stream_setup(VSTREAM *stream, int maxtime, int enable_deadline,
			          int min_data_rate)
{
    const char *myname = "smtp_stream_setup";

    if (msg_verbose)
	msg_info("%s: maxtime=%d enable_deadline=%d min_data_rate=%d",
		 myname, maxtime, enable_deadline, min_data_rate);

    vstream_control(stream,
		    CA_VSTREAM_CTL_DOUBLE,
		    CA_VSTREAM_CTL_TIMEOUT(maxtime),
		    enable_deadline ? CA_VSTREAM_CTL_START_DEADLINE
		    : CA_VSTREAM_CTL_STOP_DEADLINE,
		    CA_VSTREAM_CTL_MIN_DATA_RATE(min_data_rate),
		    CA_VSTREAM_CTL_EXCEPT,
		    CA_VSTREAM_CTL_END);
}

/* smtp_flush - flush stream */

void    smtp_flush(VSTREAM *stream)
{
    int     err;

    /*
     * Do the I/O, protected against timeout.
     */
    smtp_timeout_reset(stream);
    err = vstream_fflush(stream);

    /*
     * See if there was a problem.
     */
    if (vstream_ftimeout(stream))
	smtp_longjmp(stream, SMTP_ERR_TIME, "smtp_flush");
    if (err != 0)
	smtp_longjmp(stream, SMTP_ERR_EOF, "smtp_flush");
}

/* smtp_vprintf - write one line to SMTP peer */

void    smtp_vprintf(VSTREAM *stream, const char *fmt, va_list ap)
{
    int     err;

    /*
     * Do the I/O, protected against timeout.
     */
    smtp_timeout_reset(stream);
    vstream_vfprintf(stream, fmt, ap);
    vstream_fputs("\r\n", stream);
    err = vstream_ferror(stream);

    /*
     * See if there was a problem.
     */
    if (vstream_ftimeout(stream))
	smtp_longjmp(stream, SMTP_ERR_TIME, "smtp_vprintf");
    if (err != 0)
	smtp_longjmp(stream, SMTP_ERR_EOF, "smtp_vprintf");
}

/* smtp_printf - write one line to SMTP peer */

void    smtp_printf(VSTREAM *stream, const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    smtp_vprintf(stream, fmt, ap);
    va_end(ap);
}

/* smtp_fgetc - read one character from SMTP peer */

int     smtp_fgetc(VSTREAM *stream)
{
    int     ch;

    /*
     * Do the I/O, protected against timeout.
     */
    smtp_timeout_reset(stream);
    ch = VSTREAM_GETC(stream);

    /*
     * See if there was a problem.
     */
    if (vstream_ftimeout(stream))
	smtp_longjmp(stream, SMTP_ERR_TIME, "smtp_fgetc");
    if (vstream_feof(stream) || vstream_ferror(stream))
	smtp_longjmp(stream, SMTP_ERR_EOF, "smtp_fgetc");
    return (ch);
}

/* smtp_get - read one line from SMTP peer */

int     smtp_get(VSTRING *vp, VSTREAM *stream, ssize_t bound, int flags)
{
    int     last_char;

    /*
     * Do the I/O, protected against timeout.
     */
    smtp_timeout_reset(stream);
    last_char = smtp_get_noexcept(vp, stream, bound, flags);

    /*
     * EOF is bad, whether or not it happens in the middle of a record. Don't
     * allow data that was truncated because of EOF.
     */
    if (vstream_ftimeout(stream))
	smtp_longjmp(stream, SMTP_ERR_TIME, "smtp_get");
    if (vstream_feof(stream) || vstream_ferror(stream))
	smtp_longjmp(stream, SMTP_ERR_EOF, "smtp_get");
    return (last_char);
}

/* smtp_get_noexcept - read one line from SMTP peer, without exceptions */

int     smtp_get_noexcept(VSTRING *vp, VSTREAM *stream, ssize_t bound, int flags)
{
    int     last_char;
    int     next_char;

    /*
     * It's painful to do I/O with records that may span multiple buffers.
     * Allow for partial long lines (we will read the remainder later) and
     * allow for lines ending in bare LF. The idea is to be liberal in what
     * we accept, strict in what we send.
     * 
     * XXX 2821: Section 4.1.1.4 says that an SMTP server must not recognize
     * bare LF as record terminator.
     */
    last_char = (bound == 0 ?
		 vstring_get_flags(vp, stream,
				   (flags & SMTP_GET_FLAG_APPEND) ?
				   VSTRING_GET_FLAG_APPEND : 0) :
		 vstring_get_flags_bound(vp, stream,
					 (flags & SMTP_GET_FLAG_APPEND) ?
				       VSTRING_GET_FLAG_APPEND : 0, bound));

    switch (last_char) {

	/*
	 * Do some repair in the rare case that we stopped reading in the
	 * middle of the CRLF record terminator.
	 */
    case '\r':
	if ((next_char = VSTREAM_GETC(stream)) == '\n') {
	    VSTRING_ADDCH(vp, '\n');
	    last_char = '\n';
	    /* FALLTRHOUGH */
	} else {
	    if (next_char != VSTREAM_EOF)
		vstream_ungetc(stream, next_char);
	    break;
	}

	/*
	 * Strip off the record terminator: either CRLF or just bare LF.
	 * 
	 * XXX RFC 2821 disallows sending bare CR everywhere. We remove bare CR
	 * if received before CRLF, and leave it alone otherwise.
	 */
    case '\n':
	vstring_truncate(vp, VSTRING_LEN(vp) - 1);
	if (smtp_forbid_bare_lf
	    && (VSTRING_LEN(vp) == 0 || vstring_end(vp)[-1] != '\r'))
	    vstream_longjmp(stream, SMTP_ERR_LF);
	while (VSTRING_LEN(vp) > 0 && vstring_end(vp)[-1] == '\r')
	    vstring_truncate(vp, VSTRING_LEN(vp) - 1);
	VSTRING_TERMINATE(vp);
	/* FALLTRHOUGH */

	/*
	 * Partial line: just read the remainder later. If we ran into EOF,
	 * the next test will deal with it.
	 */
    default:
	break;
    }

    /*
     * Optionally, skip over excess input, protected by the same time limit.
     */
    if (last_char != '\n' && (flags & SMTP_GET_FLAG_SKIP)
	&& vstream_feof(stream) == 0 && vstream_ferror(stream) == 0)
	while ((next_char = VSTREAM_GETC(stream)) != VSTREAM_EOF
	       && next_char != '\n')
	     /* void */ ;

    return (last_char);
}

/* smtp_fputs - write one line to SMTP peer */

void    smtp_fputs(const char *cp, ssize_t todo, VSTREAM *stream)
{
    int     err;

    if (todo < 0)
	msg_panic("smtp_fputs: negative todo %ld", (long) todo);

    /*
     * Do the I/O, protected against timeout.
     */
    smtp_timeout_reset(stream);
    err = (vstream_fwrite(stream, cp, todo) != todo
	   || vstream_fputs("\r\n", stream) == VSTREAM_EOF);

    /*
     * See if there was a problem.
     */
    if (vstream_ftimeout(stream))
	smtp_longjmp(stream, SMTP_ERR_TIME, "smtp_fputs");
    if (err != 0)
	smtp_longjmp(stream, SMTP_ERR_EOF, "smtp_fputs");
}

/* smtp_fwrite - write one string to SMTP peer */

void    smtp_fwrite(const char *cp, ssize_t todo, VSTREAM *stream)
{
    int     err;

    if (todo < 0)
	msg_panic("smtp_fwrite: negative todo %ld", (long) todo);

    /*
     * Do the I/O, protected against timeout.
     */
    smtp_timeout_reset(stream);
    err = (vstream_fwrite(stream, cp, todo) != todo);

    /*
     * See if there was a problem.
     */
    if (vstream_ftimeout(stream))
	smtp_longjmp(stream, SMTP_ERR_TIME, "smtp_fwrite");
    if (err != 0)
	smtp_longjmp(stream, SMTP_ERR_EOF, "smtp_fwrite");
}

/* smtp_fread_buf - read one buffer from SMTP peer */

void    smtp_fread_buf(VSTRING *vp, ssize_t todo, VSTREAM *stream)
{
    int     err;

    /*
     * Do not return early if todo == 0. We still need the side effects from
     * calling vstream_fread_buf() including resetting the buffer write
     * position. Skipping the call would invalidate the buffer state.
     */
    if (todo < 0)
	msg_panic("smtp_fread_buf: negative todo %ld", (long) todo);

    /*
     * Do the I/O, protected against timeout.
     */
    smtp_timeout_reset(stream);
    err = (vstream_fread_buf(stream, vp, todo) != todo);

    /*
     * See if there was a problem.
     */
    if (vstream_ftimeout(stream))
	smtp_longjmp(stream, SMTP_ERR_TIME, "smtp_fread");
    if (err != 0)
	smtp_longjmp(stream, SMTP_ERR_EOF, "smtp_fread");
}

/* smtp_fputc - write to SMTP peer */

void    smtp_fputc(int ch, VSTREAM *stream)
{
    int     stat;

    /*
     * Do the I/O, protected against timeout.
     */
    smtp_timeout_reset(stream);
    stat = VSTREAM_PUTC(ch, stream);

    /*
     * See if there was a problem.
     */
    if (vstream_ftimeout(stream))
	smtp_longjmp(stream, SMTP_ERR_TIME, "smtp_fputc");
    if (stat == VSTREAM_EOF)
	smtp_longjmp(stream, SMTP_ERR_EOF, "smtp_fputc");
}
