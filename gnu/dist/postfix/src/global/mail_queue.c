/*++
/* NAME
/*	mail_queue 3
/* SUMMARY
/*	mail queue file access
/* SYNOPSIS
/*	#include <mail_queue.h>
/*
/*	VSTREAM	*mail_queue_enter(queue_name, mode)
/*	const char *queue_name;
/*	int	mode;
/*
/*	VSTREAM	*mail_queue_open(queue_name, queue_id, flags, mode)
/*	const char *queue_name;
/*	const char *queue_id;
/*	int	flags;
/*	int	mode;
/*
/*	char	*mail_queue_dir(buf, queue_name, queue_id)
/*	VSTRING	*buf;
/*	const char *queue_name;
/*	const char *queue_id;
/*
/*	char	*mail_queue_path(buf, queue_name, queue_id)
/*	VSTRING	*buf;
/*	const char *queue_name;
/*	const char *queue_id;
/*
/*	int	mail_queue_mkdirs(path)
/*	const char *path;
/*
/*	int	mail_queue_rename(queue_id, old_queue, new_queue)
/*	const char *queue_id;
/*	const char *old_queue;
/*	const char *new_queue;
/*
/*	int	mail_queue_remove(queue_name, queue_id)
/*	const char *queue_name;
/*	const char *queue_id;
/*
/*	int	mail_queue_name_ok(queue_name)
/*	const char *queue_name;
/*
/*	int	mail_queue_id_ok(queue_id)
/*	const char *queue_id;
/* DESCRIPTION
/*	This module encapsulates access to the mail queue hierarchy.
/*	Unlike most other modules, this one does not abort the program
/*	in case of file access problems. But it does abort when the
/*	application attempts to use a malformed queue name or queue id.
/*
/*	mail_queue_enter() creates an entry in the named queue. The queue
/*	id is the file base name, see VSTREAM_PATH().  Queue ids are
/*	relatively short strings and are recycled in the course of time.
/*	The only guarantee given is that on a given machine, no two queue
/*	entries will have the same queue ID at the same time.
/*
/*	mail_queue_open() opens the named queue file. The \fIflags\fR
/*	and \fImode\fR arguments are as with open(2). The result is a
/*	null pointer in case of problems.
/*
/*	mail_queue_dir() returns the directory name of the specified queue
/*	file. When a null result buffer pointer is provided, the result is
/*	written to a private buffer that may be overwritten upon the next
/*	call.
/*
/*	mail_queue_path() returns the pathname of the specified queue
/*	file. When a null result buffer pointer is provided, the result
/*	is written to a private buffer that may be overwritten upon the
/*	next call.
/*
/*	mail_queue_mkdirs() creates missing parent directories
/*	for the file named in \fBpath\fR. A non-zero result means
/*	that the operation failed.
/*
/*	mail_queue_rename() renames a queue file. A non-zero result
/*	means the operation failed.
/*
/*	mail_queue_remove() removes the named queue file. A non-zero result
/*	means the operation failed.
/*
/*	mail_queue_name_ok() validates a mail queue name and returns
/*	non-zero (true) if the name contains no nasty characters.
/*
/*	mail_queue_id_ok() does the same thing for mail queue ID names.
/* DIAGNOSTICS
/*	Panic: invalid queue name or id given to mail_queue_path(),
/*	mail_queue_rename(), or mail_queue_remove().
/*	Fatal error: out of memory.
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
#include <stdio.h>			/* rename() */
#include <stdlib.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>			/* gettimeofday, not in POSIX */
#include <string.h>
#include <errno.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <mymalloc.h>
#include <argv.h>
#include <dir_forest.h>
#include <make_dirs.h>
#include <split_at.h>
#include <sane_fsops.h>
#include <valid_hostname.h>

/* Global library. */

#include "file_id.h"
#include "mail_params.h"
#include "mail_queue.h"

#define STR	vstring_str

/* mail_queue_dir - construct mail queue directory name */

