/*	$NetBSD: net.c,v 1.8 2015/07/08 17:29:00 christos Exp $	*/

/*
 * Copyright (C) 2004, 2005, 2007, 2008, 2012-2015  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2003  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id */

#include <config.h>

#include <sys/types.h>

#if defined(HAVE_SYS_SYSCTL_H)
#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif
#include <sys/sysctl.h>
#endif
#include <sys/uio.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <isc/log.h>
#include <isc/msgs.h>
#include <isc/net.h>
#include <isc/netdb.h>
#include <isc/once.h>
#include <isc/strerror.h>
#include <isc/string.h>
#include <isc/util.h>

#ifndef ISC_SOCKADDR_LEN_T
#define ISC_SOCKADDR_LEN_T unsigned int
#endif

/*%
 * Definitions about UDP port range specification.  This is a total mess of
 * portability variants: some use sysctl (but the sysctl names vary), some use
 * system-specific interfaces, some have the same interface for IPv4 and IPv6,
 * some separate them, etc...
 */

/*%
 * The last resort defaults: use all non well known port space
 */
#ifndef ISC_NET_PORTRANGELOW
#define ISC_NET_PORTRANGELOW 1024
#endif	/* ISC_NET_PORTRANGELOW */
#ifndef ISC_NET_PORTRANGEHIGH
#define ISC_NET_PORTRANGEHIGH 65535
#endif	/* ISC_NET_PORTRANGEHIGH */

#ifdef HAVE_SYSCTLBYNAME

/*%
 * sysctl variants
 */
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__DragonFly__)
#define USE_SYSCTL_PORTRANGE
#define SYSCTL_V4PORTRANGE_LOW	"net.inet.ip.portrange.hifirst"
#define SYSCTL_V4PORTRANGE_HIGH	"net.inet.ip.portrange.hilast"
#define SYSCTL_V6PORTRANGE_LOW	"net.inet.ip.portrange.hifirst"
#define SYSCTL_V6PORTRANGE_HIGH	"net.inet.ip.portrange.hilast"
#endif

#ifdef __NetBSD__
#define USE_SYSCTL_PORTRANGE
#define SYSCTL_V4PORTRANGE_LOW	"net.inet.ip.anonportmin"
#define SYSCTL_V4PORTRANGE_HIGH	"net.inet.ip.anonportmax"
#define SYSCTL_V6PORTRANGE_LOW	"net.inet6.ip6.anonportmin"
#define SYSCTL_V6PORTRANGE_HIGH	"net.inet6.ip6.anonportmax"
#endif

#else /* !HAVE_SYSCTLBYNAME */

#ifdef __OpenBSD__
#define USE_SYSCTL_PORTRANGE
#define SYSCTL_V4PORTRANGE_LOW	{ CTL_NET, PF_INET, IPPROTO_IP, \
				  IPCTL_IPPORT_HIFIRSTAUTO }
#define SYSCTL_V4PORTRANGE_HIGH	{ CTL_NET, PF_INET, IPPROTO_IP, \
				  IPCTL_IPPORT_HILASTAUTO }
/* Same for IPv6 */
#define SYSCTL_V6PORTRANGE_LOW	SYSCTL_V4PORTRANGE_LOW
#define SYSCTL_V6PORTRANGE_HIGH	SYSCTL_V4PORTRANGE_HIGH
#endif

#endif /* HAVE_SYSCTLBYNAME */

#if defined(ISC_PLATFORM_HAVEIPV6)
# if defined(ISC_PLATFORM_NEEDIN6ADDRANY)
const struct in6_addr isc_net_in6addrany = IN6ADDR_ANY_INIT;
# endif

# if defined(ISC_PLATFORM_NEEDIN6ADDRLOOPBACK)
const struct in6_addr isc_net_in6addrloop = IN6ADDR_LOOPBACK_INIT;
# endif

# if defined(WANT_IPV6)
static isc_once_t 	once_ipv6only = ISC_ONCE_INIT;
# endif

