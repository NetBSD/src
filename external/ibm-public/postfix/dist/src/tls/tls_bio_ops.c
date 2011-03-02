/*	$NetBSD: tls_bio_ops.c,v 1.1.1.3 2011/03/02 19:32:27 tron Exp $	*/

/*++
/* NAME
/*	tls_bio_ops 3
/* SUMMARY
/*	TLS network basic I/O management
/* SYNOPSIS
/*	#define TLS_INTERNAL
/*	#include <tls.h>
/*
/*	int tls_bio_connect(fd, timeout, context)
/*	int	fd;
/*	int	timeout;
/*	TLS_SESS_STATE *context;
/*
/*	int tls_bio_accept(fd, timeout, context)
/*	int	fd;
/*	int	timeout;
/*	TLS_SESS_STATE *context;
/*
/*	int tls_bio_shutdown(fd, timeout, context)
/*	int	fd;
/*	int	timeout;
/*	TLS_SESS_STATE *context;
/*
/*	int tls_bio_read(fd, buf, len, timeout, context)
/*	int	fd;
/*	void	*buf;
/*	int	len;
/*	int	timeout;
/*	TLS_SESS_STATE *context;
/*
/*	int tls_bio_write(fd, buf, len, timeout, context)
/*	int	fd;
/*	void	*buf;
/*	int	len;
/*	int	timeout;
/*	TLS_SESS_STATE *context;
/* DESCRIPTION
/*	This module enforces timeouts on non-blocking I/O while
/*	performing TLS handshake or input/output operations.
/*
/*	The Postfix VSTREAM read/write routines invoke the
/*	tls_bio_read/write routines to send and receive plain-text
/*	data.  In addition, this module provides tls_bio_connect/accept
/*	routines that trigger the initial TLS handshake.  The
/*	tls_bio_xxx routines invoke the corresponding SSL routines
/*	that translate the requests into TLS protocol messages.
/*
/*	Whenever an SSL operation indicates that network input (or
/*	output) needs to happen, the tls_bio_xxx routines wait for
/*	the network to become readable (or writable) within the
/*	timeout limit, then retry the SSL operation. This works
/*	because the network socket is in non-blocking mode.
/*
/*	tls_bio_connect() performs the SSL_connect() operation.
/*
/*	tls_bio_accept() performs the SSL_accept() operation.
/*
/*	tls_bio_shutdown() performs the SSL_shutdown() operation.
/*
/*	tls_bio_read() performs the SSL_read() operation.
/*
/*	tls_bio_write() performs the SSL_write() operation.
/*
/*	Arguments:
/* .IP fd
/*	Network socket.
/* .IP buf
/*	Read/write buffer.
/* .IP len
/*	Read/write request size.
/* .IP timeout
/*	Read/write timeout.
/* .IP TLScontext
/*	TLS session state.
/* DIAGNOSTICS
/*	The result value is -1 in case of a network read/write
/*	error, otherwise it is the result value of the TLS operation.
/* LICENSE
/* .ad
/* .fi
/*	This software is free. You can do with it whatever you want.
/*	The original author kindly requests that you acknowledge
/*	the use of his software.
/* AUTHOR(S)
/*	Originally written by:
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
/*
/*	Victor Duchovni
/*	Morgan Stanley
/*--*/

/* System library. */

#include <sys_defs.h>

#ifdef USE_TLS

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

/* TLS library. */

#define TLS_INTERNAL
#include <tls.h>

/* tls_bio - perform SSL input/output operation with extreme prejudice */

int     tls_bio(int fd, int timeout, TLS_SESS_STATE *TLScontext,
		        int (*hsfunc) (SSL *),
		        int (*rfunc) (SSL *, void *, int),
		        int (*wfunc) (SSL *, const void *, int),
		        void *buf, int num)
{
    const char *myname = "tls_bio";
    int     status;
    int     err;
    int     retval = 0;
    int     done;

    /*
     * If necessary, retry the SSL handshake or read/write operation after
     * handling any pending network I/O.
     */
    for (done = 0; done == 0; /* void */ ) {
	if (hsfunc)
	    status = hsfunc(TLScontext->con);
	else if (rfunc)
	    status = rfunc(TLScontext->con, buf, num);
	else if (wfunc)
	    status = wfunc(TLScontext->con, buf, num);
	else
	    msg_panic("%s: nothing to do here", myname);
	err = SSL_get_error(TLScontext->con, status);

#if (OPENSSL_VERSION_NUMBER <= 0x0090581fL)

	/*
	 * There is a bug up to and including OpenSSL-0.9.5a: if an error
	 * occurs while checking the peers certificate due to some
	 * certificate error (e.g. as happend with a RSA-padding error), the
	 * error is put onto the error stack. If verification is not
	 * enforced, this error should be ignored, but the error-queue is not
	 * cleared, so we can find this error here. The bug has been fixed on
	 * May 28, 2000.
	 * 
	 * This bug so far has only manifested as 4800:error:0407006A:rsa
	 * routines:RSA_padding_check_PKCS1_type_1:block type is not
	 * 01:rsa_pk1.c:100: 4800:error:04067072:rsa
	 * routines:RSA_EAY_PUBLIC_DECRYPT:padding check
	 * failed:rsa_eay.c:396: 4800:error:0D079006:asn1 encoding
	 * routines:ASN1_verify:bad get asn1 object call:a_verify.c:109: so
	 * that we specifically test for this error. We print the errors to
	 * the logfile and automatically clear the error queue. Then we retry
	 * to get another error code. We cannot do better, since we can only
	 * retrieve the last entry of the error-queue without actually
	 * cleaning it on the way.
	 * 
	 * This workaround is secure, as verify_result is set to "failed"
	 * anyway.
	 */
	if (err == SSL_ERROR_SSL) {
	    if (ERR_peek_error() == 0x0407006AL) {
		tls_print_errors();
		msg_info("OpenSSL <= 0.9.5a workaround called: certificate errors ignored");
		err = SSL_get_error(TLScontext->con, status);
	    }
	}
#endif

	/*
	 * Find out if we must retry the operation and/or if there is pending
	 * network I/O.
	 * 
	 * XXX If we're the first to invoke SSL_shutdown(), then the operation
	 * isn't really complete when the call returns. We could hide that
	 * anomaly here and repeat the call.
	 */
	switch (err) {
	case SSL_ERROR_NONE:			/* success */
	    retval = status;
	    done = 1;
	    break;
	case SSL_ERROR_WANT_WRITE:
	    if (write_wait(fd, timeout) < 0)
		return (-1);			/* timeout error */
	    break;
	case SSL_ERROR_WANT_READ:
	    if (read_wait(fd, timeout) < 0)
		return (-1);			/* timeout error */
	    break;

	    /*
	     * With tls_timed_read() and tls_timed_write() the caller is the
	     * VSTREAM library module which is unaware of TLS, so we log the
	     * TLS error stack here. In a better world, each VSTREAM I/O
	     * object would provide an error reporting method in addition to
	     * the timed_read and timed_write methods, so that we would not
	     * need to have ad-hoc code like this.
	     */
	case SSL_ERROR_SSL:
	    if (rfunc || wfunc)
		tls_print_errors();
	    /* FALLTHROUGH */
	default:
	    retval = status;
	    done = 1;
	    break;
	}
    }
    return (retval);
}

#endif
