/*	$NetBSD: postqueue.c,v 1.4.2.1 2023/12/25 12:43:34 martin Exp $	*/

/*++
/* NAME
/*	postqueue 1
/* SUMMARY
/*	Postfix queue control
/* SYNOPSIS
/* .ti -4
/*	\fBTo flush the mail queue\fR:
/*
/*	\fBpostqueue\fR [\fB-v\fR] [\fB-c \fIconfig_dir\fR] \fB-f\fR
/*
/*	\fBpostqueue\fR [\fB-v\fR] [\fB-c \fIconfig_dir\fR] \fB-i \fIqueue_id\fR
/*
/*	\fBpostqueue\fR [\fB-v\fR] [\fB-c \fIconfig_dir\fR] \fB-s \fIsite\fR
/*
/* .ti -4
/*	\fBTo list the mail queue\fR:
/*
/*	\fBpostqueue\fR [\fB-v\fR] [\fB-c \fIconfig_dir\fR] \fB-j\fR
/*
/*	\fBpostqueue\fR [\fB-v\fR] [\fB-c \fIconfig_dir\fR] \fB-p\fR
/* DESCRIPTION
/*	The \fBpostqueue\fR(1) command implements the Postfix user interface
/*	for queue management. It implements operations that are
/*	traditionally available via the \fBsendmail\fR(1) command.
/*	See the \fBpostsuper\fR(1) command for queue operations
/*	that require super-user privileges such as deleting a message
/*	from the queue or changing the status of a message.
/*
/*	The following options are recognized:
/* .IP "\fB-c \fIconfig_dir\fR"
/*	The \fBmain.cf\fR configuration file is in the named directory
/*	instead of the default configuration directory. See also the
/*	MAIL_CONFIG environment setting below.
/* .IP \fB-f\fR
/*	Flush the queue: attempt to deliver all queued mail.
/*
/*	This option implements the traditional "\fBsendmail -q\fR" command,
/*	by contacting the Postfix \fBqmgr\fR(8) daemon.
/*
/*	Warning: flushing undeliverable mail frequently will result in
/*	poor delivery performance of all other mail.
/* .IP "\fB-i \fIqueue_id\fR"
/*	Schedule immediate delivery of deferred mail with the
/*	specified queue ID.
/*
/*	This option implements the traditional \fBsendmail -qI\fR
/*	command, by contacting the \fBflush\fR(8) server.
/*
/*	This feature is available with Postfix version 2.4 and later.
/* .IP "\fB-j\fR"
/*	Produce a queue listing in JSON LINES format, based on
/*	output from the showq(8) daemon. See "\fBJSON OBJECT
/*	FORMAT\fR" below for details.
/*
/*	This feature is available in Postfix 3.1 and later.
/* .IP \fB-p\fR
/*	Produce a traditional sendmail-style queue listing.
/*	This option implements the traditional \fBmailq\fR command,
/*	by contacting the Postfix \fBshowq\fR(8) daemon.
/*
/*	Each queue entry shows the queue file ID, message
/*	size, arrival time, sender, and the recipients that still need to
/*	be delivered.  If mail could not be delivered upon the last attempt,
/*	the reason for failure is shown. The queue ID string
/*	is followed by an optional status character:
/* .RS
/* .IP \fB*\fR
/*	The message is in the \fBactive\fR queue, i.e. the message is
/*	selected for delivery.
/* .IP \fB!\fR
/*	The message is in the \fBhold\fR queue, i.e. no further delivery
/*	attempt will be made until the mail is taken off hold.
/* .IP \fB#\fR
/*	The message is forced to expire. See the \fBpostsuper\fR(1)
/*	options \fB-e\fR or \fB-f\fR.
/* .sp
/*	This feature is available in Postfix 3.5 and later.
/* .RE
/* .IP "\fB-s \fIsite\fR"
/*	Schedule immediate delivery of all mail that is queued for the named
/*	\fIsite\fR. A numerical site must be specified as a valid RFC 5321
/*	address literal enclosed in [], just like in email addresses.
/*	The site must be eligible for the "fast flush" service.
/*	See \fBflush\fR(8) for more information about the "fast flush"
/*	service.
/*
/*	This option implements the traditional "\fBsendmail -qR\fIsite\fR"
/*	command, by contacting the Postfix \fBflush\fR(8) daemon.
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the software increasingly verbose. As of Postfix 2.3,
/*	this option is available for the super-user only.
/* JSON OBJECT FORMAT
/* .ad
/* .fi
/*	Each JSON object represents one queue file; it is emitted
/*	as a single text line followed by a newline character.
/*
/*	Object members have string values unless indicated otherwise.
/*	Programs should ignore object members that are not listed
/*	here; the list of members is expected to grow over time.
/* .IP \fBqueue_name\fR
/*	The name of the queue where the message was found.  Note
/*	that the contents of the mail queue may change while it is
/*	being listed; some messages may appear more than once, and
/*	some messages may be missed.
/* .IP \fBqueue_id\fR
/*	The queue file name. The queue_id may be reused within a
/*	Postfix instance unless "enable_long_queue_ids = true" and
/*	time is monotonic.  Even then, the queue_id is not expected
/*	to be unique between different Postfix instances.  Management
/*	tools that require a unique name should combine the queue_id
/*	with the myhostname setting of the Postfix instance.
/* .IP \fBarrival_time\fR
/*	The number of seconds since the start of the UNIX epoch.
/* .IP \fBmessage_size\fR
/*	The number of bytes in the message header and body. This
/*	number does not include message envelope information. It
/*	is approximately equal to the number of bytes that would
/*	be transmitted via SMTP including the <CR><LF> line endings.
/* .IP \fBforced_expire\fR
/*	The message is forced to expire (\fBtrue\fR or \fBfalse\fR).
/*	See the \fBpostsuper\fR(1) options \fB-e\fR or \fB-f\fR.
/* .sp
/*	This feature is available in Postfix 3.5 and later.
/* .IP \fBsender\fR
/*	The envelope sender address.
/* .IP \fBrecipients\fR
/*	An array containing zero or more objects with members:
/* .RS
/* .IP \fBaddress\fR
/*	One recipient address.
/* .IP \fBdelay_reason\fR
/*	If present, the reason for delayed delivery.  Delayed
/*	recipients may have no delay reason, for example, while
/*	delivery is in progress, or after the system was stopped
/*	before it could record the reason.
/* .RE
/* SECURITY
/* .ad
/* .fi
/*	This program is designed to run with set-group ID privileges, so
/*	that it can connect to Postfix daemon processes.
/* STANDARDS
/*	RFC 7159 (JSON notation)
/* DIAGNOSTICS
/*	Problems are logged to \fBsyslogd\fR(8) or \fBpostlogd\fR(8),
/*	and to the standard error stream.
/* ENVIRONMENT
/* .ad
/* .fi
/* .IP MAIL_CONFIG
/*	Directory with the \fBmain.cf\fR file. In order to avoid exploitation
/*	of set-group ID privileges, a non-standard directory is allowed only
/*	if:
/* .RS
/* .IP \(bu
/*	The name is listed in the standard \fBmain.cf\fR file with the
/*	\fBalternate_config_directories\fR configuration parameter.
/* .IP \(bu
/*	The command is invoked by the super-user.
/* .RE
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program.
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* .IP "\fBalternate_config_directories (empty)\fR"
/*	A list of non-default Postfix configuration directories that may
/*	be specified with "-c config_directory" on the command line (in the
/*	case of \fBsendmail\fR(1), with the "-C" option), or via the MAIL_CONFIG
/*	environment parameter.
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBcommand_directory (see 'postconf -d' output)\fR"
/*	The location of all postfix administrative commands.
/* .IP "\fBfast_flush_domains ($relay_domains)\fR"
/*	Optional list of destinations that are eligible for per-destination
/*	logfiles with mail that is queued to those destinations.
/* .IP "\fBimport_environment (see 'postconf -d' output)\fR"
/*	The list of environment variables that a privileged Postfix
/*	process will import from a non-Postfix parent process, or name=value
/*	environment overrides.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	A prefix that is prepended to the process name in syslog
/*	records, so that, for example, "smtpd" becomes "prefix/smtpd".
/* .IP "\fBtrigger_timeout (10s)\fR"
/*	The time limit for sending a trigger to a Postfix daemon (for
/*	example, the \fBpickup\fR(8) or \fBqmgr\fR(8) daemon).
/* .PP
/*	Available in Postfix version 2.2 and later:
/* .IP "\fBauthorized_flush_users (static:anyone)\fR"
/*	List of users who are authorized to flush the queue.
/* .IP "\fBauthorized_mailq_users (static:anyone)\fR"
/*	List of users who are authorized to view the queue.
/* FILES
/*	/var/spool/postfix, mail queue
/* SEE ALSO
/*	qmgr(8), queue manager
/*	showq(8), list mail queue
/*	flush(8), fast flush service
/*	sendmail(1), Sendmail-compatible user interface
/*	postsuper(1), privileged queue operations
/*	postlogd(8), Postfix logging
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
/* .ad
/* .fi
/*	The postqueue command was introduced with Postfix version 1.1.
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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sysexits.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <clean_env.h>
#include <vstream.h>
#include <msg_vstream.h>
#include <argv.h>
#include <safe.h>
#include <connect.h>
#include <valid_hostname.h>
#include <warn_stat.h>
#include <events.h>
#include <stringops.h>

/* Global library. */

