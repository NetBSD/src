/*++
/* NAME
/*	pickup 8
/* SUMMARY
/*	Postfix local mail pickup
/* SYNOPSIS
/*	\fBpickup\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBpickup\fR daemon waits for hints that new mail has been
/*	dropped into the \fBmaildrop\fR directory, and feeds it into the
/*	\fBcleanup\fR(8) daemon.
/*	Ill-formatted files are deleted without notifying the originator.
/*	This program expects to be run from the \fBmaster\fR(8) process
/*	manager.
/* STANDARDS
/* .ad
/* .fi
/*	None. The \fBpickup\fR daemon does not interact with the outside world.
/* SECURITY
/* .ad
/* .fi
/*	The \fBpickup\fR daemon runs with superuser privileges so that it
/*	1) can open a queue file with the rights of the submitting user
/*	and 2) can access the Postfix private IPC channels.
/*	On the positive side, the program can run chrooted, opens no files
/*	for writing, is careful about what files it opens for reading, and
/*	does not actually touch any data that is sent to its public service
/*	endpoint.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* BUGS
/*	The \fBpickup\fR daemon copies mail from file to the \fBcleanup\fR(8)
/*	daemon.  It could avoid message copying overhead by sending a file
/*	descriptor instead of file data, but then the already complex
/*	\fBcleanup\fR(8) daemon would have to deal with unfiltered user data.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program. See the Postfix \fBmain.cf\fR file for syntax details
/*	and for default values. Use the \fBpostfix reload\fR command after
/*	a configuration change.
/* .SH "Content inspection controls"
/* .IP \fBcontent_filter\fR
/*	The name of a mail delivery transport that filters mail and that
/*	either bounces mail or re-injects the result back into Postfix.
/*	This parameter uses the same syntax as the right-hand side of
/*	a Postfix transport table.
/* .SH Miscellaneous
/* .ad
/* .fi
/* .IP \fBalways_bcc\fR
/*	Address to send a copy of each message that enters the system.
/* .IP \fBmail_owner\fR
/*	The process privileges used while not opening a \fBmaildrop\fR file.
/* .IP \fBqueue_directory\fR
/*	Top-level directory of the Postfix queue.
/* SEE ALSO
/*	cleanup(8) message canonicalization
/*	master(8) process manager
/*	sendmail(1), postdrop(8) mail posting agent
/*	syslogd(8) system logging
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
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

/* Utility library. */

#include <msg.h>
#include <scan_dir.h>
#include <vstring.h>
#include <vstream.h>
#include <set_ugid.h>
#include <safe_open.h>

/* Global library. */

#include <mail_queue.h>
#include <mail_open_ok.h>
#include <mymalloc.h>
#include <mail_proto.h>
#include <cleanup_user.h>
#include <mail_date.h>
#include <mail_params.h>
#include <mail_conf.h>
#include <record.h>
#include <rec_type.h>

/* Single-threaded server skeleton. */

#include <mail_server.h>

/* Application-specific. */

char   *var_always_bcc;
char   *var_filter_xport;

 /*
  * Structure to bundle a bunch of information about a queue file.
  */
typedef struct {
    char   *id;				/* queue file basename */
    struct stat st;			/* queue file status */
    char   *path;			/* name for open/remove */
    char   *sender;			/* sender address */
    char   *rcpt;			/* recipient address */
} PICKUP_INFO;

 /*
  * What action should be taken after attempting to deliver a message: remove
  * the file from the maildrop, or leave it alone. The latter is also used
  * for files that are still being written to.
  */
#define REMOVE_MESSAGE_FILE	1
#define KEEP_MESSAGE_FILE	2

/* file_read_error - handle error while reading queue file */

static int file_read_error(PICKUP_INFO *info, int type)
{
    msg_warn("uid=%ld: unexpected or malformed record type %d",
	     (long) info->st.st_uid, type);
    return (REMOVE_MESSAGE_FILE);
}

