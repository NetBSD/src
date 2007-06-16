/*	$NetBSD: mail_stream.c,v 1.1.1.8.4.1 2007/06/16 16:59:59 snj Exp $	*/

/*++
/* NAME
/*	mail_stream 3
/* SUMMARY
/*	mail stream management
/* SYNOPSIS
/*	#include <mail_stream.h>
/*
/*	typedef struct {
/* .in +4
/*		VSTREAM	*stream;	/* read/write stream */
/*		char	*id;		/* queue ID */
/*		struct timeval ctime;	/* create time */
/*		private members...
/* .in -4
/*	} MAIL_STREAM;
/*
/*	MAIL_STREAM *mail_stream_file(queue, class, service, mode)
/*	const char *queue;
/*	const char *class;
/*	const char *service;
/*	int	mode;
/*
/*	MAIL_STREAM *mail_stream_service(class, service)
/*	const char *class;
/*	const char *service;
/*
/*	MAIL_STREAM *mail_stream_command(command)
/*	const char *command;
/*
/*	void	mail_stream_cleanup(info)
/*	MAIL_STREAM *info;
/*
/*	int	mail_stream_finish(info, why)
/*	MAIL_STREAM *info;
/*	VSTRING	*why;
/*
/*	void	mail_stream_ctl(info, op, ...)
/*	MAIL_STREAM *info;
/*	int	op;
/* DESCRIPTION
/*	This module provides a generic interface to Postfix queue file
/*	format messages to file, to Postfix server, or to external command.
/*	The routines that open a stream return a handle with an initialized
/*	stream and queue id member. The handle is either given to a cleanup
/*	routine, to dispose of a failed request, or to a finish routine, to
/*	complete the request.
/*
/*	mail_stream_file() opens a mail stream to a newly-created file and
/*	arranges for trigger delivery at finish time. This call never fails.
/*	But it may take forever. The mode argument specifies additional
/*	file permissions that will be OR-ed in when the file is finished.
/*	While embryonic files have mode 0600, finished files have mode 0700.
/*
/*	mail_stream_command() opens a mail stream to external command,
/*	and receives queue ID information from the command. The result
/*	is a null pointer when the initial handshake fails. The command
/*	is given to the shell only when necessary. At finish time, the
/*	command is expected to send a completion status.
/*
/*	mail_stream_service() opens a mail stream to Postfix service,
/*	and receives queue ID information from the command. The result
/*	is a null pointer when the initial handshake fails. At finish
/*	time, the daemon is expected to send a completion status.
/*
/*	mail_stream_cleanup() cancels the operation that was started with
/*	any of the mail_stream_xxx() routines, and destroys the argument.
/*	It is up to the caller to remove incomplete file objects.
/*
/*	mail_stream_finish() completes the operation that was started with
/*	any of the mail_stream_xxx() routines, and destroys the argument.
/*	The result is any of the status codes defined in <cleanup_user.h>.
/*	It is up to the caller to remove incomplete file objects.
/*	The why argument can be a null pointer.
/*
/*	mail_stream_ctl() selectively overrides information that
/*	was specified with mail_stream_file(); none of the attributes
/*	are applicable for other mail stream types.  The arguments
/*	are a list of (operation, value) pairs, terminated with
/*	MAIL_STREAM_CTL_END.  The following lists the operation
/*	codes and the types of the corresponding value arguments.
/* .IP "MAIL_STREAM_CTL_QUEUE (char *)"
/*	The argument specifies an alternate destination queue. The
/*	queue file is moved to the specified queue before the call
/*	returns. Failure to rename the queue file results in a fatal
/*	error.
/* .IP "MAIL_STREAM_CTL_CLASS (char *)"
/*	The argument specifies an alternate trigger class.
/* .IP "MAIL_STREAM_CTL_SERVICE (char *)"
/*	The argument specifies an alternate trigger service.
/* .IP "MAIL_STREAM_CTL_MODE (int)"
/*	The argument specifies alternate permissions that override
/*	the permissions specified with mail_stream_file().
/* .IP "MAIL_STREAM_CTL_DELAY (int)"
/*	Attempt to postpone initial delivery by advancing the queue
/*	file modification time stamp by this amount.  This has
/*	effect only within the deferred mail queue.
/*	This feature may have no effect with remote file systems.
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
#include <unistd.h>
#include <errno.h>
#include <utime.h>
#include <string.h>
#include <stdarg.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>
#include <stringops.h>
#include <argv.h>
#include <sane_fsops.h>

/* Global library. */

