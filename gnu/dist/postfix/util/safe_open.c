/*++
/* NAME
/*	safe_open 3
/* SUMMARY
/*	safely open or create regular file
/* SYNOPSIS
/*	#include <safe_open.h>
/*
/*	VSTREAM	*safe_open(path, flags, mode, user, group, why)
/*	const char *path;
/*	int	flags;
/*	int	mode;
/*	uid_t	user;
/*	gid_t	group;
/*	VSTRING	*why;
/* DESCRIPTION
/*	safe_open() carefully opens or creates a file in a directory
/*	that may be writable by untrusted users. If a file is created
/*	it is given the specified ownership and permission attributes.
/*	If an existing file is opened it must be a regular file with
/*	only one hard link.
/*
/*	Arguments:
/* .IP "path, flags, mode"
/*	These arguments are the same as with open(2). The O_EXCL flag
/*	must appear either in combination with O_CREAT, or not at all.
/* .sp
/*	No change is made to the permissions of an existing file.
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
/*	problem is returned via the \fIwhy\fR buffer; some errors
/*	cannot be reported via \fIerrno\fR.
/* HISTORY
/* .fi
/* .ad
/*	A safe open routine was discussed by Casper Dik in article
/*	<2rdb0s$568@mail.fwi.uva.nl>, posted to comp.security.unix
/*	(May 18, 1994).
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

static VSTREAM *safe_open_exist(const char *path, int flags, VSTRING *why)
{
    struct stat fstat_st;
    struct stat lstat_st;
    VSTREAM *fp;

    /*
     * Open an existing file.
     */
    if ((fp = vstream_fopen(path, flags & ~(O_CREAT | O_EXCL), 0)) == 0) {
	vstring_sprintf(why, "error opening file %s: %m", path);
	return (0);
    }

    /*
     * Examine the modes from the open file: it must have exactly one hard
     * link (so that someone can't lure us into clobbering a sensitive file
     * by making a hard link to it), and it must be a regular file.
     */
    if (fstat(vstream_fileno(fp), &fstat_st) < 0) {
	vstring_sprintf(why, "file %s: bad status: %m", path);
    } else if (S_ISREG(fstat_st.st_mode) == 0) {
	vstring_sprintf(why, "file %s: must be a regular file", path);
    } else if (fstat_st.st_nlink != 1) {
	vstring_sprintf(why, "file %s: must have one hard link", path);
    }

    /*
     * Look up the file again, this time using lstat(). Compare the fstat()
     * (open file) modes with the lstat() modes. If there is any difference,
     * either we followed a symlink while opening an existing file, someone
     * quickly changed the number of hard links, or someone replaced the file
     * after the open() call. The link and mode tests aren't really necessary
     * but the additional cost is low.
     */
    else if (lstat(path, &lstat_st) < 0
	     || fstat_st.st_dev != lstat_st.st_dev
	     || fstat_st.st_ino != lstat_st.st_ino
	     || fstat_st.st_nlink != lstat_st.st_nlink
	     || fstat_st.st_mode != lstat_st.st_mode) {
	vstring_sprintf(why, "file %s: status has changed", path);
    }

    /*
     * We are almost there...
     */
    else {
	return (fp);
    }

    /*
     * End up here in case of fstat()/lstat() problems or inconsistencies.
     * Reset errno to reduce confusion.
     */
    errno = 0;
    vstream_fclose(fp);
    return (0);
}

/* safe_open_create - create new file */

static VSTREAM *safe_open_create(const char *path, int flags, int mode,
			              uid_t user, uid_t group, VSTRING *why)
{
    VSTREAM *fp;

    /*
     * Create a non-existing file. This relies on O_CREAT | O_EXCL to not
     * follow symbolic links.
     */
    if ((fp = vstream_fopen(path, flags | (O_CREAT | O_EXCL), mode)) == 0) {
	vstring_sprintf(why, "error opening file %s: %m", path);
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
	vstring_sprintf(why, "error changing ownership of %s: %m", path);
    }

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
		           uid_t user, gid_t group, VSTRING *why)
{
    VSTREAM *fp;

    switch (flags & (O_CREAT | O_EXCL)) {

	/*
	 * Open an existing file, carefully.
	 */
    case 0:
	return (safe_open_exist(path, flags, why));

	/*
	 * Create a new file, carefully.
	 */
    case O_CREAT | O_EXCL:
	return (safe_open_create(path, flags, mode, user, group, why));

	/*
	 * Open an existing file or create a new one, carefully. When opening
	 * an existing file, we are prepared to deal with "no file" errors
	 * only. Any other error means we better give up trying.
	 */
    case O_CREAT:
	if ((fp = safe_open_exist(path, flags, why)) == 0)
	    if (errno == ENOENT)
		fp = safe_open_create(path, flags, mode, user, group, why);
	return (fp);

	/*
	 * Interface violation. Sorry, but we must be strict.
	 */
    default:
	msg_panic("safe_open: O_EXCL flag without O_CREAT flag");
    }
}