/* cleanup_service_error - handle error writing to cleanup service. */

static int cleanup_service_error(PICKUP_INFO *info, int status)
{
    msg_warn("%s: %s", info->path, cleanup_strerror(status));
    return ((status & CLEANUP_STAT_BAD) ?
	    REMOVE_MESSAGE_FILE : KEEP_MESSAGE_FILE);
}

/* copy_segment - copy a record group */

static int copy_segment(VSTREAM *qfile, VSTREAM *cleanup, PICKUP_INFO *info,
			        VSTRING *buf, char *expected)
{
    int     type;
    int     check_first = (*expected == REC_TYPE_CONTENT[0]);

    /*
     * Limit the input record size. All front-end programs should protect the
     * mail system against unreasonable inputs. This also requires that we
     * limit the size of envelope records written by the local posting agent.
     * As time stamp we use the scrutinized queue file modification time, and
     * ignore the time stamp embedded in the queue file.
     */
    for (;;) {
	if ((type = rec_get(qfile, buf, var_line_limit)) < 0
	    || strchr(expected, type) == 0)
	    return (file_read_error(info, type));
	if (type == *expected)
	    break;
	if (type == REC_TYPE_FROM)
	    if (info->sender == 0)
		info->sender = mystrdup(vstring_str(buf));
	if (type == REC_TYPE_RCPT)
	    if (info->rcpt == 0)
		info->rcpt = mystrdup(vstring_str(buf));
	if (type == REC_TYPE_TIME)
	    continue;
	if (type == REC_TYPE_ATTR)
	    continue;
	if (type == REC_TYPE_FILT && *expected == REC_TYPE_ENVELOPE[0])
	    continue;
	else {

	    /*
	     * XXX Force an empty record when the queue file content begins
	     * with whitespace, so that it won't be considered as being part
	     * of our own Received: header. What an ugly Kluge.
	     */
	    if (check_first) {
		check_first = 0;
		if (VSTRING_LEN(buf) > 0 && ISSPACE(vstring_str(buf)[0]))
		    rec_put(cleanup, REC_TYPE_NORM, "", 0);
	    }
	    if ((REC_PUT_BUF(cleanup, type, buf)) < 0)
		return (cleanup_service_error(info, CLEANUP_STAT_WRITE));
	}
    }
    return (0);
}

/* pickup_copy - copy message to cleanup service */

