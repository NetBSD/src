/*++
/* NAME
/*	flush 8
/* SUMMARY
/*	Postfix fast flush server
/* SYNOPSIS
/*	\fBflush\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The flush server maintains a record of deferred mail by destination.
/*	This information is used to improve the performance of the SMTP
/*	\fBETRN\fR request, and of its command-line equivalent,
/*	\fBsendmail -qR\fR.
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
/* .IP "\fBFLUSH_REQ_ADD\fI sitename queue_id\fR"
/*	Inform the fast flush server that the specified message is queued for
/*	\fIsitename\fR. Depending on logging policy, the fast flush server
/*	stores or ignores the information.
/* .IP "\fBFLUSH_REQ_SEND\fI sitename\fR"
/*	Request delivery of mail that is queued for \fIsitename\fR.
/*	If the destination is eligible for a fast flush logfile,
/*	this request triggers delivery of messages listed in that
/*	destination's logfile, and the logfile is truncated to zero length;
/*	if mail is undeliverable it will be added back to the logfile.
/* .sp
/*	If the destination is not eligible for a fast flush logfile,
/*	this request is rejected (see below for status codes).
/* .IP \fBTRIGGER_REQ_WAKEUP\fR
/*	This wakeup request from the master is an alternative way to
/*	request \fBFLUSH_REQ_REFRESH\fR.
/* .IP "\fBFLUSH_REQ_REFRESH\fR (completes in the background)"
/*	Refresh non-empty per-destination logfiles that were not read in
/*	\fBfast_flush_refresh_time\fR hours, by simulating
/*	send requests (see above) for the corresponding destinations.
/* .sp
/*	Delete empty per-destination logfiles that were not updated in
/*	\fBfast_flush_purge_time\fR days.
/* .IP "\fBFLUSH_REQ_PURGE\fR (completes in the background)"
/*	Refresh all non-empty per-destination logfiles, by simulating
/*	send requests (see above) for the corresponding destinations.
/*	This can be incredibly expensive when logging is enabled for
/*	many destinations, and is not recommended.
/* .sp
/*	Delete empty per-destination logfiles that were not updated in
/*	\fBfast_flush_purge_time\fR days.
/* .PP
/*	The server response is one of:
/* .IP \fBFLUSH_STAT_OK\fR
/*	The request completed normally.
/* .IP \fBFLUSH_STAT_BAD\fR
/*	The flush server rejected the request (bad request name, bad
/*	request parameter value).
/* .IP \fBFLUSH_STAT_FAIL\fR
/*	The request failed.
/* .IP \fBFLUSH_STAT_DENY\fR
/*	The request was denied because the destination domain is not
/*	eligible for fast flush service, or because the fast flush
/*	service is disabled.
/* SECURITY
/* .ad
/* .fi
/*	The fast flush server is not security-sensitive. It does not
/*	talk to the network, and it does not talk to local users.
/*	The fast flush server can run chrooted at fixed low privilege.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* BUGS
/*	Fast flush logfiles are truncated only after a \fBFLUSH_REQ_SEND\fR
/*	request, not when mail is actually delivered, and therefore can
/*	accumulate outdated or redundant data. In order to maintain sanity,
/*	\fBFLUSH_REQ_REFRESH\fR must be executed periodically. This can
/*	be automated with a suitable wakeup timer setting in the
/*	\fBmaster.cf\fR configuration file.
/*
/*	Upon receipt of a request to deliver all mail for an eligible
/*	destination, the \fBflush\fR server requests delivery of all messages
/*	that are listed in that destination's logfile, regardless of the
/*	recipients of those messages. This is not an issue for mail
/*	that is sent to a \fBrelay_domains\fR destination because
/*	such mail typically only has recipients in one domain.
/* FILES
/*	/var/spool/postfix/flush, location of "fast flush" logfiles.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	See the Postfix \fBmain.cf\fR file for syntax details and for
/*	default values. Use the \fBpostfix reload\fR command after a
/*	configuration change.
/* .IP \fBfast_flush_domains\fR
/*	What destinations can have a "fast flush" logfile. By default,
/*	this is set to \fB$relay_domains\fR.
/* .IP \fBfast_flush_refresh_time\fR
/*	Refresh a non-empty "fast flush" logfile that was not read in
/*	this amount of time (default time unit: hours), by simulating
/*	a send request for the corresponding destination.
/* .IP \fBfast_flush_purge_time\fR
/*	Remove an empty "fast flush" logfile that was not updated in
/*	this amount of time (default time unit: days).
/* .IP \fBparent_domain_matches_subdomains\fR
/*	List of Postfix features that use \fIdomain.tld\fR patterns
/*	to match \fIsub.domain.tld\fR (as opposed to
/*	requiring \fI.domain.tld\fR patterns).
/* SEE ALSO
/*	smtpd(8) Postfix SMTP server
/*	qmgr(8) Postfix queue manager
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

/* Global library. */

#include <mail_params.h>
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
#define STREQ(x,y)		(strcmp(x,y) == 0)

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
  */
#define REFRESH_ONLY		0
#define REFRESH_AND_DELIVER	1

