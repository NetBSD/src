/*++
/* NAME
/*	master 8
/* SUMMARY
/*	Postfix master process
/* SYNOPSIS
/* .fi
/*	\fBmaster\fR [\fB-c \fIconfig_dir\fR] [\fB-e \fIexit_time\fR]
/*		[\fB-D\fR] [\fB-t\fR] [\fB-v\fR]
/* DESCRIPTION
/*	The \fBmaster\fR daemon is the resident process that runs Postfix
/*	daemons on demand: daemons to send or receive messages via the
/*	network, daemons to deliver mail locally, etc.  These daemons are
/*	created on demand up to a configurable maximum number per service.
/*
/*	Postfix daemons terminate voluntarily, either after being idle for
/*	a configurable amount of time, or after having serviced a
/*	configurable number of requests. The exception to this rule is the
/*	resident Postfix queue manager.
/*
/*	The behavior of the \fBmaster\fR daemon is controlled by the
/*	\fBmaster.cf\fR configuration file. The table specifies zero or
/*	more servers in the \fBUNIX\fR or \fBINET\fR domain, or servers
/*	that take requests from a FIFO. Precise configuration details are
/*	given in the \fBmaster.cf\fR file, and in the manual pages of the
/*	respective daemons.
/*
/*	Options:
/* .IP "\fB-c \fIconfig_dir\fR"
/*	Read the \fBmain.cf\fR and \fBmaster.cf\fR configuration files in
/*	the named directory instead of the default configuration directory.
/* .IP "\fB-e \fIexit_time\fR"
/*	Terminate the master process after \fIexit_time\fR seconds. Child
/*	processes terminate at their convenience.
/* .IP \fB-D\fR
/*	After initialization, run a debugger on the master process. The
/*	debugging command is specified with the \fBdebugger_command\fR in
/*	the \fBmain.cf\fR global configuration file.
/* .IP \fB-t\fR
/*	Test mode. Return a zero exit status when the \fBmaster.pid\fR lock
/*	file does not exist or when that file is not locked.  This is evidence
/*	that the \fBmaster\fR daemon is not running.
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. This option
/*	is passed on to child processes. Multiple \fB-v\fR options
/*	make the software increasingly verbose.
/* .PP
/*	Signals:
/* .IP \fBSIGHUP\fR
/*	Upon receipt of a \fBHUP\fR signal (e.g., after \fBpostfix reload\fR),
/*	the master process re-reads its configuration files. If a service has
/*	been removed from the \fBmaster.cf\fR file, its running processes
/*	are terminated immediately.
/*	Otherwise, running processes are allowed to terminate as soon
/*	as is convenient, so that changes in configuration settings
/*	affect only new service requests.
/* .IP \fBSIGTERM\fR
/*	Upon receipt of a \fBTERM\fR signal (e.g., after \fBpostfix abort\fR),
/*	the master process passes the signal on to its child processes and
/*	terminates.
/*	This is useful for an emergency shutdown. Normally one would
/*	terminate only the master (\fBpostfix stop\fR) and allow running
/*	processes to finish what they are doing.
/* DIAGNOSTICS
/*	Problems are reported to \fBsyslogd\fR(8).
/* BUGS
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
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program. See the Postfix \fBmain.cf\fR file for syntax details
/*	and for default values. Use the \fBpostfix reload\fR command after
/*	a configuration change.
/* .SH Miscellaneous
/* .ad
/* .fi
/* .IP \fBimport_environment\fR
/* .IP \fBexport_environment\fR
/*	Lists of names of environment parameters that can be imported
/*	from (exported to) non-Postfix processes.
/* .IP \fBmail_owner\fR
/*	The owner of the mail queue and of most Postfix processes.
/* .IP \fBcommand_directory\fR
/*	Directory with Postfix support programs.
/* .IP \fBdaemon_directory\fR
/*	Directory with Postfix daemon programs.
/* .IP \fBqueue_directory\fR
/*	Top-level directory of the Postfix queue. This is also the root
/*	directory of Postfix daemons that run chrooted.
/* .SH "Resource controls"
/* .ad
/* .fi
/* .IP \fBdefault_process_limit\fR
/*	Default limit for the number of simultaneous child processes that
/*	provide a given service.
/* .IP \fBmax_idle\fR
/*	Limit the time in seconds that a child process waits between
/*	service requests.
/* .IP \fBmax_use\fR
/*	Limit the number of service requests handled by a child process.
/* .IP \fBservice_throttle_time\fR
/*	Time to avoid forking a server that appears to be broken.
/* FILES
/*	/etc/postfix/main.cf: global configuration file.
/*	/etc/postfix/master.cf: master process configuration file.
/*	/var/spool/postfix/pid/master.pid: master lock file.
/* SEE ALSO
/*	qmgr(8) queue manager
/*	pickup(8) local mail pickup
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

/* Global library. */

