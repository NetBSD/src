/*++
/* NAME
/*	safe_open 3
/* SUMMARY
/*	safely open or create regular file
/* SYNOPSIS
/*	#include <safe_open.h>
/*
/*	VSTREAM	*safe_open(path, flags, mode, st, user, group, why)
/*	const char *path;
/*	int	flags;
/*	int	mode;
/*	struct stat *st;
/*	uid_t	user;
/*	gid_t	group;
/*	VSTRING	*why;
/* DESCRIPTION
/*	safe_open() carefully opens or creates a file in a directory
/*	that may be writable by untrusted users. If a file is created
/*	it is given the specified ownership and permission attributes.
/*	If an existing file is opened it must not be a symbolic link,
/*	it must not be a directory, and it must have only one hard link.
/*
/*	Arguments:
/* .IP "path, flags, mode"
/*	These arguments are the same as with open(2). The O_EXCL flag
/*	must appear either in combination with O_CREAT, or not at all.
/* .sp
/*	No change is made to the permissions of an existing file.
/* .IP st
/*	Null pointer, or pointer to storage for the attributes of the
/*	opened file.
/* .IP "user, group"
/*	File ownership for a file created by safe_open(). Specify -1
/*	in order to disable user and/or group ownership change.
/* .sp
/*	No change is made to the ownership of an existing file.
/* .IP why
/*	A VSTRING pointer for diagnostics.
/* DIAGNOSTICS
/*	Panic: interface violations.
/*
/*	A null result means there was a problem.  The nature of the
/*	problem is returned via the \fIwhy\fR buffer; when an error
/*	cannot be reported via \fIerrno\fR, the generic value EPERM
/*	(operation not permitted) is used instead.
/* HISTORY
/* .fi
/* .ad
/*	A safe open routine was discussed by Casper Dik in article
/*	<2rdb0s$568@mail.fwi.uva.nl>, posted to comp.security.unix
/*	(May 18, 1994).
/*
/*	Olaf Kirch discusses how the lstat()/open()+fstat() test can
/*	be fooled by delaying the open() until the inode found with
/*	lstat() has been re-used for a sensitive file (article
/*	<20000103212443.A5807@monad.swb.de> posted to bugtraq on
/*	Jan 3, 2000).  This can be a concern for a set-ugid process
/*	that runs under the control of a user and that can be
/*	manipulated with start/stop signals.
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
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <safe_open.h>

/* safe_open_exist - open existing file */

static VSTREAM *safe_open_exist(const char *path, int flags,
				        struct stat * fstat_st, VSTRING *why)
{
    struct stat local_statbuf;
    struct stat lstat_st;
    VSTREAM *fp;

    /*
     * Open an existing file.
     */
    if ((fp = vstream_fopen(path, flags & ~(O_CREAT | O_EXCL), 0)) == 0) {
	vstring_sprintf(why, "cannot open file: %m");
	return (0);
    }

    /*
     * Examine the modes from the open file: it must have exactly one hard
     * link (so that someone can't lure us into clobbering a sensitive file
     * by making a hard link to it), and it must be a non-symlink file.
     */
    if (fstat_st == 0)
	fstat_st = &local_statbuf;
    if (fstat(vstream_fileno(fp), fstat_st) < 0) {
	msg_fatal("%s: bad open file status: %m", path);
    } else if (fstat_st->st_nlink != 1) {
	vstring_sprintf(why, "file has %d hard links",
			(int) fstat_st->st_nlink);
	errno = EPERM;
    } else if (S_ISDIR(fstat_st->st_mode)) {
	vstring_sprintf(why, "file is a directory");
	errno = EISDIR;
    }

