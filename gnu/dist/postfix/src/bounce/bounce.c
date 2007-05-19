/*	$NetBSD: bounce.c,v 1.1.1.8 2007/05/19 16:28:04 heas Exp $	*/

/*++
/* NAME
/*	bounce 8
/* SUMMARY
/*	Postfix delivery status reports
/* SYNOPSIS
/*	\fBbounce\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBbounce\fR(8) daemon maintains per-message log files with
/*	delivery status information. Each log file is named after the
/*	queue file that it corresponds to, and is kept in a queue subdirectory
/*	named after the service name in the \fBmaster.cf\fR file (either
/*	\fBbounce\fR, \fBdefer\fR or \fBtrace\fR).
/*	This program expects to be run from the \fBmaster\fR(8) process
/*	manager.
/*
/*	The \fBbounce\fR(8) daemon processes two types of service requests:
/* .IP \(bu
/*	Append a recipient (non-)delivery status record to a per-message
/*	log file.
/* .IP \(bu
/*	Enqueue a delivery status notification message, with a copy
/*	of a per-message log file and of the corresponding message.
/*	When the delivery status notification message is
/*	enqueued successfully, the per-message log file is deleted.
/* .PP
/*	The software does a best notification effort. A non-delivery
/*	notification is sent even when the log file or the original
/*	message cannot be read.
/*
/*	Optionally, a bounce (defer, trace) client can request that the
/*	per-message log file be deleted when the requested operation fails.
/*	This is used by clients that cannot retry transactions by
/*	themselves, and that depend on retry logic in their own client.
/* STANDARDS
/*	RFC 822 (ARPA Internet Text Messages)
/*	RFC 2045 (Format of Internet Message Bodies)
/*	RFC 2822 (ARPA Internet Text Messages)
/*	RFC 3462 (Delivery Status Notifications)
/*	RFC 3464 (Delivery Status Notifications)
/*	RFC 3834 (Auto-Submitted: message header)
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are picked up automatically, as \fBbounce\fR(8)
/*	processes run for only a limited amount of time. Use the command
/*	"\fBpostfix reload\fR" to speed up a change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* .IP "\fB2bounce_notice_recipient (postmaster)\fR"
/*	The recipient of undeliverable mail that cannot be returned to
/*	the sender.
/* .IP "\fBbackwards_bounce_logfile_compatibility (yes)\fR"
/*	Produce additional \fBbounce\fR(8) logfile records that can be read by
/*	Postfix versions before 2.0.
/* .IP "\fBbounce_notice_recipient (postmaster)\fR"
/*	The recipient of postmaster notifications with the message headers
/*	of mail that Postfix did not deliver and of SMTP conversation
/*	transcripts of mail that Postfix did not receive.
/* .IP "\fBbounce_size_limit (50000)\fR"
/*	The maximal amount of original message text that is sent in a
/*	non-delivery notification.
/* .IP "\fBbounce_template_file (empty)\fR"
/*	Pathname of a configuration file with bounce message templates.
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdaemon_timeout (18000s)\fR"
/*	How much time a Postfix daemon process may take to handle a
/*	request before it is terminated by a built-in watchdog timer.
/* .IP "\fBdelay_notice_recipient (postmaster)\fR"
/*	The recipient of postmaster notifications with the message headers
/*	of mail that cannot be delivered within $delay_warning_time time
/*	units.
/* .IP "\fBdeliver_lock_attempts (20)\fR"
/*	The maximal number of attempts to acquire an exclusive lock on a
/*	mailbox file or \fBbounce\fR(8) logfile.
/* .IP "\fBdeliver_lock_delay (1s)\fR"
/*	The time between attempts to acquire an exclusive lock on a mailbox
/*	file or \fBbounce\fR(8) logfile.
/* .IP "\fBipc_timeout (3600s)\fR"
/*	The time limit for sending or receiving information over an internal
/*	communication channel.
/* .IP "\fBinternal_mail_filter_classes (empty)\fR"
/*	What categories of Postfix-generated mail are subject to
/*	before-queue content inspection by non_smtpd_milters, header_checks
/*	and body_checks.
/* .IP "\fBmail_name (Postfix)\fR"
/*	The mail system name that is displayed in Received: headers, in
/*	the SMTP greeting banner, and in bounced mail.
/* .IP "\fBmax_idle (100s)\fR"
/*	The maximum amount of time that an idle Postfix daemon process waits
/*	for an incoming connection before terminating voluntarily.
/* .IP "\fBmax_use (100)\fR"
/*	The maximal number of incoming connections that a Postfix daemon
/*	process will service before terminating voluntarily.
/* .IP "\fBnotify_classes (resource, software)\fR"
/*	The list of error classes that are reported to the postmaster.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (postfix)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* FILES
/*	/var/spool/postfix/bounce/* non-delivery records
/*	/var/spool/postfix/defer/* non-delivery records
/*	/var/spool/postfix/trace/* delivery status records
/* SEE ALSO
/*	bounce(5), bounce message template format
/*	qmgr(8), queue manager
/*	postconf(5), configuration parameters
/*	master(5), generic daemon options
/*	master(8), process manager
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
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>
#include <stdlib.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <stringops.h>
#include <load_file.h>

/* Global library. */