# if defined(ISC_PLATFORM_HAVEIN6PKTINFO)
static isc_once_t 	once_ipv6pktinfo = ISC_ONCE_INIT;
# endif
#endif /* ISC_PLATFORM_HAVEIPV6 */

static isc_once_t 	once = ISC_ONCE_INIT;
static isc_once_t 	once_dscp = ISC_ONCE_INIT;

static isc_result_t	ipv4_result = ISC_R_NOTFOUND;
static isc_result_t	ipv6_result = ISC_R_NOTFOUND;
static isc_result_t	unix_result = ISC_R_NOTFOUND;
static isc_result_t	ipv6only_result = ISC_R_NOTFOUND;
static isc_result_t	ipv6pktinfo_result = ISC_R_NOTFOUND;
static unsigned int	dscp_result = 0;

static isc_result_t
try_proto(int domain) {
	int s;
	isc_result_t result = ISC_R_SUCCESS;
	char strbuf[ISC_STRERRORSIZE];

	s = socket(domain, SOCK_STREAM, 0);
	if (s == -1) {
		switch (errno) {
#ifdef EAFNOSUPPORT
		case EAFNOSUPPORT:
#endif
#ifdef EPROTONOSUPPORT
		case EPROTONOSUPPORT:
#endif
#ifdef EINVAL
		case EINVAL:
#endif
			return (ISC_R_NOTFOUND);
		default:
			isc__strerror(errno, strbuf, sizeof(strbuf));
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "socket() %s: %s",
					 isc_msgcat_get(isc_msgcat,
							ISC_MSGSET_GENERAL,
							ISC_MSG_FAILED,
							"failed"),
					 strbuf);
			return (ISC_R_UNEXPECTED);
		}
	}

#ifdef ISC_PLATFORM_HAVEIPV6
#ifdef WANT_IPV6
#ifdef ISC_PLATFORM_HAVEIN6PKTINFO
	if (domain == PF_INET6) {
		struct sockaddr_in6 sin6;
		unsigned int len;

		/*
		 * Check to see if IPv6 is broken, as is common on Linux.
		 */
		len = sizeof(sin6);
		if (getsockname(s, (struct sockaddr *)&sin6, (void *)&len) < 0)
		{
			isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_SOCKET, ISC_LOG_ERROR,
				      "retrieving the address of an IPv6 "
				      "socket from the kernel failed.");
			isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_SOCKET, ISC_LOG_ERROR,
				      "IPv6 is not supported.");
			result = ISC_R_NOTFOUND;
		} else {
			if (len == sizeof(struct sockaddr_in6))
				result = ISC_R_SUCCESS;
			else {
				isc_log_write(isc_lctx,
					      ISC_LOGCATEGORY_GENERAL,
					      ISC_LOGMODULE_SOCKET,
					      ISC_LOG_ERROR,
					      "IPv6 structures in kernel and "
					      "user space do not match.");
				isc_log_write(isc_lctx,
					      ISC_LOGCATEGORY_GENERAL,
					      ISC_LOGMODULE_SOCKET,
					      ISC_LOG_ERROR,
					      "IPv6 is not supported.");
				result = ISC_R_NOTFOUND;
			}
		}
	}
#endif
#endif
#endif

	(void)close(s);

	return (result);
}

static void
initialize_action(void) {
	ipv4_result = try_proto(PF_INET);
#ifdef ISC_PLATFORM_HAVEIPV6
#ifdef WANT_IPV6
#ifdef ISC_PLATFORM_HAVEIN6PKTINFO
	ipv6_result = try_proto(PF_INET6);
#endif
#endif
#endif
#ifdef ISC_PLATFORM_HAVESYSUNH
	unix_result = try_proto(PF_UNIX);
#endif
}

static void
initialize(void) {
	RUNTIME_CHECK(isc_once_do(&once, initialize_action) == ISC_R_SUCCESS);
}

isc_result_t
isc_net_probeipv4(void) {
	initialize();
	return (ipv4_result);
}

isc_result_t
isc_net_probeipv6(void) {
	initialize();
	return (ipv6_result);
}

