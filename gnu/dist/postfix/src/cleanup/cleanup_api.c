/*++
/* NAME
/*	cleanup_api 3
/* SUMMARY
/*	cleanup callable interface, message processing
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	CLEANUP_STATE *cleanup_open()
/*
/*	void	cleanup_control(state, flags)
/*	CLEANUP_STATE *state;
/*	int	flags;
/*
/*	void	CLEANUP_RECORD(state, type, buf, len)
/*	CLEANUP_STATE *state;
/*	int	type;
/*	char	*buf;
/*	int	len;
/*
/*	int	cleanup_close(state)
/*	CLEANUP_STATE *state;
/* DESCRIPTION
/*	This module implements a callable interface to the cleanup service
/*	for processing one message and for writing it to queue file.
/*	For a description of the cleanup service, see cleanup(8).
/*
/*	cleanup_open() creates a new queue file and performs other
/*	per-message initialization. The result is a handle that should be
/*	given to the cleanup_control(), cleanup_record() and cleanup_close()
/*	routines. The name of the queue file is in the queue_id result
/*	structure member.
/*
/*	cleanup_control() processes per-message flags specified by the caller.
/*	These flags control the handling of data errors, and must be set
/*	before processing the first message record.
/*
/*	CLEANUP_RECORD() is a macro that processes one message record,
/*	that copies the result to the queue file, and that maintains a
/*	little state machine. The last record in a valid message has type
/*	REC_TYPE_END.  In order to find out if a message is corrupted,
/*	the caller is encouraged to test the CLEANUP_OUT_OK(state) macro.
/*	The result is false when further message processing is futile.
/*	In that case, it is safe to call cleanup_close() immediately.
/*
/*	cleanup_close() finishes a queue file. In case of any errors,
/*	the file is removed. The result value is non-zero in case of
/*	problems. Use cleanup_strerror() to translate the result into
/*	human_readable text.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* SEE ALSO
/*	cleanup(8) cleanup service description.
/*	cleanup_init(8) cleanup callable interface, initialization
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
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <mymalloc.h>

/* Global library. */

#include <cleanup_user.h>
#include <mail_queue.h>
#include <mail_proto.h>
#include <bounce.h>
#include <mail_params.h>
#include <mail_stream.h>

/* Application-specific. */

#include "cleanup.h"

/* cleanup_open - open queue file and initialize */

CLEANUP_STATE *cleanup_open(void)
{
    CLEANUP_STATE *state;
    static char *log_queues[] = {
	MAIL_QUEUE_DEFER,
	MAIL_QUEUE_BOUNCE,
	0,
    };
    char  **cpp;

    /*
     * Initialize private state.
     */
    state = cleanup_state_alloc();

    /*
     * Open the queue file. Save the queue file name in a global variable, so
     * that the runtime error handler can clean up in case of problems.
     */
    state->handle = mail_stream_file(MAIL_QUEUE_INCOMING,
				     MAIL_CLASS_PUBLIC, MAIL_SERVICE_QUEUE);
    state->dst = state->handle->stream;
    cleanup_path = mystrdup(VSTREAM_PATH(state->dst));
    state->queue_id = mystrdup(state->handle->id);
    if (msg_verbose)
	msg_info("cleanup_open: open %s", cleanup_path);

    /*
     * If there is a time to get rid of spurious bounce/defer log files, this
     * is it. The down side is that this costs performance for every message,
     * while the probability of spurious bounce/defer log files is quite low.
     * Perhaps we should put the queue file ID inside the defer and bounce
     * files, so that the bounce and defer daemons can figure out if a file
     * is a left-over from a previous message instance. For now, we play safe
     * and check each time a new queue file is created.
     */
    for (cpp = log_queues; *cpp; cpp++) {
	if (mail_queue_remove(*cpp, state->queue_id) == 0)
	    msg_warn("%s: removed spurious %s log", *cpp, state->queue_id);
	else if (errno != ENOENT)
	    msg_fatal("%s: remove %s log: %m", *cpp, state->queue_id);
    }
    return (state);
}

/* cleanup_control - process client options */

