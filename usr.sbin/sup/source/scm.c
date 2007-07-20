/*	$NetBSD: scm.c,v 1.24 2007/07/20 16:39:05 christos Exp $	*/

/*
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * SUP Communication Module for 4.3 BSD
 *
 * SUP COMMUNICATION MODULE SPECIFICATIONS:
 *
 * IN THIS MODULE:
 *
 * CONNECTION ROUTINES
 *
 *   FOR SERVER
 *	servicesetup (port)	establish TCP port connection
 *	  char *port;			name of service
 *	service ()		accept TCP port connection
 *	servicekill ()		close TCP port in use by another process
 *	serviceprep ()		close temp ports used to make connection
 *	serviceend ()		close TCP port
 *
 *   FOR CLIENT
 *	request (port,hostname,retry) establish TCP port connection
 *	  char *port,*hostname;		  name of service and host
 *	  int retry;			  true if retries should be used
 *	requestend ()		close TCP port
 *
 * HOST NAME CHECKING
 *	p = remotehost ()	remote host name (if known)
 *	  char *p;
 *	i = samehost ()		whether remote host is also this host
 *	  int i;
 *	i = matchhost (name)	whether remote host is same as name
 *	  int i;
 *	  char *name;
 *
 * RETURN CODES
 *	All procedures return values as indicated above.  Other routines
 *	normally return SCMOK on success, SCMERR on error.
 *
 * COMMUNICATION PROTOCOL
 *
 *	Described in scmio.c.
 *
 **********************************************************************
 * HISTORY
 *  2-Oct-92  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added conditional declarations of INADDR_NONE and INADDR_LOOPBACK
 *	since Tahoe version of <netinet/in.h> does not define them.
 *
 * Revision 1.13  92/08/11  12:05:35  mrt
 * 	Added changes from stump:
 * 	  Allow for multiple interfaces, and for numeric addresses.
 * 	  Changed to use builtin port for the "supfiledbg"
 * 	    service when getservbyname() cannot find it.
 * 	  Added forward static declatations, delinted.
 * 	  Updated variable argument usage.
 * 	[92/08/08            mrt]
 *
 * Revision 1.12  92/02/08  19:01:11  mja
 * 	Add (struct sockaddr *) casts for HC 2.1.
 * 	[92/02/08  18:59:09  mja]
 *
 * Revision 1.11  89/08/03  19:49:03  mja
 * 	Updated to use v*printf() in place of _doprnt().
 * 	[89/04/19            mja]
 *
 * 11-Feb-88  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Moved sleep into computeBackoff, renamed to dobackoff.
 *
 * 10-Feb-88  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added timeout to backoff.
 *
 * 27-Dec-87  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Removed nameserver support.
 *
 * 09-Sep-87  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Fixed to depend less upon having name of remote host.
 *
 * 25-May-87  Doug Philips (dwp) at Carnegie-Mellon Universtiy
 *	Extracted backoff/sleeptime computation from "request" and
 *	created "computeBackoff" so that I could use it in sup.c when
 *	trying to get to nameservers as a group.
 *
 * 21-May-87  Chriss Stephens (chriss) at Carnegie Mellon University
 *	Merged divergent CS and EE versions.
 *
 * 02-May-87  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added some bullet-proofing code around hostname calls.
 *
 * 31-Mar-87  Dan Nydick (dan) at Carnegie-Mellon University
 *	Fixed for 4.3.
 *
 * 30-May-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to use known values for well-known ports if they are
 *	not found in the host table.
 *
 * 19-Feb-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed setsockopt SO_REUSEADDR to be non-fatal.  Added fourth
 *	parameter as described in 4.3 manual entry.
 *
 * 15-Feb-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added call of readflush() to requestend() routine.
 *
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Major rewrite for protocol version 4.  All read/write and crypt
 *	routines are now in scmio.c.
 *
 * 14-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added setsockopt SO_REUSEADDR call.
 *
 * 01-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Removed code to "gracefully" handle unexpected messages.  This
 *	seems reasonable since it didn't work anyway, and should be
 *	handled at a higher level anyway by adhering to protocol version
 *	number conventions.
 *
 * 26-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Fixed scm.c to free space for remote host name when connection
 *	is closed.
 *
 * 07-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Fixed 4.2 retry code to reload sin values before retry.
 *
 * 22-Oct-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to retry initial connection open request.
 *
 * 22-Sep-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Merged 4.1 and 4.2 versions together.
 *
 * 21-Sep-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Add close() calls after pipe() call.
 *
 * 12-Jun-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Converted for 4.2 sockets; added serviceprep() routine.
 *
 * 04-Jun-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created for 4.2 BSD.
 *
 **********************************************************************
 */

