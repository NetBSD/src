/*	$NetBSD: vstream.c,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

/*++
/* NAME
/*	vstream 3
/* SUMMARY
/*	light-weight buffered I/O package
/* SYNOPSIS
/*	#include <vstream.h>
/*
/*	VSTREAM	*vstream_fopen(path, flags, mode)
/*	const char *path;
/*	int	flags;
/*	mode_t	mode;
/*
/*	VSTREAM	*vstream_fdopen(fd, flags)
/*	int	fd;
/*	int	flags;
/*
/*	int	vstream_fclose(stream)
/*	VSTREAM	*stream;
/*
/*	int	vstream_fdclose(stream)
/*	VSTREAM	*stream;
/*
/*	VSTREAM	*vstream_printf(format, ...)
/*	const char *format;
/*
/*	VSTREAM *vstream_fprintf(stream, format, ...)
/*	VSTREAM	*stream;
/*	const char *format;
/*
/*	int	VSTREAM_GETC(stream)
/*	VSTREAM	*stream;
/*
/*	int	VSTREAM_PUTC(ch, stream)
/*	int	ch;
/*
/*	int	VSTREAM_GETCHAR(void)
/*
/*	int	VSTREAM_PUTCHAR(ch)
/*	int	ch;
/*
/*	int	vstream_ungetc(stream, ch)
/*	VSTREAM	*stream;
/*	int	ch;
/*
/*	int	vstream_fputs(str, stream)
/*	const char *str;
/*	VSTREAM	*stream;
/*
/*	off_t	vstream_ftell(stream)
/*	VSTREAM	*stream;
/*
/*	off_t	vstream_fseek(stream, offset, whence)
/*	VSTREAM	*stream;
/*	off_t	offset;
/*	int	whence;
/*
/*	int	vstream_fflush(stream)
/*	VSTREAM	*stream;
/*
/*	int	vstream_fpurge(stream, direction)
/*	VSTREAM	*stream;
/*	int	direction;
/*
/*	ssize_t	vstream_fread(stream, buf, len)
/*	VSTREAM	*stream;
/*	void	*buf;
/*	ssize_t	len;
/*
/*	ssize_t	vstream_fwrite(stream, buf, len)
/*	VSTREAM	*stream;
/*	const void *buf;
/*	ssize_t	len;
/*
/*	void	vstream_control(stream, name, ...)
/*	VSTREAM	*stream;
/*	int	name;
/*
/*	int	vstream_fileno(stream)
/*	VSTREAM	*stream;
/*
/*	const ssize_t vstream_req_bufsize(stream)
/*	VSTREAM	*stream;
/*
/*	void	*vstream_context(stream)
/*	VSTREAM	*stream;
/*
/*	int	vstream_ferror(stream)
/*	VSTREAM	*stream;
/*
/*	int	vstream_ftimeout(stream)
/*	VSTREAM	*stream;
/*
/*	int	vstream_feof(stream)
/*	VSTREAM	*stream;
/*
/*	int	vstream_clearerr(stream)
/*	VSTREAM	*stream;
/*
/*	const char *VSTREAM_PATH(stream)
/*	VSTREAM	*stream;
/*
/*	char	*vstream_vprintf(format, ap)
/*	const char *format;
/*	va_list	*ap;
/*
/*	char	*vstream_vfprintf(stream, format, ap)
/*	VSTREAM	*stream;
/*	const char *format;
/*	va_list	*ap;
/*
/*	ssize_t	vstream_bufstat(stream, command)
/*	VSTREAM	*stream;
/*	int	command;
/*
/*	ssize_t	vstream_peek(stream)
/*	VSTREAM	*stream;
/*
/*	const char *vstream_peek_data(stream)
/*	VSTREAM	*stream;
/*
/*	int	vstream_setjmp(stream)
/*	VSTREAM	*stream;
/*
/*	void	vstream_longjmp(stream, val)
/*	VSTREAM	*stream;
/*	int	val;
/*
/*	time_t	vstream_ftime(stream)
/*	VSTREAM	*stream;
/*
/*	struct timeval vstream_ftimeval(stream)
/*	VSTREAM	*stream;
/*
/*	int	vstream_rd_error(stream)
/*	VSTREAM	*stream;
/*
/*	int	vstream_wr_error(stream)
/*	VSTREAM	*stream;
/*
/*	int	vstream_rd_timeout(stream)
/*	VSTREAM	*stream;
/*
/*	int	vstream_wr_timeout(stream)
/*	VSTREAM	*stream;
/*
/*	int	vstream_fstat(stream, flags)
/*	VSTREAM	*stream;
/*	int	flags;
/* DESCRIPTION
/*	The \fIvstream\fR module implements light-weight buffered I/O
/*	similar to the standard I/O routines.
/*
/*	The interface is implemented in terms of VSTREAM structure
/*	pointers, also called streams. For convenience, three streams
/*	are predefined: VSTREAM_IN, VSTREAM_OUT, and VSTREAM_ERR. These
/*	streams are connected to the standard input, output and error
/*	file descriptors, respectively.
/*
/*	Although the interface is patterned after the standard I/O
/*	library, there are some major differences:
/* .IP \(bu
/*	File descriptors are not limited to the range 0..255. This
/*	was reason #1 to write these routines in the first place.
/* .IP \(bu
/*	The application can switch between reading and writing on
/*	the same stream without having to perform a flush or seek
/*	operation, and can change write position without having to
/*	flush.  This was reason #2. Upon position or direction change,
/*	unread input is discarded, and unwritten output is flushed
/*	automatically. Exception: with double-buffered streams, unread
/*	input is not discarded upon change of I/O direction, and
/*	output flushing is delayed until the read buffer must be refilled.
/* .IP \(bu
/*	A bidirectional stream can read and write with the same buffer
/*	and file descriptor, or it can have separate read/write
/*	buffers and/or file descriptors.
/* .IP \(bu
/*	No automatic flushing of VSTREAM_OUT upon program exit, or of
/*	VSTREAM_ERR at any time. No unbuffered or line buffered modes.
/*	This functionality may be added when it is really needed.
/* .PP
/*	vstream_fopen() opens the named file and associates a buffered
/*	stream with it.  The \fIpath\fR, \fIflags\fR and \fImode\fR
/*	arguments are passed on to the open(2) routine. The result is
/*	a null pointer in case of problems. The \fIpath\fR argument is
/*	copied and can be looked up with VSTREAM_PATH().
/*
/*	vstream_fdopen() takes an open file and associates a buffered
/*	stream with it. The \fIflags\fR argument specifies how the file
/*	was opened. vstream_fdopen() either succeeds or never returns.
/*
/*	vstream_fclose() closes the named buffered stream. The result
/*	is 0 in case of success, VSTREAM_EOF in case of problems.
/*	vstream_fclose() reports the same errors as vstream_ferror().
/*
/*	vstream_fdclose() leaves the file(s) open but is otherwise
/*	identical to vstream_fclose().
/*
/*	vstream_fprintf() formats its arguments according to the
/*	\fIformat\fR argument and writes the result to the named stream.
/*	The result is the stream argument. It understands the s, c, d, u,
/*	o, x, X, e, f and g format types, the l modifier, field width and
/*	precision, sign, and padding with zeros or spaces. In addition,
/*	vstream_fprintf() recognizes the %m format specifier and expands
/*	it to the error message corresponding to the current value of the
/*	global \fIerrno\fR variable.
/*
/*	vstream_printf() performs formatted output to the standard output
/*	stream.
/*
/*	VSTREAM_GETC() reads the next character from the named stream.
/*	The result is VSTREAM_EOF when end-of-file is reached or if a read
/*	error was detected. VSTREAM_GETC() is an unsafe macro that
/*	evaluates some arguments more than once.
/*
/*	VSTREAM_GETCHAR() is an alias for VSTREAM_GETC(VSTREAM_IN).
/*
/*	VSTREAM_PUTC() appends the specified character to the specified
/*	stream. The result is the stored character, or VSTREAM_EOF in
/*	case of problems. VSTREAM_PUTC() is an unsafe macro that
/*	evaluates some arguments more than once.
/*
/*	VSTREAM_PUTCHAR(c) is an alias for VSTREAM_PUTC(c, VSTREAM_OUT).
/*
/*	vstream_ungetc() pushes back a character onto the specified stream
/*	and returns the character, or VSTREAM_EOF in case of problems.
/*	It is an error to push back before reading (or immediately after
/*	changing the stream offset via vstream_fseek()). Upon successful
/*	return, vstream_ungetc() clears the end-of-file stream flag.
/*
/*	vstream_fputs() appends the given null-terminated string to the
/*	specified buffered stream. The result is 0 in case of success,
/*	VSTREAM_EOF in case of problems.
/*
/*	vstream_ftell() returns the file offset for the specified stream,
/*	-1 if the stream is connected to a non-seekable file.
/*
/*	vstream_fseek() changes the file position for the next read or write
/*	operation. Unwritten output is flushed. With unidirectional streams,
/*	unread input is discarded. The \fIoffset\fR argument specifies the file
/*	position from the beginning of the file (\fIwhence\fR is SEEK_SET),
/*	from the current file position (\fIwhence\fR is SEEK_CUR), or from
/*	the file end (SEEK_END). The result value is the file offset
/*	from the beginning of the file, -1 in case of problems.
/*
/*	vstream_fflush() flushes unwritten data to a file that was
/*	opened in read-write or write-only mode.
/*	vstream_fflush() returns 0 in case of success, VSTREAM_EOF in
/*	case of problems. It is an error to flush a read-only stream.
/*	vstream_fflush() reports the same errors as vstream_ferror().
/*
/*	vstream_fpurge() discards the contents of the stream buffer.
/*	If direction is VSTREAM_PURGE_READ, it discards unread data,
/*	else if direction is VSTREAM_PURGE_WRITE, it discards unwritten
/*	data. In the case of a double-buffered stream, if direction is
/*	VSTREAM_PURGE_BOTH, it discards the content of both the read
/*	and write buffers. vstream_fpurge() returns 0 in case of success,
/*	VSTREAM_EOF in case of problems.
/*
/*	vstream_fread() and vstream_fwrite() perform unformatted I/O
/*	on the named stream. The result value is the number of bytes
/*	transferred. A short count is returned in case of end-of-file
/*	or error conditions.
/*
/*	vstream_control() allows the user to fine tune the behavior of
/*	the specified stream.  The arguments are a list of macros with
/*	zero or more arguments, terminated with CA_VSTREAM_CTL_END
/*	which has none.  The following lists the names and the types
/*	of the corresponding value arguments.
/* .IP "CA_VSTREAM_CTL_READ_FN(ssize_t (*)(int, void *, size_t, int, void *))"
/*	The argument specifies an alternative for the timed_read(3) function,
/*	for example, a read function that performs decryption.
/*	This function receives as arguments a file descriptor, buffer pointer,
/*	buffer length, timeout value, and the VSTREAM's context value.
/*	A timeout value <= 0 disables the time limit.
/*	This function should return the positive number of bytes transferred,
/*	0 upon EOF, and -1 upon error with errno set appropriately.
/* .IP "CA_VSTREAM_CTL_WRITE_FN(ssize_t (*)(int, void *, size_t, int, void *))"
/*	The argument specifies an alternative for the timed_write(3) function,
/*	for example, a write function that performs encryption.
/*	This function receives as arguments a file descriptor, buffer pointer,
/*	buffer length, timeout value, and the VSTREAM's context value.
/*	A timeout value <= 0 disables the time limit.
/*	This function should return the positive number of bytes transferred,
/*	and -1 upon error with errno set appropriately. Instead of -1 it may
/*	also return 0, e.g., upon remote party-initiated protocol shutdown.
/* .IP "CA_VSTREAM_CTL_CONTEXT(void *)"
/*	The argument specifies application context that is passed on to
/*	the application-specified read/write routines. No copy is made.
/* .IP "CA_VSTREAM_CTL_PATH(const char *)"
/*	Updates the stored pathname of the specified stream. The pathname
/*	is copied.
/* .IP "CA_VSTREAM_CTL_DOUBLE (no arguments)"
/*	Use separate buffers for reading and for writing.  This prevents
/*	unread input from being discarded upon change of I/O direction.
/* .IP "CA_VSTREAM_CTL_READ_FD(int)"
/*	The argument specifies the file descriptor to be used for reading.
/*	This feature is limited to double-buffered streams, and makes the
/*	stream non-seekable.
/* .IP "CA_VSTREAM_CTL_WRITE_FD(int)"
/*	The argument specifies the file descriptor to be used for writing.
/*	This feature is limited to double-buffered streams, and makes the
/*	stream non-seekable.
/* .IP "CA_VSTREAM_CTL_SWAP_FD(VSTREAM *)"
/*	The argument specifies a VSTREAM pointer; the request swaps the
/*	file descriptor members of the two streams. This feature is limited
/*	to streams that are both double-buffered or both single-buffered.
/* .IP "CA_VSTREAM_CTL_DUPFD(int)"
/*	The argument specifies a minimum file descriptor value. If
/*	the actual stream's file descriptors are below the minimum,
/*	reallocate the descriptors to the first free value greater
/*	than or equal to the minimum. The VSTREAM_CTL_DUPFD macro
/*	is defined only on systems with fcntl() F_DUPFD support.
/* .IP "CA_VSTREAM_CTL_WAITPID_FN(int (*)(pid_t, WAIT_STATUS_T *, int))"
/*	A pointer to function that behaves like waitpid(). This information
/*	is used by the vstream_pclose() routine.
/* .IP "CA_VSTREAM_CTL_TIMEOUT(int)"
/*	The deadline for a descriptor to become readable in case of a read
/*	request, or writable in case of a write request. Specify a value
/*	of 0 to disable deadlines.
/* .IP "CA_VSTREAM_CTL_EXCEPT (no arguments)"
/*	Enable exception handling with vstream_setjmp() and vstream_longjmp().
/*	This involves allocation of additional memory that normally isn't
/*	used.
/* .IP "CA_VSTREAM_CTL_BUFSIZE(ssize_t)"
/*	Specify a non-default buffer size for the next read(2) or
/*	write(2) operation, or zero to implement a no-op. Requests
/*	to reduce the buffer size are silently ignored (i.e. any
/*	positive value <= vstream_req_bufsize()). To get a buffer
/*	size smaller than VSTREAM_BUFSIZE, make the VSTREAM_CTL_BUFSIZE
/*	request before the first stream read or write operation
/*	(i.e., vstream_req_bufsize() returns zero).  Requests to
/*	change a fixed-size buffer (i.e., VSTREAM_ERR) are not
/*	allowed.
/*
/*	NOTE: the vstream_*printf() routines may silently expand a
/*	buffer, so that the result of some %letter specifiers can
/*	be written to contiguous memory.
/* .IP CA_VSTREAM_CTL_START_DEADLINE (no arguments)
/*	Change the VSTREAM_CTL_TIMEOUT behavior, to limit the total
/*	time for all subsequent file descriptor read or write
/*	operations, and recharge the deadline timer.
/* .IP CA_VSTREAM_CTL_STOP_DEADLINE (no arguments)
/*	Revert VSTREAM_CTL_TIMEOUT behavior to the default, i.e.
/*	a time limit for individual file descriptor read or write
/*	operations.
/* .PP
/*	vstream_fileno() gives access to the file handle associated with
/*	a buffered stream. With streams that have separate read/write
/*	file descriptors, the result is the current descriptor.
/*
/*	vstream_req_bufsize() returns the buffer size that will be
/*	used for the next read(2) or write(2) operation on the named
/*	stream. A zero result means that the next read(2) or write(2)
/*	operation will use the default buffer size (VSTREAM_BUFSIZE).
/*
/*	vstream_context() returns the application context that is passed on to
/*	the application-specified read/write routines.
/*
/*	VSTREAM_PATH() is an unsafe macro that returns the name stored
/*	with vstream_fopen() or with vstream_control(). The macro is
/*	unsafe because it evaluates some arguments more than once.
/*
/*	vstream_feof() returns non-zero when a previous operation on the
/*	specified stream caused an end-of-file condition.
/*	Although further read requests after EOF may complete
/*	succesfully, vstream_feof() will keep returning non-zero
/*	until vstream_clearerr() is called for that stream.
/*
/*	vstream_ferror() returns non-zero when a previous operation on the
/*	specified stream caused a non-EOF error condition, including timeout.
/*	After a non-EOF error on a stream, no I/O request will
/*	complete until after vstream_clearerr() is called for that stream.
/*
/*	vstream_ftimeout() returns non-zero when a previous operation on the
/*	specified stream caused a timeout error condition. See
/*	vstream_ferror() for error persistence details.
/*
/*	vstream_clearerr() resets the timeout, error and end-of-file indication
/*	of the specified stream, and returns no useful result.
/*
/*	vstream_vfprintf() provides an alternate interface
/*	for formatting an argument list according to a format string.
/*
/*	vstream_vprintf() provides a similar alternative interface.
/*
/*	vstream_bufstat() provides input and output buffer status
/*	information.  The command is one of the following:
/* .IP VSTREAM_BST_IN_PEND
/*	Return the number of characters that can be read without
/*	refilling the read buffer.
/* .IP VSTREAM_BST_OUT_PEND
/*	Return the number of characters that are waiting in the
/*	write buffer.
/* .PP
/*	vstream_peek() returns the number of characters that can be
/*	read from the named stream without refilling the read buffer.
/*	This is an alias for vstream_bufstat(stream, VSTREAM_BST_IN_PEND).
/*
/*	vstream_peek_data() returns a pointer to the unread bytes
/*	that exist according to vstream_peek(), or null if no unread
/*	bytes are available.
/*
/*	vstream_setjmp() saves processing context and makes that context
/*	available for use with vstream_longjmp().  Normally, vstream_setjmp()
/*	returns zero.  A non-zero result means that vstream_setjmp() returned
/*	through a vstream_longjmp() call; the result is the \fIval\fR argument
/*	given to vstream_longjmp().
/*
/*	NB: non-local jumps such as vstream_longjmp() are not safe
/*	for jumping out of any routine that manipulates VSTREAM data.
/*	longjmp() like calls are best avoided in signal handlers.
/*
/*	vstream_ftime() returns the time of initialization, the last buffer
/*	fill operation, or the last buffer flush operation for the specified
/*	stream. This information is maintained only when stream timeouts are
/*	enabled.
/*
/*	vstream_ftimeval() is like vstream_ftime() but returns more
/*	detail.
/*
/*	vstream_rd_mumble() and vstream_wr_mumble() report on
/*	read and write error conditions, respectively.
/*
/*	vstream_fstat() queries stream status information about
/*	user-requested features. The \fIflags\fR argument is the
/*	bitwise OR of one or more of the following, and the result
/*	value is the bitwise OR of the features that are activated.
/* .IP VSTREAM_FLAG_DEADLINE
/*	The deadline feature is activated.
/* .IP VSTREAM_FLAG_DOUBLE
/*	The double-buffering feature is activated.
/* DIAGNOSTICS
/*	Panics: interface violations. Fatal errors: out of memory.
/* SEE ALSO
/*	timed_read(3) default read routine
/*	timed_write(3) default write routine
/*	vbuf_print(3) formatting engine
/*	setjmp(3) non-local jumps
/* BUGS
/*	Should use mmap() on reasonable systems.
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
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <string.h>

/* Utility library. */

