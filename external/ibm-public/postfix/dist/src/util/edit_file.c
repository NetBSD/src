/*	$NetBSD: edit_file.c,v 1.1.1.1.10.1 2013/01/23 00:05:16 yamt Exp $	*/

/*++
/* NAME
/*	edit_file 3
/* SUMMARY
/*	simple cooperative file updating protocol
/* SYNOPSIS
/*	#include <edit_file.h>
/*
/*	typedef struct {
/* .in +4
/*		char	*tmp_path;	/* temp. pathname */
/*		VSTREAM *tmp_fp;	/* temp. stream */
/*		/* private members... */
/* .in -4
/*	} EDIT_FILE;
/*
/*	EDIT_FILE *edit_file_open(original_path, output_flags, output_mode)
/*	const char *original_path;
/*	int	output_flags;
/*	mode_t	output_mode;
/*
/*	int	edit_file_close(edit_file)
/*	EDIT_FILE *edit_file;
/*
/*	void	edit_file_cleanup(edit_file)
/*	EDIT_FILE *edit_file;
/* DESCRIPTION
/*	This module implements a simple protocol for cooperative
/*	processes to update one file. The idea is to 1) create a
/*	new file under a deterministic temporary pathname, 2)
/*	populate the new file with updated information, and 3)
/*	rename the new file into the place of the original file.
/*	This module provides 1) and 3), and leaves 2) to the
/*	application. The temporary pathname is deterministic to
/*	avoid accumulation of thrash after program crashes.
/*
/*	edit_file_open() implements the first phase of the protocol.
/*	It creates or opens an output file with a deterministic
/*	temporary pathname, obtained by appending the suffix defined
/*	with EDIT_FILE_SUFFIX to the specified original file pathname.
/*	The original file itself is not opened.  edit_file_open()
/*	then locks the output file for exclusive access, and verifies
/*	that the file still exists under the temporary pathname.
/*	At this point in the protocol, the current process controls
/*	both the output file content and its temporary pathname.
/*
/*	In the second phase, the application opens the original
/*	file if needed, and updates the output file via the
/*	\fBtmp_fp\fR member of the EDIT_FILE data structure.  This
/*	phase is not implemented by the edit_file() module.
/*
/*	edit_file_close() implements the third and final phase of
/*	the protocol.  It flushes the output file to persistent
/*	storage, and renames the output file from its temporary
/*	pathname into the place of the original file. When any of
/*	these operations fails, edit_file_close() behaves as if
/*	edit_file_cleanup() was called. Regardless of whether these
/*	operations suceed, edit_file_close() releases the exclusive
/*	lock, closes the output file, and frees up memory that was
/*	allocated by edit_file_open().
/*
/*	edit_file_cleanup() aborts the protocol. It discards the
/*	output file, releases the exclusive lock, closes the output
/*	file, and frees up memory that was allocated by edit_file_open().
/*
/*	Arguments:
/* .IP original_path
/*	The pathname of the original file that will be replaced by
/*	the output file. The temporary pathname for the output file
/*	is obtained by appending the suffix defined with EDIT_FILE_SUFFIX
/*	to a copy of the specified original file pathname, and is
/*	made available via the \fBtmp_path\fR member of the EDIT_FILE
/*	data structure.
/* .IP output_flags
/*	Flags for opening the output file. These are as with open(2),
/*	except that the O_TRUNC flag is ignored.  edit_file_open()
/*	always truncates the output file after it has obtained
/*	exclusive control over the output file content and temporary
/*	pathname.
/* .IP output_mode
/*	Permissions for the output file. These are as with open(2),
/*	except that the output file is initially created with no
/*	group or other access permissions. The specified output
/*	file permissions are applied by edit_file_close().
/* .IP edit_file
/*	Pointer to data structure that is returned upon successful
/*	completion by edit_file_open(), and that must be passed to
/*	edit_file_close() or edit_file_cleanup().
/* DIAGNOSTICS
/*	Fatal errors: memory allocation failure, fstat() failure,
/*	unlink() failure, lock failure, ftruncate() failure.
/*
/*	edit_file_open() immediately returns a null pointer when
/*	it cannot open the output file.
/*
/*	edit_file_close() returns zero on success, VSTREAM_EOF on
/*	failure.
/*
/*	With both functions, the global errno variable indicates
/*	the nature of the problem.  All errors are relative to the
/*	temporary output's pathname. With both functions, this
/*	pathname is not available via the EDIT_FILE data structure,
/*	because that structure was already destroyed, or not created.
/* BUGS
/*	In the non-error case, edit_file_open() will not return
/*	until it obtains exclusive control over the output file
/*	content and temporary pathname.  Applications that are
/*	concerned about deadlock should protect the edit_file_open()
/*	call with a watchdog timer.
/*
/*	When interrupted, edit_file_close() may leave behind a
/*	world-readable output file under the temporary pathname.
/*	On some systems this can be used to inflict a shared-lock
/*	DOS on the protocol.  Applications that are concerned about
/*	maximal safety should protect the edit_file_close() call
/*	with sigdelay() and sigresume() calls, but this introduces
/*	the risk that the program will get stuck forever.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Based on code originally by:
/*	Victor Duchovni
/*	Morgan Stanley
/*
/*	Packaged into one module with minor improvements by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <stdio.h>			/* rename(2) */
#include <errno.h>

 /*
  * This mask selects all permission bits in the st_mode stat data. There is
  * no portable definition (unlike S_IFMT, which is defined for the file type
  * bits). For example, BSD / Linux have ALLPERMS, while Solaris has S_IAMB.
  */