isc_result_t
isc_net_probeunix(void) {
	initialize();
	return (unix_result);
}

#ifdef ISC_PLATFORM_HAVEIPV6
#ifdef WANT_IPV6
static void
try_ipv6only(void) {
#ifdef IPV6_V6ONLY
	int s, on;
	char strbuf[ISC_STRERRORSIZE];
#endif
	isc_result_t result;

	result = isc_net_probeipv6();
	if (result != ISC_R_SUCCESS) {
		ipv6only_result = result;
		return;
	}

#ifndef IPV6_V6ONLY
	ipv6only_result = ISC_R_NOTFOUND;
	return;
#else
	/* check for TCP sockets */
	s = socket(PF_INET6, SOCK_STREAM, 0);
	if (s == -1) {
		isc__strerror(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "socket() %s: %s",
				 isc_msgcat_get(isc_msgcat,
						ISC_MSGSET_GENERAL,
						ISC_MSG_FAILED,
						"failed"),
				 strbuf);
		ipv6only_result = ISC_R_UNEXPECTED;
		return;
	}

	on = 1;
	if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) < 0) {
		ipv6only_result = ISC_R_NOTFOUND;
		goto close;
	}

	close(s);

	/* check for UDP sockets */
	s = socket(PF_INET6, SOCK_DGRAM, 0);
	if (s == -1) {
		isc__strerror(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "socket() %s: %s",
				 isc_msgcat_get(isc_msgcat,
						ISC_MSGSET_GENERAL,
						ISC_MSG_FAILED,
						"failed"),
				 strbuf);
		ipv6only_result = ISC_R_UNEXPECTED;
		return;
	}

	on = 1;
	if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) < 0) {
		ipv6only_result = ISC_R_NOTFOUND;
		goto close;
	}

	ipv6only_result = ISC_R_SUCCESS;

close:
	close(s);
	return;
#endif /* IPV6_V6ONLY */
}

static void
initialize_ipv6only(void) {
	RUNTIME_CHECK(isc_once_do(&once_ipv6only,
				  try_ipv6only) == ISC_R_SUCCESS);
}
#endif /* WANT_IPV6 */

#ifdef ISC_PLATFORM_HAVEIN6PKTINFO
#ifdef WANT_IPV6
static void
try_ipv6pktinfo(void) {
	int s, on;
	char strbuf[ISC_STRERRORSIZE];
	isc_result_t result;
	int optname;

	result = isc_net_probeipv6();
	if (result != ISC_R_SUCCESS) {
		ipv6pktinfo_result = result;
		return;
	}

	/* we only use this for UDP sockets */
	s = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (s == -1) {
		isc__strerror(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "socket() %s: %s",
				 isc_msgcat_get(isc_msgcat,
						ISC_MSGSET_GENERAL,
						ISC_MSG_FAILED,
						"failed"),
				 strbuf);
		ipv6pktinfo_result = ISC_R_UNEXPECTED;
		return;
	}

#ifdef IPV6_RECVPKTINFO
	optname = IPV6_RECVPKTINFO;
#else
	optname = IPV6_PKTINFO;
#endif
	on = 1;
	if (setsockopt(s, IPPROTO_IPV6, optname, &on, sizeof(on)) < 0) {
		ipv6pktinfo_result = ISC_R_NOTFOUND;
		goto close;
	}

	ipv6pktinfo_result = ISC_R_SUCCESS;

close:
	close(s);
	return;
}

static void
initialize_ipv6pktinfo(void) {
	RUNTIME_CHECK(isc_once_do(&once_ipv6pktinfo,
				  try_ipv6pktinfo) == ISC_R_SUCCESS);
}
#endif /* WANT_IPV6 */
#endif /* ISC_PLATFORM_HAVEIN6PKTINFO */
#endif /* ISC_PLATFORM_HAVEIPV6 */