#include <mail_proto.h>
#include <mail_params.h>
#include <mail_version.h>
#include <mail_conf.h>
#include <mail_task.h>
#include <mail_run.h>
#include <mail_flush.h>
#include <mail_queue.h>
#include <flush_clnt.h>
#include <smtp_stream.h>
#include <user_acl.h>
#include <valid_mailhost_addr.h>
#include <mail_dict.h>
#include <mail_parm_split.h>
#include <maillog_client.h>

/* Application-specific. */

#include <postqueue.h>

 /*
  * WARNING WARNING WARNING
  * 
  * This software is designed to run set-gid. In order to avoid exploitation of
  * privilege, this software should not run any external commands, nor should
  * it take any information from the user, unless that information can be
  * properly sanitized. To get an idea of how much information a process can
  * inherit from a potentially hostile user, examine all the members of the
  * process structure (typically, in /usr/include/sys/proc.h): the current
  * directory, open files, timers, signals, environment, command line, umask,
  * and so on.
  */

 /*
  * Modes of operation.
  * 
  * XXX To support flush by recipient domain, or for destinations that have no
  * mapping to logfile, the server has to defend against resource exhaustion
  * attacks. A malicious user could fork off a postqueue client that starts
  * an expensive requests and then kills the client immediately; this way she
  * could create a high Postfix load on the system without ever exceeding her
  * own per-user process limit. To prevent this, either the server needs to
  * establish frequent proof of client liveliness with challenge/response, or
  * the client needs to restrict expensive requests to privileged users only.
  * 
  * We don't have this problem with queue listings. The showq server detects an
  * EPIPE error after reporting a few queue entries.
  */
