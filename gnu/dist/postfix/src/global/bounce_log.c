/*++
/* NAME
/*	bounce_log 3
/* SUMMARY
/*	bounce file API
/* SYNOPSIS
/*	#include <bounce_log.h>
/*
/*	typedef struct {
/* .in +4
/*	    /* No public members. */
/* .in -4
/*	} BOUNCE_LOG;
/*
/*	BOUNCE_LOG *bounce_log_open(queue, id, flags, mode)
/*	const char *queue;
/*	const char *id;
/*	int	flags;
/*	mode_t	mode;
/*
/*	BOUNCE_LOG *bounce_log_read(bp, rcpt, dsn)
/*	BOUNCE_LOG *bp;
/*	RCPT_BUF *rcpt;
/*	DSN_BUF *dsn;
/*
/*	void	bounce_log_rewind(bp)
/*	BOUNCE_LOG *bp;
/*
/*	void	bounce_log_close(bp)
/*	BOUNCE_LOG *bp;
/* DESCRIPTION
/*	This module implements a bounce/defer logfile API. Information
/*	is sanitized for control and non-ASCII characters. Fields not
/*	present in input are represented by empty strings.
/*
/*	bounce_log_open() opens the named bounce or defer logfile
/*	and returns a handle that must be used for further access.
/*	The result is a null pointer if the file cannot be opened.
/*	The caller is expected to inspect the errno code and deal
/*	with the problem.
/*
/*	bounce_log_read() reads the next record from the bounce or defer
/*	logfile (skipping over and warning about malformed data)
/*	and breaks out the recipient address, the recipient status
/*	and the text that explains why the recipient was undeliverable.
/*	bounce_log_read() returns a null pointer when no recipient was read,
/*	otherwise it returns its argument.
/*
/*	bounce_log_rewind() is a helper that seeks to the first recipient
/*	in an open bounce or defer logfile (skipping over recipients that
/*	are marked as done). The result is 0 in case of success, -1 in case
/*	of problems.
/*
/*	bounce_log_close() closes an open bounce or defer logfile and
/*	releases memory for the specified handle. The result is non-zero
/*	in case of I/O errors.
/*
/*	Arguments:
/* .IP queue
/*	The bounce or defer queue name.
/* .IP id
/*	The message queue id of bounce or defer logfile. This
/*	file has the same name as the original message file.
/* .IP flags
/*	File open flags, as with open(2).
/* .IP mode
/*	File permissions, as with open(2).
/* .IP rcpt
/*	Recipient buffer. The RECIPIENT member is updated.
/* .IP dsn
/*	Delivery status information. The DSN member is updated.
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
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstream.h>
#include <vstring.h>
#include <vstring_vstream.h>
#include <stringops.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>
#include <mail_queue.h>
#include <dsn_mask.h>
#include <bounce_log.h>

/* Application-specific. */

#define STR(x)		vstring_str(x)

/* bounce_log_open - open bounce read stream */

BOUNCE_LOG *bounce_log_open(const char *queue_name, const char *queue_id,
			            int flags, mode_t mode)
{
    BOUNCE_LOG *bp;
    VSTREAM *fp;

#define STREQ(x,y)	(strcmp((x),(y)) == 0)

    /*
     * Logfiles may contain a mixture of old-style (<recipient>: text) and
     * new-style entries with multiple attributes per recipient.
     * 
     * Kluge up default DSN status and action for old-style logfiles.
     */
    if ((fp = mail_queue_open(queue_name, queue_id, flags, mode)) == 0) {
	return (0);
    } else {
	bp = (BOUNCE_LOG *) mymalloc(sizeof(*bp));
	bp->fp = fp;
	bp->buf = vstring_alloc(100);
	if (STREQ(queue_name, MAIL_QUEUE_DEFER)) {
	    bp->compat_status = mystrdup("4.0.0");
	    bp->compat_action = mystrdup("delayed");
	} else {
	    bp->compat_status = mystrdup("5.0.0");
	    bp->compat_action = mystrdup("failed");
	}
	return (bp);
    }
}

/* bounce_log_read - read one record from bounce log file */

