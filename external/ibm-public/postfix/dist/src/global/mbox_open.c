/*	$NetBSD: mbox_open.c,v 1.1.1.1.2.2 2009/09/15 06:02:47 snj Exp $	*/

/*++
/* NAME
/*	mbox_open 3
/* SUMMARY
/*	mailbox access
/* SYNOPSIS
/*	#include <mbox_open.h>
/*
/*	typedef struct {
/* .in +4
/*		/* public members... */
/*		VSTREAM	*fp;
/* .in -4
/*	} MBOX;
/*
/*	MBOX	*mbox_open(path, flags, mode, st, user, group, lock_style,
/*				def_dsn, why)
/*	const char *path;
/*	int	flags;
/*	mode_t	mode;
/*	struct stat *st;
/*	uid_t	user;
/*	gid_t	group;
/*	int	lock_style;
/*	const char *def_dsn;
/*	DSN_BUF	*why;
/*
/*	void	mbox_release(mbox)
/*	MBOX	*mbox;
/*
/*	const char *mbox_dsn(err, def_dsn)
/*	int	err;
/*	const char *def_dsn;
/* DESCRIPTION
/*	This module manages access to UNIX mailbox-style files.
/*
/*	mbox_open() acquires exclusive access to the named file.
/*	The \fBpath, flags, mode, st, user, group, why\fR arguments
/*	are passed to the \fBsafe_open\fR() routine. Attempts to change
/*	file ownership will succeed only if the process runs with
/*	adequate effective privileges.
/*	The \fBlock_style\fR argument specifies a lock style from
/*	mbox_lock_mask(). Locks are applied to regular files only.
/*	The result is a handle that must be destroyed by mbox_release().
/*	The \fBdef_dsn\fR argument is given to mbox_dsn().
/*
/*	mbox_release() releases the named mailbox. It is up to the
/*	application to close the stream.
/*
/*	mbox_dsn() translates an errno value to a mailbox related
/*	enhanced status code.
/* .IP "EAGAIN, ESTALE"
/*	These result in a 4.2.0 soft error (mailbox problem).
/* .IP ENOSPC
/*	This results in a 4.3.0 soft error (mail system full).
/* .IP "EDQUOT, EFBIG"
/*	These result in a 5.2.2 hard error (mailbox full).
/* .PP
/*	All other errors are assigned the specified default error
/*	code. Typically, one would specify 4.2.0 or 5.2.0.
/* DIAGNOSTICS
/*	mbox_open() returns a null pointer in case of problems, and
/*	sets errno to EAGAIN if someone else has exclusive access.
/*	Other errors are likely to have a more permanent nature.
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
#include <errno.h>

#ifndef EDQUOT
#define EDQUOT EFBIG
#endif

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <safe_open.h>
#include <iostuff.h>
#include <mymalloc.h>

/* Global library. */

#include <dot_lockfile.h>
#include <deliver_flock.h>
#include <mbox_conf.h>
#include <mbox_open.h>

/* mbox_open - open mailbox-style file for exclusive access */

