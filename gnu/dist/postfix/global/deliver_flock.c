/*++
/* NAME
/*	deliver_flock 3
/* SUMMARY
/*	lock open file for mail delivery
/* SYNOPSIS
/*	#include <deliver_flock.h>
/*
/*	int	deliver_flock(fd, why)
/*	int	fd;
/*	VSTRING	*why;
/* DESCRIPTION
/*	deliver_flock() sets one exclusive kernel lock on an open file
/*	for the purpose of mail delivery. It attempts to acquire
/*	the exclusive lock several times before giving up.
/*
/*	Arguments:
/* .IP fd
/*	A file descriptor that is associated with an open file.
/* .IP why
/*	A null pointer, or storage for diagnostics.
/* DIAGNOSTICS
/*	deliver_flock() returns -1 in case of problems, 0 in case
/*	of success. The reason for failure is returned via the \fIwhy\fR
/*	parameter.
/* CONFIGURATION PARAMETERS
/*	deliver_lock_attempts, number of locking attempts
/*	deliver_lock_delay, time in seconds between attempts
/*	sun_mailtool_compatibility, disable kernel locking
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
#include <unistd.h>

/* Utility library. */

#include <vstring.h>
#include <myflock.h>

/* Global library. */

#include "mail_params.h"
#include "deliver_flock.h"

/* deliver_flock - lock open file for mail delivery*/

int     deliver_flock(int fd, VSTRING *why)
{
    int     i;

    if (var_mailtool_compat)
	return (0);

    for (i = 0; /* void */ ; i++) {
	if (i >= var_flock_tries)
	    break;
	if (i > 0)
	    sleep(var_flock_delay);
	if (myflock(fd, MYFLOCK_EXCLUSIVE | MYFLOCK_NOWAIT) == 0)
	    return (0);
    }
    if (why)
	vstring_sprintf(why, "unable to lock exclusively: %m");
    return (-1);
}