#include <mail_proto.h>
#include <mail_queue.h>
#include <mail_params.h>
#include <mail_version.h>
#include <mail_conf.h>
#include <bounce.h>
#include <mail_addr.h>
#include <rcpt_buf.h>
#include <dsb_scan.h>

/* Single-threaded server skeleton. */

#include <mail_server.h>

/* Application-specific. */

#include <bounce_service.h>

 /*
  * Tunables.
  */
int     var_bounce_limit;
int     var_max_queue_time;
int     var_delay_warn_time;
char   *var_notify_classes;
char   *var_bounce_rcpt;
char   *var_2bounce_rcpt;
char   *var_delay_rcpt;
char   *var_bounce_tmpl;

 /*
  * We're single threaded, so we can avoid some memory allocation overhead.
  */
static VSTRING *queue_id;
static VSTRING *queue_name;
static RCPT_BUF *rcpt_buf;
static VSTRING *encoding;
static VSTRING *sender;
static VSTRING *dsn_envid;
static VSTRING *verp_delims;
static DSN_BUF *dsn_buf;

 /*
  * Templates.
  */
BOUNCE_TEMPLATES *bounce_templates;

#define STR vstring_str

#define VS_NEUTER(s) printable(vstring_str(s), '?')

/* bounce_append_proto - bounce_append server protocol */

static int bounce_append_proto(char *service_name, VSTREAM *client)
{
    const char *myname = "bounce_append_proto";
    int     flags;

    /*
     * Read and validate the client request.
     */
    if (mail_command_server(client,
			    ATTR_TYPE_INT, MAIL_ATTR_FLAGS, &flags,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, queue_id,
			    ATTR_TYPE_FUNC, rcpb_scan, (void *) rcpt_buf,
			    ATTR_TYPE_FUNC, dsb_scan, (void *) dsn_buf,
			    ATTR_TYPE_END) != 4) {
	msg_warn("malformed request");
	return (-1);
    }

    /*
     * Sanitize input.
     */
    if (mail_queue_id_ok(STR(queue_id)) == 0) {
	msg_warn("malformed queue id: %s", printable(STR(queue_id), '?'));
	return (-1);
    }
    VS_NEUTER(rcpt_buf->address);
    VS_NEUTER(rcpt_buf->orig_addr);
    VS_NEUTER(rcpt_buf->dsn_orcpt);
    VS_NEUTER(dsn_buf->status);
    VS_NEUTER(dsn_buf->action);
    VS_NEUTER(dsn_buf->reason);
    VS_NEUTER(dsn_buf->dtype);
    VS_NEUTER(dsn_buf->dtext);
    VS_NEUTER(dsn_buf->mtype);
    VS_NEUTER(dsn_buf->mname);
    (void) RECIPIENT_FROM_RCPT_BUF(rcpt_buf);
    (void) DSN_FROM_DSN_BUF(dsn_buf);

    /*
     * Beware: some DSN or RECIPIENT fields may be null; access dsn_buf and
     * rcpt_buf buffers instead. See DSN_FROM_DSN_BUF() and
     * RECIPIENT_FROM_RCPT_BUF().
     */
    if (msg_verbose)
	msg_info("%s: flags=0x%x service=%s id=%s org_to=%s to=%s off=%ld dsn_org=%s, notif=0x%x stat=%s act=%s why=%s",
		 myname, flags, service_name, STR(queue_id),
		 STR(rcpt_buf->orig_addr), STR(rcpt_buf->address),
		 rcpt_buf->offset, STR(rcpt_buf->dsn_orcpt),
		 rcpt_buf->dsn_notify, STR(dsn_buf->status),
		 STR(dsn_buf->action), STR(dsn_buf->reason));

    /*
     * On request by the client, set up a trap to delete the log file in case
     * of errors.
     */
    if (flags & BOUNCE_FLAG_CLEAN)
	bounce_cleanup_register(service_name, STR(queue_id));

    /*
     * Execute the request.
     */
    return (bounce_append_service(flags, service_name, STR(queue_id),
				  &rcpt_buf->rcpt, &dsn_buf->dsn));
}

