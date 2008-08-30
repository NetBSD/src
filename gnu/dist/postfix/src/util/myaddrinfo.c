/*++
/* NAME
/*	myaddrinfo 3
/* SUMMARY
/*	addrinfo encapsulation and emulation
/* SYNOPSIS
/*	#include <myaddrinfo.h>
/*
/*	#define MAI_V4ADDR_BITS ...
/*	#define MAI_V6ADDR_BITS ...
/*	#define MAI_V4ADDR_BYTES ...
/*	#define MAI_V6ADDR_BYTES ...
/*
/*	typedef struct { char buf[....]; } MAI_HOSTNAME_STR;
/*	typedef struct { char buf[....]; } MAI_HOSTADDR_STR;
/*	typedef struct { char buf[....]; } MAI_SERVNAME_STR;
/*	typedef struct { char buf[....]; } MAI_SERVPORT_STR;
/*
/*	int	hostname_to_sockaddr(hostname, service, socktype, result)
/*	const char *hostname;
/*	const char *service;
/*	int	socktype;
/*	struct addrinfo **result;
/*
/*	int	hostaddr_to_sockaddr(hostaddr, service, socktype, result)
/*	const char *hostaddr;
/*	const char *service;
/*	int	socktype;
/*	struct addrinfo **result;
/*
/*	int	sockaddr_to_hostaddr(sa, salen, hostaddr, portnum, socktype)
/*	const struct sockaddr *sa;
/*	SOCKADDR_SIZE salen;
/*	MAI_HOSTADDR_STR *hostaddr;
/*	MAI_SERVPORT_STR *portnum;
/*	int	socktype;
/*
/*	int	sockaddr_to_hostname(sa, salen, hostname, service, socktype)
/*	const struct sockaddr *sa;
/*	SOCKADDR_SIZE salen;
/*	MAI_HOSTNAME_STR *hostname;
/*	MAI_SERVNAME_STR *service;
/*	int	socktype;
/*
/*	const char *MAI_STRERROR(error)
/*	int	error;
/* DESCRIPTION
/*	This module provides a simplified user interface to the
/*	getaddrinfo(3) and getnameinfo(3) routines (which provide
/*	a unified interface to manipulate IPv4 and IPv6 socket
/*	address structures).
/*
/*	On systems without getaddrinfo(3) and getnameinfo(3) support,
/*	emulation for IPv4 only can be enabled by defining
/*	EMULATE_IPV4_ADDRINFO.
/*
/*	hostname_to_sockaddr() looks up the binary addresses for
/*	the specified symbolic hostname or numeric address.  The
/*	result should be destroyed with freeaddrinfo(). A null host
/*	pointer converts to the null host address.
/*
/*	hostaddr_to_sockaddr() converts a printable network address
/*	into the corresponding binary form.  The result should be
/*	destroyed with freeaddrinfo(). A null host pointer converts
/*	to the null host address.
/*
/*	sockaddr_to_hostaddr() converts a binary network address
/*	into printable form. The result buffers should be large
/*	enough to hold the printable address or port including the
/*	null terminator.
/*
/*	sockaddr_to_hostname() converts a binary network address
/*	into a hostname or service.  The result buffer should be
/*	large enough to hold the hostname or service including the
/*	null terminator. This routine rejects malformed hostnames
/*	or numeric hostnames and pretends that the lookup failed.
/*
/*	MAI_STRERROR() is an unsafe macro (it evaluates the argument
/*	multiple times) that invokes strerror() or gai_strerror()
/*	as appropriate.
/*
/*	This module exports the following constants that should be
/*	user for storage allocation of name or address information:
/* .IP MAI_V4ADDR_BITS
/* .IP MAI_V6ADDR_BITS
/* .IP MAI_V4ADDR_BYTES
/* .IP MAI_V6ADDR_BYTES
/*	The number of bits or bytes needed to store a binary
/*	IPv4 or IPv6 network address.
/* .PP
/*	The types MAI_HOST{NAME,ADDR}_STR and MAI_SERV{NAME,PORT}_STR
/*	implement buffers for the storage of the string representations
/*	of symbolic or numerical hosts or services. Do not use
/*	buffer types other than the ones that are expected here,
/*	or things will blow up with buffer overflow problems.
/*
/*	Arguments:
/* .IP hostname
/*	On input to hostname_to_sockaddr(), a numeric or symbolic
/*	hostname, or a null pointer (meaning the wild-card listen
/*	address).  On output from sockaddr_to_hostname(), storage
/*	for the result hostname, or a null pointer.
/* .IP hostaddr
/*	On input to hostaddr_to_sockaddr(), a numeric hostname,
/*	or a null pointer (meaning the wild-card listen address).
/*	On output from sockaddr_to_hostaddr(), storage for the
/*	result hostaddress, or a null pointer.
/* .IP service
/*	On input to hostname/addr_to_sockaddr(), a numeric or
/*	symbolic service name, or a null pointer in which case the
/*	socktype argument is ignored.  On output from
/*	sockaddr_to_hostname/addr(), storage for the result service
/*	name, or a null pointer.
/* .IP portnum
/*	Storage for the result service port number, or a null pointer.
/* .IP socktype
/*	Socket type: SOCK_STREAM, SOCK_DGRAM, etc. This argument is
/*	ignored when no service or port are specified.
/* .IP sa
/*	Protocol-independent socket address structure.
/* .IP salen
/*	Protocol-dependent socket address structure size in bytes.
/* SEE ALSO
/*	getaddrinfo(3), getnameinfo(3), freeaddrinfo(3), gai_strerror(3)
/* DIAGNOSTICS
/*	All routines either return 0 upon success, or an error code
/*	that is compatible with gai_strerror().
/*
/*	On systems where addrinfo support is emulated by Postfix,
/*	some out-of-memory errors are not reported to the caller,
/*	but are handled by mymalloc().
/* BUGS
/*	The IPv4-only emulation code does not support requests that
/*	specify a service but no socket type. It returns an error
/*	indication, instead of enumerating all the possible answers.
/*
/*	The hostname/addr_to_sockaddr() routines should accept a
/*	list of address families that the caller is interested in,
/*	and they should return only information of those types.
/*
/*	Unfortunately, it is not possible to remove unwanted address
/*	family results from hostname_to_sockaddr(), because we
/*	don't know how the system library routine getaddrinfo()
/*	allocates memory.  For example, getaddrinfo() could save
/*	space by referencing the same string object from multiple
/*	addrinfo structures; or it could allocate a string object
/*	and the addrinfo structure as one memory block.
/*
/*	We could get around this by copying getaddrinfo() results
/*	to our own private data structures, but that would only
/*	make an already expensive API even more expensive.
/*
/*	A better workaround is to return a vector of addrinfo
/*	pointers to the elements that contain only the elements
/*	that the caller is interested in. The pointer to the
/*	original getaddrinfo() result can be hidden at the end
/*	after the null terminator, or before the first element.
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>			/* sprintf() */

