/*	$NetBSD: spawn_command.c,v 1.1.1.1 2009/06/23 10:09:01 tron Exp $	*/

/*++
/* NAME
/*	spawn_command 3
/* SUMMARY
/*	run external command
/* SYNOPSIS
/*	#include <spawn_command.h>
/*
/*	WAIT_STATUS_T spawn_command(key, value, ...)
/*	int	key;
/* DESCRIPTION
/*	spawn_command() runs a command in a child process and returns
/*	the command exit status.
/*
/*	Arguments:
/* .IP key
/*	Specifies what value will follow. spawn_command() takes a list
/*	of (key, value) arguments, terminated by SPAWN_CMD_END. The
/*	following is a listing of key codes together with the expected
/*	value type.
/* .RS
/* .IP "SPAWN_CMD_COMMAND (char *)"
/*	Specifies the command to execute as a string. The string is
/*	passed to the shell when it contains shell meta characters
/*	or when it appears to be a shell built-in command, otherwise
/*	the command is executed without invoking a shell.
/*	One of SPAWN_CMD_COMMAND or SPAWN_CMD_ARGV must be specified.
/*	See also the SPAWN_CMD_SHELL attribute below.
/* .IP "SPAWN_CMD_ARGV (char **)"
/*	The command is specified as an argument vector. This vector is
/*	passed without further inspection to the \fIexecvp\fR() routine.
/*	One of SPAWN_CMD_COMMAND or SPAWN_CMD_ARGV must be specified.
/* .IP "SPAWN_CMD_ENV (char **)"
/*	Additional environment information, in the form of a null-terminated
/*	list of name, value, name, value, ... elements. By default only the
/*	command search path is initialized to _PATH_DEFPATH.
/* .IP "SPAWN_CMD_EXPORT (char **)"
/*	Null-terminated array of names of environment parameters that can
/*	be exported. By default, everything is exported.
/* .IP "SPAWN_CMD_STDIN (int)"
/* .IP "SPAWN_CMD_STDOUT (int)"
/* .IP "SPAWN_CMD_STDERR (int)"
/*	Each of these specifies I/O redirection of one of the standard file
/*	descriptors for the command.
/* .IP "SPAWN_CMD_UID (uid_t)"
/*	The user ID to execute the command as. The value -1 is reserved
/*	and cannot be specified.
/* .IP "SPAWN_CMD_GID (gid_t)"
/*	The group ID to execute the command as. The value -1 is reserved
/*	and cannot be specified.
/* .IP "SPAWN_CMD_TIME_LIMIT (int)"
/*	The amount of time in seconds the command is allowed to run before
/*	it is terminated with SIGKILL. The default is no time limit.
/* .IP "SPAWN_CMD_SHELL (char *)"
/*	The shell to use when executing the command specified with
/*	SPAWN_CMD_COMMAND. This shell is invoked regardless of the
/*	command content.
/* .RE
/* DIAGNOSTICS
/*	Panic: interface violations (for example, a missing command).
/*
/*	Fatal error: fork() failure, other system call failures.
/*
/*	spawn_command() returns the exit status as defined by wait(2).
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* SEE ALSO
/*	exec_command(3) execute command
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#ifdef USE_PATHS_H
#include <paths.h>
#endif
#include <syslog.h>

/* Utility library. */

#include <msg.h>
#include <timed_wait.h>
#include <set_ugid.h>
#include <argv.h>
#include <spawn_command.h>
#include <exec_command.h>
#include <clean_env.h>

/* Application-specific. */

struct spawn_args {
    char  **argv;			/* argument vector */
    char   *command;			/* or a plain string */
    int     stdin_fd;			/* read stdin here */
    int     stdout_fd;			/* write stdout here */
    int     stderr_fd;			/* write stderr here */
    uid_t   uid;			/* privileges */
    gid_t   gid;			/* privileges */
    char  **env;			/* extra environment */
    char  **export;			/* exportable environment */
    char   *shell;			/* command shell */
    int     time_limit;			/* command time limit */
};

/* get_spawn_args - capture the variadic argument list */

