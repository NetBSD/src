/*++
/* NAME
/*	bounce 8
/* SUMMARY
/*	Postfix message bounce or defer daemon
/* SYNOPSIS
/*	\fBbounce\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBbounce\fR daemon maintains per-message log files with
/*	non-delivery status information. Each log file is named after the
/*	queue file that it corresponds to, and is kept in a queue subdirectory
/*	named after the service name in the \fBmaster.cf\fR file (either
/*	\fBbounce\fR or \fBdefer\fR).
/*	This program expects to be run from the \fBmaster\fR(8) process
/*	manager.
/*
/*	The \fBbounce\fR daemon processes two types of service requests:
/* .IP \(bu
/*	Append a recipient status record to a per-message log file.
/* .IP \(bu
/*	Post a bounce message, with a copy of a log file and of the
/*	corresponding message. When the bounce is posted successfully,
/*	the log file is deleted.
/* .PP
/*	The software does a best effort to notify the sender that there
/*	was a problem. A notification is sent even when the log file
/*	or original message cannot be read.
/*
/*	Optionally, a client can request that the per-message log file be
/*	deleted when the requested operation fails.
/*	This is used by clients that cannot retry transactions by
/*	themselves, and that depend on retry logic in their own client.
/* STANDARDS
/*	RFC 822 (ARPA Internet Text Messages)
/*	RFC 1894 (Delivery Status Notifications)
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* BUGS
/*	The log files use an ad-hoc, unstructured format. This will have
/*	to change in order to easily support standard delivery status
/*	notifications.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program. See the Postfix \fBmain.cf\fR file for syntax details
/*	and for default values. Use the \fBpostfix reload\fR command after
/*	a configuration change.
/* .IP \fBbounce_notice_recipient\fR
/*	The recipient of single bounce postmaster notices.
/* .IP \fB2bounce_notice_recipient\fR
/*	The recipient of double bounce postmaster notices.
/* .IP \fBdelay_notice_recipient\fR
/*	The recipient of "delayed mail" postmaster notices.
/* .IP \fBbounce_size_limit\fR
/*	Limit the amount of original message context that is sent in
/*	a non-delivery notification.
/* .IP \fBmail_name\fR
/*	Use this mail system name in the introductory text at the
/*	start of a bounce message.
/* .IP \fBnotify_classes\fR
/*	Notify the postmaster of bounced mail when this parameter
/*	includes the \fBbounce\fR class. For privacy reasons, the message
/*	body is not included.
/* SEE ALSO
/*	master(8) process manager
/*	qmgr(8) queue manager
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

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <stringops.h>

/* Global library. */

#include <mail_proto.h>
#include <mail_queue.h>
#include <mail_params.h>
#include <mail_conf.h>
#include <bounce.h>

/* Single-threaded server skeleton. */

#include <mail_server.h>

/* Application-specific. */

#include "bounce_service.h"

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

 /*
  * We're single threaded, so we can avoid some memory allocation overhead.
  */
static VSTRING *queue_id;
static VSTRING *queue_name;
static VSTRING *recipient;
static VSTRING *sender;
static VSTRING *why;

#define STR vstring_str

/* bounce_append_proto - bounce_append server protocol */

static int bounce_append_proto(char *service_name, VSTREAM *client)
{
    int     flags;

    /*
     * Read the and validate the client request.
     */
    if (mail_command_read(client, "%d %s %s %s",
			  &flags, queue_id, recipient, why) != 4) {
	msg_warn("malformed request");
	return (-1);
    }
    if (mail_queue_id_ok(STR(queue_id)) == 0) {
	msg_warn("malformed queue id: %s", printable(STR(queue_id), '?'));
	return (-1);
    }
    if (msg_verbose)
	msg_info("bounce_append_proto: service=%s id=%s to=%s why=%s",
		 service_name, STR(queue_id), STR(recipient), STR(why));

    /*
     * On request by the client, set up a trap to delete the log file in case
     * of errors.
     */
    if (flags & BOUNCE_FLAG_CLEAN)
	bounce_cleanup_register(service_name, STR(queue_id));

    /*
     * Execute the request.
     */
    return (bounce_append_service(service_name, STR(queue_id),
				  STR(recipient), STR(why)));
}