/* Utility library. */

#include <mymalloc.h>
#include <valid_hostname.h>
#include <sock_addr.h>
#include <stringops.h>
#include <msg.h>
#include <inet_proto.h>
#include <myaddrinfo.h>

/* Application-specific. */

 /*
  * Use an old trick to save some space: allocate space for two objects in
  * one. In Postfix we often use this trick for structures that have an array
  * of things at the end.
  */
struct ipv4addrinfo {
    struct addrinfo info;
    struct sockaddr_in sin;
};

 /*
  * When we're not interested in service ports, we must pick a socket type
  * otherwise getaddrinfo() will give us duplicate results: one set for TCP,
  * and another set for UDP. For consistency, we'll use the same default
  * socket type for the results from emulation mode.
  */
#define MAI_SOCKTYPE	SOCK_STREAM	/* getaddrinfo() query */

#ifdef EMULATE_IPV4_ADDRINFO

/* clone_ipv4addrinfo - clone ipv4addrinfo structure */

static struct ipv4addrinfo *clone_ipv4addrinfo(struct ipv4addrinfo * tp)
{
    struct ipv4addrinfo *ip;

    ip = (struct ipv4addrinfo *) mymalloc(sizeof(*ip));
    *ip = *tp;
    ip->info.ai_addr = (struct sockaddr *) & (ip->sin);
    return (ip);
}

/* init_ipv4addrinfo - initialize an ipv4addrinfo structure */