#define PQ_MODE_DEFAULT		0	/* noop */
#define PQ_MODE_MAILQ_LIST	1	/* list mail queue */
#define PQ_MODE_FLUSH_QUEUE	2	/* flush queue */
#define PQ_MODE_FLUSH_SITE	3	/* flush site */
#define PQ_MODE_FLUSH_FILE	4	/* flush message */
#define PQ_MODE_JSON_LIST	5	/* JSON-format queue listing */

 /*
  * Silly little macros (SLMs).
  */
#define STR	vstring_str

 /*
  * Queue manipulation access lists.
  */
char   *var_flush_acl;
char   *var_showq_acl;

static const CONFIG_STR_TABLE str_table[] = {
    VAR_FLUSH_ACL, DEF_FLUSH_ACL, &var_flush_acl, 0, 0,
    VAR_SHOWQ_ACL, DEF_SHOWQ_ACL, &var_showq_acl, 0, 0,
    0,
};

/* showq_client - run the appropriate showq protocol client */

static void showq_client(int mode, VSTREAM *showq)
{
    if (attr_scan(showq, ATTR_FLAG_STRICT,
		  RECV_ATTR_STREQ(MAIL_ATTR_PROTO, MAIL_ATTR_PROTO_SHOWQ),
		  ATTR_TYPE_END) != 0)
	msg_fatal_status(EX_SOFTWARE, "malformed showq server response");
    switch (mode) {
    case PQ_MODE_MAILQ_LIST:
	showq_compat(showq);
	break;
    case PQ_MODE_JSON_LIST:
	showq_json(showq);
	break;
    default:
	msg_panic("show_queue: unknown mode %d", mode);
    }
}

