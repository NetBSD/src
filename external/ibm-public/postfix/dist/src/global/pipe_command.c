/*	$NetBSD: pipe_command.c,v 1.1.1.1.2.2 2009/09/15 06:02:51 snj Exp $	*/

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
/*	DSN_BUF	*why;
/*	int	key;
/* DESCRIPTION
/*	pipe_command() runs a command with a message as standard
/*	input.  A limited amount of standard output and standard error
/*	output is captured for diagnostics purposes.
/*
/*	If the command invokes exit() with a non-zero status,
/*	the delivery status is taken from an RFC 3463-style code
/*	at the beginning of command output. If that information is
/*	unavailable, the delivery status is taken from the command
/*	exit status as per <sysexits.h>.
/*
/*	Arguments:
/* .IP src
/*	An open message queue file, positioned at the start of the actual
/*	message content.
/* .IP why
/*	Delivery status information.
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
/* .IP "PIPE_CMD_CHROOT (char *)"
/*	Root and working directory for command execution. This takes
/*	effect before PIPE_CMD_CWD. A null pointer means don't
/*	change root and working directory anyway. Failure to change
/*	directory causes mail delivery to be deferred.
/* .IP "PIPE_CMD_CWD (char *)"
/*	Working directory for command execution, after changing process
/*	privileges to PIPE_CMD_UID and PIPE_CMD_GID. A null pointer means
/*	don't change directory anyway. Failure to change directory
/*	causes mail delivery to be deferred.
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
/* .IP "PIPE_CMD_ORIG_RCPT (char *)"
/*	The original recipient envelope address, which is passed on
/*	to the \fImail_copy\fR() routine.
/* .IP "PIPE_CMD_DELIVERED (char *)"
/*	The recipient envelope address, which is passed on to the
/*	\fImail_copy\fR() routine.
/* .IP "PIPE_CMD_EOL (char *)"
/*	End-of-line delimiter. The default is to use the newline character.
/* .IP "PIPE_CMD_UID (uid_t)"
/*	The user ID to execute the command as. The default is
/*	the user ID corresponding to the \fIdefault_privs\fR
/*	configuration parameter. The user ID must be non-zero.
/* .IP "PIPE_CMD_GID (gid_t)"
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
#include <msg_vstream.h>
#include <vstring.h>
#include <stringops.h>
#include <iostuff.h>
#include <timed_wait.h>
#include <set_ugid.h>
#include <set_eugid.h>
#include <argv.h>
#include <chroot_uid.h>

/* Global library. */

#include <mail_params.h>
#include <mail_copy.h>
#include <clean_env.h>
#include <pipe_command.h>
#include <exec_command.h>
#include <sys_exits.h>
#include <dsn_util.h>
#include <dsn_buf.h>

/* Application-specific. */

struct pipe_args {
    int     flags;			/* see mail_copy.h */
    char   *sender;			/* envelope sender */
    char   *orig_rcpt;			/* original recipient */
    char   *delivered;			/* envelope recipient */
    char   *eol;			/* carriagecontrol */
    char  **argv;			/* either an array */
    char   *command;			/* or a plain string */
    uid_t   uid;			/* privileges */
    gid_t   gid;			/* privileges */
    char  **env;			/* extra environment */
    char  **export;			/* exportable environment */
    char   *shell;			/* command shell */
    char   *cwd;			/* preferred working directory */
    char   *chroot;			/* root directory */
};

static int pipe_command_timeout;	/* command has timed out */
static int pipe_command_maxtime;	/* available time to complete */

/* get_pipe_args - capture the variadic argument list */