isc_result_t
isc_net_probe_ipv6only(void) {
#ifdef ISC_PLATFORM_HAVEIPV6
#ifdef WANT_IPV6
	initialize_ipv6only();
#else
	ipv6only_result = ISC_R_NOTFOUND;
#endif
#endif
	return (ipv6only_result);
}

isc_result_t
isc_net_probe_ipv6pktinfo(void) {
#ifdef ISC_PLATFORM_HAVEIPV6
#ifdef ISC_PLATFORM_HAVEIN6PKTINFO
#ifdef WANT_IPV6
	initialize_ipv6pktinfo();
#else
	ipv6pktinfo_result = ISC_R_NOTFOUND;
#endif
#endif
#endif
	return (ipv6pktinfo_result);
}

static inline ISC_SOCKADDR_LEN_T
cmsg_len(ISC_SOCKADDR_LEN_T len) {
#ifdef CMSG_LEN
	return (CMSG_LEN(len));
#else
	ISC_SOCKADDR_LEN_T hdrlen;

	/*
	 * Cast NULL so that any pointer arithmetic performed by CMSG_DATA
	 * is correct.
	 */
	hdrlen = (ISC_SOCKADDR_LEN_T)CMSG_DATA(((struct cmsghdr *)NULL));
	return (hdrlen + len);
#endif
}

static inline ISC_SOCKADDR_LEN_T
cmsg_space(ISC_SOCKADDR_LEN_T len) {
#ifdef CMSG_SPACE
	return (CMSG_SPACE(len));
#else
	struct msghdr msg;
	struct cmsghdr *cmsgp;
	/*
	 * XXX: The buffer length is an ad-hoc value, but should be enough
	 * in a practical sense.
	 */
	char dummybuf[sizeof(struct cmsghdr) + 1024];

	memset(&msg, 0, sizeof(msg));
	msg.msg_control = dummybuf;
	msg.msg_controllen = sizeof(dummybuf);

	cmsgp = (struct cmsghdr *)dummybuf;
	cmsgp->cmsg_len = cmsg_len(len);

	cmsgp = CMSG_NXTHDR(&msg, cmsgp);
	if (cmsgp != NULL)
		return ((char *)cmsgp - (char *)msg.msg_control);
	else
		return (0);
#endif
}

#ifdef ISC_NET_BSD44MSGHDR
/*
 * Make a fd non-blocking.
 */