#include "mymalloc.h"
#include "msg.h"
#include "vbuf_print.h"
#include "iostuff.h"
#include "vstring.h"
#include "vstream.h"

/* Application-specific. */

 /*
  * Forward declarations.
  */
static int vstream_buf_get_ready(VBUF *);
static int vstream_buf_put_ready(VBUF *);
static int vstream_buf_space(VBUF *, ssize_t);

 /*
  * Initialization of the three pre-defined streams. Pre-allocate a static
  * I/O buffer for the standard error stream, so that the error handler can
  * produce a diagnostic even when memory allocation fails.
  */
static unsigned char vstream_fstd_buf[VSTREAM_BUFSIZE];

VSTREAM vstream_fstd[] = {
    {{
	    0,				/* flags */
	    0, 0, 0, 0,			/* buffer */
	    vstream_buf_get_ready, vstream_buf_put_ready, vstream_buf_space,
    }, STDIN_FILENO, (VSTREAM_RW_FN) timed_read, (VSTREAM_RW_FN) timed_write,
    0,},
    {{
	    0,				/* flags */
	    0, 0, 0, 0,			/* buffer */
	    vstream_buf_get_ready, vstream_buf_put_ready, vstream_buf_space,
    }, STDOUT_FILENO, (VSTREAM_RW_FN) timed_read, (VSTREAM_RW_FN) timed_write,
    0,},
    {{
	    VBUF_FLAG_FIXED | VSTREAM_FLAG_WRITE,
	    vstream_fstd_buf, VSTREAM_BUFSIZE, VSTREAM_BUFSIZE, vstream_fstd_buf,
	    vstream_buf_get_ready, vstream_buf_put_ready, vstream_buf_space,
    }, STDERR_FILENO, (VSTREAM_RW_FN) timed_read, (VSTREAM_RW_FN) timed_write,
    VSTREAM_BUFSIZE,},
};

