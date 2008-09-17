/*++
/* NAME
/*	sane_socketpair 3
/* SUMMARY
/*	sanitize socketpair() error returns
/* SYNOPSIS
/*	#include <sane_socketpair.h>
/*
/*	int	sane_socketpair(domain, type, protocol, result)
/*	int	domain;
/*	int	type;
/*	int	protocol;
/*	int	*result;
/* DESCRIPTION
/*	sane_socketpair() implements the socketpair(2) socket call, and
/*	skips over silly error results such as EINTR.
/* BUGS
/*	Bizarre systems may have other harmless error results. Such
/*	systems encourage programmers to ignore error results, and
/*	penalize programmers who code defensively.
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
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

/* Utility library. */

#include "msg.h"
#include "sane_socketpair.h"

/* sane_socketpair - sanitize socketpair() error returns */

int     sane_socketpair(int domain, int type, int protocol, int *result)
{
    static int socketpair_ok_errors[] = {
	EINTR,
	0,
    };
    int     count;
    int     err;
    int     ret;

    /*
     * Solaris socketpair() can fail with EINTR.
     */
    while ((ret = socketpair(domain, type, protocol, result)) < 0) {
	for (count = 0; /* void */ ; count++) {
	    if ((err = socketpair_ok_errors[count]) == 0)
		return (ret);
	    if (errno == err) {
		msg_warn("socketpair: %m (trying again)");
		sleep(1);
		break;
	    }
	}
    }
    return (ret);
}
