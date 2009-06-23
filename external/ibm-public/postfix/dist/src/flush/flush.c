/*	$NetBSD: flush.c,v 1.1.1.1 2009/06/23 10:08:44 tron Exp $	*/

/*++
/* NAME
/*	flush 8
/* SUMMARY
/*	Postfix fast flush server
/* SYNOPSIS
/*	\fBflush\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBflush\fR(8) server maintains a record of deferred
/*	mail by destination.
/*	This information is used to improve the performance of the SMTP
/*	\fBETRN\fR request, and of its command-line equivalent,
/*	"\fBsendmail -qR\fR" or "\fBpostqueue -f\fR".
/*	This program expects to be run from the \fBmaster\fR(8) process
/*	manager.
/*
/*	The record is implemented as a per-destination logfile with
/*	as contents the queue IDs of deferred mail. A logfile is
/*	append-only, and is truncated when delivery is requested
/*	for the corresponding destination. A destination is the
/*	part on the right-hand side of the right-most \fB@\fR in
/*	an email address.
/*
/*	Per-destination logfiles of deferred mail are maintained only for
/*	eligible destinations. The list of eligible destinations is
/*	specified with the \fBfast_flush_domains\fR configuration parameter,
/*	which defaults to \fB$relay_domains\fR.
/*
/*	This server implements the following requests:
/* .IP "\fBadd\fI sitename queueid\fR"
/*	Inform the \fBflush\fR(8) server that the message with the specified
/*	queue ID is queued for the specified destination.
/* .IP "\fBsend_site\fI sitename\fR"
/*	Request delivery of mail that is queued for the specified
/*	destination.
/* .IP "\fBsend_file\fI queueid\fR"
/*	Request delivery of the specified deferred message.
/* .IP \fBrefresh\fR
/*	Refresh non-empty per-destination logfiles that were not read in
/*	\fB$fast_flush_refresh_time\fR hours, by simulating
/*	send requests (see above) for the corresponding destinations.
/* .sp
/*	Delete empty per-destination logfiles that were not updated in
/*	\fB$fast_flush_purge_time\fR days.
/* .sp
/*	This request completes in the background.
/* .IP \fBpurge\fR
/*	Do a \fBrefresh\fR for all per-destination logfiles.
/* SECURITY
/* .ad
/* .fi
/*	The \fBflush\fR(8) server is not security-sensitive. It does not
/*	talk to the network, and it does not talk to local users.
/*	The fast flush server can run chrooted at fixed low privilege.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* BUGS
/*	Fast flush logfiles are truncated only after a "send"
/*	request, not when mail is actually delivered, and therefore can
/*	accumulate outdated or redundant data. In order to maintain sanity,
/*	"refresh" must be executed periodically. This can
/*	be automated with a suitable wakeup timer setting in the
/*	\fBmaster.cf\fR configuration file.
/*
/*	Upon receipt of a request to deliver mail for an eligible
/*	destination, the \fBflush\fR(8) server requests delivery of all messages
/*	that are listed in that destination's logfile, regardless of the
/*	recipients of those messages. This is not an issue for mail
/*	that is sent to a \fBrelay_domains\fR destination because
/*	such mail typically only has recipients in one domain.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are picked up automatically as \fBflush\fR(8)
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
/* .IP "\fBfast_flush_domains ($relay_domains)\fR"
/*	Optional list of destinations that are eligible for per-destination
/*	logfiles with mail that is queued to those destinations.
/* .IP "\fBfast_flush_refresh_time (12h)\fR"
/*	The time after which a non-empty but unread per-destination "fast
/*	flush" logfile needs to be refreshed.
/* .IP "\fBfast_flush_purge_time (7d)\fR"
/*	The time after which an empty per-destination "fast flush" logfile
/*	is deleted.
/* .IP "\fBipc_timeout (3600s)\fR"
/*	The time limit for sending or receiving information over an internal
/*	communication channel.
/* .IP "\fBmax_idle (100s)\fR"
/*	The maximum amount of time that an idle Postfix daemon process waits
/*	for an incoming connection before terminating voluntarily.
/* .IP "\fBmax_use (100)\fR"
/*	The maximal number of incoming connections that a Postfix daemon
/*	process will service before terminating voluntarily.
/* .IP "\fBparent_domain_matches_subdomains (see 'postconf -d' output)\fR"
/*	What Postfix features match subdomains of "domain.tld" automatically,
/*	instead of requiring an explicit ".domain.tld" pattern.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* FILES
/*	/var/spool/postfix/flush, "fast flush" logfiles.
/* SEE ALSO
/*	smtpd(8), SMTP server
/*	qmgr(8), queue manager
/*	postconf(5), configuration parameters
/*	master(5), generic daemon options
/*	master(8), process manager
/*	syslogd(8), system logging
/* README FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or
/*	"\fBpostconf html_directory\fR" to locate this information.
/* .na
/* .nf
/*	ETRN_README, Postfix ETRN howto
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* HISTORY
/*	This service was introduced with Postfix version 1.0.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <utime.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <events.h>
#include <vstream.h>
#include <vstring.h>
#include <vstring_vstream.h>
#include <myflock.h>
#include <htable.h>
#include <dict.h>
#include <scan_dir.h>
#include <stringops.h>
#include <safe_open.h>

/* Global library. */