static isc_result_t
make_nonblock(int fd) {
	int ret;
	int flags;
	char strbuf[ISC_STRERRORSIZE];
#ifdef USE_FIONBIO_IOCTL
	int on = 1;

	ret = ioctl(fd, FIONBIO, (char *)&on);
#else
	flags = fcntl(fd, F_GETFL, 0);
	flags |= PORT_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
#endif

	if (ret == -1) {
		isc__strerror(errno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(__FILE__, __LINE__,
#ifdef USE_FIONBIO_IOCTL
				 "ioctl(%d, FIONBIO, &on): %s", fd,
#else
				 "fcntl(%d, F_SETFL, %d): %s", fd, flags,
#endif
				 strbuf);

		return (ISC_R_UNEXPECTED);
	}

	return (ISC_R_SUCCESS);
}

static isc_boolean_t
cmsgsend(int s, int level, int type, struct addrinfo *res) {
	char strbuf[ISC_STRERRORSIZE];
	struct sockaddr_storage ss;
	ISC_SOCKADDR_LEN_T len = sizeof(ss);
	struct msghdr msg;
	union {
		struct cmsghdr h;
		unsigned char b[256];
	} control;
	struct cmsghdr *cmsgp;
	int dscp = 46;
	struct iovec iovec;
	char buf[1] = { 0 };
	isc_result_t result;

	if (bind(s, res->ai_addr, res->ai_addrlen) < 0) {
		isc__strerror(errno, strbuf, sizeof(strbuf));
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_SOCKET, ISC_LOG_DEBUG(10),
			      "bind: %s", strbuf);
		return (ISC_FALSE);
	}

	if (getsockname(s, (struct sockaddr *)&ss, &len) < 0) {
		isc__strerror(errno, strbuf, sizeof(strbuf));
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_SOCKET, ISC_LOG_DEBUG(10),
			      "getsockname: %s", strbuf);
		return (ISC_FALSE);
	}

	memset(&control, 0, sizeof(control));

	iovec.iov_base = buf;
	iovec.iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (struct sockaddr *)&ss;
	msg.msg_namelen = len;
	msg.msg_iov = &iovec;
	msg.msg_iovlen = 1;
	msg.msg_control = (void*)&control;
	msg.msg_controllen = cmsg_space(sizeof(int));
	msg.msg_flags = 0;

	cmsgp = msg.msg_control;
	cmsgp->cmsg_level = level;
	cmsgp->cmsg_type = type;

	switch (cmsgp->cmsg_type) {
#ifdef IP_TOS
	case IP_TOS:
		cmsgp->cmsg_len = cmsg_len(sizeof(char));
		*(unsigned char*)CMSG_DATA(cmsgp) = dscp;
		break;
#endif
#ifdef IPV6_TCLASS
	case IPV6_TCLASS:
		cmsgp->cmsg_len = cmsg_len(sizeof(dscp));
		memmove(CMSG_DATA(cmsgp), &dscp, sizeof(dscp));
		break;
#endif
	default:
		INSIST(0);
	}

	if (sendmsg(s, &msg, 0) < 0) {
		int debug = ISC_LOG_DEBUG(10);
		switch (errno) {
#ifdef ENOPROTOOPT
		case ENOPROTOOPT:
#endif
#ifdef EOPNOTSUPP
		case EOPNOTSUPP:
#endif
		case EINVAL:
			break;
		default:
			debug = ISC_LOG_NOTICE;
		}
		isc__strerror(errno, strbuf, sizeof(strbuf));
		if (debug != ISC_LOG_NOTICE) {
			isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
				      ISC_LOGMODULE_SOCKET, ISC_LOG_DEBUG(10),
				      "sendmsg: %s", strbuf);
		} else {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "sendmsg() %s: %s",
					 isc_msgcat_get(isc_msgcat,
							ISC_MSGSET_GENERAL,
							ISC_MSG_FAILED,
							"failed"),
					 strbuf);
		}
		return (ISC_FALSE);
	}

	/*
	 * Make sure the message actually got sent.
	 */
	result = make_nonblock(s);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	iovec.iov_base = buf;
	iovec.iov_len = sizeof(buf);

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (struct sockaddr *)&ss;
	msg.msg_namelen = sizeof(ss);
	msg.msg_iov = &iovec;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;

	if (recvmsg(s, &msg, 0) < 0)
		return (ISC_FALSE);

	return (ISC_TRUE);
}
#endif

static void
try_dscp_v4(void) {
#ifdef IP_TOS
	char strbuf[ISC_STRERRORSIZE];
	struct addrinfo hints, *res0;
	int s, dscp = 0, n;
#ifdef IP_RECVTOS
	int on = 1;
#endif /* IP_RECVTOS */

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
#ifdef AI_NUMERICHOST
	hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;
#else
	hints.ai_flags = AI_PASSIVE;
#endif

	n = getaddrinfo("127.0.0.1", NULL, &hints, &res0);
	if (n != 0 || res0 == NULL) {
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_SOCKET, ISC_LOG_DEBUG(10),
			      "getaddrinfo(127.0.0.1): %s", gai_strerror(n));
		return;
	}

	s = socket(res0->ai_family, res0->ai_socktype, res0->ai_protocol);

	if (s == -1) {
		isc__strerror(errno, strbuf, sizeof(strbuf));
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_SOCKET, ISC_LOG_DEBUG(10),
			      "socket: %s", strbuf);
		freeaddrinfo(res0);
		return;
	}

	if (setsockopt(s, IPPROTO_IP, IP_TOS, &dscp, sizeof(dscp)) == 0)
		dscp_result |= ISC_NET_DSCPSETV4;