#include "libc.h"
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <stdarg.h>
#if !defined(__linux__)
#if !defined(__CYGWIN__)
#include <ifaddrs.h>
#else
#include "ifaddrs.h"
#endif
#else
#include <sys/ioctl.h>
#endif
#ifdef __CYGWIN__
#include "getaddrinfo.h"
#endif
#include "supcdefs.h"
#include "supextern.h"

#ifndef INADDR_NONE
#define	INADDR_NONE		0xffffffff	/* -1 return */
#endif
#ifndef INADDR_LOOPBACK
#define	INADDR_LOOPBACK		(u_long)0x7f000001	/* 127.0.0.1 */
#endif

char scmversion[] = "4.3 BSD";
extern int silent;

/*************************
 ***    M A C R O S    ***
 *************************/

/* networking parameters */
#define NCONNECTS 5

/*********************************************
 ***    G L O B A L   V A R I A B L E S    ***
 *********************************************/

extern char program[];		/* name of program we are running */
extern int progpid;		/* process id to display */

int netfile = -1;		/* network file descriptor */

static int sock = -1;		/* socket used to make connection */
static struct sockaddr_storage remoteaddr;	/* remote host address */
static char *remotename = NULL;	/* remote host name */
static int swapmode;		/* byte-swapping needed on server? */


static char *myhost(void);

/***************************************************
 ***    C O N N E C T I O N   R O U T I N E S    ***
 ***    F O R   S E R V E R                      ***
 ***************************************************/

int
servicesetup(char *server, int af)
{				/* listen for clients */
	struct addrinfo hints, *res0, *res;
	char port[NI_MAXSERV];
	int error;
	const char *cause = "unknown";
	int one = 1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = af;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	error = getaddrinfo(NULL, server, &hints, &res0);
	if (error) {
		/* retry with precompiled knowledge */
		if (strcmp(server, FILEPORT) == 0)
			snprintf(port, sizeof(port), "%u", FILEPORTNUM);
		else if (strcmp(server, DEBUGFPORT) == 0)
			snprintf(port, sizeof(port), "%u", DEBUGFPORTNUM);
		else
			port[0] = '\0';
		if (port[0])
			error = getaddrinfo(NULL, port, &hints, &res0);
		if (error)
			return (scmerr(-1, "%s: %s", server,
				gai_strerror(error)));
	}
	for (res = res0; res; res = res->ai_next) {
		sock = socket(res->ai_family, res->ai_socktype,
		    res->ai_protocol);
		if (sock < 0) {
			cause = "socket";
			continue;
		}
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			(char *) &one, sizeof(int)) < 0) {
			cause = "setsockopt(SO_REUSEADDR)";
			close(sock);
			continue;
		}
		if (bind(sock, res->ai_addr, res->ai_addrlen) < 0) {
			cause = "bind";
			close(sock);
			continue;
		}
		if (listen(sock, NCONNECTS) < 0) {
			cause = "listen";
			close(sock);
			continue;
		}
		freeaddrinfo(res0);
		return SCMOK;
	}

	freeaddrinfo(res0);
	return (scmerr(errno, "%s", cause));
}

int
service(void)
{
	struct sockaddr_storage from;
	int x;
	socklen_t len;

	remotename = NULL;
	len = sizeof(from);
	do {
		netfile = accept(sock, (struct sockaddr *) & from, &len);
	} while (netfile < 0 && errno == EINTR);
	if (netfile < 0)
		return (scmerr(errno, "Can't accept connections"));
	if (len > sizeof(remoteaddr)) {
		close(netfile);
		return (scmerr(errno, "Can't accept connections"));
	}
	memcpy(&remoteaddr, &from, len);
	if (read(netfile, (char *) &x, sizeof(int)) != sizeof(int))
		return (scmerr(errno, "Can't transmit data on connection"));
	if (x == 0x01020304)
		swapmode = 0;
	else if (x == 0x04030201)
		swapmode = 1;
	else
		return (scmerr(-1, "Unexpected byteswap mode %x", x));
	return (SCMOK);
}

int
serviceprep(void)
{				/* kill temp socket in daemon */
	if (sock >= 0) {
		(void) close(sock);
		sock = -1;
	}
	return (SCMOK);
}

int
servicekill(void)
{				/* kill net file in daemon's parent */
	if (netfile >= 0) {
		(void) close(netfile);
		netfile = -1;
	}
	if (remotename) {
		free(remotename);
		remotename = NULL;
	}
	return (SCMOK);
}

