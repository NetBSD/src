/*++
/* NAME
/*	postsuper 1
/* SUMMARY
/*	Postfix superintendent
/* SYNOPSIS
/* .fi
/*	\fBpostsuper\fR [\fB-psv\fR] 
/*		[\fB-c \fIconfig_dir\fR] [\fB-d \fIqueue_id\fR]
/*		[\fB-h \fIqueue_id\fR] [\fB-H \fIqueue_id\fR]
/*		[\fB-r \fIqueue_id\fR] [\fIdirectory ...\fR]
/* DESCRIPTION
/*	The \fBpostsuper\fR command does maintenance jobs on the Postfix
/*	queue. Use of the command is restricted to the superuser.
/*	See the \fBpostqueue\fR command for unprivileged queue operations
/*	such as listing or flushing the mail queue.
/*
/*	By default, \fBpostsuper\fR performs the operations requested with the
/*	\fB-s\fR and \fB-p\fR command-line options on all Postfix queue
/*	directories - this includes the \fBincoming\fR, \fBactive\fR and
/*	\fBdeferred\fR directories with mail files and the \fBbounce\fR,
/*	\fBdefer\fR and \fBflush\fR directories with log files.
/*
/*	Options:
/* .IP "\fB-c \fIconfig_dir\fR"
/*	The \fBmain.cf\fR configuration file is in the named directory
/*	instead of the default configuration directory. See also the
/*	MAIL_CONFIG environment setting below.
/* .IP "\fB-d \fIqueue_id\fR"
/*	Delete one message with the named queue ID from the named
/*	mail queue(s) (default: \fBhold\fR, \fBincoming\fR, \fBactive\fR and
/*	\fBdeferred\fR).
/*	If a \fIqueue_id\fR of \fB-\fR is specified, the program reads
/*	queue IDs from standard input. For example, to delete all mail
/*	from or to \fBuser@example.com\fR:
/* .sp
/*	mailq | tail +2 | awk  \'BEGIN { RS = "" } \e
/* .ti +4
/*	/ user@example\e.com$/ { print $1 } \e
/* .br
/*	\' | tr -d '*!' | postsuper -d -
/* .sp
/*	Specify \fB-d ALL\fR to remove all messages; for example, specify
/*	\fB-d ALL deferred\fR to delete mail in the \fBdeferred\fR queue.
/*	As a safety measure, the word \fBALL\fR must be specified in upper
/*	case.
/* .sp
/* .ft B
/*	Postfix queue IDs are reused.
/*	There is a very small possibility that postsuper deletes the
/*	wrong message file when it is executed while the Postfix mail
/*	system is running.
/* .ft R
/* .sp
/*	The scenario is as follows:
/* .RS
/* .IP 1)
/*	The Postfix queue manager deletes the message that \fBpostsuper\fR
/*	is supposed to delete, because Postfix is finished with the
/*	message.
/* .IP 2)
/*	New mail arrives, and the new message is given the same queue ID
/*	as the message that \fBpostsuper\fR is supposed to delete.
/*	The probability for reusing a deleted queue ID is about 1 in 2**15
/*	(the number of different microsecond values that the system clock
/*	can distinguish within a second).
/* .IP 3)
/*	\fBpostsuper\fR deletes the new message, instead of the old
/*	message that it should have deleted.
/* .RE
/* .IP "\fB-h \fIqueue_id\fR"
/*	Put mail "on hold" so that no attempt is made to deliver it.
/*	Move one message with the named queue ID from the named
/*	mail queue(s) (default: \fBincoming\fR, \fBactive\fR and
/*	\fBdeferred\fR) to the \fBhold\fR queue.
/*	If a \fIqueue_id\fR of \fB-\fR is specified, the program reads
/*	queue IDs from standard input.
/* .sp
/*	Specify \fB-h ALL\fR to hold all messages; for example, specify
/*	\fB-h ALL deferred\fR to hold mail in the \fBdeferred\fR queue.
/*	As a safety measure, the word \fBALL\fR must be specified in upper
/*	case.
/* .sp
/*	Note: mail that is put "on hold" will not expire when its
/*	time in the queue exceeds the \fBmaximal_queue_lifetime\fR
/*	setting.
/* .IP "\fB-H \fIqueue_id\fR"
/*	Release mail that was put "on hold".
/*	Move one message with the named queue ID from the named
/*	mail queue(s) (default: \fBhold\fR) to the \fBdeferred\fR queue.
/*	If a \fIqueue_id\fR of \fB-\fR is specified, the program reads
/*	queue IDs from standard input.
/* .sp
/*	Specify \fB-H ALL\fR to release all mail that is "on hold".
/*	As a safety measure, the word \fBALL\fR must be specified in upper
/*	case.
/* .IP \fB-p\fR
/*	Purge old temporary files that are left over after system or
/*	software crashes.
/* .IP "\fB-r \fIqueue_id\fR"
/*	Requeue the message with the named queue ID from the named
/*	mail queue(s) (default: \fBhold\fR, \fBincoming\fR, \fBactive\fR and
/*	\fBdeferred\fR).
/*	To requeue multiple messages, specify multiple \fB-r\fR
/*	command-line options.
/*	Alternatively, if a \fIqueue_id\fR of \fB-\fR is specified,
/*	the program reads queue IDs from standard input.
/* .sp
/*	Specify \fB-r ALL\fR to requeue all messages. As a safety
/*	measure, the word \fBALL\fR must be specified in upper case.
/* .sp
/*	A requeued message is moved to the \fBmaildrop\fR queue, from
/*	where it is copied by the pickup daemon to a new file whose name
/*	is guaranteed to match the new queue file inode number. The
/*	new queue file is subjected again to mail address rewriting and
/*	substitution. This is useful when rewriting rules or virtual
/*	mappings have changed.
/* .sp
/*	Postfix queue IDs are reused.
/*	There is a very small possibility that \fBpostsuper\fR requeues
/*	the wrong message file when it is executed while the Postfix mail
/*	system is running, but no harm should be done.
/* .IP \fB-s\fR
/*	Structure check and structure repair.  It is highly recommended
/*	to perform this operation once before Postfix startup.
/* .RS
/* .IP \(bu
/*	Rename files whose name does not match the message file inode
/*	number. This operation is necessary after restoring a mail queue
/*	from a different machine, or from backup media.
/* .IP \(bu
/*	Move queue files that are in the wrong place in the file system
/*	hierarchy and remove subdirectories that are no longer needed.
/*	File position rearrangements are necessary after a change in the
/*	\fBhash_queue_names\fR and/or \fBhash_queue_depth\fR
/*	configuration parameters.
/* .RE
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the software increasingly verbose.
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream and to
/*	\fBsyslogd\fR.
/*
/*	\fBpostsuper\fR reports the number of messages deleted with \fB-d\fR,
/*	the number of messages requeued with \fB-r\fR, and the number of
/*	messages whose queue file name was fixed with \fB-s\fR. The report
/*	is written to the standard error stream and to \fBsyslogd\fR.
/* ENVIRONMENT
/* .ad
/* .fi
/* .IP MAIL_CONFIG
/*	Directory with the \fBmain.cf\fR file.
/* BUGS
/*	Mail that is not sanitized by Postfix (i.e. mail in the \fBmaildrop\fR
/*	queue) cannot be placed "on hold".
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	See the Postfix \fBmain.cf\fR file for syntax details and for
/*	default values.
/* .IP \fBhash_queue_depth\fR
/*	Number of subdirectory levels for hashed queues.
/* .IP \fBhash_queue_names\fR
/*	The names of queues that are organized into multiple levels of
/*	subdirectories.
/* SEE ALSO
/*	sendmail(1) sendmail-compatible user interface
/*	postqueue(1) unprivileged queue operations
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
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>			/* remove() */