/* bounce_notify_proto - bounce_notify server protocol */

static int bounce_notify_proto(char *service_name, VSTREAM *client,
			        int (*service) (int, char *, char *, char *,
					        char *, char *, char *, int,
						        BOUNCE_TEMPLATES *))
{
    const char *myname = "bounce_notify_proto";
    int     flags;
    int     dsn_ret;

    /*
     * Read and validate the client request.
     */
    if (mail_command_server(client,
			    ATTR_TYPE_INT, MAIL_ATTR_FLAGS, &flags,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUE, queue_name,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, queue_id,
			    ATTR_TYPE_STR, MAIL_ATTR_ENCODING, encoding,
			    ATTR_TYPE_STR, MAIL_ATTR_SENDER, sender,
			    ATTR_TYPE_STR, MAIL_ATTR_DSN_ENVID, dsn_envid,
			    ATTR_TYPE_INT, MAIL_ATTR_DSN_RET, &dsn_ret,
			    ATTR_TYPE_END) != 7) {
	msg_warn("malformed request");
	return (-1);
    }

    /*
     * Sanitize input.
     */
    if (mail_queue_name_ok(STR(queue_name)) == 0) {
	msg_warn("malformed queue name: %s", printable(STR(queue_name), '?'));
	return (-1);
    }
    if (mail_queue_id_ok(STR(queue_id)) == 0) {
	msg_warn("malformed queue id: %s", printable(STR(queue_id), '?'));
	return (-1);
    }
    printable(STR(dsn_envid), '?');
    if (msg_verbose)
	msg_info("%s: flags=0x%x service=%s queue=%s id=%s encoding=%s sender=%s envid=%s ret=0x%x",
		 myname, flags, service_name, STR(queue_name), STR(queue_id),
		 STR(encoding), STR(sender), STR(dsn_envid), dsn_ret);

    /*
     * On request by the client, set up a trap to delete the log file in case
     * of errors.
     */
    if (flags & BOUNCE_FLAG_CLEAN)
	bounce_cleanup_register(service_name, STR(queue_id));

    /*
     * Execute the request.
     */
    return (service(flags, service_name, STR(queue_name),
		    STR(queue_id), STR(encoding),
		    STR(sender), STR(dsn_envid), dsn_ret,
		    bounce_templates));
}

/* bounce_verp_proto - bounce_notify server protocol, VERP style */