#include <cleanup_user.h>
#include <mail_proto.h>
#include <mail_queue.h>
#include <opened.h>
#include <mail_params.h>
#include <mail_stream.h>

/* Application-specific. */

static VSTRING *id_buf;

#define FREE_AND_WIPE(free, arg) do { if (arg) free(arg); arg = 0; } while (0)

#define STR(x)	vstring_str(x)

/* mail_stream_cleanup - clean up after success or failure */

void    mail_stream_cleanup(MAIL_STREAM *info)
{
    FREE_AND_WIPE(info->close, info->stream);
    FREE_AND_WIPE(myfree, info->queue);
    FREE_AND_WIPE(myfree, info->id);
    FREE_AND_WIPE(myfree, info->class);
    FREE_AND_WIPE(myfree, info->service);
    myfree((char *) info);
}

#if defined(HAS_FUTIMES_AT)
#define CAN_STAMP_BY_STREAM

/* stamp_stream - update open file [am]time stamp */

static int stamp_stream(VSTREAM *fp, time_t when)
{
    struct timeval tv;

    if (when != 0) {
	tv.tv_sec = when;
	tv.tv_usec = 0;
	return (futimesat(vstream_fileno(fp), (char *) 0, &tv));
    } else {
	return (futimesat(vstream_fileno(fp), (char *) 0, (struct timeval *) 0));
    }
}

#elif defined(HAS_FUTIMES)
#define CAN_STAMP_BY_STREAM

/* stamp_stream - update open file [am]time stamp */

static int stamp_stream(VSTREAM *fp, time_t when)
{
    struct timeval tv;

    if (when != 0) {
	tv.tv_sec = when;
	tv.tv_usec = 0;
	return (futimes(vstream_fileno(fp), &tv));
    } else {
	return (futimes(vstream_fileno(fp), (struct timeval *) 0));
    }
}

#endif

/* stamp_path - update file [am]time stamp by pathname */

static int stamp_path(const char *path, time_t when)
{
    struct utimbuf tbuf;

    if (when != 0) {
	tbuf.actime = tbuf.modtime = when;
	return (utime(path, &tbuf));
    } else {
	return (utime(path, (struct utimbuf *) 0));
    }
}

/* mail_stream_finish_file - finish file mail stream */