#include <mail_params.h>
#include <mail_version.h>
#include <mail_queue.h>
#include <mail_proto.h>
#include <mail_flush.h>
#include <flush_clnt.h>
#include <mail_conf.h>
#include <mail_scan_dir.h>
#include <maps.h>
#include <domain_list.h>
#include <match_parent_style.h>

/* Single server skeleton. */

#include <mail_server.h>

/* Application-specific. */

 /*
  * Tunable parameters. The fast_flush_domains parameter is not defined here,
  * because it is also used by the global library, and therefore is owned by
  * the library.
  */
int     var_fflush_refresh;
int     var_fflush_purge;

 /*
  * Flush policy stuff.
  */
static DOMAIN_LIST *flush_domains;

 /*
  * Some hard-wired policy: how many queue IDs we remember while we're
  * flushing a logfile (duplicate elimination). Sites with 1000+ emails
  * queued should arrange for permanent connectivity.
  */
#define FLUSH_DUP_FILTER_SIZE	10000	/* graceful degradation */

 /*
  * Silly little macros.
  */
#define STR(x)			vstring_str(x)
#define STREQ(x,y)		((x) == (y) || strcmp(x,y) == 0)

 /*
  * Forward declarations resulting from breaking up routines according to
  * name space: domain names versus safe-to-use pathnames.
  */
static int flush_add_path(const char *, const char *);
static int flush_send_path(const char *, int);

 /*
  * Do we only refresh the per-destination logfile, or do we really request
  * mail delivery as if someone sent ETRN? If the latter, we must override
  * information about unavailable hosts or unavailable transports.
  * 
  * When selectively flushing deferred mail, we need to override the queue
  * manager's "dead destination" information and unthrottle transports and
  * queues. There are two options:
  * 
  * - Unthrottle all transports and queues before we move mail to the incoming
  * queue. This is less accurate, but has the advantage when flushing lots of
  * mail, because Postfix can skip delivery of flushed messages after it
  * discovers that a destination is (still) unavailable.
  * 
  * - Unthrottle some transports and queues after the queue manager moves mail
  * to the active queue. This is more accurate, but has the disadvantage when
  * flushing lots of mail, because Postfix cannot skip delivery of flushed
  * messages after it discovers that a destination is (still) unavailable.
  */
#define REFRESH_ONLY		0
#define UNTHROTTLE_BEFORE	(1<<0)
#define UNTHROTTLE_AFTER	(1<<1)

/* flush_site_to_path - convert domain or [addr] to harmless string */

static VSTRING *flush_site_to_path(VSTRING *path, const char *site)
{
    const char *ptr;
    int     ch;

    /*
     * Allocate buffer on the fly; caller still needs to clean up.
     */
    if (path == 0)
	path = vstring_alloc(10);

    /*
     * Mask characters that could upset the name-to-queue-file mapping code.
     */
    for (ptr = site; (ch = *(unsigned const char *) ptr) != 0; ptr++)
	if (ISALNUM(ch))
	    VSTRING_ADDCH(path, ch);
	else
	    VSTRING_ADDCH(path, '_');
    VSTRING_TERMINATE(path);

    if (msg_verbose)
	msg_info("site %s to path %s", site, STR(path));

    return (path);
}

