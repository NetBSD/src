/*++
/* NAME
/*	postlock 1
/* SUMMARY
/*	lock mail folder and execute command
/* SYNOPSIS
/* .fi
/*	\fBpostlock\fR [\fB-c \fIconfig_dir\fB] [\fB-v\fR]
/*		\fIfile command...\fR
/* DESCRIPTION
/*	The \fBpostlock\fR command locks \fIfile\fR for exclusive
/*	access, and executes \fIcommand\fR. The locking method is
/*	compatible with the Postfix UNIX-style local delivery agent.
/*
/*	Options:
/* .IP "\fB-c \fIconfig_dir\fR"
/*	Read configuration information from \fBmain.cf\fR in the named
/*	configuration directory.
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
/*	The result status is 75 (EX_TEMPFAIL) when the file is locked by
/*	another process, 255 (on some systems: -1) when \fBpostlock\fR
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
/*      The following \fBmain.cf\fR parameters are especially relevant to
/*      this program. See the Postfix \fBmain.cf\fR file for syntax details
/*      and for default values.
/* .SH "Locking controls"
/* .ad
/* .fi
/* .IP \fBdeliver_lock_attempts\fR
/*	Limit the number of attempts to acquire an exclusive lock.
/* .IP \fBdeliver_lock_delay\fR
/*	Time in seconds between successive attempts to acquire
/*	an exclusive lock.
/* .IP \fBstale_lock_time\fR
/*	Limit the time after which a stale lock is removed.
/* .SH "Resource controls"
/* .ad
/* .fi
/* .IP \fBfork_attempts\fR
/*	Number of attempts to \fBfork\fR() a process before giving up.
/* .IP \fBfork_delay\fR
/*	Delay in seconds between successive \fBfork\fR() attempts.
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

/* Global library. */

#include <mail_params.h>
#include <dot_lockfile.h>
#include <deliver_flock.h>
#include <mail_conf.h>
#include <sys_exits.h>

/* Application-specific. */

/* usage - explain */

static NORETURN usage(char *myname)
{
    msg_fatal("usage: %s [-c config_dir] [-v] folder command...", myname);
}

/* fatal_exit - as promised, return 255 in case of unexpected problems */

static void fatal_exit(void)
{
    exit(255);
}

/* main - go for it */

int     main(int argc, char **argv)
{
    VSTRING *why;
    char   *folder;
    char  **command;
    int     ch;
    int     fd;
    struct stat st;
    int     count;
    WAIT_STATUS_T status;
    pid_t   pid;

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
    while ((ch = GETOPT(argc, argv, "c:v")) > 0) {
	switch (ch) {
	default:
	    usage(argv[0]);
	    break;
	case 'c':
	    if (setenv(CONF_ENV_PATH, optarg, 1) < 0)
		msg_fatal("out of memory");
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
     * Read the config file.
     */
    mail_conf_read();

    /*
     * Lock the folder for exclusive access. Lose the lock upon exit. The
     * command is not supposed to disappear into the background.
     */
    if ((fd = open(folder, O_RDWR, 0)) < 0)
	msg_fatal("open %s: %m", folder);
    close_on_exec(fd, CLOSE_ON_EXEC);
    why = vstring_alloc(1);
#ifdef USE_DOT_LOCK
    if (dot_lockfile(folder, why) < 0) {
	if (errno == EEXIST) {
	    msg_warn("%s", vstring_str(why));
	    exit(EX_TEMPFAIL);
	}
	msg_fatal("%s", vstring_str(why));
    }
#endif
    if (deliver_flock(fd, why) < 0) {
	if (errno == EAGAIN) {
	    msg_warn("file %s: %s", folder, vstring_str(why));
#ifdef USE_DOT_LOCK
	    dot_unlockfile(folder);
#endif
	    exit(EX_TEMPFAIL);
	}
	msg_fatal("file %s: %s", folder, vstring_str(why));
    }

    /*
     * Run the command. Remove the lock after completion.
     */
    for (count = 0; count < var_fork_tries; count++) {
	switch (pid = fork()) {
	case -1:
	    msg_warn("fork %s: %m", command[0]);
	    break;
	case 0:
	    execvp(command[0], command);
	    msg_fatal("execvp %s: %m", command[0]);
	    /* NOTREACHED */
	default:
	    if (waitpid(pid, &status, 0) < 0)
		msg_fatal("waitpid: %m");
#ifdef USE_DOT_LOCK
	    dot_unlockfile(folder);
#endif
	    exit(WIFEXITED(status) ? WEXITSTATUS(status) : 1);
	    /* NOTREACHED */
	}
	if (count + 1 < var_fork_tries)
	    sleep(var_fork_delay);
    }
    msg_fatal("fork %s: %m", command[0]);
    /* NOTREACHED */

}
