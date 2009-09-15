/*	$NetBSD: doze.c,v 1.1.1.1.2.2 2009/09/15 06:03:57 snj Exp $	*/

/*++
/* NAME
/*	doze 3
/* SUMMARY
/*	take a nap
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	void	doze(microseconds)
/*	unsigned microseconds;
/* DESCRIPTION
/*	doze() sleeps for the specified number of microseconds. It is
/*	a simple alternative for systems that lack usleep().
/* DIAGNOSTICS
/*	All errors are fatal.
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

#include "sys_defs.h"
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#ifdef USE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

/* doze - sleep a while */

void    doze(unsigned delay)
{
    struct timeval tv;

#define MILLION	1000000

    tv.tv_sec = delay / MILLION;
    tv.tv_usec = delay % MILLION;
    while (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &tv) < 0)
	if (errno != EINTR)
	    msg_fatal("doze: select: %m");
}

#ifdef TEST

#include <stdlib.h>

int     main(int argc, char **argv)
{
    unsigned delay;

    if (argc != 2 || (delay = atol(argv[1])) == 0)
	msg_fatal("usage: %s microseconds", argv[0]);
    doze(delay);
    exit(0);
}

#endif