MBOX   *mbox_open(const char *path, int flags, mode_t mode, struct stat * st,
		          uid_t chown_uid, gid_t chown_gid,
		          int lock_style, const char *def_dsn,
		          DSN_BUF *why)
{
    struct stat local_statbuf;
    MBOX   *mp;
    int     locked = 0;
    VSTREAM *fp;

    if (st == 0)
	st = &local_statbuf;

    /*
     * If this is a regular file, create a dotlock file. This locking method
     * does not work well over NFS, but it is better than some alternatives.
     * With NFS, creating files atomically is a problem, and a successful
     * operation can fail with EEXIST.
     * 
     * If filename.lock can't be created for reasons other than "file exists",
     * issue only a warning if the application says it is non-fatal. This is
     * for bass-awkward compatibility with existing installations that
     * deliver to files in non-writable directories.
     * 
     * We dot-lock the file before opening, so we must avoid doing silly things
     * like dot-locking /dev/null. Fortunately, deliveries to non-mailbox
     * files execute with recipient privileges, so we don't have to worry
     * about creating dotlock files in places where the recipient would not
     * be able to write.
     * 
     * Note: we use stat() to follow symlinks, because safe_open() allows the
     * target to be a root-owned symlink, and we don't want to create dotlock
     * files for /dev/null or other non-file objects.
     */
    if ((lock_style & MBOX_DOT_LOCK)
	&& (stat(path, st) < 0 || S_ISREG(st->st_mode))) {
	if (dot_lockfile(path, why->reason) == 0) {
	    locked |= MBOX_DOT_LOCK;
	} else if (errno == EEXIST) {
	    dsb_status(why, mbox_dsn(EAGAIN, def_dsn));
	    return (0);
	} else if (lock_style & MBOX_DOT_LOCK_MAY_FAIL) {
	    msg_warn("%s", vstring_str(why->reason));
	} else {
	    dsb_status(why, mbox_dsn(errno, def_dsn));
	    return (0);
	}
    }

    /*
     * Open or create the target file. In case of a privileged open, the
     * privileged user may be attacked with hard/soft link tricks in an
     * unsafe parent directory. In case of an unprivileged open, the mail
     * system may be attacked by a malicious user-specified path, or the
     * unprivileged user may be attacked with hard/soft link tricks in an
     * unsafe parent directory. Open non-blocking to fend off attacks
     * involving non-file targets.
     */
    if ((fp = safe_open(path, flags | O_NONBLOCK, mode, st,
			chown_uid, chown_gid, why->reason)) == 0) {
	dsb_status(why, mbox_dsn(errno, def_dsn));
	if (locked & MBOX_DOT_LOCK)
	    dot_unlockfile(path);
	return (0);
    }
    close_on_exec(vstream_fileno(fp), CLOSE_ON_EXEC);

    /*
     * If this is a regular file, acquire kernel locks. flock() locks are not
     * intended to work across a network; fcntl() locks are supposed to work
     * over NFS, but in the real world, NFS lock daemons often have serious
     * problems.
     */
#define HUNKY_DORY(lock_mask, myflock_style) ((lock_style & (lock_mask)) == 0 \
         || deliver_flock(vstream_fileno(fp), (myflock_style), why->reason) == 0)

    if (S_ISREG(st->st_mode)) {
	if (HUNKY_DORY(MBOX_FLOCK_LOCK, MYFLOCK_STYLE_FLOCK)
	    && HUNKY_DORY(MBOX_FCNTL_LOCK, MYFLOCK_STYLE_FCNTL)) {
	    locked |= lock_style;
	} else {
	    dsb_status(why, mbox_dsn(errno, def_dsn));
	    if (locked & MBOX_DOT_LOCK)
		dot_unlockfile(path);
	    vstream_fclose(fp);
	    return (0);
	}
    }

    /*
     * Sanity check: reportedly, GNU POP3D creates a new mailbox file and
     * deletes the old one. This does not play well with software that opens
     * the mailbox first and then locks it, such as software that that uses
     * FCNTL or FLOCK locks on open file descriptors (some UNIX systems don't
     * use dotlock files).
     * 
     * To detect that GNU POP3D deletes the mailbox file we look at the target
     * file hard-link count. Note that safe_open() guarantees a hard-link
     * count of 1, so any change in this count is a sign of trouble.
     */
    if (S_ISREG(st->st_mode)
	&& (fstat(vstream_fileno(fp), st) < 0 || st->st_nlink != 1)) {
	vstring_sprintf(why->reason, "target file status changed unexpectedly");
	dsb_status(why, mbox_dsn(EAGAIN, def_dsn));
	msg_warn("%s: file status changed unexpectedly", path);
	if (locked & MBOX_DOT_LOCK)
	    dot_unlockfile(path);
	vstream_fclose(fp);
	return (0);
    }
    mp = (MBOX *) mymalloc(sizeof(*mp));
    mp->path = mystrdup(path);
    mp->fp = fp;
    mp->locked = locked;
    return (mp);
}

/* mbox_release - release mailbox exclusive access */

void    mbox_release(MBOX *mp)
{

    /*
     * Unfortunately we can't close the stream, because on some file systems
     * (AFS), the only way to find out if a file was written successfully is
     * to close it, and therefore the close() operation is in the mail_copy()
     * routine. If we really insist on owning the vstream member, then we
     * should export appropriate methods that mail_copy() can use in order to
     * manipulate a message stream.
     */
    if (mp->locked & MBOX_DOT_LOCK)
	dot_unlockfile(mp->path);
    myfree(mp->path);
    myfree((char *) mp);
}

/* mbox_dsn - map errno value to mailbox-related DSN detail */

const char *mbox_dsn(int err, const char *def_dsn)
{
#define TRY_AGAIN_ERROR(e) \
	(e == EAGAIN || e == ESTALE)
#define SYSTEM_FULL_ERROR(e) \
	(e == ENOSPC)
#define MBOX_FULL_ERROR(e) \
	(e == EDQUOT || e == EFBIG)

    return (TRY_AGAIN_ERROR(err) ? "4.2.0" :
	    SYSTEM_FULL_ERROR(err) ? "4.3.0" :
	    MBOX_FULL_ERROR(err) ? "5.2.2" :
	    def_dsn);
}