static int mail_stream_finish_file(MAIL_STREAM *info, VSTRING *unused_why)
{
    int     status = CLEANUP_STAT_OK;
    static char wakeup[] = {TRIGGER_REQ_WAKEUP};
    struct stat st;
    char   *path_to_reset = 0;
    static int incoming_fs_clock_ok = 0;
    static int incoming_clock_warned = 0;
    int     check_incoming_fs_clock;
    int     err;
    time_t  want_stamp;
    time_t  expect_stamp;

    /*
     * Make sure the message makes it to file. Set the execute bit when no
     * write error was detected. Some people believe that this code has a
     * problem if the system crashes before fsync() returns; fchmod() could
     * take effect before all the data blocks are written. Wietse claims that
     * this is not a problem. Postfix rejects incomplete queue files, even
     * when the +x attribute is set. Every Postfix queue file record has a
     * type code and a length field. Files with missing records are rejected,
     * as are files with unknown record type codes. Every Postfix queue file
     * must end with an explicit END record. Postfix queue files without END
     * record are discarded.
     * 
     * Attempt to detect file system clocks that are ahead of local time, but
     * don't check the file system clock all the time. The effect of file
     * system clock drift can be difficult to understand (Postfix ignores new
     * mail until the local clock catches up with the file mtime stamp).
     * 
     * This clock drift detection code may not work with file systems that work
     * on a local copy of the file and that update the server only after the
     * file is closed.
     * 
     * Optionally set a cooldown time.
     * 
     * XXX: We assume that utime() does control the file modification time even
     * when followed by an fchmod(), fsync(), close() sequence. This may fail
     * with remote file systems when fsync() actually updates the file. Even
     * then, we still delay the average message by 1/2 of the
     * queue_run_delay.
     * 
     * XXX: Victor does not like running utime() after the close(), since this
     * creates a race even with local filesystems. But Wietse is not
     * confident that utime() before fsync() and close() will work reliably
     * with remote file systems.
     * 
     * XXX Don't run the clock skew tests with Postfix sendmail submissions.
     * Don't whine against unsuspecting users or applications.
     */
    check_incoming_fs_clock =
	(!incoming_fs_clock_ok && !strcmp(info->queue, MAIL_QUEUE_INCOMING));

#ifdef DELAY_ACTION
    if (strcmp(info->queue, MAIL_QUEUE_DEFERRED) != 0)
	info->delay = 0;
    if (info->delay > 0)
	want_stamp = time((time_t *) 0) + info->delay;
    else
#endif
	want_stamp = 0;

    /*
     * If we can cheaply set the file time stamp (no pathname lookup) do it
     * anyway, so that we can avoid whining later about file server/client
     * clock skew.
     * 
     * Otherwise, if we must set the file time stamp for delayed delivery, use
     * whatever means we have to get the job done, no matter if it is
     * expensive.
     * 
     * XXX Unfortunately, Linux futimes() is not usable because it uses /proc.
     * This may not be available because of chroot, or because of access
     * restrictions after a process changes privileges.
     */
    if (vstream_fflush(info->stream)
#ifdef CAN_STAMP_BY_STREAM
	|| stamp_stream(info->stream, want_stamp)
#else
	|| (want_stamp && stamp_path(VSTREAM_PATH(info->stream), want_stamp))
#endif
	|| fchmod(vstream_fileno(info->stream), 0700 | info->mode)
#ifdef HAS_FSYNC
	|| fsync(vstream_fileno(info->stream))
#endif
	|| (check_incoming_fs_clock
	    && fstat(vstream_fileno(info->stream), &st) < 0)
	)
	status = (errno == EFBIG ? CLEANUP_STAT_SIZE : CLEANUP_STAT_WRITE);
#ifdef TEST
    st.st_mtime += 10;
#endif

    /*
     * Work around file system clock skew. If the file system clock is ahead
     * of the local clock, Postfix won't deliver mail immediately, which is
     * bad for performance. If the file system clock falls behind the local
     * clock, it just looks silly in mail headers.
     */
    if (status == CLEANUP_STAT_OK && check_incoming_fs_clock) {
	/* Do NOT use time() result from before fsync(). */
	expect_stamp = want_stamp ? want_stamp : time((time_t *) 0);
	if (st.st_mtime > expect_stamp) {
	    path_to_reset = mystrdup(VSTREAM_PATH(info->stream));
	    if (incoming_clock_warned == 0) {
		msg_warn("file system clock is %d seconds ahead of local clock",
			 (int) (st.st_mtime - expect_stamp));
		msg_warn("resetting file time stamps - this hurts performance");
		incoming_clock_warned = 1;
	    }
	} else {
	    if (st.st_mtime < expect_stamp - 100)
		msg_warn("file system clock is %d seconds behind local clock",
			 (int) (expect_stamp - st.st_mtime));
	    incoming_fs_clock_ok = 1;
	}
    }

    /*
     * Close the queue file and mark it as closed. Be prepared for
     * vstream_fclose() to fail even after vstream_fflush() and fsync()
     * reported no error. Reason: after a file is closed, some networked file
     * systems copy the file out to another machine. Running the queue on a
     * remote file system is not recommended, if only for performance
     * reasons.
     */
    err = info->close(info->stream);
    info->stream = 0;
    if (status == CLEANUP_STAT_OK && err != 0)
	status = (errno == EFBIG ? CLEANUP_STAT_SIZE : CLEANUP_STAT_WRITE);

    /*
     * Work around file system clocks that are ahead of local time.
     */
    if (path_to_reset != 0) {
	if (status == CLEANUP_STAT_OK) {
	    if (stamp_path(path_to_reset, expect_stamp) < 0 && errno != ENOENT)
		msg_fatal("%s: update file time stamps: %m", info->id);
	}
	myfree(path_to_reset);
    }

    /*
     * When all is well, notify the next service that a new message has been
     * queued.
     */
    if (status == CLEANUP_STAT_OK && info->class && info->service)
	mail_trigger(info->class, info->service, wakeup, sizeof(wakeup));

    /*
     * Cleanup.
     */
    mail_stream_cleanup(info);
    return (status);
}

