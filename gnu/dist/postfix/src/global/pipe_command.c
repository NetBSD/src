/*++
/* NAME
/*	pipe_command 3
/* SUMMARY
/*	deliver message to external command
/* SYNOPSIS
/*	#include <pipe_command.h>
/*
/*	int	pipe_command(src, why, key, value, ...)
/*	VSTREAM	*src;
/*	VSTRING *why;
/*	int	key;
/* DESCRIPTION
/*	pipe_command() runs a command with a message as standard
/*	input.  A limited amount of standard output and standard error
/*	output is captured for diagnostics purposes.
/*
/*	Arguments:
/* .IP src
/*	An open message queue file, positioned at the start of the actual
/*	message content.
/* .IP why
/*	Storage for diagnostic information.
/* .IP key
/*	Specifies what value will follow. pipe_command() takes a list
/*	of (key, value) arguments, terminated by PIPE_CMD_END. The
/*	following is a listing of key codes together with the expected
/*	value type.
/* .RS
/* .IP "PIPE_CMD_COMMAND (char *)"
/*	Specifies the command to execute as a string. The string is
/*	passed to the shell when it contains shell meta characters
/*	or when it appears to be a shell built-in command, otherwise
/*	the command is executed without invoking a shell.
/*	One of PIPE_CMD_COMMAND or PIPE_CMD_ARGV must be specified.
/*	See also the PIPE_CMD_SHELL attribute below.
/* .IP "PIPE_CMD_ARGV (char **)"
/*	The command is specified as an argument vector. This vector is
/*	passed without further inspection to the \fIexecvp\fR() routine.
/*	One of PIPE_CMD_COMMAND or PIPE_CMD_ARGV must be specified.
/* .IP "PIPE_CMD_ENV (char **)"
/*	Additional environment information, in the form of a null-terminated
/*	list of name, value, name, value, ... elements. By default only the
/*	command search path is initialized to _PATH_DEFPATH.
/* .IP "PIPE_CMD_EXPORT (char **)"
/*	Null-terminated array with names of environment parameters
/*	that can be exported. By default, everything is exported.
/* .IP "PIPE_CMD_COPY_FLAGS (int)"
/*	Flags that are passed on to the \fImail_copy\fR() routine.
/*	The default flags value is 0 (zero).
/* .IP "PIPE_CMD_SENDER (char *)"
/*	The envelope sender address, which is passed on to the
/*	\fImail_copy\fR() routine.
/* .IP "PIPE_CMD_DELIVERED (char *)"
/*	The recipient envelope address, which is passed on to the
/*	\fImail_copy\fR() routine.
/* .IP "PIPE_CMD_EOL (char *)"
/*	End-of-line delimiter. The default is to use the newline character.
/* .IP "PIPE_CMD_UID (int)"
/*	The user ID to execute the command as. The default is
/*	the user ID corresponding to the \fIdefault_privs\fR
/*	configuration parameter. The user ID must be non-zero.
/* .IP "PIPE_CMD_GID (int)"
/*	The group ID to execute the command as. The default is
/*	the group ID corresponding to the \fIdefault_privs\fR
/*	configuration parameter. The group ID must be non-zero.
/* .IP "PIPE_CMD_TIME_LIMIT (int)"
/*	The amount of time the command is allowed to run before it
/*	is terminated with SIGKILL. The default is the limit given
/*	with the \fIcommand_time_limit\fR configuration parameter.
/* .IP "PIPE_CMD_SHELL (char *)"
/*	The shell to use when executing the command specified with
/*	PIPE_CMD_COMMAND. This shell is invoked regardless of the
/*	command content.
/* .RE
/* DIAGNOSTICS
/*	Panic: interface violations (for example, a zero-valued
/*	user ID or group ID, or a missing command).
/*
/*	pipe_command() returns one of the following status codes:
/* .IP PIPE_STAT_OK
/*	The command has taken responsibility for further delivery of
/*	the message.
/* .IP PIPE_STAT_DEFER
/*	The command failed with a "try again" type error.
/*	The reason is given via the \fIwhy\fR argument.
/* .IP PIPE_STAT_BOUNCE
/*	The command indicated that the message was not acceptable,
/*	or the command did not finish within the time limit.
/*	The reason is given via the \fIwhy\fR argument.
/* .IP PIPE_STAT_CORRUPT
/*	The queue file is corrupted.
/* SEE ALSO
/*	mail_copy(3) deliver to any.
/*	mark_corrupt(3) mark queue file as corrupt.
/*	sys_exits(3) sendmail-compatible exit status codes.
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
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef USE_PATHS_H
#include <paths.h>
#endif
#include <syslog.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <stringops.h>
#include <iostuff.h>
#include <timed_wait.h>
#include <set_ugid.h>
#include <argv.h>

/* Global library. */