#define VSTREAM_STATIC(v) ((v) >= VSTREAM_IN && (v) <= VSTREAM_ERR)

 /*
  * A bunch of macros to make some expressions more readable. XXX We're
  * assuming that O_RDONLY == 0, O_WRONLY == 1, O_RDWR == 2.
  */
#define VSTREAM_ACC_MASK(f)	((f) & (O_APPEND | O_WRONLY | O_RDWR))

#define VSTREAM_CAN_READ(f)	(VSTREAM_ACC_MASK(f) == O_RDONLY \
				|| VSTREAM_ACC_MASK(f) == O_RDWR)
#define VSTREAM_CAN_WRITE(f)	(VSTREAM_ACC_MASK(f) & O_WRONLY \
				|| VSTREAM_ACC_MASK(f) & O_RDWR \
				|| VSTREAM_ACC_MASK(f) & O_APPEND)

#define VSTREAM_BUF_COUNT(bp, n) \
	((bp)->flags & VSTREAM_FLAG_READ ? -(n) : (n))

#define VSTREAM_BUF_AT_START(bp) { \
	(bp)->cnt = VSTREAM_BUF_COUNT((bp), (bp)->len); \
	(bp)->ptr = (bp)->data; \
    }

#define VSTREAM_BUF_AT_OFFSET(bp, offset) { \
	(bp)->ptr = (bp)->data + (offset); \
	(bp)->cnt = VSTREAM_BUF_COUNT(bp, (bp)->len - (offset)); \
    }

#define VSTREAM_BUF_AT_END(bp) { \
	(bp)->cnt = 0; \
	(bp)->ptr = (bp)->data + (bp)->len; \
    }

#define VSTREAM_BUF_ZERO(bp) { \
	(bp)->flags = 0; \
	(bp)->data = (bp)->ptr = 0; \
	(bp)->len = (bp)->cnt = 0; \
    }

#define VSTREAM_BUF_ACTIONS(bp, get_action, put_action, space_action) { \
	(bp)->get_ready = (get_action); \
	(bp)->put_ready = (put_action); \
	(bp)->space = (space_action); \
    }

#define VSTREAM_SAVE_STATE(stream, buffer, filedes) { \
	stream->buffer = stream->buf; \
	stream->filedes = stream->fd; \
    }

#define VSTREAM_RESTORE_STATE(stream, buffer, filedes) do { \
	stream->buffer.flags = stream->buf.flags; \
	stream->buf = stream->buffer; \
	stream->fd = stream->filedes; \
    } while(0)

#define VSTREAM_FORK_STATE(stream, buffer, filedes) { \
	stream->buffer = stream->buf; \
	stream->filedes = stream->fd; \
	stream->buffer.data = stream->buffer.ptr = 0; \
	stream->buffer.len = stream->buffer.cnt = 0; \
	stream->buffer.flags &= ~VSTREAM_FLAG_FIXED; \
    };

#define VSTREAM_FLAG_READ_DOUBLE (VSTREAM_FLAG_READ | VSTREAM_FLAG_DOUBLE)
#define VSTREAM_FLAG_WRITE_DOUBLE (VSTREAM_FLAG_WRITE | VSTREAM_FLAG_DOUBLE)

#define VSTREAM_FFLUSH_SOME(stream) \
	vstream_fflush_some((stream), (stream)->buf.len - (stream)->buf.cnt)

