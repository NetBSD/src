/*++
/* NAME
/*	dot_lockfile 3
/* SUMMARY
/*	dotlock file management
/* SYNOPSIS
/*	#include <dot_lockfile.h>
/*
/*	int	dot_lockfile(path, why)
/*	const char *path;
/*	VSTRING	*why;
/*
/*	void	dot_unlockfile(path)
/*	const char *path;
/* DESCRIPTION
/*	dot_lockfile() constructs a lock file name by appending ".lock" to
/*	\fIpath\fR and creates the named file exclusively. It tries several
/*	times and attempts to break stale locks. A negative result value
/*	means no lock file could be created.
/*
/*	dot_unlockfile() attempts to remove the lock file created by
/*	dot_lockfile().
/*
/*	Arguments:
/* .IP path
/*	Name of the file to be locked or unlocked.
/* .IP why
/*	A null pointer, or storage for the reason why a lock file could
/*	not be created.
/* CONFIGURATION PARAMETERS
/*	deliver_lock_attempts, how many times to try to create a lock
/*	deliver_lock_delay, how long to wait between attempts
/*	stale_lock_time, when to break a stale lock
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
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

/* Utility library. */

#include <vstring.h>
#include <stringops.h>
#include <mymalloc.h>

/* Global library. */

#include "mail_params.h"
#include "dot_lockfile.h"

/* dot_lockfile - create user.lock file */

int     dot_lockfile(const char *path, VSTRING *why)
{
    char   *lock_file;
    int     count;
    struct stat st;
    int     fd;
    int     status = -1;

    lock_file = concatenate(path, ".lock", (char *) 0);

    for (count = 0; /* void */ ; count++) {
	if (count >= var_flock_tries)
	    break;
	if (count > 0)
	    sleep(var_flock_delay);

	/*
	 * Attempt to create the lock. This code relies on O_EXCL | O_CREAT
	 * to not follow symlinks.
	 */
	if ((fd = open(lock_file, O_WRONLY | O_EXCL | O_CREAT, 0)) >= 0) {
	    close(fd);
	    status = 0;
	    break;
	}

	/*
	 * We can deal only with "file exists" errors. Any other error means
	 * we better give up trying.
	 */
	if (errno != EEXIST)
	    break;

	/*
	 * Break the lock when it is too old. Give up when we are unable to
	 * remove a stale lock.
	 */
	if (stat(lock_file, &st) == 0)
	    if (time((time_t *) 0) > st.st_ctime + var_flock_stale)
		if (unlink(lock_file) < 0)
		    if (errno != ENOENT)
			break;
	errno = EEXIST;
    }
    if (status && why)
	vstring_sprintf(why, "unable to create lock file %s: %m", lock_file);

    myfree(lock_file);
    return (status);
}

/* dot_unlockfile - remove .lock file */

void    dot_unlockfile(const char *path)
{
    char   *lock_file;

    lock_file = concatenate(path, ".lock", (char *) 0);
    (void) unlink(lock_file);
    myfree(lock_file);
}

#ifdef TEST

 /*
  * Test program for setting a .lock file.
  * 
  * Usage: dot_lockfile filename
  * 
  * Creates filename.lock and removes it.
  */
#include <msg.h>
#include <vstream.h>
#include <msg_vstream.h>
#include <mail_conf.h>

main(int argc, char **argv)
{
    VSTRING *why = vstring_alloc(100);

    msg_vstream_init(argv[0], VSTREAM_ERR);
    if (argc != 2)
	msg_fatal("usage: %s file-to-be-locked", argv[0]);
    mail_conf_read();
    if (dot_lockfile(argv[1], why) < 0)
	msg_fatal("%s", vstring_str(why));
    dot_unlockfile(argv[1]);
    vstring_free(why);
    return (0);
}

#endif