const char *mail_queue_dir(VSTRING *buf, const char *queue_name,
			           const char *queue_id)
{
    char   *myname = "mail_queue_dir";
    static VSTRING *private_buf = 0;
    static VSTRING *hash_buf = 0;
    static ARGV *hash_queue_names = 0;
    char  **cpp;

    /*
     * Sanity checks.
     */
    if (mail_queue_name_ok(queue_name) == 0)
	msg_panic("%s: bad queue name: %s", myname, queue_name);
    if (mail_queue_id_ok(queue_id) == 0)
	msg_panic("%s: bad queue id: %s", myname, queue_id);

    /*
     * Initialize.
     */
    if (buf == 0) {
	if (private_buf == 0)
	    private_buf = vstring_alloc(100);
	buf = private_buf;
    }
    if (hash_buf == 0) {
	hash_buf = vstring_alloc(100);
	hash_queue_names = argv_split(var_hash_queue_names, " \t\r\n,");
    }

    /*
     * First, put the basic queue directory name into place.
     */
    vstring_strcpy(buf, queue_name);
    vstring_strcat(buf, "/");

    /*
     * Then, see if we need to append a little directory forest.
     */
    for (cpp = hash_queue_names->argv; *cpp; cpp++) {
	if (strcasecmp(*cpp, queue_name) == 0) {
	    vstring_strcat(buf,
		      dir_forest(hash_buf, queue_id, var_hash_queue_depth));
	    break;
	}
    }
    return (STR(buf));
}

/* mail_queue_path - map mail queue id to path name */

const char *mail_queue_path(VSTRING *buf, const char *queue_name,
			            const char *queue_id)
{
    static VSTRING *private_buf = 0;

    /*
     * Initialize.
     */
    if (buf == 0) {
	if (private_buf == 0)
	    private_buf = vstring_alloc(100);
	buf = private_buf;
    }

    /*
     * Append the queue id to the possibly hashed queue directory.
     */
    (void) mail_queue_dir(buf, queue_name, queue_id);
    vstring_strcat(buf, queue_id);
    return (STR(buf));
}

/* mail_queue_mkdirs - fill in missing directories */

int     mail_queue_mkdirs(const char *path)
{
    char   *myname = "mail_queue_mkdirs";
    char   *saved_path = mystrdup(path);
    int     ret;

    /*
     * Truncate a copy of the pathname (for safety sake), and create the
     * missing directories.
     */
    if (split_at_right(saved_path, '/') == 0)
	msg_panic("%s: no slash in: %s", myname, saved_path);
    ret = make_dirs(saved_path, 0700);
    myfree(saved_path);
    return (ret);
}

/* mail_queue_rename - move message to another queue */

int     mail_queue_rename(const char *queue_id, const char *old_queue,
			          const char *new_queue)
{
    VSTRING *old_buf = vstring_alloc(100);
    VSTRING *new_buf = vstring_alloc(100);
    int     error;

    /*
     * Try the operation. If it fails, see if it is because of missing
     * intermediate directories.
     */
    error = sane_rename(mail_queue_path(old_buf, old_queue, queue_id),
			mail_queue_path(new_buf, new_queue, queue_id));
    if (error != 0 && mail_queue_mkdirs(STR(new_buf)) == 0)
	error = sane_rename(STR(old_buf), STR(new_buf));

    /*
     * Cleanup.
     */
    vstring_free(old_buf);
    vstring_free(new_buf);

    return (error);
}

/* mail_queue_remove - remove mail queue file */

int     mail_queue_remove(const char *queue_name, const char *queue_id)
{
    return (REMOVE(mail_queue_path((VSTRING *) 0, queue_name, queue_id)));
}

/* mail_queue_name_ok - validate mail queue name */

int     mail_queue_name_ok(const char *queue_name)
{
    const char *cp;

    if (*queue_name == 0 || strlen(queue_name) > 100)
	return (0);

    for (cp = queue_name; *cp; cp++)
	if (!ISALNUM(*cp))
	    return (0);
    return (1);
}

/* mail_queue_id_ok - validate mail queue id */

int     mail_queue_id_ok(const char *queue_id)
{
    const char *cp;

    if (*queue_id == 0 || strlen(queue_id) > 100)
	return (0);

    /*
     * OK if in in time+inum form.
     */
    for (cp = queue_id; /* void */ ; cp++) {
	if (*cp == 0)
	    return (1);
	if (!ISALNUM(*cp))
	    break;
    }

    /*
     * BAD if in time.pid form.
     */
    for (cp = queue_id; /* void */ ; cp++) {
	if (*cp == 0)
	    return (0);
	if (!ISDIGIT(*cp) && *cp != '.')
	    break;
    }

    /*
     * OK if in valid hostname form.
     */
    return (valid_hostname(queue_id, DO_GRIPE));
}

/* mail_queue_enter - make mail queue entry with locally-unique name */

