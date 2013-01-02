/*	$NetBSD: file_id.c,v 1.1.1.2 2013/01/02 18:58:58 tron Exp $	*/

/*++
/* NAME
/*	file_id 3
/* SUMMARY
/*	file ID printable representation
/* SYNOPSIS
/*	#include <file_id.h>
/*
/*	const char *get_file_id_fd(fd, long_flag)
/*	int	fd;
/*	int	long_flag;
/*
/*	const char *get_file_id_st(st, long_flag)
/*	struct stat *st;
/*	int	long_flag;
/*
/*	const char *get_file_id(fd)
/*	int	fd;
/* DESCRIPTION
/*	get_file_id_fd() queries the operating system for the unique
/*	file identifier for the specified file descriptor and returns
/*	a printable representation.  The result is volatile.  Make
/*	a copy if it is to be used for any appreciable amount of
/*	time.
/*
/*	get_file_id_st() returns the unique identifier for the
/*	specified file status information.
/*
/*	get_file_id() provides binary compatibility for old programs.
/*	This function should not be used by new programs.
/*
/*	Arguments:
/* .IP fd
/*	A valid file descriptor that is associated with an open file.
/* .IP st
/*	The result from e.g., stat(2) or fstat(2).
/* .IP long_flag
/*	Encode the result as appropriate for long or short queue
/*	identifiers.
/* DIAGNOSTICS
/*	All errors are fatal.
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
#include <string.h>

/* Utility library */

#include <msg.h>
#include <vstring.h>
#include <warn_stat.h>

/* Global library. */

#define MAIL_QUEUE_INTERNAL
#include <mail_queue.h>
#include "file_id.h"

/* get_file_id - binary compatibility */

const char *get_file_id(int fd)
{
    return (get_file_id_fd(fd, 0));
}

/* get_file_id_fd - return printable file identifier for file descriptor */

const char *get_file_id_fd(int fd, int long_flag)
{
    struct stat st;

    if (fstat(fd, &st) < 0)
	msg_fatal("fstat: %m");
    return (get_file_id_st(&st, long_flag));
}

/* get_file_id_st - return printable file identifier for file status */

const char *get_file_id_st(struct stat * st, int long_flag)
{
    static VSTRING *result;

    if (result == 0)
	result = vstring_alloc(1);
    if (long_flag)
	return (MQID_LG_ENCODE_INUM(result, st->st_ino));
    else
	return (MQID_SH_ENCODE_INUM(result, st->st_ino));
}