#define FILE_PERM_MASK \
	(S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO)

/* Utility Library. */

#include <msg.h>
#include <vstream.h>
#include <mymalloc.h>
#include <stringops.h>
#include <myflock.h>
#include <edit_file.h>
#include <warn_stat.h>

 /*
  * Do we reuse and truncate an output file that persists after a crash, or
  * do we unlink it and create a new file?
  */
#define EDIT_FILE_REUSE_AFTER_CRASH

 /*
  * Protocol internals: the temporary file permissions.
  */
#define EDIT_FILE_MODE	(S_IRUSR | S_IWUSR)	/* temp file mode */

 /*
  * Make complex operations more readable. We could use functions, instead.
  * The main thing is that we keep the _alloc and _free code together.
  */
#define EDIT_FILE_ALLOC(ep, path, mode) do { \
	(ep) = (EDIT_FILE *) mymalloc(sizeof(EDIT_FILE)); \
	(ep)->final_path = mystrdup(path); \
	(ep)->final_mode = (mode); \
	(ep)->tmp_path = concatenate((path), EDIT_FILE_SUFFIX, (char *) 0); \
	(ep)->tmp_fp = 0; \
    } while (0)

#define EDIT_FILE_FREE(ep) do { \
	myfree((ep)->final_path); \
	myfree((ep)->tmp_path); \
	myfree((char *) (ep)); \
    } while (0)

/* edit_file_open - open and lock file with deterministic temporary pathname */

