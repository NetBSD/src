/*	$NetBSD: argv_splitq.c,v 1.2.4.2 2017/04/21 16:52:52 bouyer Exp $	*/

/*++
/* NAME
/*	argv_splitq 3
/* SUMMARY
/*	string array utilities
/* SYNOPSIS
/*	#include <argv.h>
/*
/*	ARGV	*argv_splitq(string, delim, parens)
/*	const char *string;
/*	const char *delim;
/*	const char *parens;
/*
/*	ARGV	*argv_splitq_count(string, delim, parens, count)
/*	const char *string;
/*	const char *delim;
/*	const char *parens;
/*	ssize_t	count;
/*
/*	ARGV	*argv_splitq_append(argv, string, delim, parens)
/*	ARGV	*argv;
/*	const char *string;
/*	const char *delim;
/*	const char *parens;
/* DESCRIPTION
/*	argv_splitq() breaks up \fIstring\fR into tokens according
/*	to the delimiters specified in \fIdelim\fR, while avoiding
/*	splitting text between matching parentheses. The result is
/*	a null-terminated string array.
/*
/*	argv_splitq_count() is like argv_splitq() but stops splitting
/*	input after at most \fIcount\fR -1 times and leaves the
/*	remainder, if any, in the last array element. It is an error
/*	to specify a count < 1.
/*
/*	argv_splitq_append() performs the same operation as argv_splitq(),
/*	but appends the result to an existing string array.
/* SEE ALSO
/*	mystrtokq(), safe string splitter.
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
#include <string.h>

/* Application-specific. */

#include "mymalloc.h"
#include "stringops.h"
#include "argv.h"
#include "msg.h"

/* argv_splitq - split string into token array */

ARGV   *argv_splitq(const char *string, const char *delim, const char *parens)
{
    ARGV   *argvp = argv_alloc(1);
    char   *saved_string = mystrdup(string);
    char   *bp = saved_string;
    char   *arg;

    while ((arg = mystrtokq(&bp, delim, parens)) != 0)
	argv_add(argvp, arg, (char *) 0);
    argv_terminate(argvp);
    myfree(saved_string);
    return (argvp);
}

/* argv_splitq_count - split string into token array */

ARGV   *argv_splitq_count(const char *string, const char *delim,
			          const char *parens, ssize_t count)
{
    ARGV   *argvp = argv_alloc(1);
    char   *saved_string = mystrdup(string);
    char   *bp = saved_string;
    char   *arg;

    if (count < 1)
	msg_panic("argv_splitq_count: bad count: %ld", (long) count);
    while (count-- > 1 && (arg = mystrtokq(&bp, delim, parens)) != 0)
	argv_add(argvp, arg, (char *) 0);
    if (*bp)
	bp += strspn(bp, delim);
    if (*bp)
	argv_add(argvp, bp, (char *) 0);
    argv_terminate(argvp);
    myfree(saved_string);
    return (argvp);
}

/* argv_splitq_append - split string into token array, append to array */

ARGV   *argv_splitq_append(ARGV *argvp, const char *string, const char *delim,
			           const char *parens)
{
    char   *saved_string = mystrdup(string);
    char   *bp = saved_string;
    char   *arg;

    while ((arg = mystrtokq(&bp, delim, parens)) != 0)
	argv_add(argvp, arg, (char *) 0);
    argv_terminate(argvp);
    myfree(saved_string);
    return (argvp);
}
