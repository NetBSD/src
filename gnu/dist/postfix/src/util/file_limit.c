/*++
/* NAME
/*	file_limit 3
/* SUMMARY
/*	limit the file size
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	off_t	get_file_limit()
/*
/*	void	set_file_limit(limit)
/*	off_t	limit;
/* DESCRIPTION
/*	This module manipulates the process-wide file size limit.
/*	The limit is specified in bytes.
/*
/*	get_file_limit() looks up the process-wide file size limit.
/*
/*	set_file_limit() sets the process-wide file size limit to
/*	\fIlimit\fR.
/* DIAGNOSTICS
/*	All errors are fatal.
/* SEE ALSO
/*	setrlimit(2)
/*	ulimit(2)
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

#include <sys_defs.h>
#ifdef USE_ULIMIT
#include <ulimit.h>
#else
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#endif
#include <limits.h>

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

#define ULIMIT_BLOCK_SIZE	512

/* get_file_limit - get process-wide file size limit */

off_t   get_file_limit(void)
{
    off_t   limit;

#ifdef USE_ULIMIT
    if ((limit = ulimit(UL_GETFSIZE, 0)) < 0)
	msg_fatal("ulimit: %m");
    if (limit > INT_MAX / ULIMIT_BLOCK_SIZE)
	limit = INT_MAX / ULIMIT_BLOCK_SIZE;
    return (limit * ULIMIT_BLOCK_SIZE);
#else
    struct rlimit rlim;

    if (getrlimit(RLIMIT_FSIZE, &rlim) < 0)
	msg_fatal("getrlimit: %m");
    limit = rlim.rlim_cur;
    return (limit < 0 ? INT_MAX : rlim.rlim_cur);
#endif						/* USE_ULIMIT */
}

/* set_file_limit - process-wide file size limit */

void    set_file_limit(off_t limit)
{
#ifdef USE_ULIMIT
    if (ulimit(UL_SETFSIZE, limit / ULIMIT_BLOCK_SIZE) < 0)
	msg_fatal("ulimit: %m");
#else
    struct rlimit rlim;

    rlim.rlim_cur = rlim.rlim_max = limit;
    if (setrlimit(RLIMIT_FSIZE, &rlim) < 0)
	msg_fatal("setrlimit: %m");
#ifdef SIGXFSZ
    if (signal(SIGXFSZ, SIG_IGN) == SIG_ERR)
	msg_fatal("signal(SIGXFSZ,SIG_IGN): %m");
#endif
#endif						/* USE_ULIMIT */
}