/* Utility library. */

#include <mymalloc.h>
#include <msg.h>
#include <msg_syslog.h>
#include <vstream.h>
#include <msg_vstream.h>
#include <scan_dir.h>
#include <vstring.h>
#include <safe.h>
#include <set_ugid.h>
#include <argv.h>
#include <vstring_vstream.h>
#include <sane_fsops.h>

/* Global library. */

#include <mail_task.h>
#include <mail_conf.h>
#include <mail_params.h>
#include <mail_queue.h>
#include <mail_open_ok.h>

/* Application-specific. */

#define MAX_TEMP_AGE (60 * 60 * 24)	/* temp file maximal age */
#define STR vstring_str			/* silly little macro */

#define ACTION_STRUCT	(1<<0)		/* fix file organization */
#define ACTION_PURGE	(1<<1)		/* purge old temp files */
#define ACTION_DELETE_ONE (1<<2)	/* delete named queue file(s) */
#define ACTION_DELETE_ALL (1<<3)	/* delete all queue file(s) */
#define ACTION_REQUEUE_ONE (1<<4)	/* requeue named queue file(s) */
#define ACTION_REQUEUE_ALL (1<<5)	/* requeue all queue file(s) */
#define ACTION_HOLD_ONE	(1<<6)		/* put named queue file(s) on hold */
#define ACTION_HOLD_ALL	(1<<7)		/* put all messages on hold */
#define ACTION_RELEASE_ONE (1<<8)	/* release named queue file(s) */
#define ACTION_RELEASE_ALL (1<<9)	/* release all "on hold" mail */

#define ACTION_DEFAULT	(ACTION_STRUCT | ACTION_PURGE)

 /*
  * Actions that operate on individually named queue files. These must never
  * be done when queue file names are changed to match their inode number.
  */
#define ACTIONS_BY_QUEUE_ID	(ACTION_DELETE_ONE | ACTION_REQUEUE_ONE \
				| ACTION_HOLD_ONE | ACTION_RELEASE_ONE)

 /*
  * Mass rename operations that are postponed to a second pass after queue
  * file names are changed to match their inode number.
  */
#define ACTIONS_AFTER_INUM_FIX	(ACTION_REQUEUE_ALL | ACTION_HOLD_ALL \
				| ACTION_RELEASE_ALL)

 /*
  * Information about queue directories and what we expect to do there. If a
  * file has unexpected owner permissions and is older than some threshold,
  * the file is discarded. We don't step into maildrop subdirectories - if
  * maildrop is writable, we might end up in the wrong place, deleting the
  * wrong information.
  */
struct queue_info {
    char   *name;			/* directory name */
    int     perms;			/* expected permissions */
    int     flags;			/* see below */
};

#define RECURSE		(1<<0)		/* step into subdirectories */
#define	DONT_RECURSE	0		/* don't step into directories */

static struct queue_info queue_info[] = {
    MAIL_QUEUE_MAILDROP, MAIL_QUEUE_STAT_READY, DONT_RECURSE,
    MAIL_QUEUE_INCOMING, MAIL_QUEUE_STAT_READY, RECURSE,
    MAIL_QUEUE_ACTIVE, MAIL_QUEUE_STAT_READY, RECURSE,
    MAIL_QUEUE_DEFERRED, MAIL_QUEUE_STAT_READY, RECURSE,
    MAIL_QUEUE_HOLD, MAIL_QUEUE_STAT_READY, RECURSE,
    MAIL_QUEUE_DEFER, 0600, RECURSE,
    MAIL_QUEUE_BOUNCE, 0600, RECURSE,
    MAIL_QUEUE_FLUSH, 0600, RECURSE,
    0,
};

 /*
  * Directories with per-message meta files.
  */
