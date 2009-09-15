/*	$NetBSD: duplex_pipe.c,v 1.1.1.1.2.2 2009/09/15 06:03:57 snj Exp $	*/

/*++
/* NAME
/*	duplex_pipe 3
/* SUMMARY
/*	local IPD
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	int	duplex_pipe(fds)
/*	int	*fds;
/* DESCRIPTION
/*	duplex_pipe() uses whatever local primitive it takes
/*	to get a two-way I/O channel.
/* DIAGNOSTICS
/*	A null result means success. In case of error, the result
/*	is -1 and errno is set to the appropriate number.
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

/* System libraries */

#include <sys_defs.h>
#include <sys/socket.h>
#include <unistd.h>

/* Utility library. */

#include "iostuff.h"
#include "sane_socketpair.h"

/* duplex_pipe - give me a duplex pipe or bust */

int     duplex_pipe(int *fds)
{
#ifdef HAS_DUPLEX_PIPE
    return (pipe(fds));
#else
    return (sane_socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
#endif
}