int
serviceend(void)
{				/* kill net file after use in daemon */
	if (netfile >= 0) {
		(void) close(netfile);
		netfile = -1;
	}
	if (remotename) {
		free(remotename);
		remotename = NULL;
	}
	return (SCMOK);
}
/***************************************************
 ***    C O N N E C T I O N   R O U T I N E S    ***
 ***    F O R   C L I E N T                      ***
 ***************************************************/

int 
dobackoff(int *t, int *b)
{
	struct timeval tt;
	unsigned s;

	if (*t == 0)
		return (0);
	s = *b * 30;
	if (gettimeofday(&tt, (struct timezone *) NULL) >= 0)
		s += (tt.tv_usec >> 8) % s;
	if (*b < 32)
		*b <<= 1;
	if (*t != -1) {
		if (s > *t)
			s = *t;
		*t -= s;
	}
	if (!silent)
		(void) scmerr(-1, "Will retry in %d seconds", s);
	sleep(s);
	return (1);
}

int
request(char *server, char *hostname, int *retry)
{				/* connect to server */
	struct addrinfo hints, *res, *res0;
	int error;
	char port[NI_MAXSERV];
	int backoff;
	int x;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo(hostname, server, &hints, &res0);
	if (error) {
		/* retry with precompiled knowledge */
		if (strcmp(server, FILEPORT) == 0)
			snprintf(port, sizeof(port), "%u", FILEPORTNUM);
		else if (strcmp(server, DEBUGFPORT) == 0)
			snprintf(port, sizeof(port), "%u", DEBUGFPORTNUM);
		else
			port[0] = '\0';
		if (port[0])
			error = getaddrinfo(hostname, port, &hints, &res0);
		if (error)
			return (scmerr(-1, "%s: %s", server,
				gai_strerror(error)));
	}
	backoff = 1;
	while (1) {
		netfile = -1;
		for (res = res0; res; res = res->ai_next) {
			if (res->ai_addrlen > sizeof(remoteaddr))
				continue;
			netfile = socket(res->ai_family, res->ai_socktype,
			    res->ai_protocol);
			if (netfile < 0)
				continue;
			if (connect(netfile, res->ai_addr, res->ai_addrlen) < 0) {
				close(netfile);
				netfile = -1;
				continue;
			}
			break;
		}

		if (netfile < 0) {
			if (!dobackoff(retry, &backoff)) {
				freeaddrinfo(res0);
				return (SCMERR);
			}
			continue;
		} else
			break;
	}

	if (res == NULL) {
		freeaddrinfo(res0);
		return (SCMERR);
	}
	memcpy(&remoteaddr, res->ai_addr, res->ai_addrlen);
	remotename = estrdup(hostname);
	x = 0x01020304;
	(void) write(netfile, (char *) &x, sizeof(int));
	swapmode = 0;		/* swap only on server, not client */
	freeaddrinfo(res0);
	return (SCMOK);
}

int
requestend(void)
{				/* end connection to server */
	(void) readflush();
	if (netfile >= 0) {
		(void) close(netfile);
		netfile = -1;
	}
	if (remotename) {
		free(remotename);
		remotename = NULL;
	}
	return (SCMOK);
}
/*************************************************
 ***    H O S T   N A M E   C H E C K I N G    ***
 *************************************************/

static char *
myhost(void)
{				/* find my host name */
	struct hostent *h;
	static char name[MAXHOSTNAMELEN + 1];

	if (name[0] == '\0') {
		if (gethostname(name, sizeof name) < 0)
			return (NULL);
		name[sizeof(name) - 1] = '\0';
		if ((h = gethostbyname(name)) == NULL)
			return (NULL);
		(void) strcpy(name, h->h_name);
	}
	return (name);
}

char *
remotehost(void)
{				/* remote host name (if known) */
	char h1[NI_MAXHOST];

	if (remotename == NULL) {
		if (getnameinfo((struct sockaddr *) & remoteaddr,
#ifdef BSD4_4
			remoteaddr.ss_len,
#else
			sizeof(struct sockaddr),
#endif
			h1, sizeof(h1), NULL, 0, 0))
			return ("UNKNOWN");
		remotename = estrdup(h1);
		if (remotename == NULL)
			return ("UNKNOWN");
	}
	return (remotename);
}

int 
thishost(char *host)
{
	struct hostent *h;
	char *name;

	if ((name = myhost()) == NULL)
		logquit(1, "Can't find my host entry '%s'", myhost());
	h = gethostbyname(host);
	if (h == NULL)
		return (0);
	return (strcasecmp(name, h->h_name) == 0);
}

