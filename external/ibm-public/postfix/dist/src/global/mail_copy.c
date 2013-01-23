/*	$NetBSD: mail_copy.c,v 1.1.1.2.6.1 2013/01/23 00:05:02 yamt Exp $	*/

/*++
/* NAME
/*	mail_copy 3
/* SUMMARY
/*	copy message with extreme prejudice
/* SYNOPSIS
/*	#include <mail_copy.h>
/*
/*	int	mail_copy(sender, orig_to, delivered, src, dst, flags, eol, why)
/*	const char *sender;
/*	const char *orig_to;
/*	const char *delivered;
/*	VSTREAM	*src;
/*	VSTREAM	*dst;
/*	int	flags;
/*	const char *eol;
/*	DSN_BUF	*why;
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
/*	Prepend a UNIX-style From_ line to the message.
/* .IP MAIL_COPY_BLANK
/*	Append an empty line to the end of the message.
/* .IP MAIL_COPY_DELIVERED
/*	Prepend a Delivered-To: header with the name of the
/*	\fIdelivered\fR attribute.
/*	The address is quoted according to RFC822 rules.
/* .IP MAIL_COPY_ORIG_RCPT
/*	Prepend an X-Original-To: header with the original
/*	envelope recipient address.
/* .IP MAIL_COPY_RETURN_PATH
/*	Prepend a Return-Path: header with the value of the
/*	\fIsender\fR attribute.
/* .RE
/*	The manifest constant MAIL_COPY_MBOX is a convenient shorthand for
/*	all MAIL_COPY_XXX options that are appropriate for mailbox delivery.
/*	Use MAIL_COPY_NONE to copy a message without any options enabled.
/* .IP eol
/*	Record delimiter, for example, LF or CF LF.
/* .IP why
/*	A null pointer, or storage for the reason of failure in
/*	the form of a DSN detail code plus free text.
/* DIAGNOSTICS
/*	A non-zero result means the operation failed. Warnings: corrupt
/*	message file. A corrupt message is marked as corrupt.
/*
/*	The result is the bit-wise OR of zero or more of the following:
/* .IP MAIL_COPY_STAT_CORRUPT
/*	The queue file is marked as corrupt.
/* .IP MAIL_COPY_STAT_READ
/*	A read error was detected; errno specifies the nature of the problem.
/* .IP MAIL_COPY_STAT_WRITE
/*	A write error was detected; errno specifies the nature of the problem.
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
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <htable.h>
#include <vstream.h>
#include <vstring.h>
#include <vstring_vstream.h>
#include <stringops.h>
#include <iostuff.h>
#include <warn_stat.h>

/* Global library. */

#include "quote_822_local.h"
#include "record.h"
#include "rec_type.h"
#include "mail_queue.h"
#include "mail_addr.h"
#include "mark_corrupt.h"
#include "mail_params.h"
#include "mail_copy.h"
#include "mbox_open.h"
#include "dsn_buf.h"
#include "sys_exits.h"

/* mail_copy - copy message with extreme prejudice */

