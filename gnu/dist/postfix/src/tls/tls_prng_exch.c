/*++
/* NAME
/*	tls_prng_exch 3
/* SUMMARY
/*	maintain PRNG exchange file
/* SYNOPSIS
/*	#include <tls_prng_src.h>
/*
/*	TLS_PRNG_SRC *tls_prng_exch_open(name, timeout)
/*	const char *name;
/*	int	timeout;
/*
/*	void	tls_prng_exch_update(fh, length)
/*	TLS_PRNG_SRC *fh;
/*	size_t length;
/*
/*	void	tls_prng_exch_close(fh)
/*	TLS_PRNG_SRC *fh;
/* DESCRIPTION
/*	tls_prng_exch_open() opens the specified PRNG exchange file
/*	and returns a handle that should be used with all subsequent
/*	access.
/*
/*	tls_prng_exch_update() reads the requested number of bytes
/*	from the PRNG exchange file, updates the OpenSSL PRNG, and
/*	writes the requested number of bytes to the exchange file.
/*	The file is locked for exclusive access.
/*
/*	tls_prng_exch_close() closes the specified PRNG exchange
/*	file and releases memory that was allocated for the handle.
/*
/*	Arguments:
/* .IP name
/*	The name of the PRNG exchange file.
/* .IP length
/*	The number of bytes to read from/write to the entropy file.
/* .IP timeout
/*	Time limit on individual I/O operations.
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
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

/* OpenSSL library. */

#ifdef USE_TLS
#include <openssl/rand.h>		/* For the PRNG */

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <iostuff.h>
#include <myflock.h>

/* TLS library. */

#include <tls_prng.h>

/* Application specific. */

#define TLS_PRNG_EXCH_SIZE	1024	/* XXX Why not configurable? */

/* tls_prng_exch_open - open PRNG exchange file */

TLS_PRNG_SRC *tls_prng_exch_open(const char *name)
{
    const char *myname = "tls_prng_exch_open";
    TLS_PRNG_SRC *eh;
    int     fd;

    if ((fd = open(name, O_RDWR | O_CREAT, 0600)) < 0)
	msg_fatal("%s: cannot open PRNG exchange file %s: %m", myname, name);
    eh = (TLS_PRNG_SRC *) mymalloc(sizeof(*eh));
    eh->fd = fd;
    eh->name = mystrdup(name);
    eh->timeout = 0;
    if (msg_verbose)
	msg_info("%s: opened PRNG exchange file %s", myname, name);
    return (eh);
}

/* tls_prng_exch_update - update PRNG exchange file */

void    tls_prng_exch_update(TLS_PRNG_SRC *eh)
{
    unsigned char buffer[TLS_PRNG_EXCH_SIZE];
    ssize_t count;

    /*
     * Update the PRNG exchange file. Since other processes may have added
     * entropy, we use a read-stir-write cycle.
     */
    if (myflock(eh->fd, INTERNAL_LOCK, MYFLOCK_OP_EXCLUSIVE) != 0)
	msg_fatal("cannot lock PRNG exchange file %s: %m", eh->name);
    if (lseek(eh->fd, 0, SEEK_SET) < 0)
	msg_fatal("cannot seek PRNG exchange file %s: %m", eh->name);
    if ((count = read(eh->fd, buffer, sizeof(buffer))) < 0)
	msg_fatal("cannot read PRNG exchange file %s: %m", eh->name);

    if (count > 0)
	RAND_seed(buffer, count);
    RAND_bytes(buffer, sizeof(buffer));

    if (lseek(eh->fd, 0, SEEK_SET) < 0)
	msg_fatal("cannot seek PRNG exchange file %s: %m", eh->name);
    if (write(eh->fd, buffer, sizeof(buffer)) != sizeof(buffer))
	msg_fatal("cannot write PRNG exchange file %s: %m", eh->name);
    if (myflock(eh->fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) != 0)
	msg_fatal("cannot unlock PRNG exchange file %s: %m", eh->name);
}

/* tls_prng_exch_close - close PRNG exchange file */

void    tls_prng_exch_close(TLS_PRNG_SRC *eh)
{
    const char *myname = "tls_prng_exch_close";

    if (close(eh->fd) < 0)
	msg_fatal("close PRNG exchange file %s: %m", eh->name);
    if (msg_verbose)
	msg_info("%s: closed PRNG exchange file %s", myname, eh->name);
    myfree(eh->name);
    myfree((char *) eh);
}

#endif
