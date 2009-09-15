/*	$NetBSD: master.c,v 1.1.1.1.2.2 2009/09/15 06:02:57 snj Exp $	*/

/*++
/* NAME
/*	master 8
/* SUMMARY
/*	Postfix master process
/* SYNOPSIS
/*	\fBmaster\fR [\fB-Ddtv\fR] [\fB-c \fIconfig_dir\fR] [\fB-e \fIexit_time\fR]
/* DESCRIPTION
/*	The \fBmaster\fR(8) daemon is the resident process that runs Postfix
/*	daemons on demand: daemons to send or receive messages via the
/*	network, daemons to deliver mail locally, etc.  These daemons are
/*	created on demand up to a configurable maximum number per service.
/*
/*	Postfix daemons terminate voluntarily, either after being idle for
/*	a configurable amount of time, or after having serviced a
/*	configurable number of requests. Exceptions to this rule are the
/*	resident queue manager, address verification server, and the TLS
/*	session cache and pseudo-random number server.
/*
/*	The behavior of the \fBmaster\fR(8) daemon is controlled by the
/*	\fBmaster.cf\fR configuration file, as described in \fBmaster\fR(5).
/*
/*	Options:
/* .IP "\fB-c \fIconfig_dir\fR"
/*	Read the \fBmain.cf\fR and \fBmaster.cf\fR configuration files in
/*	the named directory instead of the default configuration directory.
/*	This also overrides the configuration files for other Postfix
/*	daemon processes.
/* .IP \fB-D\fR
/*	After initialization, run a debugger on the master process. The
/*	debugging command is specified with the \fBdebugger_command\fR in
/*	the \fBmain.cf\fR global configuration file.
/* .IP \fB-d\fR
/*	Do not redirect stdin, stdout or stderr to /dev/null, and
/*	do not discard the controlling terminal. This must be used
/*	for debugging only.
/* .IP "\fB-e \fIexit_time\fR"
/*	Terminate the master process after \fIexit_time\fR seconds. Child
/*	processes terminate at their convenience.
/* .IP \fB-t\fR
/*	Test mode. Return a zero exit status when the \fBmaster.pid\fR lock
/*	file does not exist or when that file is not locked.  This is evidence
/*	that the \fBmaster\fR(8) daemon is not running.
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. This option
/*	is passed on to child processes. Multiple \fB-v\fR options
/*	make the software increasingly verbose.
/* .PP
/*	Signals:
/* .IP \fBSIGHUP\fR
/*	Upon receipt of a \fBHUP\fR signal (e.g., after "\fBpostfix reload\fR"),
/*	the master process re-reads its configuration files. If a service has
/*	been removed from the \fBmaster.cf\fR file, its running processes
/*	are terminated immediately.
/*	Otherwise, running processes are allowed to terminate as soon
/*	as is convenient, so that changes in configuration settings
/*	affect only new service requests.
/* .IP \fBSIGTERM\fR
/*	Upon receipt of a \fBTERM\fR signal (e.g., after "\fBpostfix abort\fR"),
/*	the master process passes the signal on to its child processes and
/*	terminates.
/*	This is useful for an emergency shutdown. Normally one would
/*	terminate only the master ("\fBpostfix stop\fR") and allow running
/*	processes to finish what they are doing.
/* DIAGNOSTICS
/*	Problems are reported to \fBsyslogd\fR(8).
/* ENVIRONMENT
/* .ad
/* .fi
/* .IP \fBMAIL_DEBUG\fR
/*	After initialization, start a debugger as specified with the
/*	\fBdebugger_command\fR configuration parameter in the \fBmain.cf\fR
/*	configuration file.
/* .IP \fBMAIL_CONFIG\fR
/*	Directory with Postfix configuration files.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Unlike most Postfix daemon processes, the \fBmaster\fR(8) server does
/*	not automatically pick up changes to \fBmain.cf\fR. Changes
/*	to \fBmaster.cf\fR are never picked up automatically.
/*	Use the "\fBpostfix reload\fR" command after a configuration change.
/* RESOURCE AND RATE CONTROLS
/* .ad
/* .fi
/* .IP "\fBdefault_process_limit (100)\fR"
/*	The default maximal number of Postfix child processes that provide
/*	a given service.
/* .IP "\fBmax_idle (100s)\fR"
/*	The maximum amount of time that an idle Postfix daemon process waits
/*	for an incoming connection before terminating voluntarily.
/* .IP "\fBmax_use (100)\fR"
/*	The maximal number of incoming connections that a Postfix daemon
/*	process will service before terminating voluntarily.
/* .IP "\fBservice_throttle_time (60s)\fR"
/*	How long the Postfix \fBmaster\fR(8) waits before forking a server that
/*	appears to be malfunctioning.
/* .PP
/*	Available in Postfix version 2.6 and later:
/* .IP "\fBmaster_service_disable (empty)\fR"
/*	Selectively disable \fBmaster\fR(8) listener ports by service type
/*	or by service name and type.
/* MISCELLANEOUS CONTROLS
/* .ad
/* .fi
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdaemon_directory (see 'postconf -d' output)\fR"
/*	The directory with Postfix support programs and daemon programs.
/* .IP "\fBdebugger_command (empty)\fR"
/*	The external command to execute when a Postfix daemon program is
/*	invoked with the -D option.
/* .IP "\fBinet_interfaces (all)\fR"
/*	The network interface addresses that this mail system receives
/*	mail on.
/* .IP "\fBinet_protocols (ipv4)\fR"
/*	The Internet protocols Postfix will attempt to use when making
/*	or accepting connections.
/* .IP "\fBimport_environment (see 'postconf -d' output)\fR"
/*	The list of environment parameters that a Postfix process will
/*	import from a non-Postfix parent process.
/* .IP "\fBmail_owner (postfix)\fR"
/*	The UNIX system account that owns the Postfix queue and most Postfix
/*	daemon processes.
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
/* .ad
/* .fi
/*	To expand the directory names below into their actual values,
/*	use the command "\fBpostconf config_directory\fR" etc.
/* .na
/* .nf
/*
/*	$config_directory/main.cf, global configuration file.
/*	$config_directory/master.cf, master server configuration file.
/*	$queue_directory/pid/master.pid, master lock file.
/*	$data_directory/master.lock, master lock file.
/* SEE ALSO
/*	qmgr(8), queue manager
/*	verify(8), address verification
/*	master(5), master.cf configuration file syntax
/*	postconf(5), main.cf configuration parameter syntax
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

/* System libraries. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <syslog.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>

/* Utility library. */