int     mail_copy(const char *sender,
		          const char *orig_rcpt,
		          const char *delivered,
		          VSTREAM *src, VSTREAM *dst,
		          int flags, const char *eol, DSN_BUF *why)
{
    const char *myname = "mail_copy";
    VSTRING *buf;
    char   *bp;
    off_t   orig_length;
    int     read_error;
    int     write_error;
    int     corrupt_error = 0;
    time_t  now;
    int     type;
    int     prev_type;
    struct stat st;
    off_t   size_limit;

    /*
     * Workaround 20090114. This will hopefully get someone's attention. The
     * problem with file_size_limit < message_size_limit is that mail will be
     * delivered again and again until someone removes it from the queue by
     * hand, because Postfix cannot mark a recipient record as "completed".
     */
    if (fstat(vstream_fileno(src), &st) < 0)
	msg_fatal("fstat: %m");
    if ((size_limit = get_file_limit()) < st.st_size)
	msg_panic("file size limit %lu < message size %lu. This "
		  "causes large messages to be delivered repeatedly "
		  "after they were submitted with \"sendmail -t\" "
		  "or after recipients were added with the Milter "
		  "SMFIR_ADDRCPT request",
		  (unsigned long) size_limit,
		  (unsigned long) st.st_size);

    /*
     * Initialize.
     */
#ifndef NO_TRUNCATE
    if ((flags & MAIL_COPY_TOFILE) != 0)
	if ((orig_length = vstream_fseek(dst, (off_t) 0, SEEK_END)) < 0)
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
	    vstream_fprintf(dst, "From %s  %.24s%s", *sender == 0 ?
			    MAIL_ADDR_MAIL_DAEMON : vstring_str(buf),
			    asctime(localtime(&now)), eol);
	}
	if (flags & MAIL_COPY_RETURN_PATH) {
	    vstream_fprintf(dst, "Return-Path: <%s>%s",
			    *sender ? vstring_str(buf) : "", eol);
	}
    }
    if (flags & MAIL_COPY_ORIG_RCPT) {
	if (orig_rcpt == 0)
	    msg_panic("%s: null orig_rcpt", myname);

	/*
	 * An empty original recipient record almost certainly means that
	 * original recipient processing was disabled.
	 */
	if (*orig_rcpt) {
	    quote_822_local(buf, orig_rcpt);
	    vstream_fprintf(dst, "X-Original-To: %s%s", vstring_str(buf), eol);
	}
    }
    if (flags & MAIL_COPY_DELIVERED) {
	if (delivered == 0)
	    msg_panic("%s: null delivered", myname);
	quote_822_local(buf, delivered);
	vstream_fprintf(dst, "Delivered-To: %s%s", vstring_str(buf), eol);
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
	if (type == REC_TYPE_NORM && vstream_fputs(eol, dst) == VSTREAM_EOF)
	    break;
	prev_type = type;
    }
    if (vstream_ferror(dst) == 0) {
	if (var_fault_inj_code == 1)
	    type = 0;
	if (type != REC_TYPE_XTRA) {
	    /* XXX Where is the queue ID? */
	    msg_warn("bad record type: %d in message content", type);
	    corrupt_error = mark_corrupt(src);
	}
	if (prev_type != REC_TYPE_NORM)
	    vstream_fputs(eol, dst);
	if (flags & MAIL_COPY_BLANK)
	    vstream_fputs(eol, dst);
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
    if (var_fault_inj_code == 2) {
	read_error = 1;
	errno = ENOENT;
    }
    if (var_fault_inj_code == 3) {
	write_error = 1;
	errno = ENOENT;
    }
#ifndef NO_TRUNCATE
    if ((flags & MAIL_COPY_TOFILE) != 0)
	if (corrupt_error || read_error || write_error)
	    /* Complain about ignored "undo" errors? So sue me. */
	    (void) ftruncate(vstream_fileno(dst), orig_length);
#endif
    write_error |= vstream_fclose(dst);

    /*
     * Return the optional verbose error description.
     */
#define TRY_AGAIN_ERROR(errno) \
	(errno == EAGAIN || errno == ESTALE)

    if (why && read_error)
	dsb_unix(why, TRY_AGAIN_ERROR(errno) ? "4.3.0" : "5.3.0",
		 sys_exits_detail(EX_IOERR)->text,
		 "error reading message: %m");
    if (why && write_error)
	dsb_unix(why, mbox_dsn(errno, "5.3.0"),
		 sys_exits_detail(EX_IOERR)->text,
		 "error writing message: %m");

    /*
     * Use flag+errno description when the optional verbose description is
     * not desired.
     */
    return ((corrupt_error ? MAIL_COPY_STAT_CORRUPT : 0)
	    | (read_error ? MAIL_COPY_STAT_READ : 0)
	    | (write_error ? MAIL_COPY_STAT_WRITE : 0));
}
