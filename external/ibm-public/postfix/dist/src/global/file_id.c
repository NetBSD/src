/*	$NetBSD: file_id.c,v 1.1.1.1.2.2 2009/09/15 06:02:43 snj Exp $	*/

/*++
/* NAME
/*	file_id 3
/* SUMMARY
/*	file ID printable representation
/* SYNOPSIS
/*	#include <file_id.h>
/*
/*	const char *get_file_id(fd)
/*	int	fd;
/*
/*	int	check_file_id(fd, id)
/*	int	fd;
/*	const char *id;
/* DESCRIPTION
/*	get_file_id() queries the operating system for the unique identifier
/*	for the specified file and returns a printable representation.
/*	The result is volatile.  Make a copy if it is to be used for any
/*	appreciable amount of time.
/*
/*	check_file_id() tests if an open file matches the given
/*	printable FILE ID representation.
/*
/*	Arguments:
/* .IP fd
/*	A valid file descriptor that is associated with an open file.
/* .IP id
/*	Printable file ID.
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

/* Global library. */

#include "file_id.h"

/* get_file_id - lookup file ID, convert to printable form */

const char *get_file_id(int fd)
{
    static VSTRING *result;
    struct stat st;

    if (result == 0)
	result = vstring_alloc(1);
    if (fstat(fd, &st) < 0)
	msg_fatal("fstat: %m");
    vstring_sprintf(result, "%lX", (long) st.st_ino);
    return (vstring_str(result));
}

/* check_file_id - make sure file name matches ID */

int     check_file_id(int fd, const char *name)
{
    return (strcmp(get_file_id(fd), name));
}