VSTREAM *mail_queue_enter(const char *queue_name, int mode)
{
    char   *myname = "mail_queue_enter";
    static VSTRING *id_buf;
    static int pid;
    static VSTRING *path_buf;
    static VSTRING *temp_path;
    struct timeval tv;
    int     fd;
    const char *file_id;
    VSTREAM *stream;
    int     count;

    /*
     * Initialize.
     */
    if (id_buf == 0) {
	pid = getpid();
	id_buf = vstring_alloc(10);
	path_buf = vstring_alloc(10);
	temp_path = vstring_alloc(100);
    }
    GETTIMEOFDAY(&tv);

    /*
     * Create a file with a temporary name that does not collide. The process
     * ID alone is not sufficiently unique: maildrops can be shared via the
     * network. Not that I recommend using a network-based queue, or having
     * multiple hosts write to the same queue, but we should try to avoid
     * losing mail if we can.
     * 
     * If someone is racing against us, try to win.
     */
    for (;;) {
	vstring_sprintf(temp_path, "%s/%d.%d", queue_name,
			(int) tv.tv_usec, pid);
	if ((fd = open(STR(temp_path), O_RDWR | O_CREAT | O_EXCL, mode)) >= 0)
	    break;
	if (errno == EEXIST || errno == EISDIR) {
	    if ((int) ++tv.tv_usec < 0)
		tv.tv_usec = 0;
	    continue;
	}
	msg_warn("%s: create file %s: %m", myname, STR(temp_path));
	sleep(10);
    }

    /*
     * Rename the file to something that is derived from the file ID. I saw
     * this idea first being used in Zmailer. On any reasonable file system
     * the file ID is guaranteed to be unique. Better let the OS resolve
     * collisions than doing a worse job in an application. Another
     * attractive property of file IDs is that they can appear in messages
     * without leaking a significant amount of system information (unlike
     * process ids). Not so nice is that files need to be renamed when they
     * are moved to another file system.
     * 
     * If someone is racing against us, try to win.
     */
    file_id = get_file_id(fd);
    GETTIMEOFDAY(&tv);

    /*
     * XXX Some systems seem to have clocks that correlate with process
     * scheduling or something. Unfortunately, we cannot add random
     * quantities to the time, because the non-inode part of a queue ID must
     * not repeat within the same second. The queue ID is the sole thing that
     * prevents multiple messages from getting the same Message-ID value.
     */
    for (count = 0;; count++) {
	vstring_sprintf(id_buf, "%05X%s", (int) tv.tv_usec, file_id);
	mail_queue_path(path_buf, queue_name, STR(id_buf));
#if 0
	if (access(STR(path_buf), X_OK) == 0) {	/* collision. */
	    if ((int) ++tv.tv_usec < 0)
		tv.tv_usec = 0;
	    continue;
	}
#endif
	if (sane_rename(STR(temp_path), STR(path_buf)) == 0)	/* success */
	    break;
	if (errno == EPERM || errno == EISDIR) {/* collision. weird. */
	    if ((int) ++tv.tv_usec < 0)
		tv.tv_usec = 0;
	    continue;
	}
	if (errno != ENOENT || mail_queue_mkdirs(STR(path_buf)) < 0) {
	    msg_warn("%s: rename %s to %s: %m", myname,
		     STR(temp_path), STR(path_buf));
	}
	if (count > 1000)			/* XXX whatever */
	    msg_fatal("%s: rename %s to %s: giving up", myname,
		      STR(temp_path), STR(path_buf));
    }

    stream = vstream_fdopen(fd, O_RDWR);
    vstream_control(stream, VSTREAM_CTL_PATH, STR(path_buf), VSTREAM_CTL_END);
    return (stream);
}

/* mail_queue_open - open mail queue file */

VSTREAM *mail_queue_open(const char *queue_name, const char *queue_id,
			         int flags, int mode)
{
    const char *path = mail_queue_path((VSTRING *) 0, queue_name, queue_id);
    VSTREAM *fp;

    /*
     * Try the operation. If file creation fails, see if it is because of a
     * missing subdirectory.
     */
    if ((fp = vstream_fopen(path, flags, mode)) == 0)
	if (errno == ENOENT)
	    if ((flags & O_CREAT) == O_CREAT && mail_queue_mkdirs(path) == 0)
		fp = vstream_fopen(path, flags, mode);
    return (fp);
}