/* bounce_notify_proto - bounce_notify server protocol */

static int bounce_notify_proto(char *service_name, VSTREAM *client, int flush)
{
    int     flags;

    /*
     * Read and validate the client request.
     */
    if (mail_command_read(client, "%d %s %s %s",
			  &flags, queue_name, queue_id, sender) != 4) {
	msg_warn("malformed request");
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
    if (msg_verbose)
	msg_info("bounce_notify_proto: service=%s queue=%s id=%s sender=%s",
		 service_name, STR(queue_name), STR(queue_id), STR(sender));

    /*
     * On request by the client, set up a trap to delete the log file in case
     * of errors.
     */
    if (flags & BOUNCE_FLAG_CLEAN)
	bounce_cleanup_register(service_name, STR(queue_id));

    /*
     * Execute the request.
     */
    return (bounce_notify_service(service_name, STR(queue_name),
				  STR(queue_id), STR(sender), flush));
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
#define REALLY_BOUNCE	1
#define JUST_WARN	0

    if (mail_scan(client, "%d", &command) != 1) {
	msg_warn("malformed request");
	status = -1;
    } else if (command == BOUNCE_CMD_FLUSH) {
	status = bounce_notify_proto(service_name, client, REALLY_BOUNCE);
    } else if (command == BOUNCE_CMD_WARN) {
	status = bounce_notify_proto(service_name, client, JUST_WARN);
    } else if (command == BOUNCE_CMD_APPEND) {
	status = bounce_append_proto(service_name, client);
    } else {
	msg_warn("unknown command: %d", command);
	status = -1;
    }

    /*
     * When the request has completed, send the completion status to the
     * client.
     */
    mail_print(client, "%d", status);
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

/* post_jail_init - initialize after entering chroot jail */

static void post_jail_init(char *unused_name, char **unused_argv)
{

    /*
     * Initialize. We're single threaded so we can reuse some memory upon
     * successive requests.
     */
    queue_id = vstring_alloc(10);
    queue_name = vstring_alloc(10);
    recipient = vstring_alloc(10);
    sender = vstring_alloc(10);
    why = vstring_alloc(10);
}

/* main - the main program */

int     main(int argc, char **argv)
{
    static CONFIG_INT_TABLE int_table[] = {
	VAR_BOUNCE_LIMIT, DEF_BOUNCE_LIMIT, &var_bounce_limit, 1, 0,
	0,
    };
    static CONFIG_TIME_TABLE time_table[] = {
	VAR_MAX_QUEUE_TIME, DEF_MAX_QUEUE_TIME, &var_max_queue_time, 1, 8640000,
	VAR_DELAY_WARN_TIME, DEF_DELAY_WARN_TIME, &var_delay_warn_time, 0, 0,
	0,
    };
    static CONFIG_STR_TABLE str_table[] = {
	VAR_NOTIFY_CLASSES, DEF_NOTIFY_CLASSES, &var_notify_classes, 0, 0,
	VAR_BOUNCE_RCPT, DEF_BOUNCE_RCPT, &var_bounce_rcpt, 1, 0,
	VAR_2BOUNCE_RCPT, DEF_2BOUNCE_RCPT, &var_2bounce_rcpt, 1, 0,
	VAR_DELAY_RCPT, DEF_DELAY_RCPT, &var_delay_rcpt, 1, 0,
	0,
    };

    /*
     * Pass control to the single-threaded service skeleton.
     */
    single_server_main(argc, argv, bounce_service,
		       MAIL_SERVER_INT_TABLE, int_table,
		       MAIL_SERVER_STR_TABLE, str_table,
		       MAIL_SERVER_TIME_TABLE, time_table,
		       MAIL_SERVER_POST_INIT, post_jail_init,
		       0);
}