/* flush_policy_ok - check logging policy */

static int flush_policy_ok(const char *site)
{
    return (domain_list_match(flush_domains, site));
}

/* flush_add_service - append queue ID to per-site fast flush logfile */

static int flush_add_service(const char *site, const char *queue_id)
{
    const char *myname = "flush_add_service";
    VSTRING *site_path;
    int     status;

    if (msg_verbose)
	msg_info("%s: site %s queue_id %s", myname, site, queue_id);

    /*
     * If this site is not eligible for logging, deny the request.
     */
    if (flush_policy_ok(site) == 0)
	return (FLUSH_STAT_DENY);

    /*
     * Map site to path and update log.
     */
    site_path = flush_site_to_path((VSTRING *) 0, site);
    status = flush_add_path(STR(site_path), queue_id);
    vstring_free(site_path);

    return (status);
}

/* flush_add_path - add record to log */

static int flush_add_path(const char *path, const char *queue_id)
{
    const char *myname = "flush_add_path";
    VSTREAM *log;

    /*
     * Sanity check.
     */
    if (!mail_queue_id_ok(path))
	return (FLUSH_STAT_BAD);

    /*
     * Open the logfile or bust.
     */
    if ((log = mail_queue_open(MAIL_QUEUE_FLUSH, path,
			       O_CREAT | O_APPEND | O_WRONLY, 0600)) == 0)
	msg_fatal("%s: open fast flush logfile %s: %m", myname, path);

    /*
     * We must lock the logfile, so that we don't lose information due to
     * concurrent access. If the lock takes too long, the Postfix watchdog
     * will eventually take care of the problem, but it will take a while.
     */
    if (myflock(vstream_fileno(log), INTERNAL_LOCK, MYFLOCK_OP_EXCLUSIVE) < 0)
	msg_fatal("%s: lock fast flush logfile %s: %m", myname, path);

    /*
     * Append the queue ID. With 15 bits of microsecond time, a queue ID is
     * not recycled often enough for false hits to be a problem. If it does,
     * then we could add other signature information, such as the file size
     * in bytes.
     */
    vstream_fprintf(log, "%s\n", queue_id);
    if (vstream_fflush(log))
	msg_warn("write fast flush logfile %s: %m", path);

    /*
     * Clean up.
     */
    if (myflock(vstream_fileno(log), INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock fast flush logfile %s: %m", myname, path);
    if (vstream_fclose(log) != 0)
	msg_warn("write fast flush logfile %s: %m", path);

    return (FLUSH_STAT_OK);
}

/* flush_send_service - flush mail queued for site */

static int flush_send_service(const char *site, int how)
{
    const char *myname = "flush_send_service";
    VSTRING *site_path;
    int     status;

    if (msg_verbose)
	msg_info("%s: site %s", myname, site);

    /*
     * If this site is not eligible for logging, deny the request.
     */
    if (flush_policy_ok(site) == 0)
	return (FLUSH_STAT_DENY);

    /*
     * Map site name to path name and flush the log.
     */
    site_path = flush_site_to_path((VSTRING *) 0, site);
    status = flush_send_path(STR(site_path), how);
    vstring_free(site_path);

    return (status);
}

/* flush_one_file - move one queue file to incoming queue */

static int flush_one_file(const char *queue_id, VSTRING *queue_file,
			          struct utimbuf * tbuf, int how)
{
    const char *myname = "flush_one_file";
    const char *queue_name;
    const char *path;

    /*
     * Some other instance of this program may flush some logfile and may
     * just have moved this queue file to the incoming queue.
     */
    for (queue_name = MAIL_QUEUE_DEFERRED; /* see below */ ;
	 queue_name = MAIL_QUEUE_INCOMING) {
	path = mail_queue_path(queue_file, queue_name, queue_id);
	if (utime(path, tbuf) == 0)
	    break;
	if (errno != ENOENT)
	    msg_warn("%s: update %s time stamps: %m", myname, path);
	if (STREQ(queue_name, MAIL_QUEUE_INCOMING))
	    return (0);
    }

    /*
     * With the UNTHROTTLE_AFTER strategy, we leave it up to the queue
     * manager to unthrottle transports and queues as it reads recipients
     * from a queue file. We request this unthrottle operation by setting the
     * group read permission bit.
     * 
     * Note: we must avoid using chmod(). It is not only slower than fchmod()
     * but it is also less secure. With chmod(), an attacker could repeatedly
     * send requests to the flush server and trick it into changing
     * permissions of non-queue files, by exploiting a race condition.
     * 
     * We use safe_open() because we don't validate the file content before
     * modifying the file status.
     */
    if (how & UNTHROTTLE_AFTER) {
	VSTRING *why;
	struct stat st;
	VSTREAM *fp;

	for (why = vstring_alloc(1); /* see below */ ;
	     queue_name = MAIL_QUEUE_INCOMING,
	     path = mail_queue_path(queue_file, queue_name, queue_id)) {
	    if ((fp = safe_open(path, O_RDWR, 0, &st, -1, -1, why)) != 0)
		break;
	    if (errno != ENOENT)
		msg_warn("%s: open %s: %s", myname, path, STR(why));
	    if (errno != ENOENT || STREQ(queue_name, MAIL_QUEUE_INCOMING)) {
		vstring_free(why);
		return (0);
	    }
	}
	vstring_free(why);
	if ((st.st_mode & MAIL_QUEUE_STAT_READY) != MAIL_QUEUE_STAT_READY) {
	    (void) vstream_fclose(fp);
	    return (0);
	}
	if (fchmod(vstream_fileno(fp), st.st_mode | MAIL_QUEUE_STAT_UNTHROTTLE) < 0)
	    msg_warn("%s: fchmod %s: %m", myname, path);
	(void) vstream_fclose(fp);
    }

    /*
     * Move the file to the incoming queue, if it isn't already there.
     */
    if (STREQ(queue_name, MAIL_QUEUE_INCOMING) == 0
	&& mail_queue_rename(queue_id, queue_name, MAIL_QUEUE_INCOMING) < 0
	&& errno != ENOENT)
	msg_warn("%s: rename from %s to %s: %m",
		 path, queue_name, MAIL_QUEUE_INCOMING);

    /*
     * If we got here, we achieved something, so let's claim succes.
     */
    return (1);
}

/* flush_send_path - flush logfile file */

static int flush_send_path(const char *path, int how)
{
    const char *myname = "flush_send_path";
    VSTRING *queue_id;
    VSTRING *queue_file;
    VSTREAM *log;
    struct utimbuf tbuf;
    static char qmgr_flush_trigger[] = {
	QMGR_REQ_FLUSH_DEAD,		/* flush dead site/transport cache */
    };
    static char qmgr_scan_trigger[] = {
	QMGR_REQ_SCAN_INCOMING,		/* scan incoming queue */
    };
    HTABLE *dup_filter;
    int     count;

    /*
     * Sanity check.
     */
    if (!mail_queue_id_ok(path))
	return (FLUSH_STAT_BAD);

    /*
     * Open the logfile. If the file does not exist, then there is no queued
     * mail for this destination.
     */
    if ((log = mail_queue_open(MAIL_QUEUE_FLUSH, path, O_RDWR, 0600)) == 0) {
	if (errno != ENOENT)
	    msg_fatal("%s: open fast flush logfile %s: %m", myname, path);
	return (FLUSH_STAT_OK);
    }

    /*
     * We must lock the logfile, so that we don't lose information when it is
     * truncated. Unfortunately, this means that the file can be locked for a
     * significant amount of time. If things really get stuck the Postfix
     * watchdog will take care of it.
     */
    if (myflock(vstream_fileno(log), INTERNAL_LOCK, MYFLOCK_OP_EXCLUSIVE) < 0)
	msg_fatal("%s: lock fast flush logfile %s: %m", myname, path);

    /*
     * With the UNTHROTTLE_BEFORE strategy, we ask the queue manager to
     * unthrottle all transports and queues before we move a deferred queue
     * file to the incoming queue. This minimizes a race condition where the
     * queue manager seizes a queue file before it knows that we want to
     * flush that message.
     * 
     * This reduces the race condition time window to a very small amount (the
     * flush server does not really know when the queue manager reads its
     * command fifo). But there is a worse race, where the queue manager
     * moves a deferred queue file to the active queue before we have a
     * chance to expedite its delivery.
     */
    if (how & UNTHROTTLE_BEFORE)
	mail_trigger(MAIL_CLASS_PUBLIC, var_queue_service,
		     qmgr_flush_trigger, sizeof(qmgr_flush_trigger));

    /*
     * This is the part that dominates running time: schedule the listed
     * queue files for delivery by updating their file time stamps and by
     * moving them from the deferred queue to the incoming queue. This should
     * take no more than a couple seconds under normal conditions. Filter out
     * duplicate queue file names to avoid hammering the file system, with
     * some finite limit on the amount of memory that we are willing to
     * sacrifice for duplicate filtering. Graceful degradation.
     * 
     * By moving selected queue files from the deferred queue to the incoming
     * queue we optimize for the case where most deferred mail is for other
     * sites. If that assumption does not hold, i.e. all deferred mail is for
     * the same site, then doing a "fast flush" will cost more disk I/O than
     * a "slow flush" that delivers the entire deferred queue. This penalty
     * is only temporary - it will go away after we unite the active queue
     * and the incoming queue.
     */
    queue_id = vstring_alloc(10);
    queue_file = vstring_alloc(10);
    dup_filter = htable_create(10);
    tbuf.actime = tbuf.modtime = event_time();
    for (count = 0; vstring_get_nonl(queue_id, log) != VSTREAM_EOF; count++) {
	if (!mail_queue_id_ok(STR(queue_id))) {
	    msg_warn("bad queue id \"%.30s...\" in fast flush logfile %s",
		     STR(queue_id), path);
	    continue;
	}
	if (dup_filter->used >= FLUSH_DUP_FILTER_SIZE
	    || htable_find(dup_filter, STR(queue_id)) == 0) {
	    if (msg_verbose)
		msg_info("%s: logfile %s: update queue file %s time stamps",
			 myname, path, STR(queue_id));
	    if (dup_filter->used <= FLUSH_DUP_FILTER_SIZE)
		htable_enter(dup_filter, STR(queue_id), 0);
	    count += flush_one_file(STR(queue_id), queue_file, &tbuf, how);
	} else {
	    if (msg_verbose)
		msg_info("%s: logfile %s: skip queue file %s as duplicate",
			 myname, path, STR(queue_file));
	}
    }
    htable_free(dup_filter, (void (*) (char *)) 0);
    vstring_free(queue_file);
    vstring_free(queue_id);

    /*
     * Truncate the fast flush log.
     */
    if (count > 0 && ftruncate(vstream_fileno(log), (off_t) 0) < 0)
	msg_fatal("%s: truncate fast flush logfile %s: %m", myname, path);

    /*
     * Workaround for noatime mounts. Use futimes() if available.
     */
    (void) utimes(VSTREAM_PATH(log), (struct timeval *) 0);

    /*
     * Request delivery and clean up.
     */
    if (myflock(vstream_fileno(log), INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock fast flush logfile %s: %m", myname, path);
    if (vstream_fclose(log) != 0)
	msg_warn("%s: read fast flush logfile %s: %m", myname, path);
    if (count > 0) {
	if (msg_verbose)
	    msg_info("%s: requesting delivery for logfile %s", myname, path);
	mail_trigger(MAIL_CLASS_PUBLIC, var_queue_service,
		     qmgr_scan_trigger, sizeof(qmgr_scan_trigger));
    }
    return (FLUSH_STAT_OK);
}

/* flush_send_file_service - flush one queue file */

static int flush_send_file_service(const char *queue_id)
{
    const char *myname = "flush_send_file_service";
    VSTRING *queue_file;
    struct utimbuf tbuf;
    static char qmgr_scan_trigger[] = {
	QMGR_REQ_SCAN_INCOMING,		/* scan incoming queue */
    };

    /*
     * Sanity check.
     */
    if (!mail_queue_id_ok(queue_id))
	return (FLUSH_STAT_BAD);

    if (msg_verbose)
	msg_info("%s: requesting delivery for queue_id %s", myname, queue_id);

    queue_file = vstring_alloc(30);
    tbuf.actime = tbuf.modtime = event_time();
    if (flush_one_file(queue_id, queue_file, &tbuf, UNTHROTTLE_AFTER) > 0)
	mail_trigger(MAIL_CLASS_PUBLIC, var_queue_service,
		     qmgr_scan_trigger, sizeof(qmgr_scan_trigger));
    vstring_free(queue_file);

    return (FLUSH_STAT_OK);
}

/* flush_refresh_service - refresh logfiles beyond some age */

static int flush_refresh_service(int max_age)
{
    const char *myname = "flush_refresh_service";
    SCAN_DIR *scan;
    char   *site_path;
    struct stat st;
    VSTRING *path = vstring_alloc(10);

    scan = scan_dir_open(MAIL_QUEUE_FLUSH);
    while ((site_path = mail_scan_dir_next(scan)) != 0) {
	if (!mail_queue_id_ok(site_path))
	    continue;				/* XXX grumble. */
	mail_queue_path(path, MAIL_QUEUE_FLUSH, site_path);
	if (stat(STR(path), &st) < 0) {
	    if (errno != ENOENT)
		msg_warn("%s: stat %s: %m", myname, STR(path));
	    else if (msg_verbose)
		msg_info("%s: %s: %m", myname, STR(path));
	    continue;
	}
	if (st.st_size == 0) {
	    if (st.st_mtime + var_fflush_purge < event_time()) {
		if (unlink(STR(path)) < 0)
		    msg_warn("remove logfile %s: %m", STR(path));
		else if (msg_verbose)
		    msg_info("%s: unlink %s, empty and unchanged for %d days",
			     myname, STR(path), var_fflush_purge / 86400);
	    } else if (msg_verbose)
		msg_info("%s: skip logfile %s - empty log", myname, site_path);
	} else if (st.st_atime + max_age < event_time()) {
	    if (msg_verbose)
		msg_info("%s: flush logfile %s", myname, site_path);
	    flush_send_path(site_path, REFRESH_ONLY);
	} else {
	    if (msg_verbose)
		msg_info("%s: skip logfile %s, unread for <%d hours(s) ",
			 myname, site_path, max_age / 3600);
	}
    }
    scan_dir_close(scan);
    vstring_free(path);

    return (FLUSH_STAT_OK);
}

/* flush_request_receive - receive request */

static int flush_request_receive(VSTREAM *client_stream, VSTRING *request)
{
    int     count;

    /*
     * Kluge: choose the protocol depending on the request size.
     */
    if (read_wait(vstream_fileno(client_stream), var_ipc_timeout) < 0) {
	msg_warn("timeout while waiting for data from %s",
		 VSTREAM_PATH(client_stream));
	return (-1);
    }
    if ((count = peekfd(vstream_fileno(client_stream))) < 0) {
	msg_warn("cannot examine read buffer of %s: %m",
		 VSTREAM_PATH(client_stream));
	return (-1);
    }

    /*
     * Short request: master trigger. Use the string+null protocol.
     */
    if (count <= 2) {
	if (vstring_get_null(request, client_stream) == VSTREAM_EOF) {
	    msg_warn("end-of-input while reading request from %s: %m",
		     VSTREAM_PATH(client_stream));
	    return (-1);
	}
    }

    /*
     * Long request: real flush client. Use the attribute list protocol.
     */
    else {
	if (attr_scan(client_stream,
		      ATTR_FLAG_MORE | ATTR_FLAG_STRICT,
		      ATTR_TYPE_STR, MAIL_ATTR_REQ, request,
		      ATTR_TYPE_END) != 1) {
	    return (-1);
	}
    }
    return (0);
}

/* flush_service - perform service for client */

static void flush_service(VSTREAM *client_stream, char *unused_service,
			          char **argv)
{
    VSTRING *request = vstring_alloc(10);
    VSTRING *site = 0;
    VSTRING *queue_id = 0;
    static char wakeup[] = {		/* master wakeup request */
	TRIGGER_REQ_WAKEUP,
	0,
    };
    int     status = FLUSH_STAT_BAD;

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * This routine runs whenever a client connects to the UNIX-domain socket
     * dedicated to the fast flush service. What we see below is a little
     * protocol to (1) read a request from the client (the name of the site)
     * and (2) acknowledge that we have received the request.
     * 
     * All connection-management stuff is handled by the common code in
     * single_server.c.
     */
    if (flush_request_receive(client_stream, request) == 0) {
	if (STREQ(STR(request), FLUSH_REQ_ADD)) {
	    site = vstring_alloc(10);
	    queue_id = vstring_alloc(10);
	    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
			  ATTR_TYPE_STR, MAIL_ATTR_SITE, site,
			  ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, queue_id,
			  ATTR_TYPE_END) == 2
		&& mail_queue_id_ok(STR(queue_id)))
		status = flush_add_service(lowercase(STR(site)), STR(queue_id));
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, MAIL_ATTR_STATUS, status,
		       ATTR_TYPE_END);
	} else if (STREQ(STR(request), FLUSH_REQ_SEND_SITE)) {
	    site = vstring_alloc(10);
	    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
			  ATTR_TYPE_STR, MAIL_ATTR_SITE, site,
			  ATTR_TYPE_END) == 1)
		status = flush_send_service(lowercase(STR(site)),
					    UNTHROTTLE_BEFORE);
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, MAIL_ATTR_STATUS, status,
		       ATTR_TYPE_END);
	} else if (STREQ(STR(request), FLUSH_REQ_SEND_FILE)) {
	    queue_id = vstring_alloc(10);
	    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
			  ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, queue_id,
			  ATTR_TYPE_END) == 1)
		status = flush_send_file_service(STR(queue_id));
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, MAIL_ATTR_STATUS, status,
		       ATTR_TYPE_END);
	} else if (STREQ(STR(request), FLUSH_REQ_REFRESH)
		   || STREQ(STR(request), wakeup)) {
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, MAIL_ATTR_STATUS, FLUSH_STAT_OK,
		       ATTR_TYPE_END);
	    vstream_fflush(client_stream);
	    (void) flush_refresh_service(var_fflush_refresh);
	} else if (STREQ(STR(request), FLUSH_REQ_PURGE)) {
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, MAIL_ATTR_STATUS, FLUSH_STAT_OK,
		       ATTR_TYPE_END);
	    vstream_fflush(client_stream);
	    (void) flush_refresh_service(0);
	}
    } else
	attr_print(client_stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_INT, MAIL_ATTR_STATUS, status,
		   ATTR_TYPE_END);
    vstring_free(request);
    if (site)
	vstring_free(site);
    if (queue_id)
	vstring_free(queue_id);
}

/* pre_jail_init - pre-jail initialization */

static void pre_jail_init(char *unused_name, char **unused_argv)
{
    flush_domains = domain_list_init(match_parent_style(VAR_FFLUSH_DOMAINS),
				     var_fflush_domains);
}

MAIL_VERSION_STAMP_DECLARE;

/* main - pass control to the single-threaded skeleton */

int     main(int argc, char **argv)
{
    static const CONFIG_TIME_TABLE time_table[] = {
	VAR_FFLUSH_REFRESH, DEF_FFLUSH_REFRESH, &var_fflush_refresh, 1, 0,
	VAR_FFLUSH_PURGE, DEF_FFLUSH_PURGE, &var_fflush_purge, 1, 0,
	0,
    };

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    single_server_main(argc, argv, flush_service,
		       MAIL_SERVER_TIME_TABLE, time_table,
		       MAIL_SERVER_PRE_INIT, pre_jail_init,
		       MAIL_SERVER_UNLIMITED,
		       0);
}
