/*	$NetBSD: deliver_flock.c,v 1.1.1.1 2009/06/23 10:08:45 tron Exp $	*/

/*++
/* NAME
/*	deliver_flock 3
/* SUMMARY
/*	lock open file for mail delivery
/* SYNOPSIS
/*	#include <deliver_flock.h>
/*
/*	int	deliver_flock(fd, lock_style, why)
/*	int	fd;
/*	int	lock_style;
/*	VSTRING	*why;
/* DESCRIPTION
/*	deliver_flock() sets one exclusive kernel lock on an open file,
/*	for example in order to deliver mail.
/*	It performs several non-blocking attempts to acquire an exclusive
/*	lock before giving up.
/*
/*	Arguments:
/* .IP fd
/*	A file descriptor that is associated with an open file.
/* .IP lock_style
/*	A locking style defined in myflock(3).
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
#include <iostuff.h>

/* Global library. */

#include "mail_params.h"
#include "deliver_flock.h"

/* Application-specific. */

#define MILLION	1000000

/* deliver_flock - lock open file for mail delivery */

int     deliver_flock(int fd, int lock_style, VSTRING *why)
{
    int     i;

    for (i = 1; /* void */ ; i++) {
	if (myflock(fd, lock_style,
		    MYFLOCK_OP_EXCLUSIVE | MYFLOCK_OP_NOWAIT) == 0)
	    return (0);
	if (i >= var_flock_tries)
	    break;
	rand_sleep(var_flock_delay * MILLION, var_flock_delay * MILLION / 2);
    }
    if (why)
	vstring_sprintf(why, "unable to lock for exclusive access: %m");
    return (-1);
}
