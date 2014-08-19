/*	$NetBSD: tls_proxy_clnt.c,v 1.1.1.1.12.1 2014/08/19 23:59:45 tls Exp $	*/

/*++
/* NAME
/*	tlsproxy_clnt 3
/* SUMMARY
/*	postscreen TLS proxy support
/* SYNOPSIS
/*	#include <tlsproxy_clnt.h>
/*
/*	VSTREAM *tls_proxy_open(service, flags, peer_stream, peer_addr,
/*			          peer_port, timeout)
/*	const char *service;
/*	int	flags;
/*	VSTREAM *peer_stream;
/*	const char *peer_addr;
/*	const char *peer_port;
/*	int	timeout;
/*
/*	TLS_SESS_STATE *tls_proxy_context_receive(proxy_stream)
/*	VSTREAM *proxy_stream;
/*
/*	void	tls_proxy_context_free(tls_context)
/*	TLS_SESS_STATE *tls_context;
/* DESCRIPTION
/*	tls_proxy_open() prepares for inserting the tlsproxy(8)
/*	daemon between the current process and a remote peer (the
/*	actual insert operation is described in the next paragraph).
/*	The result value is a null pointer on failure. The peer_stream
/*	is not closed.  The resulting proxy stream is single-buffered.
/*
/*	After this, it is a good idea to use the VSTREAM_CTL_SWAP_FD
/*	request to swap the file descriptors between the plaintext
/*	peer_stream and the proxy stream from tls_proxy_open().
/*	This avoids the loss of application-configurable VSTREAM
/*	attributes on the plaintext peer_stream (such as longjmp
/*	buffer, timeout, etc.). Once the file descriptors are
/*	swapped, the proxy stream should be closed.
/*
/*	tls_proxy_context_receive() receives the TLS context object
/*	for the named proxy stream. This function must be called
/*	only if the TLS_PROXY_SEND_CONTEXT flag was specified in
/*	the tls_proxy_open() call. Note that this TLS context object
/*	is not compatible with tls_session_free(). It must be given
/*	to tls_proxy_context_free() instead.
/*
/*	After this, the proxy_stream is ready for plain-text I/O.
/*
/*	tls_proxy_context_free() destroys a TLS context object that
/*	was received with tls_proxy_context_receive().
/*
/*	Arguments:
/* .IP service
/*	The (base) name of the tlsproxy service.
/* .IP flags
/*	Bit-wise OR of:
/* .RS
/* .IP TLS_PROXY_FLAG_ROLE_SERVER
/*	Request the TLS server proxy role.
/* .IP TLS_PROXY_FLAG_ROLE_CLIENT
/*	Request the TLS client proxy role.
/* .IP TLS_PROXY_FLAG_SEND_CONTEXT
/*	Send the TLS context object.
/* .RE
/* .IP peer_stream
/*	Stream that connects the current process to a remote peer.
/* .IP peer_addr
/*	Printable IP address of the remote peer_stream endpoint.
/* .IP peer_port
/*	Printable TCP port of the remote peer_stream endpoint.
/* .IP timeout
/*	Time limit that the tlsproxy(8) daemon should use.
/* .IP proxy_stream
/*	Stream from tls_proxy_open().
/* .IP tls_context
/*	TLS session object from tls_proxy_context_receive().
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

#ifdef USE_TLS

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <connect.h>
#include <stringops.h>
#include <vstring.h>

/* Global library. */

#include <mail_proto.h>
#include <mail_params.h>

/* TLS library-specific. */

#include <tls.h>
#include <tls_proxy.h>

#define TLSPROXY_INIT_TIMEOUT		10

/* SLMs. */

#define STR	vstring_str

/* tls_proxy_open - open negotiations with TLS proxy */

