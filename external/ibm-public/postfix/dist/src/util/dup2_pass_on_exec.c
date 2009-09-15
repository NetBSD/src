/*	$NetBSD: dup2_pass_on_exec.c,v 1.1.1.1.2.2 2009/09/15 06:03:57 snj Exp $	*/

/*++
/* NAME
/*	dup2_pass_on_exec 1
/* SUMMARY
/*	dup2 close-on-exec behaviour test program
/* SYNOPSIS
/*	dup2_pass_on_exec
/* DESCRIPTION
/*	dup2_pass_on_exec sets the close-on-exec flag on its
/*	standard input and then dup2() to duplicate it.
/*	Posix-1003.1 specifies in section 6.2.1.2 that dup2(o,n) should behave
/*	as: close(n); n = fcntl(o, F_DUPFD, n); as long as o is a valid
/*	file-descriptor, n!=o, and 0<=n<=[OPEN_MAX].
/*	Section 6.5.2.2 states that the close-on-exec flag of the result of a
/*	successful fcntl(o, F_DUPFD, n) is cleared.
/*
/*	At least Ultrix4.3a does not clear the close-on-exec flag of n on
/*	dup2(o, n).
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Christian von Roques <roques@pond.sub.org>
/*	Forststrasse 71
/*	76131 Karlsruhe, GERMANY
/*--*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define DO(s)	if (s < 0) { perror(#s); exit(1); }

int     main(int unused_argc, char **unused_argv)
{
    int     res;

    printf("Setting the close-on-exec flag of file-descriptor 0.\n");
    DO(fcntl(0, F_SETFD, 1));

    printf("Duplicating file-descriptor 0 to 3.\n");
    DO(dup2(0, 3));

    printf("Testing if the close-on-exec flag of file-descriptor 3 is set.\n");
    DO((res = fcntl(3, F_GETFD, 0)));
    if (res & 1)
	printf(
"Yes, a newly dup2()ed file-descriptor has the close-on-exec \
flag cloned.\n\
THIS VIOLATES Posix1003.1 section 6.2.1.2 or 6.5.2.2!\n\
You should #define DUP2_DUPS_CLOSE_ON_EXEC in sys_defs.h \
for your OS.\n");
    else
	printf(
"No, a newly dup2()ed file-descriptor has the close-on-exec \
flag cleared.\n\
This complies with Posix1003.1 section 6.2.1.2 and 6.5.2.2!\n");

    return 0;
}
