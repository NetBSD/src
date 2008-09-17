/*++
/* NAME
/*	open_lock 3
/* SUMMARY
/*	open or create file and lock it for exclusive access
/* SYNOPSIS
/*	#include <open_lock.h>
/*
/*	VSTREAM	*open_lock(path, flags, mode, why)
/*	const char *path;
/*	int	flags;
/*	mode_t	mode;
/*	VSTRING *why;
/* DESCRIPTION
/*	This module opens or creates the named file and attempts to
/*	acquire an exclusive lock.  The lock is lost when the last
/*	process closes the file.
/*
/*	Arguments:
/* .IP "path, flags, mode"
/*	These are passed on to safe_open().
/* .IP why
/*	storage for diagnostics.
/* SEE ALSO
/*	safe_open(3) carefully open or create file
/*	myflock(3) get exclusive lock on file
/* DIAGNOSTICS
/*	In case of problems the result is a null pointer and a problem
/*	description is returned via the global \fIerrno\fR variable.
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
#include <unistd.h>
#include <fcntl.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <safe_open.h>
#include <myflock.h>
#include <open_lock.h>

/* open_lock - open file and lock it for exclusive access */

VSTREAM *open_lock(const char *path, int flags, mode_t mode, VSTRING *why)
{
    VSTREAM *fp;

    /*
     * Carefully create or open the file, and lock it down. Some systems
     * don't have the O_LOCK open() flag, or the flag does not do what we
     * want, so we roll our own lock.
     */
    if ((fp = safe_open(path, flags, mode, (struct stat *) 0, -1, -1, why)) == 0)
	return (0);
    if (myflock(vstream_fileno(fp), INTERNAL_LOCK,
		MYFLOCK_OP_EXCLUSIVE | MYFLOCK_OP_NOWAIT) < 0) {
	vstring_sprintf(why, "unable to set exclusive lock: %m");
	vstream_fclose(fp);
	return (0);
    }
    return (fp);
}