#include <mail_params.h>
#include <debug_process.h>
#include <mail_task.h>
#include <mail_conf.h>
#include <open_lock.h>

/* Application-specific. */

#include "master.h"

/* master_exit_event - exit for memory leak testing purposes */

static void master_exit_event(int unused_event, char *unused_context)
{
    msg_info("master exit time has arrived");
    exit(0);
}

/* main - main program */

int     main(int argc, char **argv)
{
    static VSTREAM *lock_fp;
    VSTRING *lock_path;
    off_t   inherited_limit;
    int     debug_me = 0;
    int     ch;
    int     fd;
    int     n;
    int     test_lock = 0;
    int     fd_limit = open_limit(0);
    VSTRING *why;
    WATCHDOG *watchdog;
    ARGV   *import_env;

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
    set_mail_conf_str(VAR_PROCNAME, var_procname);

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
    if (fd_limit > 500)
	fd_limit = 500;
    for (fd = 3; fd < fd_limit; fd++)
	(void) close(fd);

    /*
     * Initialize logging and exit handler.
     */
    msg_syslog_init(mail_task(var_procname), LOG_PID, LOG_FACILITY);

    /*
     * If started from a terminal, get rid of any tty association. This also
     * means that all errors and warnings must go to the syslog daemon.
     */
    for (fd = 0; fd < 3; fd++) {
	(void) close(fd);
	if (open("/dev/null", O_RDWR, 0) != fd)
	    msg_fatal("open /dev/null: %m");
    }
    setsid();

    /*
     * Make some room for plumbing with file descriptors. XXX This breaks
     * when a service listens on many ports. In order to do this right we
     * must change the master-child interface so that descriptors do not need
     * to have fixed numbers.
     */
    for (n = 0; n < 3; n++) {
	if (close_on_exec(dup(0), CLOSE_ON_EXEC) < 0)
	    msg_fatal("dup(0): %m");
    }

    /*
     * Process JCL.
     */
    while ((ch = GETOPT(argc, argv, "c:e:Dtv")) > 0) {
	switch (ch) {
	case 'c':
	    if (setenv(CONF_ENV_PATH, optarg, 1) < 0)
		msg_fatal("out of memory");
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
	    msg_fatal("usage: %s [-c config_dir] [-e exit_time] [-D (debug)] [-t (test)] [-v]", argv[0]);
	    /* NOTREACHED */
	}
    }

    /*
     * Final initializations. Unfortunately, we must read the global Postfix
     * configuration file after doing command-line processing, so that we get
     * consistent results when we SIGHUP the server to reload configuration
     * files.
     */
    master_vars_init();

    /*
     * Environment import filter, to enforce consistent behavior whether
     * Postfix is started by hand, or at system boot time.
     */
    import_env = argv_split(var_import_environ, ", \t\r\n");
    clean_env(import_env->argv);
    argv_free(import_env);

    if ((inherited_limit = get_file_limit()) < (off_t) INT_MAX)
	set_file_limit(INT_MAX);

    if (chdir(var_queue_dir))
	msg_fatal("chdir %s: %m", var_queue_dir);

    /*
     * Lock down the master.pid file. In test mode, no file means that it
     * isn't locked.
     */
    lock_path = vstring_alloc(10);
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

    vstring_free(why);
    vstring_free(lock_path);

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
    msg_info("daemon started");

    /*
     * Process events. The event handler will execute the read/write/timer
     * action routines. Whenever something has happened, see if we received
     * any signal in the mean time. Although the master process appears to do
     * multiple things at the same time, it really is all a single thread, so
     * that there are no concurrency conflicts within the master process.
     */
    watchdog = watchdog_create(1000, (WATCHDOG_FN) 0, (char *) 0);
    for (;;) {
#ifdef HAS_VOLATILE_LOCKS
	if (myflock(vstream_fileno(lock_fp), INTERNAL_LOCK,
		    MYFLOCK_OP_EXCLUSIVE) < 0)
	    msg_fatal("refresh exclusive lock: %m");
#endif
	watchdog_start(watchdog);		/* same as trigger servers */
	event_loop(-1);
	if (master_gotsighup) {
	    msg_info("reload configuration");
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