    /*
     * Look up the file again, this time using lstat(). Compare the fstat()
     * (open file) modes with the lstat() modes. If there is any difference,
     * either we followed a symlink while opening an existing file, someone
     * quickly changed the number of hard links, or someone replaced the file
     * after the open() call. The link and mode tests aren't really necessary
     * in daemon processes. Set-uid programs, on the other hand, can be
     * slowed down by arbitrary amounts, and there it would make sense to
     * compare even more file attributes, such as the inode generation number
     * on systems that have one.
     * 
     * Grr. Solaris /dev/whatever is a symlink. We'll have to make an exception
     * for symlinks owned by root. NEVER, NEVER, make exceptions for symlinks
     * owned by a non-root user. This would open a security hole when
     * delivering mail to a world-writable mailbox directory.
     */
    else if (lstat(path, &lstat_st) < 0) {
	vstring_sprintf(why, "file status changed unexpectedly: %m");
	errno = EPERM;
    } else if (S_ISLNK(lstat_st.st_mode)) {
	if (lstat_st.st_uid == 0)
	    return (fp);
	vstring_sprintf(why, "file is a symbolic link");
	errno = EPERM;
    } else if (fstat_st->st_dev != lstat_st.st_dev
	       || fstat_st->st_ino != lstat_st.st_ino
#ifdef HAS_ST_GEN
	       || fstat_st->st_gen != lstat_st.st_gen
#endif
	       || fstat_st->st_nlink != lstat_st.st_nlink
	       || fstat_st->st_mode != lstat_st.st_mode) {
	vstring_sprintf(why, "file status changed unexpectedly");
	errno = EPERM;
    }

    /*
     * We are almost there...
     */
    else {
	return (fp);
    }

    /*
     * End up here in case of fstat()/lstat() problems or inconsistencies.
     */
    vstream_fclose(fp);
    return (0);
}

/* safe_open_create - create new file */

static VSTREAM *safe_open_create(const char *path, int flags, int mode,
	            struct stat * st, uid_t user, uid_t group, VSTRING *why)
{
    VSTREAM *fp;

    /*
     * Create a non-existing file. This relies on O_CREAT | O_EXCL to not
     * follow symbolic links.
     */
    if ((fp = vstream_fopen(path, flags | (O_CREAT | O_EXCL), mode)) == 0) {
	vstring_sprintf(why, "cannot create file exclusively: %m");
	return (0);
    }

    /*
     * Optionally change ownership after creating a new file. If there is a
     * problem we should not attempt to delete the file. Something else may
     * have opened the file in the mean time.
     */
#define CHANGE_OWNER(user, group) (user != (uid_t) -1 || group != (gid_t) -1)

    if (CHANGE_OWNER(user, group)
	&& fchown(vstream_fileno(fp), user, group) < 0) {
	msg_warn("%s: cannot change file ownership: %m", path);
    }

    /*
     * Optionally look up the file attributes.
     */
    if (st != 0 && fstat(vstream_fileno(fp), st) < 0)
	msg_fatal("%s: bad open file status: %m", path);

    /*
     * We are almost there...
     */
    else {
	return (fp);
    }

    /*
     * End up here in case of trouble.
     */
    vstream_fclose(fp);
    return (0);
}

/* safe_open - safely open or create file */

VSTREAM *safe_open(const char *path, int flags, int mode,
	            struct stat * st, uid_t user, gid_t group, VSTRING *why)
{
    VSTREAM *fp;

    switch (flags & (O_CREAT | O_EXCL)) {

	/*
	 * Open an existing file, carefully.
	 */
    case 0:
	return (safe_open_exist(path, flags, st, why));

	/*
	 * Create a new file, carefully.
	 */
    case O_CREAT | O_EXCL:
	return (safe_open_create(path, flags, mode, st, user, group, why));

	/*
	 * Open an existing file or create a new one, carefully. When opening
	 * an existing file, we are prepared to deal with "no file" errors
	 * only. Any other error means we better give up trying.
	 */
    case O_CREAT:
	if ((fp = safe_open_exist(path, flags, st, why)) == 0)
	    if (errno == ENOENT)
		fp = safe_open_create(path, flags, mode, st, user, group, why);
	return (fp);

	/*
	 * Interface violation. Sorry, but we must be strict.
	 */
    default:
	msg_panic("safe_open: O_EXCL flag without O_CREAT flag");
    }
}