/* Note: this does not change a negative result into a zero result. */
#define VSTREAM_SUB_TIME(x, y, z) \
    do { \
	(x).tv_sec = (y).tv_sec - (z).tv_sec; \
	(x).tv_usec = (y).tv_usec - (z).tv_usec; \
	while ((x).tv_usec < 0) { \
	    (x).tv_usec += 1000000; \
	    (x).tv_sec -= 1; \
	} \
	while ((x).tv_usec >= 1000000) { \
	    (x).tv_usec -= 1000000; \
	    (x).tv_sec += 1; \
	} \
    } while (0)

/* vstream_buf_init - initialize buffer */

static void vstream_buf_init(VBUF *bp, int flags)
{

    /*
     * Initialize the buffer such that the first data access triggers a
     * buffer boundary action.
     */
    VSTREAM_BUF_ZERO(bp);
    VSTREAM_BUF_ACTIONS(bp,
			VSTREAM_CAN_READ(flags) ? vstream_buf_get_ready : 0,
			VSTREAM_CAN_WRITE(flags) ? vstream_buf_put_ready : 0,
			vstream_buf_space);
}

/* vstream_buf_alloc - allocate buffer memory */

static void vstream_buf_alloc(VBUF *bp, ssize_t len)
{
    VSTREAM *stream = VBUF_TO_APPL(bp, VSTREAM, buf);
    ssize_t used = bp->ptr - bp->data;
    const char *myname = "vstream_buf_alloc";

    if (len < bp->len)
	msg_panic("%s: attempt to shrink buffer", myname);
    if (bp->flags & VSTREAM_FLAG_FIXED)
	msg_panic("%s: unable to extend fixed-size buffer", myname);

    /*
     * Late buffer allocation allows the user to override the default policy.
     * If a buffer already exists, allow for the presence of (output) data.
     */
    bp->data = (unsigned char *)
	(bp->data ? myrealloc((void *) bp->data, len) : mymalloc(len));
    bp->len = len;
    if (bp->flags & VSTREAM_FLAG_READ) {
	bp->ptr = bp->data + used;
	if (bp->flags & VSTREAM_FLAG_DOUBLE)
	    VSTREAM_SAVE_STATE(stream, read_buf, read_fd);
    } else {
	VSTREAM_BUF_AT_OFFSET(bp, used);
	if (bp->flags & VSTREAM_FLAG_DOUBLE)
	    VSTREAM_SAVE_STATE(stream, write_buf, write_fd);
    }
}

/* vstream_buf_wipe - reset buffer to initial state */

static void vstream_buf_wipe(VBUF *bp)
{
    if ((bp->flags & VBUF_FLAG_FIXED) == 0 && bp->data)
	myfree((void *) bp->data);
    VSTREAM_BUF_ZERO(bp);
    VSTREAM_BUF_ACTIONS(bp, 0, 0, 0);
}

/* vstream_fflush_some - flush some buffered data */

static int vstream_fflush_some(VSTREAM *stream, ssize_t to_flush)
{
    const char *myname = "vstream_fflush_some";
    VBUF   *bp = &stream->buf;
    ssize_t used;
    ssize_t left_over;
    void   *data;
    ssize_t len;
    ssize_t n;
    int     timeout;
    struct timeval before;
    struct timeval elapsed;

    /*
     * Sanity checks. It is illegal to flush a read-only stream. Otherwise,
     * if there is buffered input, discard the input. If there is buffered
     * output, require that the amount to flush is larger than the amount to
     * keep, so that we can memcpy() the residue.
     */
    if (bp->put_ready == 0)
	msg_panic("%s: read-only stream", myname);
    switch (bp->flags & (VSTREAM_FLAG_WRITE | VSTREAM_FLAG_READ)) {
    case VSTREAM_FLAG_READ:			/* discard input */
	VSTREAM_BUF_AT_END(bp);
	/* FALLTHROUGH */
    case 0:					/* flush after seek? */
	return ((bp->flags & VSTREAM_FLAG_ERR) ? VSTREAM_EOF : 0);
    case VSTREAM_FLAG_WRITE:			/* output buffered */
	break;
    case VSTREAM_FLAG_WRITE | VSTREAM_FLAG_READ:
	msg_panic("%s: read/write stream", myname);
    }
    used = bp->len - bp->cnt;
    left_over = used - to_flush;

    if (msg_verbose > 2 && stream != VSTREAM_ERR)
	msg_info("%s: fd %d flush %ld", myname, stream->fd, (long) to_flush);
    if (to_flush < 0 || left_over < 0)
	msg_panic("%s: bad to_flush %ld", myname, (long) to_flush);
    if (to_flush < left_over)
	msg_panic("%s: to_flush < left_over", myname);
    if (to_flush == 0)
	return ((bp->flags & VSTREAM_FLAG_ERR) ? VSTREAM_EOF : 0);
    if (bp->flags & VSTREAM_FLAG_ERR)
	return (VSTREAM_EOF);

    /*
     * When flushing a buffer, allow for partial writes. These can happen
     * while talking to a network. Update the cached file seek position, if
     * any.
     * 
     * When deadlines are enabled, we count the elapsed time for each write
     * operation instead of simply comparing the time-of-day clock with a
     * per-stream deadline. The latter could result in anomalies when an
     * application does lengthy processing between write operations. Keep in
     * mind that a receiver may not be able to keep up when a sender suddenly
     * floods it with a lot of data as it tries to catch up with a deadline.
     */
    for (data = (void *) bp->data, len = to_flush; len > 0; len -= n, data += n) {
	if (bp->flags & VSTREAM_FLAG_DEADLINE) {
	    timeout = stream->time_limit.tv_sec + (stream->time_limit.tv_usec > 0);
	    if (timeout <= 0) {
		bp->flags |= (VSTREAM_FLAG_WR_ERR | VSTREAM_FLAG_WR_TIMEOUT);
		errno = ETIMEDOUT;
		return (VSTREAM_EOF);
	    }
	    if (len == to_flush)
		GETTIMEOFDAY(&before);
	    else
		before = stream->iotime;
	} else
	    timeout = stream->timeout;
	if ((n = stream->write_fn(stream->fd, data, len, timeout, stream->context)) <= 0) {
	    bp->flags |= VSTREAM_FLAG_WR_ERR;
	    if (errno == ETIMEDOUT) {
		bp->flags |= VSTREAM_FLAG_WR_TIMEOUT;
		stream->time_limit.tv_sec = stream->time_limit.tv_usec = 0;
	    }
	    return (VSTREAM_EOF);
	}
	if (timeout) {
	    GETTIMEOFDAY(&stream->iotime);
	    if (bp->flags & VSTREAM_FLAG_DEADLINE) {
		VSTREAM_SUB_TIME(elapsed, stream->iotime, before);
		VSTREAM_SUB_TIME(stream->time_limit, stream->time_limit, elapsed);
	    }
	}
	if (msg_verbose > 2 && stream != VSTREAM_ERR && n != to_flush)
	    msg_info("%s: %d flushed %ld/%ld", myname, stream->fd,
		     (long) n, (long) to_flush);
    }
    if (bp->flags & VSTREAM_FLAG_SEEK)
	stream->offset += to_flush;

    /*
     * Allow for partial buffer flush requests. We use memcpy() for reasons
     * of portability to pre-ANSI environments (SunOS 4.x or Ultrix 4.x :-).
     * This is OK because we have already verified that the to_flush count is
     * larger than the left_over count.
     */
    if (left_over > 0)
	memcpy(bp->data, bp->data + to_flush, left_over);
    bp->cnt += to_flush;
    bp->ptr -= to_flush;
    return ((bp->flags & VSTREAM_FLAG_ERR) ? VSTREAM_EOF : 0);
}

/* vstream_fflush_delayed - delayed stream flush for double-buffered stream */

static int vstream_fflush_delayed(VSTREAM *stream)
{
    int     status;

    /*
     * Sanity check.
     */
    if ((stream->buf.flags & VSTREAM_FLAG_READ_DOUBLE) != VSTREAM_FLAG_READ_DOUBLE)
	msg_panic("vstream_fflush_delayed: bad flags");

    /*
     * Temporarily swap buffers and flush unwritten data. This may seem like
     * a lot of work, but it's peanuts compared to the write(2) call that we
     * already have avoided. For example, delayed flush is never used on a
     * non-pipelined SMTP connection.
     */
    stream->buf.flags &= ~VSTREAM_FLAG_READ;
    VSTREAM_SAVE_STATE(stream, read_buf, read_fd);
    stream->buf.flags |= VSTREAM_FLAG_WRITE;
    VSTREAM_RESTORE_STATE(stream, write_buf, write_fd);

    status = VSTREAM_FFLUSH_SOME(stream);

    stream->buf.flags &= ~VSTREAM_FLAG_WRITE;
    VSTREAM_SAVE_STATE(stream, write_buf, write_fd);
    stream->buf.flags |= VSTREAM_FLAG_READ;
    VSTREAM_RESTORE_STATE(stream, read_buf, read_fd);

    return (status);
}