/* show_queue - show queue status */

static void show_queue(int mode)
{
    const char *errstr;
    VSTREAM *showq;
    int     n;
    uid_t   uid = getuid();

    if (uid != 0 && uid != var_owner_uid
	&& (errstr = check_user_acl_byuid(VAR_SHOWQ_ACL, var_showq_acl,
					  uid)) != 0)
	msg_fatal_status(EX_NOPERM,
		       "User %s(%ld) is not allowed to view the mail queue",
			 errstr, (long) uid);

    /*
     * Connect to the show queue service.
     */
    if ((showq = mail_connect(MAIL_CLASS_PUBLIC, var_showq_service, BLOCKING)) != 0) {
	showq_client(mode, showq);
	if (vstream_fclose(showq))
	    msg_warn("close: %m");
    }

    /*
     * Don't assume that the mail system is down when the user has
     * insufficient permission to access the showq socket.
     */
    else if (errno == EACCES || errno == EPERM) {
	msg_fatal_status(EX_SOFTWARE,
			 "Connect to the %s %s service: %m",
			 var_mail_name, var_showq_service);
    }

    /*
     * When the mail system is down, the superuser can still access the queue
     * directly. Just run the showq program in stand-alone mode.
     */
    else if (geteuid() == 0) {
	char   *showq_path;
	ARGV   *argv;
	int     stat;

	msg_warn("Mail system is down -- accessing queue directly"
		 " (Connect to the %s %s service: %m)",
		 var_mail_name, var_showq_service);
	showq_path = concatenate(var_daemon_dir, "/", var_showq_service,
				 (char *) 0);
	argv = argv_alloc(6);
	argv_add(argv, showq_path, "-u", "-S", (char *) 0);
	for (n = 0; n < msg_verbose; n++)
	    argv_add(argv, "-v", (char *) 0);
	argv_terminate(argv);
	if ((showq = vstream_popen(O_RDONLY,
				   CA_VSTREAM_POPEN_ARGV(argv->argv),
				   CA_VSTREAM_POPEN_END)) == 0) {
	    stat = -1;
	} else {
	    showq_client(mode, showq);
	    stat = vstream_pclose(showq);
	}
	argv_free(argv);
	myfree(showq_path);
	if (stat != 0)
	    msg_fatal_status(stat < 0 ? EX_OSERR : EX_SOFTWARE,
			     "Error running %s", showq_path);
    }

    /*
     * When the mail system is down, unprivileged users are stuck, because by
     * design the mail system contains no set_uid programs. The only way for
     * an unprivileged user to cross protection boundaries is to talk to the
     * showq daemon.
     */
    else {
	msg_fatal_status(EX_UNAVAILABLE,
			 "Queue report unavailable - mail system is down"
			 " (Connect to the %s %s service: %m)",
			 var_mail_name, var_showq_service);
    }
}

/* flush_queue - force delivery */

static void flush_queue(void)
{
    const char *errstr;
    uid_t   uid = getuid();

    if (uid != 0 && uid != var_owner_uid
	&& (errstr = check_user_acl_byuid(VAR_FLUSH_ACL, var_flush_acl,
					  uid)) != 0)
	msg_fatal_status(EX_NOPERM,
		      "User %s(%ld) is not allowed to flush the mail queue",
			 errstr, (long) uid);

    /*
     * Trigger the flush queue service.
     */
    if (mail_flush_deferred() < 0)
	msg_fatal_status(EX_UNAVAILABLE,
			 "Cannot flush mail queue - mail system is down");
    if (mail_flush_maildrop() < 0)
	msg_fatal_status(EX_UNAVAILABLE,
			 "Cannot flush mail queue - mail system is down");
    event_drain(2);
}