static void init_ipv4addrinfo(struct ipv4addrinfo * ip, int socktype)
{

    /*
     * Portability: null pointers aren't necessarily all-zero bits, so we
     * make explicit assignments to all the pointers that we're aware of.
     */
    memset((char *) ip, 0, sizeof(*ip));
    ip->info.ai_family = PF_INET;
    ip->info.ai_socktype = socktype;
    ip->info.ai_protocol = 0;			/* XXX */
    ip->info.ai_addrlen = sizeof(ip->sin);
    ip->info.ai_canonname = 0;
    ip->info.ai_addr = (struct sockaddr *) & (ip->sin);
    ip->info.ai_next = 0;
    ip->sin.sin_family = AF_INET;
#ifdef HAS_SA_LEN
    ip->sin.sin_len = sizeof(ip->sin);
#endif
}

/* find_service - translate numeric or symbolic service name */

static int find_service(const char *service, int socktype)
{
    struct servent *sp;
    const char *proto;
    unsigned port;

    if (alldig(service)) {
	port = atoi(service);
	return (port < 65536 ? htons(port) : -1);
    }
    if (socktype == SOCK_STREAM) {
	proto = "tcp";
    } else if (socktype == SOCK_DGRAM) {
	proto = "udp";
    } else {
	return (-1);
    }
    if ((sp = getservbyname(service, proto)) != 0) {
	return (sp->s_port);
    } else {
	return (-1);
    }
}

#endif

/* hostname_to_sockaddr - hostname to binary address form */

