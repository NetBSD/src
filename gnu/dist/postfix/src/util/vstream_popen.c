/*++
/* NAME
/*	vstream_popen 3
/* SUMMARY
/*	open stream to child process
/* SYNOPSIS
/*	#include <vstream.h>
/*
/*	VSTREAM	*vstream_popen(flags, key, value, ...)
/*	int	flags;
/*	int	key;
/*
/*	int	vstream_pclose(stream)
/*	VSTREAM	*stream;
/* DESCRIPTION
/*	vstream_popen() opens a one-way or two-way stream to a user-specified
/*	command, which is executed by a child process. The \fIflags\fR
/*	argument is as with vstream_fopen(). The child's standard input and
/*	standard output are redirected to the stream, which is based on a
/*	socketpair or other suitable local IPC. vstream_popen() takes a list
/*	of (key, value) arguments, terminated by VSTREAM_POPEN_END. The key
/*	argument specifies what value will follow. The following is a listing
/*	of key codes together with the expected value type.
/* .RS
/* .IP "VSTREAM_POPEN_COMMAND (char *)"
/*	Specifies the command to execute as a string. The string is
/*	passed to the shell when it contains shell meta characters
/*	or when it appears to be a shell built-in command, otherwise
/*	the command is executed without invoking a shell.
/*	One of VSTREAM_POPEN_COMMAND or VSTREAM_POPEN_ARGV must be specified.
/* .IP "VSTREAM_POPEN_ARGV (char **)"
/*	The command is specified as an argument vector. This vector is
/*	passed without further inspection to the \fIexecvp\fR() routine.
/*	One of VSTREAM_POPEN_COMMAND or VSTREAM_POPEN_ARGV must be specified.
/*	See also the VSTREAM_POPEN_SHELL attribute below.
/* .IP "VSTREAM_POPEN_ENV (char **)"
/*	Additional environment information, in the form of a null-terminated
/*	list of name, value, name, value, ... elements. By default only the
/*	command search path is initialized to _PATH_DEFPATH.
/* .IP "VSTREAM_POPEN_EXPORT (char **)"
/*	Null-terminated array of names of environment parameters
/*	that can be exported. By default, everything is exported.
/* .IP "VSTREAM_POPEN_UID (int)"
/*	The user ID to execute the command as. The user ID must be non-zero.
/* .IP "VSTREAM_POPEN_GID (int)"
/*	The group ID to execute the command as. The group ID must be non-zero.
/* .IP "VSTREAM_POPEN_SHELL (char *)"
/*	The shell to use when executing the command specified with
/*	VSTREAM_POPEN_COMMAND. This shell is invoked regardless of the
/*	command content.
/* .IP "VSTREAM_POPEN_WAITPID_FN ((*)(pid_t, WAIT_STATUS_T *, int))"
/*	waitpid()-like function to reap the child exit status when
/*	vstream_pclose() is called.
/* .RE
/* .PP
/*	vstream_pclose() closes the named stream and returns the child
/*	exit status. It is an error to specify a stream that was not
/*	returned by vstream_popen() or that is no longer open.
/* DIAGNOSTICS
/*	Panics: interface violations. Fatal errors: out of memory.
/*
/*	vstream_popen() returns a null pointer in case of trouble.
/*	The nature of the problem is specified via the \fIerrno\fR
/*	global variable.
/*
/*	vstream_pclose() returns -1 in case of trouble.
/*	The nature of the problem is specified via the \fIerrno\fR
/*	global variable.
/* SEE ALSO
/*	vstream(3) light-weight buffered I/O
/* BUGS
/*	The interface, stolen from popen()/pclose(), ignores errors
/*	returned when the stream is closed, and does not distinguish
/*	between exit status codes and kill signals.
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
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#ifdef USE_PATHS_H
#include <paths.h>
#endif
#include <syslog.h>

/* Utility library. */

#include <msg.h>
#include <exec_command.h>
#include <vstream.h>
#include <argv.h>
#include <set_ugid.h>
#include <clean_env.h>
#include <iostuff.h>

/* Application-specific. */

typedef struct VSTREAM_POPEN_ARGS {
    char  **argv;
    char   *command;
    uid_t   uid;
    gid_t   gid;
    int     privileged;
    char  **env;
    char  **export;
    char   *shell;
    VSTREAM_WAITPID_FN waitpid_fn;
} VSTREAM_POPEN_ARGS;

/* vstream_parse_args - get arguments from variadic list */