const char *log_queue_names[] = {
    MAIL_QUEUE_BOUNCE,
    MAIL_QUEUE_DEFER,
    0,
};

 /*
  * Cruft that we append to a file name when a queue ID is named after the
  * message file inode number. This cruft must not pass mail_queue_id_ok() so
  * that the queue manager will ignore it, should people be so unwise as to
  * run this operation on a live mail system.
  */
#define SUFFIX		"#FIX"
#define SUFFIX_LEN	4

 /*
  * Grr. These counters are global, because C only has clumsy ways to return
  * multiple results from a function.
  */
static int message_requeued = 0;	/* requeued messages */
static int message_held = 0;		/* messages put on hold */
static int message_released = 0;	/* messages released from hold */
static int message_deleted = 0;		/* deleted messages */
static int inode_fixed = 0;		/* queue id matched to inode number */
static int inode_mismatch = 0;		/* queue id inode mismatch */
static int position_mismatch = 0;	/* file position mismatch */

 /*
  * Silly little macros. These translate arcane expressions into something
  * more at a conceptual level.
  */
#define MESSAGE_QUEUE(qp) ((qp)->perms == MAIL_QUEUE_STAT_READY)
#define READY_MESSAGE(st) (((st).st_mode & S_IRWXU) == MAIL_QUEUE_STAT_READY)

/* find_queue_info - look up expected permissions field by queue name */

static struct queue_info *find_queue_info(const char *queue_name)
{
    struct queue_info *qp;

    for (qp = queue_info; qp->name; qp++)
	if (strcmp(queue_name, qp->name) == 0)
	    return (qp);
    msg_fatal("invalid directory name: %s", queue_name);
}

/* postremove - remove file with extreme prejudice */

static int postremove(const char *path)
{
    int     ret;

    if ((ret = remove(path)) < 0) {
	if (errno != ENOENT)
	    msg_fatal("remove file %s: %m", path);
    } else {
	if (msg_verbose)
	    msg_info("removed file %s", path);
    }
    return (ret);
}

/* postrename - rename file with extreme prejudice */

static int postrename(const char *old, const char *new)
{
    int     ret;

    if ((ret = sane_rename(old, new)) < 0) {
	if (errno != ENOENT
	    || mail_queue_mkdirs(new) < 0
	    || sane_rename(old, new) < 0)
	    if (errno != ENOENT)
		msg_fatal("rename file %s as %s: %m", old, new);
    } else {
	if (msg_verbose)
	    msg_info("renamed file %s as %s", old, new);
    }
    return (ret);
}

/* postrmdir - remove directory with extreme prejudice */

static int postrmdir(const char *path)
{
    int     ret;

    if ((ret = rmdir(path)) < 0) {
	if (errno != ENOENT)
	    msg_fatal("remove directory %s: %m", path);
    } else {
	if (msg_verbose)
	    msg_info("remove directory %s", path);
    }
    return (ret);
}

/* delete_one - delete one message instance and all its associated files */

static int delete_one(const char **queue_names, const char *queue_id)
{
    struct stat st;
    const char **msg_qpp;
    const char **log_qpp;
    const char *msg_path;
    VSTRING *log_path_buf;
    int     found;
    int     tries;

    /*
     * Sanity check. No early returns beyond this point.
     */
    if (!mail_queue_id_ok(queue_id)) {
	msg_warn("invalid mail queue id: %s", queue_id);
	return (0);
    }
    log_path_buf = vstring_alloc(100);

    /*
     * Skip meta file directories. Delete defer or bounce logfiles before
     * deleting the corresponding message file, and only if the message file
     * exists. This minimizes but does not eliminate a race condition with
     * queue ID reuse which results in deleting the wrong files.
     */
    for (found = 0, tries = 0; found == 0 && tries < 2; tries++) {
	for (msg_qpp = queue_names; *msg_qpp != 0; msg_qpp++) {
	    if (!MESSAGE_QUEUE(find_queue_info(*msg_qpp)))
		continue;
	    if (mail_open_ok(*msg_qpp, queue_id, &st, &msg_path) != MAIL_OPEN_YES)
		continue;
	    for (log_qpp = log_queue_names; *log_qpp != 0; log_qpp++)
		postremove(mail_queue_path(log_path_buf, *log_qpp, queue_id));
	    if (postremove(msg_path) == 0) {
		found = 1;
		msg_info("%s: removed", queue_id);
		break;
	    }					/* else: maybe lost a race */
	}
    }
    vstring_free(log_path_buf);
    return (found);
}

/* requeue_one - requeue one message instance and delete its logfiles */

static int requeue_one(const char **queue_names, const char *queue_id)
{
    struct stat st;
    const char **msg_qpp;
    const char *old_path;
    VSTRING *new_path_buf;
    int     found;
    int     tries;

    /*
     * Sanity check. No early returns beyond this point.
     */
    if (!mail_queue_id_ok(queue_id)) {
	msg_warn("invalid mail queue id: %s", queue_id);
	return (0);
    }
    new_path_buf = vstring_alloc(100);

    /*
     * Skip meta file directories. Like the mass requeue operation, we not
     * delete defer or bounce logfiles, to avoid losing a race where the
     * queue manager decides to bounce mail after all recipients have been
     * tried.
     */
    for (found = 0, tries = 0; found == 0 && tries < 2; tries++) {
	for (msg_qpp = queue_names; *msg_qpp != 0; msg_qpp++) {
	    if (strcmp(*msg_qpp, MAIL_QUEUE_MAILDROP) == 0)
		continue;
	    if (!MESSAGE_QUEUE(find_queue_info(*msg_qpp)))
		continue;
	    if (mail_open_ok(*msg_qpp, queue_id, &st, &old_path) != MAIL_OPEN_YES)
		continue;
	    (void) mail_queue_path(new_path_buf, MAIL_QUEUE_MAILDROP, queue_id);
	    if (postrename(old_path, STR(new_path_buf)) == 0) {
		msg_info("%s: requeued", queue_id);
		found = 1;
		break;
	    }					/* else: maybe lost a race */
	}
    }
    vstring_free(new_path_buf);
    return (found);
}

