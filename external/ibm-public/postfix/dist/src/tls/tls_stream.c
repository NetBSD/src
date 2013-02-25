/*	$NetBSD: tls_stream.c,v 1.1.1.1.16.1 2013/02/25 00:27:30 tls Exp $	*/

/*++
/* NAME
/*	tls_stream
/* SUMMARY
/*	VSTREAM over TLS
/* SYNOPSIS
/*	#define TLS_INTERNAL
/*	#include <tls.h>
/*
/*	void	tls_stream_start(stream, context)
/*	VSTREAM	*stream;
/*	TLS_SESS_STATE *context;
/*
/*	void	tls_stream_stop(stream)
/*	VSTREAM	*stream;
/* DESCRIPTION
/*	This module implements the VSTREAM over TLS support user interface.
/*	The hard work is done elsewhere.
/*
/*	tls_stream_start() enables TLS on the named stream. All read
/*	and write operations are directed through the TLS library,
/*	using the state information specified with the context argument.
/*
/*	tls_stream_stop() replaces the VSTREAM read/write routines
/*	by dummies that have no side effects, and deletes the
/*	VSTREAM's reference to the TLS context.
/* DIAGNOSTICS
/*	The tls_stream(3) read/write routines return the non-zero
/*	number of plaintext bytes read/written if successful; -1
/*	after TLS protocol failure, system-call failure, or for any
/*	reason described under "in addition" below; and zero when
/*	the remote party closed the connection or sent a TLS shutdown
/*	request.
/*
/*	Upon return from the tls_stream(3) read/write routines the
/*	global errno value is non-zero when the requested operation
/*	did not complete due to system call failure.
/*
/*	In addition, the result value is set to -1, and the global
/*	errno value is set to ETIMEDOUT, when a network read/write
/*	request did not complete within the time limit.
/* SEE ALSO
/*	dummy_read(3), placebo read routine
/*	dummy_write(3), placebo write routine
/* LICENSE
/* .ad
/* .fi
/*	This software is free. You can do with it whatever you want.
/*	The original author kindly requests that you acknowledge
/*	the use of his software.
/* AUTHOR(S)
/*	Based on code that was originally written by:
/*	Lutz Jaenicke
/*	BTU Cottbus
/*	Allgemeine Elektrotechnik
/*	Universitaetsplatz 3-4
/*	D-03044 Cottbus, Germany
/*
/*	Updated by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>

#ifdef USE_TLS

/* Utility library. */

#include <iostuff.h>
#include <vstream.h>
#include <msg.h>

/* TLS library. */

#define TLS_INTERNAL
#include <tls.h>

 /*
  * Interface mis-match compensation. The OpenSSL read/write routines return
  * unspecified negative values when an operation fails, while the vstream(3)
  * plaintext timed_read/write() functions follow the convention of UNIX
  * system calls, and return -1 upon error. The macro below makes OpenSSL
  * read/write results consistent with the UNIX system-call convention.
  */
#define NORMALIZED_VSTREAM_RETURN(retval) ((retval) < 0 ? -1 : (retval))

/* tls_timed_read - read content from stream, then TLS decapsulate */

static ssize_t tls_timed_read(int fd, void *buf, size_t len, int timeout,
			              void *context)
{
    const char *myname = "tls_timed_read";
    ssize_t ret;
    TLS_SESS_STATE *TLScontext;

    TLScontext = (TLS_SESS_STATE *) context;
    if (!TLScontext)
	msg_panic("%s: no context", myname);

    ret = tls_bio_read(fd, buf, len, timeout, TLScontext);
    if (ret > 0 && (TLScontext->log_mask & TLS_LOG_ALLPKTS))
	msg_info("Read %ld chars: %.*s",
		 (long) ret, (int) (ret > 40 ? 40 : ret), (char *) buf);
    return (NORMALIZED_VSTREAM_RETURN(ret));
}

/* tls_timed_write - TLS encapsulate content, then write to stream */

static ssize_t tls_timed_write(int fd, void *buf, size_t len, int timeout,
			               void *context)
{
    const char *myname = "tls_timed_write";
    ssize_t ret;
    TLS_SESS_STATE *TLScontext;

    TLScontext = (TLS_SESS_STATE *) context;
    if (!TLScontext)
	msg_panic("%s: no context", myname);

    if (TLScontext->log_mask & TLS_LOG_ALLPKTS)
	msg_info("Write %ld chars: %.*s",
		 (long) len, (int) (len > 40 ? 40 : len), (char *) buf);
    ret = tls_bio_write(fd, buf, len, timeout, TLScontext);
    return (NORMALIZED_VSTREAM_RETURN(ret));
}

/* tls_stream_start - start VSTREAM over TLS */

void    tls_stream_start(VSTREAM *stream, TLS_SESS_STATE *context)
{
    vstream_control(stream,
		    VSTREAM_CTL_READ_FN, tls_timed_read,
		    VSTREAM_CTL_WRITE_FN, tls_timed_write,
		    VSTREAM_CTL_CONTEXT, (void *) context,
		    VSTREAM_CTL_END);
}

/* tls_stream_stop - stop VSTREAM over TLS */

void    tls_stream_stop(VSTREAM *stream)
{

    /*
     * Prevent data leakage after TLS is turned off. The Postfix/TLS patch
     * provided null function pointers; we use dummy routines that make less
     * noise when used.
     */
    vstream_control(stream,
		    VSTREAM_CTL_READ_FN, dummy_read,
		    VSTREAM_CTL_WRITE_FN, dummy_write,
		    VSTREAM_CTL_CONTEXT, (void *) 0,
		    VSTREAM_CTL_END);
}

#endif
