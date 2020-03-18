/*	$NetBSD: logwriter.c,v 1.2 2020/03/18 19:05:21 christos Exp $	*/

/*++
/* NAME
/*	logwriter 3
/* SUMMARY
/*	logfile writer
/* SYNOPSIS
/*	#include <logwriter.h>
/*
/*	VSTREAM	*logwriter_open_or_die(
/*	const char *path)
/*
/*	int	logwriter_write(
/*	VSTREAM	*file,
/*	const char *buffer.
/*	ssize_t	buflen)
/*
/*	int	logwriter_close(
/*	VSTREAM	*file)
/*
/*	int	logwriter_one_shot(
/*	const char *path,
/*	const char *buffer,
/*	ssize_t	buflen)
/* DESCRIPTION
/*	This module manages a logfile writer.
/*
/*	logwriter_open_or_die() safely opens the specified file in
/*	write+append mode. File open/create errors are fatal.
/*
/*	logwriter_write() writes the buffer plus newline to the
/*	open logfile. The result is zero if successful, VSTREAM_EOF
/*	if the operation failed.
/*
/*	logwriter_close() closes the logfile and destroys the VSTREAM
/*	instance. The result is zero if there were no errors writing
/*	the file, VSTREAM_EOF otherwise.
/*
/*	logwriter_one_shot() combines all the above operations. The
/*	result is zero if successful, VSTREAM_EOF if any operation
/*	failed.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

 /*
  * Utility library.
  */
#include <iostuff.h>
#include <logwriter.h>
#include <msg.h>
#include <mymalloc.h>
#include <safe_open.h>
#include <vstream.h>

 /*
  * Application-specific.
  */

/* logwriter_open_or_die - open logfile */

VSTREAM *logwriter_open_or_die(const char *path)
{
    VSTREAM *fp;
    VSTRING *why = vstring_alloc(100);

#define NO_STATP	((struct stat *) 0)
#define NO_CHOWN	(-1)
#define NO_CHGRP	(-1)

    fp = safe_open(path, O_CREAT | O_WRONLY | O_APPEND, 0644,
		   NO_STATP, NO_CHOWN, NO_CHGRP, why);
    if (fp == 0)
	msg_fatal("open logfile '%s': %s", path, vstring_str(why));
    close_on_exec(vstream_fileno(fp), CLOSE_ON_EXEC);
    vstring_free(why);
    return (fp);
}

/* logwriter_write - append to logfile */

int     logwriter_write(VSTREAM *fp, const char *buf, ssize_t len)
{
    if (len < 0)
	msg_panic("logwriter_write: negative length %ld", (long) len);
    if (vstream_fwrite(fp, buf, len) != len)
	return (VSTREAM_EOF);
    VSTREAM_PUTC('\n', fp);
    return (vstream_fflush(fp));
}

/* logwriter_close - close logfile */

int     logwriter_close(VSTREAM *fp)
{
    return (vstream_fclose(fp));
}

/* logwriter_one_shot - one-shot logwriter */

int     logwriter_one_shot(const char *path, const char *buf, ssize_t len)
{
    VSTREAM *fp;
    int     err;

    fp = logwriter_open_or_die(path);
    err = logwriter_write(fp, buf, len);
    err |= logwriter_close(fp);
    return (err ? VSTREAM_EOF : 0);
}