/* hold_one - put "on hold" one message instance */

static int hold_one(const char **queue_names, const char *queue_id)
{
    struct stat st;
    const char **msg_qpp;
    const char *old_path;
    VSTRING *new_path_buf;
    int     found;
    int     tries;

    /*
     * Sanity check. No early returns beyond this point.
     */
    if (!mail_queue_id_ok(queue_id)) {
	msg_warn("invalid mail queue id: %s", queue_id);
	return (0);
    }
    new_path_buf = vstring_alloc(100);

    /*
     * Skip meta file directories. Like the mass requeue operation, we not
     * delete defer or bounce logfiles, to avoid losing a race where the
     * queue manager decides to bounce mail after all recipients have been
     * tried.
     * 
     * XXX We must not put maildrop mail on hold because that would mix already
     * sanitized mail with mail that still needs to be sanitized.
     */
    for (found = 0, tries = 0; found == 0 && tries < 2; tries++) {
	for (msg_qpp = queue_names; *msg_qpp != 0; msg_qpp++) {
	    if (strcmp(*msg_qpp, MAIL_QUEUE_MAILDROP) == 0)
		continue;
	    if (strcmp(*msg_qpp, MAIL_QUEUE_HOLD) == 0)
		continue;
	    if (!MESSAGE_QUEUE(find_queue_info(*msg_qpp)))
		continue;
	    if (mail_open_ok(*msg_qpp, queue_id, &st, &old_path) != MAIL_OPEN_YES)
		continue;
	    (void) mail_queue_path(new_path_buf, MAIL_QUEUE_HOLD, queue_id);
	    if (postrename(old_path, STR(new_path_buf)) == 0) {
		msg_info("%s: placed on hold", queue_id);
		found = 1;
		break;
	    }					/* else: maybe lost a race */
	}
    }
    vstring_free(new_path_buf);
    return (found);
}

/* release_one - release one message instance that was placed "on hold" */

static int release_one(const char **queue_names, const char *queue_id)
{
    struct stat st;
    const char **msg_qpp;
    const char *old_path;
    VSTRING *new_path_buf;
    int     found;

    /*
     * Sanity check. No early returns beyond this point.
     */
    if (!mail_queue_id_ok(queue_id)) {
	msg_warn("invalid mail queue id: %s", queue_id);
	return (0);
    }
    new_path_buf = vstring_alloc(100);

    /*
     * Skip inapplicable directories. This can happen when -H is combined
     * with other operations.
     */
    found = 0;
    for (msg_qpp = queue_names; *msg_qpp != 0; msg_qpp++) {
	if (strcmp(*msg_qpp, MAIL_QUEUE_HOLD) != 0)
	    continue;
	if (mail_open_ok(*msg_qpp, queue_id, &st, &old_path) != MAIL_OPEN_YES)
	    continue;
	(void) mail_queue_path(new_path_buf, MAIL_QUEUE_DEFERRED, queue_id);
	if (postrename(old_path, STR(new_path_buf)) == 0) {
	    msg_info("%s: released from hold", queue_id);
	    found = 1;
	    break;
	}
    }
    vstring_free(new_path_buf);
    return (found);
}

/* operate_stream - operate on queue IDs given on stream */

static int operate_stream(VSTREAM *fp,
		              int (*operator) (const char **, const char *),
			          const char **queues)
{
    VSTRING *buf = vstring_alloc(20);
    int     found = 0;

    while (vstring_get_nonl(buf, fp) != VSTREAM_EOF)
	found += operator(queues, STR(buf));

    vstring_free(buf);
    return (found);
}

/* fix_queue_id - make message queue ID match inode number */

static int fix_queue_id(const char *actual_path, const char *actual_queue,
			        const char *actual_id, ino_t inum)
{
    VSTRING *old_path = vstring_alloc(10);
    VSTRING *new_path = vstring_alloc(10);
    VSTRING *new_id = vstring_alloc(10);
    const char **log_qpp;
    int     ret;

    /*
     * Create the new queue ID from the existing time digits and from the new
     * inode number. Since we are renaming multiple files, the new name must
     * be deterministic so that we can recover even when the renaming
     * operation is interrupted in the middle.
     */
    vstring_sprintf(new_id, "%.5s%lX", actual_id, (unsigned long) inum);

    /*
     * Rename logfiles before renaming the message file, so that we can
     * recover when a previous attempt was interrupted.
     */
    for (log_qpp = log_queue_names; *log_qpp; log_qpp++) {
	mail_queue_path(old_path, *log_qpp, actual_id);
	mail_queue_path(new_path, *log_qpp, STR(new_id));
	vstring_strcat(new_path, SUFFIX);
	postrename(STR(old_path), STR(new_path));
    }

    /*
     * Rename the message file last, so that we know that we are done with
     * this message and with all its logfiles.
     */
    mail_queue_path(new_path, actual_queue, STR(new_id));
    vstring_strcat(new_path, SUFFIX);
    ret = postrename(actual_path, STR(new_path));

    /*
     * Clean up.
     */
    vstring_free(old_path);
    vstring_free(new_path);
    vstring_free(new_id);

    return (ret);
}

/* super - check queue structure, clean up, do wild-card operations */