#include <mail_params.h>
#include <mail_copy.h>
#include <clean_env.h>
#include <pipe_command.h>
#include <exec_command.h>
#include <sys_exits.h>

/* Application-specific. */

struct pipe_args {
    int     flags;			/* see mail_copy.h */
    char   *sender;			/* envelope sender */
    char   *delivered;			/* envelope recipient */
    char   *eol;			/* carriagecontrol */
    char  **argv;			/* either an array */
    char   *command;			/* or a plain string */
    uid_t   uid;			/* privileges */
    gid_t   gid;			/* privileges */
    char  **env;			/* extra environment */
    char  **export;			/* exportable environment */
    char   *shell;			/* command shell */
};

static int pipe_command_timeout;	/* command has timed out */
static int pipe_command_maxtime;	/* available time to complete */

/* get_pipe_args - capture the variadic argument list */

static void get_pipe_args(struct pipe_args * args, va_list ap)
{
    char   *myname = "get_pipe_args";
    int     key;

    /*
     * First, set the default values.
     */
    args->flags = 0;
    args->sender = 0;
    args->delivered = 0;
    args->eol = "\n";
    args->argv = 0;
    args->command = 0;
    args->uid = var_default_uid;
    args->gid = var_default_gid;
    args->env = 0;
    args->export = 0;
    args->shell = 0;

    pipe_command_maxtime = var_command_maxtime;

    /*
     * Then, override the defaults with user-supplied inputs.
     */
    while ((key = va_arg(ap, int)) != PIPE_CMD_END) {
	switch (key) {
	case PIPE_CMD_COPY_FLAGS:
	    args->flags |= va_arg(ap, int);
	    break;
	case PIPE_CMD_SENDER:
	    args->sender = va_arg(ap, char *);
	    break;
	case PIPE_CMD_DELIVERED:
	    args->delivered = va_arg(ap, char *);
	    break;
	case PIPE_CMD_EOL:
	    args->eol = va_arg(ap, char *);
	    break;
	case PIPE_CMD_ARGV:
	    if (args->command)
		msg_panic("%s: got PIPE_CMD_ARGV and PIPE_CMD_COMMAND", myname);
	    args->argv = va_arg(ap, char **);
	    break;
	case PIPE_CMD_COMMAND:
	    if (args->argv)
		msg_panic("%s: got PIPE_CMD_ARGV and PIPE_CMD_COMMAND", myname);
	    args->command = va_arg(ap, char *);
	    break;
	case PIPE_CMD_UID:
	    args->uid = va_arg(ap, int);	/* in case uid_t is short */
	    break;
	case PIPE_CMD_GID:
	    args->gid = va_arg(ap, int);	/* in case gid_t is short */
	    break;
	case PIPE_CMD_TIME_LIMIT:
	    pipe_command_maxtime = va_arg(ap, int);
	    break;
	case PIPE_CMD_ENV:
	    args->env = va_arg(ap, char **);
	    break;
	case PIPE_CMD_EXPORT:
	    args->export = va_arg(ap, char **);
	    break;
	case PIPE_CMD_SHELL:
	    args->shell = va_arg(ap, char *);
	    break;
	default:
	    msg_panic("%s: unknown key: %d", myname, key);
	}
    }
    if (args->command == 0 && args->argv == 0)
	msg_panic("%s: missing PIPE_CMD_ARGV or PIPE_CMD_COMMAND", myname);
    if (args->uid == 0)
	msg_panic("%s: privileged uid", myname);
    if (args->gid == 0)
	msg_panic("%s: privileged gid", myname);
}

/* pipe_command_write - write to command with time limit */

