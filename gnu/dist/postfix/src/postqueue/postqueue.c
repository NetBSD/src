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
/*	for queue management. It implements all the operations that are
/*	traditionally available via the \fBsendmail\fR(1) command.
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
/* .IP \fB-p\fR
/*	Produce a traditional sendmail-style queue listing.
/*
/*	This option implements the traditional \fBmailq\fR command,
/*	by contacting the Postfix \fBshowq\fR(8) daemon.
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
/*	Directory with the \fBmain.cf\fR file.
/*
/*	In order to avoid exploitation of set-group ID privileges, it is not
/*	possible to specify arbitrary directory names.
/*
/*	A non-standard directory is allowed only if the name is listed in the
/*	standard \fBmain.cf\fR file, in the \fBalternate_config_directories\fR
/*	configuration parameter value.
/*
/*	Only the super-user is allowed to specify arbitrary directory names.
/* FILES
/*	/var/spool/postfix, mail queue
/*	/etc/postfix, configuration files
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/* .IP \fBimport_environment\fR
/*	List of names of environment parameters that can be imported
/*	from non-Postfix processes.
/* .IP \fBqueue_directory\fR
/*	Top-level directory of the Postfix queue. This is also the root
/*	directory of Postfix daemons that run chrooted.
/* .IP \fBfast_flush_domains\fR
/*	List of domains that will receive "fast flush" service (default: all
/*	domains that this system is willing to relay mail to). This list
/*	specifies the domains that Postfix accepts in the SMTP \fBETRN\fR
/*	request and in the \fBsendmail -qR\fR command.
/* SEE ALSO
/*	sendmail(8) sendmail-compatible user interface
/*	qmgr(8) queue manager
/*	showq(8) list mail queue
/*	flush(8) fast flush service
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
    if ((showq = mail_connect(MAIL_CLASS_PUBLIC, MAIL_SERVICE_SHOWQ, BLOCKING)) != 0) {
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
			 var_mail_name, MAIL_SERVICE_SHOWQ);
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
	argv_add(argv, MAIL_SERVICE_SHOWQ, "-u", "-S", (char *) 0);
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
}

/* flush_site - flush mail for site */

static void flush_site(const char *site)
{
    int     status;

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
    if ((slash = strrchr(argv[0], '/')) != 0)
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
