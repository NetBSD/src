/*	$NetBSD: vstream_tweak.c,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

/*++
/* NAME
/*	vstream_tweak 3
/* SUMMARY
/*	performance tweaks
/* SYNOPSIS
/*	#include <vstream.h>
/*
/*	VSTREAM	*vstream_tweak_sock(stream)
/*	VSTREAM	*stream;
/*
/*	VSTREAM	*vstream_tweak_tcp(stream)
/*	VSTREAM	*stream;
/* DESCRIPTION
/*	vstream_tweak_sock() does a best effort to boost your
/*	network performance on the specified generic stream.
/*
/*	vstream_tweak_tcp() does a best effort to boost your
/*	Internet performance on the specified TCP stream.
/*
/*	Arguments:
/* .IP stream
/*	The stream being boosted.
/* DIAGNOSTICS
/*	Panics: interface violations.
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>

/* Application-specific. */

#ifdef HAS_IPV6
#define SOCKADDR_STORAGE struct sockaddr_storage
#else
#define SOCKADDR_STORAGE struct sockaddr
#endif

/* vstream_tweak_sock - boost your generic network performance */

int     vstream_tweak_sock(VSTREAM *fp)
{
    SOCKADDR_STORAGE ss;
    struct sockaddr *sa = (struct sockaddr *) &ss;
    SOCKADDR_SIZE sa_length = sizeof(ss);
    int     ret;

    /*
     * If the caller doesn't know if this socket is AF_LOCAL, AF_INET, etc.,
     * figure it out for them.
     */
    if ((ret = getsockname(vstream_fileno(fp), sa, &sa_length)) >= 0) {
	switch (sa->sa_family) {
#ifdef AF_INET6
	case AF_INET6:
#endif
	case AF_INET:
	    ret = vstream_tweak_tcp(fp);
	    break;
	}
    }
    return (ret);
}

/* vstream_tweak_tcp - boost your TCP performance */

int     vstream_tweak_tcp(VSTREAM *fp)
{
    const char *myname = "vstream_tweak_tcp";
    int     mss = 0;
    SOCKOPT_SIZE mss_len = sizeof(mss);
    int     err;

    /*
     * Avoid Nagle delays when VSTREAM buffers are smaller than the MSS.
     * 
     * Forcing TCP_NODELAY to be "always on" would hurt performance in the
     * common case where VSTREAM buffers are larger than the MSS.
     * 
     * Instead we ask the kernel what the current MSS is, and take appropriate
     * action. Linux <= 2.2 getsockopt(TCP_MAXSEG) always returns zero (or
     * whatever value was stored last with setsockopt()).
     * 
     * Some ancient FreeBSD kernels don't report 'host unreachable' errors with
     * getsockopt(SO_ERROR), and then treat getsockopt(TCP_MAXSEG) as a NOOP,
     * leaving the mss parameter value unchanged. To work around these two
     * getsockopt() bugs we set mss = 0, which is a harmless value.
     */
    if ((err = getsockopt(vstream_fileno(fp), IPPROTO_TCP, TCP_MAXSEG,
			  (void *) &mss, &mss_len)) < 0
	&& errno != ECONNRESET) {
	msg_warn("%s: getsockopt TCP_MAXSEG: %m", myname);
	return (err);
    }
    if (msg_verbose)
	msg_info("%s: TCP_MAXSEG %d", myname, mss);

    /*
     * Fix for recent Postfix versions: increase the VSTREAM buffer size if
     * it is smaller than the MSS. Note: the MSS may change when the route
     * changes and IP path MTU discovery is turned on, so we choose a
     * somewhat larger buffer.
     * 
     * Note: as of 20120527, the CA_VSTREAM_CTL_BUFSIZE request can reduce the
     * stream buffer size to less than VSTREAM_BUFSIZE, when the request is
     * made before the first stream read or write operation. We don't want to
     * reduce the buffer size.
     */
#define EFF_BUFFER_SIZE(fp) (vstream_req_bufsize(fp) ? \
		vstream_req_bufsize(fp) : VSTREAM_BUFSIZE)

#ifdef CA_VSTREAM_CTL_BUFSIZE
    if (mss > EFF_BUFFER_SIZE(fp) / 2) {
	if (mss < INT_MAX / 2)
	    mss *= 2;
	vstream_control(fp,
			CA_VSTREAM_CTL_BUFSIZE(mss),
			CA_VSTREAM_CTL_END);
    }

    /*
     * Workaround for older Postfix versions: turn on TCP_NODELAY if the
     * VSTREAM buffer size is smaller than the MSS.
     */
#else
    if (mss > VSTREAM_BUFSIZE) {
	int     nodelay = 1;

	if ((err = setsockopt(vstream_fileno(fp), IPPROTO_TCP, TCP_NODELAY,
			      (void *) &nodelay, sizeof(nodelay))) < 0
	    && errno != ECONNRESET)
	    msg_warn("%s: setsockopt TCP_NODELAY: %m", myname);
    }
#endif
    return (err);
}