#include <events.h>
#include <msg.h>
#include <msg_syslog.h>
#include <vstring.h>
#include <mymalloc.h>
#include <iostuff.h>
#include <vstream.h>
#include <stringops.h>
#include <myflock.h>
#include <watchdog.h>
#include <clean_env.h>
#include <argv.h>
#include <safe.h>
#include <set_eugid.h>
#include <set_ugid.h>

/* Global library. */

#include <mail_params.h>
#include <mail_version.h>
#include <debug_process.h>
#include <mail_task.h>
#include <mail_conf.h>
#include <open_lock.h>
#include <inet_proto.h>

/* Application-specific. */

#include "master.h"

int     master_detach = 1;

/* master_exit_event - exit for memory leak testing purposes */

static void master_exit_event(int unused_event, char *unused_context)
{
    msg_info("master exit time has arrived");
    exit(0);
}

/* usage - show hint and terminate */

static NORETURN usage(const char *me)
{
    msg_fatal("usage: %s [-c config_dir] [-D (debug)] [-d (don't detach from terminal)] [-e exit_time] [-t (test)] [-v]", me);
}

MAIL_VERSION_STAMP_DECLARE;

/* main - main program */

int     main(int argc, char **argv)
{
    static VSTREAM *lock_fp;
    static VSTREAM *data_lock_fp;
    VSTRING *lock_path;
    VSTRING *data_lock_path;
    off_t   inherited_limit;
    int     debug_me = 0;
    int     ch;
    int     fd;
    int     n;
    int     test_lock = 0;
    VSTRING *why;
    WATCHDOG *watchdog;
    ARGV   *import_env;

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    /*
     * Initialize.
     */
    umask(077);					/* never fails! */

    /*
     * Process environment options as early as we can.
     */
    if (getenv(CONF_ENV_VERB))
	msg_verbose = 1;
    if (getenv(CONF_ENV_DEBUG))
	debug_me = 1;

    /*
     * Don't die when a process goes away unexpectedly.
     */
    signal(SIGPIPE, SIG_IGN);

    /*
     * Strip and save the process name for diagnostics etc.
     */
    var_procname = mystrdup(basename(argv[0]));

    /*
     * When running a child process, don't leak any open files that were
     * leaked to us by our own (privileged) parent process. Descriptors 0-2
     * are taken care of after we have initialized error logging.
     * 
     * Some systems such as AIX have a huge per-process open file limit. In
     * those cases, limit the search for potential file descriptor leaks to
     * just the first couple hundred.
     * 
     * The Debian post-installation script passes an open file descriptor into
     * the master process and waits forever for someone to close it. Because
     * of this we have to close descriptors > 2, and pray that doing so does
     * not break things.
     */
    closefrom(3);

    /*
     * Initialize logging and exit handler.
     */
    msg_syslog_init(mail_task(var_procname), LOG_PID, LOG_FACILITY);

    /*
     * The mail system must be run by the superuser so it can revoke
     * privileges for selected operations. That's right - it takes privileges
     * to toss privileges.
     */
    if (getuid() != 0)
	msg_fatal("the master command is reserved for the superuser");
    if (unsafe() != 0)
	msg_fatal("the master command must not run as a set-uid process");

    /*
     * Process JCL.
     */
    while ((ch = GETOPT(argc, argv, "c:Dde:tv")) > 0) {
	switch (ch) {
	case 'c':
	    if (setenv(CONF_ENV_PATH, optarg, 1) < 0)
		msg_fatal("out of memory");
	    break;
	case 'd':
	    master_detach = 0;
	    break;
	case 'e':
	    event_request_timer(master_exit_event, (char *) 0, atoi(optarg));
	    break;
	case 'D':
	    debug_me = 1;
	    break;
	case 't':
	    test_lock = 1;
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	default:
	    usage(argv[0]);
	    /* NOTREACHED */
	}
    }

    /*
     * This program takes no other arguments.
     */
    if (argc > optind)
	usage(argv[0]);

    /*
     * If started from a terminal, get rid of any tty association. This also
     * means that all errors and warnings must go to the syslog daemon.
     */
    if (master_detach)
	for (fd = 0; fd < 3; fd++) {
	    (void) close(fd);
	    if (open("/dev/null", O_RDWR, 0) != fd)
		msg_fatal("open /dev/null: %m");
	}

    /*
     * Run in a separate process group, so that "postfix stop" can terminate
     * all MTA processes cleanly. Give up if we can't separate from our
     * parent process. We're not supposed to blow away the parent.
     */
    if (debug_me == 0 && master_detach != 0 && setsid() == -1 && getsid(0) != getpid())
	msg_fatal("unable to set session and process group ID: %m");

    /*
     * Make some room for plumbing with file descriptors. XXX This breaks
     * when a service listens on many ports. In order to do this right we
     * must change the master-child interface so that descriptors do not need
     * to have fixed numbers.
     * 
     * In a child we need two descriptors for the flow control pipe, one for
     * child->master status updates and at least one for listening.
     */
    for (n = 0; n < 5; n++) {
	if (close_on_exec(dup(0), CLOSE_ON_EXEC) < 0)
	    msg_fatal("dup(0): %m");
    }

    /*
     * Final initializations. Unfortunately, we must read the global Postfix
     * configuration file after doing command-line processing, so that we get
     * consistent results when we SIGHUP the server to reload configuration
     * files.
     */
    master_vars_init();

    /*
     * In case of multi-protocol support. This needs to be done because
     * master does not invoke mail_params_init() (it was written before that
     * code existed).
     */
    (void) inet_proto_init(VAR_INET_PROTOCOLS, var_inet_protocols);

    /*
     * Environment import filter, to enforce consistent behavior whether
     * Postfix is started by hand, or at system boot time.
     */
    import_env = argv_split(var_import_environ, ", \t\r\n");
    clean_env(import_env->argv);
    argv_free(import_env);

    if ((inherited_limit = get_file_limit()) < 0)
	set_file_limit(OFF_T_MAX);

    if (chdir(var_queue_dir))
	msg_fatal("chdir %s: %m", var_queue_dir);

    /*
     * Lock down the master.pid file. In test mode, no file means that it
     * isn't locked.
     */
    lock_path = vstring_alloc(10);
    data_lock_path = vstring_alloc(10);
    why = vstring_alloc(10);

    vstring_sprintf(lock_path, "%s/%s.pid", DEF_PID_DIR, var_procname);
    if (test_lock && access(vstring_str(lock_path), F_OK) < 0)
	exit(0);
    lock_fp = open_lock(vstring_str(lock_path), O_RDWR | O_CREAT, 0644, why);
    if (test_lock)
	exit(lock_fp ? 0 : 1);
    if (lock_fp == 0)
	msg_fatal("open lock file %s: %s",
		  vstring_str(lock_path), vstring_str(why));
    vstream_fprintf(lock_fp, "%*lu\n", (int) sizeof(unsigned long) * 4,
		    (unsigned long) var_pid);
    if (vstream_fflush(lock_fp))
	msg_fatal("cannot update lock file %s: %m", vstring_str(lock_path));
    close_on_exec(vstream_fileno(lock_fp), CLOSE_ON_EXEC);

    /*
     * Lock down the Postfix-writable data directory.
     */
    vstring_sprintf(data_lock_path, "%s/%s.lock", var_data_dir, var_procname);
    set_eugid(var_owner_uid, var_owner_gid);
    data_lock_fp =
	open_lock(vstring_str(data_lock_path), O_RDWR | O_CREAT, 0644, why);
    set_ugid(getuid(), getgid());
    if (data_lock_fp == 0)
	msg_fatal("open lock file %s: %s",
		  vstring_str(data_lock_path), vstring_str(why));
    vstream_fprintf(data_lock_fp, "%*lu\n", (int) sizeof(unsigned long) * 4,
		    (unsigned long) var_pid);
    if (vstream_fflush(data_lock_fp))
	msg_fatal("cannot update lock file %s: %m", vstring_str(data_lock_path));
    close_on_exec(vstream_fileno(data_lock_fp), CLOSE_ON_EXEC);

    /*
     * Clean up.
     */
    vstring_free(why);
    vstring_free(lock_path);
    vstring_free(data_lock_path);

    /*
     * Optionally start the debugger on ourself.
     */
    if (debug_me)
	debug_process();

    /*
     * Finish initialization, last part. We must process configuration files
     * after processing command-line parameters, so that we get consistent
     * results when we SIGHUP the server to reload configuration files.
     */
    master_config();
    master_sigsetup();
    master_flow_init();
    msg_info("daemon started -- version %s, configuration %s",
	     var_mail_version, var_config_dir);

    /*
     * Process events. The event handler will execute the read/write/timer
     * action routines. Whenever something has happened, see if we received
     * any signal in the mean time. Although the master process appears to do
     * multiple things at the same time, it really is all a single thread, so
     * that there are no concurrency conflicts within the master process.
     */
#define MASTER_WATCHDOG_TIME	1000

    watchdog = watchdog_create(MASTER_WATCHDOG_TIME, (WATCHDOG_FN) 0, (char *) 0);
    for (;;) {
#ifdef HAS_VOLATILE_LOCKS
	if (myflock(vstream_fileno(lock_fp), INTERNAL_LOCK,
		    MYFLOCK_OP_EXCLUSIVE) < 0)
	    msg_fatal("refresh exclusive lock: %m");
	if (myflock(vstream_fileno(data_lock_fp), INTERNAL_LOCK,
		    MYFLOCK_OP_EXCLUSIVE) < 0)
	    msg_fatal("refresh exclusive lock: %m");
#endif
	watchdog_start(watchdog);		/* same as trigger servers */
	event_loop(MASTER_WATCHDOG_TIME / 2);
	if (master_gotsighup) {
	    msg_info("reload -- version %s, configuration %s",
		     var_mail_version, var_config_dir);
	    master_gotsighup = 0;		/* this first */
	    master_vars_init();			/* then this */
	    master_refresh();			/* then this */
	}
	if (master_gotsigchld) {
	    if (msg_verbose)
		msg_info("got sigchld");
	    master_gotsigchld = 0;		/* this first */
	    master_reap_child();		/* then this */
	}
    }
}
