/*	$NetBSD: argv.c,v 1.1.1.1 2009/06/23 10:08:58 tron Exp $	*/

/*++
/* NAME
/*	argv 3
/* SUMMARY
/*	string array utilities
/* SYNOPSIS
/*	#include <argv.h>
/*
/*	ARGV	*argv_alloc(len)
/*	ssize_t	len;
/*
/*	ARGV	*argv_free(argvp)
/*	ARGV	*argvp;
/*
/*	void	argv_add(argvp, arg, ..., ARGV_END)
/*	ARGV	*argvp;
/*	char	*arg;
/*
/*	void	argv_addn(argvp, arg, arg_len, ..., ARGV_END)
/*	ARGV	*argvp;
/*	char	*arg;
/*	ssize_t	arg_len;
/*
/*	void	argv_terminate(argvp);
/*	ARGV	*argvp;
/*
/*	void	argv_truncate(argvp, len);
/*	ARGV	*argvp;
/*	ssize_t	len;
/* DESCRIPTION
/*	The functions in this module manipulate arrays of string
/*	pointers. An ARGV structure contains the following members:
/* .IP len
/*	The length of the \fIargv\fR array member.
/* .IP argc
/*	The number of \fIargv\fR elements used.
/* .IP argv
/*	An array of pointers to null-terminated strings.
/* .PP
/*	argv_alloc() returns an empty string array of the requested
/*	length. The result is ready for use by argv_add(). The array
/*	is null terminated.
/*
/*	argv_add() copies zero or more strings and adds them to the
/*	specified string array. The array is null terminated.
/*	Terminate the argument list with a null pointer. The manifest
/*	constant ARGV_END provides a convenient notation for this.
/*
/*	argv_addn() is like argv_add(), but each string is followed
/*	by a string length argument.
/*
/*	argv_free() releases storage for a string array, and conveniently
/*	returns a null pointer.
/*
/*	argv_terminate() null-terminates its string array argument.
/*
/*	argv_truncate() trucates its argument to the specified
/*	number of entries, but does not reallocate memory. The
/*	result is null-terminated.
/* SEE ALSO
/*	msg(3) diagnostics interface
/* DIAGNOSTICS
/*	Fatal errors: memory allocation problem.
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
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <string.h>

/* Application-specific. */

#include "mymalloc.h"
#include "msg.h"
#include "argv.h"

/* argv_free - destroy string array */

ARGV   *argv_free(ARGV *argvp)
{
    char  **cpp;

    for (cpp = argvp->argv; cpp < argvp->argv + argvp->argc; cpp++)
	myfree(*cpp);
    myfree((char *) argvp->argv);
    myfree((char *) argvp);
    return (0);
}

/* argv_alloc - initialize string array */

ARGV   *argv_alloc(ssize_t len)
{
    ARGV   *argvp;
    ssize_t sane_len;

    /*
     * Make sure that always argvp->argc < argvp->len.
     */
    argvp = (ARGV *) mymalloc(sizeof(*argvp));
    argvp->len = 0;
    sane_len = (len < 2 ? 2 : len);
    argvp->argv = (char **) mymalloc((sane_len + 1) * sizeof(char *));
    argvp->len = sane_len;
    argvp->argc = 0;
    argvp->argv[0] = 0;
    return (argvp);
}

/* argv_extend - extend array */

static void argv_extend(ARGV *argvp)
{
    ssize_t new_len;

    new_len = argvp->len * 2;
    argvp->argv = (char **)
	myrealloc((char *) argvp->argv, (new_len + 1) * sizeof(char *));
    argvp->len = new_len;
}

/* argv_add - add string to vector */

void    argv_add(ARGV *argvp,...)
{
    char   *arg;
    va_list ap;

    /*
     * Make sure that always argvp->argc < argvp->len.
     */
#define ARGV_SPACE_LEFT(a) ((a)->len - (a)->argc - 1)

    va_start(ap, argvp);
    while ((arg = va_arg(ap, char *)) != 0) {
	if (ARGV_SPACE_LEFT(argvp) <= 0)
	    argv_extend(argvp);
	argvp->argv[argvp->argc++] = mystrdup(arg);
    }
    va_end(ap);
    argvp->argv[argvp->argc] = 0;
}

/* argv_addn - add string to vector */

void    argv_addn(ARGV *argvp,...)
{
    char   *arg;
    ssize_t len;
    va_list ap;

    /*
     * Make sure that always argvp->argc < argvp->len.
     */
    va_start(ap, argvp);
    while ((arg = va_arg(ap, char *)) != 0) {
	if ((len = va_arg(ap, ssize_t)) < 0)
	    msg_panic("argv_addn: bad string length %ld", (long) len);
	if (ARGV_SPACE_LEFT(argvp) <= 0)
	    argv_extend(argvp);
	argvp->argv[argvp->argc++] = mystrndup(arg, len);
    }
    va_end(ap);
    argvp->argv[argvp->argc] = 0;
}

/* argv_terminate - terminate string array */

void    argv_terminate(ARGV *argvp)
{

    /*
     * Trust that argvp->argc < argvp->len.
     */
    argvp->argv[argvp->argc] = 0;
}

/* argv_truncate - truncate string array */

void    argv_truncate(ARGV *argvp, ssize_t len)
{
    char  **cpp;

    /*
     * Sanity check.
     */
    if (len < 0)
	msg_panic("argv_truncate: bad length %ld", (long) len);

    if (len < argvp->argc) {
	for (cpp = argvp->argv + len; cpp < argvp->argv + argvp->argc; cpp++)
	    myfree(*cpp);
	argvp->argc = len;
	argvp->argv[argvp->argc] = 0;
    }
}
