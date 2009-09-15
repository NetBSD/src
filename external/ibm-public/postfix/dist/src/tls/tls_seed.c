/*	$NetBSD: tls_seed.c,v 1.1.1.1.2.2 2009/09/15 06:03:51 snj Exp $	*/

/*++
/* NAME
/*	tls_seed 3
/* SUMMARY
/*	TLS PRNG seeding routines
/* SYNOPSIS
/*	#define TLS_INTERNAL
/*	#include <tls.h>
/*
/*	int	tls_ext_seed(nbytes)
/*	int	nbytes;
/*
/*	void	tls_int_seed()
/* DESCRIPTION
/*	tls_ext_seed() requests the specified number of bytes
/*	from the tlsmgr(8) PRNG pool and updates the local PRNG.
/*	The result is zero in case of success, -1 otherwise.
/*
/*	tls_int_seed() mixes the process ID and time of day into
/*	the PRNG pool. This adds a few bits of entropy with each
/*	call, provided that the calls aren't made frequently.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this
/*	software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/time.h>			/* gettimeofday() */
#include <unistd.h>			/* getpid() */

#ifdef USE_TLS

/* OpenSSL library. */

#include <openssl/rand.h>		/* RAND_seed() */

/* Utility library. */

#include <msg.h>
#include <vstring.h>

/* TLS library. */

#include <tls_mgr.h>
#define TLS_INTERNAL
#include <tls.h>

/* Application-specific. */

/* tls_int_seed - add entropy to the pool by adding the time and PID */

void    tls_int_seed(void)
{
    static struct {
	pid_t   pid;
	struct timeval tv;
    }       randseed;

    if (randseed.pid == 0)
	randseed.pid = getpid();
    GETTIMEOFDAY(&randseed.tv);
    RAND_seed(&randseed, sizeof(randseed));
}

/* tls_ext_seed - request entropy from tlsmgr(8) server */

int     tls_ext_seed(int nbytes)
{
    VSTRING *buf;
    int     status;

    buf = vstring_alloc(nbytes);
    status = tls_mgr_seed(buf, nbytes);
    RAND_seed(vstring_str(buf), VSTRING_LEN(buf));
    vstring_free(buf);
    return (status == TLS_MGR_STAT_OK ? 0 : -1);
}

#endif