void    cleanup_control(CLEANUP_STATE *state, int flags)
{

    /*
     * If the client requests us to do the bouncing in case of problems,
     * throw away the input only in case of real show-stopper errors, such as
     * unrecognizable data (which should never happen) or insufficient space
     * for the queue file (which will happen occasionally). Otherwise,
     * discard input after any lethal error. See the CLEANUP_OUT_OK() macro
     * definition.
     */
    if ((state->flags = flags) & CLEANUP_FLAG_BOUNCE) {
	state->err_mask =
	(CLEANUP_STAT_BAD | CLEANUP_STAT_WRITE | CLEANUP_STAT_SIZE);
    } else {
	state->err_mask = CLEANUP_STAT_LETHAL;
    }
}

/* cleanup_close - finish queue file */

int     cleanup_close(CLEANUP_STATE *state)
{
    char   *junk;
    int     status;

    /*
     * See if there are any errors. For example, the message is incomplete,
     * or it needs to be bounced for lack of recipients. We want to turn on
     * the execute bits on a file only when we really want the queue manager
     * to process it.
     */
    if (CLEANUP_OUT_OK(state)) {
	if (state->recip == 0)
	    state->errs |= CLEANUP_STAT_RCPT;
	if (state->end_seen == 0)
	    state->errs |= CLEANUP_STAT_BAD;
    }

    /*
     * If there are no errors, be very picky about queue file write errors
     * because we are about to tell the sender that it can throw away its
     * copy of the message.
     */
    if ((state->errs & CLEANUP_STAT_LETHAL) == 0)
	state->errs |= mail_stream_finish(state->handle);
    else
	mail_stream_cleanup(state->handle);
    state->handle = 0;
    state->dst = 0;

    /*
     * If there was an error, remove the queue file, after optionally
     * bouncing it. An incomplete message should never be bounced: it was
     * canceled by the client, and may not even have an address to bounce to.
     * That last test is redundant but we keep it just for robustness.
     * 
     * If we are responsible for bouncing a message, we must must report success
     * to the client unless the bounce message file could not be written
     * (which is just as bad as not being able to write the message queue
     * file in the first place).
     * 
     * Do not log the arrival of a message that will be bounced by the client.
     * 
     * XXX CLEANUP_STAT_LETHAL masks errors that are not directly fatal (e.g.,
     * header buffer overflow is normally allowed to happen), but that can
     * indirectly become a problem (e.g., no recipients were extracted from
     * message headers because we could not process all the message headers).
     * However, cleanup_strerror() prioritizes errors so that it can report
     * the cause (e.g., header buffer overflow), which is more useful.
     * 
     * XXX When bouncing, should log sender because qmgr won't be able to.
     */
#define CAN_BOUNCE() \
	((state->errs & (CLEANUP_STAT_BAD | CLEANUP_STAT_WRITE)) == 0 \
	    && state->sender != 0 \
	    && (state->flags & CLEANUP_FLAG_BOUNCE) != 0)

    if (state->errs & CLEANUP_STAT_LETHAL) {
	if (CAN_BOUNCE()) {
	    if (bounce_append(BOUNCE_FLAG_CLEAN, state->queue_id,
			      state->recip ? state->recip : "unknown",
			      "cleanup", state->time,
			      "Message processing aborted: %s",
			      cleanup_strerror(state->errs)) == 0
		&& bounce_flush(BOUNCE_FLAG_CLEAN, MAIL_QUEUE_INCOMING,
				state->queue_id, state->sender) == 0) {
		state->errs = 0;
	    } else {
		msg_warn("%s: bounce message failure", state->queue_id);
		state->errs = CLEANUP_STAT_WRITE;
	    }
	}
	if (REMOVE(cleanup_path))
	    msg_warn("remove %s: %m", cleanup_path);
    }

    /*
     * Make sure that our queue file will not be deleted by the error handler
     * AFTER we have taken responsibility for delivery. Better to deliver
     * twice than to lose mail.
     */
    junk = cleanup_path;
    cleanup_path = 0;				/* don't delete upon error */
    myfree(junk);

    /*
     * Cleanup internal state. This is simply complementary to the
     * initializations at the beginning of cleanup_open().
     */
    if (msg_verbose)
	msg_info("cleanup_close: status %d", state->errs);
    status = state->errs & CLEANUP_STAT_LETHAL;
    cleanup_state_free(state);
    return (status);
}