BOUNCE_LOG *bounce_log_read(BOUNCE_LOG *bp, RCPT_BUF *rcpt_buf,
			            DSN_BUF *dsn_buf)
{
    char   *recipient;
    char   *text;
    char   *cp;
    int     state;

    /*
     * Our trivial logfile parser state machine.
     */
#define START	0				/* still searching */
#define FOUND	1				/* in logfile entry */

    /*
     * Initialize.
     */
    state = START;
    rcpb_reset(rcpt_buf);
    dsb_reset(dsn_buf);

    /*
     * Support mixed logfile formats to make migration easier. The same file
     * can start with old-style records and end with new-style records. With
     * backwards compatibility, we even have old format followed by new
     * format within the same logfile entry!
     */
    for (;;) {
	if ((vstring_get_nonl(bp->buf, bp->fp) == VSTREAM_EOF))
	    return (0);

	/*
	 * Logfile entries are separated by blank lines. Even the old ad-hoc
	 * logfile format has a blank line after the last record. This means
	 * we can safely use blank lines to detect the start and end of
	 * logfile entries.
	 */
	if (STR(bp->buf)[0] == 0) {
	    if (state == FOUND)
		break;
	    state = START;
	    continue;
	}

	/*
	 * Sanitize. XXX This needs to be done more carefully with new-style
	 * logfile entries.
	 */
	cp = printable(STR(bp->buf), '?');

	if (state == START)
	    state = FOUND;

	/*
	 * New style logfile entries are in "name = value" format.
	 */
	if (ISALNUM(*cp)) {
	    const char *err;
	    char   *name;
	    char   *value;
	    long    offset;
	    int     notify;

	    /*
	     * Split into name and value.
	     */
	    if ((err = split_nameval(cp, &name, &value)) != 0) {
		msg_warn("%s: malformed record: %s", VSTREAM_PATH(bp->fp), err);
		continue;
	    }

	    /*
	     * Save attribute value.
	     */
	    if (STREQ(name, MAIL_ATTR_RECIP)) {
		vstring_strcpy(rcpt_buf->address, *value ?
			       value : "(MAILER-DAEMON)");
	    } else if (STREQ(name, MAIL_ATTR_ORCPT)) {
		vstring_strcpy(rcpt_buf->orig_addr, *value ?
			       value : "(MAILER-DAEMON)");
	    } else if (STREQ(name, MAIL_ATTR_DSN_ORCPT)) {
		vstring_strcpy(rcpt_buf->dsn_orcpt, value);
	    } else if (STREQ(name, MAIL_ATTR_DSN_NOTIFY)) {
		if ((notify = atoi(value)) > 0 && DSN_NOTIFY_OK(notify))
		    rcpt_buf->dsn_notify = notify;
	    } else if (STREQ(name, MAIL_ATTR_OFFSET)) {
		if ((offset = atol(value)) > 0)
		    rcpt_buf->offset = offset;
	    } else if (STREQ(name, MAIL_ATTR_DSN_STATUS)) {
		vstring_strcpy(dsn_buf->status, value);
	    } else if (STREQ(name, MAIL_ATTR_DSN_ACTION)) {
		vstring_strcpy(dsn_buf->action, value);
	    } else if (STREQ(name, MAIL_ATTR_DSN_DTYPE)) {
		vstring_strcpy(dsn_buf->dtype, value);
	    } else if (STREQ(name, MAIL_ATTR_DSN_DTEXT)) {
		vstring_strcpy(dsn_buf->dtext, value);
	    } else if (STREQ(name, MAIL_ATTR_DSN_MTYPE)) {
		vstring_strcpy(dsn_buf->mtype, value);
	    } else if (STREQ(name, MAIL_ATTR_DSN_MNAME)) {
		vstring_strcpy(dsn_buf->mname, value);
	    } else if (STREQ(name, MAIL_ATTR_WHY)) {
		vstring_strcpy(dsn_buf->reason, value);
	    } else {
		msg_warn("%s: unknown attribute name: %s, ignored",
			 VSTREAM_PATH(bp->fp), name);
	    }
	    continue;
	}

	/*
	 * Old-style logfile record. Find the recipient address.
	 */
	if (*cp != '<') {
	    msg_warn("%s: malformed record: %.30s...",
		     VSTREAM_PATH(bp->fp), cp);
	    continue;
	}
	recipient = cp + 1;
	if ((cp = strstr(recipient, ">: ")) == 0) {
	    msg_warn("%s: malformed record: %.30s...",
		     VSTREAM_PATH(bp->fp), cp);
	    continue;
	}
	*cp = 0;
	vstring_strcpy(rcpt_buf->address, *recipient ?
		       recipient : "(MAILER-DAEMON)");

	/*
	 * Find the text that explains why mail was not deliverable.
	 */
	text = cp + 2;
	while (*text && ISSPACE(*text))
	    text++;
	vstring_strcpy(dsn_buf->reason, text);
    }

    /*
     * Specify place holders for missing fields. See also DSN_FROM_DSN_BUF()
     * and RECIPIENT_FROM_RCPT_BUF() for null and non-null fields.
     */
#define BUF_NODATA(buf)		(STR(buf)[0] == 0)
#define BUF_ASSIGN(buf, text)	vstring_strcpy((buf), (text))

    if (BUF_NODATA(rcpt_buf->address))
	BUF_ASSIGN(rcpt_buf->address, "(recipient address unavailable)");
    if (BUF_NODATA(dsn_buf->status))
	BUF_ASSIGN(dsn_buf->status, bp->compat_status);
    if (BUF_NODATA(dsn_buf->action))
	BUF_ASSIGN(dsn_buf->action, bp->compat_action);
    if (BUF_NODATA(dsn_buf->reason))
	BUF_ASSIGN(dsn_buf->reason, "(description unavailable)");
    (void) RECIPIENT_FROM_RCPT_BUF(rcpt_buf);
    (void) DSN_FROM_DSN_BUF(dsn_buf);
    return (bp);
}

/* bounce_log_close - close bounce reader stream */

int     bounce_log_close(BOUNCE_LOG *bp)
{
    int     ret;

    ret = vstream_fclose(bp->fp);
    vstring_free(bp->buf);
    myfree(bp->compat_status);
    myfree(bp->compat_action);
    myfree((char *) bp);

    return (ret);
}