#ifdef __linux__
/* Nice and sleazy does it... */
struct ifaddrs {
	struct ifaddrs *ifa_next;	
	struct sockaddr *ifa_addr;
	struct sockaddr ifa_addrspace;
};

static int
getifaddrs(struct ifaddrs **ifap)
{
	struct ifaddrs *ifa;
	int nint;
	int n;
	char buf[10 * 1024];
	struct ifconf ifc;
	struct ifreq *ifr;
	int s;

	if ((s = socket (AF_INET, SOCK_DGRAM, 0)) == -1)
		return -1;

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;

	if (ioctl(s, SIOCGIFCONF, &ifc) == -1) {
		(void)close(s);
		return -1;
	}

	(void)close(s);

	if ((nint = ifc.ifc_len / sizeof(struct ifreq)) <= 0)
		return 0;

	if ((ifa = malloc((unsigned)nint * sizeof(struct ifaddrs))) == NULL)
		return -1;

	for (ifr = ifc.ifc_req, n = 0; n < nint; n++, ifr++) {
		ifa[n].ifa_next = &ifa[n + 1];
		ifa[n].ifa_addr = &ifa[n].ifa_addrspace;
		(void)memcpy(ifa[n].ifa_addr, &ifr->ifr_addr,
		    sizeof(*ifa[n].ifa_addr));
	}

	ifa[nint - 1].ifa_next = NULL;
	*ifap = ifa;
	return nint;
}

static void
freeifaddrs(struct ifaddrs *ifa)
{
	free(ifa);
}
	
#endif

int 
samehost(void)
{				/* is remote host same as local host? */
	struct ifaddrs *ifap, *ifa;
	char h1[NI_MAXHOST], h2[NI_MAXHOST];
	const int niflags = NI_NUMERICHOST;

	if (getnameinfo((struct sockaddr *) &remoteaddr,
#ifdef BSD4_4
	    remoteaddr.ss_len,
#else
	    sizeof(struct sockaddr),
#endif
	    h1, sizeof(h1), NULL, 0, niflags))
		return (0);
	if (getifaddrs(&ifap) < 0)
		return (0);
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (remoteaddr.ss_family != ifa->ifa_addr->sa_family)
			continue;
		if (getnameinfo(ifa->ifa_addr,
#ifdef BSD4_4
		    ifa->ifa_addr->sa_len,
#else
		    sizeof(struct sockaddr),
#endif
		    h2, sizeof(h2), NULL, 0, niflags))
			continue;
		if (strcmp(h1, h2) == 0) {
			freeifaddrs(ifap);
			return (1);
		}
	}
	freeifaddrs(ifap);
	return (0);
}

int 
matchhost(char *name)
{				/* is this name of remote host? */
	char h1[NI_MAXHOST], h2[NI_MAXHOST];
	const int niflags = NI_NUMERICHOST;
	struct addrinfo hints, *res0, *res;

	if (getnameinfo((struct sockaddr *) & remoteaddr,
#ifdef BSD4_4
	    remoteaddr.ss_len,
#else
	    sizeof(struct sockaddr),
#endif
	    h1, sizeof(h1), NULL, 0, niflags))
		return (0);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;	/* dummy */
	if (getaddrinfo(name, "0", &hints, &res0) != 0)
		return (0);
	for (res = res0; res; res = res->ai_next) {
		if (remoteaddr.ss_family != res->ai_family)
			continue;
		if (getnameinfo(res->ai_addr, res->ai_addrlen,
		    h2, sizeof(h2), NULL, 0, niflags))
			continue;
		if (strcmp(h1, h2) == 0) {
			freeaddrinfo(res0);
			return (1);
		}
	}
	freeaddrinfo(res0);
	return (0);
}

int 
scmerr(int error, char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	(void) fflush(stdout);
	if (progpid > 0)
		fprintf(stderr, "%s %d: ", program, progpid);
	else
		fprintf(stderr, "%s: ", program);

	vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (error >= 0)
		fprintf(stderr, ": %s\n", errmsg(error));
	else
		fprintf(stderr, "\n");
	(void) fflush(stderr);
	return (SCMERR);
}
/*******************************************************
 ***    I N T E G E R   B Y T E - S W A P P I N G    ***
 *******************************************************/

union intchar {
	int ui;
	char uc[sizeof(int)];
};

int 
byteswap(int in)
{
	union intchar x, y;
	int ix, iy;

	if (swapmode == 0)
		return (in);
	x.ui = in;
	iy = sizeof(int);
	for (ix = 0; ix < sizeof(int); ix++) {
		--iy;
		y.uc[iy] = x.uc[ix];
	}
	return (y.ui);
}
