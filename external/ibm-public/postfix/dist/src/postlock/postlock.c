/*	$NetBSD: postlock.c,v 1.1.1.1.16.1 2013/02/25 00:27:25 tls Exp $	*/

/*++
/* NAME
/*	postlock 1
/* SUMMARY
/*	lock mail folder and execute command
/* SYNOPSIS
/* .fi
/*	\fBpostlock\fR [\fB-c \fIconfig_dir\fB] [\fB-l \fIlock_style\fB]
/*		[\fB-v\fR] \fIfile command...\fR
/* DESCRIPTION
/*	The \fBpostlock\fR(1) command locks \fIfile\fR for exclusive
/*	access, and executes \fIcommand\fR. The locking method is
/*	compatible with the Postfix UNIX-style local delivery agent.
/*
/*	Options:
/* .IP "\fB-c \fIconfig_dir\fR"
/*	Read the \fBmain.cf\fR configuration file in the named directory
/*	instead of the default configuration directory.
/* .IP "\fB-l \fIlock_style\fR"
/*	Override the locking method specified via the
/*	\fBmailbox_delivery_lock\fR configuration parameter (see below).
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the software increasingly verbose.
/* .PP
/*	Arguments:
/* .IP \fIfile\fR
/*	A mailbox file. The user should have read/write permission.
/* .IP \fIcommand...\fR
/*	The command to execute while \fIfile\fR is locked for exclusive
/*	access.  The command is executed directly, i.e. without
/*	interpretation by a shell command interpreter.
/* DIAGNOSTICS
/*	The result status is 75 (EX_TEMPFAIL) when \fBpostlock\fR(1)
/*	could not perform the requested operation.  Otherwise, the
/*	exit status is the exit status from the command.
/* BUGS
/*	With remote file systems, the ability to acquire a lock does not
/*	necessarily eliminate access conflicts. Avoid file access by
/*	processes running on different machines.
/* ENVIRONMENT
/* .ad
/* .fi
/* .IP \fBMAIL_CONFIG\fR
/*	Directory with Postfix configuration files.
/* .IP \fBMAIL_VERBOSE\fR
/*	Enable verbose logging for debugging purposes.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program.
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* LOCKING CONTROLS
/* .ad
/* .fi
/* .IP "\fBdeliver_lock_attempts (20)\fR"
/*	The maximal number of attempts to acquire an exclusive lock on a
/*	mailbox file or \fBbounce\fR(8) logfile.
/* .IP "\fBdeliver_lock_delay (1s)\fR"
/*	The time between attempts to acquire an exclusive lock on a mailbox
/*	file or \fBbounce\fR(8) logfile.
/* .IP "\fBstale_lock_time (500s)\fR"
/*	The time after which a stale exclusive mailbox lockfile is removed.
/* .IP "\fBmailbox_delivery_lock (see 'postconf -d' output)\fR"
/*	How to lock a UNIX-style \fBlocal\fR(8) mailbox before attempting delivery.
/* RESOURCE AND RATE CONTROLS
/* .ad
/* .fi
/* .IP "\fBfork_attempts (5)\fR"
/*	The maximal number of attempts to fork() a child process.
/* .IP "\fBfork_delay (1s)\fR"
/*	The delay between attempts to fork() a child process.
/* MISCELLANEOUS CONTROLS
/* .ad
/* .fi
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* SEE ALSO
/*	postconf(5), configuration parameters
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
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <msg_vstream.h>
#include <iostuff.h>
#include <warn_stat.h>

/* Global library. */

#include <mail_params.h>
#include <mail_version.h>
#include <dot_lockfile.h>
#include <deliver_flock.h>
#include <mail_conf.h>
#include <sys_exits.h>
#include <mbox_conf.h>
#include <mbox_open.h>
#include <dsn_util.h>

/* Application-specific. */

/* usage - explain */

static NORETURN usage(char *myname)
{
    msg_fatal("usage: %s [-c config_dir] [-l lock_style] [-v] folder command...", myname);
}

/* fatal_exit - all failures are deemed recoverable */

static void fatal_exit(void)
{
    exit(EX_TEMPFAIL);
}

MAIL_VERSION_STAMP_DECLARE;

/* main - go for it */

int     main(int argc, char **argv)
{
    DSN_BUF *why;
    char   *folder;
    char  **command;
    int     ch;
    int     fd;
    struct stat st;
    int     count;
    WAIT_STATUS_T status;
    pid_t   pid;
    int     lock_mask;
    char   *lock_style = 0;
    MBOX   *mp;

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
	    msg_fatal("open /dev/null: %m");

    /*
     * Process environment options as early as we can. We are not set-uid,
     * and we are supposed to be running in a controlled environment.
     */
    if (getenv(CONF_ENV_VERB))
	msg_verbose = 1;

    /*
     * Set up logging and error handling. Intercept fatal exits so we can
     * return a distinguished exit status.
     */
    msg_vstream_init(argv[0], VSTREAM_ERR);
    msg_cleanup(fatal_exit);

    /*
     * Parse JCL.
     */
    while ((ch = GETOPT(argc, argv, "c:l:v")) > 0) {
	switch (ch) {
	default:
	    usage(argv[0]);
	    break;
	case 'c':
	    if (setenv(CONF_ENV_PATH, optarg, 1) < 0)
		msg_fatal("out of memory");
	    break;
	case 'l':
	    lock_style = optarg;
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	}
    }
    if (optind + 2 > argc)
	usage(argv[0]);
    folder = argv[optind];
    command = argv + optind + 1;

    /*
     * Read the config file. The command line lock style can override the
     * configured lock style.
     */
    mail_conf_read();
    lock_mask = mbox_lock_mask(lock_style ? lock_style :
	       get_mail_conf_str(VAR_MAILBOX_LOCK, DEF_MAILBOX_LOCK, 1, 0));

    /*
     * Lock the folder for exclusive access. Lose the lock upon exit. The
     * command is not supposed to disappear into the background.
     */
    why = dsb_create();
    if ((mp = mbox_open(folder, O_APPEND | O_WRONLY | O_CREAT,
			S_IRUSR | S_IWUSR, (struct stat *) 0,
			-1, -1, lock_mask, "5.2.0", why)) == 0)
	msg_fatal("open file %s: %s", folder, vstring_str(why->reason));
    dsb_free(why);

    /*
     * Run the command. Remove the lock after completion.
     */
    for (count = 1; (pid = fork()) == -1; count++) {
	msg_warn("fork %s: %m", command[0]);
	if (count >= var_fork_tries) {
	    mbox_release(mp);
	    exit(EX_TEMPFAIL);
	}
	sleep(var_fork_delay);
    }
    switch (pid) {
    case 0:
	(void) msg_cleanup((MSG_CLEANUP_FN) 0);
	execvp(command[0], command);
	msg_fatal("execvp %s: %m", command[0]);
    default:
	if (waitpid(pid, &status, 0) < 0)
	    msg_fatal("waitpid: %m");
	vstream_fclose(mp->fp);
	mbox_release(mp);
	exit(WIFEXITED(status) ? WEXITSTATUS(status) : 1);
    }
}
