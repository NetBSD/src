/*++
/* NAME
/*	smtp_stream 3
/* SUMMARY
/*	smtp stream I/O support
/* SYNOPSIS
/*	#include <smtp_stream.h>
/*
/*	jmp_buf	smtp_timeout_buf;
/*
/*	void	smtp_timeout_setup(stream, timeout)
/*	VSTREAM *stream;
/*	int	timeout;
/*
/*	void	smtp_printf(stream, format, ...)
/*	VSTREAM *stream;
/*	const char *format;
/*
/*	int	smtp_get(vp, stream, maxlen)
/*	VSTRING	*vp;
/*	VSTREAM *stream;
/*	int	maxlen;
/*
/*	void	smtp_fputs(str, len, stream)
/*	const char *str;
/*	int	len;
/*	VSTREAM *stream;
/*
/*	void	smtp_fwrite(str, len, stream)
/*	const char *str;
/*	int	len;
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
/* DESCRIPTION
/*	This module reads and writes text records delimited by CR LF,
/*	with error detection: timeouts or unexpected end-of-file.
/*	A trailing CR LF is added upon writing and removed upon reading.
/*
/*	smtp_timeout_setup() arranges for a time limit on the smtp read
/*	and write operations described below.
/*	This routine alters the behavior of streams as follows:
/* .IP \(bu
/*	The read routine is replaced by one than calls timed_read().
/* .IP \(bu
/*	The write routine is replaced by one that calls timed_write().
/* .IP \f(bu
/*	The stream is configured to use double buffering.
/* .IP \f(bu
/*	A timeout error is reported to the vstream module as an I/O error.
/* .PP
/*	smtp_printf() formats its arguments and writes the result to
/*	the named stream, followed by a CR LF pair. The stream is flushed.
/*	Long lines of text are not broken.
/*
/*	smtp_get() reads the named stream up to and including
/*	the next LF character and strips the trailing CR LF. The
/*	\fImaxlen\fR argument limits the length of a line of text,
/*	and protects the program against running out of memory.
/*	Specify a zero bound to turn off bounds checking.
/*	The result is the last character read, or VSTREAM_EOF.
/*
/*	smtp_fputs() writes its string argument to the named stream.
/*	Long strings are not broken. Each string is followed by a
/*	CR LF pair. The stream is not flushed.
/*
/*	smtp_fwrite() writes its string argument to the named stream.
/*	Long strings are not broken. No CR LF is appended. The stream
/*	is not flushed.
/*
/*	smtp_fputc() writes one character to the named stream.
/*	The stream is not flushed.
/*
/*	smtp_vprintf() is the machine underneath smtp_printf().
/* DIAGNOSTICS
/* .fi
/* .ad
/*	In case of error, a longjmp() is performed to the context
/*	saved in the global \fIsmtp_timeout_buf\fR.
/*	Error codes passed along with longjmp() are:
/* .IP SMTP_ERR_EOF
/*	The peer has disconnected unexpectedly.
/* .IP SMTP_ERR_TIME
/*	The time limit specified to smtp_timeout_setup() was exceeded.
/* BUGS
/*	The timeout etc. context is static, so this module can handle
/*	only one SMTP session at a time.
/*
/*	The timeout protection, including longjmp(), affects all I/O
/*	on the named stream, not just the I/O done by this module.
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

jmp_buf smtp_timeout_buf;

 /*
  * Timeout handling. When a timeout happens, we shut down the connection in
  * the appropriate direction, to force the I/O operation to fail.
  */
static int smtp_timeout_done;
static int smtp_maxtime;

#define SMTP_DIR_READ	0		/* read direction */
#define SMTP_DIR_WRITE	1		/* write direction */

/* smtp_timeout_event - timeout handler */

static void smtp_timeout_event(int fd, int direction)
{

    /*
     * Yes, this is gross, but we cannot longjump() away. Although timeouts
     * are restricted to read() and write() operations, we could still leave
     * things in an inconsistent state. Instead, we set a flag and force an
     * I/O error on the smtp stream.
     */
    if (shutdown(fd, direction) < 0)
	if (errno != ENOTCONN)
	    msg_warn("smtp_timeout_event: shutdown: %m");
    smtp_timeout_done = 1;
}

/* smtp_read - read with timeout */

static int smtp_read(int fd, void *buf, unsigned len)
{
    if (read_wait(fd, smtp_maxtime) < 0) {
	smtp_timeout_event(fd, SMTP_DIR_READ);
	return (-1);
    } else {
	return (read(fd, buf, len));
    }
}

/* smtp_write - write with timeout */

static int smtp_write(int fd, void *buf, unsigned len)
{
    if (write_wait(fd, smtp_maxtime) < 0) {
	smtp_timeout_event(fd, SMTP_DIR_WRITE);
	return (-1);
    } else {
	return (write(fd, buf, len));
    }
}

/* smtp_timeout_protect - setup timeout trap for specified stream. */

static void smtp_timeout_protect(void)
{
    smtp_timeout_done = 0;
}

/* smtp_timeout_unprotect - finish timeout trap */

static void smtp_timeout_unprotect(void)
{
    if (smtp_timeout_done)
	longjmp(smtp_timeout_buf, SMTP_ERR_TIME);
}

/* smtp_timeout_setup - configure timeout trap */

