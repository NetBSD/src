/*	$NetBSD: showq.c,v 1.4.2.1 2023/12/25 12:43:34 martin Exp $	*/

/*++
/* NAME
/*	showq 8
/* SUMMARY
/*	list the Postfix mail queue
/* SYNOPSIS
/*	\fBshowq\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBshowq\fR(8) daemon reports the Postfix mail queue status.
/*	The output is meant to be formatted by the postqueue(1) command,
/*	as it emulates the Sendmail `mailq' command.
/*
/*	The \fBshowq\fR(8) daemon can also be run in stand-alone mode
/*	by the superuser. This mode of operation is used to emulate
/*	the `mailq' command while the Postfix mail system is down.
/* SECURITY
/* .ad
/* .fi
/*	The \fBshowq\fR(8) daemon can run in a chroot jail at fixed low
/*	privilege, and takes no input from the client. Its service port
/*	is accessible to local untrusted users, so the service can be
/*	susceptible to denial of service attacks.
/* STANDARDS
/* .ad
/* .fi
/*	None. The \fBshowq\fR(8) daemon does not interact with the
/*	outside world.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8)
/*	or \fBpostlogd\fR(8).
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are picked up automatically as \fBshowq\fR(8)
/*	processes run for only a limited amount of time. Use the command
/*	"\fBpostfix reload\fR" to speed up a change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdaemon_timeout (18000s)\fR"
/*	How much time a Postfix daemon process may take to handle a
/*	request before it is terminated by a built-in watchdog timer.
/* .IP "\fBduplicate_filter_limit (1000)\fR"
/*	The maximal number of addresses remembered by the address
/*	duplicate filter for \fBaliases\fR(5) or \fBvirtual\fR(5) alias expansion, or
/*	for \fBshowq\fR(8) queue displays.
/* .IP "\fBempty_address_recipient (MAILER-DAEMON)\fR"
/*	The recipient of mail addressed to the null address.
/* .IP "\fBipc_timeout (3600s)\fR"
/*	The time limit for sending or receiving information over an internal
/*	communication channel.
/* .IP "\fBmax_idle (100s)\fR"
/*	The maximum amount of time that an idle Postfix daemon process waits
/*	for an incoming connection before terminating voluntarily.
/* .IP "\fBmax_use (100)\fR"
/*	The maximal number of incoming connections that a Postfix daemon
/*	process will service before terminating voluntarily.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	A prefix that is prepended to the process name in syslog
/*	records, so that, for example, "smtpd" becomes "prefix/smtpd".
/* .PP
/*	Available in Postfix version 2.9 and later:
/* .IP "\fBenable_long_queue_ids (no)\fR"
/*	Enable long, non-repeating, queue IDs (queue file names).
/* .PP
/*	Available in Postfix 3.3 and later:
/* .IP "\fBservice_name (read-only)\fR"
/*	The master.cf service name of a Postfix daemon process.
/* FILES
/*	/var/spool/postfix, queue directories
/* SEE ALSO
/*	pickup(8), local mail pickup service
/*	cleanup(8), canonicalize and enqueue mail
/*	qmgr(8), queue manager
/*	postconf(5), configuration parameters
/*	master(8), process manager
/*	postlogd(8), Postfix logging
/*	syslogd(8), system logging
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

/* Utility library. */

#include <msg.h>
#include <scan_dir.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <stringops.h>
#include <mymalloc.h>
#include <htable.h>

/* Global library. */

#include <mail_queue.h>
#include <mail_open_ok.h>
#include <mail_proto.h>
#include <mail_date.h>
#include <mail_params.h>
#include <mail_version.h>
#include <mail_scan_dir.h>
#include <mail_conf.h>
#include <record.h>
#include <rec_type.h>
#include <quote_822_local.h>
#include <mail_addr.h>
#include <bounce_log.h>

/* Single-threaded server skeleton. */

#include <mail_server.h>

/* Application-specific. */

int     var_dup_filter_limit;
char   *var_empty_addr;

static void showq_reasons(VSTREAM *, BOUNCE_LOG *, RCPT_BUF *, DSN_BUF *,
			          HTABLE *);

#define STR(x)	vstring_str(x)

/* showq_report - report status of sender and recipients */

