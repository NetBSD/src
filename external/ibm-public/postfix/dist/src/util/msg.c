/*	$NetBSD: msg.c,v 1.1.1.2 2013/01/02 18:59:13 tron Exp $	*/

/*++
/* NAME
/*	msg 3
/* SUMMARY
/*	diagnostic interface
/* SYNOPSIS
/*	#include <msg.h>
/*
/*	int	msg_verbose;
/*
/*	void	msg_info(format, ...)
/*	const char *format;
/*
/*	void	vmsg_info(format, ap)
/*	const char *format;
/*	va_list	ap;
/*
/*	void	msg_warn(format, ...)
/*	const char *format;
/*
/*	void	vmsg_warn(format, ap)
/*	const char *format;
/*	va_list	ap;
/*
/*	void	msg_error(format, ...)
/*	const char *format;
/*
/*	void	vmsg_error(format, ap)
/*	const char *format;
/*	va_list	ap;
/*
/*	NORETURN msg_fatal(format, ...)
/*	const char *format;
/*
/*	NORETURN vmsg_fatal(format, ap)
/*	const char *format;
/*	va_list	ap;
/*
/*	NORETURN msg_fatal_status(status, format, ...)
/*	int	status;
/*	const char *format;
/*
/*	NORETURN vmsg_fatal_status(status, format, ap)
/*	int	status;
/*	const char *format;
/*	va_list	ap;
/*
/*	NORETURN msg_panic(format, ...)
/*	const char *format;
/*
/*	NORETURN vmsg_panic(format, ap)
/*	const char *format;
/*	va_list	ap;
/*
/*	MSG_CLEANUP_FN msg_cleanup(cleanup)
/*	void (*cleanup)(void);
/* AUXILIARY FUNCTIONS
/*	int	msg_error_limit(count)
/*	int	count;
/*
/*	void	msg_error_clear()
/* DESCRIPTION
/*	This module reports diagnostics. By default, diagnostics are sent
/*	to the standard error stream, but the disposition can be changed
/*	by the user. See the hints below in the SEE ALSO section.
/*
/*	msg_info(), msg_warn(), msg_error(), msg_fatal*() and msg_panic()
/*	produce a one-line record with the program name, a severity code
/*	(except for msg_info()), and an informative message. The program
/*	name must have been set by calling one of the msg_XXX_init()
/*	functions (see the SEE ALSO section).
/*
/*	msg_error() reports a recoverable error and increments the error
/*	counter. When the error count exceeds a pre-set limit (default: 13)
/*	the program terminates by calling msg_fatal().
/*
/*	msg_fatal() reports an unrecoverable error and terminates the program
/*	with a non-zero exit status.
/*
/*	msg_fatal_status() reports an unrecoverable error and terminates the
/*	program with the specified exit status.
/*
/*	msg_panic() reports an internal inconsistency, terminates the
/*	program immediately (i.e. without calling the optional user-specified
/*	cleanup routine), and forces a core dump when possible.
/*
/*	msg_cleanup() specifies a function that msg_fatal[_status]() should
/*	invoke before terminating the program, and returns the
/*	current function pointer. Specify a null argument to disable
/*	this feature.
/*
/*	msg_error_limit() sets the error message count limit, and returns.
/*	the old limit.
/*
/*	msg_error_clear() sets the error message count to zero.
/*
/*	msg_verbose is a global flag that can be set to make software
/*	more verbose about what it is doing. By default the flag is zero.
/*	By convention, a larger value means more noise.
/* REENTRANCY
/* .ad
/* .fi
/*	The msg_info() etc. output routines are protected against
/*	ordinary recursive calls and against re-entry by signal
/*	handlers.
/*
/*	Protection against re-entry by signal handlers is subject
/*	to the following limitations:
/* .IP \(bu
/*	The signal handlers must never return. In other words, the
/*	signal handlers must do one or more of the following: call
/*	_exit(), kill the process with a signal, and permanently block
/*	the process.
/* .IP \(bu
/*	The signal handlers must invoke msg_info() etc. not until
/*	after the msg_XXX_init() functions complete initialization,
/*	and not until after the first formatted output to a VSTRING
/*	or VSTREAM.
/* .IP \(bu
/*	Each msg_cleanup() call-back function, and each Postfix or
/*	system function invoked by that call-back function, either
/*	protects itself against recursive calls and re-entry by a
/*	terminating signal handler, or is called exclusively by the
/*	msg(3) module.
/* .PP
/*	When re-entrancy is detected, the requested output and
/*	optional cleanup operations are skipped. Skipping the output
/*	operations prevents memory corruption of VSTREAM_ERR data
/*	structures, and prevents deadlock on Linux releases that
/*	use mutexes within system library routines such as syslog().
/*	This protection exists under the condition that these
/*	specific resources are accessed exclusively via the msg_info()
/*	etc.  functions.
/* SEE ALSO
/*	msg_output(3) specify diagnostics disposition
/*	msg_stdio(3) direct diagnostics to standard I/O stream
/*	msg_vstream(3) direct diagnostics to VSTREAM.
/*	msg_syslog(3) direct diagnostics to syslog daemon
/* BUGS
/*	Some output functions may suffer from intentional or accidental
/*	record length restrictions that are imposed by library routines
/*	and/or by the runtime environment.
/*
/*	Code that spawns a child process should almost always reset
/*	the cleanup handler. The exception is when the parent exits
/*	immediately and the child continues.
/*
/*	msg_cleanup() may be unsafe in code that changes process
/*	privileges, because the call-back routine may run with the
/*	wrong privileges.
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
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

/* Application-specific. */

