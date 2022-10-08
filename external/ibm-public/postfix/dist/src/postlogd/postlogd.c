/*	$NetBSD: postlogd.c,v 1.3 2022/10/08 16:12:47 christos Exp $	*/

/*++
/* NAME
/*	postlogd 8
/* SUMMARY
/*	Postfix internal log server
/* SYNOPSIS
/*	\fBpostlogd\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	This program logs events on behalf of Postfix programs
/*	when the maillog configuration parameter specifies a non-empty
/*	value.
/* BUGS
/*	Non-daemon Postfix programs don't know that they should log
/*	to the internal logging service before they have processed
/*	command-line options and main.cf parameters. These programs
/*	still log earlier events to the syslog service.
/*
/*	If Postfix is down, the non-daemon programs \fBpostfix\fR(1),
/*	\fBpostsuper\fR(1), \fBpostmulti\fR(1), and \fBpostlog\fR(1),
/*	will log directly to \fB$maillog_file\fR. These programs
/*	expect to run with root privileges, for example during
/*	Postfix start-up, reload, or shutdown.
/*
/*	Other non-daemon Postfix programs will never write directly to
/*	\fB$maillog_file\fR (also, logging to stdout would interfere
/*	with the operation of some of these programs). These programs
/*	can log to \fBpostlogd\fR(8) if they are run by the super-user,
/*	or if their executable file has set-gid permission. Do not
/*	set this permission on programs other than \fBpostdrop\fR(1),
/*	\fBpostqueue\fR(1) and (Postfix >= 3.7) \fBpostlog\fR(1).
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are picked up automatically, as
/*	\fBpostlogd\fR(8) processes run for only a limited amount
/*	of time. Use the command "\fBpostfix reload\fR" to speed
/*	up a change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBmaillog_file (empty)\fR"
/*	The name of an optional logfile that is written by the Postfix
/*	\fBpostlogd\fR(8) service.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	A prefix that is prepended to the process name in syslog
/*	records, so that, for example, "smtpd" becomes "prefix/smtpd".
/* .IP "\fBservice_name (read-only)\fR"
/*	The master.cf service name of a Postfix daemon process.
/* .IP "\fBpostlogd_watchdog_timeout (10s)\fR"
/*	How much time a \fBpostlogd\fR(8) process may take to process a request
/*	before it is terminated by a built-in watchdog timer.
/* SEE ALSO
/*	postconf(5), configuration parameters
/*	syslogd(8), system logging
/* README_FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or
/*	"\fBpostconf html_directory\fR" to locate this information.
/* .na
/* .nf
/*	MAILLOG_README, Postfix logging to file or stdout
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* HISTORY
/* .ad
/* .fi
/*	This service was introduced with Postfix version 3.4.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <logwriter.h>
#include <msg.h>
#include <msg_logger.h>
#include <stringops.h>
#include <vstream.h>

 /*
  * Global library.
  */
#include <mail_params.h>
#include <mail_task.h>
#include <mail_version.h>
#include <maillog_client.h>

 /*
  * Server skeleton.
  */
#include <mail_server.h>

 /*
  * Tunable parameters.
  */
int     var_postlogd_watchdog;

 /*
  * Silly little macros.
  */
#define STR(x)			vstring_str(x)
#define LEN(x)			VSTRING_LEN(x)

 /*
  * Logfile stream.
  */
static VSTREAM *postlogd_stream = 0;

/* postlogd_fallback - log messages from postlogd(8) itself */

static void postlogd_fallback(const char *buf)
{
    (void) logwriter_write(postlogd_stream, buf, strlen(buf));
}

/* postlogd_service - perform service for client */