static int bounce_verp_proto(char *service_name, VSTREAM *client)
{
    const char *myname = "bounce_verp_proto";
    int     flags;
    int     dsn_ret;

    /*
     * Read and validate the client request.
     */
    if (mail_command_server(client,
			    ATTR_TYPE_INT, MAIL_ATTR_FLAGS, &flags,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUE, queue_name,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, queue_id,
			    ATTR_TYPE_STR, MAIL_ATTR_ENCODING, encoding,
			    ATTR_TYPE_STR, MAIL_ATTR_SENDER, sender,
			    ATTR_TYPE_STR, MAIL_ATTR_DSN_ENVID, dsn_envid,
			    ATTR_TYPE_INT, MAIL_ATTR_DSN_RET, &dsn_ret,
			    ATTR_TYPE_STR, MAIL_ATTR_VERPDL, verp_delims,
			    ATTR_TYPE_END) != 8) {
	msg_warn("malformed request");
	return (-1);
    }

    /*
     * Sanitize input.
     */
    if (mail_queue_name_ok(STR(queue_name)) == 0) {
	msg_warn("malformed queue name: %s", printable(STR(queue_name), '?'));
	return (-1);
    }
    if (mail_queue_id_ok(STR(queue_id)) == 0) {
	msg_warn("malformed queue id: %s", printable(STR(queue_id), '?'));
	return (-1);
    }
    printable(STR(dsn_envid), '?');
    if (strlen(STR(verp_delims)) != 2) {
	msg_warn("malformed verp delimiter string: %s",
		 printable(STR(verp_delims), '?'));
	return (-1);
    }
    if (msg_verbose)
	msg_info("%s: flags=0x%x service=%s queue=%s id=%s encoding=%s sender=%s envid=%s ret=0x%x delim=%s",
		 myname, flags, service_name, STR(queue_name),
		 STR(queue_id), STR(encoding), STR(sender),
		 STR(dsn_envid), dsn_ret, STR(verp_delims));

    /*
     * On request by the client, set up a trap to delete the log file in case
     * of errors.
     */
    if (flags & BOUNCE_FLAG_CLEAN)
	bounce_cleanup_register(service_name, STR(queue_id));

    /*
     * Execute the request. Fall back to traditional notification if a bounce
     * was returned as undeliverable, because we don't want to VERPify those.
     */
    if (!*STR(sender) || !strcasecmp(STR(sender), mail_addr_double_bounce())) {
	msg_warn("request to send VERP-style notification of bounced mail");
	return (bounce_notify_service(flags, service_name, STR(queue_name),
				      STR(queue_id), STR(encoding),
				      STR(sender), STR(dsn_envid), dsn_ret,
				      bounce_templates));
    } else
	return (bounce_notify_verp(flags, service_name, STR(queue_name),
				   STR(queue_id), STR(encoding),
				   STR(sender), STR(dsn_envid), dsn_ret,
				   STR(verp_delims), bounce_templates));
}

/* bounce_one_proto - bounce_one server protocol */

