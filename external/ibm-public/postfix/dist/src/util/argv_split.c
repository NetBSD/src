/*	$NetBSD: argv_split.c,v 1.1.1.1.2.2 2009/09/15 06:03:53 snj Exp $	*/

/*++
/* NAME
/*	argv_split 3
/* SUMMARY
/*	string array utilities
/* SYNOPSIS
/*	#include <argv.h>
/*
/*	ARGV	*argv_split(string, delim)
/*	const char *string;
/*
/*	ARGV	*argv_split_count(string, delim, count)
/*	const char *string;
/*	ssize_t	count;
/*
/*	ARGV	*argv_split_append(argv, string, delim)
/*	ARGV	*argv;
/*	const char *string;
/*	const char *delim;
/* DESCRIPTION
/*	argv_split() breaks up \fIstring\fR into tokens according
/*	to the delimiters specified in \fIdelim\fR. The result is
/*	a null-terminated string array.
/*
/*	argv_split_count() is like argv_split() but stops splitting
/*	input after at most \fIcount\fR -1 times and leaves the
/*	remainder, if any, in the last array element. It is an error
/*	to specify a count < 1.
/* 
/*	argv_split_append() performs the same operation as argv_split(),
/*	but appends the result to an existing string array.
/* SEE ALSO
/*	mystrtok(), safe string splitter.
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

/* argv_split - split string into token array */

ARGV   *argv_split(const char *string, const char *delim)
{
    ARGV   *argvp = argv_alloc(1);
    char   *saved_string = mystrdup(string);
    char   *bp = saved_string;
    char   *arg;

    while ((arg = mystrtok(&bp, delim)) != 0)
	argv_add(argvp, arg, (char *) 0);
    argv_terminate(argvp);
    myfree(saved_string);
    return (argvp);
}

/* argv_split_count - split string into token array */

ARGV   *argv_split_count(const char *string, const char *delim, ssize_t count)
{
    ARGV   *argvp = argv_alloc(1);
    char   *saved_string = mystrdup(string);
    char   *bp = saved_string;
    char   *arg;

    if (count < 1)
	msg_panic("argv_split_count: bad count: %ld", (long) count);
    while (count-- > 1 && (arg = mystrtok(&bp, delim)) != 0)
	argv_add(argvp, arg, (char *) 0);
    if (*bp)
	bp += strspn(bp, delim);
    if (*bp)
	argv_add(argvp, bp, (char *) 0);
    argv_terminate(argvp);
    myfree(saved_string);
    return (argvp);
}

/* argv_split_append - split string into token array, append to array */

ARGV   *argv_split_append(ARGV *argvp, const char *string, const char *delim)
{
    char   *saved_string = mystrdup(string);
    char   *bp = saved_string;
    char   *arg;

    while ((arg = mystrtok(&bp, delim)) != 0)
	argv_add(argvp, arg, (char *) 0);
    argv_terminate(argvp);
    myfree(saved_string);
    return (argvp);
}