/* flush_site - flush mail for site */

static void flush_site(const char *site)
{
    int     status;
    const char *errstr;
    uid_t   uid = getuid();

    if (uid != 0 && uid != var_owner_uid
	&& (errstr = check_user_acl_byuid(VAR_FLUSH_ACL, var_flush_acl,
					  uid)) != 0)
	msg_fatal_status(EX_NOPERM,
		      "User %s(%ld) is not allowed to flush the mail queue",
			 errstr, (long) uid);

    flush_init();

    switch (status = flush_send_site(site)) {
    case FLUSH_STAT_OK:
	exit(0);
    case FLUSH_STAT_BAD:
	msg_fatal_status(EX_USAGE, "Invalid request: \"%s\"", site);
    case FLUSH_STAT_FAIL:
	msg_fatal_status(EX_UNAVAILABLE,
			 "Cannot flush mail queue - mail system is down");
    case FLUSH_STAT_DENY:
	msg_fatal_status(EX_UNAVAILABLE,
		   "Flush service is not configured for destination \"%s\"",
			 site);
    default:
	msg_fatal_status(EX_SOFTWARE,
			 "Unknown flush server reply status %d", status);
    }
}

/* flush_file - flush mail with specific queue ID */

static void flush_file(const char *queue_id)
{
    int     status;
    const char *errstr;
    uid_t   uid = getuid();

    if (uid != 0 && uid != var_owner_uid
	&& (errstr = check_user_acl_byuid(VAR_FLUSH_ACL, var_flush_acl,
					  uid)) != 0)
	msg_fatal_status(EX_NOPERM,
		      "User %s(%ld) is not allowed to flush the mail queue",
			 errstr, (long) uid);

    switch (status = flush_send_file(queue_id)) {
    case FLUSH_STAT_OK:
	exit(0);
    case FLUSH_STAT_BAD:
	msg_fatal_status(EX_USAGE, "Invalid request: \"%s\"", queue_id);
    case FLUSH_STAT_FAIL:
	msg_fatal_status(EX_UNAVAILABLE,
			 "Cannot flush mail queue - mail system is down");
    default:
	msg_fatal_status(EX_SOFTWARE,
			 "Unexpected flush server reply status %d", status);
    }
}

/* unavailable - sanitize exit status from library run-time errors */

static void unavailable(void)
{
    exit(EX_UNAVAILABLE);
}

/* usage - scream and die */

static NORETURN usage(void)
{
    msg_fatal_status(EX_USAGE, "usage: postqueue -f | postqueue -i queueid | postqueue -j | postqueue -p | postqueue -s site");
}

MAIL_VERSION_STAMP_DECLARE;

/* main - the main program */