/* mail_stream_finish_ipc - finish IPC mail stream */

static int mail_stream_finish_ipc(MAIL_STREAM *info, VSTRING *why)
{
    int     status = CLEANUP_STAT_WRITE;

    /*
     * Receive the peer's completion status.
     */
    if ((why && attr_scan(info->stream, ATTR_FLAG_STRICT,
			  ATTR_TYPE_INT, MAIL_ATTR_STATUS, &status,
			  ATTR_TYPE_STR, MAIL_ATTR_WHY, why,
			  ATTR_TYPE_END) != 2)
	|| (!why && attr_scan(info->stream, ATTR_FLAG_MISSING,
			      ATTR_TYPE_INT, MAIL_ATTR_STATUS, &status,
			      ATTR_TYPE_END) != 1))
	status = CLEANUP_STAT_WRITE;

    /*
     * Cleanup.
     */
    mail_stream_cleanup(info);
    return (status);
}

/* mail_stream_finish - finish action */

int     mail_stream_finish(MAIL_STREAM *info, VSTRING *why)
{
    return (info->finish(info, why));
}

/* mail_stream_file - destination is file */

MAIL_STREAM *mail_stream_file(const char *queue, const char *class,
			              const char *service, int mode)
{
    struct timeval tv;
    MAIL_STREAM *info;
    VSTREAM *stream;

    stream = mail_queue_enter(queue, 0600 | mode, &tv);
    if (msg_verbose)
	msg_info("open %s", VSTREAM_PATH(stream));

    info = (MAIL_STREAM *) mymalloc(sizeof(*info));
    info->stream = stream;
    info->finish = mail_stream_finish_file;
    info->close = vstream_fclose;
    info->queue = mystrdup(queue);
    info->id = mystrdup(basename(VSTREAM_PATH(stream)));
    info->class = mystrdup(class);
    info->service = mystrdup(service);
    info->mode = mode;
#ifdef DELAY_ACTION
    info->delay = 0;
#endif
    info->ctime = tv;
    return (info);
}

/* mail_stream_service - destination is service */

MAIL_STREAM *mail_stream_service(const char *class, const char *name)
{
    VSTREAM *stream;
    MAIL_STREAM *info;

    if (id_buf == 0)
	id_buf = vstring_alloc(10);

    stream = mail_connect_wait(class, name);
    if (attr_scan(stream, ATTR_FLAG_MISSING,
		  ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id_buf, 0) != 1) {
	vstream_fclose(stream);
	return (0);
    } else {
	info = (MAIL_STREAM *) mymalloc(sizeof(*info));
	info->stream = stream;
	info->finish = mail_stream_finish_ipc;
	info->close = vstream_fclose;
	info->queue = 0;
	info->id = mystrdup(vstring_str(id_buf));
	info->class = 0;
	info->service = 0;
	return (info);
    }
}

/* mail_stream_command - destination is command */

MAIL_STREAM *mail_stream_command(const char *command)
{
    VSTREAM *stream;
    MAIL_STREAM *info;
    ARGV   *export_env;
    int     status;

    if (id_buf == 0)
	id_buf = vstring_alloc(10);

    /*
     * Treat fork() failure as a transient problem. Treat bad handshake as a
     * permanent error.
     * 
     * XXX Are we invoking a Postfix process or a non-Postfix process? In the
     * former case we can share the full environment; in the latter case only
     * a restricted environment should be propagated. Even though we are
     * talking a Postfix-internal protocol there is no way we can tell what
     * is being executed except by duplicating a lot of existing code.
     */
    export_env = argv_split(var_export_environ, ", \t\r\n");
    while ((stream = vstream_popen(O_RDWR,
				   VSTREAM_POPEN_COMMAND, command,
				   VSTREAM_POPEN_EXPORT, export_env->argv,
				   VSTREAM_POPEN_END)) == 0) {
	msg_warn("fork: %m");
	sleep(10);
    }
    argv_free(export_env);
    vstream_control(stream,
		    VSTREAM_CTL_PATH, command,
		    VSTREAM_CTL_END);

    if (attr_scan(stream, ATTR_FLAG_MISSING,
		  ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id_buf, 0) != 1) {
	if ((status = vstream_pclose(stream)) != 0)
	    msg_warn("command \"%s\" exited with status %d", command, status);
	return (0);
    } else {
	info = (MAIL_STREAM *) mymalloc(sizeof(*info));
	info->stream = stream;
	info->finish = mail_stream_finish_ipc;
	info->close = vstream_pclose;
	info->queue = 0;
	info->id = mystrdup(vstring_str(id_buf));
	info->class = 0;
	info->service = 0;
	return (info);
    }
}