int     hostname_to_sockaddr(const char *hostname, const char *service,
			             int socktype, struct addrinfo ** res)
{
#ifdef EMULATE_IPV4_ADDRINFO

    /*
     * Emulated getaddrinfo(3) version.
     */
    static struct ipv4addrinfo template;
    struct ipv4addrinfo *ip;
    struct ipv4addrinfo *prev;
    struct in_addr addr;
    struct hostent *hp;
    char  **name_list;
    int     port;

    /*
     * Validate the service.
     */
    if (service) {
	if ((port = find_service(service, socktype)) < 0)
	    return (EAI_SERVICE);
    } else {
	port = 0;
	socktype = MAI_SOCKTYPE;
    }

    /*
     * No host means INADDR_ANY.
     */
    if (hostname == 0) {
	ip = (struct ipv4addrinfo *) mymalloc(sizeof(*ip));
	init_ipv4addrinfo(ip, socktype);
	ip->sin.sin_addr.s_addr = INADDR_ANY;
	ip->sin.sin_port = port;
	*res = &(ip->info);
	return (0);
    }

    /*
     * Numeric host.
     */
    if (inet_pton(AF_INET, hostname, (void *) &addr) == 1) {
	ip = (struct ipv4addrinfo *) mymalloc(sizeof(*ip));
	init_ipv4addrinfo(ip, socktype);
	ip->sin.sin_addr = addr;
	ip->sin.sin_port = port;
	*res = &(ip->info);
	return (0);
    }

    /*
     * Look up the IPv4 address list.
     */
    if ((hp = gethostbyname(hostname)) == 0)
	return (h_errno == TRY_AGAIN ? EAI_AGAIN : EAI_NODATA);
    if (hp->h_addrtype != AF_INET
	|| hp->h_length != sizeof(template.sin.sin_addr))
	return (EAI_NODATA);

    /*
     * Initialize the result template.
     */
    if (template.info.ai_addrlen == 0)
	init_ipv4addrinfo(&template, socktype);

    /*
     * Copy the address information into an addrinfo structure.
     */
    prev = &template;
    for (name_list = hp->h_addr_list; name_list[0]; name_list++) {
	ip = clone_ipv4addrinfo(prev);
	ip->sin.sin_addr = IN_ADDR(name_list[0]);
	ip->sin.sin_port = port;
	if (prev == &template)
	    *res = &(ip->info);
	else
	    prev->info.ai_next = &(ip->info);
	prev = ip;
    }
    return (0);
#else

    /*
     * Native getaddrinfo(3) version.
     * 
     * XXX Wild-card listener issues.
     * 
     * With most IPv4 plus IPv6 systems, an IPv6 wild-card listener also listens
     * on the IPv4 wild-card address. Connections from IPv4 clients appear as
     * IPv4-in-IPv6 addresses; when Postfix support for IPv4 is turned on,
     * Postfix automatically maps these embedded addresses to their original
     * IPv4 form. So everything seems to be fine.
     * 
     * However, some applications prefer to use separate listener sockets for
     * IPv4 and IPv6. The Postfix IPv6 patch provided such an example. And
     * this is where things become tricky. On many systems the IPv6 and IPv4
     * wild-card listeners cannot coexist. When one is already active, the
     * other fails with EADDRINUSE. Solaris 9, however, will automagically
     * "do the right thing" and allow both listeners to coexist.
     * 
     * Recent systems have the IPV6_V6ONLY feature (RFC 3493), which tells the
     * system that we really mean IPv6 when we say IPv6. This allows us to
     * set up separate wild-card listener sockets for IPv4 and IPv6. So
     * everything seems to be fine again.
     * 
     * The following workaround disables the wild-card IPv4 listener when
     * IPV6_V6ONLY is unavailable. This is necessary for some Linux versions,
     * but is not needed for Solaris 9 (which allows IPv4 and IPv6 wild-card
     * listeners to coexist). Solaris 10 beta already has IPV6_V6ONLY.
     * 
     * XXX This workaround obviously breaks if we want to support protocols in
     * addition to IPv6 and IPv4, but it is needed only until IPv6
     * implementations catch up with RFC 3493. A nicer fix is to filter the
     * getaddrinfo() result, and to return a vector of addrinfo pointers to
     * only those types of elements that the caller has expressed interested
     * in.
     * 
     * XXX Vanilla AIX 5.1 getaddrinfo() does not support a null hostname with
     * AI_PASSIVE. And since we don't know how getaddrinfo() manages its
     * memory we can't bypass it for this special case, or freeaddrinfo()
     * might blow up. Instead we turn off IPV6_V6ONLY in inet_listen(), and
     * supply a protocol-dependent hard-coded string value to getaddrinfo()
     * below, so that it will convert into the appropriate wild-card address.
     * 
     * XXX AIX 5.[1-3] getaddrinfo() may return a non-null port when a null
     * service argument is specified.
     */
    struct addrinfo hints;
    int     err;

    memset((char *) &hints, 0, sizeof(hints));
    hints.ai_family = inet_proto_info()->ai_family;
    hints.ai_socktype = service ? socktype : MAI_SOCKTYPE;
    if (!hostname) {
	hints.ai_flags = AI_PASSIVE;
#if !defined(IPV6_V6ONLY) || defined(BROKEN_AI_PASSIVE_NULL_HOST)
	switch (hints.ai_family) {
	case PF_UNSPEC:
	    hints.ai_family = PF_INET6;
#ifdef BROKEN_AI_PASSIVE_NULL_HOST
	case PF_INET6:
	    hostname = "::";
	    break;
	case PF_INET:
	    hostname = "0.0.0.0";
	    break;
#endif
	}
#endif
    }
    err = getaddrinfo(hostname, service, &hints, res);
#if defined(BROKEN_AI_NULL_SERVICE)
    if (service == 0 && err == 0) {
	struct addrinfo *r;
	unsigned short *portp;

	for (r = *res; r != 0; r = r->ai_next)
	    if (*(portp = SOCK_ADDR_PORTP(r->ai_addr)) != 0)
		*portp = 0;
    }
#endif
    return (err);
#endif
}

/* hostaddr_to_sockaddr - printable address to binary address form */