int     main(int argc, char **argv)
{
    struct stat st;
    int     c;
    int     fd;
    int     mode = PQ_MODE_DEFAULT;
    char   *site_to_flush = 0;
    char   *id_to_flush = 0;
    ARGV   *import_env;
    int     bad_site;

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

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
	    msg_fatal_status(EX_UNAVAILABLE, "open /dev/null: %m");

    /*
     * Initialize. Set up logging. Read the global configuration file after
     * parsing command-line arguments. Censor the process name: it is
     * provided by the user.
     */
    argv[0] = "postqueue";
    msg_vstream_init(argv[0], VSTREAM_ERR);
    msg_cleanup(unavailable);
    maillog_client_init(mail_task("postqueue"), MAILLOG_CLIENT_FLAG_NONE);
    set_mail_conf_str(VAR_PROCNAME, var_procname = mystrdup(argv[0]));

    /*
     * Check the Postfix library version as soon as we enable logging.
     */
    MAIL_VERSION_CHECK;

    /*
     * Parse JCL. This program is set-gid and must sanitize all command-line
     * parameters. The configuration directory argument is validated by the
     * mail configuration read routine. Don't do complex things until we have
     * completed initializations.
     */
    while ((c = GETOPT(argc, argv, "c:fi:jps:v")) > 0) {
	switch (c) {
	case 'c':				/* non-default configuration */
	    if (setenv(CONF_ENV_PATH, optarg, 1) < 0)
		msg_fatal_status(EX_UNAVAILABLE, "out of memory");
	    break;
	case 'f':				/* flush queue */
	    if (mode != PQ_MODE_DEFAULT)
		usage();
	    mode = PQ_MODE_FLUSH_QUEUE;
	    break;
	case 'i':				/* flush queue file */
	    if (mode != PQ_MODE_DEFAULT)
		usage();
	    mode = PQ_MODE_FLUSH_FILE;
	    id_to_flush = optarg;
	    break;
	case 'j':
	    if (mode != PQ_MODE_DEFAULT)
		usage();
	    mode = PQ_MODE_JSON_LIST;
	    break;
	case 'p':				/* traditional mailq */
	    if (mode != PQ_MODE_DEFAULT)
		usage();
	    mode = PQ_MODE_MAILQ_LIST;
	    break;
	case 's':				/* flush site */
	    if (mode != PQ_MODE_DEFAULT)
		usage();
	    mode = PQ_MODE_FLUSH_SITE;
	    site_to_flush = optarg;
	    break;
	case 'v':
	    if (geteuid() == 0)
		msg_verbose++;
	    break;
	default:
	    usage();
	}
    }
    if (argc > optind)
	usage();

    /*
     * Further initialization...
     */
    mail_conf_read();
    /* Re-evaluate mail_task() after reading main.cf. */
    maillog_client_init(mail_task("postqueue"), MAILLOG_CLIENT_FLAG_NONE);
    mail_dict_init();				/* proxy, sql, ldap */
    get_mail_conf_str_table(str_table);

    /*
     * This program is designed to be set-gid, which makes it a potential
     * target for attack. Strip and optionally override the process
     * environment so that we don't have to trust the C library.
     */
    import_env = mail_parm_split(VAR_IMPORT_ENVIRON, var_import_environ);
    clean_env(import_env->argv);
    argv_free(import_env);

    if (chdir(var_queue_dir))
	msg_fatal_status(EX_UNAVAILABLE, "chdir %s: %m", var_queue_dir);

    signal(SIGPIPE, SIG_IGN);

    /* End of initializations. */

    /*
     * Further input validation.
     */
    if (site_to_flush != 0) {
	bad_site = 0;
	if (*site_to_flush == '[') {
	    bad_site = !valid_mailhost_literal(site_to_flush, DONT_GRIPE);
	} else {
	    bad_site = !valid_hostname(site_to_flush, DONT_GRIPE);
	}
	if (bad_site)
	    msg_fatal_status(EX_USAGE,
	      "Cannot flush mail queue - invalid destination: \"%.100s%s\"",
		   site_to_flush, strlen(site_to_flush) > 100 ? "..." : "");
    }
    if (id_to_flush != 0) {
	if (!mail_queue_id_ok(id_to_flush))
	    msg_fatal_status(EX_USAGE,
		       "Cannot flush queue ID - invalid name: \"%.100s%s\"",
		       id_to_flush, strlen(id_to_flush) > 100 ? "..." : "");
    }

    /*
     * Start processing.
     */
    switch (mode) {
    default:
	msg_panic("unknown operation mode: %d", mode);
	/* NOTREACHED */
    case PQ_MODE_MAILQ_LIST:
    case PQ_MODE_JSON_LIST:
	show_queue(mode);
	exit(0);
	break;
    case PQ_MODE_FLUSH_SITE:
	flush_site(site_to_flush);
	exit(0);
	break;
    case PQ_MODE_FLUSH_FILE:
	flush_file(id_to_flush);
	exit(0);
	break;
    case PQ_MODE_FLUSH_QUEUE:
	flush_queue();
	exit(0);
	break;
    case PQ_MODE_DEFAULT:
	usage();
	/* NOTREACHED */
    }
}