#include "msg.h"
#include "msg_output.h"

 /*
  * Default is verbose logging off.
  */
int     msg_verbose = 0;

 /*
  * Private state.
  */
static MSG_CLEANUP_FN msg_cleanup_fn = 0;
static int msg_error_count = 0;
static int msg_error_bound = 13;

 /*
  * The msg_exiting flag prevents us from recursively reporting an error with
  * msg_fatal*() or msg_panic(), and provides a first-level safety net for
  * optional cleanup actions against signal handler re-entry problems. Note
  * that msg_vprintf() implements its own guard against re-entry.
  * 
  * XXX We specify global scope, to discourage the compiler from doing smart
  * things.
  */
volatile int msg_exiting = 0;

/* msg_info - report informative message */

void    msg_info(const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    vmsg_info(fmt, ap);
    va_end(ap);
}

void    vmsg_info(const char *fmt, va_list ap)
{
    msg_vprintf(MSG_INFO, fmt, ap);
}

/* msg_warn - report warning message */

void    msg_warn(const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    vmsg_warn(fmt, ap);
    va_end(ap);
}

void    vmsg_warn(const char *fmt, va_list ap)
{
    msg_vprintf(MSG_WARN, fmt, ap);
}

/* msg_error - report recoverable error */

void    msg_error(const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    vmsg_error(fmt, ap);
    va_end(ap);
}

void    vmsg_error(const char *fmt, va_list ap)
{
    msg_vprintf(MSG_ERROR, fmt, ap);
    if (++msg_error_count >= msg_error_bound)
	msg_fatal("too many errors - program terminated");
}

/* msg_fatal - report error and terminate gracefully */

NORETURN msg_fatal(const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    vmsg_fatal(fmt, ap);
    /* NOTREACHED */
}

NORETURN vmsg_fatal(const char *fmt, va_list ap)
{
    if (msg_exiting++ == 0) {
	msg_vprintf(MSG_FATAL, fmt, ap);
	if (msg_cleanup_fn)
	    msg_cleanup_fn();
    }
    sleep(1);
    /* In case we're running as a signal handler. */
    _exit(1);
}

/* msg_fatal_status - report error and terminate gracefully */

NORETURN msg_fatal_status(int status, const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    vmsg_fatal_status(status, fmt, ap);
    /* NOTREACHED */
}

NORETURN vmsg_fatal_status(int status, const char *fmt, va_list ap)
{
    if (msg_exiting++ == 0) {
	msg_vprintf(MSG_FATAL, fmt, ap);
	if (msg_cleanup_fn)
	    msg_cleanup_fn();
    }
    sleep(1);
    /* In case we're running as a signal handler. */
    _exit(status);
}

/* msg_panic - report error and dump core */

NORETURN msg_panic(const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    vmsg_panic(fmt, ap);
    /* NOTREACHED */
}

NORETURN vmsg_panic(const char *fmt, va_list ap)
{
    if (msg_exiting++ == 0) {
	msg_vprintf(MSG_PANIC, fmt, ap);
    }
    sleep(1);
    abort();					/* Die! */
    /* In case we're running as a signal handler. */
    _exit(1);					/* DIE!! */
}

/* msg_cleanup - specify cleanup routine */

MSG_CLEANUP_FN msg_cleanup(MSG_CLEANUP_FN cleanup_fn)
{
    MSG_CLEANUP_FN old_fn = msg_cleanup_fn;

    msg_cleanup_fn = cleanup_fn;
    return (old_fn);
}

/* msg_error_limit - set error message counter limit */

int     msg_error_limit(int limit)
{
    int     old = msg_error_bound;

    msg_error_bound = limit;
    return (old);
}

/* msg_error_clear - reset error message counter */

void    msg_error_clear(void)
{
    msg_error_count = 0;
}