#ifdef IP_RECVTOS
	on = 1;
	if (setsockopt(s, IPPROTO_IP, IP_RECVTOS, &on, sizeof(on)) == 0)
		dscp_result |= ISC_NET_DSCPRECVV4;
#endif /* IP_RECVTOS */

#ifdef ISC_NET_BSD44MSGHDR

#ifndef ISC_CMSG_IP_TOS
#ifdef __APPLE__
#define ISC_CMSG_IP_TOS 0	/* As of 10.8.2. */
#else /* ! __APPLE__ */
#define ISC_CMSG_IP_TOS 1
#endif /* ! __APPLE__ */
#endif /* ! ISC_CMSG_IP_TOS */

#if ISC_CMSG_IP_TOS
	if (cmsgsend(s, IPPROTO_IP, IP_TOS, res0))
		dscp_result |= ISC_NET_DSCPPKTV4;
#endif /* ISC_CMSG_IP_TOS */

#endif /* ISC_NET_BSD44MSGHDR */

	freeaddrinfo(res0);
	close(s);

#endif /* IP_TOS */
}

static void
try_dscp_v6(void) {
#ifdef ISC_PLATFORM_HAVEIPV6
#ifdef WANT_IPV6
#ifdef IPV6_TCLASS
	char strbuf[ISC_STRERRORSIZE];
	struct addrinfo hints, *res0;
	int s, dscp = 0, n;
#if defined(IPV6_RECVTCLASS)
	int on = 1;
#endif /* IPV6_RECVTCLASS */

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
#ifdef AI_NUMERICHOST
	hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;
#else
	hints.ai_flags = AI_PASSIVE;
#endif

	n = getaddrinfo("::1", NULL, &hints, &res0);
	if (n != 0 || res0 == NULL) {
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_SOCKET, ISC_LOG_DEBUG(10),
			      "getaddrinfo(::1): %s", gai_strerror(n));
		return;
	}

	s = socket(res0->ai_family, res0->ai_socktype, res0->ai_protocol);
	if (s == -1) {
		isc__strerror(errno, strbuf, sizeof(strbuf));
		isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL,
			      ISC_LOGMODULE_SOCKET, ISC_LOG_DEBUG(10),
			      "socket: %s", strbuf);
		freeaddrinfo(res0);
		return;
	}
	if (setsockopt(s, IPPROTO_IPV6, IPV6_TCLASS, &dscp, sizeof(dscp)) == 0)
		dscp_result |= ISC_NET_DSCPSETV6;

#ifdef IPV6_RECVTCLASS
	on = 1;
	if (setsockopt(s, IPPROTO_IPV6, IPV6_RECVTCLASS, &on, sizeof(on)) == 0)
		dscp_result |= ISC_NET_DSCPRECVV6;
#endif /* IPV6_RECVTCLASS */

#ifdef ISC_NET_BSD44MSGHDR
	if (cmsgsend(s, IPPROTO_IPV6, IPV6_TCLASS, res0))
		dscp_result |= ISC_NET_DSCPPKTV6;
#endif /* ISC_NET_BSD44MSGHDR */

	freeaddrinfo(res0);
	close(s);

#endif /* IPV6_TCLASS */
#endif /* WANT_IPV6 */
#endif /* ISC_PLATFORM_HAVEIPV6 */
}

static void
try_dscp(void) {
	try_dscp_v4();
	try_dscp_v6();
}

static void
initialize_dscp(void) {
	RUNTIME_CHECK(isc_once_do(&once_dscp, try_dscp) == ISC_R_SUCCESS);
}

unsigned int
isc_net_probedscp(void) {
	initialize_dscp();
	return (dscp_result);
}