void    smtp_timeout_setup(VSTREAM *stream, int maxtime)
{

    /*
     * XXX The timeout etc. state is static, so a process can have at most
     * one SMTP session at a time. We could use the VSTREAM file descriptor
     * number as key into a BINHASH table with per-stream contexts. This
     * would allow us to talk to multiple SMTP streams at the same time.
     * Another possibility is to use the file descriptor as an index into a
     * linear table of structure pointers. In either case we would need to
     * provide an smtp_timeout_cleanup() routine to dispose of memory that is
     * no longer needed.
     */
    vstream_control(stream,
		    VSTREAM_CTL_READ_FN, smtp_read,
		    VSTREAM_CTL_WRITE_FN, smtp_write,
		    VSTREAM_CTL_DOUBLE,
		    VSTREAM_CTL_END);
    smtp_maxtime = maxtime;
}

/* smtp_vprintf - write one line to SMTP peer */

void    smtp_vprintf(VSTREAM *stream, const char *fmt, va_list ap)
{
    int     err;

    /*
     * Do the I/O, protected against timeout.
     */
    smtp_timeout_protect();
    vstream_vfprintf(stream, fmt, ap);
    vstream_fputs("\r\n", stream);
    err = vstream_fflush(stream);
    smtp_timeout_unprotect();

    /*
     * See if there was a problem.
     */
    if (err != 0) {
	if (msg_verbose)
	    msg_info("smtp_vprintf: EOF");
	longjmp(smtp_timeout_buf, SMTP_ERR_EOF);
    }
}

/* smtp_printf - write one line to SMTP peer */

void    smtp_printf(VSTREAM *stream, const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    smtp_vprintf(stream, fmt, ap);
    va_end(ap);
}

/* smtp_get - read one line from SMTP peer */

int     smtp_get(VSTRING *vp, VSTREAM *stream, int bound)
{
    int     last_char;
    int     next_char;

    /*
     * It's painful to do I/O with records that may span multiple buffers.
     * Allow for partial long lines (we will read the remainder later) and
     * allow for lines ending in bare LF. The idea is to be liberal in what
     * we accept, strict in what we send.
     */
    smtp_timeout_protect();
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
	    last_char = '\n';
	    /* FALLTRHOUGH */
	} else {
	    if (next_char != VSTREAM_EOF)
		vstream_ungetc(stream, next_char);
	    break;
	}

	/*
	 * Strip off the record terminator: either CRLF or just bare LF.
	 */
    case '\n':
	if (VSTRING_LEN(vp) > 1 && vstring_end(vp)[-2] == '\r')
	    vstring_truncate(vp, VSTRING_LEN(vp) - 2);
	else
	    vstring_truncate(vp, VSTRING_LEN(vp) - 1);
	VSTRING_TERMINATE(vp);

	/*
	 * Partial line: just read the remainder later. If we ran into EOF,
	 * the next test will deal with it.
	 */
    default:
	break;
    }
    smtp_timeout_unprotect();

    /*
     * EOF is bad, whether or not it happens in the middle of a record. Don't
     * allow data that was truncated because of EOF.
     */
    if (vstream_feof(stream) || vstream_ferror(stream)) {
	if (msg_verbose)
	    msg_info("smtp_get: EOF");
	longjmp(smtp_timeout_buf, SMTP_ERR_EOF);
    }
    return (last_char);
}

/* smtp_fputs - write one line to SMTP peer */

void    smtp_fputs(const char *cp, int todo, VSTREAM *stream)
{
    unsigned err;

    if (todo < 0)
	msg_panic("smtp_fputs: negative todo %d", todo);

    /*
     * Do the I/O, protected against timeout.
     */
    smtp_timeout_protect();
    err = (vstream_fwrite(stream, cp, todo) != todo
	   || vstream_fputs("\r\n", stream) == VSTREAM_EOF);
    smtp_timeout_unprotect();

    /*
     * See if there was a problem.
     */
    if (err != 0) {
	if (msg_verbose)
	    msg_info("smtp_fputs: EOF");
	longjmp(smtp_timeout_buf, SMTP_ERR_EOF);
    }
}

/* smtp_fwrite - write one string to SMTP peer */

void    smtp_fwrite(const char *cp, int todo, VSTREAM *stream)
{
    unsigned err;

    if (todo < 0)
	msg_panic("smtp_fwrite: negative todo %d", todo);

    /*
     * Do the I/O, protected against timeout.
     */
    smtp_timeout_protect();
    err = (vstream_fwrite(stream, cp, todo) != todo);
    smtp_timeout_unprotect();

    /*
     * See if there was a problem.
     */
    if (err != 0) {
	if (msg_verbose)
	    msg_info("smtp_fwrite: EOF");
	longjmp(smtp_timeout_buf, SMTP_ERR_EOF);
    }
}

/* smtp_fputc - write to SMTP peer */

void    smtp_fputc(int ch, VSTREAM *stream)
{
    int     stat;

    /*
     * Do the I/O, protected against timeout.
     */
    smtp_timeout_protect();
    stat = VSTREAM_PUTC(ch, stream);
    smtp_timeout_unprotect();

    /*
     * See if there was a problem.
     */
    if (stat == VSTREAM_EOF) {
	if (msg_verbose)
	    msg_info("smtp_fputc: EOF");
	longjmp(smtp_timeout_buf, SMTP_ERR_EOF);
    }
}