static void super(const char **queues, int action)
{
    ARGV   *hash_queue_names = argv_split(var_hash_queue_names, " \t\r\n,");
    VSTRING *actual_path = vstring_alloc(10);
    VSTRING *wanted_path = vstring_alloc(10);
    struct stat st;
    const char *queue_name;
    SCAN_DIR *info;
    char   *path;
    int     actual_depth;
    int     wanted_depth;
    char  **cpp;
    struct queue_info *qp;
    unsigned long inum;

    /*
     * Make sure every file is in the right place, clean out stale files, and
     * remove non-file/non-directory objects.
     */
    while ((queue_name = *queues++) != 0) {

	if (msg_verbose)
	    msg_info("queue: %s", queue_name);

	/*
	 * Look up queue-specific properties: desired hashing depth, what
	 * file permissions to look for, and whether or not it is desirable
	 * to step into subdirectories.
	 */
	qp = find_queue_info(queue_name);
	for (cpp = hash_queue_names->argv; /* void */ ; cpp++) {
	    if (*cpp == 0) {
		wanted_depth = 0;
		break;
	    }
	    if (strcmp(*cpp, queue_name) == 0) {
		wanted_depth = var_hash_queue_depth;
		break;
	    }
	}

	/*
	 * Sanity check. Some queues just cannot be recursive.
	 */
	if (wanted_depth > 0 && (qp->flags & RECURSE) == 0)
	    msg_fatal("%s queue must not be hashed", queue_name);

	/*
	 * Other per-directory initialization.
	 */
	info = scan_dir_open(queue_name);
	actual_depth = 0;

	for (;;) {

	    /*
	     * If we reach the end of a subdirectory, return to its parent.
	     * Delete subdirectories that are no longer needed.
	     */
	    if ((path = scan_dir_next(info)) == 0) {
		if (actual_depth == 0)
		    break;
		if (actual_depth > wanted_depth)
		    postrmdir(scan_dir_path(info));
		scan_dir_pop(info);
		actual_depth--;
		continue;
	    }

	    /*
	     * If we stumble upon a subdirectory, enter it, if it is
	     * considered safe to do so. Otherwise, try to remove the
	     * subdirectory at a later stage.
	     */
	    if (strlen(path) == 1 && (qp->flags & RECURSE) != 0) {
		actual_depth++;
		scan_dir_push(info, path);
		continue;
	    }

	    /*
	     * From here on we need to keep track of operations that
	     * invalidate or revalidate the actual_path and path variables,
	     * otherwise we can hit the wrong files.
	     */
	    vstring_sprintf(actual_path, "%s/%s", scan_dir_path(info), path);
	    if (stat(STR(actual_path), &st) < 0)
		continue;

	    /*
	     * Remove alien directories. If maildrop is compromised, then we
	     * cannot abort just because we cannot remove someone's
	     * directory.
	     */
	    if (S_ISDIR(st.st_mode)) {
		if (rmdir(STR(actual_path)) < 0) {
		    if (errno != ENOENT)
			msg_warn("remove subdirectory %s: %m", STR(actual_path));
		} else {
		    if (msg_verbose)
			msg_info("remove subdirectory %s", STR(actual_path));
		}
		/* No further work on this object is possible. */
		continue;
	    }

	    /*
	     * Mass deletion. We count the deletion of mail that this system
	     * has taken responsibility for. XXX This option does not use
	     * mail_queue_remove(), so that it can avoid having to first move
	     * queue files to the "right" subdirectory level.
	     */
	    if (action & ACTION_DELETE_ALL) {
		if (postremove(STR(actual_path)) == 0)
		    if (MESSAGE_QUEUE(qp) && READY_MESSAGE(st))
			message_deleted++;
		/* No further work on this object is possible. */
		continue;
	    }

	    /*
	     * Remove non-file objects and old temporary files. Be careful
	     * not to delete bounce or defer logs just because they are more
	     * than a couple days old.
	     */
	    if (!S_ISREG(st.st_mode)
		|| ((action & ACTION_PURGE) != 0
		    && MESSAGE_QUEUE(qp)
		    && !READY_MESSAGE(st)
		    && time((time_t *) 0) > st.st_mtime + MAX_TEMP_AGE)) {
		(void) postremove(STR(actual_path));
		/* No further work on this object is possible. */
		continue;
	    }

	    /*
	     * Fix queueid#FIX names that were left from a previous pass over
	     * the queue where message queue file names were matched to their
	     * inode number. We strip the suffix and move the file into the
	     * proper subdirectory level. Make sure that the name minus
	     * suffix is well formed and that the name matches the file inode
	     * number.
	     */
	    if ((action & ACTION_STRUCT)
		&& strcmp(path + (strlen(path) - SUFFIX_LEN), SUFFIX) == 0) {
		path[strlen(path) - SUFFIX_LEN] = 0;	/* XXX */
		if (!mail_queue_id_ok(path)) {
		    msg_warn("bogus file name: %s", STR(actual_path));
		    continue;
		}
		if (MESSAGE_QUEUE(qp)) {
		    if (sscanf(path + 5, "%lx", &inum) != 1) {
			msg_warn("bogus file name: %s", STR(actual_path));
			continue;
		    }
		    if (inum != (unsigned long) st.st_ino) {
			msg_warn("name/inode mismatch: %s", STR(actual_path));
			continue;
		    }
		}
		(void) mail_queue_path(wanted_path, queue_name, path);
		if (postrename(STR(actual_path), STR(wanted_path)) < 0) {
		    /* No further work on this object is possible. */
		    continue;
		} else {
		    if (MESSAGE_QUEUE(qp))
			inode_fixed++;
		    vstring_strcpy(actual_path, STR(wanted_path));
		    /* At this point, path and actual_path are revalidated. */
		}
	    }

	    /*
	     * Skip over files with illegal names. The library routines
	     * refuse to operate on them.
	     */
	    if (!mail_queue_id_ok(path)) {
		msg_warn("bogus file name: %s", STR(actual_path));
		continue;
	    }

	    /*
	     * See if the file name matches the file inode number. Skip meta
	     * file directories. This option requires that meta files be put
	     * into their proper place before queue files, so that we can
	     * rename queue files and meta files at the same time. Mis-named
	     * files are renamed to newqueueid#FIX on the first pass, and
	     * upon the second pass the #FIX is stripped off the name. Of
	     * course we have to be prepared that the program is interrupted
	     * before it completes, so any left-over newqueueid#FIX files
	     * have to be handled properly. XXX This option cannot use
	     * mail_queue_rename(), because the queue file name violates
	     * normal queue file syntax.
	     */
	    if ((action & ACTION_STRUCT) != 0 && MESSAGE_QUEUE(qp)) {
		if (sscanf(path + 5, "%lx", &inum) != 1) {
		    msg_warn("bogus file name: %s", STR(actual_path));
		    continue;
		}
		if (inum != (unsigned long) st.st_ino) {
		    inode_mismatch++;		/* before we fix */
		    action &= ~ACTIONS_AFTER_INUM_FIX;
		    fix_queue_id(STR(actual_path), queue_name, path, st.st_ino);
		    /* At this point, path and actual_path are invalidated. */
		    continue;
		}
	    }

	    /*
	     * Mass requeuing. The pickup daemon will copy requeued mail to a
	     * new queue file, so that address rewriting is applied again.
	     * XXX This option does not use mail_queue_rename(), so that it
	     * can avoid having to first move queue files to the "right"
	     * subdirectory level. Like the requeue_one() routine, this code
	     * does not touch logfiles.
	     */
	    if ((action & ACTION_REQUEUE_ALL)
		&& MESSAGE_QUEUE(qp)
		&& strcmp(queue_name, MAIL_QUEUE_MAILDROP) != 0) {
		(void) mail_queue_path(wanted_path, MAIL_QUEUE_MAILDROP, path);
		if (postrename(STR(actual_path), STR(wanted_path)) == 0)
		    message_requeued++;
		/* At this point, path and actual_path are invalidated. */
		continue;
	    }

	    /*
	     * Mass renaming to the "on hold" queue. XXX This option does not
	     * use mail_queue_rename(), so that it can avoid having to first
	     * move queue files to the "right" subdirectory level. Like the
	     * hold_one() routine, this code does not touch logfiles, and
	     * must not touch files in the maildrop queue, because maildrop
	     * files contain data that has not yet been sanitized and
	     * therefore must not be mixed with already sanitized mail.
	     */
	    if ((action & ACTION_HOLD_ALL)
		&& MESSAGE_QUEUE(qp)
		&& strcmp(queue_name, MAIL_QUEUE_MAILDROP) != 0
		&& strcmp(queue_name, MAIL_QUEUE_HOLD) != 0) {
		(void) mail_queue_path(wanted_path, MAIL_QUEUE_HOLD, path);
		if (postrename(STR(actual_path), STR(wanted_path)) == 0)
		    message_held++;
		/* At this point, path and actual_path are invalidated. */
		continue;
	    }

	    /*
	     * Mass release from the "on hold" queue. XXX This option does
	     * not use mail_queue_rename(), so that it can avoid having to
	     * first move queue files to the "right" subdirectory level. Like
	     * the release_one() routine, this code must not touch logfiles.
	     */
	    if ((action & ACTION_RELEASE_ALL)
		&& strcmp(queue_name, MAIL_QUEUE_HOLD) == 0) {
		(void) mail_queue_path(wanted_path, MAIL_QUEUE_DEFERRED, path);
		if (postrename(STR(actual_path), STR(wanted_path)) == 0)
		    message_released++;
		/* At this point, path and actual_path are invalidated. */
		continue;
	    }

	    /*
	     * See if this file sits in the right place in the file system
	     * hierarchy. Its place may be wrong after a change to the
	     * hash_queue_{names,depth} parameter settings. This requires
	     * that the bounce/defer logfiles be at the right subdirectory
	     * level first, otherwise we would fail to properly rename
	     * bounce/defer logfiles.
	     */
	    if (action & ACTION_STRUCT) {
		(void) mail_queue_path(wanted_path, queue_name, path);
		if (strcmp(STR(actual_path), STR(wanted_path)) != 0) {
		    position_mismatch++;	/* before we fix */
		    (void) postrename(STR(actual_path), STR(wanted_path));
		    /* At this point, path and actual_path are invalidated. */
		    continue;
		}
	    }
	}
	scan_dir_close(info);
    }

    /*
     * Clean up.
     */
    vstring_free(wanted_path);
    vstring_free(actual_path);
    argv_free(hash_queue_names);
}