int     hostaddr_to_sockaddr(const char *hostaddr, const char *service,
			             int socktype, struct addrinfo ** res)
{
#ifdef EMULATE_IPV4_ADDRINFO

    /*
     * Emulated getaddrinfo(3) version.
     */
    struct ipv4addrinfo *ip;
    struct in_addr addr;
    int     port;

    /*
     * Validate the service.
     */
    if (service) {
	if ((port = find_service(service, socktype)) < 0)
	    return (EAI_SERVICE);
    } else {
	port = 0;
	socktype = MAI_SOCKTYPE;
    }

    /*
     * No host means INADDR_ANY.
     */
    if (hostaddr == 0) {
	ip = (struct ipv4addrinfo *) mymalloc(sizeof(*ip));
	init_ipv4addrinfo(ip, socktype);
	ip->sin.sin_addr.s_addr = INADDR_ANY;
	ip->sin.sin_port = port;
	*res = &(ip->info);
	return (0);
    }

    /*
     * Deal with bad address forms.
     */
    switch (inet_pton(AF_INET, hostaddr, (void *) &addr)) {
    case 1:					/* Success */
	break;
    default:					/* Unparsable */
	return (EAI_NONAME);
    case -1:					/* See errno */
	return (EAI_SYSTEM);
    }

    /*
     * Initialize the result structure.
     */
    ip = (struct ipv4addrinfo *) mymalloc(sizeof(*ip));
    init_ipv4addrinfo(ip, socktype);

    /*
     * And copy the result.
     */
    ip->sin.sin_addr = addr;
    ip->sin.sin_port = port;
    *res = &(ip->info);

    return (0);
#else

    /*
     * Native getaddrinfo(3) version. See comments in hostname_to_sockaddr().
     * 
     * XXX Vanilla AIX 5.1 getaddrinfo() returns multiple results when
     * converting a printable ipv4 or ipv6 address to socket address with
     * ai_family=PF_UNSPEC, ai_flags=AI_NUMERICHOST, ai_socktype=SOCK_STREAM,
     * ai_protocol=0 or IPPROTO_TCP, and service=0. The workaround is to
     * ignore all but the first result.
     * 
     * XXX AIX 5.[1-3] getaddrinfo() may return a non-null port when a null
     * service argument is specified.
     */
    struct addrinfo hints;
    int     err;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = inet_proto_info()->ai_family;
    hints.ai_socktype = service ? socktype : MAI_SOCKTYPE;
    hints.ai_flags = AI_NUMERICHOST;
    if (!hostaddr) {
	hints.ai_flags |= AI_PASSIVE;
#if !defined(IPV6_V6ONLY) || defined(BROKEN_AI_PASSIVE_NULL_HOST)
	switch (hints.ai_family) {
	case PF_UNSPEC:
	    hints.ai_family = PF_INET6;
#ifdef BROKEN_AI_PASSIVE_NULL_HOST
	case PF_INET6:
	    hostaddr = "::";
	    break;
	case PF_INET:
	    hostaddr = "0.0.0.0";
	    break;
#endif
	}
#endif
    }
    err = getaddrinfo(hostaddr, service, &hints, res);
#if defined(BROKEN_AI_NULL_SERVICE)
    if (service == 0 && err == 0) {
	struct addrinfo *r;
	unsigned short *portp;

	for (r = *res; r != 0; r = r->ai_next)
	    if (*(portp = SOCK_ADDR_PORTP(r->ai_addr)) != 0)
		*portp = 0;
    }
#endif
    return (err);
#endif
}

/* sockaddr_to_hostaddr - binary address to printable address form */

int     sockaddr_to_hostaddr(const struct sockaddr * sa, SOCKADDR_SIZE salen,
			             MAI_HOSTADDR_STR *hostaddr,
			             MAI_SERVPORT_STR *portnum,
			             int unused_socktype)
{
#ifdef EMULATE_IPV4_ADDRINFO
    char    portbuf[sizeof("65535")];
    ssize_t len;

    /*
     * Emulated getnameinfo(3) version. The buffer length includes the space
     * for the null terminator.
     */
    if (sa->sa_family != AF_INET) {
	errno = EAFNOSUPPORT;
	return (EAI_SYSTEM);
    }
    if (hostaddr != 0) {
	if (inet_ntop(AF_INET, (void *) &(SOCK_ADDR_IN_ADDR(sa)),
		      hostaddr->buf, sizeof(hostaddr->buf)) == 0)
	    return (EAI_SYSTEM);
    }
    if (portnum != 0) {
	sprintf(portbuf, "%d", ntohs(SOCK_ADDR_IN_PORT(sa)) & 0xffff);
	if ((len = strlen(portbuf)) >= sizeof(portnum->buf)) {
	    errno = ENOSPC;
	    return (EAI_SYSTEM);
	}
	memcpy(portnum->buf, portbuf, len + 1);
    }
    return (0);
#else

    /*
     * Native getnameinfo(3) version.
     */
    return (getnameinfo(sa, salen,
			hostaddr ? hostaddr->buf : (char *) 0,
			hostaddr ? sizeof(hostaddr->buf) : 0,
			portnum ? portnum->buf : (char *) 0,
			portnum ? sizeof(portnum->buf) : 0,
			NI_NUMERICHOST | NI_NUMERICSERV));
#endif
}

/* sockaddr_to_hostname - binary address to printable hostname */