static void get_pipe_args(struct pipe_args * args, va_list ap)
{
    const char *myname = "get_pipe_args";
    int     key;

    /*
     * First, set the default values.
     */
    args->flags = 0;
    args->sender = 0;
    args->orig_rcpt = 0;
    args->delivered = 0;
    args->eol = "\n";
    args->argv = 0;
    args->command = 0;
    args->uid = var_default_uid;
    args->gid = var_default_gid;
    args->env = 0;
    args->export = 0;
    args->shell = 0;
    args->cwd = 0;
    args->chroot = 0;

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
	case PIPE_CMD_ORIG_RCPT:
	    args->orig_rcpt = va_arg(ap, char *);
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
	    args->uid = va_arg(ap, uid_t);	/* in case uid_t is short */
	    break;
	case PIPE_CMD_GID:
	    args->gid = va_arg(ap, gid_t);	/* in case gid_t is short */
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
	case PIPE_CMD_CWD:
	    args->cwd = va_arg(ap, char *);
	    break;
	case PIPE_CMD_CHROOT:
	    args->chroot = va_arg(ap, char *);
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

static ssize_t pipe_command_write(int fd, void *buf, size_t len,
				          int unused_timeout,
				          void *unused_context)
{
    int     maxtime = (pipe_command_timeout == 0) ? pipe_command_maxtime : 0;
    const char *myname = "pipe_command_write";

    /*
     * Don't wait when all available time was already used up.
     */
    if (write_wait(fd, maxtime) < 0) {
	if (pipe_command_timeout == 0) {
	    msg_warn("%s: write time limit exceeded", myname);
	    pipe_command_timeout = 1;
	}
	return (0);
    } else {
	return (write(fd, buf, len));
    }
}

/* pipe_command_read - read from command with time limit */

static ssize_t pipe_command_read(int fd, void *buf, ssize_t len,
				         int unused_timeout,
				         void *unused_context)
{
    int     maxtime = (pipe_command_timeout == 0) ? pipe_command_maxtime : 0;
    const char *myname = "pipe_command_read";

    /*
     * Don't wait when all available time was already used up.
     */
    if (read_wait(fd, maxtime) < 0) {
	if (pipe_command_timeout == 0) {
	    msg_warn("%s: read time limit exceeded", myname);
	    pipe_command_timeout = 1;
	}
	return (0);
    } else {
	return (read(fd, buf, len));
    }
}

/* kill_command - terminate command forcibly */

static void kill_command(pid_t pid, int sig, uid_t kill_uid, gid_t kill_gid)
{
    uid_t   saved_euid = geteuid();
    gid_t   saved_egid = getegid();

    /*
     * Switch privileges to that of the child process. Terminate the child
     * and its offspring.
     */
    set_eugid(kill_uid, kill_gid);
    if (kill(-pid, sig) < 0 && kill(pid, sig) < 0)
	msg_warn("cannot kill process (group) %lu: %m",
		 (unsigned long) pid);
    set_eugid(saved_euid, saved_egid);
}

/* pipe_command_wait_or_kill - wait for command with time limit, or kill it */

static int pipe_command_wait_or_kill(pid_t pid, WAIT_STATUS_T *statusp, int sig,
				             uid_t kill_uid, gid_t kill_gid)
{
    int     maxtime = (pipe_command_timeout == 0) ? pipe_command_maxtime : 1;
    const char *myname = "pipe_command_wait_or_kill";
    int     n;

    /*
     * Don't wait when all available time was already used up.
     */
    if ((n = timed_waitpid(pid, statusp, 0, maxtime)) < 0 && errno == ETIMEDOUT) {
	if (pipe_command_timeout == 0) {
	    msg_warn("%s: child wait time limit exceeded", myname);
	    pipe_command_timeout = 1;
	}
	kill_command(pid, sig, kill_uid, kill_gid);
	n = waitpid(pid, statusp, 0);
    }
    return (n);
}

/* pipe_child_cleanup - child fatal error handler */

static void pipe_child_cleanup(void)
{

    /*
     * WARNING: don't place code here. This code may run as mail_owner, as
     * root, or as the user/group specified with the "user" attribute. The
     * only safe action is to terminate.
     * 
     * Future proofing. If you need exit() here then you broke Postfix.
     */
    _exit(EX_TEMPFAIL);
}

/* pipe_command - execute command with extreme prejudice */

int     pipe_command(VSTREAM *src, DSN_BUF *why,...)
{
    const char *myname = "pipe_command";
    va_list ap;
    VSTREAM *cmd_in_stream;
    VSTREAM *cmd_out_stream;
    char    log_buf[VSTREAM_BUFSIZE + 1];
    int     log_len;
    pid_t   pid;
    int     write_status;
    int     write_errno;
    WAIT_STATUS_T wait_status;
    int     cmd_in_pipe[2];
    int     cmd_out_pipe[2];
    struct pipe_args args;
    char  **cpp;
    ARGV   *argv;
    DSN_SPLIT dp;
    const SYS_EXITS_DETAIL *sp;

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
     * 
     * Turn on non-blocking writes to the child process so that we can enforce
     * timeouts after partial writes.
     * 
     * XXX Too much trouble with different systems returning weird write()
     * results when a pipe is writable.
     */
    if (pipe(cmd_in_pipe) < 0 || pipe(cmd_out_pipe) < 0)
	msg_fatal("%s: pipe: %m", myname);
    non_blocking(cmd_out_pipe[1], NON_BLOCKING);
#if 0
    non_blocking(cmd_in_pipe[1], NON_BLOCKING);
#endif

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
	msg_warn("fork: %m");
	dsb_unix(why, "4.3.0", sys_exits_detail(EX_OSERR)->text,
		 "Delivery failed: %m");
	return (PIPE_STAT_DEFER);

	/*
	 * Child. Run the child in a separate process group so that the
	 * parent can kill not just the child but also its offspring.
	 * 
	 * Redirect fatal exits to our own fatal exit handler (never leave the
	 * parent's handler enabled :-) so we can replace random exit status
	 * codes by EX_TEMPFAIL.
	 */
    case 0:
	(void) msg_cleanup(pipe_child_cleanup);

	/*
	 * In order to chroot it is necessary to switch euid back to root.
	 * Right after chroot we call set_ugid() so all privileges will be
	 * dropped again.
	 * 
	 * XXX For consistency we use chroot_uid() to change root+current
	 * directory. However, we must not use chroot_uid() to change process
	 * privileges (assuming a version that accepts numeric privileges).
	 * That would create a maintenance problem, because we would have two
	 * different code paths to set the external command's privileges.
	 */
	if (args.chroot) {
	    seteuid(0);
	    chroot_uid(args.chroot, (char *) 0);
	}

	/*
	 * XXX If we put code before the set_ugid() call, then the code that
	 * changes root directory must switch back to the mail_owner UID,
	 * otherwise we'd be running with root privileges.
	 */
	set_ugid(args.uid, args.gid);
	if (setsid() < 0)
	    msg_warn("setsid failed: %m");

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
	 * Working directory plumbing.
	 */
	if (args.cwd && chdir(args.cwd) < 0)
	    msg_fatal("cannot change directory to \"%s\" for uid=%lu gid=%lu: %m",
		      args.cwd, (unsigned long) args.uid,
		      (unsigned long) args.gid);

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
	 * 
	 * As a safety for buggy libraries, we close the syslog socket.
	 * Otherwise we could leak a file descriptor that was created by a
	 * privileged process.
	 * 
	 * XXX To avoid losing fatal error messages we open a VSTREAM and
	 * capture the output in the parent process.
	 */
	closelog();
	msg_vstream_init(var_procname, VSTREAM_ERR);
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
	 * Pipe the message into the command. Examine the error report only
	 * if we can't recognize a more specific error from the command exit
	 * status or from the command output.
	 */
	write_status = mail_copy(args.sender, args.orig_rcpt,
				 args.delivered, src,
				 cmd_in_stream, args.flags,
				 args.eol, why);
	write_errno = errno;

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
	    kill_command(pid, SIGKILL, args.uid, args.gid);
	if (pipe_command_wait_or_kill(pid, &wait_status, SIGKILL,
				      args.uid, args.gid) < 0)
	    msg_fatal("wait: %m");
	if (pipe_command_timeout) {
	    dsb_unix(why, "5.3.0", log_len ?
		     log_buf : sys_exits_detail(EX_SOFTWARE)->text,
		     "Command time limit exceeded: \"%s\"%s%s",
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
		dsb_unix(why, "5.3.0", log_len ?
			 log_buf : sys_exits_detail(EX_SOFTWARE)->text,
			 "Command died with signal %d: \"%s\"%s%s",
			 WTERMSIG(wait_status), args.command,
			 log_len ? ". Command output: " : "", log_buf);
		return (PIPE_STAT_DEFER);
	    }
	    /* Use "D.S.N text" command output. XXX What diagnostic code? */
	    else if (dsn_valid(log_buf) > 0) {
		dsn_split(&dp, "5.3.0", log_buf);
		dsb_unix(why, DSN_STATUS(dp.dsn), dp.text, "%s", dp.text);
		return (DSN_CLASS(dp.dsn) == '4' ?
			PIPE_STAT_DEFER : PIPE_STAT_BOUNCE);
	    }
	    /* Use <sysexits.h> compatible exit status. */
	    else if (SYS_EXITS_CODE(WEXITSTATUS(wait_status))) {
		sp = sys_exits_detail(WEXITSTATUS(wait_status));
		dsb_unix(why, sp->dsn,
			 log_len ? log_buf : sp->text, "%s%s%s", sp->text,
			 log_len ? ". Command output: " : "", log_buf);
		return (sp->dsn[0] == '4' ?
			PIPE_STAT_DEFER : PIPE_STAT_BOUNCE);
	    }

	    /*
	     * No "D.S.N text" or <sysexits.h> compatible status. Fake it.
	     */
	    else {
		sp = sys_exits_detail(WEXITSTATUS(wait_status));
		dsb_unix(why, sp->dsn,
			 log_len ? log_buf : sp->text,
			 "Command died with status %d: \"%s\"%s%s",
			 WEXITSTATUS(wait_status), args.command,
			 log_len ? ". Command output: " : "", log_buf);
		return (PIPE_STAT_BOUNCE);
	    }
	} else if (write_status &
		   MAIL_COPY_STAT_CORRUPT) {
	    return (PIPE_STAT_CORRUPT);
	} else if (write_status && write_errno != EPIPE) {
	    vstring_prepend(why->reason, "Command failed: ",
			    sizeof("Command failed: ") - 1);
	    vstring_sprintf_append(why->reason, ": \"%s\"", args.command);
	    return (PIPE_STAT_BOUNCE);
	} else {
	    return (PIPE_STAT_OK);
	}
    }
}