static int bounce_one_proto(char *service_name, VSTREAM *client)
{
    const char *myname = "bounce_one_proto";
    int     flags;
    int     dsn_ret;

    /*
     * Read and validate the client request.
     */
    if (mail_command_server(client,
			    ATTR_TYPE_INT, MAIL_ATTR_FLAGS, &flags,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUE, queue_name,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, queue_id,
			    ATTR_TYPE_STR, MAIL_ATTR_ENCODING, encoding,
			    ATTR_TYPE_STR, MAIL_ATTR_SENDER, sender,
			    ATTR_TYPE_STR, MAIL_ATTR_DSN_ENVID, dsn_envid,
			    ATTR_TYPE_INT, MAIL_ATTR_DSN_RET, &dsn_ret,
			    ATTR_TYPE_FUNC, rcpb_scan, (void *) rcpt_buf,
			    ATTR_TYPE_FUNC, dsb_scan, (void *) dsn_buf,
			    ATTR_TYPE_END) != 9) {
	msg_warn("malformed request");
	return (-1);
    }

    /*
     * Sanitize input.
     */
    if (strcmp(service_name, MAIL_SERVICE_BOUNCE) != 0) {
	msg_warn("wrong service name \"%s\" for one-recipient bouncing",
		 service_name);
	return (-1);
    }
    if (mail_queue_name_ok(STR(queue_name)) == 0) {
	msg_warn("malformed queue name: %s", printable(STR(queue_name), '?'));
	return (-1);
    }
    if (mail_queue_id_ok(STR(queue_id)) == 0) {
	msg_warn("malformed queue id: %s", printable(STR(queue_id), '?'));
	return (-1);
    }
    printable(STR(dsn_envid), '?');
    VS_NEUTER(rcpt_buf->address);
    VS_NEUTER(rcpt_buf->orig_addr);
    VS_NEUTER(rcpt_buf->dsn_orcpt);
    VS_NEUTER(dsn_buf->status);
    VS_NEUTER(dsn_buf->action);
    VS_NEUTER(dsn_buf->reason);
    VS_NEUTER(dsn_buf->dtype);
    VS_NEUTER(dsn_buf->dtext);
    VS_NEUTER(dsn_buf->mtype);
    VS_NEUTER(dsn_buf->mname);
    (void) RECIPIENT_FROM_RCPT_BUF(rcpt_buf);
    (void) DSN_FROM_DSN_BUF(dsn_buf);

    /*
     * Beware: some DSN or RECIPIENT fields may be null; access dsn_buf and
     * rcpt_buf buffers instead. See DSN_FROM_DSN_BUF() and
     * RECIPIENT_FROM_RCPT_BUF().
     */
    if (msg_verbose)
	msg_info("%s: flags=0x%x queue=%s id=%s encoding=%s sender=%s envid=%s dsn_ret=0x%x orig_to=%s to=%s off=%ld dsn_orig=%s notif=0x%x stat=%s act=%s why=%s",
		 myname, flags, STR(queue_name), STR(queue_id),
		 STR(encoding), STR(sender), STR(dsn_envid), dsn_ret,
		 STR(rcpt_buf->orig_addr), STR(rcpt_buf->address),
		 rcpt_buf->offset, STR(rcpt_buf->dsn_orcpt),
		 rcpt_buf->dsn_notify, STR(dsn_buf->status),
		 STR(dsn_buf->action), STR(dsn_buf->reason));

    /*
     * Execute the request.
     */
    return (bounce_one_service(flags, STR(queue_name), STR(queue_id),
			       STR(encoding), STR(sender), STR(dsn_envid),
			     dsn_ret, rcpt_buf, dsn_buf, bounce_templates));
}

/* bounce_service - parse bounce command type and delegate */

static void bounce_service(VSTREAM *client, char *service_name, char **argv)
{
    int     command;
    int     status;

    /*
     * Sanity check. This service takes no command-line arguments. The
     * service name should be usable as a subdirectory name.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);
    if (mail_queue_name_ok(service_name) == 0)
	msg_fatal("malformed service name: %s", service_name);

    /*
     * Read and validate the first parameter of the client request. Let the
     * request-specific protocol routines take care of the remainder.
     */
    if (attr_scan(client, ATTR_FLAG_STRICT | ATTR_FLAG_MORE,
		  ATTR_TYPE_INT, MAIL_ATTR_NREQ, &command, 0) != 1) {
	msg_warn("malformed request");
	status = -1;
    } else if (command == BOUNCE_CMD_VERP) {
	status = bounce_verp_proto(service_name, client);
    } else if (command == BOUNCE_CMD_FLUSH) {
	status = bounce_notify_proto(service_name, client,
				     bounce_notify_service);
    } else if (command == BOUNCE_CMD_WARN) {
	status = bounce_notify_proto(service_name, client,
				     bounce_warn_service);
    } else if (command == BOUNCE_CMD_TRACE) {
	status = bounce_notify_proto(service_name, client,
				     bounce_trace_service);
    } else if (command == BOUNCE_CMD_APPEND) {
	status = bounce_append_proto(service_name, client);
    } else if (command == BOUNCE_CMD_ONE) {
	status = bounce_one_proto(service_name, client);
    } else {
	msg_warn("unknown command: %d", command);
	status = -1;
    }

    /*
     * When the request has completed, send the completion status to the
     * client.
     */
    attr_print(client, ATTR_FLAG_NONE,
	       ATTR_TYPE_INT, MAIL_ATTR_STATUS, status,
	       ATTR_TYPE_END);
    vstream_fflush(client);

    /*
     * When a cleanup trap was set, delete the log file in case of error.
     * This includes errors while sending the completion status to the
     * client.
     */
    if (bounce_cleanup_path) {
	if (status || vstream_ferror(client))
	    bounce_cleanup_log();
	bounce_cleanup_unregister();
    }
}

