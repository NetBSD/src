/*++
/* NAME
/*	postqueue 1
/* SUMMARY
/*	Postfix queue control
/* SYNOPSIS
/*	\fBpostqueue\fR [\fB-c \fIconfig_dir\fR] \fB-f\fR
/* .br
/*	\fBpostqueue\fR [\fB-c \fIconfig_dir\fR] \fB-p\fR
/* .br
/*	\fBpostqueue\fR [\fB-c \fIconfig_dir\fR] \fB-s \fIsite\fR
/* DESCRIPTION
/*	The \fBpostqueue\fR program implements the Postfix user interface
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
/*	This option implements the traditional \fBsendmail -q\fR command,
/*	by contacting the Postfix \fBqmgr\fR(8) daemon.
/*
/*	Warning: flushing undeliverable mail frequently will result in
/*	poor delivery performance of all other mail.
/* .IP \fB-p\fR
/*	Produce a traditional sendmail-style queue listing.
/*	This option implements the traditional \fBmailq\fR command,
/*	by contacting the Postfix \fBshowq\fR(8) daemon.
/*
/*	Each queue entry shows the queue file ID, message
/*	size, arrival time, sender, and the recipients that still need to
/*	be delivered.  If mail could not be delivered upon the last attempt,
/*	the reason for failure is shown. This mode of operation is implemented
/*	by executing the \fBpostqueue\fR(1) command. The queue ID string
/*	is followed by an optional status character:
/* .RS
/* .IP \fB*\fR
/*	The message is in the \fBactive\fR queue, i.e. the message is
/*	selected for delivery.
/* .IP \fB!\fR
/*	The message is in the \fBhold\fR queue, i.e. no further delivery
/*	attempt will be made until the mail is taken off hold.
/* .RE
/* .IP "\fB-s \fIsite\fR"
/*	Schedule immediate delivery of all mail that is queued for the named
/*	\fIsite\fR. The site must be eligible for the "fast flush" service.
/*	See \fBflush\fR(8) for more information about the "fast flush"
/*	service.
/*
/*	This option implements the traditional \fBsendmail -qR\fIsite\fR
/*	command, by contacting the Postfix \fBflush\fR(8) daemon.
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the software increasingly verbose.
/* SECURITY
/* .ad
/* .fi
/*	This program is designed to run with set-group ID privileges, so
/*	that it can connect to Postfix daemon processes.
/* DIAGNOSTICS
/*	Problems are logged to \fBsyslogd\fR(8) and to the standard error
/*	stream.
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
/*	postconf(5) for more details including examples.
/* .IP "\fBalternate_config_directories (empty)\fR"
/*	A list of non-default Postfix configuration directories that may
/*	be specified with "-c config_directory" on the command line, or
/*	via the MAIL_CONFIG environment parameter.
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBcommand_directory (see 'postconf -d' output)\fR"
/*	The location of all postfix administrative commands.
/* .IP "\fBfast_flush_domains ($relay_domains)\fR"
/*	Optional list of destinations that are eligible for per-destination
/*	logfiles with mail that is queued to those destinations.
/* .IP "\fBimport_environment (see 'postconf -d' output)\fR"
/*	The list of environment parameters that a Postfix process will
/*	import from a non-Postfix parent process.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (postfix)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* .IP "\fBtrigger_timeout (10s)\fR"
/*	The time limit for sending a trigger to a Postfix daemon (for
/*	example, the pickup(8) or qmgr(8) daemon).
/* FILES
/*	/var/spool/postfix, mail queue
/* SEE ALSO
/*	qmgr(8), queue manager
/*	showq(8), list mail queue
/*	flush(8), fast flush service
/*	sendmail(1), Sendmail-compatible user interface
/*	postsuper(1), privileged queue operations
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
#include <msg_syslog.h>
#include <argv.h>
#include <safe.h>
#include <connect.h>
#include <valid_hostname.h>

/* Global library. */

#include <mail_proto.h>
#include <mail_params.h>
#include <mail_conf.h>
#include <mail_task.h>
#include <debug_process.h>
#include <mail_run.h>
#include <mail_flush.h>
#include <flush_clnt.h>
#include <smtp_stream.h>

/* Application-specific. */

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
  */
#define PQ_MODE_DEFAULT		0	/* noop */
#define PQ_MODE_MAILQ_LIST	1	/* list mail queue */
#define PQ_MODE_FLUSH_QUEUE	2	/* flush queue */
#define PQ_MODE_FLUSH_SITE	3	/* flush site */

 /*
  * Silly little macros (SLMs).
  */
#define STR	vstring_str

/* show_queue - show queue status */

