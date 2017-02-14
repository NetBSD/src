/*	$NetBSD: tls_prng_file.c,v 1.2 2017/02/14 01:16:48 christos Exp $	*/

/*++
/* NAME
/*	tls_prng_file 3
/* SUMMARY
/*	seed OpenSSL PRNG from entropy file
/* SYNOPSIS
/*	#include <tls_prng_src.h>
/*
/*	TLS_PRNG_SRC *tls_prng_file_open(name, timeout)
/*	const char *name;
/*	int	timeout;
/*
/*	ssize_t tls_prng_file_read(fh, length)
/*	TLS_PRNG_SRC *fh;
/*	size_t length;
/*
/*	int	tls_prng_file_close(fh)
/*	TLS_PRNG_SRC *fh;
/* DESCRIPTION
/*	tls_prng_file_open() open the specified file and returns
/*	a handle that should be used with all subsequent access.
/*
/*	tls_prng_file_read() reads the requested number of bytes from
/*	the entropy file and updates the OpenSSL PRNG. The file is not
/*	locked for shared or exclusive access.
/*
/*	tls_prng_file_close() closes the specified entropy file
/*	and releases memory that was allocated for the handle.
/*
/*	Arguments:
/* .IP name
/*	The pathname of the entropy file.
/* .IP length
/*	The number of bytes to read from the entropy file.
/* .IP timeout
/*	Time limit on individual I/O operations.
/* DIAGNOSTICS
/*	tls_prng_file_open() returns a null pointer on error.
/*
/*	tls_prng_file_read() returns -1 on error, the number
/*	of bytes received on success.
/*
/*	tls_prng_file_close() returns -1 on error, 0 on success.
/*
/*	In all cases the errno variable indicates the type of error.
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
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

/* OpenSSL library. */

#ifdef USE_TLS
#include <openssl/rand.h>		/* For the PRNG */

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <connect.h>
#include <iostuff.h>

/* TLS library. */

#include <tls_prng.h>

/* tls_prng_file_open - open entropy file */

TLS_PRNG_SRC *tls_prng_file_open(const char *name, int timeout)
{
    const char *myname = "tls_prng_file_open";
    TLS_PRNG_SRC *fh;
    int     fd;

    if ((fd = open(name, O_RDONLY, 0)) < 0) {
	if (msg_verbose)
	    msg_info("%s: cannot open entropy file %s: %m", myname, name);
	return (0);
    } else {
	fh = (TLS_PRNG_SRC *) mymalloc(sizeof(*fh));
	fh->fd = fd;
	fh->name = mystrdup(name);
	fh->timeout = timeout;
	if (msg_verbose)
	    msg_info("%s: opened entropy file %s", myname, name);
	return (fh);
    }
}

/* tls_prng_file_read - update internal PRNG from entropy file */

ssize_t tls_prng_file_read(TLS_PRNG_SRC *fh, size_t len)
{
    const char *myname = "tls_prng_file_read";
    char    buffer[8192];
    ssize_t to_read;
    ssize_t count;

    if (msg_verbose)
	msg_info("%s: seed internal pool from file %s", myname, fh->name);

    if (lseek(fh->fd, 0, SEEK_SET) < 0) {
	if (msg_verbose)
	    msg_info("cannot seek entropy file %s: %m", fh->name);
	return (-1);
    }
    errno = 0;
    for (to_read = len; to_read > 0; to_read -= count) {
	if ((count = timed_read(fh->fd, buffer, to_read > sizeof(buffer) ?
				sizeof(buffer) : to_read,
				fh->timeout, (void *) 0)) < 0) {
	    if (msg_verbose)
		msg_info("cannot read entropy file %s: %m", fh->name);
	    return (-1);
	}
	if (count == 0)
	    break;
	RAND_seed(buffer, count);
    }
    if (msg_verbose)
	msg_info("read %ld bytes from entropy file %s: %m",
		 (long) (len - to_read), fh->name);
    return (len - to_read);
}

/* tls_prng_file_close - close entropy file */

int     tls_prng_file_close(TLS_PRNG_SRC *fh)
{
    const char *myname = "tls_prng_file_close";
    int     err;

    if (msg_verbose)
	msg_info("%s: close entropy file %s", myname, fh->name);
    err = close(fh->fd);
    myfree(fh->name);
    myfree((void *) fh);
    return (err);
}

#endif