/* mail_stream_ctl - update file-based mail stream properties */

void    mail_stream_ctl(MAIL_STREAM *info, int op,...)
{
    const char *myname = "mail_stream_ctl";
    va_list ap;
    char   *new_queue = 0;
    char   *string_value;

    /*
     * Sanity check. None of the attributes below are applicable unless the
     * target is a file-based stream.
     */
    if (info->finish != mail_stream_finish_file)
	msg_panic("%s: attempt to update non-file stream %s",
		  myname, info->id);

    for (va_start(ap, op); op != MAIL_STREAM_CTL_END; op = va_arg(ap, int)) {

	switch (op) {

	    /*
	     * Change the queue directory. We do this at the end of this
	     * call.
	     */
	case MAIL_STREAM_CTL_QUEUE:
	    if ((new_queue = va_arg(ap, char *)) == 0)
		msg_panic("%s: NULL queue",
			  myname);
	    break;

	    /*
	     * Change the service that needs to be notified.
	     */
	case MAIL_STREAM_CTL_CLASS:
	    FREE_AND_WIPE(myfree, info->class);
	    if ((string_value = va_arg(ap, char *)) != 0)
		info->class = mystrdup(string_value);
	    break;

	case MAIL_STREAM_CTL_SERVICE:
	    FREE_AND_WIPE(myfree, info->service);
	    if ((string_value = va_arg(ap, char *)) != 0)
		info->service = mystrdup(string_value);
	    break;

	    /*
	     * Change the (finished) file access mode.
	     */
	case MAIL_STREAM_CTL_MODE:
	    info->mode = va_arg(ap, int);
	    break;

	    /*
	     * Advance the (finished) file modification time.
	     */
#ifdef DELAY_ACTION
	case MAIL_STREAM_CTL_DELAY:
	    if ((info->delay = va_arg(ap, int)) < 0)
		msg_panic("%s: bad delay time %d", myname, info->delay);
	    break;
#endif

	default:
	    msg_panic("%s: bad op code %d", myname, op);
	}
    }
    va_end(ap);

    /*
     * Rename the queue file after allocating memory for new information, so
     * that the caller can still remove an embryonic file when memory
     * allocation fails (there is no risk of deleting the wrong file).
     * 
     * Wietse opposed the idea to update run-time error handler information
     * here, because this module wasn't designed to defend against internal
     * concurrency issues with error handlers that attempt to follow dangling
     * pointers.
     * 
     * This code duplicates mail_queue_rename(), except that we need the new
     * path to update the stream pathname.
     */
    if (new_queue != 0 && strcmp(info->queue, new_queue) != 0) {
	char   *saved_queue = info->queue;
	char   *saved_path = mystrdup(VSTREAM_PATH(info->stream));
	VSTRING *new_path = vstring_alloc(100);

	(void) mail_queue_path(new_path, new_queue, info->id);
	info->queue = mystrdup(new_queue);
	vstream_control(info->stream, VSTREAM_CTL_PATH, STR(new_path),
			VSTREAM_CTL_END);

	if (sane_rename(saved_path, STR(new_path)) == 0
	    || (mail_queue_mkdirs(STR(new_path)) == 0
		&& sane_rename(saved_path, STR(new_path)) == 0)) {
	    if (msg_verbose)
		msg_info("%s: placed in %s queue", info->id, info->queue);
	} else {
	    msg_fatal("%s: move to %s queue failed: %m", info->id,
		      info->queue);
	}

	myfree(saved_path);
	myfree(saved_queue);
	vstring_free(new_path);
    }
}