int     sockaddr_to_hostname(const struct sockaddr * sa, SOCKADDR_SIZE salen,
			             MAI_HOSTNAME_STR *hostname,
			             MAI_SERVNAME_STR *service,
			             int socktype)
{
#ifdef EMULATE_IPV4_ADDRINFO

    /*
     * Emulated getnameinfo(3) version.
     */
    struct hostent *hp;
    struct servent *sp;
    size_t  len;

    /*
     * Sanity check.
     */
    if (sa->sa_family != AF_INET)
	return (EAI_NODATA);

    /*
     * Look up the host name.
     */
    if (hostname != 0) {
	if ((hp = gethostbyaddr((char *) &(SOCK_ADDR_IN_ADDR(sa)),
				sizeof(SOCK_ADDR_IN_ADDR(sa)),
				AF_INET)) == 0)
	    return (h_errno == TRY_AGAIN ? EAI_AGAIN : EAI_NONAME);

	/*
	 * Save the result. The buffer length includes the space for the null
	 * terminator. Hostname sanity checks are at the end of this
	 * function.
	 */
	if ((len = strlen(hp->h_name)) >= sizeof(hostname->buf)) {
	    errno = ENOSPC;
	    return (EAI_SYSTEM);
	}
	memcpy(hostname->buf, hp->h_name, len + 1);
    }

    /*
     * Look up the service.
     */
    if (service != 0) {
	if ((sp = getservbyport(ntohs(SOCK_ADDR_IN_PORT(sa)),
			      socktype == SOCK_DGRAM ? "udp" : "tcp")) == 0)
	    return (EAI_NONAME);

	/*
	 * Save the result. The buffer length includes the space for the null
	 * terminator.
	 */
	if ((len = strlen(sp->s_name)) >= sizeof(service->buf)) {
	    errno = ENOSPC;
	    return (EAI_SYSTEM);
	}
	memcpy(service->buf, sp->s_name, len + 1);
    }
#else

    /*
     * Native getnameinfo(3) version.
     */
    int     err;

    err = getnameinfo(sa, salen,
		      hostname ? hostname->buf : (char *) 0,
		      hostname ? sizeof(hostname->buf) : 0,
		      service ? service->buf : (char *) 0,
		      service ? sizeof(service->buf) : 0,
		      socktype == SOCK_DGRAM ?
		      NI_NAMEREQD | NI_DGRAM : NI_NAMEREQD);
    if (err != 0)
	return (err);
#endif

    /*
     * Hostname sanity checks.
     */
    if (hostname != 0) {
	if (valid_hostaddr(hostname->buf, DONT_GRIPE)) {
	    msg_warn("numeric hostname: %s", hostname->buf);
	    return (EAI_NONAME);
	}
	if (!valid_hostname(hostname->buf, DO_GRIPE))
	    return (EAI_NONAME);
    }
    return (0);
}

/* myaddrinfo_control - fine control */

void    myaddrinfo_control(int name,...)
{
    const char *myname = "myaddrinfo_control";
    va_list ap;

    for (va_start(ap, name); name != 0; name = va_arg(ap, int)) {
	switch (name) {
	default:
	    msg_panic("%s: bad name %d", myname, name);
	}
    }
    va_end(ap);
}

#ifdef EMULATE_IPV4_ADDRINFO

/* freeaddrinfo - release storage */

void    freeaddrinfo(struct addrinfo * ai)
{
    struct addrinfo *ap;
    struct addrinfo *next;

    /*
     * Artefact of implementation: tolerate a null pointer argument.
     */
    for (ap = ai; ap != 0; ap = next) {
	next = ap->ai_next;
	if (ap->ai_canonname)
	    myfree(ap->ai_canonname);
	/* ap->ai_addr is allocated within this memory block */
	myfree((char *) ap);
    }
}