static void vstream_parse_args(VSTREAM_POPEN_ARGS *args, va_list ap)
{
    char   *myname = "vstream_parse_args";
    int     key;

    /*
     * First, set the default values (on all non-zero entries)
     */
    args->argv = 0;
    args->command = 0;
    args->uid = 0;
    args->gid = 0;
    args->privileged = 0;
    args->env = 0;
    args->export = 0;
    args->shell = 0;
    args->waitpid_fn = 0;

    /*
     * Then, override the defaults with user-supplied inputs.
     */
    while ((key = va_arg(ap, int)) != VSTREAM_POPEN_END) {
	switch (key) {
	case VSTREAM_POPEN_ARGV:
	    if (args->command != 0)
		msg_panic("%s: got VSTREAM_POPEN_ARGV and VSTREAM_POPEN_COMMAND", myname);
	    args->argv = va_arg(ap, char **);
	    break;
	case VSTREAM_POPEN_COMMAND:
	    if (args->argv != 0)
		msg_panic("%s: got VSTREAM_POPEN_ARGV and VSTREAM_POPEN_COMMAND", myname);
	    args->command = va_arg(ap, char *);
	    break;
	case VSTREAM_POPEN_UID:
	    args->privileged = 1;
	    args->uid = va_arg(ap, int);
	    break;
	case VSTREAM_POPEN_GID:
	    args->privileged = 1;
	    args->gid = va_arg(ap, int);
	    break;
	case VSTREAM_POPEN_ENV:
	    args->env = va_arg(ap, char **);
	    break;
	case VSTREAM_POPEN_EXPORT:
	    args->export = va_arg(ap, char **);
	    break;
	case VSTREAM_POPEN_SHELL:
	    args->shell = va_arg(ap, char *);
	    break;
	case VSTREAM_POPEN_WAITPID_FN:
	    args->waitpid_fn = va_arg(ap, VSTREAM_WAITPID_FN);
	    break;
	default:
	    msg_panic("%s: unknown key: %d", myname, key);
	}
    }

    if (args->command == 0 && args->argv == 0)
	msg_panic("%s: missing VSTREAM_POPEN_ARGV or VSTREAM_POPEN_COMMAND", myname);
    if (args->privileged != 0 && args->uid == 0)
	msg_panic("%s: privileged uid", myname);
    if (args->privileged != 0 && args->gid == 0)
	msg_panic("%s: privileged gid", myname);
}

/* vstream_popen - open stream to child process */

VSTREAM *vstream_popen(int flags,...)
{
    char   *myname = "vstream_popen";
    VSTREAM_POPEN_ARGS args;
    va_list ap;
    VSTREAM *stream;
    int     sockfd[2];
    int     pid;
    int     fd;
    ARGV   *argv;
    char  **cpp;

    va_start(ap, flags);
    vstream_parse_args(&args, ap);
    va_end(ap);

    if (args.command == 0)
	args.command = args.argv[0];

    if (duplex_pipe(sockfd) < 0)
	return (0);

    switch (pid = fork()) {
    case -1:					/* error */
	return (0);
    case 0:					/* child */
	if (close(sockfd[1]))
	    msg_warn("close: %m");
	for (fd = 0; fd < 2; fd++)
	    if (sockfd[0] != fd)
		if (DUP2(sockfd[0], fd) < 0)
		    msg_fatal("dup2: %m");
	if (sockfd[0] >= 2 && close(sockfd[0]))
	    msg_warn("close: %m");

	/*
	 * Don't try to become someone else unless the user specified it.
	 */
	if (args.privileged)
	    set_ugid(args.uid, args.gid);

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
    default:					/* parent */
	if (close(sockfd[0]))
	    msg_warn("close: %m");
	stream = vstream_fdopen(sockfd[1], flags);
	stream->waitpid_fn = args.waitpid_fn;
	stream->pid = pid;
	return (stream);
    }
}

/* vstream_pclose - close stream to child process */

int     vstream_pclose(VSTREAM *stream)
{
    pid_t   saved_pid = stream->pid;
    VSTREAM_WAITPID_FN saved_waitpid_fn = stream->waitpid_fn;
    pid_t   pid;
    WAIT_STATUS_T wait_status;

    /*
     * Close the pipe. Don't trigger an alarm in vstream_fclose().
     */
    if (saved_pid == 0)
	msg_panic("vstream_pclose: stream has no process");
    stream->pid = 0;
    vstream_fclose(stream);

    /*
     * Reap the child exit status.
     */
    do {
	if (saved_waitpid_fn != 0)
	    pid = saved_waitpid_fn(saved_pid, &wait_status, 0);
	else
	    pid = waitpid(saved_pid, &wait_status, 0);
    } while (pid == -1 && errno == EINTR);
    return (pid == -1 ? -1 :
	    WIFSIGNALED(wait_status) ? WTERMSIG(wait_status) :
	    WEXITSTATUS(wait_status));
}

#ifdef TEST

#include <fcntl.h>
#include <vstring.h>
#include <vstring_vstream.h>

 /*
  * Test program. Run a command and copy lines one by one.
  */
int     main(int argc, char **argv)
{
    VSTRING *buf = vstring_alloc(100);
    VSTREAM *stream;
    int     status;

    /*
     * Sanity check.
     */
    if (argc < 2)
	msg_fatal("usage: %s 'command'", argv[0]);

    /*
     * Open stream to child process.
     */
    if ((stream = vstream_popen(O_RDWR,
				VSTREAM_POPEN_ARGV, argv + 1,
				VSTREAM_POPEN_END)) == 0)
	msg_fatal("vstream_popen: %m");

    /*
     * Copy loop, one line at a time.
     */
    while (vstring_fgets(buf, stream) != 0) {
	if (vstream_fwrite(VSTREAM_OUT, vstring_str(buf), VSTRING_LEN(buf))
	    != VSTRING_LEN(buf))
	    msg_fatal("vstream_fwrite: %m");
	if (vstream_fflush(VSTREAM_OUT) != 0)
	    msg_fatal("vstream_fflush: %m");
	if (vstring_fgets(buf, VSTREAM_IN) == 0)
	    break;
	if (vstream_fwrite(stream, vstring_str(buf), VSTRING_LEN(buf))
	    != VSTRING_LEN(buf))
	    msg_fatal("vstream_fwrite: %m");
    }

    /*
     * Cleanup.
     */
    vstring_free(buf);
    if ((status = vstream_pclose(stream)) != 0)
	msg_warn("exit status: %d", status);

    exit(status);
}

#endif
