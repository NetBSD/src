/*++
/* NAME
/*	bounce_append_service 3
/* SUMMARY
/*	append record to bounce log, server side
/* SYNOPSIS
/*	#include "bounce_service.h"
/*
/*	int     bounce_append_service(queue_id, recipient, why)
/*	char	*queue_id;
/*	char	*recipient;
/*	char	*why;
/* DESCRIPTION
/*	This module implements the server side of the bounce_append()
/*	(append bounce log) request. This routine either succeeds or
/*	it raises a fatal error.
/* DIAGNOSTICS
/*	Fatal errors: all file access errors; memory allocation errors.
/* BUGS
/* SEE ALSO
/*	bounce(3) basic bounce service client interface
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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <stringops.h>

/* Global library. */

#include <mail_queue.h>
#include <quote_822_local.h>
#include <deliver_flock.h>

/* Application-specific. */

#include "bounce_service.h"

/* bounce_append_service - append bounce log */

int     bounce_append_service(char *service, char *queue_id,
			              char *recipient, char *why)
{
    VSTRING *in_buf = vstring_alloc(100);
    VSTRING *out_buf = vstring_alloc(100);
    VSTREAM *log;
    long    orig_length;

    /*
     * This code is paranoid for a good reason. Once the bounce service takes
     * responsibility, the mail system will make no further attempts to
     * deliver this recipient. Whenever file access fails, assume that the
     * system is under stress or that something has been mis-configured, and
     * force a backoff by raising a fatal run-time error.
     */
    log = mail_queue_open(service, queue_id,
			  O_WRONLY | O_APPEND | O_CREAT, 0600);
    if (log == 0)
	msg_fatal("open file %s %s: %m", service, queue_id);

    /*
     * Lock out other processes to avoid truncating someone else's data in
     * case of trouble.
     */
    if (deliver_flock(vstream_fileno(log), INTERNAL_LOCK, (VSTRING *) 0) < 0)
	msg_fatal("lock file %s %s: %m", service, queue_id);

    /*
     * Now, go for it. Append a record. Truncate the log to the original
     * length when the append operation fails. We use the plain stream-lf
     * file format because we do not need anything more complicated. As a
     * benefit, we can still recover some data when the file is a little
     * garbled.
     * 
     * XXX addresses in defer logfiles are in printable quoted form, while
     * addresses in message envelope records are in raw unquoted form. This
     * may change once we replace the present ad-hoc bounce/defer logfile
     * format by one that is transparent for control etc. characters. See
     * also: showq/showq.c.
     */
    if ((orig_length = vstream_fseek(log, 0L, SEEK_END)) < 0)
	msg_fatal("seek file %s %s: %m", service, queue_id);

    if (*recipient)
	vstream_fprintf(log, "<%s>: ",
	   printable(vstring_str(quote_822_local(in_buf, recipient)), '?'));
    else
	vstream_fprintf(log, "<>: ");
    vstream_fputs(printable(why, '?'), log);
    vstream_fputs("\n\n", log);

    if (vstream_fflush(log) != 0 || fsync(vstream_fileno(log)) < 0) {
#ifndef NO_TRUNCATE
	if (ftruncate(vstream_fileno(log), (off_t) orig_length) < 0)
	    msg_fatal("truncate file %s %s: %m", service, queue_id);
#endif
	msg_fatal("append file %s %s: %m", service, queue_id);
    }

    /*
     * Darn. If closing the log detects a problem, the only way to undo the
     * damage is to open the log once more, and to truncate the log to the
     * original length. But, this could happen only when the log is kept on a
     * remote file system, and that is not recommended practice anyway.
     */
    if (vstream_fclose(log) != 0)
	msg_warn("append file %s %s: %m", service, queue_id);

    vstring_free(in_buf);
    vstring_free(out_buf);
    return (0);
}