static void show_queue(void)
{
    char    buf[VSTREAM_BUFSIZE];
    VSTREAM *showq;
    int     n;

    /*
     * Connect to the show queue service. Terminate silently when piping into
     * a program that terminates early.
     */
    if ((showq = mail_connect(MAIL_CLASS_PUBLIC, var_showq_service, BLOCKING)) != 0) {
	while ((n = vstream_fread(showq, buf, sizeof(buf))) > 0) {
	    if (vstream_fwrite(VSTREAM_OUT, buf, n) != n
		|| vstream_fflush(VSTREAM_OUT) != 0) {
		if (errno == EPIPE)
		    break;
		msg_fatal("write error: %m");
	    }
	}
	if (vstream_fclose(showq) && errno != EPIPE)
	    msg_warn("close: %m");
    }

    /*
     * Don't assume that the mail system is down when the user has
     * insufficient permission to access the showq socket.
     */
    else if (errno == EACCES) {
	msg_fatal_status(EX_SOFTWARE,
			 "Connect to the %s %s service: %m",
			 var_mail_name, var_showq_service);
    }

    /*
     * When the mail system is down, the superuser can still access the queue
     * directly. Just run the showq program in stand-alone mode.
     */
    else if (geteuid() == 0) {
	ARGV   *argv;
	int     stat;

	msg_warn("Mail system is down -- accessing queue directly");
	argv = argv_alloc(6);
	argv_add(argv, var_showq_service, "-u", "-S", (char *) 0);
	for (n = 0; n < msg_verbose; n++)
	    argv_add(argv, "-v", (char *) 0);
	argv_terminate(argv);
	stat = mail_run_foreground(var_daemon_dir, argv->argv);
	argv_free(argv);
    }

    /*
     * When the mail system is down, unprivileged users are stuck, because by
     * design the mail system contains no set_uid programs. The only way for
     * an unprivileged user to cross protection boundaries is to talk to the
     * showq daemon.
     */
    else {
	msg_fatal_status(EX_UNAVAILABLE,
			 "Queue report unavailable - mail system is down");
    }
}

/* flush_queue - force delivery */

static void flush_queue(void)
{

    /*
     * Trigger the flush queue service.
     */
    if (mail_flush_deferred() < 0)
	msg_fatal_status(EX_UNAVAILABLE,
			 "Cannot flush mail queue - mail system is down");
    if (mail_flush_maildrop() < 0)
	msg_fatal_status(EX_UNAVAILABLE,
			 "Cannot flush mail queue - mail system is down");
}

/* flush_site - flush mail for site */

static void flush_site(const char *site)
{
    int     status;

    flush_init();

    switch (status = flush_send(site)) {
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

/* usage - scream and die */

static NORETURN usage(void)
{
    msg_fatal_status(EX_USAGE, "usage: postqueue -f | postqueue -p | postqueue -s site");
}

/* main - the main program */

int     main(int argc, char **argv)
{
    struct stat st;
    char   *slash;
    int     c;
    int     fd;
    int     mode = PQ_MODE_DEFAULT;
    char   *site_to_flush = 0;
    ARGV   *import_env;
    char   *last;
    int     bad_site;

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
     * Initialize. Set up logging, read the global configuration file and
     * extract configuration information. Set up signal handlers so that we
     * can clean up incomplete output.
     */
    if ((slash = strrchr(argv[0], '/')) != 0 && slash[1])
	argv[0] = slash + 1;
    msg_vstream_init(argv[0], VSTREAM_ERR);
    msg_syslog_init(mail_task("postqueue"), LOG_PID, LOG_FACILITY);
    set_mail_conf_str(VAR_PROCNAME, var_procname = mystrdup(argv[0]));

    /*
     * Parse JCL. This program is set-gid and must sanitize all command-line
     * parameters. The configuration directory argument is validated by the
     * mail configuration read routine. Don't do complex things until we have
     * completed initializations.
     */
    while ((c = GETOPT(argc, argv, "c:fps:v")) > 0) {
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

    /*
     * This program is designed to be set-gid, which makes it a potential
     * target for attack. If not running as root, strip the environment so we
     * don't have to trust the C library. If running as root, don't strip the
     * environment so that showq can receive non-default configuration
     * directory info when the mail system is down.
     */
    if (geteuid() != 0) {
	import_env = argv_split(var_import_environ, ", \t\r\n");
	clean_env(import_env->argv);
	argv_free(import_env);
    }
    if (chdir(var_queue_dir))
	msg_fatal_status(EX_UNAVAILABLE, "chdir %s: %m", var_queue_dir);

    signal(SIGPIPE, SIG_IGN);

    /* End of initializations. */

    /*
     * Further input validation.
     */
    if (site_to_flush != 0) {
	bad_site = 0;
	if (*site_to_flush == '['
	    && *(last = site_to_flush + strlen(site_to_flush) - 1) == ']') {
	    *last = 0;
	    bad_site = !valid_hostaddr(site_to_flush + 1, DONT_GRIPE);
	    *last = ']';
	} else {
	    bad_site = (!valid_hostname(site_to_flush, DONT_GRIPE)
			&& !valid_hostaddr(site_to_flush, DONT_GRIPE));
	}
	if (bad_site)
	    msg_fatal_status(EX_USAGE,
	      "Cannot flush mail queue - invalid destination: \"%.100s%s\"",
		   site_to_flush, strlen(site_to_flush) > 100 ? "..." : "");
    }

    /*
     * Start processing.
     */
    switch (mode) {
    default:
	msg_panic("unknown operation mode: %d", mode);
	/* NOTREACHED */
    case PQ_MODE_MAILQ_LIST:
	show_queue();
	exit(0);
	break;
    case PQ_MODE_FLUSH_SITE:
	flush_site(site_to_flush);
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