static void load_helper(VSTREAM *stream, void *context)
{
    BOUNCE_TEMPLATES *templates = (BOUNCE_TEMPLATES *) context;

    bounce_templates_load(stream, templates);
}

/* pre_jail_init - pre-jail initialization */

static void pre_jail_init(char *unused_name, char **unused_argv)
{

    /*
     * Bundle up a bunch of bounce template information.
     */
    bounce_templates = bounce_templates_create();

    /*
     * Load the alternate message files (if specified) before entering the
     * chroot jail.
     */
    if (*var_bounce_tmpl)
	load_file(var_bounce_tmpl, load_helper, (char *) bounce_templates);
}

/* post_jail_init - initialize after entering chroot jail */

static void post_jail_init(char *service_name, char **unused_argv)
{

    /*
     * Special case: dump bounce templates. This is not part of the master(5)
     * public interface. This internal interface is used by the postconf
     * command. It was implemented before bounce templates were isolated into
     * modules that could have been called directly.
     */
    if (strcmp(service_name, "dump_templates") == 0) {
	bounce_templates_dump(VSTREAM_OUT, bounce_templates);
	vstream_fflush(VSTREAM_OUT);
	exit(0);
    }
    if (strcmp(service_name, "expand_templates") == 0) {
	bounce_templates_expand(VSTREAM_OUT, bounce_templates);
	vstream_fflush(VSTREAM_OUT);
	exit(0);
    }

    /*
     * Initialize. We're single threaded so we can reuse some memory upon
     * successive requests.
     */
    queue_id = vstring_alloc(10);
    queue_name = vstring_alloc(10);
    rcpt_buf = rcpb_create();
    encoding = vstring_alloc(10);
    sender = vstring_alloc(10);
    dsn_envid = vstring_alloc(10);
    verp_delims = vstring_alloc(10);
    dsn_buf = dsb_create();
}

MAIL_VERSION_STAMP_DECLARE;

/* main - the main program */

int     main(int argc, char **argv)
{
    static CONFIG_INT_TABLE int_table[] = {
	VAR_BOUNCE_LIMIT, DEF_BOUNCE_LIMIT, &var_bounce_limit, 1, 0,
	0,
    };
    static CONFIG_TIME_TABLE time_table[] = {
	VAR_MAX_QUEUE_TIME, DEF_MAX_QUEUE_TIME, &var_max_queue_time, 0, 8640000,
	VAR_DELAY_WARN_TIME, DEF_DELAY_WARN_TIME, &var_delay_warn_time, 0, 0,
	0,
    };
    static CONFIG_STR_TABLE str_table[] = {
	VAR_NOTIFY_CLASSES, DEF_NOTIFY_CLASSES, &var_notify_classes, 0, 0,
	VAR_BOUNCE_RCPT, DEF_BOUNCE_RCPT, &var_bounce_rcpt, 1, 0,
	VAR_2BOUNCE_RCPT, DEF_2BOUNCE_RCPT, &var_2bounce_rcpt, 1, 0,
	VAR_DELAY_RCPT, DEF_DELAY_RCPT, &var_delay_rcpt, 1, 0,
	VAR_BOUNCE_TMPL, DEF_BOUNCE_TMPL, &var_bounce_tmpl, 0, 0,
	0,
    };

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    /*
     * Pass control to the single-threaded service skeleton.
     */
    single_server_main(argc, argv, bounce_service,
		       MAIL_SERVER_INT_TABLE, int_table,
		       MAIL_SERVER_STR_TABLE, str_table,
		       MAIL_SERVER_TIME_TABLE, time_table,
		       MAIL_SERVER_PRE_INIT, pre_jail_init,
		       MAIL_SERVER_POST_INIT, post_jail_init,
		       MAIL_SERVER_UNLIMITED,
		       0);
}
