/*	$NetBSD: argv_split_at.c,v 1.2 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	argv_split_at 3
/* SUMMARY
/*	string array utilities
/* SYNOPSIS
/*	#include <argv.h>
/*
/*	ARGV	*argv_split_at(string, sep)
/*	const char *string;
/*	int	sep;
/*
/*	ARGV	*argv_split_at_count(string, sep, count)
/*	const char *string;
/*	int	sep;
/*	ssize_t	count;
/*
/*	ARGV	*argv_split_at_append(argv, string, sep)
/*	ARGV	*argv;
/*	const char *string;
/*	int	sep;
/* DESCRIPTION
/*	argv_split_at() splits \fIstring\fR into fields using a
/*	single separator specified in \fIsep\fR. The result is a
/*	null-terminated string array.
/*
/*	argv_split_at_count() is like argv_split_at() but stops
/*	splitting input after at most \fIcount\fR -1 times and
/*	leaves the remainder, if any, in the last array element.
/*	It is an error to specify a count < 1.
/*
/*	argv_split_at_append() performs the same operation as
/*	argv_split_at(), but appends the result to an existing
/*	string array.
/* SEE ALSO
/*	split_at(), trivial string splitter.
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System libraries. */

#include <sys_defs.h>
#include <string.h>

/* Application-specific. */

#include <mymalloc.h>
#include <stringops.h>
#include <argv.h>
#include <msg.h>
#include <split_at.h>

/* argv_split_at - split string into field array */

ARGV   *argv_split_at(const char *string, int sep)
{
    ARGV   *argvp = argv_alloc(1);
    char   *saved_string = mystrdup(string);
    char   *bp = saved_string;
    char   *arg;

    while ((arg = split_at(bp, sep)) != 0) {
	argv_add(argvp, bp, (char *) 0);
	bp = arg;
    }
    argv_add(argvp, bp, (char *) 0);
    argv_terminate(argvp);
    myfree(saved_string);
    return (argvp);
}

/* argv_split_at_count - split string into field array */

ARGV   *argv_split_at_count(const char *string, int sep, ssize_t count)
{
    ARGV   *argvp = argv_alloc(1);
    char   *saved_string = mystrdup(string);
    char   *bp = saved_string;
    char   *arg;

    if (count < 1)
	msg_panic("argv_split_at_count: bad count: %ld", (long) count);
    while (count-- > 1 && (arg = split_at(bp, sep)) != 0) {
	argv_add(argvp, bp, (char *) 0);
	bp = arg;
    }
    argv_add(argvp, bp, (char *) 0);
    argv_terminate(argvp);
    myfree(saved_string);
    return (argvp);
}

/* argv_split_at_append - split string into field array, append to array */

ARGV   *argv_split_at_append(ARGV *argvp, const char *string, int sep)
{
    char   *saved_string = mystrdup(string);
    char   *bp = saved_string;
    char   *arg;

    while ((arg = split_at(bp, sep)) != 0) {
	argv_add(argvp, bp, (char *) 0);
	bp = arg;
    }
    argv_add(argvp, bp, (char *) 0);
    argv_terminate(argvp);
    myfree(saved_string);
    return (argvp);
}