VSTREAM *tls_proxy_open(const char *service, int flags,
			        VSTREAM *peer_stream,
			        const char *peer_addr,
			        const char *peer_port,
			        int timeout)
{
    VSTREAM *tlsproxy_stream;
    int     status;
    int     fd;
    static VSTRING *tlsproxy_service = 0;
    static VSTRING *remote_endpt = 0;

    /*
     * Initialize.
     */
    if (tlsproxy_service == 0) {
	tlsproxy_service = vstring_alloc(20);
	remote_endpt = vstring_alloc(20);
    }

    /*
     * Connect to the tlsproxy(8) daemon.
     */
    vstring_sprintf(tlsproxy_service, "%s/%s", MAIL_CLASS_PRIVATE, service);
    if ((fd = LOCAL_CONNECT(STR(tlsproxy_service), BLOCKING,
			    TLSPROXY_INIT_TIMEOUT)) < 0) {
	msg_warn("connect to %s service: %m", STR(tlsproxy_service));
	return (0);
    }

    /*
     * Initial handshake. Send the data attributes now, and send the client
     * file descriptor in a later transaction.
     * 
     * XXX The formatted endpoint should be a state member. Then, we can
     * simplify all the format strings throughout the program.
     */
    tlsproxy_stream = vstream_fdopen(fd, O_RDWR);
    vstring_sprintf(remote_endpt, "[%s]:%s", peer_addr, peer_port);
    attr_print(tlsproxy_stream, ATTR_FLAG_NONE,
	       ATTR_TYPE_STR, MAIL_ATTR_REMOTE_ENDPT, STR(remote_endpt),
	       ATTR_TYPE_INT, MAIL_ATTR_FLAGS, flags,
	       ATTR_TYPE_INT, MAIL_ATTR_TIMEOUT, timeout,
	       ATTR_TYPE_END);
    if (vstream_fflush(tlsproxy_stream) != 0) {
	msg_warn("error sending request to %s service: %m",
		 STR(tlsproxy_service));
	vstream_fclose(tlsproxy_stream);
	return (0);
    }

    /*
     * Receive the "TLS is available" indication.
     * 
     * This may seem out of order, but we must have a read transaction between
     * sending the request attributes and sending the SMTP client file
     * descriptor. We can't assume UNIX-domain socket semantics here.
     */
    if (attr_scan(tlsproxy_stream, ATTR_FLAG_STRICT,
		  ATTR_TYPE_INT, MAIL_ATTR_STATUS, &status,
		  ATTR_TYPE_END) != 1 || status == 0) {

	/*
	 * The TLS proxy reports that the TLS engine is not available (due to
	 * configuration error, or other causes).
	 */
	msg_warn("%s service role \"%s\" is not available",
		 STR(tlsproxy_service),
		 (flags & TLS_PROXY_FLAG_ROLE_SERVER) ? "server" :
		 (flags & TLS_PROXY_FLAG_ROLE_CLIENT) ? "client" :
		 "bogus role");
	vstream_fclose(tlsproxy_stream);
	return (0);
    }

    /*
     * Send the remote SMTP client file descriptor.
     */
    if (LOCAL_SEND_FD(vstream_fileno(tlsproxy_stream),
		      vstream_fileno(peer_stream)) < 0) {

	/*
	 * Some error: drop the TLS proxy stream.
	 */
	msg_warn("sending file handle to %s service: %m",
		 STR(tlsproxy_service));
	vstream_fclose(tlsproxy_stream);
	return (0);
    }
    return (tlsproxy_stream);
}

/* tls_proxy_context_receive - receive TLS session object from tlsproxy(8) */

TLS_SESS_STATE *tls_proxy_context_receive(VSTREAM *proxy_stream)
{
    TLS_SESS_STATE *tls_context;

    tls_context = (TLS_SESS_STATE *) mymalloc(sizeof(*tls_context));

    if (attr_scan(proxy_stream, ATTR_FLAG_STRICT,
	       ATTR_TYPE_FUNC, tls_proxy_context_scan, (char *) tls_context,
		  ATTR_TYPE_END) != 1) {
	tls_proxy_context_free(tls_context);
	return (0);
    } else {
	return (tls_context);
    }
}

/* tls_proxy_context_free - destroy object from tls_proxy_context_receive() */

void    tls_proxy_context_free(TLS_SESS_STATE *tls_context)
{
    if (tls_context->peer_CN)
	myfree(tls_context->peer_CN);
    if (tls_context->issuer_CN)
	myfree(tls_context->issuer_CN);
    if (tls_context->peer_cert_fprint)
	myfree(tls_context->peer_cert_fprint);
    if (tls_context->protocol)
	myfree((char *) tls_context->protocol);
    if (tls_context->cipher_name)
	myfree((char *) tls_context->cipher_name);
    myfree((char *) tls_context);
}

#endif