static void showq_report(VSTREAM *client, char *queue, char *id,
			         VSTREAM *qfile, long size, time_t mtime,
			         mode_t mode)
{
    VSTRING *buf = vstring_alloc(100);
    VSTRING *printable_quoted_addr = vstring_alloc(100);
    int     rec_type;
    time_t  arrival_time = 0;
    char   *start;
    long    msg_size = size;
    BOUNCE_LOG *logfile;
    HTABLE *dup_filter = 0;
    RCPT_BUF *rcpt_buf = 0;
    DSN_BUF *dsn_buf = 0;
    int     sender_seen = 0;
    int     msg_size_ok = 0;

    /*
     * Let the optimizer worry about eliminating duplicate code.
     */
#define SHOWQ_CLEANUP_AND_RETURN { \
	if (sender_seen > 0) \
	    attr_print(client, ATTR_FLAG_NONE, ATTR_TYPE_END); \
	vstring_free(buf); \
	vstring_free(printable_quoted_addr); \
	if (rcpt_buf) \
	    rcpb_free(rcpt_buf); \
	if (dsn_buf) \
	    dsb_free(dsn_buf); \
	if (dup_filter) \
	    htable_free(dup_filter, (void (*) (void *)) 0); \
    }

    /*
     * XXX addresses in defer logfiles are in printable quoted form, while
     * addresses in message envelope records are in raw unquoted form. This
     * may change once we replace the present ad-hoc bounce/defer logfile
     * format by one that is transparent for control etc. characters. See
     * also: bounce/bounce_append_service.c.
     * 
     * XXX With Postfix <= 2.0, "postsuper -r" results in obsolete size records
     * from previous cleanup runs. Skip the obsolete size records.
     */
    while (!vstream_ferror(client) && (rec_type = rec_get(qfile, buf, 0)) > 0) {
	start = vstring_str(buf);
	if (msg_verbose)
	    msg_info("record %c %s", rec_type, printable(start, '?'));
	switch (rec_type) {
	case REC_TYPE_TIME:
	    /* TODO: parse seconds and microseconds. */
	    if (arrival_time == 0)
		arrival_time = atol(start);
	    break;
	case REC_TYPE_SIZE:
	    if (msg_size_ok == 0) {
		msg_size_ok = (start[strspn(start, "0123456789 ")] == 0
			       && (msg_size = atol(start)) >= 0);
		if (msg_size_ok == 0) {
		    msg_warn("%s: malformed size record: %.100s "
			     "-- using file size instead",
			     id, printable(start, '?'));
		    msg_size = size;
		}
	    }
	    break;
	case REC_TYPE_FROM:
	    if (*start == 0)
		start = var_empty_addr;
	    quote_822_local(printable_quoted_addr, start);
	    /* For consistency with REC_TYPE_RCPT below. */
	    printable(STR(printable_quoted_addr), '?');
	    if (sender_seen++ > 0) {
		msg_warn("%s: duplicate sender address: %s "
			 "-- skipping remainder of this file",
			 id, STR(printable_quoted_addr));
		SHOWQ_CLEANUP_AND_RETURN;
	    }
	    attr_print(client, ATTR_FLAG_MORE,
		       SEND_ATTR_STR(MAIL_ATTR_QUEUE, queue),
		       SEND_ATTR_STR(MAIL_ATTR_QUEUEID, id),
		       SEND_ATTR_LONG(MAIL_ATTR_TIME, arrival_time > 0 ?
				      arrival_time : mtime),
		       SEND_ATTR_LONG(MAIL_ATTR_SIZE, msg_size),
		       SEND_ATTR_INT(MAIL_ATTR_FORCED_EXPIRE,
				     (mode & MAIL_QUEUE_STAT_EXPIRE) != 0),
		       SEND_ATTR_STR(MAIL_ATTR_SENDER,
				     STR(printable_quoted_addr)),
		       ATTR_TYPE_END);
	    break;
	case REC_TYPE_RCPT:
	    if (sender_seen == 0) {
		msg_warn("%s: missing sender address: %s "
			 "-- skipping remainder of this file",
			 id, STR(printable_quoted_addr));
		SHOWQ_CLEANUP_AND_RETURN;
	    }
	    if (*start == 0)			/* can't happen? */
		start = var_empty_addr;
	    quote_822_local(printable_quoted_addr, start);
	    /* For consistency with recipients in bounce logfile. */
	    printable(STR(printable_quoted_addr), '?');
	    if (dup_filter == 0
	      || htable_locate(dup_filter, STR(printable_quoted_addr)) == 0)
		attr_print(client, ATTR_FLAG_MORE,
			   SEND_ATTR_STR(MAIL_ATTR_RECIP,
					 STR(printable_quoted_addr)),
			   SEND_ATTR_STR(MAIL_ATTR_WHY, ""),
			   ATTR_TYPE_END);
	    break;
	case REC_TYPE_MESG:
	    if (msg_size_ok && vstream_fseek(qfile, msg_size, SEEK_CUR) < 0)
		msg_fatal("seek file %s: %m", VSTREAM_PATH(qfile));
	    break;
	case REC_TYPE_END:
	    break;
	}

	/*
	 * Before listing any recipients from the queue file, try to list
	 * recipients from the corresponding defer logfile with per-recipient
	 * descriptions why delivery was deferred.
	 * 
	 * The defer logfile is not necessarily complete: delivery may be
	 * interrupted (postfix stop or reload) before all recipients have
	 * been tried.
	 * 
	 * Therefore we keep a record of recipients found in the defer logfile,
	 * and try to avoid listing those recipients again when processing
	 * recipients from the queue file.
	 */
	if (rec_type == REC_TYPE_FROM
	    && (logfile = bounce_log_open(MAIL_QUEUE_DEFER, id, O_RDONLY, 0)) != 0) {
	    if (dup_filter != 0)
		msg_panic("showq_report: attempt to reuse duplicate filter");
	    dup_filter = htable_create(var_dup_filter_limit);
	    if (rcpt_buf == 0)
		rcpt_buf = rcpb_create();
	    if (dsn_buf == 0)
		dsn_buf = dsb_create();
	    showq_reasons(client, logfile, rcpt_buf, dsn_buf, dup_filter);
	    if (bounce_log_close(logfile))
		msg_warn("close %s %s: %m", MAIL_QUEUE_DEFER, id);
	}
    }
    SHOWQ_CLEANUP_AND_RETURN;
}