#if defined(USE_SYSCTL_PORTRANGE)
#if defined(HAVE_SYSCTLBYNAME)
static isc_result_t
getudpportrange_sysctl(int af, in_port_t *low, in_port_t *high) {
	int port_low, port_high;
	size_t portlen;
	const char *sysctlname_lowport, *sysctlname_hiport;

	if (af == AF_INET) {
		sysctlname_lowport = SYSCTL_V4PORTRANGE_LOW;
		sysctlname_hiport = SYSCTL_V4PORTRANGE_HIGH;
	} else {
		sysctlname_lowport = SYSCTL_V6PORTRANGE_LOW;
		sysctlname_hiport = SYSCTL_V6PORTRANGE_HIGH;
	}
	portlen = sizeof(portlen);
	if (sysctlbyname(sysctlname_lowport, &port_low, &portlen,
			 NULL, 0) < 0) {
		return (ISC_R_FAILURE);
	}
	portlen = sizeof(portlen);
	if (sysctlbyname(sysctlname_hiport, &port_high, &portlen,
			 NULL, 0) < 0) {
		return (ISC_R_FAILURE);
	}
	if ((port_low & ~0xffff) != 0 || (port_high & ~0xffff) != 0)
		return (ISC_R_RANGE);

	*low = (in_port_t)port_low;
	*high = (in_port_t)port_high;

	return (ISC_R_SUCCESS);
}
#else /* !HAVE_SYSCTLBYNAME */
static isc_result_t
getudpportrange_sysctl(int af, in_port_t *low, in_port_t *high) {
	int mib_lo4[4] = SYSCTL_V4PORTRANGE_LOW;
	int mib_hi4[4] = SYSCTL_V4PORTRANGE_HIGH;
	int mib_lo6[4] = SYSCTL_V6PORTRANGE_LOW;
	int mib_hi6[4] = SYSCTL_V6PORTRANGE_HIGH;
	int *mib_lo, *mib_hi, miblen;
	int port_low, port_high;
	size_t portlen;

	if (af == AF_INET) {
		mib_lo = mib_lo4;
		mib_hi = mib_hi4;
		miblen = sizeof(mib_lo4) / sizeof(mib_lo4[0]);
	} else {
		mib_lo = mib_lo6;
		mib_hi = mib_hi6;
		miblen = sizeof(mib_lo6) / sizeof(mib_lo6[0]);
	}

	portlen = sizeof(portlen);
	if (sysctl(mib_lo, miblen, &port_low, &portlen, NULL, 0) < 0) {
		return (ISC_R_FAILURE);
	}

	portlen = sizeof(portlen);
	if (sysctl(mib_hi, miblen, &port_high, &portlen, NULL, 0) < 0) {
		return (ISC_R_FAILURE);
	}

	if ((port_low & ~0xffff) != 0 || (port_high & ~0xffff) != 0)
		return (ISC_R_RANGE);

	*low = (in_port_t) port_low;
	*high = (in_port_t) port_high;

	return (ISC_R_SUCCESS);
}
#endif /* HAVE_SYSCTLBYNAME */
#endif /* USE_SYSCTL_PORTRANGE */

isc_result_t
isc_net_getudpportrange(int af, in_port_t *low, in_port_t *high) {
	int result = ISC_R_FAILURE;

	REQUIRE(low != NULL && high != NULL);

#if defined(USE_SYSCTL_PORTRANGE)
	result = getudpportrange_sysctl(af, low, high);
#else
	UNUSED(af);
#endif

	if (result != ISC_R_SUCCESS) {
		*low = ISC_NET_PORTRANGELOW;
		*high = ISC_NET_PORTRANGEHIGH;
	}

	return (ISC_R_SUCCESS);	/* we currently never fail in this function */
}

void
isc_net_disableipv4(void) {
	initialize();
	if (ipv4_result == ISC_R_SUCCESS)
		ipv4_result = ISC_R_DISABLED;
}

void
isc_net_disableipv6(void) {
	initialize();
	if (ipv6_result == ISC_R_SUCCESS)
		ipv6_result = ISC_R_DISABLED;
}

void
isc_net_enableipv4(void) {
	initialize();
	if (ipv4_result == ISC_R_DISABLED)
		ipv4_result = ISC_R_SUCCESS;
}

void
isc_net_enableipv6(void) {
	initialize();
	if (ipv6_result == ISC_R_DISABLED)
		ipv6_result = ISC_R_SUCCESS;
}