/* vstream_buf_get_ready - vbuf callback to make buffer ready for reading */

static int vstream_buf_get_ready(VBUF *bp)
{
    VSTREAM *stream = VBUF_TO_APPL(bp, VSTREAM, buf);
    const char *myname = "vstream_buf_get_ready";
    ssize_t n;
    struct timeval before;
    struct timeval elapsed;
    int     timeout;

    /*
     * Detect a change of I/O direction or position. If so, flush any
     * unwritten output immediately when the stream is single-buffered, or
     * when the stream is double-buffered and the read buffer is empty.
     */
    switch (bp->flags & (VSTREAM_FLAG_WRITE | VSTREAM_FLAG_READ)) {
    case VSTREAM_FLAG_WRITE:			/* change direction */
	if (bp->ptr > bp->data)
	    if ((bp->flags & VSTREAM_FLAG_DOUBLE) == 0
		|| stream->read_buf.cnt >= 0)
		if (VSTREAM_FFLUSH_SOME(stream))
		    return (VSTREAM_EOF);
	bp->flags &= ~VSTREAM_FLAG_WRITE;
	if (bp->flags & VSTREAM_FLAG_DOUBLE)
	    VSTREAM_SAVE_STATE(stream, write_buf, write_fd);
	/* FALLTHROUGH */
    case 0:					/* change position */
	bp->flags |= VSTREAM_FLAG_READ;
	if (bp->flags & VSTREAM_FLAG_DOUBLE) {
	    VSTREAM_RESTORE_STATE(stream, read_buf, read_fd);
	    if (bp->cnt < 0)
		return (0);
	}
	/* FALLTHROUGH */
    case VSTREAM_FLAG_READ:			/* no change */
	break;
    case VSTREAM_FLAG_WRITE | VSTREAM_FLAG_READ:
	msg_panic("%s: read/write stream", myname);
    }

    /*
     * If this is the first GET operation, allocate a buffer. Late buffer
     * allocation gives the application a chance to override the default
     * buffering policy.
     * 
     * XXX Subtle code to set the preferred buffer size as late as possible.
     */
    if (stream->req_bufsize == 0)
	stream->req_bufsize = VSTREAM_BUFSIZE;
    if (bp->len < stream->req_bufsize)
	vstream_buf_alloc(bp, stream->req_bufsize);

    /*
     * If the stream is double-buffered and the write buffer is not empty,
     * this is the time to flush the write buffer. Delayed flushes reduce
     * system call overhead, and on TCP sockets, avoid triggering Nagle's
     * algorithm.
     */
    if ((bp->flags & VSTREAM_FLAG_DOUBLE)
	&& stream->write_buf.len > stream->write_buf.cnt)
	if (vstream_fflush_delayed(stream))
	    return (VSTREAM_EOF);

    /*
     * Did we receive an EOF indication?
     */
    if (bp->flags & VSTREAM_FLAG_EOF)
	return (VSTREAM_EOF);

    /*
     * Fill the buffer with as much data as we can handle, or with as much
     * data as is available right now, whichever is less. Update the cached
     * file seek position, if any.
     * 
     * When deadlines are enabled, we count the elapsed time for each read
     * operation instead of simply comparing the time-of-day clock with a
     * per-stream deadline. The latter could result in anomalies when an
     * application does lengthy processing between read operations. Keep in
     * mind that a sender may get blocked, and may not be able to keep up
     * when a receiver suddenly wants to read a lot of data as it tries to
     * catch up with a deadline.
     */
    if (bp->flags & VSTREAM_FLAG_DEADLINE) {
	timeout = stream->time_limit.tv_sec + (stream->time_limit.tv_usec > 0);
	if (timeout <= 0) {
	    bp->flags |= (VSTREAM_FLAG_RD_ERR | VSTREAM_FLAG_RD_TIMEOUT);
	    errno = ETIMEDOUT;
	    return (VSTREAM_EOF);
	}
	GETTIMEOFDAY(&before);
    } else
	timeout = stream->timeout;
    switch (n = stream->read_fn(stream->fd, bp->data, bp->len, timeout, stream->context)) {
    case -1:
	bp->flags |= VSTREAM_FLAG_RD_ERR;
	if (errno == ETIMEDOUT) {
	    bp->flags |= VSTREAM_FLAG_RD_TIMEOUT;
	    stream->time_limit.tv_sec = stream->time_limit.tv_usec = 0;
	}
	return (VSTREAM_EOF);
    case 0:
	bp->flags |= VSTREAM_FLAG_EOF;
	return (VSTREAM_EOF);
    default:
	if (timeout) {
	    GETTIMEOFDAY(&stream->iotime);
	    if (bp->flags & VSTREAM_FLAG_DEADLINE) {
		VSTREAM_SUB_TIME(elapsed, stream->iotime, before);
		VSTREAM_SUB_TIME(stream->time_limit, stream->time_limit, elapsed);
	    }
	}
	if (msg_verbose > 2)
	    msg_info("%s: fd %d got %ld", myname, stream->fd, (long) n);
	bp->cnt = -n;
	bp->ptr = bp->data;
	if (bp->flags & VSTREAM_FLAG_SEEK)
	    stream->offset += n;
	return (0);
    }
}

/* vstream_buf_put_ready - vbuf callback to make buffer ready for writing */

static int vstream_buf_put_ready(VBUF *bp)
{
    VSTREAM *stream = VBUF_TO_APPL(bp, VSTREAM, buf);
    const char *myname = "vstream_buf_put_ready";

    /*
     * Sanity checks. Detect a change of I/O direction or position. If so,
     * discard unread input, and reset the buffer to the beginning.
     */
    switch (bp->flags & (VSTREAM_FLAG_WRITE | VSTREAM_FLAG_READ)) {
    case VSTREAM_FLAG_READ:			/* change direction */
	bp->flags &= ~VSTREAM_FLAG_READ;
	if (bp->flags & VSTREAM_FLAG_DOUBLE)
	    VSTREAM_SAVE_STATE(stream, read_buf, read_fd);
	/* FALLTHROUGH */
    case 0:					/* change position */
	bp->flags |= VSTREAM_FLAG_WRITE;
	if (bp->flags & VSTREAM_FLAG_DOUBLE)
	    VSTREAM_RESTORE_STATE(stream, write_buf, write_fd);
	else
	    VSTREAM_BUF_AT_START(bp);
	/* FALLTHROUGH */
    case VSTREAM_FLAG_WRITE:			/* no change */
	break;
    case VSTREAM_FLAG_WRITE | VSTREAM_FLAG_READ:
	msg_panic("%s: read/write stream", myname);
    }

    /*
     * Remember the direction. If this is the first PUT operation for this
     * stream or if the buffer is smaller than the requested size, allocate a
     * new buffer; obviously there is no data to be flushed yet. Otherwise,
     * flush the buffer.
     * 
     * XXX Subtle code to set the preferred buffer size as late as possible.
     */
    if (stream->req_bufsize == 0)
	stream->req_bufsize = VSTREAM_BUFSIZE;
    if (bp->len < stream->req_bufsize) {
	vstream_buf_alloc(bp, stream->req_bufsize);
    } else if (bp->cnt <= 0) {
	if (VSTREAM_FFLUSH_SOME(stream))
	    return (VSTREAM_EOF);
    }
    return (0);
}

/* vstream_buf_space - reserve space ahead of time */