EDIT_FILE *edit_file_open(const char *path, int flags, mode_t mode)
{
    struct stat before_lock;
    struct stat after_lock;
    int     saved_errno;
    EDIT_FILE *ep;

    /*
     * Initialize. Do not bother to optimize for the error case.
     */
    EDIT_FILE_ALLOC(ep, path, mode);

    /*
     * As long as the output file can be opened under the temporary pathname,
     * this code can loop or block forever.
     * 
     * Applications that are concerned about deadlock should protect the
     * edit_file_open() call with a watchdog timer.
     */
    for ( /* void */ ; /* void */ ; (void) vstream_fclose(ep->tmp_fp)) {

	/*
	 * Try to open the output file under the temporary pathname. This
	 * succeeds or fails immediately. To avoid creating a shared-lock DOS
	 * opportunity after we crash, we create the output file with no
	 * group or other permissions, and set the final permissions at the
	 * end (this is one reason why we try to get exclusive control over
	 * the output file instead of the original file). We postpone file
	 * truncation until we have obtained exclusive control over the file
	 * content and temporary pathname. If the open operation fails, we
	 * give up immediately. The caller can retry the call if desirable.
	 * 
	 * XXX If we replace the vstream_fopen() call by safe_open(), then we
	 * should replace the stat() call below by lstat().
	 */
	if ((ep->tmp_fp = vstream_fopen(ep->tmp_path, flags & ~(O_TRUNC),
				    EDIT_FILE_MODE)) == 0) {
	    saved_errno = errno;
	    EDIT_FILE_FREE(ep);
	    errno = saved_errno;
	    return (0);
	}

	/*
	 * At this point we may have opened an existing output file that was
	 * already locked. Try to lock the open file exclusively. This may
	 * take some time.
	 */
	if (myflock(vstream_fileno(ep->tmp_fp), INTERNAL_LOCK,
		    MYFLOCK_OP_EXCLUSIVE) < 0)
	    msg_fatal("lock %s: %m", ep->tmp_path);

	/*
	 * At this point we have an exclusive lock, but some other process
	 * may have renamed or removed the output file while we were waiting
	 * for the lock. If that is the case, back out and try again.
	 */
	if (fstat(vstream_fileno(ep->tmp_fp), &before_lock) < 0)
	    msg_fatal("open %s: %m", ep->tmp_path);
	if (stat(ep->tmp_path, &after_lock) < 0
	    || before_lock.st_dev != after_lock.st_dev
	    || before_lock.st_ino != after_lock.st_ino
#ifdef HAS_ST_GEN
	    || before_lock.st_gen != after_lock.st_gen
#endif
	/* No need to compare st_rdev or st_nlink here. */
	    ) {
	    continue;
	}

	/*
	 * At this point we have exclusive control over the output file
	 * content and its temporary pathname (within the rules of the
	 * cooperative protocol). But wait, there is more.
	 * 
	 * There are many opportunies for trouble when opening a pre-existing
	 * output file. Here are just a few.
	 * 
	 * - Victor observes that a system crash in the middle of the
	 * final-phase rename() operation may result in the output file
	 * having both the temporary pathname and the final pathname. In that
	 * case we must not write to the output file.
	 * 
	 * - Wietse observes that crashes may also leave the output file in
	 * other inconsistent states. To avoid permission-related trouble, we
	 * simply refuse to work with an output file that has the wrong
	 * temporary permissions. This won't stop the shared-lock DOS if we
	 * crash after changing the file permissions, though.
	 * 
	 * To work around these crash-related problems, remove the temporary
	 * pathname, back out, and try again.
	 */
	if (!S_ISREG(after_lock.st_mode)
#ifndef EDIT_FILE_REUSE_AFTER_CRASH
	    || after_lock.st_size > 0
#endif
	    || after_lock.st_nlink > 1
	    || (after_lock.st_mode & FILE_PERM_MASK) != EDIT_FILE_MODE) {
	    if (unlink(ep->tmp_path) < 0 && errno != ENOENT)
		msg_fatal("unlink %s: %m", ep->tmp_path);
	    continue;
	}

	/*
	 * Settle the final details.
	 */
#ifdef EDIT_FILE_REUSE_AFTER_CRASH
	if (ftruncate(vstream_fileno(ep->tmp_fp), 0) < 0)
	    msg_fatal("truncate %s: %m", ep->tmp_path);
#endif
	return (ep);
    }
}

/* edit_file_cleanup - clean up without completing the protocol */

void    edit_file_cleanup(EDIT_FILE *ep)
{

    /*
     * Don't touch the file after we lose the exclusive lock!
     */
    if (unlink(ep->tmp_path) < 0 && errno != ENOENT)
	msg_fatal("unlink %s: %m", ep->tmp_path);
    (void) vstream_fclose(ep->tmp_fp);
    EDIT_FILE_FREE(ep);
}

/* edit_file_close - rename the file into place and and close the file */

int     edit_file_close(EDIT_FILE *ep)
{
    VSTREAM *fp = ep->tmp_fp;
    int     fd = vstream_fileno(fp);
    int     saved_errno;

    /*
     * The rename/unlock portion of the protocol is relatively simple. The
     * only things that really matter here are that we change permissions as
     * late as possible, and that we rename the file to its final pathname
     * before we lose the exclusive lock.
     * 
     * Applications that are concerned about maximal safety should protect the
     * edit_file_close() call with sigdelay() and sigresume() calls. It is
     * not safe for us to call these functions directly, because the calls do
     * not nest. It is also not nice to force every caller to run with
     * interrupts turned off.
     */
    if (vstream_fflush(fp) < 0
	|| fchmod(fd, ep->final_mode) < 0
#ifdef HAS_FSYNC
	|| fsync(fd) < 0
#endif
	|| rename(ep->tmp_path, ep->final_path) < 0) {
	saved_errno = errno;
	edit_file_cleanup(ep);
	errno = saved_errno;
	return (VSTREAM_EOF);
    } else {
	(void) vstream_fclose(ep->tmp_fp);
	EDIT_FILE_FREE(ep);
	return (0);
    }
}