static void postlogd_service(char *buf, ssize_t len, char *unused_service,
			             char **unused_argv)
{

    if (postlogd_stream) {
	(void) logwriter_write(postlogd_stream, buf, len);
    }

    /*
     * After a configuration change that removes the maillog_file pathname,
     * this service may still receive messages (after "postfix reload" or
     * after process refresh) from programs that use the old maillog_file
     * setting. Redirect those messages to the current logging mechanism.
     */
    else {
	char   *bp = buf;
	char   *progname_pid;

	/*
	 * Avoid surprises: strip off the date, time, host, and program[pid]:
	 * prefix that were prepended by msg_logger(3). Then, hope that the
	 * current logging driver suppresses its own PID, when it sees that
	 * there is a PID embedded in the 'program name'.
	 */
	(void) mystrtok(&bp, CHARS_SPACE);	/* month */
	(void) mystrtok(&bp, CHARS_SPACE);	/* day */
	(void) mystrtok(&bp, CHARS_SPACE);	/* time */
	(void) mystrtok(&bp, CHARS_SPACE);	/* host */
	progname_pid = mystrtok(&bp, ":" CHARS_SPACE);	/* name[pid] sans ':' */
	bp += strspn(bp, CHARS_SPACE);
	if (progname_pid)
	    maillog_client_init(progname_pid, MAILLOG_CLIENT_FLAG_NONE);
	msg_info("%.*s", (int) (len - (bp - buf)), bp);

	/*
	 * Restore the program name, in case postlogd(8) needs to log
	 * something about itself. We have to call maillog_client_init() in
	 * any case, because neither msg_syslog_init() nor openlog() make a
	 * copy of the name argument. We can't leave that pointing into the
	 * middle of the above message buffer.
	 */
	maillog_client_init(mail_task((char *) 0), MAILLOG_CLIENT_FLAG_NONE);
    }
}

/* pre_jail_init - pre-jail handling */

static void pre_jail_init(char *unused_service_name, char **argv)
{

    /*
     * During process initialization, the postlogd daemon will log events to
     * the postlog socket, so that they can be logged to file later. Once the
     * postlogd daemon is handling requests, it will stop logging to the
     * postlog socket and will instead write to the logfile, to avoid
     * infinite recursion.
     */

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * After a configuration change that removes the maillog_file pathname,
     * this service may still receive messages from processes that still use
     * the old configuration. Those messages will have to be redirected to
     * the current logging subsystem.
     */
    if (*var_maillog_file != 0) {

	/*
	 * Instantiate the logwriter or bust.
	 */
	postlogd_stream = logwriter_open_or_die(var_maillog_file);

	/*
	 * Inform the msg_logger client to stop using the postlog socket, and
	 * to call our logwriter.
	 */
	msg_logger_control(CA_MSG_LOGGER_CTL_FALLBACK_ONLY,
			   CA_MSG_LOGGER_CTL_FALLBACK_FN(postlogd_fallback),
			   CA_MSG_LOGGER_CTL_END);
    }
}

/* post_jail_init - post-jail initialization */

static void post_jail_init(char *unused_name, char **unused_argv)
{

    /*
     * Prevent automatic process suicide after a limited number of client
     * requests. It is OK to terminate after a limited amount of idle time.
     */
    var_use_limit = 0;
}

MAIL_VERSION_STAMP_DECLARE;

/* main - pass control to the multi-threaded skeleton */

int     main(int argc, char **argv)
{
    static const CONFIG_TIME_TABLE time_table[] = {
	VAR_POSTLOGD_WATCHDOG, DEF_POSTLOGD_WATCHDOG, &var_postlogd_watchdog, 10, 0,
	0,
    };

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    /*
     * This is a datagram service, not a stream service, so that postlogd can
     * restart immediately after "postfix reload" without requiring clients
     * to resend messages. Those messages remain queued in the kernel until a
     * new postlogd process retrieves them. It would be unreasonable to
     * require that clients retransmit logs, especially in the case of a
     * fatal or panic error.
     */
    dgram_server_main(argc, argv, postlogd_service,
		      CA_MAIL_SERVER_TIME_TABLE(time_table),
		      CA_MAIL_SERVER_PRE_INIT(pre_jail_init),
		      CA_MAIL_SERVER_POST_INIT(post_jail_init),
		      CA_MAIL_SERVER_SOLITARY,
		      CA_MAIL_SERVER_WATCHDOG(&var_postlogd_watchdog),
		      0);
}
