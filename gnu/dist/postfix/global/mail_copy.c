/*++
/* NAME
/*	mail_copy 3
/* SUMMARY
/*	copy message with extreme prejudice
/* SYNOPSIS
/*	#include <mail_copy.h>
/*
/*	int	mail_copy(sender, delivered, src, dst, flags, why)
/*	const char *sender;
/*	const char *delivered;
/*	VSTREAM	*src;
/*	VSTREAM	*dst;
/*	int	flags;
/*	VSTRING	*why;
/* DESCRIPTION
/*	mail_copy() copies a mail message from record stream to stream-lf
/*	stream, and attempts to detect all possible I/O errors.
/*
/*	Arguments:
/* .IP sender
/*	The sender envelope address.
/* .IP delivered
/*	Null pointer or delivered-to: header address.
/* .IP src
/*	The source record stream, positioned at the beginning of the
/*	message contents.
/* .IP dst
/*	The destination byte stream (in stream-lf format). If the message
/*	ends in an incomplete line, a newline character is appended to
/*	the output.
/* .IP flags
/*	The binary OR of zero or more of the following:
/* .RS
/* .IP MAIL_COPY_QUOTE
/*	Prepend a `>' character to lines beginning with `From '.
/* .IP MAIL_COPY_DOT
/*	Prepend a `.' character to lines beginning with `.'.
/* .IP MAIL_COPY_TOFILE
/*	On systems that support this, use fsync() to flush the
/*	data to stable storage, and truncate the destination
/*	file to its original length in case of problems.
/* .IP MAIL_COPY_FROM
/*	Prepend a UNIX-style From_ line to the message, and append
/*	an empty line to the end of the message.
/* .IP MAIL_COPY_DELIVERED
/*	Prepend a Delivered-To: header with the name of the
/*	\fIdelivered\fR attribute.
/* .IP MAIL_COPY_RETURN_PATH
/*	Prepend a Return-Path: header with the value of the
/*	\fIsender\fR attribute.
/* .RE
/*	The manifest constant MAIL_COPY_MBOX is a convenient shorthand for
/*	all MAIL_COPY_XXX options that are appropriate for mailbox delivery.
/*	Use MAIL_COPY_NONE to copy a message without any options enabled.
/* .IP why
/*	A null pointer, or storage for the reason of failure.
/* DIAGNOSTICS
/*	A non-zero result means the operation failed. Warnings: corrupt
/*	message file. A corrupt message is marked as corrupt.
/* SEE ALSO
/*	mark_corrupt(3), mark queue file as corrupted.
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
#include <string.h>
#include <unistd.h>
#include <time.h>

/* Utility library. */

#include <msg.h>
#include <htable.h>
#include <vstream.h>
#include <vstring.h>
#include <vstring_vstream.h>
#include <stringops.h>

/* Global library. */

#include "quote_822_local.h"
#include "record.h"
#include "rec_type.h"
#include "mail_queue.h"
#include "mail_addr.h"
#include "mail_copy.h"
#include "mark_corrupt.h"

/* mail_copy - copy message with extreme prejudice */

