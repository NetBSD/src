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
/*	    /* public members... */
/*	    const char *recipient;
/*	    const char *orig_rcpt;
/*	    long 	rcpt_offset;
/*	    const char *dsn_status;
/*	    const char *dsn_action;
/*	    const char *text;
/* .in -4
/*	} BOUNCE_LOG;
/*
/*	BOUNCE_LOG *bounce_log_open(queue, id, flags, mode)
/*	const char *queue;
/*	const char *id;
/*	int	flags;
/*	int	mode;
/*
/*	BOUNCE_LOG *bounce_log_read(bp)
/*	BOUNCE_LOG *bp;
/*
/*	void	bounce_log_rewind(bp)
/*	BOUNCE_LOG *bp;
/*
/*	BOUNCE_LOG *bounce_log_forge(orig_rcpt, recipient, rcpt_offset,
/*					dsn_status, dsn_action, why)
/*	const char *orig_rcpt;
/*	const char *recipient;
/*	long	rcpt_offset;
/*	const char *dsn_status;
/*	const char *dsn_action;
/*	const char *why;
/*
/*	void	bounce_log_close(bp)
/*	BOUNCE_LOG *bp;
/* DESCRIPTION
/*	This module implements a bounce/defer logfile API. Information
/*	is sanitized for control and non-ASCII characters.
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
/*	bounce_log_forge() forges one recipient status record
/*	without actually accessing a logfile.
/*	The result cannot be used for any logfile access operation
/*	and must be disposed of by passing it to bounce_log_close().
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
/* .IP more
/*	File permissions, as with open(2).
/* .PP
/*	Results:
/* .IP recipient
/*	The final recipient address in RFC 822 external form, or <>
/*	in case of the null recipient address.
/* .IP orig_rcpt
/*      Null pointer or the original recipient address in RFC 822
/*	external form.
/* .IP rcpt_offset
/*	Queue file offset of recipient record.
/* .IP text
/*	The text that explains why the recipient was undeliverable.
/* .IP dsn_status
/*	String with DSN compatible status code (digit.digit.digit).
/* .IP dsn_action
/*	"delivered", "failed", "delayed" and so on.
/* .PP
/*	Other fields will be added as the code evolves.
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
#include <bounce_log.h>

/* Application-specific. */

#define STR(x)		vstring_str(x)

/* bounce_log_init - initialize structure */

static BOUNCE_LOG *bounce_log_init(VSTREAM *fp,
				           VSTRING *buf,
				           VSTRING *orcp_buf,
				           VSTRING *rcpt_buf,
				           long rcpt_offset,
				           VSTRING *status_buf,
				           const char *compat_status,
				           VSTRING *action_buf,
				           const char *compat_action,
				           VSTRING *text_buf)
{
    BOUNCE_LOG *bp;

#define SET_BUFFER(bp, buf, str) { \
	bp->buf = buf; \
	bp->str = (buf && STR(buf)[0] ? STR(buf) : 0); \
    }

    bp = (BOUNCE_LOG *) mymalloc(sizeof(*bp));
    bp->fp = fp;
    bp->buf = buf;
    SET_BUFFER(bp, orcp_buf, orig_rcpt);
    SET_BUFFER(bp, rcpt_buf, recipient);
    bp->rcpt_offset = rcpt_offset;
    SET_BUFFER(bp, status_buf, dsn_status);
    bp->compat_status = compat_status;
    SET_BUFFER(bp, action_buf, dsn_action);
    bp->compat_action = compat_action;
    SET_BUFFER(bp, text_buf, text);
    return (bp);
}

/* bounce_log_open - open bounce read stream */

BOUNCE_LOG *bounce_log_open(const char *queue_name, const char *queue_id,
			            int flags, int mode)
{
    VSTREAM *fp;

#define STREQ(x,y)	(strcmp((x),(y)) == 0)
#define SAVE_TO_VSTRING(s)	vstring_strcpy(vstring_alloc(10), (s))

    /*
     * TODO: peek at the first byte to see if this is an old-style log
     * (<recipient>: text) or a new-style extensible log with multiple
     * attributes per recipient. That would not help during a transition from
     * old to new style, where one can expect to find mixed format files.
     * 
     * Kluge up default DSN status and action for old-style logfiles.
     */
    if ((fp = mail_queue_open(queue_name, queue_id, flags, mode)) == 0) {
	return (0);
    } else {
	return (bounce_log_init(fp,		/* stream */
				vstring_alloc(100),	/* buffer */
				vstring_alloc(10),	/* orig_rcpt */
				vstring_alloc(10),	/* recipient */
				(long) 0,	/* offset */
				vstring_alloc(10),	/* dsn_status */
				STREQ(queue_name, MAIL_QUEUE_DEFER) ?
				"4.0.0" : "5.0.0",	/* compatibility */
				vstring_alloc(10),	/* dsn_action */
				STREQ(queue_name, MAIL_QUEUE_DEFER) ?
				"delayed" : "failed",	/* compatibility */
				vstring_alloc(10)));	/* text */
    }
}

