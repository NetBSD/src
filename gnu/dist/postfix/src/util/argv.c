/*++
/* NAME
/*	argv 3
/* SUMMARY
/*	string array utilities
/* SYNOPSIS
/*	#include <argv.h>
/*
/*	ARGV	*argv_alloc(len)
/*	int	len;
/*
/*	ARGV	*argv_free(argvp)
/*	ARGV	*argvp;
/*
/*	void	argv_add(argvp, arg, ..., ARGV_END)
/*	ARGV	*argvp;
/*	char	*arg;
/*
/*	void	argv_terminate(argvp);
/*	ARGV	*argvp;
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
/*	is not null terminated.
/*
/*	argv_add() copies zero or more strings and adds them to the
/*	specified string array. The array is not null terminated.
/*	Terminate the argument list with a null pointer. The manifest
/*	constant ARGV_END provides a convenient notation for this.
/*
/*	argv_free() releases storage for a string array, and conveniently
/*	returns a null pointer.
/*
/*	argv_terminate() null-terminates its string array argument.
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

ARGV   *argv_alloc(int len)
{
    ARGV   *argvp;

    /*
     * Make sure that always argvp->argc < argvp->len.
     */
    argvp = (ARGV *) mymalloc(sizeof(*argvp));
    argvp->len = (len < 2 ? 2 : len);
    argvp->argv = (char **) mymalloc((argvp->len + 1) * sizeof(char *));
    argvp->argc = 0;
    argvp->argv[0] = 0;
    return (argvp);
}

/* argv_add - add string to vector */

void    argv_add(ARGV *argvp,...)
{
    char   *arg;
    va_list ap;

    /*
     * Make sure that always argvp->argc < argvp->len.
     */
    va_start(ap, argvp);
    while ((arg = va_arg(ap, char *)) != 0) {
	if (argvp->argc + 1 >= argvp->len) {
	    argvp->len *= 2;
	    argvp->argv = (char **)
		myrealloc((char *) argvp->argv,
			  (argvp->len + 1) * sizeof(char *));
	}
	argvp->argv[argvp->argc++] = mystrdup(arg);
    }
    va_end(ap);
}

/* argv_terminate - terminate string array */

void    argv_terminate(ARGV *argvp)
{

    /*
     * Trust that argvp->argc < argvp->len.
     */
    argvp->argv[argvp->argc] = 0;
}