/* fatal_exit - print warning if queue fix is incomplete */

static void fatal_exit(void)
{
    if (inode_mismatch > 0 || inode_fixed > 0 || position_mismatch > 0)
	msg_fatal("OPERATION INCOMPLETE -- RERUN COMMAND TO FIX THE QUEUE FIRST");
}

/* interrupted - signal handler */

static void interrupted(int unused_sig)
{
    fatal_exit();
}

int     main(int argc, char **argv)
{
    int     fd;
    struct stat st;
    char   *slash;
    int     action = 0;
    const char **queues;
    int     c;
    ARGV   *requeue_names = 0;
    ARGV   *delete_names = 0;
    ARGV   *hold_names = 0;
    ARGV   *release_names = 0;
    char  **cpp;

    /*
     * Defaults. The structural checks must fix the directory levels of "log
     * file" directories (bounce, defer) before doing structural checks on
     * the "message file" directories, so that we can find the logfiles in
     * the right place when message files need to be renamed to match their
     * inode number.
     */
    static char *default_queues[] = {
	MAIL_QUEUE_DEFER,		/* before message directories */
	MAIL_QUEUE_BOUNCE,		/* before message directories */
	MAIL_QUEUE_MAILDROP,
	MAIL_QUEUE_INCOMING,
	MAIL_QUEUE_ACTIVE,
	MAIL_QUEUE_DEFERRED,
	MAIL_QUEUE_HOLD,
	MAIL_QUEUE_FLUSH,
	0,
    };
    static char *default_hold_queues[] = {
	MAIL_QUEUE_INCOMING,
	MAIL_QUEUE_ACTIVE,
	MAIL_QUEUE_DEFERRED,
	0,
    };
    static char *default_release_queues[] = {
	MAIL_QUEUE_HOLD,
	0,
    };

    /*
     * Be consistent with file permissions.
     */
    umask(022);

    /*
     * To minimize confusion, make sure that the standard file descriptors
     * are open before opening anything else. XXX Work around for 44BSD where
     * fstat can return EBADF on an open file descriptor.
     */
    for (fd = 0; fd < 3; fd++)
	if (fstat(fd, &st) == -1
	    && (close(fd), open("/dev/null", O_RDWR, 0)) != fd)
	    msg_fatal("open /dev/null: %m");

    /*
     * Process environment options as early as we can. We might be called
     * from a set-uid (set-gid) program, so be careful with importing
     * environment variables.
     */
    if (safe_getenv(CONF_ENV_VERB))
	msg_verbose = 1;

    /*
     * Initialize. Set up logging, read the global configuration file and
     * extract configuration information.
     */
    if ((slash = strrchr(argv[0], '/')) != 0)
	argv[0] = slash + 1;
    msg_vstream_init(argv[0], VSTREAM_ERR);
    msg_syslog_init(mail_task(argv[0]), LOG_PID, LOG_FACILITY);
    set_mail_conf_str(VAR_PROCNAME, var_procname = mystrdup(argv[0]));

    mail_conf_read();
    if (chdir(var_queue_dir))
	msg_fatal("chdir %s: %m", var_queue_dir);

    /*
     * Be sure to log a warning if we do not finish structural repair. Maybe
     * we should have an fsck-style "clean" flag so Postfix will not start
     * with a broken queue.
     */
    signal(SIGHUP, interrupted);
    signal(SIGINT, interrupted);
    signal(SIGQUIT, interrupted);
    signal(SIGTERM, interrupted);
    msg_cleanup(fatal_exit);

    /*
     * All file/directory updates must be done as the mail system owner. This
     * is because Postfix daemons manipulate the queue with those same
     * privileges, so directories must be created with the right ownership.
     * 
     * Running as a non-root user is also required for security reasons. When
     * the Postfix queue hierarchy is compromised, an attacker could trick us
     * into entering other file hierarchies and afflicting damage. Running as
     * a non-root user limits the damage to the already compromised mail
     * owner.
     */
    if (getuid())
	msg_fatal("use of this command is reserved for the superuser");
    set_ugid(var_owner_uid, var_owner_gid);

    /*
     * Parse JCL.
     */
    while ((c = GETOPT(argc, argv, "d:h:H:pr:sv")) > 0) {
	switch (c) {
	default:
	    msg_fatal("usage: %s [-d queue_id (delete)] "
		      "[-h queue_id (hold)] [-H queue_id (un-hold)] "
		      "[-p (purge temporary files)] [-r queue_id (requeue)] "
		      "[-s (structure fix)] [-v (verbose)] "
		      "[queue...]", argv[0]);
	case 'd':
	    if (delete_names == 0)
		delete_names = argv_alloc(1);
	    argv_add(delete_names, optarg, (char *) 0);
	    action |= (strcmp(optarg, "ALL") == 0 ?
		       ACTION_DELETE_ALL : ACTION_DELETE_ONE);
	    break;
	case 'h':
	    if (hold_names == 0)
		hold_names = argv_alloc(1);
	    argv_add(hold_names, optarg, (char *) 0);
	    action |= (strcmp(optarg, "ALL") == 0 ?
		       ACTION_HOLD_ALL : ACTION_HOLD_ONE);
	    break;
	case 'H':
	    if (release_names == 0)
		release_names = argv_alloc(1);
	    argv_add(release_names, optarg, (char *) 0);
	    action |= (strcmp(optarg, "ALL") == 0 ?
		       ACTION_RELEASE_ALL : ACTION_RELEASE_ONE);
	    break;
	case 'p':
	    action |= ACTION_PURGE;
	    break;
	case 'r':
	    if (requeue_names == 0)
		requeue_names = argv_alloc(1);
	    argv_add(requeue_names, optarg, (char *) 0);
	    action |= (strcmp(optarg, "ALL") == 0 ?
		       ACTION_REQUEUE_ALL : ACTION_REQUEUE_ONE);
	    break;
	case 's':
	    action |= ACTION_STRUCT;
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	}
    }

    /*
     * Sanity checks.
     */
    if ((action & ACTION_DELETE_ALL) && (action & ACTION_DELETE_ONE)) {
	msg_warn("option \"-d ALL\" will ignore other command line queue IDs");
	action &= ~ACTION_DELETE_ONE;
    }
    if ((action & ACTION_REQUEUE_ALL) && (action & ACTION_REQUEUE_ONE)) {
	msg_warn("option \"-r ALL\" will ignore other command line queue IDs");
	action &= ~ACTION_REQUEUE_ONE;
    }
    if ((action & ACTION_HOLD_ALL) && (action & ACTION_HOLD_ONE)) {
	msg_warn("option \"-h ALL\" will ignore other command line queue IDs");
	action &= ~ACTION_HOLD_ONE;
    }
    if ((action & ACTION_RELEASE_ALL) && (action & ACTION_RELEASE_ONE)) {
	msg_warn("option \"-H ALL\" will ignore other command line queue IDs");
	action &= ~ACTION_RELEASE_ONE;
    }

    /*
     * Execute the explicitly specified (or default) action, on the
     * explicitly specified (or default) queues.
     * 
     * XXX Work around gcc const brain damage.
     * 
     * XXX The file name/inode number fix should always run over all message
     * file directories, and should always be preceded by a subdirectory
     * level check of the bounce and defer logfile directories.
     */
    if (action == 0)
	action = ACTION_DEFAULT;
    if (argv[optind] != 0)
	queues = (const char **) argv + optind;
    else if (action == ACTION_HOLD_ALL)
	queues = (const char **) default_hold_queues;
    else if (action == ACTION_RELEASE_ALL)
	queues = (const char **) default_release_queues;
    else
	queues = (const char **) default_queues;

    /*
     * Basic queue maintenance, as well as mass deletion, mass requeuing, and
     * mass name-to-inode fixing. This ensures that queue files are in the
     * right place before the file-by-name operations are done.
     */
    if (action & ~ACTIONS_BY_QUEUE_ID)
	super(queues, action);

    /*
     * If any file names needed changing to match the message file inode
     * number, those files were named newqeueid#FIX. We need a second pass to
     * strip the suffix from the new queue ID, and to complete any requested
     * operations that had to be skipped in the first pass.
     */
    if (inode_mismatch > 0)
	super(queues, action);

    /*
     * Don't do actions by queue file name if any queue files changed name
     * because they did not match the queue file inode number. We could be
     * acting on the wrong queue file and lose mail.
     */
    if ((action & ACTIONS_BY_QUEUE_ID)
	&& (inode_mismatch > 0 || inode_fixed > 0)) {
	msg_error("QUEUE FILE NAMES WERE CHANGED TO MATCH INODE NUMBERS");
	msg_fatal("CHECK YOUR QUEUE IDS AND RE-ISSUE THE COMMAND");
    }

    /*
     * Delete queue files by name. This must not be done when queue file
     * names have changed names as a result of inode number mismatches,
     * because we could be deleting the wrong message.
     */
    if (action & ACTION_DELETE_ONE) {
	argv_terminate(delete_names);
	queues = (const char **)
	    (argv[optind] ? argv + optind : default_queues);
	for (cpp = delete_names->argv; *cpp; cpp++) {
	    if (strcmp(*cpp, "ALL") == 0)
		continue;
	    if (strcmp(*cpp, "-") == 0)
		message_deleted +=
		    operate_stream(VSTREAM_IN, delete_one, queues);
	    else
		message_deleted += delete_one(queues, *cpp);
	}
    }

    /*
     * Requeue queue files by name. This must not be done when queue file
     * names have changed names as a result of inode number mismatches,
     * because we could be requeuing the wrong message.
     */
    if (action & ACTION_REQUEUE_ONE) {
	argv_terminate(requeue_names);
	queues = (const char **)
	    (argv[optind] ? argv + optind : default_queues);
	for (cpp = requeue_names->argv; *cpp; cpp++) {
	    if (strcmp(*cpp, "ALL") == 0)
		continue;
	    if (strcmp(*cpp, "-") == 0)
		message_requeued +=
		    operate_stream(VSTREAM_IN, requeue_one, queues);
	    else
		message_requeued += requeue_one(queues, *cpp);
	}
    }

    /*
     * Put on hold queue files by name. This must not be done when queue file
     * names have changed names as a result of inode number mismatches,
     * because we could put on hold the wrong message.
     */
    if (action & ACTION_HOLD_ONE) {
	argv_terminate(hold_names);
	queues = (const char **)
	    (argv[optind] ? argv + optind : default_hold_queues);
	for (cpp = hold_names->argv; *cpp; cpp++) {
	    if (strcmp(*cpp, "ALL") == 0)
		continue;
	    if (strcmp(*cpp, "-") == 0)
		message_held +=
		    operate_stream(VSTREAM_IN, hold_one, queues);
	    else
		message_held += hold_one(queues, *cpp);
	}
    }

    /*
     * Take "off hold" queue files by name. This must not be done when queue
     * file names have changed names as a result of inode number mismatches,
     * because we could take off hold the wrong message.
     */
    if (action & ACTION_RELEASE_ONE) {
	argv_terminate(release_names);
	queues = (const char **)
	    (argv[optind] ? argv + optind : default_release_queues);
	for (cpp = release_names->argv; *cpp; cpp++) {
	    if (strcmp(*cpp, "ALL") == 0)
		continue;
	    if (strcmp(*cpp, "-") == 0)
		message_released +=
		    operate_stream(VSTREAM_IN, release_one, queues);
	    else
		message_released += release_one(queues, *cpp);
	}
    }

    /*
     * Report.
     */
    if (message_requeued > 0)
	msg_info("Requeued: %d message%s", message_requeued,
		 message_requeued > 1 ? "s" : "");
    if (message_deleted > 0)
	msg_info("Deleted: %d message%s", message_deleted,
		 message_deleted > 1 ? "s" : "");
    if (message_held > 0)
	msg_info("Placed on hold: %d message%s",
		 message_held, message_held > 1 ? "s" : "");
    if (message_released > 0)
	msg_info("Released from hold: %d message%s",
		 message_released, message_released > 1 ? "s" : "");
    if (inode_fixed > 0)
	msg_info("Renamed to match inode number: %d message%s", inode_fixed,
		 inode_fixed > 1 ? "s" : "");
    if (inode_mismatch > 0 || inode_fixed > 0)
	msg_warn("QUEUE FILE NAMES WERE CHANGED TO MATCH INODE NUMBERS");

    /*
     * Clean up.
     */
    if (requeue_names)
	argv_free(requeue_names);
    if (delete_names)
	argv_free(delete_names);
    if (hold_names)
	argv_free(hold_names);
    if (release_names)
	argv_free(release_names);

    exit(0);
}