/* flush_site_to_path - convert domain or [addr] to harmless string */

static VSTRING *flush_site_to_path(VSTRING *path, const char *site)
{
    int     ch;

    /*
     * Allocate buffer on the fly; caller still needs to clean up.
     */
    if (path == 0)
	path = vstring_alloc(10);

    /*
     * Mask characters that could upset the name-to-queue-file mapping code.
     */
    while ((ch = *(unsigned const char *) site++) != 0)
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
    char   *myname = "flush_add_service";
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
    char   *myname = "flush_add_path";
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
    char   *myname = "flush_send_service";
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

/* flush_send_path - flush logfile file */

static int flush_send_path(const char *path, int how)
{
    const char *myname = "flush_send_path";
    VSTRING *queue_id;
    VSTRING *queue_file;
    VSTREAM *log;
    struct utimbuf tbuf;
    static char qmgr_deliver_trigger[] = {
	QMGR_REQ_SCAN_INCOMING,		/* scan incoming queue */
	QMGR_REQ_FLUSH_DEAD,		/* flush dead site/transport cache */
    };
    static char qmgr_refresh_trigger[] = {
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

	    mail_queue_path(queue_file, MAIL_QUEUE_DEFERRED, STR(queue_id));
	    if (utime(STR(queue_file), &tbuf) < 0) {
		if (errno != ENOENT)
		    msg_warn("%s: update %s time stamps: %m",
			     myname, STR(queue_file));
		/* XXX Wart... */
		mail_queue_path(queue_file, MAIL_QUEUE_INCOMING, STR(queue_id));
		if (utime(STR(queue_file), &tbuf) < 0)
		    if (errno != ENOENT)
			msg_warn("%s: update %s time stamps: %m",
				 myname, STR(queue_file));
	    } else if (mail_queue_rename(STR(queue_id), MAIL_QUEUE_DEFERRED,
					 MAIL_QUEUE_INCOMING) < 0) {
		if (errno != ENOENT)
		    msg_warn("%s: rename from %s to %s: %m",
			     STR(queue_file), MAIL_QUEUE_DEFERRED,
			     MAIL_QUEUE_INCOMING);
	    }
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
     * Request delivery and clean up.
     */
    if (myflock(vstream_fileno(log), INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock fast flush logfile %s: %m", myname, path);
    if (vstream_fclose(log) != 0)
	msg_warn("%s: read fast flush logfile %s: %m", myname, path);
    if (count > 0) {
	if (msg_verbose)
	    msg_info("%s: requesting delivery for logfile %s", myname, path);
	if (how == REFRESH_ONLY)
	    mail_trigger(MAIL_CLASS_PUBLIC, MAIL_SERVICE_QUEUE,
			 qmgr_refresh_trigger, sizeof(qmgr_refresh_trigger));
	else
	    mail_trigger(MAIL_CLASS_PUBLIC, MAIL_SERVICE_QUEUE,
			 qmgr_deliver_trigger, sizeof(qmgr_deliver_trigger));
    }
    return (FLUSH_STAT_OK);
}

/* flush_refresh_service - refresh logfiles beyond some age */

static int flush_refresh_service(int max_age)
{
    char   *myname = "flush_refresh_service";
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
		       ATTR_TYPE_NUM, MAIL_ATTR_STATUS, status,
		       ATTR_TYPE_END);
	} else if (STREQ(STR(request), FLUSH_REQ_SEND)) {
	    site = vstring_alloc(10);
	    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
			  ATTR_TYPE_STR, MAIL_ATTR_SITE, site,
			  ATTR_TYPE_END) == 1)
		status = flush_send_service(lowercase(STR(site)),
					    REFRESH_AND_DELIVER);
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_NUM, MAIL_ATTR_STATUS, status,
		       ATTR_TYPE_END);
	} else if (STREQ(STR(request), FLUSH_REQ_REFRESH)
		   || STREQ(STR(request), wakeup)) {
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_NUM, MAIL_ATTR_STATUS, FLUSH_STAT_OK,
		       ATTR_TYPE_END);
	    vstream_fflush(client_stream);
	    (void) flush_refresh_service(var_fflush_refresh);
	} else if (STREQ(STR(request), FLUSH_REQ_PURGE)) {
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_NUM, MAIL_ATTR_STATUS, FLUSH_STAT_OK,
		       ATTR_TYPE_END);
	    vstream_fflush(client_stream);
	    (void) flush_refresh_service(0);
	}
    } else
	attr_print(client_stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_NUM, MAIL_ATTR_STATUS, status,
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

/* main - pass control to the single-threaded skeleton */

int     main(int argc, char **argv)
{
    static CONFIG_TIME_TABLE time_table[] = {
	VAR_FFLUSH_REFRESH, DEF_FFLUSH_REFRESH, &var_fflush_refresh, 1, 0,
	VAR_FFLUSH_PURGE, DEF_FFLUSH_PURGE, &var_fflush_purge, 1, 0,
	0,
    };

    single_server_main(argc, argv, flush_service,
		       MAIL_SERVER_TIME_TABLE, time_table,
		       MAIL_SERVER_PRE_INIT, pre_jail_init,
		       0);
}