static int pickup_copy(VSTREAM *qfile, VSTREAM *cleanup,
		               PICKUP_INFO *info, VSTRING *buf)
{
    time_t  now = time((time_t *) 0);
    int     status;

    /*
     * Protect against time-warped time stamps. Warn about mail that has been
     * queued for an excessive amount of time. Allow for some time drift with
     * network clients that mount the maildrop remotely - especially clients
     * that can't get their daylight savings offsets right.
     */
#define DAY_SECONDS 86400
#define HOUR_SECONDS 3600

    if (info->st.st_mtime > now + 2 * HOUR_SECONDS) {
	msg_warn("%s: message dated %ld seconds into the future",
		 info->id, (long) (info->st.st_mtime - now));
	info->st.st_mtime = now;
    } else if (info->st.st_mtime < now - DAY_SECONDS) {
	msg_warn("%s: message has been queued for %d days",
		 info->id, (int) (now - info->st.st_mtime) / DAY_SECONDS);
    }

    /*
     * Make sure the message has a posting-time record.
     */
    rec_fprintf(cleanup, REC_TYPE_TIME, "%ld", (long) info->st.st_mtime);

    /*
     * Add content inspection transport.
     */
    if (*var_filter_xport)
	rec_fprintf(cleanup, REC_TYPE_FILT, "%s", var_filter_xport);

    /*
     * Copy the message envelope segment. Allow only those records that we
     * expect to see in the envelope section. The envelope segment must
     * contain an envelope sender address.
     * 
     * If the segment contains a recipient address, include the optional
     * always_bcc recipient.
     */
    if ((status = copy_segment(qfile, cleanup, info, buf, REC_TYPE_ENVELOPE)) != 0)
	return (status);
    if (info->sender == 0) {
	msg_warn("%s: uid=%ld: no envelope sender",
		 info->id, (long) info->st.st_uid);
	return (REMOVE_MESSAGE_FILE);
    }
    msg_info("%s: uid=%d from=<%s>", info->id,
	     (int) info->st.st_uid, info->sender);

    if (info->rcpt) {
	if (*var_always_bcc)
	    rec_fputs(cleanup, REC_TYPE_RCPT, var_always_bcc);
    }

    /*
     * Message content segment. Send a dummy message length. Prepend a
     * Received: header to the message contents. For tracing purposes,
     * include the message file ownership, without revealing the login name.
     */
    rec_fputs(cleanup, REC_TYPE_MESG, "");
    rec_fprintf(cleanup, REC_TYPE_NORM, "Received: by %s (%s, from userid %ld)",
		var_myhostname, var_mail_name, (long) info->st.st_uid);
    rec_fprintf(cleanup, REC_TYPE_NORM, "\tid %s; %s", info->id,
		mail_date(info->st.st_mtime));

    /*
     * Copy the message content segment. Allow only those records that we
     * expect to see in the message content section.
     */
    if ((status = copy_segment(qfile, cleanup, info, buf, REC_TYPE_CONTENT)) != 0)
	return (status);

    /*
     * Send the segment with information extracted from message headers.
     * Permit a non-empty extracted segment, so that list manager software
     * can to output recipients after the message, and so that sysadmins can
     * re-inject messages after a change of configuration.
     */
    rec_fputs(cleanup, REC_TYPE_XTRA, "");
    if ((status = copy_segment(qfile, cleanup, info, buf, REC_TYPE_EXTRACT)) != 0)
	return (status);

    /*
     * There are no errors. Send the end-of-data marker, and get the cleanup
     * service completion status. XXX Since the pickup service is unable to
     * bounce, the cleanup service can report only soft errors here.
     */
    rec_fputs(cleanup, REC_TYPE_END, "");
    if (attr_scan(cleanup, ATTR_FLAG_MISSING,
		  ATTR_TYPE_NUM, MAIL_ATTR_STATUS, &status,
		  ATTR_TYPE_END) != 1)
	return (cleanup_service_error(info, CLEANUP_STAT_WRITE));

    /*
     * Depending on the cleanup service completion status, delete the message
     * file, or try again later. Bounces are dealt with by the cleanup
     * service itself. The master process wakes up the cleanup service every
     * now and then.
     */
    if (status) {
	return (cleanup_service_error(info, status));
    } else {
	return (REMOVE_MESSAGE_FILE);
    }
}

/* pickup_file - initialize for file copy and cleanup */