static char *ai_errlist[] = {
    "Success",
    "Address family for hostname not supported",	/* EAI_ADDRFAMILY */
    "Temporary failure in name resolution",	/* EAI_AGAIN	 */
    "Invalid value for ai_flags",	/* EAI_BADFLAGS   */
    "Non-recoverable failure in name resolution",	/* EAI_FAIL	 */
    "ai_family not supported",		/* EAI_FAMILY     */
    "Memory allocation failure",	/* EAI_MEMORY     */
    "No address associated with hostname",	/* EAI_NODATA     */
    "hostname nor servname provided, or not known",	/* EAI_NONAME     */
    "service name not supported for ai_socktype",	/* EAI_SERVICE    */
    "ai_socktype not supported",	/* EAI_SOCKTYPE   */
    "System error returned in errno",	/* EAI_SYSTEM     */
    "Invalid value for hints",		/* EAI_BADHINTS   */
    "Resolved protocol is unknown",	/* EAI_PROTOCOL   */
    "Unknown error",			/* EAI_MAX	  */
};

/* gai_strerror - error number to string */

char   *gai_strerror(int ecode)
{

    /*
     * Note: EAI_SYSTEM errors are not automatically handed over to
     * strerror(). The application decides.
     */
    if (ecode < 0 || ecode > EAI_MAX)
	ecode = EAI_MAX;
    return (ai_errlist[ecode]);
}

#endif

#ifdef TEST

 /*
  * A test program that takes some info from the command line and runs it
  * forward and backward through the above conversion routines.
  */
#include <msg.h>
#include <vstream.h>
#include <msg_vstream.h>

int     main(int argc, char **argv)
{
    struct addrinfo *info;
    struct addrinfo *ip;
    MAI_HOSTNAME_STR host;
    MAI_HOSTADDR_STR addr;
    int     err;

    msg_vstream_init(argv[0], VSTREAM_ERR);

    if (argc != 4)
	msg_fatal("usage: %s protocols hostname hostaddress", argv[0]);

    inet_proto_init(argv[0], argv[1]);

    msg_info("=== hostname %s ===", argv[2]);

    if ((err = hostname_to_sockaddr(argv[2], (char *) 0, 0, &info)) != 0) {
	msg_info("hostname_to_sockaddr(%s): %s",
	  argv[2], err == EAI_SYSTEM ? strerror(errno) : gai_strerror(err));
    } else {
	for (ip = info; ip != 0; ip = ip->ai_next) {
	    if ((err = sockaddr_to_hostaddr(ip->ai_addr, ip->ai_addrlen, &addr,
					 (MAI_SERVPORT_STR *) 0, 0)) != 0) {
		msg_info("sockaddr_to_hostaddr: %s",
		   err == EAI_SYSTEM ? strerror(errno) : gai_strerror(err));
		continue;
	    }
	    msg_info("%s -> family=%d sock=%d proto=%d %s", argv[2],
		 ip->ai_family, ip->ai_socktype, ip->ai_protocol, addr.buf);
	    if ((err = sockaddr_to_hostname(ip->ai_addr, ip->ai_addrlen, &host,
					 (MAI_SERVNAME_STR *) 0, 0)) != 0) {
		msg_info("sockaddr_to_hostname: %s",
		   err == EAI_SYSTEM ? strerror(errno) : gai_strerror(err));
		continue;
	    }
	    msg_info("%s -> %s", addr.buf, host.buf);
	}
	freeaddrinfo(info);
    }

    msg_info("=== host address %s ===", argv[3]);

    if ((err = hostaddr_to_sockaddr(argv[3], (char *) 0, 0, &ip)) != 0) {
	msg_info("hostaddr_to_sockaddr(%s): %s",
	  argv[3], err == EAI_SYSTEM ? strerror(errno) : gai_strerror(err));
    } else {
	if ((err = sockaddr_to_hostaddr(ip->ai_addr, ip->ai_addrlen, &addr,
					(MAI_SERVPORT_STR *) 0, 0)) != 0) {
	    msg_info("sockaddr_to_hostaddr: %s",
		   err == EAI_SYSTEM ? strerror(errno) : gai_strerror(err));
	} else {
	    msg_info("%s -> family=%d sock=%d proto=%d %s", argv[3],
		 ip->ai_family, ip->ai_socktype, ip->ai_protocol, addr.buf);
	    if ((err = sockaddr_to_hostname(ip->ai_addr, ip->ai_addrlen, &host,
					 (MAI_SERVNAME_STR *) 0, 0)) != 0) {
		msg_info("sockaddr_to_hostname: %s",
		   err == EAI_SYSTEM ? strerror(errno) : gai_strerror(err));
	    } else
		msg_info("%s -> %s", addr.buf, host.buf);
	    freeaddrinfo(ip);
	}
    }
    exit(0);
}

#endif
