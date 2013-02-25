/*	$NetBSD: mail_open_ok.c,v 1.1.1.1.16.1 2013/02/25 00:27:18 tls Exp $	*/

/*++
/* NAME
/*	mail_open_ok 3
/* SUMMARY
/*	scrutinize mail queue file
/* SYNOPSIS
/*	#include <mail_open_ok.h>
/*
/*	int	mail_open_ok(queue_name, queue_id, statp, pathp)
/*	const char *queue_name;
/*	const char *queue_id;
/*	struct stat *statp;
/*	char	**pathp
/* DESCRIPTION
/*	mail_open_ok() determines if it is OK to open the specified
/*	queue file.
/*
/*	The queue name and queue id should conform to the syntax
/*	requirements for these names.
/*	Unfortunately, on some systems readdir() etc. can return bogus
/*	file names. For this reason, the code silently ignores invalid
/*	queue file names.
/*
/*	The file should have mode 0700. Files with other permissions
/*	are silently ignored.
/*
/*	The file should be a regular file.
/*	Files that do not satisfy this requirement are skipped with
/*	a warning.
/*
/*	The file link count is not restricted. With a writable maildrop
/*	directory, refusal to deliver linked files is prone to denial of
/*	service attack; it's better to deliver mail too often than not.
/*
/*	Upon return, the \fIstatp\fR argument receives the file
/*	attributes and \fIpathp\fR a copy of the file name. The file
/*	name is volatile. Make a copy if it is to be used for any
/*	appreciable amount of time.
/* DIAGNOSTICS
/*	Warnings: bad file attributes (file type), multiple hard links.
/*	mail_open_ok() returns MAIL_OPEN_YES for good files, MAIL_OPEN_NO
/*	for anything else. It is left up to the system administrator to
/*	deal with non-file objects.
/* BUGS
/*	mail_open_ok() examines a queue file without actually opening
/*	it, and therefore is susceptible to race conditions.
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

/* System libraries. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <warn_stat.h>

/* Global library. */

#include "mail_queue.h"
#include "mail_open_ok.h"

/* mail_open_ok - see if this file is OK to open */

int     mail_open_ok(const char *queue_name, const char *queue_id,
		             struct stat * statp, const char **path)
{
    if (mail_queue_name_ok(queue_name) == 0) {
	msg_warn("bad mail queue name: %s", queue_name);
	return (MAIL_OPEN_NO);
    }
    if (mail_queue_id_ok(queue_id) == 0)
	return (MAIL_OPEN_NO);


    /*
     * I really would like to look up the file attributes *after* opening the
     * file so that we could save one directory traversal on systems without
     * name-to-inode cache. However, we don't necessarily always want to open
     * the file.
     */
    *path = mail_queue_path((VSTRING *) 0, queue_name, queue_id);

    if (lstat(*path, statp) < 0) {
	if (errno != ENOENT)
	    msg_warn("%s: %m", *path);
	return (MAIL_OPEN_NO);
    }
    if (!S_ISREG(statp->st_mode)) {
	msg_warn("%s: uid %ld: not a regular file", *path, (long) statp->st_uid);
	return (MAIL_OPEN_NO);
    }
    if ((statp->st_mode & S_IRWXU) != MAIL_QUEUE_STAT_READY)
	return (MAIL_OPEN_NO);

    /*
     * Workaround for spurious "file has 2 links" warnings in showq. As
     * kernels have evolved from non-interruptible system calls towards
     * fine-grained locks, the showq command has become likely to observe a
     * file while the queue manager is in the middle of renaming it, at a
     * time when the file has links to both the old and new name. We now log
     * the warning only when the condition appears to be persistent.
     */
#define MINUTE_SECONDS	60			/* XXX should be centralized */

    if (statp->st_nlink > 1) {
	if (msg_verbose)
	    msg_info("%s: uid %ld: file has %d links", *path,
		     (long) statp->st_uid, (int) statp->st_nlink);
	else if (statp->st_ctime < time((time_t *) 0) - MINUTE_SECONDS)
	    msg_warn("%s: uid %ld: file has %d links", *path,
		     (long) statp->st_uid, (int) statp->st_nlink);
    }
    return (MAIL_OPEN_YES);
}