static int vstream_buf_space(VBUF *bp, ssize_t want)
{
    VSTREAM *stream = VBUF_TO_APPL(bp, VSTREAM, buf);
    ssize_t used;
    ssize_t incr;
    ssize_t shortage;
    const char *myname = "vstream_buf_space";

    /*
     * Sanity checks. Reserving space implies writing. It is illegal to write
     * to a read-only stream. Detect a change of I/O direction or position.
     * If so, reset the buffer to the beginning.
     */
    if (bp->put_ready == 0)
	msg_panic("%s: read-only stream", myname);
    switch (bp->flags & (VSTREAM_FLAG_READ | VSTREAM_FLAG_WRITE)) {
    case VSTREAM_FLAG_READ:			/* change direction */
	bp->flags &= ~VSTREAM_FLAG_READ;
	if (bp->flags & VSTREAM_FLAG_DOUBLE)
	    VSTREAM_SAVE_STATE(stream, read_buf, read_fd);
	/* FALLTHROUGH */
    case 0:					/* change position */
	bp->flags |= VSTREAM_FLAG_WRITE;
	if (bp->flags & VSTREAM_FLAG_DOUBLE)
	    VSTREAM_RESTORE_STATE(stream, write_buf, write_fd);
	else
	    VSTREAM_BUF_AT_START(bp);
	/* FALLTHROUGH */
    case VSTREAM_FLAG_WRITE:			/* no change */
	break;
    case VSTREAM_FLAG_READ | VSTREAM_FLAG_WRITE:
	msg_panic("%s: read/write stream", myname);
    }

    /*
     * See if enough space is available. If not, flush a multiple of
     * VSTREAM_BUFSIZE bytes and resize the buffer to a multiple of
     * VSTREAM_BUFSIZE. We flush multiples of VSTREAM_BUFSIZE in an attempt
     * to keep file updates block-aligned for better performance.
     * 
     * XXX Subtle code to set the preferred buffer size as late as possible.
     */
#define VSTREAM_TRUNCATE(count, base)	(((count) / (base)) * (base))
#define VSTREAM_ROUNDUP(count, base)	VSTREAM_TRUNCATE(count + base - 1, base)

    if (stream->req_bufsize == 0)
	stream->req_bufsize = VSTREAM_BUFSIZE;
    if (want > bp->cnt) {
	if ((used = bp->len - bp->cnt) > stream->req_bufsize)
	    if (vstream_fflush_some(stream, VSTREAM_TRUNCATE(used, stream->req_bufsize)))
		return (VSTREAM_EOF);
	if ((shortage = (want - bp->cnt)) > 0) {
	    if ((bp->flags & VSTREAM_FLAG_FIXED)
		|| shortage > __MAXINT__(ssize_t) -bp->len - stream->req_bufsize) {
		bp->flags |= VSTREAM_FLAG_WR_ERR;
	    } else {
		incr = VSTREAM_ROUNDUP(shortage, stream->req_bufsize);
		vstream_buf_alloc(bp, bp->len + incr);
	    }
	}
    }
    return (vstream_ferror(stream) ? VSTREAM_EOF : 0);	/* mmap() may fail */
}

/* vstream_fpurge - discard unread or unwritten content */

int     vstream_fpurge(VSTREAM *stream, int direction)
{
    const char *myname = "vstream_fpurge";
    VBUF   *bp = &stream->buf;

#define VSTREAM_MAYBE_PURGE_WRITE(d, b) if ((d) & VSTREAM_PURGE_WRITE) \
	VSTREAM_BUF_AT_START((b))
#define VSTREAM_MAYBE_PURGE_READ(d, b) if ((d) & VSTREAM_PURGE_READ) \
	VSTREAM_BUF_AT_END((b))

    /*
     * To discard all unread contents, position the read buffer at its end,
     * so that we skip over any unread data, and so that the next read
     * operation will refill the buffer.
     * 
     * To discard all unwritten content, position the write buffer at its
     * beginning, so that the next write operation clobbers any unwritten
     * data.
     */
    switch (bp->flags & (VSTREAM_FLAG_READ_DOUBLE | VSTREAM_FLAG_WRITE)) {
    case VSTREAM_FLAG_READ_DOUBLE:
	VSTREAM_MAYBE_PURGE_WRITE(direction, &stream->write_buf);
	/* FALLTHROUGH */
    case VSTREAM_FLAG_READ:
	VSTREAM_MAYBE_PURGE_READ(direction, bp);
	break;
    case VSTREAM_FLAG_DOUBLE:
	VSTREAM_MAYBE_PURGE_WRITE(direction, &stream->write_buf);
	VSTREAM_MAYBE_PURGE_READ(direction, &stream->read_buf);
	break;
    case VSTREAM_FLAG_WRITE_DOUBLE:
	VSTREAM_MAYBE_PURGE_READ(direction, &stream->read_buf);
	/* FALLTHROUGH */
    case VSTREAM_FLAG_WRITE:
	VSTREAM_MAYBE_PURGE_WRITE(direction, bp);
	break;
    case VSTREAM_FLAG_READ_DOUBLE | VSTREAM_FLAG_WRITE:
    case VSTREAM_FLAG_READ | VSTREAM_FLAG_WRITE:
	msg_panic("%s: read/write stream", myname);
    }

    /*
     * Invalidate the cached file seek position.
     */
    bp->flags &= ~VSTREAM_FLAG_SEEK;
    stream->offset = 0;

    return (0);
}

/* vstream_fseek - change I/O position */

off_t   vstream_fseek(VSTREAM *stream, off_t offset, int whence)
{
    const char *myname = "vstream_fseek";
    VBUF   *bp = &stream->buf;

    /*
     * Flush any unwritten output. Discard any unread input. Position the
     * buffer at the end, so that the next GET or PUT operation triggers a
     * buffer boundary action.
     */
    switch (bp->flags & (VSTREAM_FLAG_READ | VSTREAM_FLAG_WRITE)) {
    case VSTREAM_FLAG_WRITE:
	if (bp->ptr > bp->data) {
	    if (whence == SEEK_CUR)
		offset += (bp->ptr - bp->data);	/* add unwritten data */
	    else if (whence == SEEK_END)
		bp->flags &= ~VSTREAM_FLAG_SEEK;
	    if (VSTREAM_FFLUSH_SOME(stream))
		return (-1);
	}
	VSTREAM_BUF_AT_END(bp);
	break;
    case VSTREAM_FLAG_READ:
	if (whence == SEEK_CUR)
	    offset += bp->cnt;			/* subtract unread data */
	else if (whence == SEEK_END)
	    bp->flags &= ~VSTREAM_FLAG_SEEK;
	/* FALLTHROUGH */
    case 0:
	VSTREAM_BUF_AT_END(bp);
	break;
    case VSTREAM_FLAG_READ | VSTREAM_FLAG_WRITE:
	msg_panic("%s: read/write stream", myname);
    }

    /*
     * Clear the read/write flags to inform the buffer boundary action
     * routines that we may have changed I/O position.
     */
    bp->flags &= ~(VSTREAM_FLAG_READ | VSTREAM_FLAG_WRITE);

    /*
     * Shave an unnecessary system call.
     */
    if (bp->flags & VSTREAM_FLAG_NSEEK) {
	errno = ESPIPE;
	return (-1);
    }

    /*
     * Update the cached file seek position.
     */
    if ((stream->offset = lseek(stream->fd, offset, whence)) < 0) {
	if (errno == ESPIPE)
	    bp->flags |= VSTREAM_FLAG_NSEEK;
    } else {
	bp->flags |= VSTREAM_FLAG_SEEK;
    }
    bp->flags &= ~VSTREAM_FLAG_EOF;
    return (stream->offset);
}

/* vstream_ftell - return file offset */

off_t   vstream_ftell(VSTREAM *stream)
{
    VBUF   *bp = &stream->buf;

    /*
     * Shave an unnecessary syscall.
     */
    if (bp->flags & VSTREAM_FLAG_NSEEK) {
	errno = ESPIPE;
	return (-1);
    }

    /*
     * Use the cached file offset when available. This is the offset after
     * the last read, write or seek operation.
     */
    if ((bp->flags & VSTREAM_FLAG_SEEK) == 0) {
	if ((stream->offset = lseek(stream->fd, (off_t) 0, SEEK_CUR)) < 0) {
	    bp->flags |= VSTREAM_FLAG_NSEEK;
	    return (-1);
	}
	bp->flags |= VSTREAM_FLAG_SEEK;
    }

    /*
     * If this is a read buffer, subtract the number of unread bytes from the
     * cached offset. Remember that read counts are negative.
     */
    if (bp->flags & VSTREAM_FLAG_READ)
	return (stream->offset + bp->cnt);

    /*
     * If this is a write buffer, add the number of unwritten bytes to the
     * cached offset.
     */
    if (bp->flags & VSTREAM_FLAG_WRITE)
	return (stream->offset + (bp->ptr - bp->data));

    /*
     * Apparently, this is a new buffer, or a buffer after seek, so there is
     * no need to account for unread or unwritten data.
     */
    return (stream->offset);
}