/* showq_reasons - show deferral reasons */

static void showq_reasons(VSTREAM *client, BOUNCE_LOG *bp, RCPT_BUF *rcpt_buf,
			          DSN_BUF *dsn_buf, HTABLE *dup_filter)
{
    RECIPIENT *rcpt = &rcpt_buf->rcpt;
    DSN    *dsn = &dsn_buf->dsn;

    while (bounce_log_read(bp, rcpt_buf, dsn_buf) != 0) {

	/*
	 * Update the duplicate filter.
	 */
	if (var_dup_filter_limit == 0
	    || dup_filter->used < var_dup_filter_limit)
	    if (htable_locate(dup_filter, rcpt->address) == 0)
		htable_enter(dup_filter, rcpt->address, (void *) 0);

	attr_print(client, ATTR_FLAG_MORE,
		   SEND_ATTR_STR(MAIL_ATTR_RECIP, rcpt->address),
		   SEND_ATTR_STR(MAIL_ATTR_WHY, dsn->reason),
		   ATTR_TYPE_END);
    }
}


/* showq_service - service client */

static void showq_service(VSTREAM *client, char *unused_service, char **argv)
{
    VSTREAM *qfile;
    const char *path;
    int     status;
    char   *id;
    struct stat st;
    struct queue_info {
	char   *name;			/* queue name */
	char   *(*scan_next) (SCAN_DIR *);	/* flat or recursive */
    };
    struct queue_info *qp;

    static struct queue_info queue_info[] = {
	MAIL_QUEUE_MAILDROP, scan_dir_next,
	MAIL_QUEUE_ACTIVE, mail_scan_dir_next,
	MAIL_QUEUE_INCOMING, mail_scan_dir_next,
	MAIL_QUEUE_DEFERRED, mail_scan_dir_next,
	MAIL_QUEUE_HOLD, mail_scan_dir_next,
	0,
    };

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * Protocol identification.
     */
    (void) attr_print(client, ATTR_FLAG_NONE,
		      SEND_ATTR_STR(MAIL_ATTR_PROTO, MAIL_ATTR_PROTO_SHOWQ),
		      ATTR_TYPE_END);

    /*
     * Skip any files that have the wrong permissions. If we can't open an
     * existing file, assume the system is out of resources or that it is
     * mis-configured, and force backoff by raising a fatal error.
     */
    for (qp = queue_info; qp->name != 0; qp++) {
	SCAN_DIR *scan = scan_dir_open(qp->name);
	char   *saved_id = 0;

	while ((id = qp->scan_next(scan)) != 0) {

	    /*
	     * XXX I have seen showq loop on the same queue id. That would be
	     * an operating system bug, but who cares whose fault it is. Make
	     * sure this will never happen again.
	     */
	    if (saved_id) {
		if (strcmp(saved_id, id) == 0) {
		    msg_warn("readdir loop on queue %s id %s", qp->name, id);
		    break;
		}
		myfree(saved_id);
	    }
	    saved_id = mystrdup(id);
	    status = mail_open_ok(qp->name, id, &st, &path);
	    if (status == MAIL_OPEN_YES) {
		if ((qfile = mail_queue_open(qp->name, id, O_RDONLY, 0)) != 0) {
		    showq_report(client, qp->name, id, qfile, (long) st.st_size,
				 st.st_mtime, st.st_mode);
		    if (vstream_fclose(qfile))
			msg_warn("close file %s %s: %m", qp->name, id);
		} else if (errno != ENOENT) {
		    msg_warn("open %s %s: %m", qp->name, id);
		}
	    }
	    vstream_fflush(client);
	}
	if (saved_id)
	    myfree(saved_id);
	scan_dir_close(scan);
    }
    attr_print(client, ATTR_FLAG_NONE, ATTR_TYPE_END);
}

MAIL_VERSION_STAMP_DECLARE;

/* main - pass control to the single-threaded server skeleton */

int     main(int argc, char **argv)
{
    static const CONFIG_INT_TABLE int_table[] = {
	VAR_DUP_FILTER_LIMIT, DEF_DUP_FILTER_LIMIT, &var_dup_filter_limit, 0, 0,
	0,
    };
    CONFIG_STR_TABLE str_table[] = {
	VAR_EMPTY_ADDR, DEF_EMPTY_ADDR, &var_empty_addr, 1, 0,
	0,
    };

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    single_server_main(argc, argv, showq_service,
		       CA_MAIL_SERVER_INT_TABLE(int_table),
		       CA_MAIL_SERVER_STR_TABLE(str_table),
		       0);
}