static int pipe_command_write(int fd, void *buf, unsigned len)
{
    int     maxtime = (pipe_command_timeout == 0) ? pipe_command_maxtime : 0;
    char   *myname = "pipe_command_write";

    /*
     * Don't wait when all available time was already used up.
     */
    if (write_wait(fd, maxtime) < 0) {
	if (pipe_command_timeout == 0) {
	    if (msg_verbose)
		msg_info("%s: time limit exceeded", myname);
	    pipe_command_timeout = 1;
	}
	return (0);
    } else {
	return (write(fd, buf, len));
    }
}

/* pipe_command_read - read from command with time limit */

static int pipe_command_read(int fd, void *buf, unsigned len)
{
    int     maxtime = (pipe_command_timeout == 0) ? pipe_command_maxtime : 0;
    char   *myname = "pipe_command_read";

    /*
     * Don't wait when all available time was already used up.
     */
    if (read_wait(fd, maxtime) < 0) {
	if (pipe_command_timeout == 0) {
	    if (msg_verbose)
		msg_info("%s: time limit exceeded", myname);
	    pipe_command_timeout = 1;
	}
	return (0);
    } else {
	return (read(fd, buf, len));
    }
}

/* pipe_command_wait_or_kill - wait for command with time limit, or kill it */

static int pipe_command_wait_or_kill(pid_t pid, WAIT_STATUS_T *statusp, int sig)
{
    int     maxtime = (pipe_command_timeout == 0) ? pipe_command_maxtime : 1;
    char   *myname = "pipe_command_wait_or_kill";
    int     n;

    /*
     * Don't wait when all available time was already used up.
     */
    if ((n = timed_waitpid(pid, statusp, 0, maxtime)) < 0 && errno == ETIMEDOUT) {
	if (pipe_command_timeout == 0) {
	    if (msg_verbose)
		msg_info("%s: time limit exceeded", myname);
	    pipe_command_timeout = 1;
	}
	kill(-pid, sig);
	n = waitpid(pid, statusp, 0);
    }
    return (n);
}

/* pipe_command - execute command with extreme prejudice */