/* vstream_fdopen - add buffering to pre-opened stream */

VSTREAM *vstream_fdopen(int fd, int flags)
{
    VSTREAM *stream;

    /*
     * Sanity check.
     */
    if (fd < 0)
	msg_panic("vstream_fdopen: bad file %d", fd);

    /*
     * Initialize buffers etc. but do as little as possible. Late buffer
     * allocation etc. gives the application a chance to override default
     * policies. Either this, or the vstream*open() routines would have to
     * have a really ugly interface with lots of mostly-unused arguments (can
     * you say VMS?).
     */
    stream = (VSTREAM *) mymalloc(sizeof(*stream));
    stream->fd = fd;
    stream->read_fn = VSTREAM_CAN_READ(flags) ? (VSTREAM_RW_FN) timed_read : 0;
    stream->write_fn = VSTREAM_CAN_WRITE(flags) ? (VSTREAM_RW_FN) timed_write : 0;
    vstream_buf_init(&stream->buf, flags);
    stream->offset = 0;
    stream->path = 0;
    stream->pid = 0;
    stream->waitpid_fn = 0;
    stream->timeout = 0;
    stream->context = 0;
    stream->jbuf = 0;
    stream->iotime.tv_sec = stream->iotime.tv_usec = 0;
    stream->time_limit.tv_sec = stream->time_limit.tv_usec = 0;
    stream->req_bufsize = 0;
    return (stream);
}

/* vstream_fopen - open buffered file stream */

VSTREAM *vstream_fopen(const char *path, int flags, mode_t mode)
{
    VSTREAM *stream;
    int     fd;

    if ((fd = open(path, flags, mode)) < 0) {
	return (0);
    } else {
	stream = vstream_fdopen(fd, flags);
	stream->path = mystrdup(path);
	return (stream);
    }
}

/* vstream_fflush - flush write buffer */

int     vstream_fflush(VSTREAM *stream)
{
    if ((stream->buf.flags & VSTREAM_FLAG_READ_DOUBLE)
	== VSTREAM_FLAG_READ_DOUBLE
	&& stream->write_buf.len > stream->write_buf.cnt)
	vstream_fflush_delayed(stream);
    return (VSTREAM_FFLUSH_SOME(stream));
}

/* vstream_fclose - close buffered stream */

int     vstream_fclose(VSTREAM *stream)
{
    int     err;

    /*
     * NOTE: Negative file descriptors are not part of the external
     * interface. They are for internal use only, in order to support
     * vstream_fdclose() without a lot of code duplication. Applications that
     * rely on negative VSTREAM file descriptors will break without warning.
     */
    if (stream->pid != 0)
	msg_panic("vstream_fclose: stream has process");
    if ((stream->buf.flags & VSTREAM_FLAG_WRITE_DOUBLE) != 0 && stream->fd >= 0)
	vstream_fflush(stream);
    /* Do not remove: vstream_fdclose() depends on this error test. */
    err = vstream_ferror(stream);
    if (stream->buf.flags & VSTREAM_FLAG_DOUBLE) {
	if (stream->read_fd >= 0)
	    err |= close(stream->read_fd);
	if (stream->write_fd != stream->read_fd)
	    if (stream->write_fd >= 0)
		err |= close(stream->write_fd);
	vstream_buf_wipe(&stream->read_buf);
	vstream_buf_wipe(&stream->write_buf);
	stream->buf = stream->read_buf;
    } else {
	if (stream->fd >= 0)
	    err |= close(stream->fd);
	vstream_buf_wipe(&stream->buf);
    }
    if (stream->path)
	myfree(stream->path);
    if (stream->jbuf)
	myfree((void *) stream->jbuf);
    if (!VSTREAM_STATIC(stream))
	myfree((void *) stream);
    return (err ? VSTREAM_EOF : 0);
}

/* vstream_fdclose - close stream, leave file(s) open */

int     vstream_fdclose(VSTREAM *stream)
{

    /*
     * Flush unwritten output, just like vstream_fclose(). Errors are
     * reported by vstream_fclose().
     */
    if ((stream->buf.flags & VSTREAM_FLAG_WRITE_DOUBLE) != 0)
	(void) vstream_fflush(stream);

    /*
     * NOTE: Negative file descriptors are not part of the external
     * interface. They are for internal use only, in order to support
     * vstream_fdclose() without a lot of code duplication. Applications that
     * rely on negative VSTREAM file descriptors will break without warning.
     */
    if (stream->buf.flags & VSTREAM_FLAG_DOUBLE) {
	stream->fd = stream->read_fd = stream->write_fd = -1;
    } else {
	stream->fd = -1;
    }
    return (vstream_fclose(stream));
}

/* vstream_printf - formatted print to stdout */

VSTREAM *vstream_printf(const char *fmt,...)
{
    VSTREAM *stream = VSTREAM_OUT;
    va_list ap;

    va_start(ap, fmt);
    vbuf_print(&stream->buf, fmt, ap);
    va_end(ap);
    return (stream);
}

/* vstream_fprintf - formatted print to buffered stream */

VSTREAM *vstream_fprintf(VSTREAM *stream, const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    vbuf_print(&stream->buf, fmt, ap);
    va_end(ap);
    return (stream);
}

/* vstream_fputs - write string to stream */

int     vstream_fputs(const char *str, VSTREAM *stream)
{
    int     ch;

    while ((ch = *str++) != 0)
	if (VSTREAM_PUTC(ch, stream) == VSTREAM_EOF)
	    return (VSTREAM_EOF);
    return (0);
}

/* vstream_control - fine control */