int     mail_copy(const char *sender, const char *delivered,
		          VSTREAM *src, VSTREAM *dst,
		          int flags, VSTRING *why)
{
    char   *myname = "mail_copy";
    VSTRING *buf;
    char   *bp;
    long    orig_length;
    int     read_error;
    int     write_error;
    int     corrupt_error = 0;
    time_t  now;
    int     type;
    int     prev_type;

    /*
     * Initialize.
     */
#ifndef NO_TRUNCATE
    if ((flags & MAIL_COPY_TOFILE) != 0)
	if ((orig_length = vstream_fseek(dst, 0L, SEEK_END)) < 0)
	    msg_fatal("seek file %s: %m", VSTREAM_PATH(dst));
#endif
    buf = vstring_alloc(100);

    /*
     * Prepend a bunch of headers to the message.
     */
    if (flags & (MAIL_COPY_FROM | MAIL_COPY_RETURN_PATH)) {
	if (sender == 0)
	    msg_panic("%s: null sender", myname);
	quote_822_local(buf, sender);
	if (flags & MAIL_COPY_FROM) {
	    time(&now);
	    vstream_fprintf(dst, "From %s  %s", *sender == 0 ?
			    MAIL_ADDR_MAIL_DAEMON :
			    vstring_str(buf),
			    asctime(localtime(&now)));
	}
	if (flags & MAIL_COPY_RETURN_PATH) {
	    vstream_fprintf(dst, "Return-Path: <%s>\n",
			    *sender ? vstring_str(buf) : "");
	}
    }
    if (flags & MAIL_COPY_DELIVERED) {
	if (delivered == 0)
	    msg_panic("%s: null delivered", myname);
	quote_822_local(buf, delivered);
	vstream_fprintf(dst, "Delivered-To: %s\n", lowercase(vstring_str(buf)));
    }

    /*
     * Copy the message. Escape lines that could be confused with the ugly
     * From_ line. Make sure that there is a blank line at the end of the
     * message so that the next ugly From_ can be found by mail reading
     * software.
     * 
     * XXX Rely on the front-end services to enforce record size limits.
     */
#define VSTREAM_FWRITE_BUF(s,b) \
	vstream_fwrite((s),vstring_str(b),VSTRING_LEN(b))

    prev_type = REC_TYPE_NORM;
    while ((type = rec_get(src, buf, 0)) > 0) {
	if (type != REC_TYPE_NORM && type != REC_TYPE_CONT)
	    break;
	bp = vstring_str(buf);
	if (prev_type == REC_TYPE_NORM) {
	    if ((flags & MAIL_COPY_QUOTE) && *bp == 'F' && !strncmp(bp, "From ", 5))
		VSTREAM_PUTC('>', dst);
	    if ((flags & MAIL_COPY_DOT) && *bp == '.')
		VSTREAM_PUTC('.', dst);
	}
	if (VSTRING_LEN(buf) && VSTREAM_FWRITE_BUF(dst, buf) != VSTRING_LEN(buf))
	    break;
	if (type == REC_TYPE_NORM && VSTREAM_PUTC('\n', dst) == VSTREAM_EOF)
	    break;
	prev_type = type;
    }
    if (vstream_ferror(dst) == 0) {
	if (type != REC_TYPE_XTRA)
	    corrupt_error = mark_corrupt(src);
	if (prev_type != REC_TYPE_NORM)
	    VSTREAM_PUTC('\n', dst);
	if (flags & MAIL_COPY_FROM)
	    VSTREAM_PUTC('\n', dst);
    }
    vstring_free(buf);

    /*
     * Make sure we read and wrote all. Truncate the file to its original
     * length when the delivery failed. POSIX does not require ftruncate(),
     * so we may have a portability problem. Note that fclose() may fail even
     * while fflush and fsync() succeed. Think of remote file systems such as
     * AFS that copy the file back to the server upon close. Oh well, no
     * point optimizing the error case. XXX On systems that use flock()
     * locking, we must truncate the file file before closing it (and losing
     * the exclusive lock).
     */
    read_error = vstream_ferror(src);
    write_error = vstream_fflush(dst);
#ifdef HAS_FSYNC
    if ((flags & MAIL_COPY_TOFILE) != 0)
	write_error |= fsync(vstream_fileno(dst));
#endif
#ifndef NO_TRUNCATE
    if ((flags & MAIL_COPY_TOFILE) != 0)
	if (read_error || write_error)
	    ftruncate(vstream_fileno(dst), (off_t) orig_length);
#endif
    write_error |= vstream_fclose(dst);
    if (why && read_error)
	vstring_sprintf(why, "error reading message: %m");
    if (why && write_error)
	vstring_sprintf(why, "error writing message: %m");
    return (corrupt_error || read_error || write_error);
}