static void get_spawn_args(struct spawn_args * args, int init_key, va_list ap)
{
    const char *myname = "get_spawn_args";
    int     key;

    /*
     * First, set the default values.
     */
    args->argv = 0;
    args->command = 0;
    args->stdin_fd = -1;
    args->stdout_fd = -1;
    args->stderr_fd = -1;
    args->uid = (uid_t) - 1;
    args->gid = (gid_t) - 1;
    args->env = 0;
    args->export = 0;
    args->shell = 0;
    args->time_limit = 0;

    /*
     * Then, override the defaults with user-supplied inputs.
     */
    for (key = init_key; key != SPAWN_CMD_END; key = va_arg(ap, int)) {
	switch (key) {
	case SPAWN_CMD_ARGV:
	    if (args->command)
		msg_panic("%s: specify SPAWN_CMD_ARGV or SPAWN_CMD_COMMAND",
			  myname);
	    args->argv = va_arg(ap, char **);
	    break;
	case SPAWN_CMD_COMMAND:
	    if (args->argv)
		msg_panic("%s: specify SPAWN_CMD_ARGV or SPAWN_CMD_COMMAND",
			  myname);
	    args->command = va_arg(ap, char *);
	    break;
	case SPAWN_CMD_STDIN:
	    args->stdin_fd = va_arg(ap, int);
	    break;
	case SPAWN_CMD_STDOUT:
	    args->stdout_fd = va_arg(ap, int);
	    break;
	case SPAWN_CMD_STDERR:
	    args->stderr_fd = va_arg(ap, int);
	    break;
	case SPAWN_CMD_UID:
	    args->uid = va_arg(ap, uid_t);
	    if (args->uid == (uid_t) (-1))
		msg_panic("spawn_command: request with reserved user ID: -1");
	    break;
	case SPAWN_CMD_GID:
	    args->gid = va_arg(ap, gid_t);
	    if (args->gid == (gid_t) (-1))
		msg_panic("spawn_command: request with reserved group ID: -1");
	    break;
	case SPAWN_CMD_TIME_LIMIT:
	    args->time_limit = va_arg(ap, int);
	    break;
	case SPAWN_CMD_ENV:
	    args->env = va_arg(ap, char **);
	    break;
	case SPAWN_CMD_EXPORT:
	    args->export = va_arg(ap, char **);
	    break;
	case SPAWN_CMD_SHELL:
	    args->shell = va_arg(ap, char *);
	    break;
	default:
	    msg_panic("%s: unknown key: %d", myname, key);
	}
    }
    if (args->command == 0 && args->argv == 0)
	msg_panic("%s: missing SPAWN_CMD_ARGV or SPAWN_CMD_COMMAND", myname);
    if (args->command == 0 && args->shell != 0)
	msg_panic("%s: SPAWN_CMD_ARGV cannot be used with SPAWN_CMD_SHELL",
		  myname);
}

/* spawn_command - execute command with extreme prejudice */

WAIT_STATUS_T spawn_command(int key,...)
{
    const char *myname = "spawn_comand";
    va_list ap;
    pid_t   pid;
    WAIT_STATUS_T wait_status;
    struct spawn_args args;
    char  **cpp;
    ARGV   *argv;
    int     err;

    /*
     * Process the variadic argument list. This also does sanity checks on
     * what data the caller is passing to us.
     */
    va_start(ap, key);
    get_spawn_args(&args, key, ap);
    va_end(ap);

    /*
     * For convenience...
     */
    if (args.command == 0)
	args.command = args.argv[0];

    /*
     * Spawn off a child process and irrevocably change privilege to the
     * user. This includes revoking all rights on open files (via the close
     * on exec flag). If we cannot run the command now, try again some time
     * later.
     */
    switch (pid = fork()) {

	/*
	 * Error. Instead of trying again right now, back off, give the
	 * system a chance to recover, and try again later.
	 */
    case -1:
	msg_fatal("fork: %m");

	/*
	 * Child. Run the child in a separate process group so that the
	 * parent can kill not just the child but also its offspring.
	 */
    case 0:
	if (args.uid != (uid_t) - 1 || args.gid != (gid_t) - 1)
	    set_ugid(args.uid, args.gid);
	setsid();

	/*
	 * Pipe plumbing.
	 */
	if ((args.stdin_fd >= 0 && DUP2(args.stdin_fd, STDIN_FILENO) < 0)
	 || (args.stdout_fd >= 0 && DUP2(args.stdout_fd, STDOUT_FILENO) < 0)
	|| (args.stderr_fd >= 0 && DUP2(args.stderr_fd, STDERR_FILENO) < 0))
	    msg_fatal("%s: dup2: %m", myname);

	/*
	 * Environment plumbing. Always reset the command search path. XXX
	 * That should probably be done by clean_env().
	 */
	if (args.export)
	    clean_env(args.export);
	if (setenv("PATH", _PATH_DEFPATH, 1))
	    msg_fatal("%s: setenv: %m", myname);
	if (args.env)
	    for (cpp = args.env; *cpp; cpp += 2)
		if (setenv(cpp[0], cpp[1], 1))
		    msg_fatal("setenv: %m");

	/*
	 * Process plumbing. If possible, avoid running a shell.
	 */
	closelog();
	if (args.argv) {
	    execvp(args.argv[0], args.argv);
	    msg_fatal("%s: execvp %s: %m", myname, args.argv[0]);
	} else if (args.shell && *args.shell) {
	    argv = argv_split(args.shell, " \t\r\n");
	    argv_add(argv, args.command, (char *) 0);
	    argv_terminate(argv);
	    execvp(argv->argv[0], argv->argv);
	    msg_fatal("%s: execvp %s: %m", myname, argv->argv[0]);
	} else {
	    exec_command(args.command);
	}
	/* NOTREACHED */

	/*
	 * Parent.
	 */
    default:

	/*
	 * Be prepared for the situation that the child does not terminate.
	 * Make sure that the child terminates before the parent attempts to
	 * retrieve its exit status, otherwise the parent could become stuck,
	 * and the mail system would eventually run out of exec daemons. Do a
	 * thorough job, and kill not just the child process but also its
	 * offspring.
	 */
	if ((err = timed_waitpid(pid, &wait_status, 0, args.time_limit)) < 0
	    && errno == ETIMEDOUT) {
	    msg_warn("%s: process id %lu: command time limit exceeded",
		     args.command, (unsigned long) pid);
	    kill(-pid, SIGKILL);
	    err = waitpid(pid, &wait_status, 0);
	}
	if (err < 0)
	    msg_fatal("wait: %m");
	return (wait_status);
    }
}