int     pipe_command(VSTREAM *src, VSTRING *why,...)
{
    char   *myname = "pipe_comand";
    va_list ap;
    VSTREAM *cmd_in_stream;
    VSTREAM *cmd_out_stream;
    char    log_buf[VSTREAM_BUFSIZE + 1];
    int     log_len;
    pid_t   pid;
    int     write_status;
    WAIT_STATUS_T wait_status;
    int     cmd_in_pipe[2];
    int     cmd_out_pipe[2];
    struct pipe_args args;
    char  **cpp;
    ARGV   *argv;

    /*
     * Process the variadic argument list. This also does sanity checks on
     * what data the caller is passing to us.
     */
    va_start(ap, why);
    get_pipe_args(&args, ap);
    va_end(ap);

    /*
     * For convenience...
     */
    if (args.command == 0)
	args.command = args.argv[0];

    /*
     * Set up pipes that connect us to the command input and output streams.
     * We're using a rather disgusting hack to capture command output: set
     * the output to non-blocking mode, and don't attempt to read the output
     * until AFTER the process has terminated. The rationale for this is: 1)
     * the command output will be used only when delivery fails; 2) the
     * amount of output is expected to be small; 3) the output can be
     * truncated without too much loss. I could even argue that truncating
     * the amount of diagnostic output is a good thing to do, but I won't go
     * that far.
     */
    if (pipe(cmd_in_pipe) < 0 || pipe(cmd_out_pipe) < 0)
	msg_fatal("%s: pipe: %m", myname);
    non_blocking(cmd_out_pipe[1], NON_BLOCKING);

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
	vstring_sprintf(why, "Delivery failed: %m");
	return (PIPE_STAT_DEFER);

	/*
	 * Child. Run the child in a separate process group so that the
	 * parent can kill not just the child but also its offspring.
	 */
    case 0:
	set_ugid(args.uid, args.gid);
	setsid();

	/*
	 * Pipe plumbing.
	 */
	close(cmd_in_pipe[1]);
	close(cmd_out_pipe[0]);
	if (DUP2(cmd_in_pipe[0], STDIN_FILENO) < 0
	    || DUP2(cmd_out_pipe[1], STDOUT_FILENO) < 0
	    || DUP2(cmd_out_pipe[1], STDERR_FILENO) < 0)
	    msg_fatal("%s: dup2: %m", myname);
	close(cmd_in_pipe[0]);
	close(cmd_out_pipe[1]);

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
	close(cmd_in_pipe[0]);
	close(cmd_out_pipe[1]);

	cmd_in_stream = vstream_fdopen(cmd_in_pipe[1], O_WRONLY);
	cmd_out_stream = vstream_fdopen(cmd_out_pipe[0], O_RDONLY);

	/*
	 * Give the command a limited amount of time to run, by enforcing
	 * timeouts on all I/O from and to it.
	 */
	vstream_control(cmd_in_stream,
			VSTREAM_CTL_WRITE_FN, pipe_command_write,
			VSTREAM_CTL_END);
	vstream_control(cmd_out_stream,
			VSTREAM_CTL_READ_FN, pipe_command_read,
			VSTREAM_CTL_END);
	pipe_command_timeout = 0;

	/*
	 * Pipe the message into the command. XXX We shouldn't be ignoring
	 * screams for help from mail_copy() like this. But, the command may
	 * stop reading input early, and that should not be considered an
	 * error condition.
	 */
#define DONT_CARE_WHY	((VSTRING *) 0)

	write_status = mail_copy(args.sender, args.delivered, src,
				 cmd_in_stream, args.flags,
				 args.eol, DONT_CARE_WHY);

	/*
	 * Capture a limited amount of command output, for inclusion in a
	 * bounce message. Turn tabs and newlines into whitespace, and
	 * replace other non-printable characters by underscore.
	 */
	log_len = vstream_fread(cmd_out_stream, log_buf, sizeof(log_buf) - 1);
	(void) vstream_fclose(cmd_out_stream);
	log_buf[log_len] = 0;
	translit(log_buf, "\t\n", "  ");
	printable(log_buf, '_');

	/*
	 * Just because the child closes its output streams, don't assume
	 * that it will terminate. Instead, be prepared for the situation
	 * that the child does not terminate, even when the parent
	 * experiences no read/write timeout. Make sure that the child
	 * terminates before the parent attempts to retrieve its exit status,
	 * otherwise the parent could become stuck, and the mail system would
	 * eventually run out of delivery agents. Do a thorough job, and kill
	 * not just the child process but also its offspring.
	 */
	if (pipe_command_timeout)
	    (void) kill(-pid, SIGKILL);
	if (pipe_command_wait_or_kill(pid, &wait_status, SIGKILL) < 0)
	    msg_fatal("wait: %m");
	if (pipe_command_timeout) {
	    vstring_sprintf(why, "Command time limit exceeded: \"%s\"%s%s",
			    args.command,
			    log_len ? ". Command output: " : "", log_buf);
	    return (PIPE_STAT_BOUNCE);
	}

	/*
	 * Command exits. Give special treatment to sendmail style exit
	 * status codes.
	 */
	if (!NORMAL_EXIT_STATUS(wait_status)) {
	    if (WIFSIGNALED(wait_status)) {
		vstring_sprintf(why, "Command died with signal %d: \"%s\"%s%s",
				WTERMSIG(wait_status),
				args.command,
			      log_len ? ". Command output: " : "", log_buf);
		return (PIPE_STAT_DEFER);
	    } else if (SYS_EXITS_CODE(WEXITSTATUS(wait_status))) {
		vstring_sprintf(why, "%s%s%s",
				sys_exits_strerror(WEXITSTATUS(wait_status)),
			      log_len ? ". Command output: " : "", log_buf);
		return (sys_exits_softerror(WEXITSTATUS(wait_status)) ?
			PIPE_STAT_DEFER : PIPE_STAT_BOUNCE);
	    } else {
		vstring_sprintf(why, "Command died with status %d: \"%s\"%s%s",
				WEXITSTATUS(wait_status),
				args.command,
			      log_len ? ". Command output: " : "", log_buf);
		return (PIPE_STAT_BOUNCE);
	    }
	} else if (write_status & MAIL_COPY_STAT_CORRUPT) {
	    return (PIPE_STAT_CORRUPT);
	} else if (write_status && errno != EPIPE) {
	    vstring_sprintf(why, "Command failed: %m: \"%s\"", args.command);
	    return (PIPE_STAT_DEFER);
	} else {
	    return (PIPE_STAT_OK);
	}
    }
}