/* bounce_log_read - read one record from bounce log file */

BOUNCE_LOG *bounce_log_read(BOUNCE_LOG *bp)
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
    bp->recipient = "(unavailable)";
    bp->orig_rcpt = 0;
    bp->rcpt_offset = 0;
    bp->dsn_status = "(unavailable)";
    bp->dsn_action = "(unavailable)";
    bp->text = "(unavailable)";

    /*
     * Support mixed logfile formats to make transitions easier. The same
     * file can start with old-style records and end with new-style records.
     * With backwards compatibility, we even have old format followed by new
     * format within the same logfile entry!
     */
    while ((vstring_get_nonl(bp->buf, bp->fp) != VSTREAM_EOF)) {

	/*
	 * Logfile entries are separated by blank lines. Even the old ad-hoc
	 * logfile format has a blank line after the last record. This means
	 * we can safely use blank lines to detect the start and end of
	 * logfile entries.
	 */
	if (STR(bp->buf)[0] == 0) {
	    if (state == FOUND)
		return (bp);
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
		bp->recipient = STR(vstring_strcpy(bp->rcpt_buf, *value ?
						value : "(MAILER-DAEMON)"));
	    } else if (STREQ(name, MAIL_ATTR_ORCPT)) {
		bp->orig_rcpt = STR(vstring_strcpy(bp->orcp_buf, *value ?
						value : "(MAILER-DAEMON)"));
	    } else if (STREQ(name, MAIL_ATTR_OFFSET)) {
		bp->rcpt_offset = atol(value);
	    } else if (STREQ(name, MAIL_ATTR_STATUS)) {
		bp->dsn_status = STR(vstring_strcpy(bp->status_buf, value));
	    } else if (STREQ(name, MAIL_ATTR_ACTION)) {
		bp->dsn_action = STR(vstring_strcpy(bp->action_buf, value));
	    } else if (STREQ(name, MAIL_ATTR_WHY)) {
		bp->text = STR(vstring_strcpy(bp->text_buf, value));
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
	vstring_strcpy(bp->rcpt_buf, *recipient ?
		       recipient : "(MAILER-DAEMON)");
	bp->recipient = STR(bp->rcpt_buf);

	/*
	 * Find the text that explains why mail was not deliverable.
	 */
	text = cp + 2;
	while (*text && ISSPACE(*text))
	    text++;
	vstring_strcpy(bp->text_buf, text);
	bp->text = STR(bp->text_buf);

	/*
	 * Add compatibility status and action info, to make up for data that
	 * was not stored in old-style bounce logfiles.
	 */
	bp->dsn_status = bp->compat_status;
	bp->dsn_action = bp->compat_action;
    }
    return (0);
}

/* bounce_log_forge - forge one recipient status record */

BOUNCE_LOG *bounce_log_forge(const char *orig_rcpt, const char *recipient,
			           long rcpt_offset, const char *dsn_status,
			           const char *dsn_action, const char *text)
{
    return (bounce_log_init((VSTREAM *) 0,
			    (VSTRING *) 0,
			    SAVE_TO_VSTRING(orig_rcpt),
			    SAVE_TO_VSTRING(recipient),
			    rcpt_offset,
			    SAVE_TO_VSTRING(dsn_status),
			    "(unavailable)",
			    SAVE_TO_VSTRING(dsn_action),
			    "(unavailable)",
			    SAVE_TO_VSTRING(text)));
}

/* bounce_log_close - close bounce reader stream */

int     bounce_log_close(BOUNCE_LOG *bp)
{
    int     ret;

    if (bp->fp)
	ret = vstream_fclose(bp->fp);
    else
	ret = 0;
    if (bp->buf)
	vstring_free(bp->buf);
    if (bp->rcpt_buf)
	vstring_free(bp->rcpt_buf);
    if (bp->orcp_buf)
	vstring_free(bp->orcp_buf);
    if (bp->status_buf)
	vstring_free(bp->status_buf);
    if (bp->action_buf)
	vstring_free(bp->action_buf);
    if (bp->text_buf)
	vstring_free(bp->text_buf);
    myfree((char *) bp);
    return (ret);
}