static int pickup_file(PICKUP_INFO *info)
{
    VSTRING *buf = vstring_alloc(100);
    int     status;
    VSTREAM *qfile;
    VSTREAM *cleanup;
    int     fd;

    /*
     * Open the submitted file. If we cannot open it, and we're not having a
     * file descriptor leak problem, delete the submitted file, so that we
     * won't keep complaining about the same file again and again. XXX
     * Perhaps we should save "bad" files elsewhere for further inspection.
     * XXX How can we delete a file when open() fails with ENOENT?
     */
    qfile = safe_open(info->path, O_RDONLY | O_NONBLOCK, 0, 
		(struct stat *) 0, -1, -1, buf);
    if (qfile == 0) {
	if (errno != ENOENT)
	    msg_warn("open input file %s: %s", info->path, vstring_str(buf));
	vstring_free(buf);
	return (errno == EACCES ? KEEP_MESSAGE_FILE : REMOVE_MESSAGE_FILE);
    }

    /*
     * Contact the cleanup service and read the queue ID that it has
     * allocated. In case of trouble, request that the cleanup service
     * bounces its copy of the message. because the original input file is
     * not readable by the bounce service.
     * 
     * The actual message copying code is in a separate routine, so that it is
     * easier to implement the many possible error exits without forgetting
     * to close files, or to release memory.
     */
#define PICKUP_CLEANUP_FLAGS	(CLEANUP_FLAG_BOUNCE | CLEANUP_FLAG_FILTER)

    cleanup = mail_connect_wait(MAIL_CLASS_PUBLIC, MAIL_SERVICE_CLEANUP);
    if (attr_scan(cleanup, ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, buf,
		  ATTR_TYPE_END) != 1
	|| attr_print(cleanup, ATTR_FLAG_NONE,
		      ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, PICKUP_CLEANUP_FLAGS,
		      ATTR_TYPE_END) != 0) {
	status = KEEP_MESSAGE_FILE;
    } else {
	info->id = mystrdup(vstring_str(buf));
	status = pickup_copy(qfile, cleanup, info, buf);
    }
    vstream_fclose(qfile);
    vstream_fclose(cleanup);
    vstring_free(buf);
    return (status);
}

/* pickup_init - init info structure */

static void pickup_init(PICKUP_INFO *info)
{
    info->id = 0;
    info->path = 0;
    info->sender = 0;
    info->rcpt = 0;
}

/* pickup_free - wipe info structure */

static void pickup_free(PICKUP_INFO *info)
{
#define SAFE_FREE(x) { if (x) myfree(x); }

    SAFE_FREE(info->id);
    SAFE_FREE(info->path);
    SAFE_FREE(info->sender);
    SAFE_FREE(info->rcpt);
}

/* pickup_service - service client */

static void pickup_service(char *unused_buf, int unused_len,
			           char *unused_service, char **argv)
{
    SCAN_DIR *scan;
    char   *queue_name;
    PICKUP_INFO info;
    const char *path;
    char   *id;
    int     file_count;

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * Skip over things that we don't want to open, such as files that are
     * still being written, or garbage. Leave it up to the sysadmin to remove
     * garbage. Keep scanning the queue directory until we stop removing
     * files from it.
     */
    queue_name = MAIL_QUEUE_MAILDROP;		/* XXX should be a list */
    do {
	file_count = 0;
	scan = scan_dir_open(queue_name);
	while ((id = scan_dir_next(scan)) != 0) {
	    if (mail_open_ok(queue_name, id, &info.st, &path) == MAIL_OPEN_YES) {
		pickup_init(&info);
		info.path = mystrdup(path);
		if (pickup_file(&info) == REMOVE_MESSAGE_FILE) {
		    if (REMOVE(info.path))
			msg_warn("remove %s: %m", info.path);
		    else
			file_count++;
		}
		pickup_free(&info);
	    }
	}
	scan_dir_close(scan);
    } while (file_count);
}

/* drop_privileges - drop privileges */

static void drop_privileges(char *unused_name, char **unused_argv)
{

    /*
     * In case master.cf was not updated for unprivileged service.
     */
    if (getuid() != var_owner_uid)
	set_ugid(var_owner_uid, var_owner_gid);
}

/* main - pass control to the multi-threaded server skeleton */

int     main(int argc, char **argv)
{
    static CONFIG_STR_TABLE str_table[] = {
	VAR_ALWAYS_BCC, DEF_ALWAYS_BCC, &var_always_bcc, 0, 0,
	VAR_FILTER_XPORT, DEF_FILTER_XPORT, &var_filter_xport, 0, 0,
	0,
    };

    /*
     * Use the multi-threaded skeleton, because no-one else should be
     * monitoring our service socket while this process runs.
     */
    trigger_server_main(argc, argv, pickup_service,
			MAIL_SERVER_STR_TABLE, str_table,
			MAIL_SERVER_POST_INIT, drop_privileges,
			0);
}