void    vstream_control(VSTREAM *stream, int name,...)
{
    const char *myname = "vstream_control";
    va_list ap;
    int     floor;
    int     old_fd;
    ssize_t req_bufsize = 0;
    VSTREAM *stream2;

#define SWAP(type,a,b) do { type temp = (a); (a) = (b); (b) = (temp); } while (0)

    for (va_start(ap, name); name != VSTREAM_CTL_END; name = va_arg(ap, int)) {
	switch (name) {
	case VSTREAM_CTL_READ_FN:
	    stream->read_fn = va_arg(ap, VSTREAM_RW_FN);
	    break;
	case VSTREAM_CTL_WRITE_FN:
	    stream->write_fn = va_arg(ap, VSTREAM_RW_FN);
	    break;
	case VSTREAM_CTL_CONTEXT:
	    stream->context = va_arg(ap, void *);
	    break;
	case VSTREAM_CTL_PATH:
	    if (stream->path)
		myfree(stream->path);
	    stream->path = mystrdup(va_arg(ap, char *));
	    break;
	case VSTREAM_CTL_DOUBLE:
	    if ((stream->buf.flags & VSTREAM_FLAG_DOUBLE) == 0) {
		stream->buf.flags |= VSTREAM_FLAG_DOUBLE;
		if (stream->buf.flags & VSTREAM_FLAG_READ) {
		    VSTREAM_SAVE_STATE(stream, read_buf, read_fd);
		    VSTREAM_FORK_STATE(stream, write_buf, write_fd);
		} else {
		    VSTREAM_SAVE_STATE(stream, write_buf, write_fd);
		    VSTREAM_FORK_STATE(stream, read_buf, read_fd);
		}
	    }
	    break;
	case VSTREAM_CTL_READ_FD:
	    if ((stream->buf.flags & VSTREAM_FLAG_DOUBLE) == 0)
		msg_panic("VSTREAM_CTL_READ_FD requires double buffering");
	    stream->read_fd = va_arg(ap, int);
	    stream->buf.flags |= VSTREAM_FLAG_NSEEK;
	    break;
	case VSTREAM_CTL_WRITE_FD:
	    if ((stream->buf.flags & VSTREAM_FLAG_DOUBLE) == 0)
		msg_panic("VSTREAM_CTL_WRITE_FD requires double buffering");
	    stream->write_fd = va_arg(ap, int);
	    stream->buf.flags |= VSTREAM_FLAG_NSEEK;
	    break;
	case VSTREAM_CTL_SWAP_FD:
	    stream2 = va_arg(ap, VSTREAM *);
	    if ((stream->buf.flags & VSTREAM_FLAG_DOUBLE)
		!= (stream2->buf.flags & VSTREAM_FLAG_DOUBLE))
		msg_panic("VSTREAM_CTL_SWAP_FD can't swap descriptors between "
			  "single-buffered and double-buffered streams");
	    if (stream->buf.flags & VSTREAM_FLAG_DOUBLE) {
		SWAP(int, stream->read_fd, stream2->read_fd);
		SWAP(int, stream->write_fd, stream2->write_fd);
		stream->fd = ((stream->buf.flags & VSTREAM_FLAG_WRITE) ?
			      stream->write_fd : stream->read_fd);
	    } else {
		SWAP(int, stream->fd, stream2->fd);
	    }
	    break;
	case VSTREAM_CTL_TIMEOUT:
	    if (stream->timeout == 0)
		GETTIMEOFDAY(&stream->iotime);
	    stream->timeout = va_arg(ap, int);
	    if (stream->timeout < 0)
		msg_panic("%s: bad timeout %d", myname, stream->timeout);
	    break;
	case VSTREAM_CTL_EXCEPT:
	    if (stream->jbuf == 0)
		stream->jbuf =
		    (VSTREAM_JMP_BUF *) mymalloc(sizeof(VSTREAM_JMP_BUF));
	    break;

#ifdef VSTREAM_CTL_DUPFD

#define VSTREAM_TRY_DUPFD(backup, fd, floor) do { \
	if (((backup) = (fd)) < floor) { \
	    if (((fd) = fcntl((backup), F_DUPFD, (floor))) < 0) \
		msg_fatal("fcntl F_DUPFD %d: %m", (floor)); \
	    (void) close(backup); \
	} \
    } while (0)

	case VSTREAM_CTL_DUPFD:
	    floor = va_arg(ap, int);
	    if (stream->buf.flags & VSTREAM_FLAG_DOUBLE) {
		VSTREAM_TRY_DUPFD(old_fd, stream->read_fd, floor);
		if (stream->write_fd == old_fd)
		    stream->write_fd = stream->read_fd;
		else
		    VSTREAM_TRY_DUPFD(old_fd, stream->write_fd, floor);
		stream->fd = (stream->buf.flags & VSTREAM_FLAG_READ) ?
		    stream->read_fd : stream->write_fd;
	    } else {
		VSTREAM_TRY_DUPFD(old_fd, stream->fd, floor);
	    }
	    break;
#endif

	    /*
	     * Postpone memory (re)allocation until the space is needed.
	     */
	case VSTREAM_CTL_BUFSIZE:
	    req_bufsize = va_arg(ap, ssize_t);
	    /* Heuristic to detect missing (ssize_t) type cast on LP64 hosts. */
	    if (req_bufsize < 0 || req_bufsize > INT_MAX)
		msg_panic("unreasonable VSTREAM_CTL_BUFSIZE request: %ld",
			  (long) req_bufsize);
	    if ((stream->buf.flags & VSTREAM_FLAG_FIXED) == 0
		&& req_bufsize > stream->req_bufsize) {
		if (msg_verbose)
		    msg_info("fd=%d: stream buffer size old=%ld new=%ld",
			     vstream_fileno(stream),
			     (long) stream->req_bufsize,
			     (long) req_bufsize);
		stream->req_bufsize = req_bufsize;
	    }
	    break;

	    /*
	     * Make no gettimeofday() etc. system call until we really know
	     * that we need to do I/O. This avoids a performance hit when
	     * sending or receiving body content one line at a time.
	     */
	case VSTREAM_CTL_STOP_DEADLINE:
	    stream->buf.flags &= ~VSTREAM_FLAG_DEADLINE;
	    break;
	case VSTREAM_CTL_START_DEADLINE:
	    if (stream->timeout <= 0)
		msg_panic("%s: bad timeout %d", myname, stream->timeout);
	    stream->buf.flags |= VSTREAM_FLAG_DEADLINE;
	    stream->time_limit.tv_sec = stream->timeout;
	    stream->time_limit.tv_usec = 0;
	    break;
	default:
	    msg_panic("%s: bad name %d", myname, name);
	}
    }
    va_end(ap);
}

/* vstream_vprintf - formatted print to stdout */

VSTREAM *vstream_vprintf(const char *format, va_list ap)
{
    VSTREAM *vp = VSTREAM_OUT;

    vbuf_print(&vp->buf, format, ap);
    return (vp);
}

/* vstream_vfprintf - formatted print engine */

VSTREAM *vstream_vfprintf(VSTREAM *vp, const char *format, va_list ap)
{
    vbuf_print(&vp->buf, format, ap);
    return (vp);
}

/* vstream_bufstat - get stream buffer status */

ssize_t vstream_bufstat(VSTREAM *vp, int command)
{
    VBUF   *bp;

    switch (command & VSTREAM_BST_MASK_DIR) {
    case VSTREAM_BST_FLAG_IN:
	if (vp->buf.flags & VSTREAM_FLAG_READ) {
	    bp = &vp->buf;
	} else if (vp->buf.flags & VSTREAM_FLAG_DOUBLE) {
	    bp = &vp->read_buf;
	} else {
	    bp = 0;
	}
	switch (command & ~VSTREAM_BST_MASK_DIR) {
	case VSTREAM_BST_FLAG_PEND:
	    return (bp ? -bp->cnt : 0);
	    /* Add other requests below. */
	}
	break;
    case VSTREAM_BST_FLAG_OUT:
	if (vp->buf.flags & VSTREAM_FLAG_WRITE) {
	    bp = &vp->buf;
	} else if (vp->buf.flags & VSTREAM_FLAG_DOUBLE) {
	    bp = &vp->write_buf;
	} else {
	    bp = 0;
	}
	switch (command & ~VSTREAM_BST_MASK_DIR) {
	case VSTREAM_BST_FLAG_PEND:
	    return (bp ? bp->len - bp->cnt : 0);
	    /* Add other requests below. */
	}
	break;
    }
    msg_panic("vstream_bufstat: unknown command: %d", command);
}

#undef vstream_peek			/* API binary compatibility. */

/* vstream_peek - peek at a stream */

ssize_t vstream_peek(VSTREAM *vp)
{
    if (vp->buf.flags & VSTREAM_FLAG_READ) {
	return (-vp->buf.cnt);
    } else if (vp->buf.flags & VSTREAM_FLAG_DOUBLE) {
	return (-vp->read_buf.cnt);
    } else {
	return (0);
    }
}

/* vstream_peek_data - peek at unread data */

const char *vstream_peek_data(VSTREAM *vp)
{
    if (vp->buf.flags & VSTREAM_FLAG_READ) {
	return ((const char *) vp->buf.ptr);
    } else if (vp->buf.flags & VSTREAM_FLAG_DOUBLE) {
	return ((const char *) vp->read_buf.ptr);
    } else {
	return (0);
    }
}

#ifdef TEST

static void copy_line(ssize_t bufsize)
{
    int     c;

    vstream_control(VSTREAM_IN, VSTREAM_CTL_BUFSIZE(bufsize), VSTREAM_CTL_END);
    vstream_control(VSTREAM_OUT, VSTREAM_CTL_BUFSIZE(bufsize), VSTREAM_CTL_END);
    while ((c = VSTREAM_GETC(VSTREAM_IN)) != VSTREAM_EOF) {
	VSTREAM_PUTC(c, VSTREAM_OUT);
	if (c == '\n')
	    break;
    }
    vstream_fflush(VSTREAM_OUT);
}

static void printf_number(void)
{
    vstream_printf("%d\n", __MAXINT__(int));
    vstream_fflush(VSTREAM_OUT);
}

 /*
  * Exercise some of the features.
  */
int     main(int argc, char **argv)
{

    /*
     * Test buffer expansion and shrinking. Formatted print may silently
     * expand the write buffer and cause multiple bytes to be written.
     */
    copy_line(1);			/* one-byte read/write */
    copy_line(2);				/* two-byte read/write */
    copy_line(1);				/* two-byte read/write */
    printf_number();				/* multi-byte write */
    exit(0);
}

#endif
